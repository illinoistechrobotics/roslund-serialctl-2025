#include "serio.h"
#include "base64.h"
#include "packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct connection {
    HANDLE fd;
    DCB    dcb;
    char   device[160];
    char   ba[RECVBUF];
    int    bidx;
};


static const char *win_err_hint(DWORD err)
{
    switch (err) {
    case ERROR_FILE_NOT_FOUND:
        return "no such port check the number in Device Manager";
    case ERROR_ACCESS_DENIED:
        return "port is already open close any other terminal, or "
                "a previous run that did not exit cleanly";
    case ERROR_SEM_TIMEOUT:
        return "device did not respond is the Bluetooth link up?";
    default:
        return "unexpected error";
    }
}

connection_t *serio_open(const char *device)
{
    struct connection *ctx;
    COMMTIMEOUTS timeouts;
    DWORD err;

    if (device == NULL){
        return NULL;
    }

    ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL){
        return NULL;
    }

    ctx->fd = INVALID_HANDLE_VALUE;

    if (device[0] == '\\'){
        snprintf(ctx->device, sizeof(ctx->device), "%s", device);
    }
    else{
        snprintf(ctx->device, sizeof(ctx->device), "\\\\.\\%s", device);
    }
    
    ctx->fd = CreateFileA(
        ctx->device,
        GENERIC_READ | GENERIC_WRITE,
        0,      //no sharing
        NULL,   //default security
        OPEN_EXISTING,  //required for devices
        0,  //synchronous
        NULL       //no template 
    );                  

    if (ctx->fd == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        fprintf(stderr, "failed to open port %s\n  (error %lu, %s)\n",
                ctx->device, (unsigned long)err, win_err_hint(err));
        free(ctx);
        return NULL;
    }

    ctx->dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(ctx->fd, &ctx->dcb)) {
        err = GetLastError();
        fprintf(stderr, "GetCommState failed on %s (error %lu)\n, is this actually a serial port?\n",
                ctx->device, (unsigned long)err);
        CloseHandle(ctx->fd);
        free(ctx);
        return NULL;
    }

    ctx->dcb.BaudRate = CBR_57600;
    ctx->dcb.ByteSize = 8;
    ctx->dcb.Parity = NOPARITY;
    ctx->dcb.StopBits = ONESTOPBIT;

    ctx->dcb.fBinary = TRUE;

    ctx->dcb.fOutxCtsFlow = FALSE;
    ctx->dcb.fOutxDsrFlow = FALSE;
    ctx->dcb.fOutX = FALSE;
    ctx->dcb.fInX = FALSE;

    ctx->dcb.fDtrControl = DTR_CONTROL_ENABLE;
    ctx->dcb.fRtsControl = RTS_CONTROL_ENABLE;

    ctx->dcb.fAbortOnError = FALSE;

    ctx->dcb.fParity = FALSE;
    ctx->dcb.fErrorChar = FALSE;
    ctx->dcb.fNull = FALSE;

    if (!SetCommState(ctx->fd, &ctx->dcb)) {
        err = GetLastError();
        fprintf(stderr, "SetCommState failed on %s (error %lu)\n",
                ctx->device, (unsigned long)err);
        CloseHandle(ctx->fd);
        free(ctx);
        return NULL;
    }

    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(ctx->fd, &timeouts)) {
        err = GetLastError();
        fprintf(stderr, "SetCommTimeouts failed on %s (error %lu)\n",
                ctx->device, (unsigned long)err);
        CloseHandle(ctx->fd);
        free(ctx);
        return NULL;
    }

    PurgeComm(ctx->fd, PURGE_RXCLEAR | PURGE_TXCLEAR);

    printf("serial port opened: %s\n", ctx->device);
    return ctx;
}


void serio_close(connection_t *ctx)
{
    if (ctx == NULL){
       return;
    }

    if (ctx->fd != INVALID_HANDLE_VALUE){
       CloseHandle(ctx->fd);   //replaces close()
    }
    free(ctx);
}


int serio_send(connection_t *ctx, const void *data, size_t len)
{
    char datap[2 + B64_ENC_LEN(sizeof(packet_t))];
    int rv;
    DWORD written = 0;

    if (ctx == NULL || ctx->fd == INVALID_HANDLE_VALUE ||
        len == 0 || data == NULL){
        return -3;
    }

    if (len > sizeof(packet_t)){
       return -3;
    }

    datap[0] = SFRAME;
    rv = base64_encode(datap + 1, (char *)data, (int)len);

    if (rv != (int)B64_ENC_LEN(len)){
       return -2;
    }

    datap[1 + rv] = EFRAME;

    if (!WriteFile(ctx->fd, datap, (DWORD)(2 + rv), &written, NULL)){
       return -1;
    }

    return (int)written;
}


int serio_recv(connection_t *ctx, char *buf, size_t buflen)
{
    char *next;
    size_t linelen;
    DWORD n;

    if (ctx == NULL || ctx->fd == INVALID_HANDLE_VALUE ||
        buf == NULL || buflen == 0){
        return -1;
    }

    do {
        if (ctx->bidx >= RECVBUF - 1) {
            ctx->bidx = 0;
            memset(ctx->ba, 0, RECVBUF);
            fprintf(stderr, "Input buffer overflow!\n");
        }

        n = 0;
        if (!ReadFile(ctx->fd, ctx->ba + ctx->bidx,
            (DWORD)(RECVBUF - (ctx->bidx + 1)), &n, NULL))
                return -1;  //read error

        if (n == 0) {
            Sleep(5);
            continue;
        }

        ctx->bidx += (int)n;
        ctx->ba[ctx->bidx] = '\0';

    }  while(strchr(ctx->ba,'\n') == NULL);
        buf[0]='\0';
        next = strchr(ctx->ba,'\n');
        *next = '\0';
        next++;
        if(ctx->ba[0] != '$'){
                linelen = strlen(ctx->ba);
                if (linelen > buflen - 1){
                        linelen = buflen - 1;
                }
                memcpy(buf, ctx->ba, linelen);
                buf[linelen]='\0';
        }
        //Move rest including null to front
        memmove(ctx->ba,next,strlen(next)+1);
        //update length
        ctx->bidx = (int)strlen(ctx->ba);
        return (int)strlen(buf);
}
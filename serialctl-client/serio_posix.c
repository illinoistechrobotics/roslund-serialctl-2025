#include "serio.h"
#include "base64.h"
#include "packet.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct connection{
        int fd;
        struct termios spconfig;
        char device[128];
        char ba[RECVBUF];       //receive accumulator
        int bidx;       //bytes currently in ba
};

connection_t *serio_open(const char *device){

        struct conection * ctx;
        int opts;

        if (device == NULL){
                return NULL;
        }

        ctx = calloc(1, sizeof(*ctx));
        if (ctx == NULL){
                return NULL;
        }

        snprintf(ctx->device, sizeof(ctx->device), "%s", device);

        ctx->fd = open(ctx->device, O_RDWR | O_NOCTTY | O_NDELAY);
        if (ctx->fd == -1 || !isatty(ctx->fd) || tcgetattr(ctx->fd, &ctx->spconfig) < 0){
                fprintf(stderr, "failed to open port %s\n", ctx->device);
                
                if (ctx-> fd != -1){
                        close(ctx->fd);
                }
                free(ctx);
                return NULL;
        }

        // 8N1
        (ctx->spconfig).c_cflag &= ~PARENB;
        (ctx->spconfig).c_cflag &= ~CSTOPB;
        (ctx->spconfig).c_cflag &= ~CSIZE;
        (ctx->spconfig).c_cflag |= CS8;
        // no flow control
        (ctx->spconfig).c_cflag &= ~CRTSCTS;

        (ctx->spconfig).c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
        (ctx->spconfig).c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

        cfmakeraw(&(ctx->spconfig)); // make raw

        // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
        (ctx->spconfig).c_cc[VMIN]  = 1;
        (ctx->spconfig).c_cc[VTIME]  = 0;

        cfsetispeed(&(ctx->spconfig), B57600);
        cfsetospeed(&(ctx->spconfig), B57600);
        if (tcsetattr(ctx->fd,TCSANOW,&(ctx->spconfig)) < 0){

                perror ("tcsetattr");
                close(ctx->fd);
                free(ctx);
                return NULL;
        }


        opts = fcntl(ctx->fd,F_GETFL);
        if (opts < 0) {
                perror("fcntl(F_GETFL)");
                close(ctx->fd);
                free(ctx);
                return NULL;
        }
        opts = (opts & ~(O_NDELAY));
        if (fcntl(ctx->fd,F_SETFL,opts) < 0) {
                perror("fcntl(F_SETFL)");
                close(ctx->fd);
                free(ctx);
                return NULL;
        }
        printf("serial port opened\n");
        tcflush(ctx->fd,TCIOFLUSH);
        return ctx;
}
void serio_close(connection_t *ctx){

        if (ctx == NULL){
                return;
        }
        if (ctx->fd != -1){
                close(ctx->fd);
        }
        free(ctx);
}

ssize_t serio_send(connection_t *ctx, const void *data, size_t len){
        
        char datap[2 + B64_ENC_LEN(sizeof(packet_t))];
        
        int rv;
        ssize_t written;
        
        char datap[2+B64_ENC_LEN(len)];
        if(ctx == NULL || ctx->fd==-1 || len==0 || data==NULL){
                return -3;
        }

        if (len > sizeof(packet_t)){
                return -3;
        }

        //Add 2 bytes overhead, encode and send
        datap[0] = SFRAME;
        rv = base64_encode(datap+1,(char *)data,(int)len);
        if((int)B64_ENC_LEN(len) != rv){
                return -2;
        }
        datap[1+rv] = EFRAME;
        written = write(ctx->fd,datap,(size_t)(2 + rv));

        if (written < 0){
                return -1;
        }
        return (int)written;
}

int serio_recv(connection_t *ctx, char *buf, size_t buflen)
{ 
        char *next;
        size_t linelen;
        ssize_t n;

        if (citx == NULL || ctx->fd == -1 || buf == NULL || buflen == 0){
                return -1;
        }





        do { 
                if(ctx->bidx == RECVBUF-1){
                        ctx->bidx=0;
                        fprintf("Input buffer overflow!\n");
                        memset(ctx->ba,0x00,RECVBUF);
                }
                n = read(ctx->fd, ctx->ba+ctx->bidx, (size_t)(RECVBUF-(ctx->bidx+1)));  // read as much as possible
                if( n < 0) return -1;    // couldn't read
                if( n==0 ) {
                        usleep( 5 * 1000 ); // wait 5 msec try again
                        continue;
                }
                ctx->bidx += (int)n;
                ctx->ba[ctx->bidx]='\0';
        } while(strchr(ctx->ba,'\n') == NULL);
        buf[0]='\0';
        next = strchr(ba,'\n');
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


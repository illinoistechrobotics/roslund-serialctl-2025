#ifndef SERIO_H
#define SERIO_H

#include <stddef.h>

#define RECVBUF 256
typedef struct connection connection_t;
connection_t *serio_open(constchar *device);


int serio_init(connection_t *ctx, const char *serdev);
int serio_send(connection_t *ctx, const void *data, size_t len);
ssize_t serio_recv(connection_t *ctx, char *buf, size_t buflen);
void serio_close(connection_t *ctx);

#endif

#include "serio.h"
#include "crc16.h"
#include "joystick.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static uint16_t to_big_endian16(uint16_t v){
    return (uint16_t)((v << 8) | (v >> 8));
}


void packet_crc(packet_t *p){
    p->cksum = to_big_endian16(compute_crc((char *)p,sizeof(packet_t)-sizeof(uint16_t)));
    }

int main(int argc, char ** argv){
        char msg[RECVBUF];
        if(argc != 3){
            printf("Usage: serialctl.exe <serial port> <joystick_num>\n");
            return 3;
            }
        int rc = 0;
        connection_t *c;
        packet_t ctl;
        c = serio_open(argv[1]);
        if (c == NULL){
            return 2;
        }
        if(joystick_init(atoi(argv[2])) != 0){
            serio_close(c);
            return 1;
        }
        if(joystick_wait_safe() != 0){
            serio_close(c);
            joystick_release();
            return 1;
        }
        while(1){
          if(serio_recv(c, msg, sizeof(msg)) < 0){
            printf("Error reading data!\n");
            rc = 2;
            break;
            }
        if(joystick_update(&ctl) != 0){
           rc = 1;
           break;
            }
        packet_crc(&ctl); 
        if(serio_send(c, &ctl, sizeof(packet_t)) < 0){
            printf("Unable to send data!\n");
            rc = 2;
            break;
            }
        printf("LX: %i, LY: %i,RX: %i,RY: %i CRC: %i, Resp: %s\n", ctl.stickLX, ctl.stickLY,ctl.stickRX,ctl.stickRY, ctl.cksum, msg); 
//        usleep(150E3);
        }
        serio_close(c);
        joystick_release();
        return rc;
}

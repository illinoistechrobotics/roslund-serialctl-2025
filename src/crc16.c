//#include <util/crc16.h>

/* http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
 * uint8_t serno[] = { 0x02, 0x1c, 0xb8, 0x01, 0, 0, 0, 0xa2 };
 * int
 * checkcrc(void)
 * {
 * uint8_t crc = 0, i;
 * for (i = 0; i < sizeof serno / sizeof serno[0]; i++)
 * crc = _crc_ibutton_update(crc, serno[i]);
 * return crc; // must be 0
 * 
 */
// C implemtation for computer
#define uint16_t unsigned short

unsigned short crc16(const unsigned char* data_p, int length){
    unsigned char x;
    uint16_t crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}

uint16_t compute_crc(char *data, int len){
        return crc16(data, len);
}


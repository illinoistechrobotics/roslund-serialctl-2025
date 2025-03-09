#pragma once
#include "packet.h"

extern packet_t pA, pB, safe;
extern packet_t *astate, *incoming;
extern comm_state cs;
extern char comm_ok;
extern long last_p;


#include <SerialBT.h>
#define SerComm SerialBT  //Serial port connected to Xbee
#define DIAMOND_LEFT 0
#define DIAMOND_DOWN 1
#define DIAMOND_RIGHT 2
#define DIAMOND_UP 3
#define SHOULDER_TOP_LEFT 4
#define SHOULDER_TOP_RIGHT 5
#define SHOULDER_BOTTOM_LEFT 6
#define SHOULDER_BOTTOM_RIGHT 7
#define SMALL_LEFT 8
#define SMALL_RIGHT 9
//10 and 11 are probably the stick buttons
//but we haven't checked recently
#define DPAD_UP 12
#define DPAD_RIGHT 13
#define DPAD_DOWN 14
#define DPAD_LEFT 15

// pins for motor controller 1 (right)
#define ALI1 0
#define AHI1 1
#define BHI1 2
#define BLI1 3
#define DENABLE1 8
//#define DREADY1 30

// and 2 (left)
#define ALI2 4
#define AHI2 5
#define BHI2 6
#define BLI2 7
#define DENABLE2 9
//#define DREADY2 31

#define try_enable_right(e,VBATT) try_enable_osmc(e,DENABLE1,VBATT,ALI1,BLI1,AHI1,BHI1)
#define try_enable_left(e,VBATT) try_enable_osmc(e,DENABLE2,VBATT,ALI2,BLI2,AHI2,BHI2)
#define drive_right(e,x) drive_osmc(e,DENABLE1,x,0,ALI1,BLI1,AHI1,BHI1)
#define drive_left(e,x) drive_osmc(e,DENABLE2,x,0,ALI2,BLI2,AHI2,BHI2)

#define DEADBAND_HALF_WIDTH 10  // Control input deadband radius
#define FAILTIME 500    //Failsafe timeout in milliseconds
#define DEBUGPRINT(x) SerCommDbg.println(x)
#define SerCommDbg Serial   //Serial port for debugging info

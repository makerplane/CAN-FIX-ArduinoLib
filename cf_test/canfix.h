/*  CAN-FIX Interface library for the Arduino 
 *  Copyright (c) 2013 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CANFIX_H
#define __CANFIX_H

#include "can.h"

#define EE_NODE 0x00
#define EE_BITRATE 0x01

// Node Specific Message Control Codes
#define NSM_ID       0
#define NSM_BITRATE  1
#define NSM_NODE_SET 2
#define NSM_DISABLE  3
#define NSM_ENABLE   4
#define NSM_REPORT   5
#define NSM_STATUS   6
#define NSM_FIRMWARE 7
#define NSM_TWOWAY   8
#define NSM_CONFSET  9
#define NSM_CONFGET  10

#define MODE_BLOCK    0
#define MODE_NONBLOCK 1

#define FCB_ANNUNC    0x01;
#define FCB_QUALITY   0x02;
#define FCB_FAIL      0x04;

class CFParameter
{
  public:
    word type;
    byte node;
    byte index;
    byte fcb;
    byte data[5];
    byte length;
    void setMetaData(byte meta);
    byte getMetaData(void);
    void setFlags(byte flags);
    byte getFlags(void);
};
    
class CanFix 
{
private:
  byte deviceid, fw_version;  //Node identification information
  unsigned long model;  //Model number for node identification
  CAN *can;             // Pointer to can object
  byte writeFrame(CanFrame frame, byte mode);
  //Function Pointers for Callbacks
  void (*report_callback)(void);
  byte (*twoway_callback)(byte, word);
  byte (*config_callback)(word, byte *);
  byte (*query_callback)(word, byte *);
  void (*param_callback)(CFParameter);
  void (*alarm_callback)(byte, word, byte*);
  void (*stream_callback)(byte, byte *, byte);
public:
  CanFix(byte pin, byte device);
  // Main Event Loop - Well sorta loop
  void exec(void);
  // Property Functions
  byte getNodeNumber(void);
  int getBitRate(void);
  void setBitRate(int bitrate);
  void setModel(unsigned long m);
  void setFwVersion(byte v);
  
  // Data Transfer Functions
  void sendStatus(word type, byte *data, byte length);
  void sendParam(CFParameter p);
  void sendAlarm(word type, byte *data, byte length);
  void sendStream(byte channel, byte *data, byte length);

  // Callback function assignments
  void set_report_callback(void (*report_callback)(void));
  void set_twoway_callback(byte (*twoway_callback)(byte, word));
  void set_config_callback(byte (*config_callback)(word, byte *));
  void set_query_callback(byte (*query_callback)(word, byte *));
  void set_param_callback(void (*param_callback)(CFParameter));
  void set_alarm_callback(void (*alarm_callback)(byte, word, byte*));
  void set_stream_callback(void (*stream_callback)(byte, byte *, byte));

  // Utility Functions
  byte checkParameterEnable(word id);
};

#endif  //__CANFIX_H

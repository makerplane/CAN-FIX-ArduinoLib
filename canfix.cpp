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

#include "canfix.h"
#include <EEPROM.h>

void CFParameter::setMetaData(byte meta) {
    fcb &= 0x0F;
    fcb |= (meta << 4);
}

byte CFParameter::getMetaData(void) {
    return fcb >> 4;
}

void CFParameter::setFlags(byte flags) {
    fcb &= 0xF0;
    fcb |= (flags & 0x0F);
}

byte CFParameter::getFlags(void) {
    return fcb & 0x0F;
}


/* Constructor for the CanFix Class.  pin is the slave select
   pin that the CAN controller should use.  device is the
   device type that will be used. */
CanFix::CanFix(byte pin, byte device) {
    byte bitrate;

    deviceid = device;
    model = 0L;
    //model = 0x123456L;
    fw_version = 0;
    report_callback = NULL;
    twoway_callback = NULL;
    config_callback = NULL;
    query_callback = NULL;
    param_callback = NULL;
    alarm_callback = NULL;
    stream_callback = NULL;

    can = new CAN(pin);
    can->sendCommand(CMD_RESET);
    //can->setBitRate(bitrate = getBitRate());
    can->setBitRate(bitrate = 125);
    can->setMode(MODE_NORMAL);
}

/* This is a wrapper for the CAN.writeFrame() function. It attempts
   to write the frame into the transmit buffer.  If the transmit
   buffer is full and mode == MODE_BLOCK then it will spin wait
   until the frame is written and return 0.  If mode == MODE_NONBLOCK
   it will attempt to send the frame and if it is successful it will
   return 0 otherwise it will return non-zero */
byte CanFix::writeFrame(CanFrame frame, byte mode) {
    byte result;

    while(1) {
        result = can->writeFrame(frame);
        if(result == 0) {
            return 0;
        } else {
            if(mode) return 1;
        }
    }
}

void CanFix::parameterEnable(CanFrame frame) {
    byte index, offset, result;
    int x;
    CanFrame rframe; //Response frame

    rframe.eid = 0x00;
    rframe.data[0] = frame.data[0];
    rframe.data[1] = frame.id - NSM_START;
    rframe.id = NSM_START + getNodeNumber();

    x = frame.data[2];
    x |= frame.data[3] << 8;
    index = x/8;
    offset = x%8;
    rframe.length = 3;
    /* Check Range */
    if(x<256 || x>1759) {
        rframe.data[2]=0x01;
        can->writeFrame(rframe);
        return;
    }
    rframe.data[2]=0x00;
    result = EEPROM.read(index);
    if(frame.data[1]==NSM_DISABLE) {
        if(!bitRead(result, offset)) {  //If the bit is clear
            bitSet(result, offset);
            EEPROM.write(index, result);
        }
    } else {  //Enable command - doesn't respond to broadcast
        if(bitRead(result,offset) && frame.data[0]!=0x00) { //If the bit is set
            bitClear(result,offset);
            EEPROM.write(index, result);
        }
    }
    can->writeFrame(rframe);
}

void CanFix::handleNodeSpecific(CanFrame frame) {
    int x;
    byte length;
    CanFrame rframe; //Response frame

    // This prepares a generic response
    rframe.eid = 0x00;
    rframe.data[0] = frame.data[0];
    rframe.data[1] = frame.id - NSM_START;
    rframe.id = NSM_START + getNodeNumber();

    switch(frame.data[0]) {
        case NSM_ID: // Node Identify Message
            if(frame.data[1] == getNodeNumber() || frame.data[0]==0) {
                rframe.data[2] = 0x01;
                rframe.data[3] = deviceid;
                rframe.data[4] = fw_version;
                rframe.data[5] = (byte)model;
                rframe.data[6] = (byte)(model >> 8);
                rframe.data[7] = (byte)(model >> 16);
                rframe.length = 8;
                break;
            } else {
                return;
            }
        case NSM_BITRATE:
            if(frame.data[1] == getNodeNumber() || frame.data[0]==0) {
                if(frame.data[2] == 1) {
                    x = 125;
                } else if(frame.data[2] ==2) {
                    x = 250;
                } else if(frame.data[2] == 3) {
                    x = 500;
                } else if(frame.data[2] == 4) {
                    x = 1000;
                } else {
                    rframe.data[2] = 0x01;
                    rframe.length = 3;
                    can->writeFrame(rframe);
                    return;
                }
                setBitRate(x);
                can->setMode(MODE_CONFIG);
                can->setBitRate(x);
                can->setMode(MODE_NORMAL);
            } else {
                return;
            }
        case NSM_NODE_SET: // Node Set Message
            if(frame.data[1] == getNodeNumber() || frame.data[0]==0) {
                if(frame.data[2] != 0x00) { // Doesn't respond to broadcast
                    if(EEPROM.read(EE_NODE) != frame.data[2]) {
                        EEPROM.write(EE_NODE, frame.data[2]);
                    }
                    // Gotta do this again because we just changed it
                    rframe.id = NSM_START + getNodeNumber();
                    rframe.data[2] = 0x00;
                } else {
                    rframe.data[2] = 0x01;
                }
                rframe.length = 2;
                break;
            } else {
                return;
            }
        /* We use a bitmask in the EEPROM to determine whether or not a
           parameter is disabled.  A 0 in the bit location means enabled
           and a 1 means disabled. */
        case NSM_DISABLE:
            if(frame.data[1]==getNodeNumber() || frame.data[1]==0) {
                parameterEnable(frame);
            }
            return;
        case NSM_ENABLE:
            if(frame.data[1]==getNodeNumber()) {
                parameterEnable(frame);
            }
            return;
        case NSM_REPORT:
            if(frame.data[1]==getNodeNumber() || frame.data[1]==0) {
                if(report_callback) report_callback();
            }
            return;
        case NSM_FIRMWARE: //Not implemented yet
            if(frame.data[1]==getNodeNumber()) {
                ; // Do some stuff
            }
            return;
        case NSM_TWOWAY:
            if(frame.data[1]==getNodeNumber()) {
                if(twoway_callback && frame.data[0]!=0x00) {
                    if(twoway_callback(frame.data[2], *((word *)(&frame.data[3]))) == 0) {
                        rframe.data[2]=0x00;
                        rframe.length = 3;
                        break;
                    }
                }
            } else {
                return;
            }
        case NSM_CONFSET:
            if(frame.data[1]==getNodeNumber()) {
                if(config_callback) {
                    rframe.data[2] = config_callback(*((word *)(&frame.data[2])), &frame.data[4]);
                } else {
                    rframe.data[2] = 1;
                }
                rframe.length = 3;
                break;
            } else {
                return;
            }
        case NSM_CONFGET:
            if(frame.data[1]==getNodeNumber()) {
                if(query_callback) {
                    rframe.data[2] = query_callback(*((word *)(&frame.data[2])), &rframe.data[3], &length);
                } else {
                    rframe.data[2] = 1;
                }
                if(rframe.data[2]==0) { /* Send Success with data */
                    rframe.length = 3+length;
                } else {
                    rframe.length = 3; /* Just send the error */
                }
                break;
            } else {
                return;
            }
        default:
          return;
    }
    can->writeFrame(rframe);
}

void CanFix::handleFrame(CanFrame frame) {
  byte n;
  CFParameter par;

  if(frame.id == 0x00) { /* Ignore ID 0 */
      ;
  } else if(frame.id < 256) { /* Node Alarms */
      if(alarm_callback) {
        alarm_callback(frame.id, *((word *)(&frame.data[0])), &frame.data[2], frame.length-2);
      }
  } else if(frame.id < 0x6E0) { /* Parameters */
      if(param_callback) {
          par.type = frame.id;
          par.node = frame.data[0];
          par.index = frame.data[1];
          par.fcb = frame.data[2];
          par.length = frame.length - 3;
          for(n = 0; n<par.length; n++) par.data[n] = frame.data[3+n];
          param_callback(par);
      }
  } else if(frame.id < 0x7E0) { /* Node Specific Message */
      handleNodeSpecific(frame);
  } else { /* Communication Channel */
      ; /* Not implemented at the moment */
  }
}

/* The exec() function is the heart and soul of this library.  The
   application should call this function in each loop.  It will
   receive the data from the CAN Bus, deal with mandatory protocol
   issues and fire events if certain frames are received. */
void CanFix::exec(void) {
    byte rxstat;
    CanFrame frame;

    rxstat = can->getRxStatus();
    if(rxstat & 0x40) {
        frame = can->readFrame(0);
        handleFrame(frame);
    } else if(rxstat & 0x80) {
        frame = can->readFrame(1);
        handleFrame(frame);
    } else {
        return;
    }

    // TODO Check for CAN errors and buffer overflows
}

int CanFix::getBitRate(void) {
    byte bitrate;
    bitrate = EEPROM.read(EE_BITRATE);
    if(bitrate == BITRATE_125) return 125;
    else if(bitrate == BITRATE_250) return 250;
    else if(bitrate == BITRATE_500) return 500;
    else if(bitrate == BITRATE_1000) return 1000;
    else setBitRate(125);

    return 125;
}

/* This sets the bitrate in the EEPROM.  It does not change the
 actual bitrate in the controller */
void CanFix::setBitRate(int bitrate) {
    byte br = 0;
    if(bitrate==125)  br = BITRATE_125;
    if(bitrate==250)  br = BITRATE_250;
    if(bitrate==500)  br = BITRATE_500;
    if(bitrate==1000) br = BITRATE_1000;
    if(EEPROM.read(EE_BITRATE) != br) {
        EEPROM.write(EE_BITRATE, br);
    }
}

byte CanFix::getNodeNumber(void) {
    byte x;

    x = EEPROM.read(EE_NODE);
    if(x == 0x00) x = deviceid;
    return x;
}

void CanFix::setModel(unsigned long m) {
    model = m;
}

void CanFix::setFwVersion(byte v) {
    fw_version = v;
}

/* Sends a Node Status Information Message.  type is the parameter type,
   *data is the buffer of up to 4 bytes and length is the number of
   bytes in *data */
void CanFix::sendStatus(word type, byte *data, byte length) {
    CanFrame frame;
    byte n;
    frame.id = NSM_START + getNodeNumber();
    frame.eid = 0x00;
    frame.data[0] = NSM_STATUS;
    frame.data[1] = type;
    frame.data[2] = type >> 8;
    for(n=0; n<length; n++) {
        frame.data[3+n] = data[n];
    }
    frame.length = length + 3;
    writeFrame(frame, MODE_BLOCK);
}

void CanFix::sendParam(CFParameter p) {
    byte n;
    CanFrame frame;
    frame.id = p.type;
    frame.eid = 0;
    frame.data[0] = getNodeNumber();
    frame.data[1] = p.index;
    frame.data[2] = p.fcb;
    frame.length = p.length + 3;
    for(n = 0; n<p.length && n<5; n++) frame.data[3+n] = p.data[n];
    writeFrame(frame, MODE_BLOCK);
}

void CanFix::sendAlarm(word type, byte *data, byte length) {
    ;
}

void CanFix::set_report_callback(void (*f)(void)) {
    report_callback = f;
}

void CanFix::set_twoway_callback(byte (*f)(byte, word)) {
    twoway_callback = f;
}

void CanFix::set_config_callback(byte (*f)(word, byte *)) {
    config_callback = f;
}

void CanFix::set_query_callback(byte (*f)(word, byte *, byte *)) {
    query_callback = f;
}

void CanFix::set_param_callback(void (*f)(CFParameter)) {
    param_callback = f;
}

void CanFix::set_alarm_callback(void (*f)(byte, word, byte*, byte)) {
    alarm_callback = f;
}

void CanFix::set_stream_callback(void (*f)(byte, byte *, byte)) {
    stream_callback = f;
}

/* Returns non-zero if the parameter is enabled */
byte CanFix::checkParameterEnable(word id) {
    byte index, offset, result;
    index = id / 8;
    offset = id % 8;
    result = EEPROM.read(index);
    if(bitRead(result, offset)) {
        return 0;
    } else {
        return 1;
    }
}

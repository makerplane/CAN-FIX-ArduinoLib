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

/* Constructor for the CanFix Class.  pin is the slave select
 pin that the CAN controller should use.  device is the 
 device type that will be used. */
CanFix::CanFix(byte pin, byte device)
{
  byte bitrate;
  
  deviceid = device;
  model = 0L;
  //model = 0x123456L;
  fw_version = 0;
  
  can = new CAN(pin);
  can->sendCommand(CMD_RESET);
  can->setBitRate(bitrate = getBitRate());
  can->setMode(MODE_NORMAL);
}

/* The exec() function is the heart and soul of this library.  The
 application should call this function in each loop.  It will
 receive the data from the CAN Bus, deal with mandatory protocol
 issues and fire events if certain parameters are received. */
void CanFix::exec(void)
{
  byte rxstat;
  int x;
  CanFrame frame;
  CanFrame rframe; //Response frame
  
  rframe.eid = 0x00;
  rxstat = can->getRxStatus();
  if(rxstat & 0x40) {
    frame = can->readFrame(0);
  } else if(rxstat & 0x80) {
    frame = can->readFrame(1);
  } else {
    return;
  }
  if(frame.id >= 0x700) {
    // If the first data byte matches our node id or 0
    if(frame.data[0] == getNodeNumber() || frame.data[0]==0) {
      // This prepares a generic response
      rframe.data[0] = frame.id & 0xFF;
      rframe.data[1] = frame.data[1];
      rframe.id = 0x700 + getNodeNumber();
              
      switch(frame.data[1]) {
        case NSM_ID: // Node Identify Message
          rframe.data[2] = 0x01;
          rframe.data[3] = deviceid;
          rframe.data[4] = fw_version;
          rframe.data[5] = (byte)model;
          rframe.data[6] = (byte)(model >> 8);
          rframe.data[7] = (byte)(model >> 16);
          rframe.length = 8;
          break;
        case NSM_BITRATE:
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
          Serial.print("Set Bitrate to ");
          Serial.println(x);
          return;
        case NSM_NODE_SET: // Node Set Message
          if(frame.data[2] != 0x00) {
            EEPROM.write(EE_NODE, frame.data[2]);
            // Gotta do this agin because we just changed it
            rframe.id = 0x700 + getNodeNumber();
            rframe.data[2] = 0x00;
          } else {
            rframe.data[2] = 0x01;
          }
          rframe.length = 2;
          break;
        case NSM_DISABLE:
        case NSM_ENABLE:
        case NSM_REPORT:
        case NSM_STATUS:
        case NSM_FIRMWARE:
        case NSM_TWOWAY:
        case NSM_CONFSET:
        case NSM_CONFGET:
          return;
        default:
          return;
      }
      can->writeFrame(rframe);
          
      Serial.print("Got Message From Node ");
      Serial.println(frame.id & 0x00FF, HEX);
    }
  }
}

int CanFix::getBitRate(void)
{
  byte bitrate;
  bitrate = EEPROM.read(EE_BITRATE);
  if(bitrate == 1) return 125;
  if(bitrate == 2) return 250;
  if(bitrate == 5) return 500;
  if(bitrate == 10) return 1000;
  /* If we get here the value in the EEPROM is bad.
   We'll set a default. */
  setBitRate(125);
}

/* This sets the bitrate in the EEPROM.  It does not change the
 actual bitrate in the controller */
void CanFix::setBitRate(int bitrate)
{
  if(bitrate==125)  EEPROM.write(EE_BITRATE, 1);
  if(bitrate==250)  EEPROM.write(EE_BITRATE, 2);
  if(bitrate==500)  EEPROM.write(EE_BITRATE, 5);
  if(bitrate==1000) EEPROM.write(EE_BITRATE, 10);
}

byte CanFix::getNodeNumber(void)
{
  byte x;

  x = EEPROM.read(EE_NODE);
  if(x == 0x00) x = deviceid;
}

void CanFix::setModel(unsigned long m)
{
  model = m;
}

void CanFix::setFwVersion(byte v)
{
  fw_version = v;
}


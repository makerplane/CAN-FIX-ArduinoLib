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
  model = 0;
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
  CanFrame frame;
  
  rxstat = can->getRxStatus();
  if(rxstat & 0x40) {
    frame = can->readFrame(0);
    Serial.print("RX0 Received Frame, ID = ");
    Serial.println(frame.id, HEX);
  } else if(rxstat & 0x80) {
    frame = can->readFrame(1);
  } else {
    return;
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



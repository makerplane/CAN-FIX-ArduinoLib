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

class CanFix 
{
private:
  byte deviceid, fw_version;
  unsigned long model;
  CAN *can;
public:
  CanFix(byte pin, byte device);
  void exec(void);
  byte getNodeNumber(void);
  int getBitRate(void);
  void setBitRate(int bitrate);

};

#endif  //__CANFIX_H

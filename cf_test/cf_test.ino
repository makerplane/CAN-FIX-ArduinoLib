/*  USB<->CAN Interface for the Arduino 
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

#define CAN_MHZ 20
#include "canfix.h"

#include <Serial.h>

CanFix *cf;

void setup()
{
  Serial.begin(115200);
  cf = new CanFix(10, 0xB0);
  cf->setModel(0x123456);
  cf->setFwVersion(0x01);
  cf->sendStatus(0x00, NULL, 0);
}

void loop()
{
  static unsigned long lasttime[4], now;
  static byte count;
  byte buff[8];
  /* Event Loop function */
  cf->exec();
  now = millis();
  if(now - lasttime[0] > 500) {
    lasttime[0] = now;
  }
  if(now - lasttime[1] > 1000) {
    lasttime[1] = now;
  }
  if(now - lasttime[2] > 2000) {
    lasttime[2] = now;
  }
  if(now - lasttime[3] > 5000) {
    lasttime[3] = now;
    cf->sendStatus(0x00, NULL, 0);
    if((++count % 10) == 0) {
      buff[0]=count;
      buff[1]=buff[0]*2;
      cf->sendStatus(4567, buff, 2);
    }
  }
  
    
    
//  delay(1000);
//  Serial.println(cf->getNodeNumber(), HEX);
}

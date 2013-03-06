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

/* These are our callback function definitions */
void report_callback(void);
byte twoway_callback(byte channel, word type);
byte config_callback(word key, byte *data);
byte query_callback(word key, byte *data);
void param_callback(CFParameter p);
void alarm_callback(word type, byte *data, byte length);
void stream_callback(byte channel, byte *data, byte length);


void setup()
{
  Serial.begin(115200);
  cf = new CanFix(10, 0xB0);
  cf->setModel(0x123456);
  cf->setFwVersion(0x01);
  cf->sendStatus(0x00, NULL, 0);
  cf->set_report_callback(report_callback);
  cf->set_twoway_callback(twoway_callback);
  cf->set_config_callback(config_callback);
  cf->set_query_callback(query_callback);
  cf->set_param_callback(param_callback);
  cf->set_alarm_callback(alarm_callback);
  cf->set_stream_callback(stream_callback);
}

#define TIMER_COUNT 4

void loop()
{
  static unsigned long lasttime[TIMER_COUNT], now;
  static byte count, n;
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
  /* Once every 50 days or so millis() will overflow.  This makes sure
     that we don't lock up. */
  if(now < lasttime[0]) {
    for(n=0;n<TIMER_COUNT;n++) lasttime[n]=0;
  }
}

/* The exec() function will call this callback if a Node Report
   message is received with our node number or the broadcast node
   address, 0x00.  This function should be used to indicate that
   we need to start sending all of our parameters including all
   the meta data associated with them. */
void report_callback(void)
{
  Serial.println("Node Report");
}

/* The exec() function will call this callback if a Two Way
   channel request is recieved with our node number.  It will
   not send it if the node is the broadcast.  If we accept the
   connection we should return 0x00, otherwise return non
   zero and the connection will be ignored. */
byte twoway_callback(byte channel, word type)
{
  Serial.print("Channel = ");
  Serial.print(channel);
  Serial.print(" Type = ");
  Serial.println(type, HEX);
  return 0;
}

/* The exec() function will call this callback if a Node
   Configuration Command is received on the bus. The proper
   error from the specification should be returned if there
   is a problem.  The return value will be sent back to the
   node that sent the request. */
byte config_callback(word key, byte *data)
{
  byte n;
  Serial.print("Config Key = ");
  Serial.println(key, HEX);
  for(n=0;n<4;n++) {
    Serial.print("Data[");
    Serial.print(n);
    Serial.print("]= ");
    Serial.println(data[n],HEX);
  }
  if(key == 512) return 1;
  else return 0;
}

/* The exec() function will call this callback if a Node
   configuration query command is received.  We should
   populate the *data buffer with the proper data and return
   0 if successful.  Return nonzero if the configuration 
   parameter is unknown or read only. */
byte query_callback(word key, byte *data)
{
  Serial.print("Query Key = ");
  Serial.println(key, HEX);
  if(key == 512) return 1;
  else return 0;
}

void param_callback(CFParameter p)
{
  ;
}

void alarm_callback(word type, byte *data)
{
  ;
}

void stream_callback(byte channel, byte *data, byte length)
{
  ;
}



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

#define CAN_MHZ 20
#include "canfix.h"

//#include <Serial.h>

/* The CanFix class represents our CAN-FIX connection */
CanFix *cf;

/* These are our callback function definitions */
void report_callback(void);
byte twoway_callback(byte channel, word type);
byte config_callback(word key, byte *data);
byte query_callback(word key, byte *data);
void param_callback(CFParameter p);
void alarm_callback(byte node, word type, byte *data, byte length);
void stream_callback(byte channel, byte *data, byte length);

int hOffset, vOffset, hMaxPos, hMaxNeg, vMaxPos, vMaxNeg;

void sendPitchAngle(float angle) {
  CFParameter p;
  int x;

  //if(cf->checkParameterEnable(0x180)) { /* Check that it's enabled */
    p.type = 0x180;
    p.index = 0;
    p.fcb = 0x00;
    x = angle*100;
    p.data[0] = x;
    p.data[1] = x>>8;
    p.length = 2;

    cf->sendParam(p);
  //}
}

void sendRollAngle(float angle) {
  CFParameter p;
  int x;

  //if(cf->checkParameterEnable(0x181)) { /* Check that it's enabled */
    p.type = 0x181;
    p.index = 0;
    p.fcb = 0x00;
    x = angle*100;
    p.data[0] = x;
    p.data[1] = x>>8;
    p.length = 2;

    cf->sendParam(p);
  //}
}

void sendHeading(float angle) {
  CFParameter p;
  int x;

  //if(cf->checkParameterEnable(0x185)) { /* Check that it's enabled */
    p.type = 0x185;
    p.index = 0;
    p.fcb = 0x00;
    x = angle*10;
    p.data[0] = x;
    p.data[1] = x>>8;
    p.length = 2;

    cf->sendParam(p);
  //}
}

void sendTurnRate(float rate) {
  CFParameter p;
  int x;

  //if(cf->checkParameterEnable(0x185)) { /* Check that it's enabled */
    p.type = 0x403;
    p.index = 0;
    p.fcb = 0x00;
    x = rate*10;
    p.data[0] = x;
    p.data[1] = x>>8;
    p.length = 2;

    cf->sendParam(p);
  //}
}

void sendPitchControl(int pos) {
  CFParameter p;

  //if(cf->checkParameterEnable(0x185)) { /* Check that it's enabled */
    p.type = 0x11E;
    p.index = 0;
    p.fcb = 0x00;
    p.data[0] = pos;
    p.data[1] = pos>>8;
    p.length = 2;

    cf->sendParam(p);
  //}
}

void sendRollControl(int pos) {
  CFParameter p;

  //if(cf->checkParameterEnable(0x185)) { /* Check that it's enabled */
    p.type = 0x11F;
    p.index = 0;
    p.fcb = 0x00;
    p.data[0] = pos;
    p.data[1] = pos>>8;
    p.length = 2;

    cf->sendParam(p);
  //}
}

void sendButtons(byte button) {
  CFParameter p;

  //if(cf->checkParameterEnable(0x185)) { /* Check that it's enabled */
    p.type = 0x11C;
    p.index = 0;
    p.fcb = 0x00;
    p.data[0] = button;
    p.length = 5;

    cf->sendParam(p);
  //}
}

void setup() {
  Serial.begin(115200);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

    /* The constructor takes the pin that slave select for the MCP2515
     is connected to and our CAN-FIX device type.  The Device type
     will be used as the node id if there is nothing in the EEPROM */
  cf = new CanFix(10, 0xB0);
  /* This sets the model number and firmware version of our device.
     This is what will be returned when the Node ID is requested */
  cf->setModel(0x123456);
  cf->setFwVersion(0x01);
  /*  We send a Node status message of zero to indicate that we are
      up and running and all is okay. */
  cf->sendStatus(0x00, NULL, 0);
  /*  Assign all the callback function for the cf object. */
  cf->set_report_callback(report_callback);
  cf->set_twoway_callback(twoway_callback);
  cf->set_config_callback(config_callback);
  cf->set_query_callback(query_callback);
  cf->set_param_callback(param_callback);
  cf->set_alarm_callback(alarm_callback);
  cf->set_stream_callback(stream_callback);

  hOffset = 512 - analogRead(A0);
  vOffset = 512 - analogRead(A1);
  hMaxPos = 512 - hOffset;
  hMaxNeg = -512 - hOffset+1;
  vMaxPos = 512 - vOffset;
  vMaxNeg = -512 - vOffset+1;
}

#define TIMER_COUNT 5

void loop() {
  static unsigned long lasttime[TIMER_COUNT], now;
  static byte count, n, buttons[4];
  static float pitch, roll, pitchRate, rollRate;
  static float heading, hdgRate;
  byte buff[8], result;
  int hJoyStick, vJoyStick;

  /* read the values from the sensor */
  hJoyStick = 512 - analogRead(A0) - hOffset;
  if(hJoyStick < 0) {
      rollRate = (float)hJoyStick / hMaxNeg * 45.0;
  } else {
      rollRate = (float)hJoyStick / hMaxPos * -45.0;
  }

  vJoyStick = 512 - analogRead(A1) - vOffset;
  if(vJoyStick < 0) {
      pitchRate = (float)vJoyStick / vMaxNeg * -15.0;
  } else {
      pitchRate = (float)vJoyStick / vMaxPos * 15.0;
  }
  if(roll > 0.1 && hJoyStick == 0) {
      rollRate = -2.0;
  } else if(roll < -0.1 && hJoyStick == 0) {
      rollRate = 2.0;
  }
  if(pitch > 0.1 && vJoyStick == 0) {
      pitchRate = -1.0;
  } else if(pitch < -0.1 && vJoyStick == 0) {
      pitchRate = 1.0;
  }
  if(roll > 0.1) {
      hdgRate = 12.0*(min(roll, 90.0) / 90.0);
  } else if (roll < 0.1) {
      hdgRate = 12.0*(max(roll, -90.0) / 90.0);
  }
  if(abs(pitch)<0.2) pitch = 0;
  if(abs(roll)<0.2) roll = 0;

/* Event Dispatch Function - This should be called every time
   through the loop.  It handles all of the communications on
   the bus and calls the callback functions when certain message
   are received. */
  cf->exec();

/* The timing here is simply based on the millis() function.  We
   read the millis function and store several lasttime[] values
   that we can use for comparison.  We could have as many of
   these as we want. */
  now = millis();
  /* Pitch and Roll */
  if(now - lasttime[0] > 100) {
    pitch += pitchRate / 5.0;
    roll += rollRate / 5.0;
    if(pitch > 90.0) pitch = 90.0;
    if(pitch < -90.0) pitch = -90.0;
    sendPitchAngle(pitch);
    if(roll > 180) roll = 180.0;
    if(roll < -180) roll = -180.0;
    sendRollAngle(roll);
    lasttime[0] = now;
  }
  /* Heading */
  if(now - lasttime[1] > 300) {
      heading+=hdgRate / 3.3333333;
      if(heading > 360.0) heading -= 360.0;
      if(heading < 0) heading += 360;
      sendHeading(heading);
      sendTurnRate(hdgRate);
      lasttime[1] = now;
  }
  if(now - lasttime[2] > 100) {
      sendPitchControl(vJoyStick*20);
      sendRollControl(hJoyStick*20);
      lasttime[2] = now;
  }

  /* We are sending a good status every five seconds.  Every 50
     seconds or so we're sending some bogus status message just
     for grins. */
  if(now - lasttime[3] > 5000) {
    lasttime[3] = now;
    cf->sendStatus(0x00, NULL, 0);
    if((++count % 10) == 0) {
      buff[0]=count;
      buff[1]=buff[0]*2;
      cf->sendStatus(4567, buff, 2);
    }
  }
  if(now - lasttime[4] > 30) {
    lasttime[4] = now;
    result = 0; // Use this to decide if we need to send buttons
    buff[0] = digitalRead(4);
    buff[1] = digitalRead(5);
    buff[2] = digitalRead(6);
    buff[3] = digitalRead(3);
    for(n=0; n<4; n++) {
      //Serial.print(buff[n], HEX);
      if(buff[n] != buttons[n]) {
        buttons[n] = buff[n];
        result = 1;
      }
    }
    //Serial.println("");
    if(result) {
      result = 0x00;
      for(n=0;n<4;n++) {
        if(buttons[n]==LOW) {
          result |= 0x01 << n;
        }
      }
      //Serial.print("Buttons ");
      //Serial.println(result, HEX);
      sendButtons(result);
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

/* This function is called from the exec() function if
   a parameter update message is received.  The parameter
   is passed as an object of type CFParameter */
void param_callback(CFParameter p)
{
  byte n;
  /*Serial.print("Parameter ");
  Serial.print(p.type, HEX);
  Serial.print(" From Node ");
  Serial.print(p.node, HEX);
  Serial.print(" Index ");
  Serial.print(p.index, HEX);
  Serial.print(" Function Code ");
  Serial.println(p.fcb, HEX);
  for(n=0; n<p.length; n++) {
     Serial.print(" ");
     Serial.print(p.data[n], HEX);
  }*/
}

/* This function is called from exec() if a node alarm is
   received.  The node address, alarm type and a pointer
   to the data in the message is passed */
void alarm_callback(byte node, word type, byte *data)
{
  Serial.print("Alarm from Node ");
  Serial.print(node, HEX);
  Serial.print(", Type = ");
  Serial.println(type, HEX);
}

/* This function is called from exec() if data is received on
   a channel that we currently have a connection on */
/* This is currently not implemented */
void stream_callback(byte channel, byte *data, byte length)
{
  ;
}

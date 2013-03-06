
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

#include <SPI.h>
#include "can.h"

CAN::CAN(byte pin)
{
  // set the Slave Select Pin as an output:
  ss_pin = pin;
  pinMode (ss_pin, OUTPUT);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  // initialize SPI:
  SPI.begin();
  digitalWrite(ss_pin, HIGH);
}

/* Writes data to the MCP2515 starting at reg.  Writes the
   number of bytes in buff given by count */
void CAN::write(byte reg, byte *buff, byte count)
{
  byte n;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(reg);
  for(n = 0; n < count; n++) {
    SPI.transfer(buff[n]);
  }  
  digitalWrite(ss_pin, HIGH);
}

/* Reads from the MCP2515 starting at reg, writes
   the bytes that are read into buff */
void CAN::read(byte reg, byte *buff, byte count)
{
  byte n;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_READ);
  SPI.transfer(reg);
  for(n = 0; n < count; n++) {
    buff[n] = SPI.transfer(0x00);
  }  
  digitalWrite(ss_pin, HIGH);
}

/* These two functions are to convert back and forth between
   the actual Frame ID and the buffers that are suitable for
   the MCP2515.  The regs buffer should be a four byte buffer
   and it represents the SIDH, SIDL, EID8 and EID0 buffers
   respectively.
   
   This function takes the registers and returns a CanFrame
   with the id and the eid members set accordingly. */
CanFrame CAN::reg2frame(byte *regs)
{
  CanFrame frame;
  
  if(regs[1] & 0x08) {   // Extended Identifier
    frame.id = ((unsigned long)regs[0] << 21);         //SIDH
    frame.id |= ((unsigned long)regs[1] & 0xE0)<<13;   //SIDL
    frame.id |= ((unsigned long)regs[1] & 0x03)<<16;   //SIDL
    frame.id |= ((unsigned long)regs[2]<<8) | (unsigned long)regs[3]; //EID8  EID0
    frame.eid = 0x01;
  } else {
    frame.id = (regs[0]<<3) | (regs[1]>>5);
    frame.eid = 0x00;
  }
  return frame;
}

/* This function requires that frame.id and frame.eid
   both be set properly.  It will fill in 'regs' with
   the four registers. */
void CAN::frame2reg(CanFrame frame, byte *regs)
{
  if(frame.eid) {
    regs[3] = frame.id;        //EID0
    regs[2] = frame.id >>8;    //EID8
    regs[1] = ((frame.id>>16) & 0x03) | ((frame.id>>13) & 0xE0) | 0x08;  //SIDL
    regs[0] = frame.id>>21;    //SIDH
  } else {
    regs[1] = frame.id<<5;     //SIDL
    regs[0] = frame.id>>3;     //SIDH
  }
}


void CAN::sendCommand(byte command)
{
  digitalWrite(ss_pin, LOW);
  SPI.transfer(command);
  digitalWrite(ss_pin, HIGH);
}

void CAN::setBitRate(int bitrate)
{
  byte buff[3];
  switch(bitrate) {
    case 125:
      buff[0] = CNF3_125;
      buff[1] = CNF2_125;
      buff[2] = CNF1_125;
      break;
    case 250:
      buff[0] = CNF3_250;
      buff[1] = CNF2_250;
      buff[2] = CNF1_250;
      break;
    case 500:
      buff[0] = CNF3_500;
      buff[1] = CNF2_500;
      buff[2] = CNF1_500;
      break;
    case 1000:
      buff[0] = CNF3_1000;
      buff[1] = CNF2_1000;
      buff[2] = CNF1_1000;
      break;
    default:
      return;
  }
  write(REG_CNF3, buff, 3);
}

void CAN::setMode(byte mode)
{
  byte buff;
  buff = mode;
  write(REG_CANCTRL, &buff, 1);
}

byte CAN::getMode(void)
{
  byte buff;
  read(REG_CANSTAT, &buff, 1);
  return buff >> 5;
}

byte CAN::getRxStatus(void)
{
  byte result;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_RXSTATUS);
  result = SPI.transfer(0x00);
  digitalWrite(ss_pin, HIGH);
  return result;
}

CanFrame CAN::readFrame(byte rxb)
{
  byte n;
  byte regs[4];
  CanFrame frame;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_READRXB | (rxb<<2));
  regs[0] = SPI.transfer(0x00); //SIDH
  regs[1] = SPI.transfer(0x00); //SIDL
  regs[2] = SPI.transfer(0x00); //EID8
  regs[3] = SPI.transfer(0x00); //EID0
  frame = reg2frame(regs);
  frame.length = SPI.transfer(0x00);  //DLC
  for(n=0; n<frame.length; n++) {
    frame.data[n] = SPI.transfer(0x00);
  }
  digitalWrite(ss_pin, HIGH);
  return frame;
}

byte CAN::writeFrame(CanFrame frame)
{
  byte result, i, j;
  byte regs[4];
  byte ctrl[] = {REG_TXB0CTRL, REG_TXB1CTRL, REG_TXB2CTRL};

  for(i=0; i<3; i++) {  //Loop through all three TX buffers
     read(ctrl[i], &result, 1); //Read the TXBxCTRL register
     if(!(result & TXREQ)) {    //If the TXREQ bit is not set then...
       frame2reg(frame, regs);
         
       digitalWrite(ss_pin, LOW);
       SPI.transfer(CMD_WRITETXB | (i<<1));   //Write the buffers
       SPI.transfer(regs[0]); //SIDH
       SPI.transfer(regs[1]); //SIDL
       SPI.transfer(regs[2]); //EID8
       SPI.transfer(regs[3]); //EID0
       SPI.transfer(frame.length);  //DLC
       for(j=0; j<frame.length; j++) {
          SPI.transfer(frame.data[j]);
       }
       digitalWrite(ss_pin, HIGH);  // Might need delay here
       sendCommand(CMD_RTS | (0x01 << i));
       return 0;
     }
  }   
  return 1;  //All the buffers are full
}

/* Writes the filter that is in the id and eid of frame.
   If the filter index is out of range it returns -1
   or zero on sucess */
int CAN::writeFilter(CanFrame frame, byte filter)
{
  byte start;
  byte buff[4];
  
  if(filter > 5) {
    return -1;
  }
  if(filter < 3) start = filter << 2;
  else           start = (filter - 3) <<2 | 0x10;
  frame2reg(frame, buff);
  write(start, buff, 4);
}

int CAN::writeMask(CanFrame frame, byte mask)
{
  byte start;
  byte buff[4];
  
  if(mask > 1) {
    return -1;
  }
  start = mask<<2 | 0x20;
  frame2reg(frame, buff);
  write(start, buff, 4);
}

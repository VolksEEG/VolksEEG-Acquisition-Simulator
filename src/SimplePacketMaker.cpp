#include <Arduino.h>
#include "SimplePacketMaker.h"

void WriteOutInt32AsInt16(int32_t toConvert);

extern void SendOutPacket(OutPacket* ToSend) 
{
    //WriteOutInt32AsInt16(ToSend->sync);
    //WriteOutInt32AsInt16(ToSend->counter);
    //~Serial1.println(ToSend->counter);
    ToSend->counter += 1; 

    /*
    for (int i=0; i<(int)(sizeof(ToSend->values)/sizeof(int)); i++)
    {
        WriteOutInt32AsInt16(ToSend->values[i]);
    }
    */
}

void WriteOutInt32AsInt16(int32_t ToSend) //, char* chars[2])
{   
    char byteBuffer;
    uint32_t tempInt;
    byteBuffer = ToSend & 0xFF;
    //~Serial1.print(byteBuffer);
    tempInt = (ToSend >> 8);
    //~byteBuffer = tempInt & 0xFF;
    Serial1.print(byteBuffer);
}

void SendLowest24Bits(uint32_t ToSend)
{
  char byteBuffer;
  uint32_t tempInt;
  byteBuffer = ToSend & 0xFF;
  Serial1.print(byteBuffer);
  tempInt = (ToSend >> 8);
  byteBuffer = tempInt & 0xFF;
  Serial1.print(byteBuffer);
  tempInt = (ToSend >> 16);
  byteBuffer = tempInt & 0xFF;
  Serial1.print(byteBuffer);
}
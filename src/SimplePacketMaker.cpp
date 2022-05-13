#include <Arduino.h>
#include "SimplePacketMaker.h"

void WriteOutInt32AsInt16(int32_t toConvert);

extern void SendOutPacket(OutPacket* ToSend) 
{
    uint32_t lastCounter = 0;
    WriteOutInt32AsInt16(ToSend->sync);
    WriteOutInt32AsInt16(ToSend->counter);
    ToSend->counter += 1; 

    for (int i=0; i<(int)(sizeof(ToSend->values)/sizeof(int)); i++)
    {
        WriteOutInt32AsInt16(ToSend->values[i]);
    }

}

void WriteOutInt32AsInt16(int32_t ToSend) //, char* chars[2])
{   
    char byteBuffer;
    uint32_t tempInt;
    byteBuffer = ToSend & 0xFF;
    Serial1.print(byteBuffer);
    tempInt = (ToSend >> 8);        //WARNING!!! This line seems to break Segger HW debugger, 
                                    //when examined in debugger, tempInt is always 0 
                                    //and causes ripple effects(IIRC)
                                    //but this is only when examining values in the debugger
                                    //the application actually behaves as it should
    byteBuffer = tempInt & 0xFF;
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
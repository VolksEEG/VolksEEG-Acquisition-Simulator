struct OutPacket
{
  uint32_t sync = 0xFFFF;
  uint32_t counter = 0;
  int32_t values[8];
};

void SendOutPacket(OutPacket* ToSend);
void SendLowest24Bits(uint32_t ToSend);



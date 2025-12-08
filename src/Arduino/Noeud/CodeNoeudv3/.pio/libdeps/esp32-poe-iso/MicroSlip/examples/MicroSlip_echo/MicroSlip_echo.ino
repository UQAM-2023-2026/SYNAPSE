#include <Arduino.h>
#include <MicroSlip.h>

MicroSlip mySlip(Serial);

unsigned long sendTimerStart;

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  // RECEIVING
  const int receiveBufferSize = 64;
  uint8_t receiveBuffer[receiveBufferSize];

  int receivedCount = mySlip.parsePacket(receiveBuffer, receiveBufferSize);
  if (receivedCount > 0) // WE RECEIVED DATA
  {
    // ECHO THE DATA

    mySlip.beginPacket(); // BEGIN PACKET

    for ( int i =0; i < receivedCount; i++ ) {
      mySlip.write(receiveBuffer[i]);
    }

    mySlip.endPacket(); // END PACKET
  }


}

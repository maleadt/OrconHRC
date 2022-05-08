/*
   Original Author: Klusjesman & supersjimmie

   Tested with STK500 + ATMega328P
   GCC-AVR compiler

   Modified by arjenhiemstra:
   Complete rework of the itho packet section, cleanup and easier to understand
   Library structure is preserved, should be a drop in replacement (apart from device id)
   Decode incoming messages to direct usable decimals without further bit-shifting
   DeviceID is now 3 bytes long and can be set during runtime
   Counter2 is the decimal sum of all bytes in decoded form from deviceType up to the last byte before counter2 subtracted from zero.
   Encode outgoing messages in itho compatible format
   Added ICACHE_RAM_ATTR to 'void ITHOcheck()' for ESP8266/ESP32 compatibility
   Trigger on the falling edge and simplified ISR routine for more robust packet handling
   Move SYNC word from 171,170 further down the message to 179,42,163,42 to filter out more non-itho messages in CC1101 hardware
   Check validity of incoming message

   Tested on ESP8266 & ESP32
*/

/*
  CC11xx pins    ESP pins Arduino pins  Description
  1 - VCC        VCC      VCC           3v3
  2 - GND        GND      GND           Ground
  3 - MOSI       13=D7    Pin 11        Data input to CC11xx
  4 - SCK        14=D5    Pin 13        Clock pin
  5 - MISO/GDO1  12=D6    Pin 12        Data output from CC11xx / serial clock from CC11xx
  6 - GDO2       04=D2    Pin 2?        Serial data to CC11xx
  7 - GDO0       ?        Pin  ?        output as a symbol of receiving or sending data
  8 - CSN        15=D8    Pin 10        Chip select / (SPI_SS)
*/

#include <SPI.h>
#include "RAMSES.h"
#include "RAMSESMessage.h"

#define ITHO_IRQ_PIN 22 // pin 17 / D22

Orcon rf;

ICACHE_RAM_ATTR void ITHOcheck();
void showPacket(const Orcon &rf);

void setup(void) {
  Serial.begin(115200);
  delay(500);

  Serial.println("Initialization");
  // rf.setDeviceID(130, 11, 156);
  rf.init();

  //Serial.println("Registering");
  //sendRegister();

  Serial.println("Listening for messages");
  pinMode(ITHO_IRQ_PIN, INPUT);
  attachInterrupt(ITHO_IRQ_PIN, ITHOcheck, FALLING);
}

bool has_packet = false;

void loop(void) {
  // if (has_packet) {
    if (rf.checkForNewPacket()) {
      has_packet = false;
    }
  // }
}

ICACHE_RAM_ATTR void ITHOcheck() {
  has_packet = true;
}

const char *int_to_binary_str(int x, int N_bits){
    static char b[512];
    char *p = b;
    b[0] = '\0';

    for(int i=(N_bits-1); i>=0; i--){
      *p++ = (x & (1<<i)) ? '1' : '0';
    }
    return b;
}

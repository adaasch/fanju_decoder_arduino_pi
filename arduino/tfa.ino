//New receiving method. This method checks the Rx Fifo for any data it contains.
//It allows you to do several things in a loop.
//In addition, the gdo0 and gdo2 pin are not required.
//https://github.com/LSatan/SmartRC-CC1101-Driver-Lib
//by Little_S@tan
#include <ELECHOUSE_CC1101_SRC_DRV.h>

//#include "CRC8.h"
//#include "CRC.h"

#define BUF_SIZE 280

uint64_t last = 0;
uint16_t widx = 0;
uint16_t ridx = 0;
int16_t lvl;
uint8_t buffer[BUF_SIZE] = {0};
//byte buf[100] = {0};

void push(uint8_t v){
  buffer[widx++] = v;
  lvl++;
  if(widx == BUF_SIZE)
    widx = 0;
}

uint16_t pop(){
  if(lvl < 1 )
    return __UINT16_MAX__;
  uint8_t res =  buffer[ridx++];
  if(ridx == BUF_SIZE)
    ridx = 0;
  return res;
}

int16_t fillLvl(){
  return lvl;
}


void isr(){
  uint64_t cur = micros()/100;
  push(cur-last);
  last = cur;

}

void setup()
{

  Serial.begin(115200);
  //delay(2000);
  if (ELECHOUSE_cc1101.getCC1101())
  { // Check the CC1101 Spi connection.
    Serial.println("Connection OK");
  }
  else
  {
    Serial.println("Connection Error");
  }

  ELECHOUSE_cc1101.Init();                  // must be set to initialize the cc1101!
  ELECHOUSE_cc1101.setCCMode(0);            // set config for internal transmission mode.
  ELECHOUSE_cc1101.setModulation(2);        // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
  ELECHOUSE_cc1101.setMHZ(433.850);         // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
  ELECHOUSE_cc1101.setDeviation(0);         // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
  ELECHOUSE_cc1101.setChannel(0);           // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
  ELECHOUSE_cc1101.setChsp(199.95);         // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
  ELECHOUSE_cc1101.setRxBW(350);            // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
  ELECHOUSE_cc1101.setDRate(3.8);           //3.8         // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
  ELECHOUSE_cc1101.setPA(10);               // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
  ELECHOUSE_cc1101.setSyncMode(4);          //4        // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
  ELECHOUSE_cc1101.setSyncWord(0xf0, 0xf0); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
  ELECHOUSE_cc1101.setAdrChk(0);            // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
  ELECHOUSE_cc1101.setAddr(0);              // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
  ELECHOUSE_cc1101.setWhiteData(0);         // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
  ELECHOUSE_cc1101.setPktFormat(3);         // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
  ELECHOUSE_cc1101.setLengthConfig(2);      // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
  ELECHOUSE_cc1101.setPacketLength(640);    // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
  ELECHOUSE_cc1101.setCrc(0);               // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
  ELECHOUSE_cc1101.setCRC_AF(0);            // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
  ELECHOUSE_cc1101.setDcFilterOff(0);       // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
  ELECHOUSE_cc1101.setManchester(0);        // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
  ELECHOUSE_cc1101.setFEC(0);               // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
  ELECHOUSE_cc1101.setPRE(0);               // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
  ELECHOUSE_cc1101.setPQT(0);               // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
  ELECHOUSE_cc1101.setAppendStatus(0);      // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.

  Serial.println("Rx Mode");

  ELECHOUSE_cc1101.SetRx();

  int pin = 2;
  pinMode(pin, INPUT);

  attachInterrupt(digitalPinToInterrupt(pin), isr, RISING);
}

byte buffer[BUF_SIZE] = {0};
byte buf[100] = {0};

void print_hex(byte inp)
{
  char c[3];
  itoa(inp, c, 16);
  if (c[1] == '\0')
    Serial.print("0");
  Serial.print(c);
  Serial.print(" ");
}

void loop()
{
 
  delay(10);
  return;

  //Checks whether something has been received.
  //When something is received we give some time to receive the message in full.(time in millis)
  int idx = 0;

  while (true)
  {
    int len = ELECHOUSE_cc1101.CheckRxFifo(1);
    if (len)
    {
      if (idx + len >= BUF_SIZE)
      {

        Serial.println("Buffer full");
        break;
      }
      idx += ELECHOUSE_cc1101.ReceiveData(&buffer[idx]);
      delay(10);
    }
    else if (idx > 0)
      break;
    else
      delay(10);
  }

  if (idx)
  {

    //Rssi Level in dBm
    Serial.print("Rssi: ");
    Serial.println(ELECHOUSE_cc1101.getRssi());

    //Link Quality Indicator
    Serial.print("LQI: ");
    Serial.println(ELECHOUSE_cc1101.getLqi());

    //Get received Data and calculate length

    //    if(len < 5)
    //      return;
    //    buffer[len] = '\0';

    Serial.print("Len: ");
    Serial.println(idx);
    Serial.flush();

    //Print received in char format.
    //Serial.println((char *) buffer);

    //Print received in bytes format.
    byte b = 0x80;
    int x = 0;
    int k = 0;
    for (int i = 1; i < idx; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        if (buffer[i] & (b >> j))
        {
          if (k)
          {
            buf[x++] = k;
            k = 0;
          }
        }
        else
          k++;
      }
      print_hex(buffer[i]);
    }
    Serial.println();
    for (int i = 0; i < x; i++)
    {
      Serial.print(buf[i]);
      Serial.print(" ");
    }
    Serial.println();

    //    bool start = false;
    //    int sync = 0;
    //    byte data[5] = {0};
    //    int di = 0;
    //    int bi = 0;
    //    for (int i = 0; i<x; i++){
    //      if(start){
    //        if(buf[i] > 50){
    //          start = 0;
    //          sync = 0;
    //          Serial.println();
    //          Serial.print("di: ");
    //          Serial.print(di);
    //          Serial.print(" bi: ");
    //          Serial.print(bi);
    //          Serial.println();
    //          continue;
    //        }
    //        if(buf[i] > 10)
    //          data[di] |= (0x80>>bi);
    //        bi++;
    //        if(bi == 8){
    //          print_hex(data[di]);
    //          bi = 0;
    //          di++;
    //        }
    //        if(di == 5){
    //          Serial.print("end");
    //          Serial.println();
    //        }
    //        continue;
    //      }
    //
    //      if (sync == 4 && buf[i] > 25){
    //        start = true;
    //        di = 0;
    //        bi = 0;
    //        continue;
    //      }
    //      if(buf[i]< 5)
    //        sync++;
    //      else if( buf[i] >=5 && sync)
    //        sync = 0;
    //    }
    Serial.println();
    //
    //    Serial.print("CRC: ");
    //    uint8_t chk = crc8((uint8_t *)buffer, 4, 0x31);
    //    char c[3];
    //    itoa(chk,c,16);
    //    if(c[1] == '\0')
    //      Serial.print("0");
    //    Serial.print(c);
    //    if((byte)chk == buffer[4]){
    //      Serial.print(" MATCH!");
    //      Serial.println();
    //      float temp=((buffer[1]&0xf)*100+(buffer[2]>>4)*10+(buffer[2]&0xf));
    //      temp=temp/10-40;
    //      byte hum  =  buffer[3];
    //      Serial.print(temp);
    //      Serial.print("'C ");
    //      Serial.print(hum);
    //      Serial.print("%");
    //      Serial.println();
    //    }
    Serial.println();

    Serial.flush();
  }
  delay(1000);
}

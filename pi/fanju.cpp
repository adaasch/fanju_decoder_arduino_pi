/**
 * @file cc1101_New_Receive_method_advanced.ino
 * @author Andreas Daasch (andreas.daasch@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-12-29
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <pigpiod_if2.h>

#define BUF_SIZE 512

uint64_t last = 0;
uint16_t widx = 0;
uint16_t ridx = 0;
int16_t lvl;
uint8_t buffer[BUF_SIZE] = {0};

int myPi;

void push(uint8_t v)
{
  buffer[widx++] = v;
  lvl++;
  if (widx == BUF_SIZE)
    widx = 0;
}

uint16_t pop()
{
  if (lvl < 1)
    return __UINT16_MAX__;
  uint8_t res = buffer[ridx++];
  lvl--;
  if (ridx == BUF_SIZE)
    ridx = 0;
  return res;
}

int16_t fillLvl()
{
  return lvl;
}

void isr(int pi, unsigned user_gpio, unsigned level, uint32_t tick)
{
  uint32_t cur = tick / 100;
  push(cur - last);
  last = cur;
}

void setup()
{
  myPi = pigpio_start(NULL, NULL);

  if (myPi < 0 || !ELECHOUSE_cc1101.getCC1101())
  {
    printf("Init Error\n");
    while (1)
      ;
  }

  ELECHOUSE_cc1101.Init();                  // must be set to initialize the cc1101!
  ELECHOUSE_cc1101.setCCMode(0);            // set config for internal transmission mode.
  ELECHOUSE_cc1101.setModulation(2);        // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
  ELECHOUSE_cc1101.setMHZ(433.850);         // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
  ELECHOUSE_cc1101.setDeviation(0);         // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
  ELECHOUSE_cc1101.setChannel(0);           // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
  ELECHOUSE_cc1101.setChsp(199.95);         // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
  ELECHOUSE_cc1101.setRxBW(58.03);          // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
  ELECHOUSE_cc1101.setDRate(3.8);           //3.8         // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
  ELECHOUSE_cc1101.setPA(10);               // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
  ELECHOUSE_cc1101.setSyncMode(4);          //4        // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
  ELECHOUSE_cc1101.setSyncWord(0xf0, 0xf0); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
  ELECHOUSE_cc1101.setAdrChk(0);            // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
  ELECHOUSE_cc1101.setAddr(0);              // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
  ELECHOUSE_cc1101.setWhiteData(0);         // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
  ELECHOUSE_cc1101.setPktFormat(3);         // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
  ELECHOUSE_cc1101.setLengthConfig(2);      // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
  ELECHOUSE_cc1101.setPacketLength(0xff);   // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
  ELECHOUSE_cc1101.setCrc(0);               // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
  ELECHOUSE_cc1101.setCRC_AF(0);            // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
  ELECHOUSE_cc1101.setDcFilterOff(0);       // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
  ELECHOUSE_cc1101.setManchester(0);        // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
  ELECHOUSE_cc1101.setFEC(0);               // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
  ELECHOUSE_cc1101.setPRE(0);               // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
  ELECHOUSE_cc1101.setPQT(0);               // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
  ELECHOUSE_cc1101.setAppendStatus(0);      // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.

  ELECHOUSE_cc1101.SetRx();

  int pin = 25;
  set_mode(myPi, pin, PI_INPUT);
  callback(myPi, pin, RISING_EDGE, isr);
}

bool chkChkSum(uint8_t *data)
{
  uint8_t mask = 0xc;
  uint8_t bit, csum = 0;

  uint8_t chkSum = (data[1] & 0xf0) >> 4;
  data[1] = (data[1] & 0x0f) | ((data[4] & 0x0f) << 4);
  for (uint8_t i = 0; i < 36; i++)
  {
    bit = mask & 0x1;
    mask >>= 1;
    if (bit == 1)
      mask ^= 0x9;
    if (data[i / 8] & (0x80 >> (i % 8)))
      csum ^= mask;
  }
  return csum == chkSum;
}

void analyse(uint8_t *data)
{
  if (chkChkSum(data))
  {
    uint8_t hum = (data[3] & 0x0f) * 10 + ((data[4] & 0xf0) >> 4);
    int16_t t = (data[2] << 4) + ((data[3] & 0xf0) >> 4);
    float temp = (t - 900 - 320) * 5 / 90.0;
    uint8_t chan = data[4] & 0x3;
    bool tx_req = data[1] & 0x8;
    bool bat_ok = !(data[1] & 0x4);

    printf("%.3f,%.1f,%u,%u,%u,%u\n", time_time(), temp, hum, bat_ok, tx_req, chan);
  }
}

enum mode
{
  SYNC,
  DATA
};

static int state = mode::SYNC;
static uint8_t sync = 0;
static uint8_t bits = 0;
static uint8_t fail = 0;

static uint8_t data[5] = {0};
static uint8_t empty[5] = {0};

void loop()
{
  time_sleep(.1);
  //printf("fill: %u\n",fillLvl());
  if (fillLvl() < 40)
    return;

  time_sleep(1); // Wait for full reception

  while (fillLvl() > 0)
  {
    uint16_t v = pop();
    //printf("%u ",v);
    switch (state)
    {
    case mode::SYNC:
      if (v < 22 && v > 18)
        sync++;
      else if (v > 80 && v < 90 && sync > 1)
      {
        state = mode::DATA;
        memcpy(data, empty, 5);
        sync = 0;
      }
      else
      {
        sync = 0;
      }
      break;
    case mode::DATA:
      if (v < 22 || v > 50)
      {
        fail++;
        if (fail > 3)
        {
          state = mode::SYNC;
          bits = 0;
          fail = 0;
        }
      }
      else if (v >= 30)
      {
        data[bits / 8] |= 0x80 >> (bits % 8);
        bits++;
      }
      else if (v < 30)
      {
        bits++;
      }
      else
      {
        state = mode::SYNC;
        bits = 0;
        fail = 0;
      }
      break;
    default:
      break;
    }
    if (bits == 40)
    {
      // for (size_t i = 0; i < 5; i++)
      //   print_hex(data[i]);
      // Serial.println();
      analyse(data);
      state = mode::SYNC;
      bits = 0;
      fail = 0;
    }
  }
}

int main(int, char **)
{
  setup();
  while (1)
    loop();
  return 0;
}

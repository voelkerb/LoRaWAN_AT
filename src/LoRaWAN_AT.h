/***************************************************
 Library for interfacing with a microcontroller over 
 mqtt. Subscribe to differen topics and have callback functions

 Feel free to use the code as it is.

 Benjamin Völker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 ****************************************************/

#ifndef LORAWAN_AT_h
#define LORAWAN_AT_h

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define LORA_JOIN_TIME 10000
#define LORA_SEND_TIME 10000
#define LORA_RCV_BUF_SIZE 128
#define LORA_MAX_DATA_SIZE 64

struct LoRaWANConfiguration {
  //NOTE: Relay state must be first in configuration
  uint8_t app_eui[8]; //0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06
  uint8_t dev_eui[8]; //0x2C, 0xF7, 0xF1, 0x20, 0x24, 0x90, 0x11, 0xC1
  uint8_t app_key[16]; //0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
  uint8_t port; // invalid port
}; 

class LoRaWAN_AT {
  public:
    LoRaWAN_AT();

    bool init(Stream * uart, void (*logFunc)(const char * msg, ...)=NULL, void (*downLinkMsgCB)(const char * data, int port, int snr, int rssi)=NULL);
    void setOTAA(LoRaWANConfiguration theConfig);
    // void setOTAA(const char *app_eui, const char *dev_eui, const char *app_key, uint8_t port);
    bool joinNetwork();
    void update();
    void sendCommand(const char * cmd);
    void getInfo();
    char* toHexStr(uint8_t *a, size_t size);

    bool connected;
    bool joined;
    LoRaWANConfiguration config;
  private:

    uint32_t _lastJoin;

    void readLine();
    void at_recv();
    void at_send(const char * cmd, ...);
    int at_send_check_response(const char * contains_ack, int timeout_ms, const char * cmd, ...);

    int _port;
    char _recv_buf[LORA_RCV_BUF_SIZE];
    char _data_buf[LORA_MAX_DATA_SIZE];
    const char * _APP_KEY;
    const char * _DEV_EUI;
    const char * _APP_EUI;
    Stream * _uart;
    // A callback on NTP success called
    void (*_logFunc)(const char * msg, ...);
    void (*_downLinkMsgCB)(const char * data, int port, int snr, int rssi);
};

#endif
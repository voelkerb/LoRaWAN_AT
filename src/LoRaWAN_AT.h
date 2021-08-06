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
  uint8_t app_eui[8];
  uint8_t dev_eui[8];
  uint8_t app_key[16];
  uint8_t port;
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
    char _recv_buf[LORA_RCV_BUF_SIZE];
    char _data_buf[LORA_MAX_DATA_SIZE];
    Stream * _uart;
    // A callback on NTP success called
    void (*_logFunc)(const char * msg, ...);
    void (*_downLinkMsgCB)(const char * data, int port, int snr, int rssi);
};

#endif
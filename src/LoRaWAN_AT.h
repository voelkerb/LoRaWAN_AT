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
#define LORA_MAX_DATA_SIZE 32

class LoRaWAN_AT {
  public:
    LoRaWAN_AT();

    bool init(Stream * uart, void (*logFunc)(const char * msg, ...)=NULL, void (*downLinkMsgCB)(const char * data, int port, int snr, int rssi)=NULL);
    void setOTAA(const char *app_eui, const char *dev_eui, const char *app_key, uint8_t port);
    bool joinNetwork();
    void update();
    void sendCommand(const char * cmd);
    void getInfo();

    bool connected;
    bool joined;
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
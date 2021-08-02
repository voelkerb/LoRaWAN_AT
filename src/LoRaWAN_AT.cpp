/***************************************************
 Library for interfacing with a microcontroller over 
 mqtt. Subscribe to differen topics and have callback functions

 Feel free to use the code as it is.

 Benjamin Völker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 ****************************************************/

#include "LoRaWAN_AT.h"


LoRaWAN_AT::LoRaWAN_AT() {
  connected = false;
  joined = false;
  _lastJoin = millis();
  snprintf(_recv_buf, LORA_RCV_BUF_SIZE, "LoRa-RX: ");
  snprintf(_data_buf, LORA_MAX_DATA_SIZE, "");
  _APP_EUI = NULL;
  _DEV_EUI = NULL;
  _APP_KEY = NULL;
  _port = -1;
}

bool LoRaWAN_AT::init(Stream * uart, void (*logFunc)(const char * msg, ...), void (*downLinkMsgCB)(const char * data, int port, int snr, int rssi)) {
  _uart = uart;
  _logFunc = logFunc;
  _downLinkMsgCB = downLinkMsgCB;
  if (at_send_check_response("+AT: OK", 200, "AT")) {
    connected = true;
  }
  // TODO: remove
  // connected = true;
  return connected;
}

void LoRaWAN_AT::setOTAA(const char * app_eui, const char * dev_eui, const char * app_key, uint8_t port) {
  _APP_EUI = app_eui;
  _DEV_EUI = dev_eui;
  _APP_KEY = app_key;
  _port = (int)port;
}

void LoRaWAN_AT::getInfo() {
  at_send_check_response("+ID: AppEui", 1000, "AT+ID");
}

bool LoRaWAN_AT::joinNetwork() {
  if (connected && not joined && millis() - _lastJoin > LORA_JOIN_TIME) {
    if (_DEV_EUI) at_send_check_response("+ID: DevEui", 1000, "AT+ID=DevEui, \"%s\"", _DEV_EUI);
    if (_APP_EUI) at_send_check_response("+ID: AppEui", 1000, "AT+ID=AppEui, \"%s\"", _APP_EUI);
    at_send_check_response("+MODE: LWOTAA", 1000, "AT+MODE=LWOTAA");
    at_send_check_response("+DR: EU868", 1000, "AT+DR=EU868");
    at_send_check_response("+CH: NUM", 1000, "AT+CH=NUM,0-2");
    if (_APP_KEY) at_send_check_response("+KEY: APPKEY", 1000, "AT+KEY=APPKEY,\"%s\"", _APP_KEY);
    at_send_check_response("+CLASS: A", 1000, "AT+CLASS=A");
    if (_port > 0 && _port < 224) at_send_check_response("+PORT: 8", 1000, "AT+PORT=%i", _port);
    bool ret = at_send_check_response("+JOIN: Network joined", 12000, "AT+JOIN");
    if (ret) joined = true;
    // Only try each XX seconds
    _lastJoin = millis();
  } 
  return joined;
}

void LoRaWAN_AT::sendCommand(const char * cmd) {
  // if (not connected) return;
  at_send(cmd);
}

void LoRaWAN_AT::readLine() {
  snprintf(_recv_buf, LORA_RCV_BUF_SIZE, "%s", _uart->readStringUntil('\n').c_str());
  uint32_t length = strlen(_recv_buf);
  if (_recv_buf[length-1] == '\n') _recv_buf[length-1] = '\0';
  length = strlen(_recv_buf);
  if (_recv_buf[length-1] == '\r') _recv_buf[length-1] = '\0';
}

void LoRaWAN_AT::update() {
  if (not connected) return;
  while (_uart->available()) {
    readLine();
    // On network join
    if (strstr(_recv_buf, "+JOIN: Network joined") != NULL) {
      joined = true;
    } else if (strstr(_recv_buf, "+JOIN: Joined already") != NULL) {
      joined = true;
    // On network disconnect
    } else if (strstr(_recv_buf, "Please join network first") != NULL) {
      joined = false;
    // On RX message
    } else if (strstr(_recv_buf, "RX: \"") != NULL) {
      at_recv();
      continue;
    }
    if (_logFunc) _logFunc("LoRa-RX: %s", _recv_buf);
  }
}

void LoRaWAN_AT::at_recv() {
  char *p_start = NULL;
  int rssi = 0;
  int port = 0;
  int snr = 0;
  bool success = false;
  // Delete what was previously in the receive array.
  snprintf(_data_buf, LORA_MAX_DATA_SIZE, "");

  if (_logFunc) _logFunc("LoRa-Msg: %s", _recv_buf);
  p_start = strstr(_recv_buf, "PORT:");
  if (p_start && (1 == sscanf(p_start, "PORT: %d", &port))) {
    if (_logFunc) _logFunc("LoRa-Port: %i", port);
  }
  p_start = strstr(_recv_buf, "RX");
  if (p_start && (1 == sscanf(p_start, "RX: \"%s\"", &_data_buf))) {
    success = true;
    uint32_t length = strlen(_data_buf);
    if (_data_buf[length-1] == '\"') _data_buf[length-1] = '\0';
    if (_logFunc) _logFunc("LoRa-Data: %s", _data_buf);
  }
  // Read next line which contains RSSI and SNR info
  readLine();
  if (_logFunc) _logFunc("LoRa-Msg: %s", _recv_buf);
  p_start = strstr(_recv_buf, "RSSI");
  if (p_start && (1 == sscanf(p_start, "RSSI %d,", &rssi))) {
    if (_logFunc) _logFunc("LoRa-RSSI: %i", rssi);
  }
  p_start = strstr(_recv_buf, "SNR");
  if (p_start && (1 == sscanf(p_start, "SNR %d", &snr))) {
    if (_logFunc) _logFunc("LoRa-SNR: %i", snr);
  }
  if (success and _downLinkMsgCB) _downLinkMsgCB(_data_buf, port, snr, rssi);
}

void LoRaWAN_AT::at_send(const char * cmd, ...) {
  va_list args;
  va_start(args, cmd);
  vsnprintf(_recv_buf, LORA_RCV_BUF_SIZE, cmd, args);
  va_end(args);
  _uart->printf("%s\r\n",_recv_buf);
  if (_logFunc) _logFunc("LoRa-TX: %s", _recv_buf);
}

int LoRaWAN_AT::at_send_check_response(const char *contains_ack, int timeout_ms, const char * cmd, ...) {
  int index = 0;
  va_list args;
  va_start(args, cmd);
  vsnprintf(_recv_buf, LORA_RCV_BUF_SIZE, cmd, args);
  va_end(args);
  _uart->printf("%s\r\n",_recv_buf);
  if (_logFunc) _logFunc("LoRa-TX: %s", _recv_buf);
  if (contains_ack == NULL) return 0;
  delay(20);
  uint32_t startMillis = millis();
  snprintf(_recv_buf, LORA_RCV_BUF_SIZE, "");
  while (millis() - startMillis < timeout_ms) {
    while (_uart->available() > 0) {
      readLine();
      // Pass to log func
      if (_logFunc) _logFunc("LoRa-RX: %s", _recv_buf);
      // Return 
      if (strstr(_recv_buf, contains_ack) != NULL) return 1;
    }
  } 
  return 0;
}


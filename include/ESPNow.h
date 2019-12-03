
#ifndef WiFi
#include <WiFi.h>
#include <esp_now.h>
#endif

bool clientConnected = false;

// Global copy of slave
esp_now_peer_info_t slave;
#define CHANNEL 1
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0

// Prototypes
void deletePeer();

// Init ESP Now with fallback
void setupESPNow()
{
  WiFi.mode(WIFI_STA);
  Serial.println("ESPNow/Basic/Master Example");
  Serial.printf("MacAddr: %s\n", WiFi.macAddress());

  // Serial.print("STA MAC: ");
  // Serial.println(WiFi.macAddress);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESPNow Init Success");
  }
  else
  {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

bool printSendStatus(esp_err_t status) {
  if (status == ESP_OK)
  {
    // Pair success
    Serial.println("Success");
    return true;
  }
  else if (status == ESP_ERR_ESPNOW_NOT_INIT)
  {
    // How did we get so far!!
    Serial.println("ESPNOW Not Init");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_FULL)
  {
    Serial.println("Peer list full");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("Out of memory");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_EXIST)
  {
    Serial.println("Exists");
    return true;
  }
  else
  {
    Serial.println("Not sure what happened");
    return false;
  }

}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
bool pairSlave()
{
  if (slave.channel == CHANNEL)
  {
    if (DELETEBEFOREPAIR)
    {
      deletePeer();
    }

    Serial.print("Slave Status: ");
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(slave.peer_addr);
    if (exists)
    {
      // Slave already paired.
      Serial.println("Already Paired");
      return true;
    }
    else
    {
      // Slave not paired, attempt pair
      esp_err_t addStatus = esp_now_add_peer(&slave);
      return printSendStatus(addStatus);
    }
  }
  else
  {
    // No slave found to process
    Serial.println("No Slave found to process");
    return false;
  }
}

// Scan for slaves in AP mode
void ScanForSlave()
{
  int8_t scanResults = WiFi.scanNetworks();
  // reset on each scan
  bool slaveFound = 0;
  memset(&slave, 0, sizeof(slave));

  Serial.println("");
  if (scanResults == 0)
  {
    Serial.println("No WiFi devices in AP Mode found");
  }
  else
  {
    Serial.print("Found ");
    Serial.print(scanResults);
    Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i)
    {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      if (PRINTSCANRESULTS)
      {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(SSID);
        Serial.print(" (");
        Serial.print(RSSI);
        Serial.print(")");
        Serial.println("");
      }
      delay(10);
      // Check if the current device starts with `Slave`
      if (SSID.indexOf("Slave") == 0)
      {
        // SSID of interest
        Serial.println("Found a Slave.");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(SSID);
        Serial.print(" [");
        Serial.print(BSSIDstr);
        Serial.print("]");
        Serial.print(" (");
        Serial.print(RSSI);
        Serial.print(")");
        Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]))
        {
          for (int ii = 0; ii < 6; ++ii)
          {
            slave.peer_addr[ii] = (uint8_t)mac[ii];
          }
        }

        slave.channel = CHANNEL; // pick a channel
        slave.encrypt = 0;       // no encryption

        slaveFound = 1;
        // we are planning to have only one slave in this example;
        // Hence, break after we find one, to be a bit efficient
        break;
      }
    }
  }

  if (slaveFound)
  {
    Serial.println("Slave Found, processing..");
  }
  else
  {
    Serial.println("Slave Not Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
}

void deletePeer()
{
  esp_err_t delStatus = esp_now_del_peer(slave.peer_addr);
  Serial.print("Slave Delete Status: ");
  printSendStatus(delStatus);
}

// uint8_t data = 0;
// send data
void sendData(const uint8_t *data, int len)
{
  uint8_t d;
  memcpy(&d, data, len);

  // data++;
  const uint8_t *peer_addr = slave.peer_addr;
  // Serial.print("Sending: ");
  // Serial.println(data);

  esp_err_t result = esp_now_send(peer_addr, data, len);

  Serial.print("Send Status: ");
  printSendStatus(result);
}

void macAddrToString(const uint8_t *mac_addr, char* macAddr) {
  char macStr1[18];
  snprintf(macAddr, sizeof(macStr1), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], 
           mac_addr[1], 
           mac_addr[2], 
           mac_addr[3], 
           mac_addr[4], 
           mac_addr[5]);
}

// callback when data is sent from Master to Slave
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // char macStr1[18];
  // macAddrToString(mac_addr, macStr1);
  // Serial.print("Last Packet Sent to: ");
  // Serial.println( macStr1 );
}

unsigned long lastPacketRxTime = 0;

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  VescData rx;
  memcpy(/*to*/&rx, /*from*/data, data_len);

  lastPacketRxTime = millis();
  Serial.print("Last Packet Recv Data: ");
  Serial.println(rx.id);

  rx.batteryVoltage = 3.4;

  uint8_t bs[sizeof(rx)];
  memcpy(bs, &rx, sizeof(rx));

  // echo to slave
  if (!btn1.isPressed()) {
    sendData(bs, sizeof(bs));
    DEBUGVAL(rx.id, rx.batteryVoltage);
Serial.println("-------------");
  }
  else {
    Serial.printf("Not replying!\n");
    Serial.println("-------------");
  }
}
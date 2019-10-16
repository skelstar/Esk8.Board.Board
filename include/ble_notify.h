#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

bool bleClientConnected = false;

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
	// receive

  #define CLEAR_TRIP_ODO_COMMAND  99

  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0 && value[0] == CLEAR_TRIP_ODO_COMMAND) {
      // clearTripMeterAndOdometer();
    }
  }
};


class MyServerCallbacks2: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      bleClientConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      bleClientConnected = false;
    }
};

void setupBLE() {

    BLEDevice::init("ESP32 Esk8.Board.Server");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks2());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_NOTIFY
    );
	  pCharacteristic->addDescriptor(new BLE2902());

    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    // pCharacteristic->setValue("Hello World says Neil");
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

void sendDataToClient() {

	uint8_t bs[sizeof(vescdata)];
	memcpy(bs, &vescdata, sizeof(vescdata));

	pCharacteristic->setValue(bs, sizeof(bs));
	pCharacteristic->notify();
}

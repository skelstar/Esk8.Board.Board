#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Wire.h>
#include <MCP23017.h>

#define I2CPORTEXP1_TASK

namespace nsI2CPortExp1Task
{
  // prototypes
}

class I2CPortExp1Task : public TaskBase
{
public:
  bool printWarnings = true;

  enum PortPin
  {
    PIN_0 = (1 << 0),
    PIN_1 = (1 << 1),
    PIN_2 = (1 << 2),
    PIN_3 = (1 << 3),
    PIN_4 = (1 << 4),
    PIN_5 = (1 << 5),
    PIN_6 = (1 << 6),
    PIN_7 = (1 << 7),
  };

private:
  Queue1::Manager<I2CPinsType> *i2cPinsQueue = nullptr;
  elapsedMillis _sinceInitialised;
  bool initialClearedPorts = false;

  const uint8_t ERROR_MUX_LOCKED = 99;

  const uint8_t LIGHT_PIN = PIN_7;

#define FRONT_EXP_ADDR 0x20
#define REAR_EXP_ADDR 0x21

  enum ExpanderDevice
  {
    FRONT = 0,
    REAR,
  };

  MCP23017 portExpFront = MCP23017(FRONT_EXP_ADDR);
  MCP23017 portExpRear = MCP23017(REAR_EXP_ADDR);

  uint8_t //_inputPins = 0x00,
      _outputPinsFront = 0x00,
      _outputPinsRear = 0x00;

  I2CPinsType m_i2cPins;

public:
  I2CPortExp1Task() : TaskBase("I2CPortExp1Task", 3000)
  {
    _core = CORE_0;
  }

private:
  void _initialise()
  {
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    // FRONT
    if (take(mux_I2C, TICKS_10ms))
    {
      portExpFront.init();
      portExpFront.portMode(MCP23017Port::A, 0);                  //port A output
      portExpFront.portMode(MCP23017Port::B, 0b11111111);         //port B input
      portExpFront.writeRegister(MCP23017Register::GPIO_A, 0x00); //Reset port A
      portExpFront.writeRegister(MCP23017Register::GPIO_B, 0x00); //Reset port B
      give(mux_I2C);
    }

    // REAR
    if (take(mux_I2C, TICKS_10ms))
    {
      portExpRear.init();
      portExpRear.portMode(MCP23017Port::A, 0);                  //port A output
      portExpRear.writeRegister(MCP23017Register::GPIO_A, 0x00); //Reset port A
      give(mux_I2C);
    }

    i2cPinsQueue = createQueue<I2CPinsType>("(I2CPortExp1Task) i2cPinsQueue");
    i2cPinsQueue->printMissedPacket = true;

    _sinceInitialised = 0;
  }

  bool flashingOutput = false;
  elapsedMillis sinceStartedFlashing = 0;
  const unsigned long FLASH_DURATION = doWorkIntervalFast;

  void doWork()
  {
    if (i2cPinsQueue->hasValue())
      _handleI2CPins(i2cPinsQueue->payload);

    updateInputs();

    if (!initialClearedPorts && _sinceInitialised > PERIOD_1s)
    {
      // make sure outputs (lights) are off after 1s
      // fix a bug where lights were ON at startup
      _clearOutputPortPin(ExpanderDevice::FRONT, LIGHT_PIN);
      _clearOutputPortPin(ExpanderDevice::REAR, LIGHT_PIN);
      initialClearedPorts = true;
    }
  }

  void cleanup()
  {
    delete (i2cPinsQueue);
  }

private:
  void _handleI2CPins(const I2CPinsType &payload)
  {
    i2cPinsQueue->payload.print("I2CPortExpTask");
    m_i2cPins = payload;

    if (take(mux_I2C, TICKS_50ms))
    {
      portExpFront.writePort(MCP23017Port::A, (uint8_t)payload.outputs);
      portExpRear.writePort(MCP23017Port::A, (uint8_t)(payload.outputs >> 8));
      give(mux_I2C);
    }
  }

  void updateInputs()
  {
    const uint8_t ALL_ON_CONDIITION = 0xff;

    uint8_t latestFront = ~_readInputs(); // flip bits

    if (latestFront != ERROR_MUX_LOCKED &&
        m_i2cPins.inputs != latestFront &&
        latestFront != ALL_ON_CONDIITION)
    {
      m_i2cPins.inputs = latestFront;
      i2cPinsQueue->send(&m_i2cPins);
    }
  }

  void _setOutputPortPin(uint8_t expander, uint8_t pin)
  {
    if (take(mux_I2C, TICKS_50ms))
    {
      if (expander == ExpanderDevice::FRONT)
      {
        _outputPinsFront |= pin;
        portExpFront.writePort(MCP23017Port::A, _outputPinsFront);
      }
      else if (expander == ExpanderDevice::REAR)
      {
        _outputPinsRear |= pin;
        portExpRear.writePort(MCP23017Port::A, _outputPinsRear);
      }
      give(mux_I2C);
    }
  }

  void _clearOutputPortPin(uint8_t expander, uint8_t pin)
  {
    if (take(mux_I2C, TICKS_50ms))
    {
      if (expander == ExpanderDevice::FRONT)
      {
        _outputPinsFront &= ~pin;
        portExpFront.writePort(MCP23017Port::A, _outputPinsFront);
      }
      else if (expander == ExpanderDevice::REAR)
      {
        _outputPinsRear &= ~pin;
        portExpRear.writePort(MCP23017Port::A, _outputPinsRear);
      }
      give(mux_I2C);
    }
  }

  uint8_t _readInputs()
  {
    if (take(mux_I2C, TICKS_50ms))
    {
      portExpFront.writePort(MCP23017Port::B, 0x00); // write LOW before read
      uint8_t currentB = portExpFront.readPort(MCP23017Port::B);
      give(mux_I2C);
      return currentB;
    }
    return ERROR_MUX_LOCKED;
  }
};

I2CPortExp1Task i2cPortExpTask;

namespace nsI2CPortExp1Task
{
  void task1(void *parameters)
  {
    i2cPortExpTask.task(parameters);
  }
}

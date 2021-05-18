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
  void _handleSimplMessage(SimplMessageObj obj);
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
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;
  SimplMessageObj _simplMsg;

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

  uint8_t _inputPins = 0x00,
          _outputPinsFront = 0x00,
          _outputPinsRear = 0x00;

public:
  I2CPortExp1Task() : TaskBase("I2CPortExp1Task", 3000, PERIOD_100ms)
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

    simplMsgQueue = createQueue<SimplMessageObj>("(I2CPortExp1Task) simplMsgQueue");
    _simplMsg.setGetMessageCallback(getSimplMessage);
  }

  bool flashingOutput = false;
  elapsedMillis sinceStartedFlashing = 0;
  const unsigned long FLASH_DURATION = doWorkInterval;

  void doWork()
  {
    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    checkExp1Inputs();

    if (flashingOutput && sinceStartedFlashing > FLASH_DURATION)
    {
      _clearOutputPortPin(ExpanderDevice::FRONT, LIGHT_PIN);
      _clearOutputPortPin(ExpanderDevice::REAR, LIGHT_PIN);
      //cleanup
      flashingOutput = false;
      sinceStartedFlashing = 0;
    }
  }

  void cleanup()
  {
    delete (simplMsgQueue);
  }

private:
  void _handleSimplMessage(SimplMessageObj obj)
  {
    _simplMsg = obj;
    _simplMsg.setGetMessageCallback(getSimplMessage);

    switch (_simplMsg.message)
    {
    case SIMPL_HEADLIGHT_ON:
      _setOutputPortPin(ExpanderDevice::FRONT, LIGHT_PIN);
      _setOutputPortPin(ExpanderDevice::REAR, LIGHT_PIN);
      break;
    case SIMPL_HEADLIGHT_OFF:
      _clearOutputPortPin(ExpanderDevice::FRONT, LIGHT_PIN);
      _clearOutputPortPin(ExpanderDevice::REAR, LIGHT_PIN);
      break;
    case SIMPL_HEADLIGHT_FLASH:
      // turn on, off later
      _setOutputPortPin(ExpanderDevice::FRONT, LIGHT_PIN);
      _setOutputPortPin(ExpanderDevice::REAR, LIGHT_PIN);
      flashingOutput = true;
      sinceStartedFlashing = 0;
    }
    // _simplMsg.print("[Task: I2CPortExpTask]");
  }

  void checkExp1Inputs()
  {
    uint8_t latestFront = _readInputs();
    if (latestFront != ERROR_MUX_LOCKED && _inputPins != latestFront)
    {
      if (CHECK_BIT_LOW(latestFront, PIN_7))
      {
        _simplMsg.message = I2C_INPUT_7_PRESSED; // light switch?
        simplMsgQueue->send(&_simplMsg);
      }
      _inputPins = latestFront;
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

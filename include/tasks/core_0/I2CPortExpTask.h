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

#define MCP23017_ADDR 0x20
  MCP23017 mcp = MCP23017(MCP23017_ADDR);

  uint8_t _inputPins = 0x00,
          _outputPins = 0x00;

public:
  I2CPortExp1Task() : TaskBase("I2CPortExp1Task", 3000, PERIOD_100ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_0;
  }

private:
  void initialise()
  {
    mcp.init();
    mcp.portMode(MCP23017Port::A, 0);          //port A output
    mcp.portMode(MCP23017Port::B, 0b11111111); //port B input

    mcp.writeRegister(MCP23017Register::GPIO_A, 0x00); //Reset port A
    mcp.writeRegister(MCP23017Register::GPIO_B, 0x00); //Reset port B

    simplMsgQueue = createQueue<SimplMessageObj>("(I2CPortExp1Task) simplMsgQueue");
  }

  void doWork()
  {
    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    uint8_t latest = _readInputs();
    if (_inputPins != latest)
    {
      if (CHECK_BIT_LOW(latest, PIN_7))
      {
        _simplMsg.message = I2C_INPUT_7_PRESSED;
        simplMsgQueue->send(&_simplMsg);
      }
      _inputPins = latest;
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
    if (_simplMsg.message == SIMPL_HEADLIGHT_ON)
    {
      _setOutputPortPin(PIN_7);
      _setOutputPortPin(PIN_6);
    }
    else if (_simplMsg.message == SIMPL_HEADLIGHT_OFF)
    {
      _clearOutputPortPin(PIN_7);
      _clearOutputPortPin(PIN_6);
    }
  }

  void _setOutputPortPin(uint8_t pin)
  {
    _outputPins |= pin;
    mcp.writePort(MCP23017Port::A, _outputPins);
  }

  void _clearOutputPortPin(uint8_t pin)
  {
    _outputPins &= ~pin;
    mcp.writePort(MCP23017Port::A, _outputPins);
  }

  uint8_t _readInputs()
  {
    mcp.writePort(MCP23017Port::B, 0x00); // write LOW before read
    uint8_t currentB = mcp.readPort(MCP23017Port::B);
    return currentB;
  }
};

I2CPortExp1Task i2cButtonTask;

namespace nsI2CPortExp1Task
{
  void task1(void *parameters)
  {
    i2cButtonTask.task(parameters);
  }
}

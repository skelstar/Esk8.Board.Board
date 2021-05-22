
#include <tasks/core_0/ControllerCommsTask.h>
#include <tasks/core_0/HeadlightTask.h>
#include <tasks/core_0/I2CPortExpTask.h>
#include <tasks/core_0/VescCommsTask.h>

#if SEND_TO_VESC == 0
#include <tasks/core_0/MockVescTask.h>
#endif
#ifdef USE_M5STACK_DISPLAY
#include <tasks/core_0/M5StackDisplayTask.h>
#endif
#ifdef USE_128x64OLED_TASK
#include <tasks/core_0/I2COledTask.h>
#endif
#ifdef USE_IMU_TASK
#include <tasks/core_0/IMUTask.h>
#endif
#ifdef USE_FOOTLIGHT_TASK
#include <tasks/core_0/FootLightTask.h>
#endif

#define NUM_TASKS 20 // used in taskList
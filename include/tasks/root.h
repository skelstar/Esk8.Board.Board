
#include <tasks/core_0/ControllerCommsTask.h>
#if SEND_TO_VESC == 0
#include <tasks/core_0/MockVescTask.h>
#endif
#ifdef USING_M5STACK_DISPLAY
#include <tasks/core_0/M5StackDisplayTask.h>
#endif

#include <tasks/core_0/VescCommsTask.h>
#include <tasks/core_0/FootLightTask.h>
#include <tasks/core_0/HeadlightTask.h>

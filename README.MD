# Tasks

## ControllerCommsTask
- when controller packet available (polled)
  - put packet on controllerData queue
- watch the vescData queue for the response... 
  - vescData queue will have the updated data from the Vesc (fetched just previously)
  - should match the controller packet id

## VescCommsTask
- a short time before before getting controller packet on queue (50ms?), fetch the Vesc data
  - put Vesc data in the local vescDataQueue payload, ready to send
- when rx controller packet, send throttle data to Vesc
  - "respond" by using the vescDataQueue (with updated data)
  - this will give illusion of the round trip

# TTGO T-Display

![image](https://github.com/Xinyuan-LilyGO/TTGO-T-Display/blob/master/image/pinmap.jpg)
## 1.Install the following dependency library files:
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)        
- [Button2](https://github.com/LennartHennigs/Button2)


## 2.TFT_eSPI settings
- Move or copy `User_Setups/TTGO_T_Display.h` to `TFT_eSPI/User_Setups/`
- Add `#include <User_Setups/TTGO_T_Display.h>` to  `TFT_eSPI/User_Setup_Select.h`
  

## Pinout
| Name       | V18    |
| ---------- | ------ |
| TFT Driver | ST7789 |
| TFT_MISO   | N/A    |
| TFT_MOSI   | 19     |
| TFT_SCLK   | 18     |
| TFT_CS     | 5      |
| TFT_DC     | 16     |
| TFT_RST    | N/A    |
| TFT_BL     | 4      |
| I2C_SDA    | 21     |
| I2C_SCL    | 22     |
| ADC_IN     | 34     |
| BUTTON1    | 35     |
| BUTTON2    | 0      |
| ADC Power  | 14     |

2019/08/06:
* The TFT_eSPI and Button2 libraries have been synchronized to the main branch

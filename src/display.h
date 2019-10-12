#ifndef u8g2
#include <U8g2lib.h>
#endif

#define OLED_SDA  4
#define OLED_SCL  15


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

void initDisplay()
{
  // OLED
  // u8g2.setI2CAddress(0x3C);
  u8g2.begin();
  // u8g2.setContrast(OLED_CONTRAST_HIGH);

  u8g2.clearBuffer();
  // u8g2.setFont(u8g2_font_logisoso26_tf); // u8g2_font_logisoso46_tf
  int width = u8g2.getStrWidth("ready!");
  u8g2.drawStr((128 / 2) - (width / 2), (64 / 2) + (26 / 2), "ready!");
  u8g2.sendBuffer();
}
#include <Arduino.h>

/*--------------------------------------------------------------------------------*/

const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

//--------------------------------------------------------------

void setup()
{
	Serial.begin(115200);
  Serial.println("\nStarting Esk8.Board.Server!");
}

void loop() {

  delay(10);
}
// #ifndef tft
// TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
// #endif

//----------------------------------------------------------
void displayPopup(char* message) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(4);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, TFT_HEIGHT/2, TFT_WIDTH/2);
}
//----------------------------------------------------------
#define SCREEN_WIDTH    TFT_HEIGHT
#define SCREEN_HEIGHT   TFT_WIDTH
#define BATTERY_WIDTH   200
#define BATTERY_HEIGHT  90
#define BORDER_SIZE     10
#define KNOB_WIDTH     30

void drawBattery(int percent)
{
    int outsideX = (SCREEN_WIDTH - (BATTERY_WIDTH + BORDER_SIZE)) / 2;
    int outsideY = (SCREEN_HEIGHT - BATTERY_HEIGHT) / 2;
    // clear
    tft.fillScreen(TFT_BLACK);
    // body
    tft.fillRect(outsideX, outsideY, BATTERY_WIDTH, BATTERY_HEIGHT, TFT_WHITE);
    // knob
    tft.fillRect(
        outsideX + BATTERY_WIDTH,
        outsideY + (BATTERY_HEIGHT - KNOB_WIDTH) / 2,
        BORDER_SIZE,
        KNOB_WIDTH,
        TFT_WHITE);
    // body-inside
    tft.fillRect(
        outsideX + BORDER_SIZE,
        outsideY + BORDER_SIZE,
        BATTERY_WIDTH - BORDER_SIZE * 2,
        BATTERY_HEIGHT - BORDER_SIZE * 2,
        TFT_BLACK);
    // capacity
    tft.fillRect(
        outsideX + BORDER_SIZE * 2,
        outsideY + BORDER_SIZE * 2,
        (BATTERY_WIDTH - BORDER_SIZE * 4) * percent / 100,
        BATTERY_HEIGHT - BORDER_SIZE * 4,
        TFT_WHITE);
}
//----------------------------------------------------------

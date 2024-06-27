#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DFRobotDFPlayerMini.h"
#include "OneButton.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define FPSerial Serial1
#define SCREEN_ADDRESS 0x3C
#define button1 34
#define button2 35
#define button3 32

OneButton buttonOne(button1, true);
OneButton buttonTwo(button2, true);
OneButton buttonThree(button3, true);

bool motionDetected = false;
int songNumber = 1;
int volume = 15;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;
DFRobotDFPlayerMini myDFPlayer;

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void initializeDF(){
   Serial.println(F("DFRobot DFPlayer initialization"));

  delay(500);

  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  if (!myDFPlayer.begin(FPSerial, true, true)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true) {
      delay(0);
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.setTimeOut(500); // Set serial communication timeout 500ms

  myDFPlayer.volume(15); // Set volume value (0~30)
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

}

void initializeMPU(){
    while (!Serial) {
    delay(10); // Will pause Zero, Leonardo, etc until serial console opens
  }

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize the MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  // Setup motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(32);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true); // Keep it latched. Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  Serial.println("");
}

void initializeButton(int buttonPin, const char* buttonName) {
  Serial.print("initializing ");
  Serial.println(buttonName);
  pinMode(buttonPin, INPUT);
  if (digitalRead(buttonPin) == HIGH) {
    Serial.print(buttonName);
    Serial.println(" state is HIGH");
  } else {
    Serial.print(buttonName);
    Serial.println(" state is LOW");
  }
}

void drawLoadingBar() {
  int progressBarWidth = 100; // Width of the progress bar
  int progressBarHeight = 10; // Height of the progress bar
  int progressBarX = (SCREEN_WIDTH - progressBarWidth) / 2; // X position of the progress bar
  int progressBarY = (SCREEN_HEIGHT - progressBarHeight) / 2 + 10; // Y position of the progress bar

  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor((SCREEN_WIDTH - 48) / 2, progressBarY - 20); // Center text horizontally, above the progress bar
  display.println(F("Loading"));
  display.display();

  for (int i = 0; i <= 100; i++) {
    // Clear the portion of the screen where the progress bar will be drawn
    display.fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, SSD1306_BLACK);

    // Draw the border of the progress bar
    display.drawRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, SSD1306_WHITE);

    // Draw the filled portion of the progress bar
    display.fillRect(progressBarX, progressBarY, i, progressBarHeight, SSD1306_WHITE);

    // Update the display with the new progress bar state
    display.display();

    // Delay to simulate loading time (adjust as needed)
    delay(5);
  }
}

void displaySongAndVolume() {
  // Clear the display buffer
  display.clearDisplay();

  // Display "Song selected: -"
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((SCREEN_WIDTH - 96) / 2, SCREEN_HEIGHT / 2 - 16);
  display.print(F("Song selected: "));
  display.println(songNumber);

  // Display volume text below the song selection
  int volume = 0; // Initial volume
  display.setCursor((SCREEN_WIDTH - 72) / 2, SCREEN_HEIGHT / 2);
  display.print(F("Volume: "));
  display.print(volume = 15);

  // Update the display with the new content
  display.display();
}

void IRAM_ATTR updateDisplay(int songNumber, int volume) {
  // Clear the display buffer
  display.clearDisplay();

  // Display "Song selected: X"
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((SCREEN_WIDTH - 96) / 2, SCREEN_HEIGHT / 2 - 16);
  display.print(F("Song selected: "));
  display.println(songNumber);

  // Display volume text below the song selection
  display.setCursor((SCREEN_WIDTH - 72) / 2, SCREEN_HEIGHT / 2);
  display.print(F("Volume: "));
  display.print(volume);

  // Update the display with the new content
  display.display();
}

void initializeDisplay(){  
  Serial.println("initializing display");

  delay(100);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.display();
  delay(100); // Pause for 100 ms

  // Clear the buffer
  display.clearDisplay();
  display.display();

  delay(100);

  drawLoadingBar();

  // Clear display after loading bar
  display.clearDisplay();
  display.display();

  // Invert display for 1 second
  display.invertDisplay(true);
  delay(500);
  display.invertDisplay(false);
  delay(500);

  // Display song and volume information
  displaySongAndVolume();

  Serial.println("initializing display done");
}

void selectSong1() {
  songNumber = 1;
  myDFPlayer.volume (0);
  myDFPlayer.playMp3Folder(songNumber);
  myDFPlayer.pause();
  updateDisplay(songNumber, volume);
}

void selectSong2() {
  songNumber = 2;
  myDFPlayer.volume (0);
  myDFPlayer.playMp3Folder(songNumber);
  myDFPlayer.pause();
  updateDisplay(songNumber, volume);
}

void selectSong3() {  
  songNumber = 3;
  myDFPlayer.volume (0);
  myDFPlayer.playMp3Folder(songNumber);
  myDFPlayer.pause();
  updateDisplay(songNumber, volume);
}

void volumeUp() {
  if (volume == 30) {
    volume = 30;
  } else {
    volume = volume + 1;
  }
  updateDisplay(songNumber, volume);
}

void volumeDown() {
  if (volume == 0) {
    volume = 0;
  } else {
    volume = volume - 1;
  }
  updateDisplay(songNumber, volume);
}

void setup() {
  FPSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  initializeDF();

  myDFPlayer.volume(0);
  myDFPlayer.playMp3Folder(songNumber);
  myDFPlayer.pause();
 
  delay(100);

  initializeMPU();

  delay(100);

  initializeButton(button1, "button 1");
  initializeButton(button2, "button 2");
  initializeButton(button3, "button 3");

  buttonOne.attachClick(selectSong1);
  buttonOne.attachDoubleClick(selectSong2);
  buttonOne.attachMultiClick(selectSong3);
  buttonTwo.attachClick(volumeUp);
  buttonThree.attachClick(volumeDown);

  initializeDisplay();

}

void loop() {

  buttonOne.tick();
  buttonTwo.tick();
  buttonThree.tick();


  if(mpu.getMotionInterruptStatus()) {
      Serial.println("Motion detected!");
      digitalWrite(LED_BUILTIN, HIGH);
      myDFPlayer.start();
      myDFPlayer.volume(volume);
      motionDetected = true;
  }

  if(motionDetected)
  {
    delay(1000);
    if(!mpu.getMotionInterruptStatus())
    {
      Serial.println("pausing song");
      digitalWrite(LED_BUILTIN, LOW);
      for (int i = volume; i > 1; i--)
      {
        myDFPlayer.volume(i);;
        delay(100);
      }
      
      myDFPlayer.pause();
      motionDetected = false;
    }
  }


if (myDFPlayer.available()) {
  Serial.println("available");
  printDetail(myDFPlayer.readType(), myDFPlayer.read()); 
  }

  
}
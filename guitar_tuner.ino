#include <arduinoFFT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);

const int buttonPin1 = 2; // button1 to start/stop tuning mode
const int buttonPin2 = 3; // button2 to cycle through options
const int micPin = 0;     // analog pin to read the microphone signal
const int chipSelect = 53; // chip select pin for SD card module

int buttonState1 = 0;
int buttonState2 = 0;

#define SAMPLES 512
#define SAMPLING_FREQUENCY 2048

arduinoFFT FFT = arduinoFFT();

unsigned int samplingPeriod;
unsigned long microSeconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

String tuningNames[] = {"Standard", "DropD", "DropC", "OpenD", "OpenE", "OpenG"};
String selectedTuning;
int tuningIndex = 0;

struct StringFrequency {
  String name;
  float frequency;
};

StringFrequency stringFrequencies[6];
int currentString = 0;
bool tuningMode = false;
bool selectingTuning = false;

void setup() {
  lcd.init();
  lcd.backlight();

  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);

  if (!SD.begin(chipSelect)) {
    lcd.print("SD init failed");
    while (1);
  }

  lcd.setCursor(0, 0);
  lcd.print(centerText("Start tuning"));
  Serial.begin(115200);

  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() {
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);

  if (!tuningMode && !selectingTuning) {
    if (buttonState1 == HIGH) {
      selectingTuning = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Press B1 to stop"));
      lcd.setCursor(0, 1);
      lcd.print(centerText("anytime"));
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Choose tuning:"));
      lcd.setCursor(0, 1);
      lcd.print(centerText(tuningNames[tuningIndex]));
      delay(500);
    }
  } else if (selectingTuning) {
    if (buttonState2 == HIGH) {
      tuningIndex = (tuningIndex + 1) % 6;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(centerText(tuningNames[tuningIndex]));
      delay(500);
    }

    if (buttonState1 == HIGH) {
      selectedTuning = tuningNames[tuningIndex];
      loadTuningFromFile(selectedTuning);
      selectingTuning = false;
      tuningMode = true;
      currentString = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Tuning: "));
      lcd.print(stringFrequencies[currentString].name);
      delay(500);
    }
  } else if (tuningMode) {
    double frequency = calculateFrequency();
    float targetFrequency = stringFrequencies[currentString].frequency;
    lcd.setCursor(0, 1);

    if (frequency < targetFrequency - 10) {
      lcd.print("Too low by ");
      lcd.print(targetFrequency - frequency);
      lcd.print(" Hz");
    } else if (frequency > targetFrequency + 10) {
      lcd.print("Too high by ");
      lcd.print(frequency - targetFrequency);
      lcd.print(" Hz");
    } else {
      lcd.clear();
      lcd.print(centerText("In tune "));
      delay(1000); // wait for 1 second before moving to the next string
      currentString++;
      if (currentString >= 6) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(centerText("Your guitar is"));
        lcd.setCursor(0, 1);
        lcd.print(centerText("tuned!"));
        delay(3000); // Display message for 3 seconds
        currentString = 0;
        tuningMode = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(centerText("Start tuning"));
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(centerText("Tuning: "));
        lcd.print(stringFrequencies[currentString].name);
        delay(500);
      }
    }

    // Check if button 1 is pressed to reset to the start tuning screen
    if (buttonState1 == HIGH) {
      tuningMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Start tuning"));
      delay(500);
    }

    delay(200);
  }
}

double calculateFrequency() {
  for (int i = 0; i < SAMPLES; i++) {
    microSeconds = micros();

    vReal[i] = analogRead(micPin);
    vImag[i] = 0;

    while ((micros() - microSeconds) < samplingPeriod) {
      // nothing
    }
    microSeconds += samplingPeriod;
  }

  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
  Serial.println(peak);
  return peak / 2;
}

void loadTuningFromFile(String tuningName) {
  File tuningFile = SD.open(tuningName + ".txt");
  if (tuningFile) {
    for (int i = 0; i < 6; i++) {
      stringFrequencies[i].name = tuningFile.readStringUntil(',');
      stringFrequencies[i].frequency = tuningFile.readStringUntil('\n').toFloat();
    }
    tuningFile.close();
  } else {
    lcd.clear();
    lcd.print("Failed to open");
    lcd.setCursor(0, 1);
    lcd.print("tuning file");
    while (1);
  }
}

// function to center the text
String centerText(const char* text) {
  int len = strlen(text);
  int pos = (16 - len) / 2;
  String centeredText = "";
  for (int i = 0; i < pos; i++) {
    centeredText += " ";
  }
  centeredText += text;
  return centeredText;
}

String centerText(String text) {
  int len = text.length();
  int pos = (16 - len) / 2;
  String centeredText = "";
  for (int i = 0; i < pos; i++) {
    centeredText += " ";
  }
  centeredText += text;
  return centeredText;
}

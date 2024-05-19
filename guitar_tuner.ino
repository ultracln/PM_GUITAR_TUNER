#include <arduinoFFT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);

const int buttonPin1 = 2; // Button to start/stop tuning mode
const int buttonPin2 = 3; // Button to cycle through strings
const int micPin = 0;     // Analog pin to read the microphone signal

int buttonState1 = 0;
int buttonState2 = 0;

#define SAMPLES 128
#define SAMPLING_FREQUENCY 2048

arduinoFFT FFT = arduinoFFT();

unsigned int samplingPeriod;
unsigned long microSeconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

const float stringFrequencies[6] = {82.41, 110.00, 146.83, 196.00, 246.94, 329.63};
const char* stringNames[6] = {"E", "A", "D", "G", "B", "e"};

int selectedString = 0;
bool tuningMode = false;
bool selectingString = false;

void setup() {
  lcd.init();
  lcd.backlight();

  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);

  lcd.setCursor(0, 0);
  lcd.print(centerText("Start tuning"));
  Serial.begin(115200);

  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() {
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);

  if (!tuningMode && !selectingString) {
    if (buttonState1 == HIGH) {
      selectingString = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Press B1 to stop"));
      lcd.setCursor(0, 1);
      lcd.print(centerText("anytime"));
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Select string:"));
      lcd.setCursor(0, 1);
      lcd.print(centerText(stringNames[selectedString]));
      delay(500);
    }
  } else if (selectingString) {
    if (buttonState2 == HIGH) {
      selectedString = (selectedString + 1) % 6;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(centerText(stringNames[selectedString]));
      delay(500);
    }

    if (buttonState1 == HIGH) {
      selectingString = false;
      tuningMode = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Tuning: "));
      lcd.print(stringNames[selectedString]);
      delay(500);
    }
  } else if (tuningMode) {
    double frequency = calculateFrequency();
    float targetFrequency = stringFrequencies[selectedString];
    lcd.setCursor(0, 1);

    if (frequency < targetFrequency - 5) {
      lcd.print(centerText("Too low "));
    } else if (frequency > targetFrequency + 5) {
      lcd.print(centerText("Too high"));
    } else {
      lcd.print(centerText("In tune "));
      tuningMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(centerText("Tuned!"));
      lcd.setCursor(0, 1);
      lcd.print(centerText("Wait to continue..."));
      delay(3000); // Wait for 3 seconds

      selectedString++;
      if (selectedString >= 6) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(centerText("Your guitar is"));
        lcd.setCursor(0, 1);
        lcd.print(centerText("tuned!"));
        delay(3000); // Display message for 3 seconds
        selectedString = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(centerText("Start tuning"));
      } else {
        tuningMode = true; // Resume tuning for the next string
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(centerText("Tuning: "));
        lcd.print(stringNames[selectedString]);
        delay(500);
      }
    }

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

    while (micros() < (microSeconds + samplingPeriod)) {
      // nothing
    }
  }

  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
  Serial.println(peak);
  return peak;
}

// Function to center the text
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

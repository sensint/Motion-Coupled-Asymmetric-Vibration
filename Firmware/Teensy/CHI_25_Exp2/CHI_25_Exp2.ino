#include <Wire.h>
// #include <elapsedMillis.h>
#include <Audio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define VERSION "v1.1.0"

enum class Waveform : short {
  kSine = 0,
  kSawtooth = 1,
  kSquare = 2,
  kTriangle = 3,
  kArbitrary = 4,
  kPulse = 5,
  kSawtoothReverse = 6,
  kSampleHold = 7,
  kTriangleVariable = 8,
  kBandlimitSawtooth = 9,
  kBandlimitSawtoothReverse = 10,
  kBandlimitSquare = 11,
  kBandlimitPulse = 12
};

char selectedMode = '\0';

//=========== audio variables ===========
AudioSynthWaveform signal;
AudioOutputPT8211 dac;
AudioConnection patchCord1(signal, 0, dac, 0);
AudioConnection patchCord2(signal, 0, dac, 1);

//=========== control flow variables ===========
float kFilterWeight = 0.5;
float kSensorMinValue = 0;
float kSensorMaxValue = 100;
elapsedMicros pulse_time_us = 0;
bool is_vibrating = false;
uint16_t mapped_bin_id = 0;
uint16_t last_bin_id = 0;

// ========= Sensing ================
float filtered_sensor_value_1 = 0.f;
float filtered_sensor_value_2 = 0.f;
unsigned long measuredDistance_1;
unsigned long measuredDistance_2;
float filtered_sensor_value = 0.f;
float filtered_sensor_value_new = 0.f;
float last_triggered_sensor_val = 0.f;
unsigned long measuredDistance;
int sensorSamplingFrequency = 100;

//=========== signal generator ===========
static uint16_t kNumberOfBins = 100;
static constexpr short kSignalWaveform = static_cast<short>(Waveform::kArbitrary);
static uint32_t kSignalDurationUs = 25 * 1000;  // in microseconds
static float kSignalFrequencyHz = 40.f;
float kSignalAsymAmp = 1.f;
static constexpr float kSignalContinuousAmp = 1.0f;
const int16_t dat[256] = { -19754, -19235, -18393, -17264, -15891, -14325, -12622, -10836, -9021, -7223, -5485, -3840, -2314, -924, 317, 1405, 2341, 3129, 3776, 4293, 4692, 4983, 5180, 5295, 5339, 5322, 5255, 5147, 5006, 4838, 4651, 4449, 4236, 4018, 3796, 3574, 3355, 3139, 2929, 2725, 2528, 2339, 2159, 1987, 1825, 1671, 1527, 1391, 1265, 1146, 1036, 935, 841, 754, 675, 602, 537, 477, 424, 376, 334, 296, 264, 236, 213, 193, 178, 166, 157, 152, 149, 149, 152, 157, 164, 174, 185, 198, 212, 228, 246, 264, 283, 304, 325, 347, 369, 392, 415, 439, 463, 487, 511, 536, 560, 584, 608, 632, 655, 678, 701, 723, 745, 766, 787, 807, 826, 845, 863, 881, 897, 913, 928, 943, 956, 969, 980, 991, 1001, 1010, 1018, 1026, 1032, 1037, 1041, 1045, 1047, 1049, 1049, 1049, 1047, 1045, 1041, 1037, 1032, 1026, 1018, 1010, 1001, 991, 980, 969, 956, 943, 928, 913, 897, 881, 863, 845, 826, 807, 787, 766, 745, 723, 701, 678, 655, 632, 608, 584, 560, 536, 511, 487, 463, 439, 415, 392, 369, 347, 325, 304, 283, 264, 246, 228, 212, 198, 185, 174, 164, 157, 152, 149, 149, 152, 157, 166, 178, 193, 213, 236, 264, 296, 334, 376, 424, 477, 537, 602, 675, 754, 841, 935, 1036, 1146, 1265, 1391, 1527, 1671, 1825, 1987, 2159, 2339, 2528, 2725, 2929, 3139, 3355, 3574, 3796, 4018, 4236, 4449, 4651, 4838, 5006, 5147, 5255, 5322, 5339, 5295, 5180, 4983, 4692, 4293, 3776, 3129, 2341, 1405, 317, -924, -2314, -3840, -5485, -7223, -9021, -10836, -12622, -14325, -15891, -17264, -18393, -19235 };
int16_t negDat[256];
int16_t negDatTrial[256];

// Timing and control variables for pseudo forces
unsigned long previousPseudoForcesMillis = 0;
int repetitionCountPseudoForces = 0;
int stepPseudoForces = 0;  // Step variable to track the current step in the pseudo forces sequence
const long kContinuousVibrationDuration = 2500;
uint16_t kpseudoForceRepetition = 1;
uint16_t kpseudoForceRepetitionTrial = 1;
const long kNoVibrationDuration = 100;

static constexpr int kBaudRate = 9600;

//============= UNITY communication ===========
struct ParsedData {
  char scene;
  int algorithm;
  int state;
  float value;
  float multiplier;
};

ParsedData parseSerialData(const String &data) {
  ParsedData parsedData = { '\0', 0, 0, 0.0f };

  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);
  int thirdComma = data.indexOf(',', secondComma + 1);
  int fourthComma = data.indexOf(',', thirdComma + 1);

  if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
    parsedData.scene = data.charAt(0);  // Extract first character as Scene ID
    parsedData.algorithm = data.substring(firstComma + 1, secondComma).toInt();
    parsedData.state = data.substring(secondComma + 1, thirdComma).toInt();
    parsedData.value = data.substring(thirdComma + 1).toFloat();
    parsedData.multiplier = data.substring(fourthComma + 1).toFloat();
  }

  return parsedData;
}

void SetupSerial() {
  while (!Serial && millis() < 2000)
    ;
  Serial.begin(kBaudRate);
  delay(500);
}

void SetupAudio() {
  AudioMemory(20);
  delay(50);  // time for DAC voltage stable
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(dat, 170);
  signal.frequency(kSignalFrequencyHz);
}

void StartPulsePosPF() {
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(dat, 170);
  signal.frequency(kSignalFrequencyHz);
  signal.phase(0.0);
  signal.amplitude(kSignalAsymAmp);
  pulse_time_us = 0;
  is_vibrating = true;
  // Serial.printf("Start Pos pulse \n\t bins: %d", mapped_bin_id);
  // Serial.println(F("=====================================================\n\n"));
}

void StartPulseRepelPF(){
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(negDatTrial, 170);
  signal.frequency(kSignalFrequencyHz);
  signal.phase(0.0);
  signal.amplitude(kSignalAsymAmp);
  pulse_time_us = 0;
  is_vibrating = true;
  // Serial.printf("Start Pos pulse \n\t bins: %d", mapped_bin_id);
  // Serial.println(F("=====================================================\n\n"));
}

void StartPulseNegPF() {
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(negDat, 170);
  signal.frequency(kSignalFrequencyHz);
  signal.phase(0.0);
  signal.amplitude(kSignalAsymAmp);
  pulse_time_us = 0;
  is_vibrating = true;
  // Serial.printf("Start Neg pulse \n\t bins: %d", mapped_bin_id);
  // Serial.println(F("=====================================================\n\n"));
}

void StopPulse() {
  signal.amplitude(0.f);
  is_vibrating = false;
  // Serial.println("Stop pulse");
}

void GeneratePseudoForcesSimple() {
  for (int i = 0; i < 3; i++) {
    signal.begin(kSignalWaveform);
    signal.arbitraryWaveform(dat, 170);
    signal.frequency(kSignalFrequencyHz);
    signal.amplitude(kSignalAsymAmp);
    delay(2500);
    signal.amplitude(0);
    delay(500);
    signal.begin(kSignalWaveform);
    signal.arbitraryWaveform(negDatTrial, 170);
    signal.frequency(kSignalFrequencyHz);
    signal.amplitude(kSignalAsymAmp);
    delay(2500);
    signal.amplitude(0);
    delay(500);
    i++;
  }
}

void GeneratePseudoForces() {
  unsigned long currentMillis = millis();

  switch (stepPseudoForces) {
    case 0:  // Start positive vibration
      if (repetitionCountPseudoForces < kpseudoForceRepetitionTrial) {
        signal.begin(kSignalWaveform);
        signal.arbitraryWaveform(dat, 170);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 1;
        is_vibrating = true;
        // Serial.println("Pseudo Forces: Starting positive vibration");
      }
      break;

    case 1:  // End positive vibration
      if (currentMillis - previousPseudoForcesMillis >= kContinuousVibrationDuration) {
        signal.frequency(0);
        signal.amplitude(0);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 2;
        is_vibrating = false;
      }
      break;

    case 2:  // Pause after positive vibration
      if (currentMillis - previousPseudoForcesMillis >= kNoVibrationDuration) {
        signal.begin(kSignalWaveform);
        signal.arbitraryWaveform(negDatTrial, 170);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 3;
        // Serial.println("Pseudo Forces: Starting negative vibration");
        is_vibrating = true;
      }
      break;

    case 3:  // End negative vibration
      if (currentMillis - previousPseudoForcesMillis >= kContinuousVibrationDuration) {
        signal.frequency(0);
        signal.amplitude(0);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 4;
        is_vibrating = false;
      }
      break;

    case 4:  // Pause after negative vibration
      if (currentMillis - previousPseudoForcesMillis >= kNoVibrationDuration) {
        repetitionCountPseudoForces = 0;
        stepPseudoForces = 0;
      }
      break;

    default:  // All repetitions complete
      if (repetitionCountPseudoForces >= kpseudoForceRepetitionTrial) {
        // Serial.println("Pseudo Forces: All repetitions complete");
      }
      break;
  }
}

void GeneratePseudoForcesBasic() {
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(dat, 170);
  signal.frequency(kSignalFrequencyHz);
  signal.amplitude(kSignalAsymAmp);
}

void GeneratePseudoForcesBasicRepel(){
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(negDatTrial, 170);
  signal.frequency(kSignalFrequencyHz);
  signal.amplitude(kSignalAsymAmp);
}

void GenerateMotionCoupledPseudoForces() {
  if (mapped_bin_id < last_bin_id) {
    // Uncomment below to stop the current vibration and play the next one.
    // if (is_vibrating) {  // This loop is for the case when we want to stop the ongoing vibration and start the next one.
    //   StopPulse();
    //   delayMicroseconds(1);
    // }

    StartPulsePosPF();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  } else if (mapped_bin_id > last_bin_id) {
    // if (is_vibrating) {  // This loop is for the case when we want to stop the ongoing vibration and start the next one.
    //   StopPulse();
    //   delayMicroseconds(1);
    // }

    StartPulseNegPF();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  }

  if (is_vibrating && pulse_time_us >= kSignalDurationUs) {
    StopPulse();
  }
}

void GenerateMotionCoupledPseudoForcesRepel(){
  if (mapped_bin_id < last_bin_id) {
    StartPulseRepelPF();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  } else if (mapped_bin_id > last_bin_id) {
    StartPulseRepelPF();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  }

  if (is_vibrating && pulse_time_us >= kSignalDurationUs) {
    StopPulse();
  }
}

void DoNothing() {
  StopPulse();
  Serial.println("Base State");
}

void BowArrow(const ParsedData &parsedData) {
  switch (parsedData.algorithm) {
    case 0:  // Controller
      switch (parsedData.state) {
        case 0:  // No Stretch
          DoNothing();
          break;
        case 1:
          Serial.print("BowArrow - Controller - Stretched with percent stretch: ");
          Serial.println(parsedData.value);
          break;
      }
      break;
    case 1:  // Continuous pseudo forces
      switch (parsedData.state) {
        case 0:
          DoNothing();
          break;
        case 1:
          Serial.print("BowArrow - CPF - Stretched with percent stretch: ");
          Serial.println(parsedData.value);
          kSignalAsymAmp = parsedData.value;
          signal.amplitude(kSignalAsymAmp);
          GeneratePseudoForcesBasic();
          break;
      }
      break;
    case 2:  // Motion-Coupled pseudo forces
      switch (parsedData.state) {
        case 0:
          DoNothing();
          break;
        case 1:
          Serial.print("BowArrow - MCPF - Stretched with percent stretch: ");
          Serial.println(parsedData.value);
          kSignalAsymAmp = parsedData.value;
          signal.amplitude(kSignalAsymAmp);
          StopPulse();
          GenerateMotionCoupledPseudoForces();
          break;
      }
      break;
    default:
      Serial.println("BowArrow - Unknown Algorithm");
      break;
  }
}

void WeightsInBoxes(const ParsedData &parsedData) {
  switch (parsedData.algorithm) {
    case 0:  // Controller
      switch (parsedData.state) {
        case 0:  // No Stretch
          DoNothing();
          break;
        case 1:
          Serial.print("WeightsInBoxes - Controller - Lifted with percent lift: ");
          Serial.println(parsedData.value);
          break;
      }
      break;
    case 1:  // Continuous pseudo forces
      switch (parsedData.state) {
        case 0:
          DoNothing();
          break;
        case 1:
          Serial.print("WeightsInBoxes - CPF - Lifted with percent lift: ");
          Serial.println(parsedData.value);
          if (parsedData.value == 0.25){kSignalAsymAmp = 0.4;}
          if (parsedData.value == 0.5){kSignalAsymAmp = 0.7;}
          if (parsedData.value == 1.0){kSignalAsymAmp = 1.0;}
          // kSignalAsymAmp = parsedData.value;  // If linear mapped
          signal.amplitude(kSignalAsymAmp);
          GeneratePseudoForcesBasic();
          break;
      }
      break;
    case 2:  // Motion-Coupled pseudo forces
      switch (parsedData.state) {
        case 0:
          DoNothing();
          break;
        case 1:
          Serial.print("WeightsInBoxes - MCPF - Lifted with percent lift: ");
          Serial.println(parsedData.value);
          if (parsedData.value == 0.25){kSignalAsymAmp = 0.4;}
          if (parsedData.value == 0.5){kSignalAsymAmp = 0.7;}
          if (parsedData.value == 1.0){kSignalAsymAmp = 1.0;}
          // kSignalAsymAmp = parsedData.value; // If linear mapped
          signal.amplitude(kSignalAsymAmp);
          StopPulse();
          GenerateMotionCoupledPseudoForces();
          break;
      }
      break;
    default:
      Serial.println("WeightsInBoxes - Unknown Algorithm");
      break;
  }
}

void HapticMagnets(const ParsedData &parsedData) {
  // If either hand is flipped, the magents would repel.
  // If both hands are flipped, the magnets would attract again.

  // float a = -2.0;
  // float b = 3.0;
  // float c = -1.0;
  // float d = 0.5;

  // float normalizedDistance = parsedData.value;

  switch (parsedData.algorithm) {
    case 0:  // Controller
      switch (parsedData.state) {
        case 0:  // No Stretch
          DoNothing();
          break;
        case 1:
          Serial.print("Magnets - Controller - Repel with distance: ");
          Serial.println(parsedData.value);
          break;
        case 2:
          Serial.print("Magnets - Controller - Attract with distance: ");
          Serial.println(parsedData.value);
          break;
      }
      break;
    case 1:  // Continuous pseudo forces
      switch (parsedData.state) {
        case 0:
          DoNothing();
          break;
        case 1:
          Serial.print("Magnets - CPF - Repel with distance: ");
          Serial.println(parsedData.value);
          kSignalAsymAmp = parsedData.value;  // If linear mapped
          // kSignalAsymAmp = a * pow(normalizedDistance, 3) + b * pow(normalizedDistance, 2) + c * normalizedDistance + d; // Cubic Mapped
          signal.amplitude(kSignalAsymAmp);
          GeneratePseudoForcesBasicRepel();
          
          break;
        case 2:
          Serial.print("Magnets - CPF - Attract with distance: ");
          Serial.println(parsedData.value);
          kSignalAsymAmp = parsedData.value;  // If linear mapped
          // kSignalAsymAmp = a * pow(normalizedDistance, 3) + b * pow(normalizedDistance, 2) + c * normalizedDistance + d; // Cubic Mapped
          signal.amplitude(kSignalAsymAmp);
          GeneratePseudoForcesBasic();
          break;
      }
      break;
    case 2:  // Motion-Coupled pseudo forces
      switch (parsedData.state) {
        case 0:
          DoNothing();
          break;
        case 1:
          Serial.print("Magnets - CPF - Repel with distance: ");
          Serial.println(parsedData.value);
          kSignalAsymAmp = parsedData.value; // If linear mapped
          // kSignalAsymAmp = a * pow(normalizedDistance, 3) + b * pow(normalizedDistance, 2) + c * normalizedDistance + d; // Cubic Mapped
          signal.amplitude(kSignalAsymAmp);
          StopPulse();
          GenerateMotionCoupledPseudoForcesRepel();
          break;
        case 2:
          Serial.print("Magnets - CPF - Attract with distance: ");
          Serial.println(parsedData.value);
          kSignalAsymAmp = parsedData.value; // If linear mapped
          // kSignalAsymAmp = a * pow(normalizedDistance, 3) + b * pow(normalizedDistance, 2) + c * normalizedDistance + d; // Cubic Mapped
          signal.amplitude(kSignalAsymAmp);
          StopPulse();
          GenerateMotionCoupledPseudoForces();
          break;
      }
      break;
    default:
      Serial.println("Magnets - Unknown Algorithm");
      break;
  }
}

void LinDisp2Amp3() {
  // Displacement linearly mapped to amplitude
  kNumberOfBins = 200;  // This one works better with higher bin numbers.
  float kSignalAsymAmpMin = 0.4;
  float kSignalAsymAmpMax = 1.0;

  if (filtered_sensor_value < kSensorMinValue) {
    kSignalAsymAmp = 0;
  } else if (filtered_sensor_value >= kSensorMaxValue) {
    kSignalAsymAmp = kSignalAsymAmpMax;
  } else {
    kSignalAsymAmp = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, kSignalAsymAmpMin, kSignalAsymAmpMax);
  }

  signal.amplitude(kSignalAsymAmp);
  StopPulse();

  Serial.print("Adjusted amplitude based on distance: ");
  Serial.println(kSignalAsymAmp);
  GenerateMotionCoupledPseudoForces();
}

void setup() {
  SetupSerial();
  SetupAudio();
  for (int i = 0; i < 256; i++) {
    negDat[i] = dat[i];
    negDatTrial[i] = -dat[i];
  }
}

void loop() {

  if (Serial.available()) {

    String data = Serial.readStringUntil('\n');
    if (data == "d") {
      Serial.println("Do Nothing");
      delay(10);
      StopPulse();
      return;
    } else if (data == "e") {
      GeneratePseudoForcesSimple();
      Serial.println("Playing Exposure Condition");
      return;
    }

    ParsedData parsedData = parseSerialData(data);
    measuredDistance = parsedData.value * 100;
    mapped_bin_id = map(measuredDistance, kSensorMinValue, kSensorMaxValue, 0, kNumberOfBins);

    switch (parsedData.scene) {
      case 'B':  // Bow Arrow
        BowArrow(parsedData);
        break;
      case 'M':  // Magnets
        HapticMagnets(parsedData);
        break;
      case 'W':  // Weight in Boxes
        WeightsInBoxes(parsedData);
        break;
      default:
        Serial.println("Unknown Scene Identifier");
        // StopPulse();
        // signal.amplitude(0);
        break;
    }
  }
}

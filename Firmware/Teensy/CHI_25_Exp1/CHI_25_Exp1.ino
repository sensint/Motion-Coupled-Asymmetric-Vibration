#include <Wire.h>
// #include <elapsedMillis.h>
#include <Audio.h>
#include <vl53l4cd_class.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// TO DO MAIN:
// Change the max and min of sensor to what the sensor actually measures
// Urgent: Check if you can modulate the pseudo forces
// Looking at different mappings
// To decide whether to stop the current pulse and play the next or just to play the next after finishing the current pulse

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

enum Mode {
  MODE_1,     // Motion-Coupled Pseudo Forces
  MODE_2,     // record-play with movement
  MODE_3,     // record-play without movement
  MODE_4,     // Continous pseudo forces with movement
  MODE_5,     // Continous pseudo forces without movement
  MODE_Debug  // Debug Mode
};

Mode currentMode = MODE_4;  // Initialize current mode to MODE_3

//=========== Laser Sensing ===========
#define DEV_I2C Wire2
VL53L4CD sensor_vl53l4cd_sat(&DEV_I2C, A1);
float filtered_sensor_value = 0.f;
float last_triggered_sensor_val = 0.f;
float measuredDistance;

//=========== Laser Sensing Constants ===========
static constexpr float kFilterWeight = 0.7;
static constexpr uint32_t kSensorMinValue = 0;
static constexpr uint32_t kSensorMaxValue = 830;
static constexpr uint32_t kSensorJitterThreshold = 30;

//=========== audio variables ===========
AudioSynthWaveform signal;
AudioOutputPT8211 dac;
AudioConnection patchCord1(signal, 0, dac, 0);
AudioConnection patchCord2(signal, 0, dac, 1);

//=========== control flow variables ===========
elapsedMicros pulse_time_us = 0;
bool is_vibrating = false;
uint16_t mapped_bin_id = 0;
uint16_t last_bin_id = 0;
bool augmentation_enabled = false;
bool recording_enabled = false;

//=========== signal generator ===========
static uint16_t kNumberOfBins = 50;
static constexpr short kSignalWaveform = static_cast<short>(Waveform::kArbitrary);
static constexpr short kSignalContinuousWaveform = static_cast<short>(Waveform::kSawtooth);
static constexpr short kSignalContinuousWaveform2 = static_cast<short>(Waveform::kSawtoothReverse);
static uint32_t kSignalDurationUs = 25 * 1000;  // in microseconds
static float kSignalFrequencyHz = 40.f;
static float kSignalAsymAmp = 1.f;
static constexpr float kSignalContinuousAmp = 1.0f;
const int16_t dat[256] = { -19754, -19235, -18393, -17264, -15891, -14325, -12622, -10836, -9021, -7223, -5485, -3840, -2314, -924, 317, 1405, 2341, 3129, 3776, 4293, 4692, 4983, 5180, 5295, 5339, 5322, 5255, 5147, 5006, 4838, 4651, 4449, 4236, 4018, 3796, 3574, 3355, 3139, 2929, 2725, 2528, 2339, 2159, 1987, 1825, 1671, 1527, 1391, 1265, 1146, 1036, 935, 841, 754, 675, 602, 537, 477, 424, 376, 334, 296, 264, 236, 213, 193, 178, 166, 157, 152, 149, 149, 152, 157, 164, 174, 185, 198, 212, 228, 246, 264, 283, 304, 325, 347, 369, 392, 415, 439, 463, 487, 511, 536, 560, 584, 608, 632, 655, 678, 701, 723, 745, 766, 787, 807, 826, 845, 863, 881, 897, 913, 928, 943, 956, 969, 980, 991, 1001, 1010, 1018, 1026, 1032, 1037, 1041, 1045, 1047, 1049, 1049, 1049, 1047, 1045, 1041, 1037, 1032, 1026, 1018, 1010, 1001, 991, 980, 969, 956, 943, 928, 913, 897, 881, 863, 845, 826, 807, 787, 766, 745, 723, 701, 678, 655, 632, 608, 584, 560, 536, 511, 487, 463, 439, 415, 392, 369, 347, 325, 304, 283, 264, 246, 228, 212, 198, 185, 174, 164, 157, 152, 149, 149, 152, 157, 166, 178, 193, 213, 236, 264, 296, 334, 376, 424, 477, 537, 602, 675, 754, 841, 935, 1036, 1146, 1265, 1391, 1527, 1671, 1825, 1987, 2159, 2339, 2528, 2725, 2929, 3139, 3355, 3574, 3796, 4018, 4236, 4449, 4651, 4838, 5006, 5147, 5255, 5322, 5339, 5295, 5180, 4983, 4692, 4293, 3776, 3129, 2341, 1405, 317, -924, -2314, -3840, -5485, -7223, -9021, -10836, -12622, -14325, -15891, -17264, -18393, -19235 };
int16_t negDat[256];

#define Amplitude_ARRAYSIZE 2  // Change if we decide on 3 levels of amplitude
// float receivedInts[Amplitude_ARRAYSIZE] = { 0.4, 0.7, 1.0 };
float receivedInts[Amplitude_ARRAYSIZE] = { 0.5, 1.0 };

//=========== Continuous Vibration ===========
const long kContinuousVibrationDuration = 3000;
uint16_t kpseudoForceRepetition = 3;
const long kNoVibrationDuration = 1000;
uint16_t kContinuousVibrationRepetition = 3;
unsigned long previousMillis = 0;

// Timing and control variables for pseudo forces
unsigned long previousPseudoForcesMillis = 0;
int repetitionCountPseudoForces = 0;
int stepPseudoForces = 0;  // Step variable to track the current step in the pseudo forces sequence

// Timing and control variables for continuous vibration
unsigned long previousContinuousVibrationMillis = 0;
int repetitionCountContinuousVibration = 0;
bool is_vibrating_continuous = false;

//=========== serial ===========
static constexpr int kBaudRate = 115200;

//=========== helper functions ===========
inline void SetupSerial() __attribute__((always_inline));

void SetupSerial() {
  while (!Serial && millis() < 2000)
    ;
  Serial.begin(kBaudRate);
  delay(500);
}

void InitializeSensor() {
  DEV_I2C.begin();                                      // Initialize I2C bus.
  sensor_vl53l4cd_sat.begin();                          // Configure VL53L4CD satellite component.
  sensor_vl53l4cd_sat.VL53L4CD_Off();                   // Switch off VL53L4CD satellite component.
  sensor_vl53l4cd_sat.InitSensor();                     //Initialize VL53L4CD satellite component.
  sensor_vl53l4cd_sat.VL53L4CD_SetRangeTiming(300, 0);  // Program the highest possible TimingBudget, without enabling the low power mode. This should give the best accuracy
  sensor_vl53l4cd_sat.VL53L4CD_StartRanging();          // Start Measurements
}

void SetupAudio() {
  AudioMemory(20);
  delay(50);  // time for DAC voltage stable
  signal.begin(WAVEFORM_SINE);
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

void StartPulseCV() {
  signal.begin(WAVEFORM_SINE);
  signal.frequency(kSignalFrequencyHz);
  signal.phase(0.0);
  signal.amplitude(kSignalContinuousAmp);
  pulse_time_us = 0;
  is_vibrating = true;
  // Serial.printf("Start continuous pulse \n\t bins: %d", mapped_bin_id);
  // Serial.println(F("=====================================================\n\n"));
}

void StopPulse() {
  signal.amplitude(0.f);
  is_vibrating = false;
  // Serial.println("Stop pulse");
}

void GeneratePseudoForces() {
  unsigned long currentMillis = millis();

  switch (stepPseudoForces) {
    case 0:  // Start positive vibration
      if (repetitionCountPseudoForces < kpseudoForceRepetition) {
        signal.begin(kSignalWaveform);
        signal.arbitraryWaveform(dat, 170);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 1;
        // Serial.println("Pseudo Forces: Starting positive vibration");
      }
      break;

    case 1:  // End positive vibration
      if (currentMillis - previousPseudoForcesMillis >= kContinuousVibrationDuration) {
        signal.frequency(0);
        signal.amplitude(0);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 2;
      }
      break;

    case 2:  // Pause after positive vibration
      if (currentMillis - previousPseudoForcesMillis >= kNoVibrationDuration) {
        signal.begin(kSignalWaveform);
        signal.arbitraryWaveform(negDat, 170);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 3;
        // Serial.println("Pseudo Forces: Starting negative vibration");
      }
      break;

    case 3:  // End negative vibration
      if (currentMillis - previousPseudoForcesMillis >= kContinuousVibrationDuration) {
        signal.frequency(0);
        signal.amplitude(0);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 4;
      }
      break;

    case 4:  // Pause after negative vibration
      if (currentMillis - previousPseudoForcesMillis >= kNoVibrationDuration) {
        repetitionCountPseudoForces = 0;
        stepPseudoForces = 0;
      }
      break;

    default:  // All repetitions complete
      if (repetitionCountPseudoForces >= kpseudoForceRepetition) {
        Serial.println("Pseudo Forces: All repetitions complete");
      }
      break;
  }
}

void GenerateContinuousVibration() {
  unsigned long currentMillis = millis();

  if (!is_vibrating_continuous) {
    if (repetitionCountContinuousVibration < kContinuousVibrationRepetition) {
      if (currentMillis - previousContinuousVibrationMillis >= kContinuousVibrationDuration) {
        signal.begin(WAVEFORM_SINE);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalContinuousAmp);
        previousContinuousVibrationMillis = currentMillis;
        is_vibrating_continuous = true;
        // Serial.println("Continuous Vibration: Start");
      }
    } else {
      repetitionCountContinuousVibration = 0;
    }
  } else {
    if (currentMillis - previousContinuousVibrationMillis >= kContinuousVibrationDuration) {
      signal.frequency(0);
      signal.amplitude(0);
      previousContinuousVibrationMillis = currentMillis;
      delay(kNoVibrationDuration);  // Delay for the pause
      is_vibrating_continuous = false;
      repetitionCountContinuousVibration++;
      // Serial.println("Continuous Vibration: Stop");
    }
  }
}

void GenerateMotionCoupledPseudoForces() {
  if (mapped_bin_id < last_bin_id) {
    // Uncomment below to stop the current vibration and play the next one.
    if (is_vibrating) {  // This loop is for the case when we want to stop the ongoing vibration and start the next one.
      StopPulse();
      delayMicroseconds(100);
    }

    StartPulsePosPF();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  }

  if (mapped_bin_id > last_bin_id) {
    if (is_vibrating) {  // This loop is for the case when we want to stop the ongoing vibration and start the next one.
      StopPulse();
      delayMicroseconds(100);
    }

    StartPulseNegPF();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  }

  if (is_vibrating && pulse_time_us >= kSignalDurationUs) {
    StopPulse();
  }
}

void GenerateMotionCoupledVibration() {
  if (mapped_bin_id != last_bin_id) {
    // Uncomment below to stop the current vibration and play the next one.
    if (is_vibrating) {  // This loop is for the case when we want to stop the ongoing vibration and start the next one.
      StopPulse();
      delayMicroseconds(100);
    }

    StartPulseCV();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  }

  if (is_vibrating && pulse_time_us >= kSignalDurationUs) {
    StopPulse();
  }
}

void MappingFunction() {
  filtered_sensor_value = (1.f - kFilterWeight) * filtered_sensor_value + (kFilterWeight)*measuredDistance;
  mapped_bin_id = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, 0, kNumberOfBins);
  // Serial.println(mapped_bin_id);

  auto dist = abs((float)(filtered_sensor_value - last_triggered_sensor_val));
  if (dist < kSensorJitterThreshold) {
    return;
  }
}

void setup() {
  SetupSerial();
  InitializeSensor();
  SetupAudio();
  for (int i = 0; i < 256; i++) {
    negDat[i] = -dat[i];
  }
}

void loop() {
  uint8_t NewDataReady = 0;
  VL53L4CD_Result_t results;
  uint8_t status;
  char report[64];

  do {
    status = sensor_vl53l4cd_sat.VL53L4CD_CheckForDataReady(&NewDataReady);
  } while (!NewDataReady);

  if ((!status) && (NewDataReady != 0)) {
    // (Mandatory) Clear HW interrupt to restart measurements
    sensor_vl53l4cd_sat.VL53L4CD_ClearInterrupt();

    // Read measured distance. RangeStatus = 0 means valid data
    sensor_vl53l4cd_sat.VL53L4CD_GetResult(&results);
  }

  measuredDistance = results.distance_mm;  // make this fast
  MappingFunction();

  if (Serial.available()) {
    auto serial_c = (char)Serial.read();
    switch (serial_c) {
      case 'a': {
          currentMode = MODE_1;
          // Serial.println("Mode 1: Continuous Pseudo Forces");
          if (Serial.available()) {
            char amplitude_char = Serial.read();
            switch (amplitude_char) {
              case '0':
                kSignalAsymAmp = receivedInts[0];
                // Serial.println("Setting Amplitude to 0.5");
                signal.amplitude(kSignalAsymAmp);
                break;
              case '1':
                kSignalAsymAmp = receivedInts[1];
                // Serial.println("Setting Amplitude to 1.0");
                signal.amplitude(kSignalAsymAmp);  // Assuming signal is defined elsewhere
                break;
              default:
                // Serial.println("Invalid Amplitude Command");
                break;
            }
          }
        break;
    }
    case 'b': {
          currentMode = MODE_2;
          // Serial.println("Mode 2: Continuous Vibration");
          if (Serial.available()) {
            char amplitude_char = Serial.read();
            switch (amplitude_char) {
              case '0':
                kSignalAsymAmp = receivedInts[0];
                // Serial.println("Setting Amplitude to 0.5");
                signal.amplitude(kSignalAsymAmp);
                break;
              case '1':
                kSignalAsymAmp = receivedInts[1];
                // Serial.println("Setting Amplitude to 1.0");
                signal.amplitude(kSignalAsymAmp);  // Assuming signal is defined elsewhere
                break;
              default:
                // Serial.println("Invalid Amplitude Command");
                break;
            }
          }
        break;
    }
    case 'c': {
          currentMode = MODE_3;
          // Serial.println("Mode 3: Motion Coupled Pseudo Forces");
          if (Serial.available()) {
            char amplitude_char = Serial.read();
            switch (amplitude_char) {
              case '0':
                kSignalAsymAmp = receivedInts[0];
                // Serial.println("Setting Amplitude to 0.5");
                signal.amplitude(kSignalAsymAmp);
                break;
              case '1':
                kSignalAsymAmp = receivedInts[1];
                // Serial.println("Setting Amplitude to 1.0");
                signal.amplitude(kSignalAsymAmp);  // Assuming signal is defined elsewhere
                break;
              default:
                // Serial.println("Invalid Amplitude Command");
                break;
            }
          }
        break;
    }
    case 'd': {
          currentMode = MODE_4;
          // Serial.println("Mode 4: Motion Coupled Vibration");
          if (Serial.available()) {
            char amplitude_char = Serial.read();
            switch (amplitude_char) {
              case '0':
                kSignalAsymAmp = receivedInts[0];
                // Serial.println("Setting Amplitude to 0.5");
                signal.amplitude(kSignalAsymAmp);
                break;
              case '1':
                kSignalAsymAmp = receivedInts[1];
                // Serial.println("Setting Amplitude to 1.0");
                signal.amplitude(kSignalAsymAmp);  // Assuming signal is defined elsewhere
                break;
              default:
                // Serial.println("Invalid Amplitude Command");
                break;
            }
          }
        break;
    }
    case 'e': {
          currentMode = MODE_5;
          // Serial.println("Mode 5: Random");
          if (Serial.available()) {
            char amplitude_char = Serial.read();
            switch (amplitude_char) {
              case '0':
                kSignalAsymAmp = receivedInts[0];
                // Serial.println("Setting Amplitude to 0.5");
                signal.amplitude(kSignalAsymAmp);
                break;
              case '1':
                kSignalAsymAmp = receivedInts[1];
                // Serial.println("Setting Amplitude to 1.0");
                signal.amplitude(kSignalAsymAmp);  // Assuming signal is defined elsewhere
                break;
              default:
                // Serial.println("Invalid Amplitude Command");
                break;
            }
          }
        break;
    }
    case 'r':
      currentMode = MODE_Debug;
      Serial.println("Debug Mode");
      break;
    case 'x':
      {
        if (Serial.available()) {
          kSignalFrequencyHz = Serial.parseFloat();
          signal.frequency(kSignalFrequencyHz);
          Serial.printf("new frequency: %dHz\n", (int)kSignalFrequencyHz);
        }
        break;
      }
    case 'y':
      {  // Bins
        if (Serial.available()) {
          kNumberOfBins = (uint16_t)Serial.parseFloat();
          Serial.printf("new number of bins: %d\n", (int)kNumberOfBins);
        }
        break;
      }
    case 'z':
      {  // pulse duration
        if (Serial.available()) {
          kSignalDurationUs = (uint32_t)Serial.parseInt();
          Serial.printf("new pulse duration: %dus\n", (int)kSignalDurationUs);
        }
        break;
      }
  }
}

switch (currentMode) {
  case MODE_1:
    GeneratePseudoForces();
    break;
  case MODE_2:
    GenerateContinuousVibration();
    break;
  case MODE_3:
    GenerateMotionCoupledPseudoForces();
    break;
  case MODE_4:
    GenerateMotionCoupledVibration();
    break;
  case MODE_5:
    GeneratePseudoForces();
    break;
  case MODE_Debug:
    measuredDistance = results.distance_mm;  // make this fast
    snprintf(report, sizeof(report), "Status = %3u, Distance = %5u mm, Signal = %6u kcps/spad\r\n",
             results.range_status,
             results.distance_mm,
             results.signal_per_spad_kcps);
    Serial.print(report);
    break;
}

measuredDistance = results.distance_mm;  // make this fast
snprintf(report, sizeof(report), "Status = %3u, Distance = %5u mm, Signal = %6u kcps/spad\r\n", 
         results.range_status,
         results.distance_mm,
         results.signal_per_spad_kcps);
Serial.print(report);
}

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
// Change the max and min of sensor to what the sensor actually measure
// Maybe having the positive and negative cycles of the vibration stored?
// Maybe for condition c and d, we divide the total cycles/2 and then play for the positive and for the negative.

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
char amplitude_char;
bool modeRunning = false;

//=========== Laser Sensing ===========
#define DEV_I2C Wire2
VL53L4CD sensor_vl53l4cd_sat(&DEV_I2C, A1);
int16_t x_lib_offset = 0;
#define CalibLength 100
#define XZero 85
float filtered_sensor_value = 0.f;
float last_triggered_sensor_val = 0.f;
unsigned long measuredDistance;
int sensorSamplingFrequency = 100;

//=========== Laser Sensing Constants ===========
static constexpr float kFilterWeight = 0.5;
static constexpr uint32_t kSensorMinValue = 20;
static constexpr uint32_t kSensorMaxValue = 620;
static constexpr uint32_t kSensorJitterThreshold = 50;

//============================= STORING ===========================
const int maxDataPoints = 30000;
struct DataPoint {
  unsigned long timestamp;
  unsigned long distance;
  bool VibrationStatus;
};

DataPoint data[maxDataPoints];
int dataIndex = 0;

// Timer Variables
unsigned long startRecordingMillis;
unsigned long currentRecordingMillis;
const unsigned long conditionPeriod = 15000;  // 15 seconds

// ========== Replay =============
unsigned int currentIndexReplay = 0;
unsigned int dataSize = sensorSamplingFrequency * conditionPeriod * 0.001;

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

uint16_t countVibrationsTriggered = 0;
uint16_t saveCountofVibrationsTriggered = 0;

//=========== signal generator ===========
static uint16_t kNumberOfBins = 60;
static constexpr short kSignalWaveform = static_cast<short>(Waveform::kArbitrary);
static uint32_t kSignalDurationUs = 25 * 1000;  // in microseconds
static uint16_t kSignalDurationMs = 25;         // in milliseconds
static float kSignalFrequencyHz = 40.f;
static float kSignalAsymAmp = 1.f;
static constexpr float kSignalContinuousAmp = 1.0f;
const int16_t dat[256] = { -19754, -19235, -18393, -17264, -15891, -14325, -12622, -10836, -9021, -7223, -5485, -3840, -2314, -924, 317, 1405, 2341, 3129, 3776, 4293, 4692, 4983, 5180, 5295, 5339, 5322, 5255, 5147, 5006, 4838, 4651, 4449, 4236, 4018, 3796, 3574, 3355, 3139, 2929, 2725, 2528, 2339, 2159, 1987, 1825, 1671, 1527, 1391, 1265, 1146, 1036, 935, 841, 754, 675, 602, 537, 477, 424, 376, 334, 296, 264, 236, 213, 193, 178, 166, 157, 152, 149, 149, 152, 157, 164, 174, 185, 198, 212, 228, 246, 264, 283, 304, 325, 347, 369, 392, 415, 439, 463, 487, 511, 536, 560, 584, 608, 632, 655, 678, 701, 723, 745, 766, 787, 807, 826, 845, 863, 881, 897, 913, 928, 943, 956, 969, 980, 991, 1001, 1010, 1018, 1026, 1032, 1037, 1041, 1045, 1047, 1049, 1049, 1049, 1047, 1045, 1041, 1037, 1032, 1026, 1018, 1010, 1001, 991, 980, 969, 956, 943, 928, 913, 897, 881, 863, 845, 826, 807, 787, 766, 745, 723, 701, 678, 655, 632, 608, 584, 560, 536, 511, 487, 463, 439, 415, 392, 369, 347, 325, 304, 283, 264, 246, 228, 212, 198, 185, 174, 164, 157, 152, 149, 149, 152, 157, 166, 178, 193, 213, 236, 264, 296, 334, 376, 424, 477, 537, 602, 675, 754, 841, 935, 1036, 1146, 1265, 1391, 1527, 1671, 1825, 1987, 2159, 2339, 2528, 2725, 2929, 3139, 3355, 3574, 3796, 4018, 4236, 4449, 4651, 4838, 5006, 5147, 5255, 5322, 5339, 5295, 5180, 4983, 4692, 4293, 3776, 3129, 2341, 1405, 317, -924, -2314, -3840, -5485, -7223, -9021, -10836, -12622, -14325, -15891, -17264, -18393, -19235 };
int16_t negDat[256];

#define Amplitude_ARRAYSIZE 3  // Change if we decide on 3 levels of amplitude
float receivedInts[Amplitude_ARRAYSIZE] = { 0.4, 0.7, 1.0 };
// float receivedInts[Amplitude_ARRAYSIZE] = { 0.5, 1.0 };
char report[64];
char full_report[160];  // Define a buffer for the full report

//=========== Continuous Vibration ===========
const long kContinuousVibrationDuration = 2500;
uint16_t kpseudoForceRepetition = 2;
const long kNoVibrationDuration = 500;
uint16_t kContinuousVibrationRepetition = 3;
unsigned long previousMillis = 0;

// Timing and control variables for pseudo forces
unsigned long previousPseudoForcesMillis = 0;
int repetitionCountPseudoForces = 0;
int stepPseudoForces = 0;  // Step variable to track the current step in the pseudo forces sequence

// Timing and control variables for continuous vibration
unsigned long previousContinuousVibrationMillis = 0;
int repetitionCountContinuousVibration = 0;

//=========== serial ===========
static constexpr int kBaudRate = 115200;

void SetupSerial() {
  while (!Serial && millis() < 2000)
    ;
  Serial.begin(kBaudRate);
  delay(500);
}

void InitializeSensor() {
  DEV_I2C.begin();                     // Initialize I2C bus.
  sensor_vl53l4cd_sat.begin();         // Configure VL53L4CD satellite component.
  sensor_vl53l4cd_sat.VL53L4CD_Off();  // Switch off VL53L4CD satellite component.
  sensor_vl53l4cd_sat.InitSensor();    //Initialize VL53L4CD satellite component.
  sensor_vl53l4cd_sat.VL53L4CD_CalibrateOffset(XZero, &x_lib_offset, 100);
  sensor_vl53l4cd_sat.VL53L4CD_GetOffset(&x_lib_offset);
  sensor_vl53l4cd_sat.VL53L4CD_SetRangeTiming(10, 0);  // Program the highest possible TimingBudget, without enabling the low power mode. This should give the best accuracy
  sensor_vl53l4cd_sat.VL53L4CD_StartRanging();         // Start Measurements
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
        signal.arbitraryWaveform(negDat, 170);
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
      if (repetitionCountPseudoForces >= kpseudoForceRepetition) {
        Serial.println("Pseudo Forces: All repetitions complete");
      }
      break;
  }
}

void GenerateContinuousVibration() {
  unsigned long currentMillis = millis();

  if (!is_vibrating) {
    if (repetitionCountContinuousVibration < kContinuousVibrationRepetition) {
      if (currentMillis - previousContinuousVibrationMillis >= kContinuousVibrationDuration) {
        signal.begin(WAVEFORM_SINE);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalContinuousAmp);
        previousContinuousVibrationMillis = currentMillis;
        is_vibrating = true;
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
      is_vibrating = false;
      repetitionCountContinuousVibration++;
      // Serial.println("Continuous Vibration: Stop");
    }
  }
}

void GenerateMotionCoupledPseudoForces() {
  if (mapped_bin_id < last_bin_id) {
    // Uncomment below to stop the current vibration and play the next one.
    // if (is_vibrating) {  // This loop is for the case when we want to stop the ongoing vibration and start the next one.
    //   StopPulse();
    //   delayMicroseconds(10);
    // }

    StartPulsePosPF();
    last_bin_id = mapped_bin_id;
    last_triggered_sensor_val = filtered_sensor_value;
  }

  if (mapped_bin_id > last_bin_id) {
    // if (is_vibrating) {  // This loop is for the case when we want to stop the ongoing vibration and start the next one.
    //   StopPulse();
    //   delayMicroseconds(10);
    // }

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

void ReplayPseudoForces() {
  static unsigned long startMillisReplay = millis();
  static unsigned long ReplaypulseStartMillis = 0;
  static bool isVibrating = false;

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    int commaIndex = input.indexOf(',');
    if (commaIndex > 0) {
      String timeString = input.substring(0, commaIndex);
      String isVibratingString = input.substring(commaIndex + 1);

      unsigned long timeWhenVibrates = timeString.toFloat();
      bool shouldVibrate = isVibratingString.toInt() == 1;

      unsigned long currentMillisReplay = millis();
      unsigned long elapsedMillisReplay = currentMillisReplay - startMillisReplay;

      if (elapsedMillisReplay >= timeWhenVibrates) {
        if (shouldVibrate && !isVibrating) {
          StartPulsePosPF();
          ReplaypulseStartMillis = currentMillisReplay;
          isVibrating = true;
        }
      } else {
        // Calculate delay needed to match the timing
        unsigned long delayTime = timeWhenVibrates - elapsedMillisReplay;
        delay(delayTime);                // Temporarily use delay to align with the incoming timing
        currentMillisReplay = millis();  // Update currentMillis after delay
        if (shouldVibrate && !isVibrating) {
          StartPulsePosPF();
          ReplaypulseStartMillis = currentMillisReplay;
          isVibrating = true;
        }
      }
    }
  }

  if (isVibrating && millis() - ReplaypulseStartMillis >= kSignalDurationMs) {
    StopPulse();
    isVibrating = false;
  }
}

void ReplayPseudoForcesLocal() {
  static unsigned long startMillisReplay = 0;
  static unsigned long ReplaypulseStartMillis = 0;
  static bool isVibrating = false;
  // static unsigned int currentIndexReplay = 0;

  if (currentIndexReplay < dataSize) {
    unsigned long timeWhenVibrates = data[dataSize - 1 - currentIndexReplay].timestamp;
    bool shouldVibrate = data[dataSize - 1 - currentIndexReplay].VibrationStatus;

    unsigned long currentMillisReplay = millis();
    unsigned long elapsedMillisReplay = currentMillisReplay - startMillisReplay;
    if (elapsedMillisReplay >= timeWhenVibrates) {
      if (shouldVibrate && !isVibrating) {
        StartPulsePosPF();
        ReplaypulseStartMillis = currentMillisReplay;
        isVibrating = true;
        is_vibrating = true;
      }
      currentIndexReplay++;
    } else {
      unsigned long delayTime = timeWhenVibrates - elapsedMillisReplay;
      delay(delayTime);                // Temporarily use delay to align with the incoming timing
      currentMillisReplay = millis();  // Update currentMillis after delay
      if (shouldVibrate && !isVibrating) {
        StartPulsePosPF();
        ReplaypulseStartMillis = currentMillisReplay;
        isVibrating = true;
        is_vibrating = true;
      }
    }
  }

  if (isVibrating && millis() - ReplaypulseStartMillis >= kSignalDurationMs) {
    StopPulse();
    isVibrating = false;
    is_vibrating = false;
  }
}

void SummaryStatPseudoForces() {
  unsigned long currentMillis = millis();
  unsigned long SummaryStatPF_duration = saveCountofVibrationsTriggered * kSignalDurationUs;

  switch (stepPseudoForces) {
    case 0:  // Start positive vibration
      if (repetitionCountPseudoForces < kpseudoForceRepetition) {
        signal.begin(kSignalWaveform);
        signal.arbitraryWaveform(dat, 170);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 1;
        is_vibrating = true;
      }
      break;

    case 1:  // End positive vibration
      if (currentMillis - previousPseudoForcesMillis >= SummaryStatPF_duration) {
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
        signal.arbitraryWaveform(negDat, 170);
        signal.frequency(kSignalFrequencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 3;
        // Serial.println("Pseudo Forces: Starting negative vibration");
        is_vibrating = true;
      }
      break;

    case 3:  // End negative vibration
      if (currentMillis - previousPseudoForcesMillis >= SummaryStatPF_duration) {
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
      if (repetitionCountPseudoForces >= kpseudoForceRepetition) {
        Serial.println("Pseudo Forces: All repetitions complete");
      }
      break;
  }
}

void MappingFunction(uint16_t measuredDistance) {
  filtered_sensor_value = (1.f - kFilterWeight) * filtered_sensor_value + (kFilterWeight) * measuredDistance;
  if (filtered_sensor_value >= kSensorMaxValue) {
    filtered_sensor_value = kSensorMaxValue;
  }
  if (filtered_sensor_value <= kSensorMinValue) {
    filtered_sensor_value = kSensorMinValue;
  }

  mapped_bin_id = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, 0, kNumberOfBins);
  // Serial.println(mapped_bin_id);

  auto dist = abs((float)(filtered_sensor_value - last_triggered_sensor_val));
  if (dist < kSensorJitterThreshold) {
    return;
  }
}

void handleSerialInput(char serial_c) {
  if (serial_c == 'a' || serial_c == 'b' || serial_c == 'c' || serial_c == 'd' || serial_c == 'e') {
    if (Serial.available()) {
      amplitude_char = Serial.read();
      switch (amplitude_char) {
        case '0':
          kSignalAsymAmp = receivedInts[0];
          signal.amplitude(kSignalAsymAmp);
          break;
        case '1':
          kSignalAsymAmp = receivedInts[1];
          signal.amplitude(kSignalAsymAmp);
          break;
        case '2':
          kSignalAsymAmp = receivedInts[2];
          signal.amplitude(kSignalAsymAmp);
          break;
        default:
          Serial.println("Invalid Amplitude Command");
          break;
      }
      StopPulse();
    }
  } else if (serial_c == 'x') {
    if (Serial.available()) {
      float kSignalFrequencyHz = Serial.parseFloat();
      signal.frequency(kSignalFrequencyHz);
      Serial.printf("new frequency: %dHz\n", (int)kSignalFrequencyHz);
    }
  } else if (serial_c == 'y') {
    if (Serial.available()) {
      uint16_t kNumberOfBins = (uint16_t)Serial.parseFloat();
      Serial.printf("new number of bins: %d\n", (int)kNumberOfBins);
    }
  } else if (serial_c == 'z') {
    if (Serial.available()) {
      uint32_t kSignalDurationUs = (uint32_t)Serial.parseInt();
      Serial.printf("new pulse duration: %dus\n", (int)kSignalDurationUs);
    }
  }
}

void printDataArray(char mode, char amplitudeLevel) {
  // Serial.println("Data array contents:");
  for (unsigned int i = 0; i < dataSize; i++) {
    Serial.print("Condition: ");
    Serial.print(mode);
    Serial.print(", AmpLevel: ");
    Serial.print(amplitudeLevel);
    Serial.print(", Index: ");
    Serial.print(i);
    Serial.print(", Time: ");
    Serial.print(data[i].timestamp);
    Serial.print(", Distance: ");
    Serial.print(data[i].distance);
    Serial.print(", Vibration: ");
    Serial.println(data[i].VibrationStatus);
  }
}

void setup() {
  SetupSerial();
  InitializeSensor();
  SetupAudio();
  for (int i = 0; i < 256; i++) {
    negDat[i] = -dat[i];
  }
  memset(data, 0, sizeof(data));
}

void loop() {
  uint8_t NewDataReady = 0;
  VL53L4CD_Result_t results;
  uint8_t status;

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
  MappingFunction(measuredDistance);
  // snprintf(report, sizeof(report), "Status = %3u, Distance = %5u mm, Signal = %6u kcps/spad",
  //          results.range_status,
  //          results.distance_mm,
  //          results.signal_per_spad_kcps);
  // snprintf(full_report, sizeof(full_report), "%s, Vibrating = %1u\r\n", report, is_vibrating);
  // Serial.print(full_report);

  if (Serial.available()) {
    auto serial_c = (char)Serial.read();
    if (serial_c == 'a' || serial_c == 'b' || serial_c == 'c' || serial_c == 'd' || serial_c == 'e') {
      startRecordingMillis = millis();
      selectedMode = serial_c;
      modeRunning = true;
    }
    handleSerialInput(serial_c);
  }

  if (modeRunning) {
    if (millis() - startRecordingMillis < conditionPeriod) {
      switch (selectedMode) {
        case 'a':
          GenerateMotionCoupledPseudoForces();
          data[dataIndex] = { millis() - startRecordingMillis, (unsigned long)filtered_sensor_value, is_vibrating };
          dataIndex++;
          countVibrationsTriggered += is_vibrating;
          // countVibrationsTriggered++;
          // Serial.println(countVibrationsTriggered);
          break;
        case 'b':
          ReplayPseudoForcesLocal();
          data[dataIndex] = { millis() - startRecordingMillis, (unsigned long)filtered_sensor_value, is_vibrating };
          dataIndex++;
          break;
        case 'c':
          data[dataIndex] = { millis() - startRecordingMillis, (unsigned long)filtered_sensor_value, is_vibrating };
          dataIndex++;
          // Serial.println(saveCountofVibrationsTriggered);
          // SummaryStatPseudoForces();
          GeneratePseudoForces();
          break;
        case 'd':
          data[dataIndex] = { millis() - startRecordingMillis, (unsigned long)filtered_sensor_value, is_vibrating };
          dataIndex++;
          // SummaryStatPseudoForces();
          GeneratePseudoForces();
          break;

        // Non Experiment Conditions //
        case 'e':
          GeneratePseudoForces();
          break;
        case 'f':
          GenerateContinuousVibration();
          break;
        case 'g':
          GenerateMotionCoupledVibration();
          break;
      }
    } else {
      modeRunning = false;
      StopPulse();
      // Serial.println("Time is up!");
      saveCountofVibrationsTriggered = countVibrationsTriggered;
      countVibrationsTriggered = 0;
      currentIndexReplay = 0;
      dataIndex = 0;
      if (selectedMode == 'a' || selectedMode == 'b' || selectedMode == 'c' || selectedMode == 'd') {
        printDataArray(selectedMode, amplitude_char);
      }
      if (selectedMode == 'c' || selectedMode == 'd') {
        memset(data, 0, sizeof(data));
      }
    }
  }
  last_triggered_sensor_val = filtered_sensor_value;
}

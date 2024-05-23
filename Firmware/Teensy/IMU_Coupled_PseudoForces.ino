#include <Wire.h>
// #include <elapsedMillis.h>
#include <Audio.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <EEPROM.h>

// TO DO MAIN:
// Urgent: Check if you can modulate the pseudo forces
// Looking at different mappings
// To decide whether to stop the current pulse and play the next or just to play the next after finishing the current pulse

// TO DO (OPTIONAL):
// Implementing rubber band (torsional spring kind of effects)
// Max and Min of IMU

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
  MODE_1,     // Continuous Pseudo Forces
  MODE_2,     // Continuous Vibration
  MODE_3,     // Motion-Coupled Pseudo Forces
  MODE_4,     // Motion-Coupled Continous Vibration
  MODE_Debug  // Debug Mode
};

Mode currentMode = MODE_3;  // Initialize current mode to MODE_3

//=========== IMU sensor calibration and retreival ===========
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire2);  // Use Wire1 for I2C communication
#define BNO055_SAMPLERATE_DELAY_MS (100)

void displaySensorDetails(void) {
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print("Sensor:       ");
  Serial.println(sensor.name);
  Serial.print("Driver Ver:   ");
  Serial.println(sensor.version);
  Serial.print("Unique ID:    ");
  Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    ");
  Serial.print(sensor.max_value);
  Serial.println(" xxx");
  Serial.print("Min Value:    ");
  Serial.print(sensor.min_value);
  Serial.println(" xxx");
  Serial.print("Resolution:   ");
  Serial.print(sensor.resolution);
  Serial.println(" xxx");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void displaySensorStatus(void) {
  /* Get the system status values (mostly for debugging purposes) */
  uint8_t system_status, self_test_results, system_error;
  system_status = self_test_results = system_error = 0;
  bno.getSystemStatus(&system_status, &self_test_results, &system_error);

  /* Display the results in the Serial Monitor */
  Serial.println("");
  Serial.print("System Status: 0x");
  Serial.println(system_status, HEX);
  Serial.print("Self Test:     0x");
  Serial.println(self_test_results, HEX);
  Serial.print("System Error:  0x");
  Serial.println(system_error, HEX);
  Serial.println("");
  delay(500);
}

void displayCalStatus(void) {
  /* Get the four calibration values (0..3) */
  /* Any sensor data reporting 0 should be ignored, */
  /* 3 means 'fully calibrated" */
  uint8_t system, gyro, accel, mag;
  system = gyro = accel = mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);

  /* The data should be ignored until the system calibration is > 0 */
  Serial.print("\t");
  if (!system) {
    Serial.print("! ");
  }

  /* Display the individual values */
  Serial.print("Sys:");
  Serial.print(system, DEC);
  Serial.print(" G:");
  Serial.print(gyro, DEC);
  Serial.print(" A:");
  Serial.print(accel, DEC);
  Serial.print(" M:");
  Serial.print(mag, DEC);
}

void displaySensorOffsets(const adafruit_bno055_offsets_t &calibData) {
  Serial.print("Accelerometer: ");
  Serial.print(calibData.accel_offset_x);
  Serial.print(" ");
  Serial.print(calibData.accel_offset_y);
  Serial.print(" ");
  Serial.print(calibData.accel_offset_z);
  Serial.print(" ");

  Serial.print("\nGyro: ");
  Serial.print(calibData.gyro_offset_x);
  Serial.print(" ");
  Serial.print(calibData.gyro_offset_y);
  Serial.print(" ");
  Serial.print(calibData.gyro_offset_z);
  Serial.print(" ");

  Serial.print("\nMag: ");
  Serial.print(calibData.mag_offset_x);
  Serial.print(" ");
  Serial.print(calibData.mag_offset_y);
  Serial.print(" ");
  Serial.print(calibData.mag_offset_z);
  Serial.print(" ");

  Serial.print("\nAccel Radius: ");
  Serial.print(calibData.accel_radius);

  Serial.print("\nMag Radius: ");
  Serial.print(calibData.mag_radius);
}

//=========== IMU sensor constants ===========
uint8_t sys, gyro, accel, mag = 0;
float ACC_THRESHOLD = 0.5f;
float prev_acc_x, prev_acc_y, prev_acc_z = 0.f;
static constexpr float kFilterWeight = 0.7;
static constexpr uint32_t kSensorMinValue = 0;
static constexpr uint32_t kSensorMaxValue = 360;
static constexpr uint32_t kSensorJitterThreshold = 30;

//=========== IMU sensor variables ===========
float orientation = 0.f;
int orientation2degrees = 0;
float filtered_sensor_value = 0.f;
float last_triggered_sensor_val = 0.f;

//=========== Distance Calculation ===========
int start = 0;                 //variable that indicates sampling may begin
double pos_x = 0;              //x- position on 2d plane (m)
double pos_y = 0;              //y-position on 2d plane (m)
double pos_mag = 0;            //total displacement, used in straight line walking test
double leg_len = 1.0;          //leg length in meters
double accelerationMagnitude;  //the magnitude of the acceleration vector excluding gravity
double eX_offset = 0;          //This eX_offset value sets whatever direction your first step is in equal to 0 degrees

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
static constexpr uint16_t kNumberOfBins = 50;
static constexpr short kSignalWaveform = static_cast<short>(Waveform::kArbitrary);
static constexpr short kSignalContinuousWaveform = static_cast<short>(Waveform::kSawtooth);
static constexpr short kSignalContinuousWaveform2 = static_cast<short>(Waveform::kSawtoothReverse);
static constexpr uint32_t kSignalDurationUs = 100 * 1000;  // in microseconds
static constexpr uint32_t kNonMCVDurationUs = 3 * 10 ^ 6;  // in microseconds
static constexpr float kSignalFreqencyHz = 50.f;
static constexpr float kSignalAsymAmp = 1.f;
static constexpr float kSignalContinuousAmp = 0.4f;
const int16_t dat[256] = { -19754, -19235, -18393, -17264, -15891, -14325, -12622, -10836, -9021, -7223, -5485, -3840, -2314, -924, 317, 1405, 2341, 3129, 3776, 4293, 4692, 4983, 5180, 5295, 5339, 5322, 5255, 5147, 5006, 4838, 4651, 4449, 4236, 4018, 3796, 3574, 3355, 3139, 2929, 2725, 2528, 2339, 2159, 1987, 1825, 1671, 1527, 1391, 1265, 1146, 1036, 935, 841, 754, 675, 602, 537, 477, 424, 376, 334, 296, 264, 236, 213, 193, 178, 166, 157, 152, 149, 149, 152, 157, 164, 174, 185, 198, 212, 228, 246, 264, 283, 304, 325, 347, 369, 392, 415, 439, 463, 487, 511, 536, 560, 584, 608, 632, 655, 678, 701, 723, 745, 766, 787, 807, 826, 845, 863, 881, 897, 913, 928, 943, 956, 969, 980, 991, 1001, 1010, 1018, 1026, 1032, 1037, 1041, 1045, 1047, 1049, 1049, 1049, 1047, 1045, 1041, 1037, 1032, 1026, 1018, 1010, 1001, 991, 980, 969, 956, 943, 928, 913, 897, 881, 863, 845, 826, 807, 787, 766, 745, 723, 701, 678, 655, 632, 608, 584, 560, 536, 511, 487, 463, 439, 415, 392, 369, 347, 325, 304, 283, 264, 246, 228, 212, 198, 185, 174, 164, 157, 152, 149, 149, 152, 157, 166, 178, 193, 213, 236, 264, 296, 334, 376, 424, 477, 537, 602, 675, 754, 841, 935, 1036, 1146, 1265, 1391, 1527, 1671, 1825, 1987, 2159, 2339, 2528, 2725, 2929, 3139, 3355, 3574, 3796, 4018, 4236, 4449, 4651, 4838, 5006, 5147, 5255, 5322, 5339, 5295, 5180, 4983, 4692, 4293, 3776, 3129, 2341, 1405, 317, -924, -2314, -3840, -5485, -7223, -9021, -10836, -12622, -14325, -15891, -17264, -18393, -19235 };
int16_t negDat[256];

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
inline void InitializeSensor() __attribute__((always_inline));
inline void CalibrateSensor() __attribute__((always_inline));

void SetupSerial() {
  while (!Serial && millis() < 2000)
    ;
  Serial.begin(kBaudRate);
}

void SetupAudio() {
  AudioMemory(20);
  delay(50);  // time for DAC voltage stable
  signal.begin(WAVEFORM_SINE);
  signal.frequency(kSignalFreqencyHz);
}

void InitializeSensor() {
  if (!bno.begin()) {
    Serial.println("Failed to initialize BNO055 sensor. Check your wiring or I2C address.");
    while (1)
      ;
  }
  // delay(500);
  // bno.begin();
  Serial.println("BNO055 sensor initialized successfully!");

  int eeAddress = 0;
  long bnoID;
  bool foundCalib = false;

  EEPROM.get(eeAddress, bnoID);

  adafruit_bno055_offsets_t calibrationData;
  sensor_t sensor;

  bno.getSensor(&sensor);
  if (bnoID != sensor.sensor_id) {
    Serial.println("\nNo Calibration Data for this sensor exists in EEPROM");
    delay(500);
  } else {
    Serial.println("\nFound Calibration for this sensor in EEPROM.");
    eeAddress += sizeof(long);
    EEPROM.get(eeAddress, calibrationData);

    displaySensorOffsets(calibrationData);

    Serial.println("\n\nRestoring Calibration data to the BNO055...");
    bno.setSensorOffsets(calibrationData);

    Serial.println("\n\nCalibration data loaded into BNO055");
    foundCalib = true;
  }

  delay(500);

  displaySensorDetails();
  displaySensorStatus();

  bno.setExtCrystalUse(true);

  sensors_event_t event;
  bno.getEvent(&event);
  /* always recal the mag as It goes out of calibration very often */
  if (foundCalib) {
    Serial.println("Move sensor slightly to calibrate magnetometers");
    while (!bno.isFullyCalibrated()) {
      bno.getEvent(&event);
      delay(BNO055_SAMPLERATE_DELAY_MS);
    }
  } else {
    Serial.println("Please Calibrate Sensor: ");
    while (!bno.isFullyCalibrated()) {
      bno.getEvent(&event);

      Serial.print("X: ");
      Serial.print(event.orientation.x, 4);
      Serial.print("\tY: ");
      Serial.print(event.orientation.y, 4);
      Serial.print("\tZ: ");
      Serial.print(event.orientation.z, 4);

      /* Optional: Display calibration status */
      displayCalStatus();

      /* New line for the next sample */
      Serial.println("");

      /* Wait the specified delay before requesting new data */
      delay(BNO055_SAMPLERATE_DELAY_MS);
    }
  }

  Serial.println("\nFully calibrated!");
  Serial.println("--------------------------------");
  Serial.println("Calibration Results: ");
  adafruit_bno055_offsets_t newCalib;
  bno.getSensorOffsets(newCalib);
  displaySensorOffsets(newCalib);

  Serial.println("\n\nStoring calibration data to EEPROM...");

  eeAddress = 0;
  bno.getSensor(&sensor);
  bnoID = sensor.sensor_id;

  EEPROM.put(eeAddress, bnoID);

  eeAddress += sizeof(long);
  EEPROM.put(eeAddress, newCalib);
  Serial.println("Data stored to EEPROM.");

  Serial.println("\n--------------------------------\n");
  delay(500);
}

void GetOrientationData() {
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  imu::Vector<3> acc = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  if (abs(acc.x()) < ACC_THRESHOLD && abs(acc.y()) < ACC_THRESHOLD && abs(acc.z()) < ACC_THRESHOLD) {
    // StopPulse();
    return;
  }
  // Serial.printf("%f,%f,%f,%d,%d,%d,%d\n", acc.x(), acc.y(), acc.z(), gyro, accel, mag, sys);

  // Get quaternion data
  sensors_event_t event;
  bno.getEvent(&event);
  imu::Quaternion quat = bno.getQuat();

  // Convert quaternion to Euler angles
  imu::Vector<3> euler = quat.toEuler();

  // Extract Euler angles (in radians) from the vector
  float head = euler.x() * 180.0 / PI;
  float roll = euler.y() * 180.0 / PI;
  float pitch = euler.z() * 180.0 / PI;

  orientation2degrees = sqrt((head * head) + (roll * roll) + (pitch * pitch));
  // Serial.println(orientation2degrees);
  delay(50);
}

void GetDistanceData() {
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  imu::Vector<3> acc = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  accelerationMagnitude = (sqrt(pow(acc.x(), 2) + pow(acc.y(), 2) + pow(acc.z(), 2))) - 9.81;
  pos_x = pos_x + 2 * (2 * leg_len * cos((euler.y() - 10) * PI / 180)) * cos((euler.x() - eX_offset) * PI / 180);
  pos_y = pos_y + 2 * (2 * leg_len * cos((euler.y() - 10) * PI / 180)) * sin((euler.x() - eX_offset) * PI / 180);
  pos_mag = pos_mag + 2 * (2 * leg_len * cos((euler.y() - 10) * PI / 180));
  Serial.println(pos_mag);
}

void StartPulsePosPF() {
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(dat, 170);
  signal.frequency(kSignalFreqencyHz);
  signal.phase(0.0);
  signal.amplitude(kSignalAsymAmp);
  pulse_time_us = 0;
  is_vibrating = true;
  Serial.printf("Start Pos pulse \n\t bins: %d", mapped_bin_id);
  Serial.println(F("=====================================================\n\n"));
}

void StartPulseNegPF() {
  signal.begin(kSignalWaveform);
  signal.arbitraryWaveform(negDat, 170);
  signal.frequency(kSignalFreqencyHz);
  signal.phase(0.0);
  signal.amplitude(kSignalAsymAmp);
  pulse_time_us = 0;
  is_vibrating = true;
  Serial.printf("Start Neg pulse \n\t bins: %d", mapped_bin_id);
  Serial.println(F("=====================================================\n\n"));
}

void StartPulseCV() {
  signal.begin(WAVEFORM_SINE);
  signal.frequency(kSignalFreqencyHz);
  signal.phase(0.0);
  signal.amplitude(kSignalContinuousAmp);
  pulse_time_us = 0;
  is_vibrating = true;
  Serial.printf("Start continuous pulse \n\t bins: %d", mapped_bin_id);
  Serial.println(F("=====================================================\n\n"));
}

void StopPulse() {
  signal.amplitude(0.f);
  is_vibrating = false;
  Serial.println("Stop pulse");
}

void GeneratePseudoForces() {
  unsigned long currentMillis = millis();
  
  switch (stepPseudoForces) {
    case 0: // Start positive vibration
      if (repetitionCountPseudoForces < kpseudoForceRepetition) {
        signal.begin(kSignalWaveform);
        signal.arbitraryWaveform(dat, 170);
        signal.frequency(kSignalFreqencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 1;
        Serial.println("Pseudo Forces: Starting positive vibration");
      }
      break;

    case 1: // End positive vibration
      if (currentMillis - previousPseudoForcesMillis >= kContinuousVibrationDuration) {
        signal.frequency(0);
        signal.amplitude(0);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 2;
      }
      break;

    case 2: // Pause after positive vibration
      if (currentMillis - previousPseudoForcesMillis >= kNoVibrationDuration) {
        signal.begin(kSignalWaveform);
        signal.arbitraryWaveform(negDat, 170);
        signal.frequency(kSignalFreqencyHz);
        signal.amplitude(kSignalAsymAmp);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 3;
        Serial.println("Pseudo Forces: Starting negative vibration");
      }
      break;

    case 3: // End negative vibration
      if (currentMillis - previousPseudoForcesMillis >= kContinuousVibrationDuration) {
        signal.frequency(0);
        signal.amplitude(0);
        previousPseudoForcesMillis = currentMillis;
        stepPseudoForces = 4;
      }
      break;

    case 4: // Pause after negative vibration
      if (currentMillis - previousPseudoForcesMillis >= kNoVibrationDuration) {
        repetitionCountPseudoForces = 0;
        stepPseudoForces = 0;
      }
      break;

    default: // All repetitions complete
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
        signal.frequency(kSignalFreqencyHz);
        signal.amplitude(kSignalContinuousAmp);
        previousContinuousVibrationMillis = currentMillis;
        is_vibrating_continuous = true;
        Serial.println("Continuous Vibration: Start");
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
      Serial.println("Continuous Vibration: Stop");
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
  filtered_sensor_value = (1.f - kFilterWeight) * filtered_sensor_value + (kFilterWeight)*orientation2degrees;
  mapped_bin_id = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, 0, kNumberOfBins);
  // Serial.println(mapped_bin_id);
}

void setup() {
  SetupSerial();
  SetupAudio();
  InitializeSensor();
  for (int i = 0; i < 256; i++) {
    negDat[i] = -dat[i];
  }
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);

  if (Serial.available()) {
    auto serial_c = (char)Serial.read();
    switch (serial_c) {
      case '1':
        currentMode = MODE_1;
        Serial.println("Mode 1: Continuous Pseudo Forces");
        break;
      case '2':
        currentMode = MODE_2;
        Serial.println("Mode 2: Continuous Vibration");
        break;
      case '3':
        currentMode = MODE_3;
        Serial.println("Mode 3: Motion-Coupled Pseudo Forces");
        break;
      case '4':
        currentMode = MODE_4;
        Serial.println("Mode 4: Motion Coupled Continuous Vibration");
        break;
      case 'r':
        currentMode = MODE_Debug;
        Serial.println("Debug Mode");
        break;
    }
  }

  GetOrientationData();
  MappingFunction();

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
    case MODE_Debug:
      /* Get a new sensor event */
      sensors_event_t event;
      bno.getEvent(&event);
      displayCalStatus();
      Serial.println("");
      delay(BNO055_SAMPLERATE_DELAY_MS);
      break;
  }
}

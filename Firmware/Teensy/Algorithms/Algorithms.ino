#include <Wire.h>
// #include <elapsedMillis.h>
#include <Audio.h>
#include <vl53l4cd_class.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define VERSION "v1.1.0"

// TODO:
// 1. Check if the filtered_sensor_value is more smoother
// 2. Bell Curve algorithm for number of grains
// 3. Writing the function for algorithm 2, time-coupled algorithm
// 4. Writing the functions for the other 2 applications

// Our algorithms are motion-coupled

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

//============ Velocity to Amplitude Mapping ============
unsigned long lastTimeVel = 0;
unsigned long currentTimeVel;
float lastDistance = 0;
float currentDistance = 0;
float currentVelocity = 0;
float filtered_velocity_value = 0.f;
static constexpr float kVelocityFilterWeight = 0.1;
static constexpr float kMaxVelocity = 40;  // cm/s
static constexpr float kMinVelocity = 5;   // cm/s (Basically, if the velocity is lower than this, consider it to have stopped)

//=========== Laser Sensing ===========
#define DEV_I2C1 Wire
#define DEV_I2C2 Wire1
VL53L4CD sensor_vl53l4cd_1(&DEV_I2C1, A0);
VL53L4CD sensor_vl53l4cd_2(&DEV_I2C2, A1);

int16_t x_lib_offset = 0;
#define CalibLength 100
#define XZero 85

float filtered_sensor_value_1 = 0.f;
float filtered_sensor_value_2 = 0.f;
unsigned long measuredDistance_1;
unsigned long measuredDistance_2;
float filtered_sensor_value = 0.f;
float filtered_sensor_value_new = 0.f;
float last_triggered_sensor_val = 0.f;
unsigned long measuredDistance;
int sensorSamplingFrequency = 100;

//=========== Laser Sensing Constants ===========
float kFilterWeight = 0.5;
static constexpr float kFilterWeightNear = 8;
static constexpr float kFilterWeightFar = 2;
static constexpr uint32_t kSensorMinValue = 20;
static constexpr uint32_t kSensorMaxValue = 600;
static constexpr uint32_t kSensorJitterThreshold = 5;

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
uint16_t saveCountofVibrationsTriggeredSummary = 0;

//=========== signal generator ===========
static uint16_t kNumberOfBins = 60;
static constexpr short kSignalWaveform = static_cast<short>(Waveform::kArbitrary);
static uint32_t kSignalDurationUs = 25 * 1000;  // in microseconds
static float kSignalFrequencyHz = 40.f;
static float kSignalAsymAmp = 1.f;
static constexpr float kSignalContinuousAmp = 1.0f;
const int16_t dat[256] = { -19754, -19235, -18393, -17264, -15891, -14325, -12622, -10836, -9021, -7223, -5485, -3840, -2314, -924, 317, 1405, 2341, 3129, 3776, 4293, 4692, 4983, 5180, 5295, 5339, 5322, 5255, 5147, 5006, 4838, 4651, 4449, 4236, 4018, 3796, 3574, 3355, 3139, 2929, 2725, 2528, 2339, 2159, 1987, 1825, 1671, 1527, 1391, 1265, 1146, 1036, 935, 841, 754, 675, 602, 537, 477, 424, 376, 334, 296, 264, 236, 213, 193, 178, 166, 157, 152, 149, 149, 152, 157, 164, 174, 185, 198, 212, 228, 246, 264, 283, 304, 325, 347, 369, 392, 415, 439, 463, 487, 511, 536, 560, 584, 608, 632, 655, 678, 701, 723, 745, 766, 787, 807, 826, 845, 863, 881, 897, 913, 928, 943, 956, 969, 980, 991, 1001, 1010, 1018, 1026, 1032, 1037, 1041, 1045, 1047, 1049, 1049, 1049, 1047, 1045, 1041, 1037, 1032, 1026, 1018, 1010, 1001, 991, 980, 969, 956, 943, 928, 913, 897, 881, 863, 845, 826, 807, 787, 766, 745, 723, 701, 678, 655, 632, 608, 584, 560, 536, 511, 487, 463, 439, 415, 392, 369, 347, 325, 304, 283, 264, 246, 228, 212, 198, 185, 174, 164, 157, 152, 149, 149, 152, 157, 166, 178, 193, 213, 236, 264, 296, 334, 376, 424, 477, 537, 602, 675, 754, 841, 935, 1036, 1146, 1265, 1391, 1527, 1671, 1825, 1987, 2159, 2339, 2528, 2725, 2929, 3139, 3355, 3574, 3796, 4018, 4236, 4449, 4651, 4838, 5006, 5147, 5255, 5322, 5339, 5295, 5180, 4983, 4692, 4293, 3776, 3129, 2341, 1405, 317, -924, -2314, -3840, -5485, -7223, -9021, -10836, -12622, -14325, -15891, -17264, -18393, -19235 };
int16_t negDat[256];
int16_t negDatTrial[256];

#define Amplitude_ARRAYSIZE 3  // Change if we decide on 3 levels of amplitude
float receivedInts[Amplitude_ARRAYSIZE] = { 0.4, 0.7, 1.0 };
char report[64];
char full_report[160];  // Define a buffer for the full report

//=========== Continuous Vibration ===========
const long kContinuousVibrationDuration = 2500;
uint16_t kpseudoForceRepetition = 1;
uint16_t kpseudoForceRepetitionTrial = 3;
const long kNoVibrationDuration = 100;

// Timing and control variables for pseudo forces
unsigned long previousPseudoForcesMillis = 0;
int repetitionCountPseudoForces = 0;
int stepPseudoForces = 0;  // Step variable to track the current step in the pseudo forces sequence

// Area
int kNumSamples = 256;
float timePeriod;
float deltaTimePeriod;

//=========== serial ===========
static constexpr int kBaudRate = 115200;
struct ParsedData {
  int objectID;
  int state;
  float value;
};

ParsedData parseSerialData(const String &data) {
  ParsedData parsedData = { 0, 0, 0.0f };

  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);

  if (firstComma != -1 && secondComma != -1) {
    parsedData.objectID = data.substring(0, firstComma).toInt();
    parsedData.state = data.substring(firstComma + 1, secondComma).toInt();
    parsedData.value = data.substring(secondComma + 1).toFloat();
  }

  return parsedData;
}

void SetupSerial() {
  while (!Serial && millis() < 2000)
    ;
  Serial.begin(kBaudRate);
  delay(500);
}

void InitializeSensor(VL53L4CD &sensor) {
  sensor.begin();                         // Configure VL53L4CD satellite component.
  sensor.VL53L4CD_Off();                  // Switch off VL53L4CD satellite component.
  sensor.InitSensor();                    // Initialize VL53L4CD satellite component.
  sensor.VL53L4CD_SetRangeTiming(10, 0);  // Program the highest possible TimingBudget, without enabling the low power mode. This should give the best accuracy
  sensor.VL53L4CD_StartRanging();         // Start Measurements
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

void StopPulse() {
  signal.amplitude(0.f);
  is_vibrating = false;
  // Serial.println("Stop pulse");
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

void CalculateArea() {
  timePeriod = 1 / kSignalFrequencyHz;
  deltaTimePeriod = timePeriod / kNumSamples;

  // Calculate the area under the curve
  double area = 0.0;
  for (int i = 0; i < kNumSamples - 1; i++) {
    area += (dat[i] + dat[i + 1]) / 2.0;  // * deltaTimePeriod;
    Serial.print("Area under the curve: ");
    Serial.println(area);
  }

  // Serial.print("Area under the curve: ");
  // Serial.println(area - dat[0]);
  delay(5);
  Serial.print("Final Area:");
  Serial.println(area - dat[0]);
}

void MappingFunction(uint16_t measuredDistance, float &filtered_sensor_value) {
  kFilterWeight = map(measuredDistance, kSensorMinValue, kSensorMaxValue, kFilterWeightNear, kFilterWeightFar) * 0.1;
  filtered_sensor_value = (1.f - kFilterWeight) * filtered_sensor_value + (kFilterWeight)*measuredDistance;
  if (filtered_sensor_value >= kSensorMaxValue) {
    filtered_sensor_value = kSensorMaxValue;
  }
  if (filtered_sensor_value <= kSensorMinValue) {
    filtered_sensor_value = kSensorMinValue;
  }
  mapped_bin_id = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, 0, kNumberOfBins);
  // Serial.println(mapped_bin_id);
}

void VelocityMappingFunction() {
  filtered_velocity_value = (1.f - kVelocityFilterWeight) * filtered_velocity_value + (kVelocityFilterWeight)*currentVelocity;
  if (filtered_velocity_value < kMinVelocity) {
    filtered_velocity_value = 0;
  }
  if (filtered_velocity_value >= kMaxVelocity) {
    filtered_velocity_value = kMaxVelocity;
  }
  mapped_bin_id = map(filtered_velocity_value, 0, kMaxVelocity, 0, kNumberOfBins);
}

void handleSerialInput(char serial_c) {
  if (serial_c == 'a' || serial_c == 'b' || serial_c == 'c' || serial_c == 'd' || serial_c == 'e') {
    // if (Serial.available()) {
    //   amplitude_char = Serial.read();
    //   switch (amplitude_char) {
    //     case '0':
    //       kSignalAsymAmp = receivedInts[0];
    //       signal.amplitude(kSignalAsymAmp);
    //       break;
    //     case '1':
    //       kSignalAsymAmp = receivedInts[1];
    //       signal.amplitude(kSignalAsymAmp);
    //       break;
    //     case '2':
    //       kSignalAsymAmp = receivedInts[2];
    //       signal.amplitude(kSignalAsymAmp);
    //       break;
    //     default:
    //       Serial.println("Invalid Amplitude Command");
    //       break;
    //   }
    //   StopPulse();
    // }
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

void runAlgorithm1() {
  // Velocity mapped to amplitude, number of grains,
  int kNumberOfBinsMin = 10;
  int kNumberOfBinsMax = 100;
  float kSignalAsymAmpMin = 0.1;
  float kSignalAsymAmpMax = 1.0;
  if (filtered_sensor_value < kSensorMinValue) {
    kSignalAsymAmp = kSignalAsymAmpMin;
    kNumberOfBins = 0;
  } else if (filtered_sensor_value > kSensorMaxValue) {
    kSignalAsymAmp = kSignalAsymAmpMax;
    kNumberOfBins = 0;
  } else {
    kNumberOfBins = map(filtered_velocity_value, kSensorMinValue, kSensorMaxValue, kNumberOfBinsMin, kNumberOfBinsMax);  // Mapping based on velocity
    kSignalAsymAmp = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, kSignalAsymAmpMin, kSignalAsymAmpMax);
  }
  Serial.printf("Bins are: %d Amplitude is: %.2f\n", kNumberOfBins, kSignalAsymAmp);
  // mapped_bin_id = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, 0, kNumberOfBins);
  // Something similar needs to be done for velocity.
  VelocityMappingFunction();
  GenerateMotionCoupledPseudoForces();
}

void runAlgorithm2() {
  // Frequency based algorithm (Theoretical Algorithm based on Motion-coupled vibration)
  // Step 1: When we are moving slow, the frequency of pulses should be more spread out
  // Step 2: When we are moving fast, the frequency of pulses should be denser.
  // So basically, we need to map the number of grains to the movement velocity

  float kNumberOfBinsMin = 10;
  float kNumberOfBinZero = 0;
  float kNumberOfBinsMax = 100;
  if (filtered_sensor_value < kSensorMinValue) {
    kNumberOfBins = kNumberOfBinZero;
  } else if (filtered_sensor_value > kSensorMaxValue) {
    kNumberOfBins = kNumberOfBinZero;
  } else {
    kNumberOfBins = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, kNumberOfBinsMin, kNumberOfBinsMax);  // Mapping based on distance
    // kNumberofBins = map(filtered_sensor_value,  kSensorMinValue, kSensorMaxValue, kNumberOfBinsMax, kNumberOfBinsMin); // Inverse Mapping based on distance
    // kNumberofBins = map(filtered_velocity_value,  kSensorMinValue, kSensorMaxValue, kNumberOfBinsMin, kNumberOfBinsMax); // Mapping based on velocity
    // kNumberofBins = map(filtered_velocity_value,  kSensorMinValue, kSensorMaxValue, kNumberOfBinsMax, kNumberOfBinsMin); // Inverse Mapping based on velocity
  }
  Serial.print("Adjusted Bins: ");
  Serial.println(kNumberOfBins);
  mapped_bin_id = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, 0, kNumberOfBins);
  GenerateMotionCoupledPseudoForces();
}

void runAlgorithm3() {
  // Area based algorithm (Theoretical Algorithm based on Pseudo Forces) TIME COUPLED ASYMMETRIC VIBRATION
  // Step 1: Calculate the area under one cycle which is being provided.
  // Step 2: Calculate the area of over the movement speed (or basically how many cycles are provided)
  // Step 3: Have the same area irrespective of the speed of movement. The way to do this is to play around with more cycles as the speed increases or trigger more individual pulses
}

void runAlgorithm4() {
  // Amplitude based algorithm
  // This is more like an envelope based algorithm, where the amplitude is increased if the movement speed increases.
  // It can also be mapped to the distance, where the amplitude increases as the distance increases.
  float kSignalAsymAmpMin = 0.1;
  float kSignalAsymAmpMax = 1.0;
  if (filtered_sensor_value < kSensorMinValue) {
    kSignalAsymAmp = kSignalAsymAmpMin;
  } else if (filtered_sensor_value > kSensorMaxValue) {
    kSignalAsymAmp = kSignalAsymAmpMax;
  } else {
    kSignalAsymAmp = map(filtered_sensor_value, kSensorMinValue, kSensorMaxValue, kSignalAsymAmpMin, kSignalAsymAmpMax);
  }

  signal.amplitude(kSignalAsymAmp);

  Serial.print("Adjusted amplitude based on distance: ");
  Serial.println(kSignalAsymAmp);

  GenerateMotionCoupledPseudoForces();
}

void runAlgorithmBellCurve() {
  float bellCurveAmplitude = 1.0f;
  float mu = 0.5f;     // Center of the bell curve (in terms of velocity)
  float sigma = 0.2f;  // Width of the bell curve

  float normalizedVelocity = constrain(filtered_velocity_value, 0, 1);  // Normalizing the velocity
  kSignalAsymAmp = bellCurveAmplitude * exp(-pow((normalizedVelocity - mu), 2) / (2 * pow(sigma, 2)));
  signal.amplitude(kSignalAsymAmp);

  Serial.print("Adjusted amplitude based on distance: ");
  Serial.println(kSignalAsymAmp);

  GenerateMotionCoupledPseudoForces();
}

void BowArrow(const ParsedData &parsedData) {
  // The more away from the sensor we move, the larger the amplitude
  // So basically, we need to map the position to the amplitude.
  // This mapping algorithm should use the a-star (Generate Increasing Pseudo Forces) algorithms we might get
  switch (parsedData.state) {
    case 0:  // No Stretch
      kSignalAsymAmp = 0;
      StopPulse();
      Serial.println("BowArrow - No Stretch");
      break;
    case 1:
      kSignalAsymAmp = map(parsedData.value, 0, 100, 0.2f, 1.0f);
      signal.amplitude(kSignalAsymAmp);
      Serial.print("BowArrow - Stretched with percent stretch: ");
      Serial.println(parsedData.value);
      break;
  }
  GenerateMotionCoupledPseudoForces();
}

void WalkTheDog() {
  // Walk-the-dog: <Object_Identifier, State, percent_pull>; State: 0 (no_pull); State: 1 (sniffing); State: 2 (continuous pull).
  // The sniffing might need further definitions probably.
}

void HapticMagnets() {
  // Magnets: <Object_Identifier, State, distance>; State: 0 (repel); State: 1 (attract)
  //   float distanceThresholdMagnets;
  //   float distanceBetweenMagnets; // Will be recieved over serial
  //   if (distanceBetweenMagnets > distanceThresholdMagnets){
  //     return;
  //   } else{
  //     if (magnetMode == modeAttract){
  //       // Generate Continuous Positive / Negative Pseudo Forces on both hands with Amplitude increasing based on the distance between the magnets
  //     } else{
  //       // Generate Continous One Positive - One Negative Pseudo force on both hands with amplitude increasing based on the distance between the magnets.
  //     }
  //   }
}

void setup() {
  SetupSerial();
  DEV_I2C1.begin();  // Initialize I2C bus.
  DEV_I2C2.begin();  // Initialize I2C bus.
  InitializeSensor(sensor_vl53l4cd_1);
  InitializeSensor(sensor_vl53l4cd_2);
  SetupAudio();
  for (int i = 0; i < 256; i++) {
    negDat[i] = dat[i];
    negDatTrial[i] = -dat[i];
  }
  // CalculateArea();
}

void loop() {
  uint8_t NewDataReady_1 = 0;
  uint8_t NewDataReady_2 = 0;
  VL53L4CD_Result_t results_1;
  VL53L4CD_Result_t results_2;
  uint8_t status_1;
  uint8_t status_2;

  do {
    status_1 = sensor_vl53l4cd_1.VL53L4CD_CheckForDataReady(&NewDataReady_1);
  } while (!NewDataReady_1);

  if ((!status_1) && (NewDataReady_1 != 0)) {
    // (Mandatory) Clear HW interrupt to restart measurements
    sensor_vl53l4cd_1.VL53L4CD_ClearInterrupt();

    // Read measured distance. RangeStatus = 0 means valid data
    sensor_vl53l4cd_1.VL53L4CD_GetResult(&results_1);
  }

  do {
    status_2 = sensor_vl53l4cd_2.VL53L4CD_CheckForDataReady(&NewDataReady_2);
  } while (!NewDataReady_2);

  if ((!status_2) && (NewDataReady_2 != 0)) {
    // (Mandatory) Clear HW interrupt to restart measurements
    sensor_vl53l4cd_2.VL53L4CD_ClearInterrupt();

    // Read measured distance. RangeStatus = 0 means valid data
    sensor_vl53l4cd_2.VL53L4CD_GetResult(&results_2);
  }

  // CalculateArea();

  currentTimeVel = millis();
  measuredDistance_1 = results_1.distance_mm;  // make this fast
  measuredDistance_2 = results_2.distance_mm;  // make this fast
  MappingFunction(measuredDistance_1, filtered_sensor_value_1);
  MappingFunction(measuredDistance_2, filtered_sensor_value_2);

  filtered_sensor_value = (measuredDistance_1 + measuredDistance_2) / 2;
  filtered_sensor_value_new = (filtered_sensor_value_1 + filtered_sensor_value_2) / 2;
  Serial.print(filtered_sensor_value);
  Serial.print(",");
  Serial.println(filtered_sensor_value_new);
  delay(10);

  currentDistance = (measuredDistance_1 + measuredDistance_2) / 2;
  currentVelocity = (currentDistance - lastDistance);  // / (lastTimeVel - currentTimeVel);
  filtered_velocity_value = (1.f - kVelocityFilterWeight) * filtered_velocity_value + (kVelocityFilterWeight)*currentVelocity;

  // Serial.println(currentVelocity);
  // delay(50);

  if (Serial.available()) {
    auto serial_c = (char)Serial.read();
    if (serial_c == 'a' || serial_c == 'b' || serial_c == 'c' || serial_c == 'd' || serial_c == 'e') {
      selectedMode = serial_c;
    }
    handleSerialInput(serial_c);
  }

  switch (selectedMode) {
    case 'a':
      // if (abs(currentVelocity) < kSensorJitterThreshold) {return;}
      GenerateMotionCoupledPseudoForces();
      Serial.println("MCAV_Basic");
      break;
    case 'b':
      runAlgorithm2();
      Serial.println("Algorithm 2");
      break;
    case 'c':
      HapticMagnets();
      Serial.println("Magnets");
      break;
    case 'd':
      Serial.println("Do Nothing");
      return;
      break;
    case 'e':
      GeneratePseudoForces();
      break;
    case 'f':
      runAlgorithm3();
      break;
    case 'g':
      runAlgorithm4();
      break;
  }
  lastTimeVel = currentTimeVel;
  lastDistance = currentDistance;
}

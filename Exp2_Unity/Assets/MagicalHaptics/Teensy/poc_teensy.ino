String inputString = "";         // A string to hold incoming data
bool stringComplete = false;     // Whether the string is complete

const int ledPin = 13; // Onboard LED is connected to GPIO13

void setup() {
  pinMode(ledPin, OUTPUT); // Set the LED pin as an output
  Serial.begin(115200);    // Initialize Serial for debugging
  inputString.reserve(200); // Reserve some memory for the input string
  
  while (!Serial) {
    ; // Wait for the serial port to connect. Needed for native USB
  }
  Serial.println("Teensy Ready");
}

void loop() {
  if (Serial.available() > 0) {
    checkSerialData();
  }

  // If a full line is received, process it
  if (stringComplete) {
    // Print the full received line
    Serial.print("Received: ");
    Serial.println(inputString);

    // Clear the string:
    inputString = "";
    stringComplete = false;
  }

  performNormalOperations();
}

void checkSerialData() {
  char inChar = (char)Serial.read();  // Read the incoming byte

  // Add it to the inputString:
  inputString += inChar;

  // If the incoming character is a newline, set a flag so the main loop can process it:
  if (inChar == '\n') {
    stringComplete = true;
  }
}

void performNormalOperations() {
  // Perform other non-blocking tasks
  // Example: Blink an LED
  static unsigned long lastMillis = 0;
  const unsigned long interval = 500; // Interval for LED blink (milliseconds)

  if (millis() - lastMillis >= interval) {
    lastMillis = millis();
    digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle the LED
  }
}

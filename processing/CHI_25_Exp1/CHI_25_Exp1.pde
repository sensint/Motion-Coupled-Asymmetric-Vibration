import controlP5.*;
import processing.serial.*;
import g4p_controls.*;
import processing.data.Table;
import processing.data.TableRow;

Table dataTable; // Table to store data entries

GTextField participantIdTextfield;
GTextField sequenceTextfield;

Serial serial; // Serial object for communication with Arduino
String[] serialPorts; // Array to store available serial ports
ControlP5 cp5;
Button validateButton;
Button nextButton;
Button replayButton;
Button nextConditionButton;

int countdownTime = 15; // Countdown time in seconds
int startTime;
boolean timerFinished = false;
int currentScreen = 1; // 1: Screen 1, 2: Screen 2, 3: Screen 3
String participantId;
String sequence;
String[] conditions_amplitudes;
char[]conditions;
char[] amplitudes;
int trials =0;
boolean incremented=false;
int currentConditionIndex = 0;
int totalconditions =0;
String text_Screen3 ="";
String dataString="";

void setup() {
  size(800, 600); // Window size
  
  //INITIALIZE SERIAL COMUNICATION
    // Get a list of available serial ports
  serialPorts = Serial.list();
  
  if (serialPorts.length == 0) {
    println("No serial ports found.");
  }
  else {
    println("Available serial ports:");
    for (int i = 0; i < serialPorts.length; i++) {
      println(i + ": " + serialPorts[i]);
    }
    
    // Select the first serial port in the list (modify as needed)
    String portName = serialPorts[0];
    
    // Initialize serial communication
    serial = new Serial(this, portName, 9600); // Adjust baud rate as per your setup
  }
  
  //INITIALIZE DATA STORAGE
   // Create a new table
  dataTable = new Table();
  
  // Add columns to the table
  dataTable.addColumn("Condition");
  dataTable.addColumn("AmpLevel");
  dataTable.addColumn("Index");
  dataTable.addColumn("Time");
  dataTable.addColumn("Distance");
  dataTable.addColumn("Vibration");
  
  //INITIALIZE VISUAL COMPONENTS
  cp5 = new ControlP5(this);

  // Screen 1: Input fields and Validate button

  // Initialize G4P textfields
  participantIdTextfield = new GTextField(this, 300, 210, 300, 40);
  participantIdTextfield.setFont(new java.awt.Font("Arial", java.awt.Font.PLAIN, 24));

  sequenceTextfield = new GTextField(this, 300, 280, 300, 40);
  sequenceTextfield.setFont(new java.awt.Font("Arial", java.awt.Font.PLAIN, 24));

  validateButton = cp5.addButton("validate")
    .setPosition(350, 380)
    .setSize(120, 50)
    .setFont(createFont("arial", 24))
    .setLabel("Validate")
    .onClick(new CallbackListener() {
      public void controlEvent(CallbackEvent event) {
        validateInputs();
      }
    });

  // Screen 2: Next and Replay buttons
  nextButton = cp5.addButton("next")
    .setPosition(250, 400)
    .setSize(100, 50)
    .setFont(createFont("arial", 24))
    .setLabel("Next")
    .setVisible(false)
    .onClick(new CallbackListener() {
      public void controlEvent(CallbackEvent event) {
        switchToNextScreen();
      }
    });

  replayButton = cp5.addButton("replay")
    .setPosition(450, 400)
    .setSize(100, 50)
    .setFont(createFont("arial", 24))
    .setLabel("Replay")
    .setVisible(false)
    .onClick(new CallbackListener() {
      public void controlEvent(CallbackEvent event) {
        restartSequence();
      }
    });

  // Screen 3: Next Condition button
  nextConditionButton = cp5.addButton("nextCondition")
    .setPosition(300, 400)
    .setSize(200, 50)
    .setFont(createFont("arial", 18))
    .setLabel("Next Condition")
    .setVisible(false)
    .onClick(new CallbackListener() {
      public void controlEvent(CallbackEvent event) {
        switchToNextCondition();
      }
    });
}

void draw() {
  background(240);
  
  if (serial.available() > 0) {
    // Read the incoming data from the serial port
    dataString = serial.readStringUntil('\n');
    
    if(dataString!= null)
    {
      // Print the received data to the console
      //println("Received data: " + dataString);
      dataString = trim(dataString);
      // Remove commas from the data string
      dataString = dataString.replace(",", "");
    
      // Split the string by spaces and colons
      String[] parts = split(dataString," ");
    
      if(parts.length == 12)
      {
        // Extract values from the split parts
        String condition = parts[1];
        int ampLevel = Integer.parseInt(parts[3]);
        int index = Integer.parseInt(parts[5]);
        int time = Integer.parseInt(parts[7]);
        int distance = Integer.parseInt(parts[9]);
        int vibration = Integer.parseInt(parts[11]);
      
        // Add a new row to the table
        TableRow newRow = dataTable.addRow();
        newRow.setString("Condition", condition);
        newRow.setInt("AmpLevel", ampLevel);
        newRow.setInt("Index", index);
        newRow.setInt("Time", time);
        newRow.setInt("Distance", distance);
        newRow.setInt("Vibration", vibration);
        }
        
         // Print the stored data for verification
      //for (TableRow row : dataTable.rows()) {
      //  println("Condition: " + row.getString("Condition") + ", AmpLevel: " + row.getInt("AmpLevel") + ", Index: " + row.getInt("Index") + ", Time: " + row.getInt("Time") + ", Distance: " + row.getInt("Distance") + ", Vibration: " + row.getInt("Vibration"));
      //}
    }
  }

  if (currentScreen == 1) {
    // Screen 1: Input fields and Validate button
    fill(0);
    textSize(24);
    textAlign(LEFT, CENTER);
    text("Participant ID:", 100, 230);
    text("Sequence:", 150, 300);
  } else if (currentScreen == 2) {
    // Screen 2: Countdown timer
    fill(0);
    textSize(96);
    textAlign(CENTER, CENTER);

    if (!timerFinished) {
      int elapsedTime = (millis() - startTime) / 1000;
      int remainingTime = countdownTime - elapsedTime;
      if (remainingTime > 0) {
        text(remainingTime, width / 2, height / 2);
      } else {
        timerFinished = true;
        textSize(50);
        text("Time's up!", width / 2, height / 2);
        nextButton.show();
        replayButton.show();
        SaveOrganiseCSV( dataTable, participantId, trials, conditions[currentConditionIndex], amplitudes[currentConditionIndex]);
      }
    } else {
      textSize(50);
      text("Time's up!", width / 2, height / 2);
    }
  } else if (currentScreen == 3) {
    // Screen 3: "Take a break" and "Next Condition" button
    fill(0);
    textSize(48);
    textAlign(CENTER, CENTER);
    
    // adapt texte on screen 3 and update trial number
    //println((((trials+1)*(conditions.length/conditions_amplitudes.length))-1)+"  "+currentConditionIndex+"  "+trials);
    if((((trials+1)*(conditions.length/conditions_amplitudes.length))-1)== currentConditionIndex)
    {
      text_Screen3= "Take a break";
      if (incremented == false)
      {
        trials++;
        incremented =true;
      }
    }
    else
    {
      text_Screen3= "Go to Next Condition";
    }
    text(text_Screen3, width / 2, height / 2 - 50);
  }
}

void validateInputs() {
  participantId = participantIdTextfield.getText();
  sequence = sequenceTextfield.getText();

  println("Participant ID: " + participantId);
  println("Sequence: " + sequence);

  // Split sequence into conditions
  conditions_amplitudes = sequence.split("_");
  currentConditionIndex = 0;
  
  //intialisation of conditions and amplitudes tab
  for (int i=0; i<conditions_amplitudes.length; i++)
  {
    for (int j=0; j<conditions_amplitudes[i].length();j++)
    {
      totalconditions++;
    }
  }
  conditions=new char[totalconditions/2];
  amplitudes=new char[totalconditions/2];
  
  // fill conditions and amplitudes tab
  int comp =0;
   for (int i=0; i<conditions_amplitudes.length; i++)
  {
    for (int j=0; j<conditions_amplitudes[i].length();j=j+2)
    {
    conditions[comp]= conditions_amplitudes[i].charAt(j);
    amplitudes[comp]=conditions_amplitudes[i].charAt(j+1);
    comp++;
    }
  }
  //print verification of parsing
  //for(int i=0;i<conditions.length;i++){
  //  println("conditions "+conditions[i]+" ; amplitudes "+amplitudes[i]);
  //}

  // Validation logic here
  if (participantId.isEmpty() || sequence.isEmpty()) {
    println("Both fields are required.");
  } else if (conditions_amplitudes.length == 0) {
    println("Invalid sequence format.");
  } else {
    println("Validation successful.");
    
     if (serial != null) {
      // Send "a1" over serial
      String sent = conditions[currentConditionIndex]+""+amplitudes[currentConditionIndex];
      serial.write(sent);
      println("Sent: "+sent);
    }
    currentScreen = 2; // Move to screen 2
    startTime = millis(); // Start the countdown timer
    validateButton.hide();
    participantIdTextfield.setVisible(false);
    sequenceTextfield.setVisible(false);
    nextButton.hide();
    replayButton.hide();
    nextConditionButton.hide();
  }
}

void SaveOrganiseCSV(Table table, String participantID, int trialNumber, char condition, char amplitude) {
  // Create participant directory
  File participantDir = new File(dataPath(participantID));
  if (!participantDir.exists()) {
    participantDir.mkdirs();
  }
  
  // Create trial directory inside participant directory
  File trialDir = new File(dataPath(participantID+ "/Trial" + trialNumber));
  if (!trialDir.exists()) {
    trialDir.mkdirs();
  }
  
  // Save the table to a CSV file inside the trial directory
  String filePath = dataPath(participantID+ "/Trial" + trialNumber+"/"+participantID+"_Trial" + trialNumber+"_"+condition+amplitude+".csv");
  saveTable(table, filePath);
}

void switchToNextScreen() {
  currentScreen = 3; // Move to screen 3
  nextButton.hide();
  replayButton.hide();
  nextConditionButton.show();
}

void switchToNextCondition() {
  if (currentConditionIndex < conditions.length-1 ) 
  {
    currentConditionIndex++;
    incremented = false;
     //send on serial
    if (serial != null) 
    {
        // Send "a1" over serial
        String sent = conditions[currentConditionIndex]+""+amplitudes[currentConditionIndex];
        serial.write(sent);
        println("Sent: "+sent);
    }
    currentScreen = 2; // Move to screen 2
    startTime = millis(); // Start the countdown timer
    validateButton.hide();
    participantIdTextfield.setVisible(false);
    sequenceTextfield.setVisible(false);
    nextButton.hide();
    replayButton.hide();
    nextConditionButton.hide();
    timerFinished = false;
  } 
  else {
      switchToScreen1(); // If all conditions are played, switch back to screen 1
  }
}

void switchToScreen1() {
  currentScreen = 1; // Move to screen 1
  participantIdTextfield.setText("");
  participantIdTextfield.setVisible(true);
  sequenceTextfield.setText("");
  sequenceTextfield.setVisible(true);
  validateButton.show();
  nextButton.hide();
  replayButton.hide();
  nextConditionButton.hide();
  timerFinished = false;
  trials=0;
  currentConditionIndex=0;
  incremented=false;
  totalconditions =0;
  text_Screen3 ="";
  dataString="";

}

void restartSequence() {
  currentScreen = 2; // Move to screen 2
   //send on serial
    if (serial != null) {
      // Send "a1" over serial
      String sent = conditions[currentConditionIndex]+""+amplitudes[currentConditionIndex];
      serial.write(sent);
      println("restarted: "+sent);
    }
  startTime = millis(); // Restart the countdown timer
  timerFinished = false;
  nextButton.hide();
  replayButton.hide();
}

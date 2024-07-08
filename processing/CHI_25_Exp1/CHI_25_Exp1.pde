import processing.serial.*;  // Import the serial library
import java.io.*;  // Import for handling files

// GLOBAL VARIABLES
boolean firstScreen = true; // show first screen for participant identification
boolean secondScreen = false; // show second screen with experiment
boolean thirdScreen = false; // show third screen break time
int iteration = 0; // sequence length, number of experiments (2) for one participant
int iterationTotal =0; //sequence length fix
char amplitude = '1';
int trial = 0;
int staticCountdown = 3;  // Countdown start value (in seconds)
String[] conditions;
String[] amplitudes;


//SCREEN 1 VARABLES  
String participantID = "";  // Variable to store the participant ID
String sequence = "";  // Variable to store the sequence
String[] sequenceSmall; // array, store the sequence for each trial
boolean typingID = true;  // Flag to indicate if typing is enabled for participant ID
boolean typingSeq = false;  // Flag to indicate if typing is enabled for sequence

//SCREEN 2 VARABLES
Serial myPort;  // The serial port object
int countdown = staticCountdown;  // Countdown start value (in seconds)
int lastMillis;  // To track time for the countdown
int startTime;  // To store the time when the countdown starts
Table table;  // Table object to store data
String distance =""; //get distance value
boolean wait =false;//prevent skip the countdown
int counter = 0;// force saving only one time
int i =0; // sequence indice (which condition is running
String displayText = "Go to next Condition";


void setup() {
  size(700, 600);
  textSize(32);
  surface.setLocation(600, 200);
  fill(0);
  
  
  // SERIAL PORT DEFINITION
  
   // List all the available serial ports
  printArray(Serial.list());
  
  // Change the number to match your port index (e.g., Serial.list()[0], Serial.list()[1], etc.)
  String portName = Serial.list()[0];  
  if (portName == null) {
    println("Error: No serial ports available.");
    exit();  // Exit the program if no ports are available
  }

  try {
    myPort = new Serial(this, portName, 115200);  // Initialize the serial port
  } catch (Exception e) {
    println("Error: Could not initialize serial port " + portName);
    e.printStackTrace();
    exit();  // Exit the program if the port can't be initialized
  }
  
  
  //TIMER INITIALISATION
  
  lastMillis = millis();  // Initialize lastMillis with the current time
  startTime = millis();  // Store the start time of the countdown


  // SAVING VARIABLE INITIALISATION
  
  table = new Table();
  table.addColumn("Time");
  table.addColumn("Sensor Reading");
  table.addColumn("Vibration");
}

void draw() {
  
  //============================================ setup the experiment (Id + sequence)====================================================
  
  if(firstScreen){
    background(255);  // White background
    textSize(32);
    surface.setLocation(600, 200);
    
    // Participant ID Field
    fill(0);
    text("Participant ID:", width / 2 - 90, height / 2 - 120);
    if (typingID) {
      fill(200);  // Light gray for selected field
    } else {
      fill(255);  // White for non-selected field
    }
    rect(width / 2 - 190, height / 2 - 100, 380, 40);  // Text input box for participant ID
    
    // Sequence Field
    fill(0);
    text("Sequence:", width / 2 - 80, height / 2);
    if (typingSeq ) {
      fill(200);  // Light gray for selected field
    } else {
      fill(255);  // White for non-selected field
    }
    rect(width / 2 - 190, height / 2 + 20, 380, 40);  // Text input box for sequence
    
    // Display the typed text
    fill(0);
    text(participantID, width / 2 - 180, height / 2 - 70);  // Display the typed participant ID
    text(sequence, width / 2 - 180, height / 2 + 50);  // Display the typed sequence
    
    // Validate Button
    fill(0, 200, 200);
    rect(width / 2 - 50, height / 2 + 80, 100, 40);  // Validate button
    fill(0);
    textSize(20);
    text("Validate", width / 2 -40, height / 2 + 108);
    textSize(32);
  }
  
  //====================================================== experiment screen (count down , info and saved file) ===============================================================
  else if (secondScreen)
  {
    background(255);  // White background
    textSize(100);
    textAlign(CENTER, CENTER);
    surface.setLocation(600, 200);
    fill(0);
    if(counter==0){
    textSize(20);
    text(nameSave(participantID, sequenceSmall[trial].charAt(i), amplitude,trial),20+textWidth(nameSave(participantID, sequenceSmall[trial].charAt(i), amplitude,trial))/2,40 );
    }
    
    
      // Read data from the serial port
    if (myPort.available() > 0) {
      String inString = myPort.readStringUntil('\n');  // Read a string from the serial port until a newline character
      if (inString != null) {
        inString = trim(inString);  // Remove any whitespace characters (including the newline)
        String [] result= split(inString," ");
        //println("inString= "+inString);
        if (int(result[9])< 100)
        {
          distance=result[10];
          //println("Received from Arduino: " + result[10]);  // Print the received data
        }
        else
        {
          distance=result[9];
         // println("Received from Arduino: " + result[9]);  // Print the received data
        }
        
        
        // Write the data to the table
        float elapsedTime = (millis() - startTime) / 1000.0;  // Calculate the elapsed time in seconds with decimals
        if( elapsedTime<=30)
        {
          TableRow newRow = table.addRow();
          newRow.setFloat("Time", elapsedTime);
          newRow.setString("Sensor Reading", distance);
          newRow.setString("Vibration", "false");
        }
      }
    }
  
    // Display the countdown
    fill(0);

    if(countdown >=0){
      textSize(100);
      text(countdown, width / 2, height / 2);
    }
    else{
      textSize(100);
      text(0, width / 2, height / 2);
    }
    
  
    // Check if one second has passed
    if (millis() - lastMillis >= 1000) {
      countdown--;  // Decrease the countdown
      lastMillis = millis();  // Reset lastMillis to the current time
    }
  
    // When countdown reaches zero, stop updating
    if (countdown <= 0 ) {
      wait = true;
      fill(255, 0, 0);
      textSize(50);
      text("Time's up!", width / 2, height / 2 + 100);
      
      fill(0, 200, 200);
      rect(width / 2 - 50, height / 2 + 200, 100, 40);  // Ready button
      fill(0);
      textSize(20);
      text("Next", width / 2 - textWidth("go to 3")/2 +30, height / 2 + 220);
      textSize(32);
    }
    if (countdown <=0 && counter ==0){
        //create folder idparticipant+sequence 
        
        // Save the table to a CSV file
        //saveTable(table, nameSave(participantID, sequence.charAt(i), amplitude,trial));
        //println("Recording complete. "+nameSave(participantID, sequence.charAt(i), amplitude,trial));
        SaveOrganiseCSV(table,participantID,trial,sequenceSmall[trial].charAt(i),amplitude);
        i++;
        counter ++;
        
    }
  }   
    // =================================================================== take a break screen =================================================================================
    else if (thirdScreen) {
      // Draw the "Take a Break" screen
      background(255);
      fill(0);
      textSize(40);
      text(displayText, width / 2 , height / 2 - 60);
      
      // Ready Button
      fill(0, 200, 200);
      rect(width / 2 - 50, height / 2 - 20, 100, 40);  // Ready button
      fill(0);
      textSize(20);
      text("Ready!", width / 2 - 10, height / 2 );
      textSize(32);
    }
}

String nameSave(String ID, char condition, char amplitude, int trial)
{
    String result = ID+"_"+condition+str(amplitude)+"_Trial "+str(trial)+".csv";
    return result;
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

void splitConditionsAndAmplitudes(String input) {
  String[] segments = split(input, '_');
  conditions = new String[segments.length];
  amplitudes = new String[segments.length];
  
  for (int i = 0; i < segments.length; i++) {
    String segment = segments[i];
    StringBuilder cond = new StringBuilder();
    StringBuilder amp = new StringBuilder();
    
    for (int j = 0; j < segment.length(); j++) {
      char c = segment.charAt(j);
      if (Character.isLetter(c)) {
        cond.append(c);
      } else if (Character.isDigit(c)) {
        amp.append(c);
      }
    }
    conditions[i] = cond.toString();
    amplitudes[i] = amp.toString();
  }
}


void keyPressed() {

    if (typingID) {
      if (keyCode == BACKSPACE && participantID.length() > 0) {
        participantID = participantID.substring(0, participantID.length() - 1);
      }  else if(keyCode == ENTER){ // change field active when enter pressed
        typingID = false;
        typingSeq =true;
      }else if (keyCode != BACKSPACE && keyCode != DELETE  && keyCode != RETURN) {
        participantID += key;
      }
    } else if (typingSeq) {
      if (keyCode == BACKSPACE && sequence.length() > 0) {
        sequence = sequence.substring(0, sequence.length() - 1);
      } 
      else if(keyCode == ENTER){ // change field active when enter pressed (validate ?)
        typingID = false;
        typingSeq = false;
        splitConditionsAndAmplitudes(sequence);
        sequenceSmall= conditions;
        for(int j=0;j<sequenceSmall.length;j++)
        {
          iteration += sequenceSmall[j].length();
        }
        iterationTotal = iteration;
        println("interaction TOT : "+ iterationTotal);
        
        
        //write on serial port
        amplitude =amplitudes[trial].charAt(i);
        myPort.write(sequenceSmall[trial].charAt(i)+str(amplitude));
        println(sequenceSmall[trial].charAt(i)+str(amplitude));
        secondScreen = true;
        firstScreen = false;
      }
      else if (keyCode != BACKSPACE && keyCode != DELETE && keyCode != RETURN) {
        sequence += key;
      }
    }
}

void mousePressed() {
  if(firstScreen){
      // function to pass first to second screen and determine active field
      if (mouseX > width / 2 - 190 && mouseX < width / 2 + 190 && mouseY > height / 2 - 100 && mouseY < height / 2 - 60) {
        typingID = true;
        typingSeq = false;
      } else if (mouseX > width / 2 - 190 && mouseX < width / 2 + 190 && mouseY > height / 2 + 20 && mouseY < height / 2 + 60) {
        typingID = false;
        typingSeq = true;
      } 
    }
    if (mouseX > width / 2 - 50 && mouseX < width / 2 + 50 && mouseY > height / 2 + 80 && mouseY < height / 2 + 120 && sequence!="") {
      splitConditionsAndAmplitudes(sequence);  
      sequenceSmall= conditions;
        for(int j=0;j<sequenceSmall.length;j++)
        {
          iteration += sequenceSmall[j].length();
        }
        iterationTotal=iteration;
        println("interaction TOT : "+ iterationTotal);
        
        
        //write on serial port
        amplitude =amplitudes[trial].charAt(i);
        myPort.write(sequenceSmall[trial].charAt(i)+str(amplitude));
        println(sequenceSmall[trial].charAt(i)+str(amplitude));
        secondScreen = true;
        firstScreen = false;
    }
  
  else if(secondScreen){ // fuction to pass second to third screen
  //width / 2 - 50, height / 2 + 200, 100, 40
    if (mouseX > width / 2 - 50 && mouseX < width / 2 + 50 && mouseY > height / 2 + 200 && mouseY < height / 2 + 240 && wait==true) {
        thirdScreen = true;
        secondScreen = false;
        wait=false;
        
    }
  }
  else if (thirdScreen) { 
    // Check if the "Ready" button is clicked
    if (mouseX > width / 2 - 50 && mouseX < width / 2 + 50 && mouseY > height / 2 - 20 && mouseY < height / 2 + 20) {
      // experiment with the same participant (go to screen 2)
      iteration --;
      //println(str((iterationTotal-iteration)/sequenceSmall.length)+"  "+str(trial+1));
      if((iterationTotal-iteration)/sequenceSmall[trial].length() >= trial+1)                                                          
      {
        trial ++;
        i=0;
        displayText="Take a break !";
      }
      else{
        displayText = "Go to next Condition";
      }
      if(iteration >0){
        //write on serial port
        amplitude =amplitudes[trial].charAt(i);
        myPort.write(sequenceSmall[trial].charAt(i)+str(amplitude));
        println(sequenceSmall[trial].charAt(i)+str(amplitude));
        secondScreen = true;
        thirdScreen = false;
        counter =0;
        countdown = staticCountdown;
        
        
        //Clean second screen and initialise varables
        background(255);  // White background
        textSize(100);
        textAlign(CENTER, CENTER);
        surface.setLocation(600, 200);
        fill(0);
        
        //TIMER INITIALISATION
        lastMillis = millis();  // Initialize lastMillis with the current time
        startTime = millis();  // Store the start time of the countdown
      
        // SAVING VARIABLE INITIALISATION
        table = new Table();
        table.addColumn("Time");
        table.addColumn("Sensor Reading");
        table.addColumn("Vibration");
      }
      else {
        // here restart for new participant (go to screen 1)
        firstScreen = true;
        thirdScreen = false;
        participantID = "";  
        sequence = "";  
        typingID = true;  
        typingSeq = false;
        i =0;
        
      }
    }
  }
}

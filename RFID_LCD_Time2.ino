/*
  RFID TAG READER AND INTERFACE
  Authors: Sarah Le Cam, Hao Zheng, Noshin Nisa
  Code is heavily altered from original version to allow for
  button and LCD interfaces and registering of object state. The
  code implements millis() timing for smooth messaging and 
  interactions. The bag is able to operate intirely independently 
  of a computer. The association between RFID tags and objects 
  are currently hardcoded.  
  
  Original Code:
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  Constantly reads and outputs any tags heard

  If using the Simultaneous RFID Tag Reader (SRTR) shield, make sure the serial slide
  switch is in the 'SW-UART' position
*/

#include <SoftwareSerial.h> //Used for transmitting to the device

SoftwareSerial softSerial(2, 3); //RX, TX

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module
#include <LiquidCrystal.h>

RFID nano; //Create instance

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
//const int rs = 12, en = 11, d4 = 10, d5 = 13, d6 = 4, d7 = 5;
const int rs = 9, en = 8, d4 = 7, d5 = 6, d6 = 5, d7 = 4;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// digital pin 2 has a pushbutton attached to it. Give it a name:
int computer_button = 12;
int bottle_button = A1;
int keys_button = A3;
int book_button = A0;
int check_button = A2;

// Hardcoded value for backpack RFID
String computer = "E2 00 00 17 22 11 02 38 18 10 5E 22 ";
String bottle = "E2 00 00 17 22 11 02 37 18 10 5E 29 ";
String keys = "E2 00 00 17 22 11 02 31 18 10 5E 1A ";
String notebook = "E2 00 00 17 22 11 02 25 18 10 5E 0B ";

const long disconnectTimer = 1500;
unsigned long timed;

String selection = "none";
const int selectionTimer = 500;
unsigned long lastSelection = 0;

// Computer tracking state
int computerState = 0;
unsigned long compCountDown = 0;

// bottle tracking state
int bottleState = 0;
unsigned long bottleCountDown = 0;

// keys tracking state
int keysState = 0;
unsigned long keysCountDown = 0;

// notebook tracking state
int bookState = 0;
unsigned long bookCountDown = 0;

void setup()
{
  Serial.begin(115200);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
 
  // Print a message to the LCD.
//  lcd.setCursor(0,0);
//  lcd.print("hello, world!");
  
  while (!Serial); //Wait for the serial port to come online

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  nano.setRegion(REGION_NORTHAMERICA); //Set to North America

  nano.setReadPower(1700); //5.00 dBm. Higher values may caues USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

// make string lower case for comparision
  computer.toLowerCase();
  bottle.toLowerCase();
  keys.toLowerCase();
  notebook.toLowerCase();

  // setting input modes for the button PINs
//  pinMode(computer_button, INPUT_PULLUP);
  pinMode(computer_button, INPUT);
  pinMode(bottle_button, INPUT_PULLUP);
  pinMode(keys_button, INPUT_PULLUP );
  pinMode(book_button, INPUT_PULLUP);
  pinMode(check_button, INPUT_PULLUP);  // set pull-up on analog pin 0 

  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
  
//  
//  Serial.println(F("Press a key to begin scanning for tags."));
//  while (!Serial.available()); //Wait for user to send a character
//  Serial.read(); //Throw away the user's character

  nano.startReading(); //Begin scanning for tags
}

String getEPCcode() {
  //If we have a full record we can pull out the fun bits
//      int rssi = nano.getTagRSSI(); //Get the RSSI for this tag read

//      long freq = nano.getTagFreq(); //Get the frequency this tag was detected at

//      long timeStamp = nano.getTagTimestamp(); //Get the time this was read, (ms) since last keep-alive message

      byte tagEPCBytes = nano.getTagEPCBytes(); //Get the number of bytes of EPC from response

//      
//      Serial.print(F(" rssi["));
//      Serial.print(rssi);
//      Serial.print(F("]"));
//
//      Serial.print(F(" freq["));
//      Serial.print(freq);
//      Serial.print(F("]"));
//
//      Serial.print(F(" time["));
//      Serial.print(timeStamp);
//      Serial.print(F("]"));

      //Print EPC bytes, this is a subsection of bytes from the response/msg array
//      Serial.print(F(" epc["));
//      collecting EPC in string form for later comparison
      String epcCode = "";
      for (byte x = 0 ; x < tagEPCBytes ; x++)
      {
        if (nano.msg[31 + x] < 0x10) {
//          Serial.print(F("0")); //Pretty print
          epcCode += "0";
        }
//        Serial.print(nano.msg[31 + x], HEX);
        epcCode += String(nano.msg[31 + x], HEX);
//        Serial.print(F(" "));
        epcCode += " ";
      }
//      Serial.print(F("]"));

//      Serial.println();
    return epcCode;
  
}

void checkComp(String epcCode) {
  if (epcCode == computer && computerState != 0) {
    if (computerState != 2) {
      if (selection == "computer") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Comp in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      Serial.println("Computer detected!");
      computerState = 2;
    }
    compCountDown = timed;
  }
}

void checkCompInRange() {
  
  if (computerState == 2) {
    if (timed - compCountDown >= disconnectTimer) {
      if (selection == "computer") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Comp not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      Serial.println("Your Computer is no longer detected");
      computerState = 1;
    } 
  }
}

void checkCompButton() {
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (digitalRead(computer_button) == HIGH && timed - lastSelection >= selectionTimer) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (selection == "computer") {
      if (computerState == 0) {
        computerState = 1;
        lcd.print("Comp not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      } else {
        computerState = 0;
        lcd.print("Comp not tracked.");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      }
    } else {
      if (computerState == 0) {
        lcd.print("Comp not tracked");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      } else {
        if (computerState == 2) {
          lcd.print("Comp in bag");
        } else {
          lcd.print("Comp not in bag");
        }
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      selection = "computer";
    }
    lastSelection = timed; 
  }
}

void checkBottle(String epcCode) {
  if (epcCode == bottle && bottleState != 0) {
    if (bottleState != 2) {
      if (selection == "bottle") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Bottle in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      Serial.println("Bottle detected!");
      bottleState = 2;
    }
    bottleCountDown = timed;
  }
}

void checkBottleInRange() {
  if (bottleState == 2) {
    if (timed - bottleCountDown >= disconnectTimer) {
      if (selection == "bottle") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Bottle not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      Serial.println("Your Bottle is no longer detected");
      bottleState = 1;
    } 
  }
}

void checkBottleButton() {
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (digitalRead(bottle_button) == HIGH && timed - lastSelection >= selectionTimer) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (selection == "bottle") {
      if (bottleState == 0) {
        bottleState = 1;
        lcd.print("Bottle not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      } else {
        bottleState = 0;
        lcd.print("Bottle not tracked.");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      }
    } else {
      if (bottleState == 0) {
        lcd.print("Bottle not tracked");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      } else {
        if (bottleState == 2) {
          lcd.print("Bottle in bag");
        } else {
          lcd.print("Bottle not in bag");
        }
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      selection = "bottle";
    }
    lastSelection = timed; 
  }
}

void checkKeys(String epcCode) {
  if (epcCode == keys && keysState != 0) {
    if (keysState != 2) {
      if (selection == "keys") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Keys in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      keysState = 2;
      Serial.println("Keys detected!");
    }
    keysCountDown = timed;
  }
}

void checkKeysInRange() {
  if (keysState == 2) {
    if (timed - keysCountDown >= disconnectTimer) {
      if (selection == "keys") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Keys not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      Serial.println("Your Keys are no longer detected");
      keysState = 1;
    }
  }
}

void checkKeysButton() {
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (digitalRead(keys_button) == HIGH && timed - lastSelection >= selectionTimer) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (selection == "keys") {
      if (keysState == 0) {
        keysState = 1;
        lcd.print("Keys not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      } else {
        keysState = 0;
        lcd.print("Keys not tracked.");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      }
    } else {
      if (keysState == 0) {
        lcd.print("Keys not tracked");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      } else {
        if (keysState == 2) {
          lcd.print("Keys in bag");
        } else {
          lcd.print("Keys not in bag");
        }
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      selection = "keys";
    }
    lastSelection = timed; 
  }
}

void checkBook(String epcCode) {
  if (epcCode == notebook && bookState != 0) {
    if (bookState != 2) {
//      Serial.println("Hi");
      if (selection == "book") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Notebook in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      bookState = 2;
      Serial.println("Book detected!");
    }
    bookCountDown = timed;
  }
}

void checkBookInRange() {
  if (bookState == 2) {
    if (timed - bookCountDown >= disconnectTimer) {
      if (selection == "book") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Notebook not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      bookState = 1;
      Serial.println("Your Book is no longer detected");
    }
  }
}

void checkBookButton() {
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (digitalRead(book_button) == HIGH && timed - lastSelection >= selectionTimer) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (selection == "book") {
      if (bookState == 0) {
        bookState = 1;
        lcd.print("Notebook not in bag");
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      } else {
        bookState = 0;
        lcd.print("Notebook not tracked.");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      }
    } else {
      if (bookState == 0) {
        lcd.print("Notebook not tracked");
        lcd.setCursor(0,1);
        lcd.print("Press to track");
      } else {
        if (bookState == 2) {
          lcd.print("Notebook in bag");
        } else {
          lcd.print("Notebook not in bag");
        }
        lcd.setCursor(0,1);
        lcd.print("Press to end tracking");
      }
      selection = "book";
    }
    lastSelection = timed; 
  }
}

int checkStatus() {
//  String missing[];
  int displayVar = 0;
  while (displayVar < 5) {
    if (computerState != 1 && bottleState != 1 && bookState != 1 && keysState != 1) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("You have all");
      lcd.setCursor(0,1);
      lcd.print("your stuff!");

      delay(2000);
      
      lcd.clear();
      selection = "none";
      
      displayVar = 5;
      
    } else {
      if (displayVar == 0 && computerState == 1) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Missing Items: ");
        lcd.setCursor(0,1);
        lcd.print("Computer");
        delay(2000);
      } else if (displayVar == 1 && bottleState == 1) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Missing Items: ");
        lcd.setCursor(0,1);
        lcd.print("Bottle");
        delay(2000);
      } else if (displayVar == 2 && keysState == 1) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Missing Items: ");
        lcd.setCursor(0,1);
        lcd.print("Keys");
        delay(2000);
      } else if (displayVar == 3 && bookState == 1) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Missing Items: ");
        lcd.setCursor(0,1);
        lcd.print("Notebook");
        delay(2000);
      } else if (displayVar == 4) {
        lcd.clear();
        selection = "none";
      }
      displayVar ++;
    }
  }
  
}

void checkCheckButton() {
  if (digitalRead(check_button) == HIGH && timed - lastSelection >= selectionTimer) {
    selection = "display";
    checkStatus();
  }
}

void checkTag(String epcCode) {
  checkComp(epcCode);
  checkBottle(epcCode);
  checkKeys(epcCode);
  checkBook(epcCode);
}

void checkInRange() {
  checkCompInRange();
  checkBottleInRange();
  checkKeysInRange();
  checkBookInRange();
//  Serial.println(bookCountDown);
}

void checkButtons() {
  checkCompButton();
  checkBottleButton();
  checkKeysButton();
  checkBookButton();
  checkCheckButton();
}

void loop()
{
  timed = millis();
  
  if (nano.check() == true) //Check to see if any new data has come in from module
  {
    
    byte responseType = nano.parseResponse(); //Break response into tag ID, RSSI, frequency, and timestamp

    
    if (responseType == RESPONSE_IS_KEEPALIVE)
    {
//      Serial.println(F("Scanning"));
    }
    else if (responseType == RESPONSE_IS_TAGFOUND)
    {
      String epcCode = getEPCcode();

      checkTag(epcCode);
      
    }
    else if (responseType == ERROR_CORRUPT_RESPONSE)
    {
      Serial.println("Bad CRC");
    }
    else
    {
      //Unknown response
      Serial.print("Unknown error");
    }
//    Serial.println(compCountDown);
    
  }

  checkInRange();
  checkButtons();
 
}

//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(softSerial); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while(!softSerial); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while(softSerial.available()) softSerial.read();
  
  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
    nano.stopReading();

    Serial.println(F("Module continuously reading. Asking it to stop..."));

    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    softSerial.begin(115200); //Start software serial at 115200

    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}


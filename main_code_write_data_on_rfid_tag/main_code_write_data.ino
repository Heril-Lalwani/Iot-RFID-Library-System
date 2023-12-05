#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SS_PIN D4
#define RST_PIN D3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::StatusCode status; //variable to get card status

byte buffer[18];  //data transfer buffer (16+2 bytes data+CRC)
byte size = sizeof(buffer);

uint8_t pageAddr = 0x06;  //In this example we will write/read 16 bytes (page 6,7,8 and 9).
                            //Ultraligth mem = 16 pages. 4 bytes per page. 
                            //Pages 0 to 4 are for special functions.

const int MUX_SELECT_PIN_1 = D0; // GPIO pin for mux select line 1
const int MUX_SELECT_PIN_2 = D8; // GPIO pin for mux select line 2           

void setup() {
  Serial.begin(9600); // Initialize serial communications with the PC

  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  pinMode(MUX_SELECT_PIN_1, OUTPUT);
  pinMode(MUX_SELECT_PIN_2, OUTPUT);


  // Clear the buffer.
  display.clearDisplay();
  display.setTextColor(WHITE); // Set text color to white
  display.setTextSize(1); // Set text size
  display.setCursor(0, 0); // Set cursor position

  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card 
  Serial.println(F("Sketch has been started!"));
  getInputFromUser();
}

void getInputFromUser(){
  Serial.println(F("Please enter a 7, 10, or 13 character input: "));
  displayString("Please enter a 7, 10, or 13 character input: ");
  char userInput[14]; // Buffer to store user input (maximum 16 characters + null terminator)
  int charCount = 0;

  while (charCount < 13) { // Read up to 16 characters
    while (!Serial.available()) {
      // Wait for user input
    }
    char c = Serial.read();

    if (c == '\r' || c == '\n') { // Check for Enter key
      if (charCount == 13 || charCount == 10 || charCount == 7) { // Valid input lengths
        break;
      } else {
        Serial.println(F("Invalid input length. Please enter 7, 10 or 13"));
        displayString("Invalid input length. Please enter 7, 10 or 13");
        continue;
      }
    }

    userInput[charCount] = c;
    charCount++;
  }

  userInput[charCount] = '\0'; // Null-terminate the string

  // Copy the user input to the buffer
  memcpy(buffer, userInput, 16);


}

void loop() {
  digitalWrite(MUX_SELECT_PIN_1, LOW);
  digitalWrite(MUX_SELECT_PIN_2, LOW);
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;


  // Write data ***********************************************

  for (int i=0; i < 4; i++) {
    //data is writen in blocks of 4 bytes (4 bytes per page)
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Ultralight_Write(pageAddr+i, &buffer[i*4], 4);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
  }
  Serial.println(F("MIFARE_Ultralight_Write() OK "));
  Serial.println();


  


  // Read data ***************************************************
  
  Serial.println(F("Reading data ... "));
  //data in 4 block is readed at once.
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }


  Serial.print(F("Readed data: "));
  //Dump a byte array to Serial
  for (byte i = 0; i < 16; i++) {
    Serial.write(buffer[i]);
  }
  Serial.println();

  // Display the data on the OLED screen
  char readDataStr[50]; // Create a buffer to hold the read data as a string
  snprintf(readDataStr, sizeof(readDataStr), "Readed data: %s", buffer);
  displayString(readDataStr);

  digitalWrite(MUX_SELECT_PIN_1, HIGH);
  digitalWrite(MUX_SELECT_PIN_2, LOW);
  delay(1000);
  digitalWrite(MUX_SELECT_PIN_1, LOW);
  digitalWrite(MUX_SELECT_PIN_2, LOW);

  mfrc522.PICC_HaltA();

}


// Function to display a string on the OLED screen with word wrapping
void displayString(const char* str) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  int16_t x = 0;
  int16_t y = 0;
  int16_t maxLineWidth = 21; // Character width limit per line on your OLED display
  const int16_t textHeight = 8; // Assuming a standard text height

  char workingStr[strlen(str) + 1]; // Create a working copy of the input string
  strcpy(workingStr, str);

  char wordBuffer[50]; // Buffer to store a word (adjust the size as needed)
  strcpy(wordBuffer,"");
  char displayBuffer[50]; // Buffer to store concatenated words
  int16_t wordLengths[50]; // Array to store the length of each word

  // Pre-process the input string to contain words with spaces
  char* token = strtok(workingStr, " ");
  while (token != NULL) {
    strcpy(displayBuffer,wordBuffer);
    // Copy the word to the word buffer
    strcat(wordBuffer, token);
    strcat(wordBuffer, " ");

    if (strlen(wordBuffer) > maxLineWidth) {
      // Move to the next line and display the concatenated words
      x = 0;
      y += textHeight; // Move to the next line

      // Display the concatenated words
      display.setCursor(x, y);
      display.print(displayBuffer); 

      // Reset word count for the new line
      strcpy(wordBuffer,token);
      strcat(wordBuffer, " ");
      strcpy(displayBuffer," ");
    }
    
    token = strtok(NULL, " ");
  }

  // Display any remaining concatenated words
  if (strlen(wordBuffer) > 0) {
    x = 0;
    y += textHeight; // Move to the next line
    Serial.print(displayBuffer);
    display.setCursor(x, y);
    display.print(wordBuffer);
  }

  display.display();
}



// // Function to display a string on the OLED screen
// void displayString(const char* str) {
//   display.clearDisplay();
//   display.setCursor(0, 0);
//   display.println(str);
//   display.display();
// }


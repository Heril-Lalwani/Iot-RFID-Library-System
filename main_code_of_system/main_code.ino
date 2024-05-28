#ifdef __AVR__
#include <SD.h>
#endif
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <vector>
#include <string>


#define FIREBASE_HOST "iot3-635b7-default-rtdb.europe-west1.firebasedatabase.app"  
#define FIREBASE_AUTH "PWjYjk149hCMbwTcE0YaCTAQ6yD42yvC56MAkC3s" 
// #define FIREBASE_HOST "iot2-3dc5d-default-rtdb.asia-southeast1.firebasedatabase.app"  
// #define FIREBASE_AUTH "KzKetNEhHtJeBQ0bwGfZLz5ADd2j57lD42H1EJAd" 
// #define FIREBASE_HOST "iot-rfid-c6a2a-default-rtdb.firebaseio.com"
// #define FIREBASE_AUTH "8gCuhlpMX5DkeXFmdJm74kPF1ZpdBaZmZ9shyAnP"

// #define WIFI_SSID "Redmi Note 9 Pro"
// #define WIFI_PASSWORD "prey@123"
// #define WIFI_SSID "RJ"
// #define WIFI_PASSWORD "passw0rd"
// #define WIFI_SSID "POCO F5"
// #define WIFI_PASSWORD "1234567890"
#define WIFI_SSID "Prashant"
#define WIFI_PASSWORD "PrashantLalwani"




#define SS_PIN D4
#define RST_PIN D3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::StatusCode status;

byte buffer[18] ={0};
byte size = sizeof(buffer);
int sizeOfBooks = 0;

uint8_t pageAddr = 0x06;  // In this example, we will write/read 16 bytes (page 6,7,8 and 9).


const char* OPEN_LIBRARY_API_HOST = "openlibrary.org";
const int OPEN_LIBRARY_API_PORT = 80;

// const unsigned char Active_buzzer = 14;
// const unsigned char built_in_led = 16;

DynamicJsonBuffer jsonBuffer;


std::vector<String> getBlock(String blockName);
std::vector<String> getAllBlocks();
std::vector<String> getBooksearch(String bookName);
std::vector<String> keysVector;
std::vector<String> blocksVector;
std::vector<String> searchVector;

bool emptyBlock = false;
bool detectedBlockTag = false; 
String currentBlockTag = ""; // Variable to store the current block tag
bool inBlockScan = false;   // Flag to indicate if we're in block scan mode
String bookName = "";
String author = "";
String publisher = "";
String database = "database/";


const int MUX_SELECT_PIN_1 = D0; // GPIO pin for mux select line 1
const int MUX_SELECT_PIN_2 = D8; // GPIO pin for mux select line 2


void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  randomSeed(analogRead(0));

  pinMode(MUX_SELECT_PIN_1, OUTPUT);
  pinMode(MUX_SELECT_PIN_2, OUTPUT);


  // pinMode (Active_buzzer,OUTPUT) ;
  // pinMode (built_in_led,OUTPUT) ;

  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer.
  display.clearDisplay();
  display.setTextColor(WHITE); // Set text color to white
  display.setTextSize(1); // Set text size
  display.setCursor(0, 0); // Set cursor position

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  displayString("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  displayString("Connected");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  Serial.println(F("Sketch has been started!"));

  Serial.println(F("Waiting for a tag to scan..."));
  displayString("Waiting for a tag to scan...");

}


void loop() {  
  while(1){
    if (!readTag()) {
      delay(100);
      if(detectedBlockTag){
        detectedBlockTag = false;
        while (true) {
          if(!detectedBlockTag){
            while (!readTag()) {
                delay(100);
            }
          } else {
              // Detected another block tag, exit block scan mode
              detectedBlockTag = false;
              if(!emptyBlock){
                Serial.println(F("Waiting for a tag to scan..."));
                displayString("Waiting for a tag to scan...");
                break;
                // Exit the book scan loop and go back to waiting for a block tag
              }

            }
        }
      }
    }    
  }
}


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
  strcpy(wordBuffer, "");
  char displayBuffer[50]; // Buffer to store concatenated words
  int16_t wordLengths[50]; // Array to store the length of each word

  // Pre-process the input string to contain words with spaces and blank lines for '\n'
  char* token = strtok(workingStr, " ");
  while (token != NULL) {
    if (strcmp(token, "\n") == 0) {
      // Insert a next line for '\n'
      x = 0;
      y += textHeight; // Move to the next line

      display.setCursor(x, y);
      display.print(displayBuffer);

      strcpy(wordBuffer, "");
      strcpy(displayBuffer, wordBuffer);

    } else {
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
        strcpy(wordBuffer, token);
        strcat(wordBuffer, " ");
        strcpy(displayBuffer, "");
      }

      strcpy(displayBuffer, wordBuffer);
    }

    token = strtok(NULL, " ");
  }

  // Display any remaining concatenated words
  if (strlen(wordBuffer) > 0) {
    x = 0;
    y += textHeight; // Move to the next line
    display.setCursor(x, y);
    display.print(wordBuffer);
  }

  display.display();
}



bool readTag() {

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
    return false;

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())
    return false;

  // Read Book data ***************************************************
  Serial.println(F("Reading tag..."));
  displayString("Reading tag...");
  // Data in 4 blocks is read at once.
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    displayString("MIFARE_Read() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }


  // For block tag  **********************************

  String tagData = extractBlockTag(buffer, size);

  if (!tagData.isEmpty()) {

    detectedBlockTag = true;

    Serial.print(F("Detected Block Tag: "));

    digitalWrite(MUX_SELECT_PIN_1, HIGH);
    digitalWrite(MUX_SELECT_PIN_2, LOW);
    delay(50);
    digitalWrite(MUX_SELECT_PIN_1, LOW);
    digitalWrite(MUX_SELECT_PIN_2, LOW);

    char readData[120];
    snprintf(readData, sizeof(readData), "Detected Block Tag: %s.", tagData.c_str());
    displayString(readData);
    delay(2000);

    Serial.println(tagData);



    if (keysVector.empty()) {
      emptyBlock = true;

      currentBlockTag = tagData;
      sizeOfBooks=0; // Reset the book count when a new block is detected

      Serial.print(F("Block no: "));
      Serial.println(currentBlockTag);

      // Use UID as the key for the database
      String uid = getUID();
      Serial.print(F("UID: "));
      Serial.println(uid);

      Serial.print(F("Now scanning books in block: "));
      Serial.println(currentBlockTag);

      char readData[120];
      snprintf(readData, sizeof(readData), "Now scanning books in block: %s", currentBlockTag.c_str());
      displayString(readData);

      // blocksVector = getAllBlocks();
      // blocksVector.erase(blocksVector.begin());
      if(getAllBlocks(currentBlockTag)){
        keysVector = getBlock(currentBlockTag);
        keysVector.erase(keysVector.begin());
      } else {
        newBlock(currentBlockTag);
      }


    } else {
      emptyBlock = false;
      printKeysVector(currentBlockTag);
    }

    
    mfrc522.PICC_HaltA();
    return true;
  }

  // ************************************************



  // Extract and clean ISBN from the read data
  String isbn = extractISBN(buffer, size);
  if(strlen(isbn.c_str()) < 10){
    return false;
  }

  Serial.println(F("Reading book data..."));
  displayString("Reading book data...");

  bool check = checkUIDInBlockAndRemove(currentBlockTag);

  // Check if the book UID is already present in the current block
  if (check) {
    displayString("Book read successfully");
    Serial.print("Book read successfully");

    digitalWrite(MUX_SELECT_PIN_1, HIGH);
    digitalWrite(MUX_SELECT_PIN_2, LOW);
    delay(100);
    digitalWrite(MUX_SELECT_PIN_1, LOW);
    digitalWrite(MUX_SELECT_PIN_2, LOW);

    return true;
  }else{
     if (!isbn.isEmpty() && !isbn.startsWith("block_")) {
      // Serial.print(F("ISBN: "));
      // Serial.println(isbn);
      // displayString("Extra Book found");

      digitalWrite(MUX_SELECT_PIN_1, LOW);
      digitalWrite(MUX_SELECT_PIN_2, HIGH);
      delay(300);
      digitalWrite(MUX_SELECT_PIN_1, LOW);
      digitalWrite(MUX_SELECT_PIN_2, LOW);


      // Use UID as the key for the database
      String uid = getUID();
      Serial.print(F("UID: "));
      Serial.println(uid);


      String blockNo = getBooklocation(uid);

      if (!blockNo.isEmpty()){
        char readData[120];
        snprintf(readData, sizeof(readData), "Unknown Book found \n \n Book correct Location: %s",blockNo.c_str());
        displayString(readData);
        // delay(3000);

        Serial.print("Book correct Location: ");
        Serial.println(blockNo);
        return true;
      } else {

        if(currentBlockTag==""){
          displayString("Unregistered Book found \n \n Scan a block tag to add book to Database");
          delay(2000);
          return true;
        }

        char readData[120];
        snprintf(readData, sizeof(readData), "Unregistered Book found \n \n Adding Book to Database",blockNo.c_str());
        displayString(readData);
      }


      // Make an HTTP request to Open Library API
      String bookInfo = getBookInfo(isbn, 5);
      // Serial.println(F("API Response: "));
      // Serial.println(bookInfo); // Print API response for debugging
        
        String locationdb = "location/";
        String blockKey = database + currentBlockTag + "/books" + "/";
        String UidsKey = database + currentBlockTag + "/uids" + "/";
        String sizekey = database + currentBlockTag + "/size";

        // Adding dummy key initially
        Firebase.setString(UidsKey + "000", "000");
        Firebase.setBool(UidsKey + uid, true);
        Firebase.setString(locationdb + uid, currentBlockTag);

        // Adding dummy key initially
        Firebase.setString(blockKey + "000", "000");

        Firebase.setString(blockKey + uid + "/ISBN", isbn);
        Firebase.setString(blockKey + uid + "/Book_name", bookName);
        Firebase.setString(blockKey + uid + "/Author", author);
        Firebase.setString(blockKey + uid + "/Publisher", publisher);

        // Initialize sizeOfBooks from Firebase
        sizeOfBooks = getSizeOfBooksForBlock(currentBlockTag);

        // updating size of books in blocks
        Firebase.setInt("blocks/" + currentBlockTag, sizeOfBooks-1);

        Firebase.setInt(sizekey, sizeOfBooks-1);

        Serial.println(F("Book Tag Detected:"));
        Serial.print(F("ISBN: "));
        Serial.println(isbn);
        Serial.print(F("Book Name: "));
        Serial.println(bookName);
        Serial.print(F("No of Books read: "));
        Serial.println(sizeOfBooks); 

        digitalWrite(MUX_SELECT_PIN_1, HIGH);
        digitalWrite(MUX_SELECT_PIN_2, LOW);
        delay(100);
        digitalWrite(MUX_SELECT_PIN_1, LOW);
        digitalWrite(MUX_SELECT_PIN_2, LOW);

        displayString("Added New Book");
        bool checkSearch = verifyBlockSearch(bookName, currentBlockTag);
        delay(2000);

        char readData[120];
        snprintf(readData, sizeof(readData), "UID: %s \n \n Book name: %s", uid.c_str(),bookName.c_str());
        displayString(readData);

        mfrc522.PICC_HaltA();
        return true;
     }
    else {
      displayString("Invalid ISBN");
    }
  }

  return false;
}



bool verifyBlockSearch(String bookName, String blockName) {
  int quantity = 0;
  String searchdb = "search/" + bookName + "/" + blockName;

  for (int retry = 0; retry < 1; retry++) {
      quantity = Firebase.getInt(searchdb);
      if(quantity!=0){
        Firebase.setInt(searchdb, quantity + 1);
        Serial.print("update done");
        return true;
      }else{
        Serial.println(F("Trying to get book in search route, retry no:"));
        Serial.println(retry);
      } 
  }

  String bookSearchdb = "search/" + bookName + "/";
  Firebase.setString(bookSearchdb + "000", "000");
  // update quantity
  Firebase.setInt(searchdb, 1);
  Serial.println("Failed to verifyBookSearch from Firebase.");
  
  return false;
}



void newBlock(String blockName){
  String blocksdb = "blocks/";
  Firebase.setString(blocksdb + "000", "000");
  Firebase.setInt(blocksdb + blockName, 0);
  // Serial.print("done");
}


bool verifyBlock(String blockName) {
  if (!blocksVector.empty()) {
    for (auto it = blocksVector.begin(); it != blocksVector.end(); ++it) {
      if (*it == blockName) {
        return true;
      }
    }
  }

  return false;
}


// Function to check the existence of a block tag in Firebase
bool getAllBlocks(String blockName) {
  String blocksdb = "blocks/" + blockName;
  int block;
  for (int retry = 0; retry < 2; retry++) {
      block = Firebase.getInt(blocksdb);
      if(block!=0){
        Serial.println(F("Existing block found"));
        return true;
      }else{
        Serial.println(F("Trying to get book location, retry no:"));
        Serial.println(retry);
      } 
  }
  return false;
}  


  // blocksVector.clear();
  // jsonBuffer.clear();
  // JsonObject& blocks = Firebase.get(blocksdb).getJsonVariant().asObject();
  
  // if (blocks.success()) {
  //   for (JsonObject::iterator it = blocks.begin(); it != blocks.end(); ++it) {
  //     const char* key = it->key;
  //     // Store the key in the vector
  //     blocksVector.push_back(key);
  //   }
  // } else {
  //   Serial.println("Failed to getAllBlocks from Firebase.");
  // }
  
  // return blocksVector; // Return the vector of keys





String getBooklocation(String uid) {
  String block = "";
  String bookkey = "location/" + uid;

  for (int retry = 0; retry < 2; retry++) {
      block = Firebase.getString(bookkey);
      if(!block.isEmpty()){
        break;
      }else{
        Serial.println(F("Trying to get book location, retry no:"));
        Serial.println(retry);
      } 
  }

  return block;
  
}



bool checkUIDInBlockAndRemove(String blockName) {
  String currentUID = getUID();

  // Check if the current UID is in the keysVector
  if (!keysVector.empty()) {
    for (auto it = keysVector.begin(); it != keysVector.end(); ++it) {
      if (*it == currentUID) {
        // UID found, remove it from the vector
        keysVector.erase(it);
        return true;
      }
    }
  }
  // UID not found in the vector
  return false;
}



// get all books stored inside current block
std::vector<String> getBlock(String blockName) {

  keysVector.clear();


  String blockKey = database + blockName + "/uids";
  jsonBuffer.clear();
  JsonObject& books = Firebase.get(blockKey).getJsonVariant().asObject();
  
  if (books.success()) {
    for (JsonObject::iterator it = books.begin(); it != books.end(); ++it) {
      const char* key = it->key;
      // Store the key in the vector
      keysVector.push_back(key);
    }
  } else {
    Serial.println("Failed to getBlock from Firebase.");
  }
  
  return keysVector; // Return the vector of keys
}


// Function to print the contents of keysVector to the serial monitor
void printKeysVector(String currentBlockTag) {
  Serial.println("Keys Vector Contents:");

  int size = keysVector.size();

  char readData[120];
  snprintf(readData, sizeof(readData), "Total number of missing books: %d", size);
  Serial.println(readData);
  displayString(readData);
      

  digitalWrite(MUX_SELECT_PIN_1, HIGH);
  digitalWrite(MUX_SELECT_PIN_2, HIGH);
  delay(100);
  digitalWrite(MUX_SELECT_PIN_1, LOW);
  digitalWrite(MUX_SELECT_PIN_2, LOW);
  delay(50);
  digitalWrite(MUX_SELECT_PIN_1, HIGH);
  digitalWrite(MUX_SELECT_PIN_2, HIGH);
  delay(100);
  digitalWrite(MUX_SELECT_PIN_1, LOW);
  digitalWrite(MUX_SELECT_PIN_2, LOW);

  delay(2000); 
  
  for (const String& key : keysVector) {
    String Book_Name = "";
    String BookName = database + currentBlockTag + "/books/" + key + "/Book_name";


    for (int retry = 0; retry < 5; retry++) {
      Book_Name = Firebase.getString(BookName);
      if(!Book_Name.isEmpty()){
        break;
      }else{
        Serial.println(F("Trying to get book name, retry no:"));
        Serial.println(retry);
      } 
    }

    digitalWrite(MUX_SELECT_PIN_1, HIGH);
    digitalWrite(MUX_SELECT_PIN_2, HIGH);
    delay(100);
    digitalWrite(MUX_SELECT_PIN_1, LOW);
    digitalWrite(MUX_SELECT_PIN_2, LOW);


    if (!Book_Name.isEmpty()){
      char readData[120];
      snprintf(readData, sizeof(readData), "UID: %s \n \n Book name: %s", key.c_str(),Book_Name.c_str());
      Serial.println(readData);
      displayString(readData);
      delay(5000);
    }else{
      Serial.println("Failed to retrieve Book_Name from Firebase.");
      char readData[120];
      snprintf(readData, sizeof(readData), "UID: %s \n \n Book name: %s", key.c_str(),Book_Name.c_str());
      Serial.println(readData);
      displayString(readData);
      delay(5000);
    }

    Serial.println(key);
  }
  Serial.println("End of Keys Vector");
  keysVector.clear();
}




void printBooksJsonObject(JsonObject& books) {
  for (JsonObject::iterator it = books.begin(); it != books.end(); ++it) {
    const char* key = it->key;
    Serial.print("Key: ");
    Serial.println(key);

    // Assuming the values are strings, you can print them too
    const char* value = it->value.as<const char*>();
    Serial.print("Value: ");
    Serial.println(value);
  }
}



// get realtime size of books
int getSizeOfBooksForBlock(String blockName) {
  String blockKey = database + blockName + "/uids";
  jsonBuffer.clear();
  JsonObject& books = Firebase.get(blockKey).getJsonVariant().asObject();

  if (books.success()) {
    int itemCount = 0;
    for (auto kvp : books) {
      String key = kvp.key;
      if (!key.isEmpty()) {

        itemCount++;
      }
    }
    
    return itemCount;
  } else {
    Serial.println("Failed to retrieve data for size of books.");
    return 0; // Return 0 or another default value in case of an error.
  }
}


// extract isbn number
String extractISBN(byte *data, byte dataSize) {
  String isbn = "";
  bool isIsbn13 = false; // Flag to indicate if it's ISBN-13

  // Check if the data starts with "978" or "979" (indicators for ISBN-13)
  if (dataSize >= 3 && (data[0] == '9' && data[1] == '7' && (data[2] == '8' || data[2] == '9'))) {
    isIsbn13 = true;
  }

  // Loop through the data and add digits to the ISBN string, limiting to 13 digits for ISBN-13 or 10 digits for ISBN-10
  for (byte i = 0; i < dataSize && (isIsbn13 ? isbn.length() < 13 : isbn.length() < 10); i++) {
    if (isDigit(data[i])) {
      isbn += char(data[i]);
    }
  }

  return isbn;
}


// extract block tag data
String extractBlockTag(byte *data, byte dataSize) {
  String blockTag = "";
  // Wait for a short delay
  delay(1000); // Adjust the delay as needed
  int res = strncmp((const char*)data, "block_", 6);

  // Check if the data starts with "block_"
  if (res==0) {
    // Check if characters following "block_" are digits
    
    // Include "block_" as the initial part of the block tag
    blockTag += "block_";

    for (byte i = 6; i < dataSize; i++) {
      if (isDigit(data[i])) {
        blockTag += char(data[i]);
      } else {
        break; // Stop when a non-digit character is encountered
      }
    }
  }

  return blockTag;
}


// get the UID of tag
String getUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  return uid;
}


String getBookInfo(String isbn, int maxRetries) {
  // Create an HTTP client
  HTTPClient http;
  String payload = "";

  // String bookInfo[3]; // Initialize an array to store book information


  // Create the URL for the Open Library API request
  String url = "http://" + String(OPEN_LIBRARY_API_HOST) + "/api/books?bibkeys=ISBN:" + isbn + "&jscmd=data&format=json";

  // Send the GET request
  http.begin(url);
  int httpResponseCode = http.GET();
  delay(1000); // Add a delay to give time for the response

  // Check for a successful request
  if (httpResponseCode == 200) {
    payload = http.getString();

    // Check if the payload is empty or incomplete
    if (payload.isEmpty() || payload == "{}" || payload == "null") {
      Serial.println(F("Empty or incomplete API response."));
      displayString("Empty or incomplete API response.");
      return ""; // Return an empty string
    }

    // Print only the first 10 and last 10 characters of the payload for debugging
    // Serial.print(F("First 10 characters of payload: "));
    // Serial.println(payload.substring(0, 60));
    // Serial.print(F("Last 10 characters of payload: "));
    // Serial.println(payload.substring(payload.length() - 60));

    // Parse and store bookName, author, and publisher
    bookName = parseBookName(payload, isbn);
    author = parseAuthor(payload, isbn);
    publisher = parsePublisher(payload, isbn);

    // Retry up to maxRetries times if any field is empty
    for (int retry = 0; retry < maxRetries; retry++) {
      if(!bookName.isEmpty() && !author.isEmpty() && !publisher.isEmpty()){
        break;
      }else{
          if (bookName.isEmpty()) {
            bookName = parseBookName(payload, isbn);
          }
          if (author.isEmpty()) {
            author = parseAuthor(payload, isbn);
          }
          if (publisher.isEmpty()) {
            publisher = parsePublisher(payload, isbn);
          }
          Serial.println(F("Trying to resolve parse json, retry no:"));
          Serial.println(retry);

          char readData[120];
          snprintf(readData, sizeof(readData), "Trying to resolve parse json, retry no: %d", retry);
          displayString(readData);

      } 
    }

    return payload;
  } else {
    Serial.print(F("HTTP Error code: "));
    Serial.println(httpResponseCode);
    delay(1000); // Delay before retrying
  }

  return ""; // Return an empty string if maxRetries is exhausted
}


String parseBookName(String bookInfo, String isbn) {
  // Clear the existing buffer
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.parseObject(bookInfo);

  // Check if parsing was successful
  if (!root.success()) {
    Serial.println(F("Failed to parse JSON"));
    return "";
  }

  String isbnKey = "ISBN:" + isbn;

  if (root.containsKey(isbnKey)) {
    return root[isbnKey]["title"].asString();
  } else {
    Serial.println(F("ISBN not found in JSON"));
    return "";
  }
}


String parseAuthor(String bookInfo, String isbn) {
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.parseObject(bookInfo);
  if (!root.success()) {
    Serial.println(F("Failed to parse JSON"));
    return "";
  }

  String isbnKey = "ISBN:" + isbn;

  if (root.containsKey(isbnKey) && root[isbnKey]["authors"].size() > 0) {
    return root[isbnKey]["authors"][0]["name"].asString();
  } else {
    Serial.println(F("Author information not found in JSON"));
    return "";
  }

}

String parsePublisher(String bookInfo, String isbn) {
  // Clear the existing buffer
  jsonBuffer.clear();
  // Parse publisher from JSON response
  JsonObject& root = jsonBuffer.parseObject(bookInfo);
  if (!root.success()) {
    Serial.println(F("Failed to parse JSON"));
    return "";
  }

  String isbnKey = "ISBN:" + isbn;

  if (root.containsKey(isbnKey) && root[isbnKey]["publishers"].size() > 0) {
    return root[isbnKey]["publishers"][0]["name"].asString();
  } else {
    Serial.println(F("Publisher information not found in JSON"));
    return "";
  }

}

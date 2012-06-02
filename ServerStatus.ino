/*
 * Created May 2012
 * @author DietCoder https://github.com/DietCoder
 */

#include <SPI.h>
#include <Ethernet.h>

// --- START CONFIG --- 
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// server hosts
char* servers[] = {"arduino.cc", 
                   "sparkfun.com", 
                   "google.com"};

// paths on hosts                 
char* serverUrlPaths[] = {"/en/Main/FAQ",
                          "/",
                          "/"};
                          
#define server1RedPin   2
#define server1GreenPin 3
#define server2RedPin   4
#define server2GreenPin 5
#define server3RedPin   6
#define server3GreenPin 7

// --- END CONFIG ---

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// reference constants
#define NUM_SERVERS 3
#define LED_RED_STATE B0
#define LED_GREEN_STATE B1
#define LED_OFF_STATE B10

// state
unsigned int startServer = 1;
unsigned int currentServer;

// http state
int lastHTTPStatusCode = -1;
int cHTTPCharInLinePosition = 0;
unsigned int HTTPCharReadPerLineLimit = 15;
String lastLineString = "000000000000000";

void setup() {
  // setup the leds
  pinMode(server1GreenPin, OUTPUT);
  pinMode(server1RedPin, OUTPUT);
  
  pinMode(server2GreenPin, OUTPUT);
  pinMode(server2RedPin, OUTPUT);
  
  pinMode(server3GreenPin, OUTPUT);
  pinMode(server3RedPin, OUTPUT);
  
  setAllServersToStatus(LED_OFF_STATE);
  
  // init vars
  currentServer = startServer;
  
  // start the serial library:
  Serial.begin(9600);
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    
    // update ui
    setAllServersToStatus(LED_RED_STATE);
    
    // fail
    for(;;)
      ;
  }
  // give the ethernet shield time to initialize
  int waitTimeForComponentsToStartMs = 1000;
  delay(waitTimeForComponentsToStartMs);
  Serial.println();
  Serial.print(waitTimeForComponentsToStartMs);
  Serial.println("ms startup wait time elapsed");
  
  setupServerNRequest(currentServer);
}

void setupServerNRequest(int serverNumber) {
  //IPAddress server;
  int serversIndex;
  
  switch (serverNumber) {
    case 1:
      serversIndex = 0;
      break;
    case 2:
      serversIndex = 1;
      break;
    case 3:
      serversIndex = 2;
      break;
    default:
      Serial.println("Invalid server passed to setupServerNRequest");
      Serial.println(serverNumber);
      return;
  }
  
  if (client.connect(servers[serversIndex], 80)) {
    Serial.print("Connected to Server ");
    Serial.println(serverNumber);
    
    Serial.println("Sending GET package");
    
    // Make a HTTP request
    client.print("GET ");
    client.print(serverUrlPaths[serversIndex]);
    client.println(" HTTP/1.0");
    client.println();
  } 
  else {
    Serial.print("Server ");
    Serial.print(serverNumber);
    Serial.println(" connection failed");
    setServerNStatus(LED_RED_STATE,serverNumber);
  } 
}



void loop()
{
  // bytes from stream available
  if (client.available()) {
    
    char c = client.read();
    Serial.print(c);

    // look for the HTTP status code until it's found or there's nothing left to read
    if(lastHTTPStatusCode == -1) {
      
      if(c == '\n' || cHTTPCharInLinePosition == HTTPCharReadPerLineLimit) {
        
        // Parse HTTP status code - expected format HTTP/d.d ddd.*\n
        if(lastLineString.indexOf("HTTP/") == 0 && cHTTPCharInLinePosition > 12) {
            unsigned int httpStatusCodeLength = 3;
            unsigned int startHTTPCode = 12-httpStatusCodeLength;
            unsigned int endHTTPCode = 12;
            String httpCodeStr = lastLineString.substring(startHTTPCode, endHTTPCode);
            
            int httpStatusCodeTerminatedCharArrayLength = httpStatusCodeLength+1;
            char httpStatusChars[httpStatusCodeTerminatedCharArrayLength];
            httpCodeStr.toCharArray(httpStatusChars, httpStatusCodeTerminatedCharArrayLength);
            lastHTTPStatusCode = atoi(httpStatusChars);
        }
        
        cHTTPCharInLinePosition = 0;
      }
      else if(cHTTPCharInLinePosition < HTTPCharReadPerLineLimit) {
        lastLineString.setCharAt(cHTTPCharInLinePosition,c);
        ++cHTTPCharInLinePosition;
      }
    }
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    
    Serial.println();
    Serial.print("[STATUS]Server ");
    Serial.print(currentServer);
    Serial.print(" ");
    Serial.print(lastHTTPStatusCode);
    Serial.println(" HTTP status");
    
    // update server status LED
    if(lastHTTPStatusCode == 200)
        setServerNStatus(LED_GREEN_STATE,currentServer);
    else
        setServerNStatus(LED_RED_STATE,currentServer);
    
    // reset parsing state
    cHTTPCharInLinePosition = 0;
    
    // reset http status code
    lastHTTPStatusCode = -1;
    
    Serial.print("disconnecting from server ");
    Serial.println(currentServer);
    client.stop();
    
    // next server
    currentServer = (currentServer + 1) % NUM_SERVERS;
    currentServer = currentServer == 0 ? 1 : currentServer;   
    
    // after delay
    Serial.println("Sleep...");
    delay(60000);
    Serial.println("Awake...");
    setupServerNRequest(currentServer);
  }
}

void setAllServersToStatus(byte state) {
    for(int i = 1; i < NUM_SERVERS+1; i++)
      setServerNStatus(state,i);
}

void setServerNStatus(byte state, int serverNumber) {
  int greenPin = 1;
  int redPin = 0;
  
  switch (serverNumber) {
    case 1:
      greenPin = server1GreenPin;
      redPin = server1RedPin;
      break;
    case 2:
      greenPin = server2GreenPin;
      redPin = server2RedPin;
      break;
    case 3:
      greenPin = server3GreenPin;
      redPin = server3RedPin;
      break;
    default: 
      // if nothing else matches, do the default
      // default is optional
      Serial.println("Invalid server passed to setServerNStatus");
      Serial.println(state);
      Serial.println(serverNumber);
      return;
  }
  
  
  switch (state) {
    case B0: //LED_RED_STATE
      digitalWrite(greenPin, LOW);
      digitalWrite(redPin, HIGH);
      break;
    case B1: //LED_GREEN_STATE
      digitalWrite(greenPin, HIGH);
      digitalWrite(redPin, LOW);
      break;
    case B10: //LED_OFF_STATE
      digitalWrite(greenPin, LOW);
      digitalWrite(redPin, LOW);
      break;
    default: 
      // if nothing else matches, do the default
      // default is optional
      Serial.println("Invalid state passed to setServerNStatus");
      Serial.println(state);
      Serial.println(serverNumber);
      return;
  }
}

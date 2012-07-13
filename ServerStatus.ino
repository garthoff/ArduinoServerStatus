/*
 * Created May 2012
 * @author DietCoder https://github.com/DietCoder
 */

#include <SPI.h>
#include <Ethernet.h>

// --- START Server config ---
// Enter MAC address printed on ethernet shield
byte mac[] = {  0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// server hosts
char* servers[] = {"arduino.cc", 
                   "sparkfun.com", 
                   "google.com"};

// server ports
int serverPorts[] = {80, 80, 80};

// paths on server hosts                 
char* serverUrlPaths[] = {"/en/Main/FAQ",
                          "/",
                          "/"};
// --- END Server config ---

#define server1RedPin   3
#define server1GreenPin 2
#define server2RedPin   6
#define server2GreenPin 5
#define server3RedPin   8
#define server3GreenPin 7

#define sleepTimeBetweenServerConnectionsMs 60000
#define waitTimeForComponentsToStartMs 1000

// Ethernet client library
EthernetClient client;

// reference constants
#define LED_RED_STATE B0
#define LED_GREEN_STATE B1
#define LED_OFF_STATE B10

#define NUM_SERVERS 3

// state
int startServer;
int currentServer;

// http state
int lastHTTPStatusCode;
int cHTTPCharInLinePosition;
int HTTPCharReadPerLineLimit;
String lastLineString;

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
  lastHTTPStatusCode = -1;
  cHTTPCharInLinePosition = 0;
  HTTPCharReadPerLineLimit = 15;
  lastLineString = "000000000000000";
  startServer = 1;
  currentServer = startServer;
  
  // start the serial library
  Serial.begin(9600);
  
  // start the ethernet connection
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    
    // update leds
    setAllServersToStatus(LED_RED_STATE);
    
    // fail
    for(;;)
      ;
  }
  
  // give the ethernet shield time to initialize
  delay(waitTimeForComponentsToStartMs);
  Serial.println();
  Serial.print(waitTimeForComponentsToStartMs);
  Serial.println("ms startup wait time elapsed");
  
  setupServerNRequest(currentServer);
}

void setupServerNRequest(int serverNumber) {
  if(serverNumber > NUM_SERVERS) {
    Serial.println("Invalid server passed to setupServerNRequest");
      Serial.println(serverNumber);
      return;
  }
  
  
  // 0 based index
  int serversIndex = serverNumber - 1;
  
  if (client.connect(servers[serversIndex], serverPorts[serversIndex])) {
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
  // bytes from recieve stream available
  if (client.available()) {
    
    // print the server's entire response to the serial connection for debugging
    char c = client.read();
    Serial.print(c);

    // look for the HTTP status code until it's found or there's nothing left to read
    if(lastHTTPStatusCode == -1) {
      
      if(c == '\n' || cHTTPCharInLinePosition == HTTPCharReadPerLineLimit) {
        
        // Parse HTTP status code - expected format HTTP/... ddd.*\n
        if(lastLineString.indexOf("HTTP/") == 0 && cHTTPCharInLinePosition > 12) {
            int httpStatusCodeLength = 3;
            int startHTTPCode = 12-httpStatusCodeLength;
            int endHTTPCode = 12;
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

  // connection closed
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
    
    // sleep only if we've connected to all servers
    boolean shouldSleep = currentServer == NUM_SERVERS;
    
    // next server
    currentServer = (currentServer % NUM_SERVERS) + 1;
    
    // after delay
    if(shouldSleep) {
      Serial.print("Sleeping ");
      Serial.print(sleepTimeBetweenServerConnectionsMs);
      Serial.println("ms ...");
      delay(sleepTimeBetweenServerConnectionsMs);
      Serial.println("... Awake");
    }
    
    setupServerNRequest(currentServer);
  }
}

void setAllServersToStatus(byte state) {
    for(int i = 1; i <= NUM_SERVERS; i++)
      setServerNStatus(state,i);
}

void setServerNStatus(byte state, int serverNumber) {
  int greenPin;
  int redPin;
  
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
      Serial.println("Invalid server passed to setServerNStatus");
      Serial.println(state);
      Serial.println(serverNumber);
      return;
  }
  
  switch (state) {
    case LED_RED_STATE:
      digitalWrite(greenPin, LOW);
      digitalWrite(redPin, HIGH);
      break;
    case LED_GREEN_STATE:
      digitalWrite(greenPin, HIGH);
      digitalWrite(redPin, LOW);
      break;
    case LED_OFF_STATE:
      digitalWrite(greenPin, LOW);
      digitalWrite(redPin, LOW);
      break;
    default:
      Serial.println("Invalid state passed to setServerNStatus");
      Serial.println(state);
      Serial.println(serverNumber);
      return;
  }
}
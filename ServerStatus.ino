/*
 * Created May 2012
 * @author DietCoder https://github.com/DietCoder
 */

#include <SPI.h>
#include <Ethernet.h>

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

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

int server1RedPin = 2;
int server1GreenPin = 3;

int server2RedPin = 4;
int server2GreenPin = 5;

int server3RedPin = 6;
int server3GreenPin = 7;

int numServers = 3;

byte RED_STATE = B0;
byte GREEN_STATE = B1;
byte OFF_STATE = B10;

// state
int startServer = 1;
int currentServer;

// http state
int lastHTTPStatusCode = -1;
int cHTTPCharInLinePosition = 0;
int HTTPCharReadPerLineLimit = 15;
String lastLineString = "000000000000000";

void setup() {
  // setup the leds
  pinMode(server1GreenPin, OUTPUT);
  pinMode(server1RedPin, OUTPUT);
  pinMode(server2GreenPin, OUTPUT);
  pinMode(server2RedPin, OUTPUT);
  pinMode(server3GreenPin, OUTPUT);
  pinMode(server3RedPin, OUTPUT);
  
  setAllLEDsOFF();
  
  // init vars
  currentServer = startServer;
  
  // start the serial library:
  Serial.begin(9600);
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    
    // update ui
    setAllLEDsRed();
    
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // give the Ethernet shield a second to initialize:
  delay(10000);
  Serial.println("Startup complete ...");
  
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
      // if nothing else matches, do the default
      // default is optional
      Serial.println("Invalid server passed to setupServerNRequest");
      Serial.println(serverNumber);
      return;
  }
  
  // if you get a connection, report back via serial:
  if (client.connect(servers[serversIndex], 80)) {
    Serial.print("Connected to Server ");
    Serial.print(serverNumber);
    Serial.println();
    
    Serial.println("Sending GET package");
    
    // Make a HTTP request:
    client.print("GET ");
    client.print(serverUrlPaths[serversIndex]);
    client.println(" HTTP/1.0");
    client.println();
  } 
  else {
    // kf you didn't get a connection to the server:
    Serial.print("Server ");
    Serial.print(serverNumber);
    Serial.print(" connection failed");
    Serial.println();
    setServerNStatus(RED_STATE,serverNumber);
  } 
}



void loop()
{
  // if there are incoming bytes available 
  // from the server, read them and print them:
  if (client.available()) {
    
    char c = client.read();
    Serial.print(c);

    // look for the HTTP status code until it's found
    if(lastHTTPStatusCode == -1) {
      
      if(c == '\n' || cHTTPCharInLinePosition == HTTPCharReadPerLineLimit) {
        if(lastLineString.indexOf("HTTP/1.1 200") > -1)
          lastHTTPStatusCode = 200;
        else if(lastLineString.indexOf("HTTP/1.1 404") > -1)
          lastHTTPStatusCode = 404;
        else if(lastLineString.indexOf("HTTP/1.1 503") > -1)
          lastHTTPStatusCode = 503;
        
        // clear the string?
        //for(int i = 0; i < HTTPCharReadPerLineLimit; i++)
        //  lastLineString.setCharAt(cHTTPCharInLinePosition,'0');
        
        cHTTPCharInLinePosition = 0;
      }
      else if(cHTTPCharInLinePosition < HTTPCharReadPerLineLimit) {
        lastLineString.setCharAt(cHTTPCharInLinePosition,c);
        cHTTPCharInLinePosition++;
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
    Serial.print(" HTTP status");
    Serial.println();
    
    // update the status of this server
    // base on the response code
    if(lastHTTPStatusCode == 200)
        setServerNStatus(GREEN_STATE,currentServer);
    else
        setServerNStatus(RED_STATE,currentServer);
    
    // reset parsing state
    cHTTPCharInLinePosition = 0;
    
    // reset http status code
    lastHTTPStatusCode = -1;
    
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    
    // next server
    if((currentServer+1) > numServers)
      currentServer = startServer;
    else
      currentServer = currentServer + 1;
    
    // after 60 sec sleep
    Serial.println("Sleep...");
    delay(60000);
    Serial.println("Awake...");
    setupServerNRequest(currentServer);
  }
}

void setAllLEDsRed() {
  for(int i = 1; i < numServers+1; i++)
    setServerNStatus(RED_STATE,i);
}

void setAllLEDsOFF() {
  for(int i = 1; i < numServers+1; i++)
    setServerNStatus(OFF_STATE,i);
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
    case B0: //RED_STATE
      digitalWrite(greenPin, LOW);
      digitalWrite(redPin, HIGH);
      break;
    case B1: //GREEN_STATE
      digitalWrite(greenPin, HIGH);
      digitalWrite(redPin, LOW);
      break;
    case B10: //OFF_STATE
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

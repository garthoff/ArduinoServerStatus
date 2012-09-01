/*
 * @author DietCoder https://github.com/DietCoder
 */

#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h> // for soft reboot

// --- Start Server Config ---
// Enter mac address printed on bottom of ethernet board
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
// --- End Server Config ---

#define server1RedPin   3
#define server1GreenPin 2
#define server2RedPin   6
#define server2GreenPin 5
#define server3RedPin   8
#define server3GreenPin 7

#define sleepTimeBetweenServerConnectionsMs 60000
#define serialPrintHttpResponse 0

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// enables software reboot
#define onErrorSleepTimeBeforeSoftResetting 60000
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
// Function Implementation of watchdog timer init to off
void wdt_init(void) { MCUSR = 0; wdt_disable(); return; }

// reference constants
#define NUM_SERVERS 3
#define httpStatusCodeNotFound -1
#define httpInvalidStatusCode -2

#define LED_RED_STATE B0
#define LED_GREEN_STATE B1
#define LED_OFF_STATE B10

// state
boolean sendNextExternalClientRequest;
int currentServer;
unsigned long nextExternalServersCheckMili;
int retrieve_server_status_watchdog_counter;
const int serverStatusWatchDogLength = NUM_SERVERS*5; // reset after 1 round of no connection errors
int serverStatusWatchdog[serverStatusWatchDogLength];
int cServerStatusWatchDogIndex;

// http state
int lastHTTPStatusCode;
int cHTTPCharInLinePosition;
int HTTPCharReadPerLineLimit;
String lastLineString;

void setup() {
  // init watchdog timer to off
  wdt_init();

  // setup the leds
  pinMode(server1GreenPin, OUTPUT);
  pinMode(server1RedPin, OUTPUT);

  pinMode(server2GreenPin, OUTPUT);
  pinMode(server2RedPin, OUTPUT);

  pinMode(server3GreenPin, OUTPUT);
  pinMode(server3RedPin, OUTPUT);

  // init vars
  sendNextExternalClientRequest = true;
  currentServer = 1; // start server
  retrieve_server_status_watchdog_counter = 0;
  nextExternalServersCheckMili = 0;
  lastHTTPStatusCode = httpStatusCodeNotFound;
  cHTTPCharInLinePosition = 0;
  HTTPCharReadPerLineLimit = 15;
  lastLineString = "000000000000000";
  // reset the arduino if we get 10 rounds of failure in a row
  // init the state to all 200's
  cServerStatusWatchDogIndex = 0;
  for(int i = 0; i < serverStatusWatchDogLength; i++) {
      serverStatusWatchdog[i] = 200;
  }

  // start the serial library:
  Serial.begin(9600);
  Serial.println("Startup");

  // init display state
  setAllServersToStatus(LED_OFF_STATE);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");

    // update leds
    setAllServersToStatus(LED_RED_STATE);

    // reset and try again
    softReset();
  }

  // give the ethernet shield time to initialize
  int waitTimeForComponentsToStartMs = 1000;
  delay(waitTimeForComponentsToStartMs);
  Serial.println();
  Serial.print(waitTimeForComponentsToStartMs);
  Serial.println("ms startup wait time elapsed");
}

void setupServerNRequest(int serverNumber) {
  if(serverNumber > NUM_SERVERS) {
    Serial.println("Invalid server passed to setupServerNRequest");
      Serial.println(serverNumber);
      return;
  }

  int serversIndex = serverNumber - 1;

  Serial.print("Connecting to Server ");
  Serial.print(serverNumber);
  Serial.print(" ");
  Serial.print(servers[serversIndex]);
  Serial.print(":");
  Serial.println(serverPorts[serversIndex]);

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

void loop() {
  unsigned long milli = millis();
  if(nextExternalServersCheckMili == 0 || (milli > nextExternalServersCheckMili && ((milli + sleepTimeBetweenServerConnectionsMs) > milli))) {
      retrieve_server_status_loop();
  }
   
   // sprinkled in every unbounded loop
   maintain();
}

void maintain() {
   // supposed to do ip address lease renewal if called at least once a second
   Ethernet.maintain();
}

void retrieve_server_status_loop()
{
  if(sendNextExternalClientRequest) {
    sendNextExternalClientRequest = false;
    setupServerNRequest(currentServer);
  }

  // bytes from recieve stream available
  if (client.available()) {

    char c = client.read();
#if serialPrintHttpResponse
    Serial.print(c);
#endif

    // look for the HTTP status code until it's found or there's nothing left to read
    if(lastHTTPStatusCode == httpStatusCodeNotFound) {

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
            unsigned long safeHttpPStatusCode = strtoul(httpStatusChars, NULL, 10);

            // 1dd-5dd defined in http 1.1 rfc. 2dd-5dd defined in http 1.0 rfc
            if(safeHttpPStatusCode < 600 && safeHttpPStatusCode > 99)
              lastHTTPStatusCode = (int)safeHttpPStatusCode;
            else
              lastHTTPStatusCode = httpInvalidStatusCode;
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
        
    // update watchdog status
    serverStatusWatchdog[cServerStatusWatchDogIndex] = lastHTTPStatusCode;
    cServerStatusWatchDogIndex = (cServerStatusWatchDogIndex + 1) % serverStatusWatchDogLength;

    // reset parsing state
    cHTTPCharInLinePosition = 0;

    // reset http status code
    lastHTTPStatusCode = httpStatusCodeNotFound;

    Serial.print("disconnecting from server ");
    Serial.println(currentServer);
    client.stop();

    // determine if the server status has been all errors for too long
    // via watchdog
    boolean shouldSoftReset = serverStatusWatchDogLength > 0;
    for(int i = 0; i < serverStatusWatchDogLength; i++) {
        shouldSoftReset &= serverStatusWatchdog[i] == httpStatusCodeNotFound;
    }
    if(shouldSoftReset)
      softReset();

    // sleep only if we've connected to all servers
    boolean shouldSleep = currentServer == NUM_SERVERS;

    // next server
    currentServer = (currentServer % NUM_SERVERS) + 1;

    // after delay
    if(shouldSleep) {
      nextExternalServersCheckMili = millis() + sleepTimeBetweenServerConnectionsMs;

      Serial.print("Scheduling next external server check in ");
      Serial.print(sleepTimeBetweenServerConnectionsMs);
      Serial.println("ms");
    }

    // connect to the next server during the next loop
    sendNextExternalClientRequest = true;
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

void softReset() {
    Serial.print("Soft resetting in ");
    Serial.print(onErrorSleepTimeBeforeSoftResetting);
    Serial.println("ms");

    delay(onErrorSleepTimeBeforeSoftResetting);

    // enable watchdog with short timeout on inactivity
    wdt_enable(WDTO_15MS);

    // do nothing until watchdog resets us
    for(;;){
    }
}
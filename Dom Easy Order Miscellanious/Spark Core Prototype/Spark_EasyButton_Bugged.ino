// This #include statement was automatically added by the Spark IDE.


// This #include statement was automatically added by the Spark IDE.
#include "application.h"
#include "neopixel/neopixel.h"

//Dominos Easy Order Button to Post, Get, and check status using HTTP Requests
//Press Button Once to initially post request to server
//Press Again to Confirm order
//Press Subsequently to check status (States 1:5) of Order
//Miran Alhaideri - 07/22/14

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D1         
#define PIXEL_COUNT 8        
#define PIXEL_TYPE WS2812B   

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

typedef enum {INITIAL, WAITING_CONFIRM, STATUS} state;            //different states for Device to be in. Determines flow control
typedef enum {READY_TO_ORDER, ORDER_PLACED, PREP, BAKE, QUALITY_CHECK, OUT_FOR_DELIVERY, FINISHED} tracker;     //corresponds to 7 states of pizza tracker

TCPClient client;
byte server[] = { 104, 131, 136, 115 }; // Gabe's Server IP Address

#define Button A0
#define IDLE_TIMEOUT_MS 3000

//GLOBAL VARIABLES
char *confNum;               //stores confirmation # of HTTP init Post request used throughout code
char *conf_response;             //stores the response of the get_user_confirmation() HTTP request to server. should equal "confirmed"
char *status_string;          //holds the status value returned from http request.
volatile state dom_state = INITIAL;      //dominos state variable to determine HTTP requests, polling, etc.
volatile tracker pizza_tracker = READY_TO_ORDER;      //initial state. ready for order

void setup()
{
  //setup button for http request
  pinMode(Button, INPUT_PULLUP);

  strip.setBrightness(100);
  strip.begin();
  strip.show();     // Initialize all pixels to 'off'

  //Make sure your Serial Terminal app is closed before powering your Core
  Serial.begin(9600);
  // Now open your Serial Terminal, and hit any key to continue!
  //  while(!Serial.available()) SPARK_WLAN_Loop();

  Serial.println("connecting...");

    //diagnostic run through all lights individually (NON-ESSENTIAL)
    for (int i = 0; i< PIXEL_COUNT; i++) {
        strip.setPixelColor(i, Wheel(20*i));
        strip.show();
        delay(500);
    }
    //turn all pixels off
    for (int i=0; i<PIXEL_COUNT; i++) {
        strip.setPixelColor(i, 0);
    }
    strip.show();
    
}//end setup

void loop()
{

/*
    // if we have a confirmation number, it means we're currently tracking our order
    // if we don't have one, we're waiting to start a new order
    
    if(dom_state == STATUS){
        get_status();
        delay(5000);
    }else if(dom_state == WAITING_CONFIRM) {   
        if (confirm_order() == true) {
            dom_state = STATUS;
        }
        delay(2000);
    }else if(dom_state == INITIAL) {
        // waiting for a new order
        int val = digitalRead(Button);
        Serial.print("Button = ");
        Serial.println(val);
        if (val == 0) {
            new_order();    //place new order
            dom_state = WAITING_CONFIRM;
        }
        delay(250);
    }
*/

/*
    //Test Status indicator
    if (1) {
        //test status_indicator
        status_indicator(0, Wheel(80));
        delay(2000);
        status_indicator(1, Wheel(80));
        delay(2000);
        status_indicator(2, Wheel(80));
        delay(2000);
        status_indicator(3, Wheel(80));
        delay(2000);
        status_indicator(4, Wheel(80));
        delay(2000);
        status_indicator(5, Wheel(80));
        delay(2000);
        status_indicator(6, Wheel(80));
        delay(2000);
    }
*/
    //char A[] = get_string('A');
//    char B = *get_string('A');
//    char C[] = "Hello";

    int button_state = digitalRead(Button);     //sample button
    //state machine for device
    switch (dom_state) {
        case INITIAL:
            Serial.println("Entered Initial State");
            //if button is pressed, request (init) Post from Server
            if (button_state == LOW)     //if Button pressed
            {
                Serial.println("Button Pressed");
                new_order();            //Post new Order Request, order confirmation # generated
                dom_state = WAITING_CONFIRM;        //change state
            }    
            break;
        case WAITING_CONFIRM:       //blink LED while waiting for order to be confirmed
            Serial.println("Entered Waiting_confirm State");
            //press to confirm order 
            button_state = digitalRead(Button);     //may be redundant (sanity check)
            if (button_state == LOW) {
                bool confirmed = confirm_order();       //send http confirmation request
                if (confirmed) {
                    Serial.println();
                    Serial.println("Order confirmed!");
                    dom_state = STATUS;
                }
                else {
                    Serial.println();
                    Serial.println("Order NOT confirmed!");
                }
            } 
            else {
                //blink 1st LED
                status_indicator(0, Wheel(80), 1250, 1000);     //Status = 0 (blinks 1st LED)
                Serial.println("Order is waiting Confirmation!");
                }
            break;
        case STATUS:
            Serial.println("Entered STATUS State");
            //add code here to blink appropriate # of LEDs
            int status = get_status();      //ping server for status update
            
            //set LEDs based on status
            status_indicator(status, Wheel(80), 750, 500);    //Turn on for 1250 ms, turn off for 1000 ms (blinking rates)
            
            Serial.print("Status = ");
            Serial.print(status);
            
            //clear order once status = 6
            if (status == 6) {
                //call a clear order function
                delay(5000);       //delay 10 sec
                //turn off display and clear order
                clear_order();      //how do we verify that orders were cleared?
                dom_state = INITIAL;        //go back to initial state
            }
            delay(5000);    //determines polling rate in this state
            break;
    }//end switch
    
} //end loop

/*
//parse Text between start and stop characters
char* parseText(char start, char stop, int time ) {
  //printing the return of the request
  bool startRead = false;
  unsigned long lastRead = millis();
  int stringPos = 0;
  //memset( &confNum, 0, 32 );        
  char parsedResponse[32] = {};
  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      
    //   //parse the confirmation # out of the string and store it in confNum c-string array
      if ( c == start && !startRead){
        startRead = true;
      }
      if ( startRead && c != start){
        if ( c != stop ){
          parsedResponse[stringPos] = c;
          stringPos++;
        }
        else {
          startRead = false;
        }
      }
       lastRead = millis();
    }//inner while
  }//outer while    
    
    return *parsedResonse;    
    
}//end parseText
*/

//initial (post) HTTP Request when user pushes button for the first time
//parses the confirmation # from the server post response and sets the confNum global variable
//sends POST http request to server to indicate that a new order has been placed
//stores returned confirmation number in a global cstring
//returns True or False indicating if the order was successfully placed

void new_order()
{
    Serial.println();
    Serial.println(" -- new_order() called --");
  if (client.connect(server, 3000))
  {
    Serial.println("connected");
    client.println("POST /v1/orders/new/sparkCore HTTP/1.1");
    client.println("Host: 104.131.136.115:3000");
    client.println("Content-Length: 0");
    client.println();
  }
  else
  {
    Serial.println("connection failed");
  }
  bool startRead = false;   //sets flag for confirmation #
  //printing the return of the request
  unsigned long lastRead = millis();
  int stringPos = 0;
  //memset( &confNum, 0, 32 );
  confNum = {};
  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      
    //parse the confirmation # out of the string and store it in confNum c-string array
      if ( c == '{' && !startRead){
        startRead = true;
      }
      if ( startRead && c != '{'){
        if ( c != '}' ){
          confNum[stringPos] = c;
          stringPos++;
        }
        else {
          startRead = false;
        }
      }
      lastRead = millis();
    }//inner while
  }//outer while

  Serial.println("");
  Serial.println("");
  Serial.print("Confirmation # = ");
  Serial.println(confNum);
} //end init_requests

//clear order after wait time has elapsed
//send DELETE HTTP Request when called
void clear_order()
{
    Serial.println();
    Serial.println(" -- clear_order() called --");
    if (client.connect(server, 3000))
    {
        Serial.println("connected");
        client.println("DELETE /v1/orders/clear HTTP/1.1");
        client.println("Host: 104.131.136.115:3000");
        client.println("Content-Length: 0");
        client.println();
    }
    else
    {
    Serial.println("connection failed");
    }

}//end clear order

//Makes a GET request to the server using confNum from init_request
//sets the order to "confirmed" and returns true if server returns back "confirmed"
bool confirm_order()
{
  bool startRead = false;
  Serial.println();
  Serial.println(" -- confirm_order() called --");

  if (client.connect(server, 3000))
  {
    Serial.println("** connected");
    client.print("GET /v1/orders/confirm/");
    client.print(confNum);
    client.println(" HTTP/1.1");
    client.println("Host: 104.131.136.115:3000");
    client.println("Content-Length: 0");
    client.println();
  }
  else
  {
    Serial.println("connection failed");
  }

  //printing the return of the request
  unsigned long lastRead = millis();
  int stringPos = 0;
  memset( &conf_response, 0, 32 );            //sets first 32 bytes of response to 0 ??? Why do we need this?
  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);      //diagnostic - Delete when done
      
      //don't need this anymore
      //parse the 'confirmed' keyword out of the string
      if ( c == '{' && !startRead){    //starting condition
        startRead = true;
      }
      if ( startRead && c != '{' ){
        if ( c != '}' ){       //stopping condition
          conf_response[stringPos] = c;      
          stringPos++;
        }
        else {
          startRead = false;
        }
      }
      lastRead = millis();
    }//end inner while
  }//end outer while

  client.stop();
  Serial.print("** http://104.131.136.115:3000/v1/orders/confirm/");
  Serial.println(confNum);
  Serial.println("");
  Serial.print("Response # = ");
  Serial.println(conf_response);
  
  //check if conf_response == "confirmed"
  if (strcmp(conf_response, "confirmed") == 0) {    //check if response == "confirmed" 
        Serial.print("Order Confirmed: TRUE");
        return true;
  }else{
        Serial.print("Order Confirmed: FALSE");
        return false;
  }
    
}//end get_user_confirmation

//get user status (2nd button press)
//returns status (b/t 0 and 6) from the server response. 0-order not confirmed. 1-6: state of tracker
int get_status()
{
  bool startRead = false;
    Serial.println();
    Serial.println();
    Serial.println(" -- get_status() called --");
    Serial.print(" **** SANITY CHECK confNum: ");
    Serial.print(confNum);
    Serial.println(" ****");

  if (client.connect(server, 3000))
  {
    Serial.println("connected");
    client.print("GET /v1/orders/status/");
    client.print(confNum);
    client.println(" HTTP/1.1");
    client.println("Host: 104.131.136.115:3000");
    client.println("Content-Length: 0");
    client.println();
  }
  else
  {
    Serial.println("connection failed");
  }

  //printing the return of the request
  unsigned long lastRead = millis();
  int stringPos = 0;
//   memset( &confNum, 0, 32 );
  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      
      //parse the status out of the string. Might not need to loop but we can keep it general for now.
      if ( c == '{' && !startRead){
        startRead = true;
      }
      if ( startRead && c != '{' ){
        if ( c != '}' ){
          status_string[stringPos] = c;
          stringPos++;
        }
        else {
          startRead = false;
        }
      }
      lastRead = millis();
    }
  }
  client.stop();
  Serial.print("** http://104.131.136.115:3000/v1/orders/status/");
  Serial.println(confNum);
  Serial.println("");
  Serial.print("Response # = ");
  Serial.println(status_string);

  return atoi(status_string);     //converts status to an integer and return it

}//end get_status

//NeoPixels
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//light appropriate NeoPixel LEDs based on the status value
//status = 0 to 6 indicating state of tracker
//color = output of Wheel() function to determine RGB color
//wait_on: time (ms) LED blinks high        typ: 1250
//wait_off: time (ms) LED blinks low        typ: 1000, slightly shorter to accomodate loop delays
void status_indicator(int status, int color, int wait_on, int wait_off)
{
    switch (status) {
        case READY_TO_ORDER:     //STATUS = 0
            //all LEDs off
            for (int i=0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, 0);
            }
        case ORDER_PLACED:     //STATUS = 1
            //blink 1st LED
            strip.setPixelColor(0, color);     //ON
            strip.show();
            delay(wait_on);
            strip.setPixelColor(0, 0);      //Off
            strip.show();
            delay(wait_off);
            break;
        case PREP:     //STATUS = 2
            strip.setPixelColor(0, color);  //turn 1st led ON
            //blink 2nd LED
            strip.setPixelColor(1, color);     //ON
            strip.show();
            delay(wait_on);
            strip.setPixelColor(1, 0);      //Off
            strip.show();
            delay(wait_off);
            //Turn other LEDs off
            for (int i = 2; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, 0);
            }
            strip.show();       //update LED display
            break;
        case BAKE:      //STATUS = 3  
            //turn first 2 LEDs on
            for (int i = 0; i < 2; i++) {
                strip.setPixelColor(i, color);  //2 On
            }
            //blink LEDS 3:4
            strip.setPixelColor(2, color);  //on
            strip.setPixelColor(3, color);  //on
            strip.show();
            delay(wait_on);
            strip.setPixelColor(2, 0);  //off
            strip.setPixelColor(3, 0);  //off
            delay(wait_off);
            //turn remaining LEDs off
            for (int i=4; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, 0);  //6 Off
            }
            strip.show();   //update
            break;
        case QUALITY_CHECK:          //STATUS = 4
            //turn first 4 LEDs on
            for (int i = 0; i < 4; i++) {       //4 LEDs on
                strip.setPixelColor(i, color);  //On
            }
            //blink 5th LED
            strip.setPixelColor(4, color);
            strip.show();
            delay(wait_on);
            strip.setPixelColor(4, 0);
            strip.show();
            delay(wait_off);
            
            for (int i=5; i<strip.numPixels(); i++) {   //4 Off
                strip.setPixelColor(i, 0);  //Off
            }
            strip.show();   //update
            break;
        case OUT_FOR_DELIVERY:     //STATUS = 5
            //turn first 5 LEDs on
            for (int i = 0; i < 5; i++) {
                strip.setPixelColor(i, color);  //5 On
            }
            //blink last 3 LEDs
            for (int i = 5; i < strip.numPixels(); i++) {           //ON
                strip.setPixelColor(i, color);  //5 On
            }
            strip.show();
            delay(wait_on);
            for (int i = 5; i < strip.numPixels(); i++) {           //OFF
                strip.setPixelColor(i, 0);  //5 On
            }            
            strip.show();
            delay(wait_off);
            break;
        case FINISHED:     //STATUS = 6
            //turn all LEDs on (solid)
            for (int i=0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, color);
            }
            strip.show();
        
    }//end switch
    
}//end status_indicator


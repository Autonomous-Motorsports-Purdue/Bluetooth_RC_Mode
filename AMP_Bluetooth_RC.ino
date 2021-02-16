/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution

  Use: download Bluefruit Connect app --> connect to Bluefruit feather
    --> controller --> control Pad --> 

  Press button 1 --> reset to no throttle, steering angle middle
  Press forward [^] button --> increate speed by THROTTLE_RESOLUITION
  Press reverse [v] button --> decrease speed by THROTTLE_RESOLUITION
  Press right [->] button --> increase steering angle by STEERING_RESOLUTION
  Press left [<-] button --> decrease steering angle by STEERING_RESOLUTION
  Press button 4 --> toggle between forward and reverse mode


TODO: implement digital enable signals

 
*********************************************************************/

#include <bluefruit.h>

// OTA DFU service
BLEDfu bledfu;

// Uart over BLE service
BLEUart bleuart;

// Function prototypes for packetparser.cpp
uint8_t readPacket (BLEUart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printHex   (const uint8_t * data, const uint32_t numBytes);

// Packet buffer
extern uint8_t packetbuffer[];

#define STEERING_CONTROL_MAX 5000
#define STEERING_CONTROL_MIN 1000
#define STEERING_RESOLUTION 500

int steering_ctrl = STEERING_CONTROL_MAX - STEERING_CONTROL_MIN; //start with middle possition

#define THROTTLE_PIN 27
#define THROTTLE_RESOLUTION 5
int throttle_ctrl = 0;  //init to no throttle

int forwardMode = 1;

void setup(void)
{
  pinMode(THROTTLE_PIN, OUTPUT);
  
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println(F("Adafruit Bluefruit52 Controller App Example"));
  Serial.println(F("-------------------------------------------"));

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName("Bluefruit52");

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Configure and start the BLE Uart service
  bleuart.begin();

  // Set up and start advertising
  startAdv();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();  
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{
  // Wait for new data to arrive
  uint8_t len = readPacket(&bleuart, 500);
  if (len == 0) return;

  // Got a packet!
  // printHex(packetbuffer, len);

  // set steering angle
  tone(16, steering_ctrl); 

  // set throttle pwm duty cycle
  analogWrite(THROTTLE_PIN, throttle_ctrl);

  // Buttons
  if (packetbuffer[1] == 'B') {
    uint8_t buttnum = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';

    if (pressed) {
      switch(buttnum) {
        case 1 : //reset everything
        {
          throttle_ctrl = 0;
          steering_ctrl = STEERING_CONTROL_MAX - STEERING_CONTROL_MIN;
          
          break;
        }
        case 5 : //forward
        {
          Serial.println("forward");
          throttle_ctrl += THROTTLE_RESOLUTION;
          if (throttle_ctrl > 255)
          {
            throttle_ctrl = 255;
          }
          break;
        }
        case 6 : //reverse
        {
          Serial.println("reverse");
          throttle_ctrl -= THROTTLE_RESOLUTION;
          if (throttle_ctrl < 0)
          {
            throttle_ctrl = 0;
          }
          break;
        }
        case 7 : //left
        {
          Serial.println("left");
          steering_ctrl -= STEERING_RESOLUTION;
          if (steering_ctrl < STEERING_CONTROL_MIN)
          {
            steering_ctrl = STEERING_CONTROL_MIN;
          }
          break;
        }
        case 8 : //right
        {
          Serial.println("right");
          steering_ctrl += STEERING_RESOLUTION;
          if (steering_ctrl > STEERING_CONTROL_MAX)
          {
            steering_ctrl = STEERING_CONTROL_MAX;
          }
          break;
        }
        case 4: //forward reverse toggle
        {
          forwardMode = !forwardMode;
          break;
        }
      }
    }
  }
}

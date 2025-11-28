#include <Arduino.h>

#include <NodeStateAndID.h> // Custom Node state and ID management
#include <SerialCommunication.h> // Custom serial communication header
// my two cents
#include<HardwareSerial.h>

/*-----------Rhizome base stats----------------------*/
NodeStateAndID node(0); // Initialize Node with ID 1
/*---------------------------------------------------*/

//create les trois serial (use core-provided instances instead of constructing HardwareSerial)
HardwareSerial &uart0 = Serial;     // UART0 / USB debug serial
HardwareSerial &uart1 = Serial1;    // UART1
HardwareSerial &uart2 = Serial2;    // UART2

// add module-level pointers to the UARTs
static HardwareSerial *pUart0 = nullptr;
static HardwareSerial *pUart1 = nullptr;
static HardwareSerial *pUart2 = nullptr;

void setup() {
  //UART0 (on many boards Serial is UART0)
  uart0.begin(9600, SERIAL_8N1, 4, 16);   // RX-GPIO4, TX-GPIO16
  // UART1
  uart1.begin(9600, SERIAL_8N1, 13, 14);   // RX-GPIO13, TX-GPIO14
  // UART2
  uart2.begin(9600, SERIAL_8N1, 32, 33);     // RX-GPIO32, TX-GPIO33

  // start USB/debug serial so Serial.println() works
  Serial.begin(115200);
  Serial.println("Setup complete.");

  beginSerialCommunication(node, uart0, uart1, uart2);
}

void loop() {
  checkConnectionStatus();
  lookForMessages();
}

void beginSerialCommunication(NodeStateAndID &node, HardwareSerial &uart0, HardwareSerial &uart1, HardwareSerial &uart2) {
  pNode = &node;

  // store references so other functions in this module can use them
  pUart0 = &uart0;
  pUart1 = &uart1;
  pUart2 = &uart2;

  // If you want this module to handle Serial.begin for the UARTs, do it here.
  // Otherwise, since main.cpp already calls uartX.begin(...), skip begin() calls.
  // pUart2->begin(9600);

  pinMode(9, INPUT_PULLUP); // grounded when connected
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(9), connected, CHANGE); // detect connect/disconnect

  Serial.println("Serial Communication Initialized.");
}


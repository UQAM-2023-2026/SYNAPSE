#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

#define CONNECTION_TIMEOUT 500
#define CONNECT_DEBOUNCE 150
#define DISCONNECT_DEBOUNCE 600
#define NODE_SEND_INTERVAL 1000

// Two separate OSC instances for two UARTs
MicroOscSlip<32> oscNode1(&Serial2);
MicroOscSlip<32> oscNode2(&Serial1);

static NodeStateAndID *pNode = nullptr;

// Connection state machine
enum ConnectionState {
  CONN_DISCONNECTED,
  CONN_DEBOUNCING_CONNECT,
  CONN_CONNECTED,
  CONN_DEBOUNCING_DISCONNECT
};

// UART 1 state
struct UartState {
  int rhizomeID;
  int energyValue;
  ConnectionState connState;
  unsigned long debounceStart;
  bool isConnected;
  unsigned long lastNodeSent;
};

UartState uart1 = {0, 0, CONN_DISCONNECTED, 0, false, 0};
UartState uart2 = {0, 0, CONN_DISCONNECTED, 0, false, 0};

void onConnected(int uartNum) {
  Serial.print("🔌 UART");
  Serial.print(uartNum);
  Serial.print(" CONNECTED - Sending drainRate: ");
  
  float drainRate = pNode->getDrainRate();
  Serial.println(drainRate);
  
  if (uartNum == 1) {
    oscNode1.sendMessage("/node", "f", drainRate);
    uart1.lastNodeSent = millis();
    Serial.println("   ✓ Sent to UART1");
  } else {
    oscNode2.sendMessage("/node", "f", drainRate);
    uart2.lastNodeSent = millis();
    Serial.println("   ✓ Sent to UART2");
  }
}

void onDisconnected(int uartNum) {
  Serial.print("❌ UART");
  Serial.print(uartNum);
  Serial.println(" DISCONNECTED");
}

void updateConnectionState(int uartNum, UartState &state, int pinNum) {
  unsigned long now = millis();
  bool pinState = digitalRead(pinNum) == LOW;
  
  switch (state.connState) {
    case CONN_DISCONNECTED:
      if (pinState) {
        state.connState = CONN_DEBOUNCING_CONNECT;
        state.debounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!pinState) {
        state.connState = CONN_DISCONNECTED;
      } else if (now - state.debounceStart >= CONNECT_DEBOUNCE) {
        state.connState = CONN_CONNECTED;
        state.isConnected = true;
        onConnected(uartNum);
      }
      break;
      
    case CONN_CONNECTED:
      if (!pinState) {
        state.connState = CONN_DEBOUNCING_DISCONNECT;
        state.debounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      if (pinState) {
        state.connState = CONN_CONNECTED;
      } else if (now - state.debounceStart >= DISCONNECT_DEBOUNCE) {
        state.connState = CONN_DISCONNECTED;
        state.isConnected = false;
        onDisconnected(uartNum);
      }
      break;
  }
}

void handleNodeMessage(MicroOscMessage &msg, int uartNum, UartState &state) {
  if (msg.checkOscAddress("/discover_list")) {
    // Silently ignore discovery messages
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    state.rhizomeID = receivedId;
    state.energyValue = static_cast<int>(energy);
    
    // Print received data
    Serial.print("📨 UART");
    Serial.print(uartNum);
    Serial.print(" | ID: ");
    Serial.print(receivedId);
    Serial.print(" | Energy: ");
    Serial.print(energy);
    Serial.print("%");
    
    // Add warning if low
    if (energy <= 5) {
      Serial.print(" ⚠️ LOW!");
    }
    Serial.println();
  }
}

void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;

  // Initialize UART1 (Serial2)
  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
  pinMode(UART1_CONNECT_PIN, INPUT_PULLUP);

  // Initialize UART2 (Serial1)
  Serial1.begin(SERIAL_BAUD, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
  pinMode(UART2_CONNECT_PIN, INPUT_PULLUP);

  pNode->setDrainRate(5.0f);
 
  Serial.println("=================================");
  Serial.println("DUAL NODE READY");
  Serial.println("Initial Drain Rate: 5.0");
  Serial.println("Waiting for rhizomes...");
  Serial.println("=================================");
}

void lookForMessages(MicroOscMessage &msg) {
  // Legacy function kept for compatibility
}

float getRhizomeValue() {
  // Return 1.0 if either UART is connected
  return (uart1.isConnected || uart2.isConnected) ? 1.0f : 0.0f;
}

void loopSendToTouch() {
  // Send data from BOTH UARTs to TouchDesigner
  // Only print if there's actual data to send
  
  if (uart1.isConnected) {
    sendOSC(uart1.rhizomeID, uart1.energyValue);
  }
  
  if (uart2.isConnected) {
    sendOSC(uart2.rhizomeID, uart2.energyValue);
  }
  
  // If neither is connected, send zeros
  if (!uart1.isConnected && !uart2.isConnected) {
    sendOSC(0, 0);
  }
}

void SerialLoop() {
  unsigned long now = millis();
  
  // Update connection states for both UARTs
  updateConnectionState(1, uart1, UART1_CONNECT_PIN);
  updateConnectionState(2, uart2, UART2_CONNECT_PIN);
  
  // Listen for messages on UART1
  oscNode1.onOscMessageReceived([](MicroOscMessage &msg) {
    handleNodeMessage(msg, 1, uart1);
  });
  
  // Listen for messages on UART2
  oscNode2.onOscMessageReceived([](MicroOscMessage &msg) {
    handleNodeMessage(msg, 2, uart2);
  });
}

void sendDrainRateToAllUARTs(float drainRate) {
  Serial.print("📤 Sending DrainRate ");
  Serial.print(drainRate);
  Serial.print(" to: ");
  
  bool sentToAny = false;
  
  if (uart1.isConnected) {
    oscNode1.sendMessage("/node", "f", drainRate);
    uart1.lastNodeSent = millis();
    Serial.print("UART1 ");
    sentToAny = true;
  }
  
  if (uart2.isConnected) {
    oscNode2.sendMessage("/node", "f", drainRate);
    uart2.lastNodeSent = millis();
    Serial.print("UART2");
    sentToAny = true;
  }
  
  if (!sentToAny) {
    Serial.print("(none connected)");
  }
  
  Serial.println();
}

void checkConnectionStatus() {
  // Legacy function kept for compatibility
}
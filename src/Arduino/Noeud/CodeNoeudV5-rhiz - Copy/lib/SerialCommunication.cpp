#include "SerialCommunication.h"

// Initialize static instance
NodeSimulator* NodeSimulator::instance = nullptr;

NodeSimulator::NodeSimulator(HardwareSerial &serial) : oscNode(&serial) {
  instance = this;
}

void NodeSimulator::begin() {
  Serial.begin(115200);
  Serial2.begin(SERIAL_BAUD);
  
  Serial.println("=== NODE SIMULATOR STARTED ===");
  Serial.print("Drain rate: ");
  Serial.println(NODE_DRAIN_RATE);
  Serial.println("Waiting for rhizomes...");
}

void NodeSimulator::staticMessageHandler(MicroOscMessage &msg) {
  if (instance) {
    instance->handleNodeMessage(msg);
  }
}

void NodeSimulator::handleNodeMessage(MicroOscMessage &msg) {
  if (msg.checkOscAddress("/discover")) {
    int origin = msg.nextAsInt();
    int count = msg.nextAsInt();
    
    Serial.println("---");
    Serial.print("[RECV] /discover from rhizome ");
    Serial.print(origin);
    Serial.print(" (chain of ");
    Serial.print(count);
    Serial.println(" rhizomes)");
    
    // Respond with /node message
    // The rhizome chain will handle routing the drain to the last rhizome
    delay(50); // Small delay to ensure message is processed
    oscNode.sendMessage("/node", "f", NODE_DRAIN_RATE);
    
    Serial.print("[SEND] /node drainRate=");
    Serial.println(NODE_DRAIN_RATE);
    Serial.println("  → Chain updated, last rhizome will drain");
    Serial.println("---");
    
  } else if (msg.checkOscAddress("/energy")) {
    int rhizomeId = msg.nextAsInt();
    float energy = msg.nextAsFloat();
    
    Serial.print("[ENERGY] Rhizome ");
    Serial.print(rhizomeId);
    Serial.print(" reporting energy: ");
    Serial.println(energy);
    
    if (energy <= 5) {
      Serial.println("  ⚠️  WARNING: Rhizome nearly depleted!");
    }
  }
}

void NodeSimulator::update() {
  // Listen for messages from rhizomes
  oscNode.onOscMessageReceived(staticMessageHandler);
  delay(10);
}

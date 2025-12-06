#ifndef NODE_SIMULATOR_H
#define NODE_SIMULATOR_H

#include <Arduino.h>
#include <MicroOscSlip.h>

#define SERIAL_BAUD 9600
#define NODE_DRAIN_RATE 5.0f  // Énergie/seconde à drainer

class NodeSimulator {
public:
  NodeSimulator(HardwareSerial &serial);
  
  void begin();
  void update();

private:
  MicroOscSlip<32> oscNode;
  
  // Static instance pointer for callback
  static NodeSimulator* instance;
  
  void handleNodeMessage(MicroOscMessage &msg);
  static void staticMessageHandler(MicroOscMessage &msg);
};

#endif

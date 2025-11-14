#ifndef IR_COMMUNICATION_H
#define IR_COMMUNICATION_H

#pragma once
#include <Arduino.h>
#include <RhizomeStateAndID.h>


void SetupIR();

void receive_ir_data();

void send_ir_from_rhizome(const RhizomeStateAndID &r);

#endif
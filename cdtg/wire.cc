#include "wire.h"
#include <iostream>

// CREATORS
Wire::Wire(string name, wireType type, int cnfVar){
  d_name       = name;
  d_type       = type;
  d_cnfVar     = cnfVar;
  d_sourceGate = NULL;
}

// ACCESSORS
string Wire::getName(){
  return d_name;
}

Wire::wireType Wire::getType(){
  return d_type;
}

void Wire::printType(){
  switch(d_type){
    case PI:  cout << "  Type : \"input\"" << endl;
      break;
    case PO: cout << "  Type : \"output\"" << endl;
      break;
    case PPI:  cout << "  Type : \"pseudo-input\"" << endl;
      break;
    case PPO: cout << "  Type : \"pseudo-output\"" << endl;
      break;
    case NORMAL: cout << "  Type : \"normal\"" << endl;
      break;
    case CLK: cout << "  Type : \"clk\"" << endl;
      break;
    case RST: cout << "  Type : \"rst\"" << endl;
      break;
    default: cout << "Unknown type" << endl;
  }
}

int Wire::getCnfVar(){
  return d_cnfVar;
}

int Wire::fanoutSize(){
  return d_destGateList.size();
}

Gate* Wire::getSourceGate(){
  return d_sourceGate;
}

Gate* Wire::getDestGate(int i){
  return d_destGateList.at(i);
}

// MANIPULATORS
void Wire::setWireType(wireType type){
  d_type = type;
}

void Wire::setSourceGate(Gate* gate){
  d_sourceGate = gate;
}

void Wire::addDestGate(Gate* gate){
  d_destGateList.push_back(gate);
}

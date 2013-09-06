#include "gate.h"
#include <iostream>
#include <assert.h>

// CREATORS
Gate::Gate(gateType type, string utility, int id){
  d_type          = type;
  d_utilityString = utility;
  d_id            = id;
  //d_hasClkOrRst   = false;
}

Gate::Gate(gateType type, int id){
  d_type        = type;
  d_id          = id;
  //d_hasClkOrRst = false;
}

// ACCESSORS
string Gate::getUtilityStr(){
  return d_utilityString;
}

int Gate::getId(){
  return d_id;
}

// bool Gate::getHasClkOrRst(){
//   return d_hasClkOrRst;
// }

Gate::gateType Gate::getType(){
  return d_type;
}

void Gate::printType(){
  switch(d_type){
    case BUF:     cout << " BUF ";     break;
    case INV:     cout << " INV ";     break;
    case AND2:    cout << " AND2 ";    break;
    case AND3:    cout << " AND3 ";    break;
    case OR2:     cout << " OR2 ";     break;
    case OR3:     cout << " OR3 ";     break;
    case NAND2:   cout << " NAND2 ";   break;
    case NAND3:   cout << " NAND3 ";   break;
    case NAND4:   cout << " NAND4 ";   break;
    case NOR2:    cout << " NOR2 ";    break;
    case NOR3:    cout << " NOR3 ";    break;
    case XOR2:    cout << " XOR2 ";    break;
    case XNOR2:   cout << " XNOR2 ";   break;
    case MX2:     cout << " MX2 ";     break;
    case DFF:     cout << " DFF ";     break;
    case INPUT:   cout << " INPUT ";   break;
    case OUTPUT:  cout << " OUTPUT ";  break;
    case CONST0:  cout << " CONST0 ";  break;
    case CONST1:  cout << " CONST1 ";  break;
    case UNKNOWN: cout << " UNKNOWN "; break;
    case AOI21:   cout << " AOI21 ";   break;
    case OAI21:   cout << " OAI21 ";   break;
    //default:      cout << "Unknown type: " << d_utilityString;
  }
}

int Gate::outputSize(){
  return d_outputWireList.size();
}

int Gate::inputSize(){
  return d_inputWireList.size();
}

int Gate::dffOutSize(){
  return d_dffOutputPortList.size();
}

Wire* Gate::getOutWire(int i){
  return d_outputWireList.at(i);
}

char Gate::getDFFPort(int i){
  if(d_dffOutputPortList.empty()){
    cout << "NO DFF port at " << d_utilityString << endl;
    exit(1);
  }
  else if((i+1) > d_dffOutputPortList.size()){
    cout << "NO DFF port d_dffOutputPortList[" << i << "] at "<< d_utilityString << endl;
    exit(1);
  }
  return d_dffOutputPortList.at(i);
}

Wire* Gate::getInWire(int i){
  return d_inputWireList.at(i);
}

vector<int>& Gate::getCNFClause(){
  return d_cnfClause;
}

void Gate::printCNFClause(){
  if(d_cnfClause.size() == 0)
    cout << "There is no clause in gate " << d_id << " : " << d_type << endl;
  else{
    for(int i=0;i<d_cnfClause.size();++i){
      if(d_cnfClause.at(i) == 0)
        cout << d_cnfClause.at(i) << endl;
      else
        cout << d_cnfClause.at(i) << " ";
    }
  }
}

// MANIPULATORS
// void Gate::setHasClkOrRst(bool b){
//   d_hasClkOrRst = b;
// }

void Gate::addOutWire(Wire* wire){
  d_outputWireList.push_back(wire);
}

void Gate::addInWire(Wire* wire){
  d_inputWireList.push_back(wire);
}

void Gate::addDFFOutPort (char port){
  d_dffOutputPortList.push_back(port);
  assert(d_dffOutputPortList.size() == d_outputWireList.size());
}

void Gate::addCNFClause(int v){
  d_cnfClause.push_back(v);
  d_cnfClause.push_back(0);
}

void Gate::addCNFClause(int v1, int v2){
  d_cnfClause.push_back(v1);
  d_cnfClause.push_back(v2);
  d_cnfClause.push_back(0);
}

void Gate::addCNFClause(int v1, int v2, int v3){
  d_cnfClause.push_back(v1);
  d_cnfClause.push_back(v2);
  d_cnfClause.push_back(v3);
  d_cnfClause.push_back(0);
}

void Gate::addCNFClause(int v1, int v2, int v3, int v4){
  d_cnfClause.push_back(v1);
  d_cnfClause.push_back(v2);
  d_cnfClause.push_back(v3);
  d_cnfClause.push_back(v4);
  d_cnfClause.push_back(0);
}

void Gate::addCNFClause(int v1, int v2, int v3, int v4, int v5){
  d_cnfClause.push_back(v1);
  d_cnfClause.push_back(v2);
  d_cnfClause.push_back(v3);
  d_cnfClause.push_back(v4);
  d_cnfClause.push_back(v5);
  d_cnfClause.push_back(0);
}

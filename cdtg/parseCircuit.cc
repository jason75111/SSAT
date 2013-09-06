#include "parseCircuit.h"
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <assert.h>

// CREATORS
ParseCircuit::ParseCircuit(Circuit* circuit_p, char* circuitFile, int* cnfVar_p){
    d_circuit_p = circuit_p;
    d_circuitFile.open(circuitFile, ifstream::in);
    d_cnfVar_p = cnfVar_p;
    if(!d_circuitFile){
        cout << "File could not be opened" << endl;
        exit(1);
    }
    if(d_gateTypeMap.empty()){
        initializeGateTypeMap();
    }
}

// MANIPULATORS
// MAIN PARSING FUNCTION
void ParseCircuit::parse(){
    char c_str[Global::STR_LENGTH];
    string str, nextstr, substr;
    
    while(d_circuitFile.getline(c_str, Global::STR_LENGTH)){
        str = string(c_str);
        Global::eraseBlanks(&str);
        if(str.length()==0) continue;
        substr = str.substr(0, 4);
        if(substr.compare("inpu")==0 || substr.compare("outp")==0 || substr.compare("wire")==0){
            parseWires(str);
        }
        else if(substr.compare("modu")!=0 && substr.compare("endm")!=0){// Not module neither endmodule
            // Formalize strings of gate cells so every string ends with a ';'
            formalizeGateLine(&str);
            parseGates(str);
        }
    }
  
    // Create IO pseudo-gates
    // cout << "Creating io connection..." << endl;
    createInoutGates();
    
    // Create connection between wires and gates
    // cout << "Creating gate conneciton..." << endl;
    gateConnection();
    
    // Classify wires
    // cout << "Classifying wires..." << endl;
    d_circuit_p->classifyWire();
    
    // Makesure at least one clk and rst input
    //assert(d_circuit_p->CLKListSize() > 0);
    //assert(d_circuit_p->RSTListSize() > 0);
    
    // Create CNF clauses
    // cout << "Generating CNF of circuit..." << endl;
    d_circuit_p->setCNF();
    
    //cout << "End ParseCircuit::parse"  << endl;
}
  
// UTILITIES
void ParseCircuit::initializeGateTypeMap(){
    ////////////////////// BUF /////////////////////////
    d_gateTypeMap[(string)"BUF"]     = Gate::BUF;
    d_gateTypeMap[(string)"CLKBUF"]  = Gate::BUF;
    ///////////////////////////////////////////////////
    d_gateTypeMap[(string)"INV"]     = Gate::INV;
    d_gateTypeMap[(string)"AND2"]    = Gate::AND2;
    d_gateTypeMap[(string)"AND3"]    = Gate::AND3;
    d_gateTypeMap[(string)"OR2"]     = Gate::OR2;
    d_gateTypeMap[(string)"OR3"]     = Gate::OR3;
    d_gateTypeMap[(string)"NAND2"]   = Gate::NAND2;
    d_gateTypeMap[(string)"NAND3"]   = Gate::NAND3;
    d_gateTypeMap[(string)"NAND4"]   = Gate::NAND4;
    d_gateTypeMap[(string)"NOR2"]    = Gate::NOR2;
    d_gateTypeMap[(string)"NOR3"]    = Gate::NOR3;
    d_gateTypeMap[(string)"XOR2"]    = Gate::XOR2;
    d_gateTypeMap[(string)"XNOR2"]   = Gate::XNOR2;
    d_gateTypeMap[(string)"MX2"]     = Gate::MX2;
    ////////////////////// DFF /////////////////////////
    d_gateTypeMap[(string)"DFFRHQ"]  = Gate::DFF;
    d_gateTypeMap[(string)"DFFHQ"]   = Gate::DFF;
    d_gateTypeMap[(string)"DFFTR"]   = Gate::DFF;
    d_gateTypeMap[(string)"DFFSR"]   = Gate::DFF;
    d_gateTypeMap[(string)"DFFS"]    = Gate::DFF;
    d_gateTypeMap[(string)"DFFR"]    = Gate::DFF;
    d_gateTypeMap[(string)"DFF"]     = Gate::DFF;
    ///////////////////////////////////////////////////
    d_gateTypeMap[(string)"INPUT"]   = Gate::INPUT;
    d_gateTypeMap[(string)"OUTPUT"]  = Gate::OUTPUT;
    d_gateTypeMap[(string)"CONST0"]  = Gate::CONST0;
    d_gateTypeMap[(string)"CONST1"]  = Gate::CONST1;
    d_gateTypeMap[(string)"UNKNOWN"] = Gate::UNKNOWN;
    ////////// Add new gate type if need ////////////
    d_gateTypeMap[(string)"OAI21"]   = Gate::OAI21;
    d_gateTypeMap[(string)"AOI21"]   = Gate::AOI21;
}

void ParseCircuit::formalizeGateLine(string* str_p){
    char c_str[Global::STR_LENGTH];
    string nextstr;
    
    while(str_p->at(str_p->length() - 1) != ';'){
        d_circuitFile.getline(c_str, Global::STR_LENGTH);
        nextstr = string(c_str);
        Global::eraseBlanks(&nextstr);
        (*str_p) += nextstr;
        assert(str_p->length() < str_p->max_size());
    }  
}

void ParseCircuit::parseWires(string str){
    char    c_str[Global::STR_LENGTH];
    string  substr, intstr, tempstr;
    size_t  tail = 0, mid = 0, high = 0, low = 0;
    int     h, l;
    bool    isInput = false, passEnd = false;
    Wire::wireType type;
      
    // Recognize the type of wire
    substr = str.substr(0, 4);
    if(substr.compare("inpu")==0){
        type = Wire::PI;
        str.erase(0, 5);// Erase "input"
        isInput = true;
    }
    else if(substr.compare("outp")==0){
        type = Wire::PO;
        str.erase(0, 6);// Erase "output"
    }
    else{
        type = Wire::NORMAL;
        str.erase(0, 4);// Erase "wire"
    }
    
    passEnd = false;
    while(!passEnd){
        while(str.length() != 0){
            if((tail = str.find_first_of(',')) == string::npos){
                tail = str.length() - 1;
                passEnd = true;
                assert(str.at(tail) == ';');
            }
            substr = str.substr(0, tail);
            // Handle [H:L], add a vector of wires
            if(substr.at(0)=='['){
                mid    = substr.find_first_of(']');
                high   = 1;
                low    = substr.find_first_of(':');
                intstr = substr.substr(high, low-high+1);
                h      = atoi(intstr.c_str());
                high   = low+1;
                low    = mid-1;
                intstr = substr.substr(high, low-high+1);
                l      = atoi(intstr.c_str());
          
                substr.erase(0, mid+1);
                for(int i = l; i <= h; ++i){
                    tempstr =  substr;
                    intstr  =  Global::int2str(i);
                    tempstr += '[';
                    tempstr += intstr;
                    tempstr += ']';
                    Wire* wire_p = new Wire(tempstr, type, ++(*d_cnfVar_p));
                    d_circuit_p->addWire(wire_p);
                }
            }
            else{ // Add a single wire
//              cout << "Add new wire: \"" << substr << endl;
              if(isInput){
                  size_t find;
                  if((find = substr.find("clk")) != string::npos || (find = substr.find("CLK")) != string::npos ||
                     (find = substr.find("clock")) != string::npos || (find = substr.find("CLOCK")) != string::npos)
                    type = Wire::CLK;
                  else if((find = substr.find("rst")) != string::npos   || (find = substr.find("RST")) != string::npos   ||
                          (find = substr.find("reset")) != string::npos || (find = substr.find("RESET")) != string::npos)
                    type = Wire::RST;
                  else
                    type = Wire::PI;
              }
              Wire* wire_p = new Wire(substr, type, ++(*d_cnfVar_p));
              d_circuit_p->addWire(wire_p);
            }
            str.erase(0, tail+1);
      }
      
      if(!passEnd){
          d_circuitFile.getline(c_str, Global::STR_LENGTH);
          str = string(c_str);
          Global::eraseBlanks(&str);
      }
    }
}

void ParseCircuit::parseGates(string str){
  Gate::gateType type;
  static int gateId = 1;

  type = getGateType(str);
  // Store the string for further handle
  Gate* gate_p = new Gate(type, str, gateId);
  d_circuit_p->addGate(gate_p);
  gateId++;
}

void ParseCircuit::createInoutGates(){
  int gateId = d_circuit_p->gateListSize() + 1;
  
  for(int i=0;i<d_circuit_p->PIListSize();++i){
    Gate* gate_p = new Gate(Gate::INPUT, gateId++);
    gate_p->addOutWire(d_circuit_p->getPI(i));
    d_circuit_p->addGate(gate_p);
  }
  for(int i=0;i<d_circuit_p->POListSize();++i){
    Gate* gate_p = new Gate(Gate::OUTPUT, gateId++);
    gate_p->addInWire(d_circuit_p->getPO(i));
    d_circuit_p->addGate(gate_p);
  }  
  for(int i=0;i<d_circuit_p->CLKListSize();++i){
    Gate* gate_p = new Gate(Gate::INPUT, gateId++);
    //gate_p->setHasClkOrRst(true);
    gate_p->addOutWire(d_circuit_p->getCLK(i));
    d_circuit_p->addGate(gate_p);
  }
  for(int i=0;i<d_circuit_p->RSTListSize();++i){
    Gate* gate_p = new Gate(Gate::INPUT, gateId++);
    //gate_p->setHasClkOrRst(true);
    gate_p->addOutWire(d_circuit_p->getRST(i));
    d_circuit_p->addGate(gate_p);
  }
}

void ParseCircuit::gateConnection(){
  string         str, wireName;
  char           portChar;
  size_t         found;
  bool           output;               // used to identify output port
  Gate::gateType gtype;
  vector<Wire>::iterator wit;
    
  for(int i = 0; i < d_circuit_p->gateListSize(); ++i){
    gtype = d_circuit_p->getGate(i)->getType();
// cout << "Utility: " << d_circuit_p->getGate(i)->getUtilityStr() << endl;
    if(gtype == Gate::CONST0 || gtype == Gate::CONST1){
      str = d_circuit_p->getGate(i)->getUtilityStr();
      // We assume "assign wire_out = constant"
      // Identify output wire
      str.erase(0, 6); // Erase "assign" in the front
      found = str.find_first_of('=');
      wireName = str.substr(0, found);
      // Connect output wire to gate
      for(int j = 0; j < d_circuit_p->wireListSize(); ++j){
        if(wireName.compare(d_circuit_p->getWire(j)->getName()) == 0){
          d_circuit_p->getGate(i)->addOutWire(d_circuit_p->getWire(j));                                                                            
          d_circuit_p->getWire(j)->setSourceGate(d_circuit_p->getGate(i));
          break;
        }
        if(j == (d_circuit_p->wireListSize() - 1)){
          cout << __FILE__ << "::" << __LINE__ <<  ": Cannot find wire: " << wireName << endl;
          exit(1);
        }
      }
    }
    else if(gtype == Gate::BUF){
      bool isAssign = false;
      str = d_circuit_p->getGate(i)->getUtilityStr();
      // Identify output wire
      if(str.substr(0, 6).compare("assign") == 0){
        // Here buffer means a pseudo buffer for "assign wire_out = wire_in"
        isAssign = true;
        str.erase(0, 6); // Erase "assign" in the front
        found = str.find_first_of('=');
        wireName = str.substr(0, found);
      }
      else{
        found = str.find_first_of('.');
        str.erase(0, found);
        wireName = getWireName(d_circuit_p->getGate(i)->getType(), &str, &portChar, &output);
      }
      // Connect output wire to gate
      for(int j = 0; j < d_circuit_p->wireListSize(); ++j){
        if(wireName.compare(d_circuit_p->getWire(j)->getName()) == 0){
          d_circuit_p->getGate(i)->addOutWire(d_circuit_p->getWire(j));                                                                            
          d_circuit_p->getWire(j)->setSourceGate(d_circuit_p->getGate(i));
          break;
        }
        if(j == (d_circuit_p->wireListSize() - 1)){
          cout << __FILE__ << "::" << __LINE__ <<  ": Cannot find wire: " << wireName << endl;
          exit(1);
        }
      }
      // Identify input wire
      if(isAssign){
        str.erase(0, found + 1);
        str.erase(str.length() - 1, 1);
        wireName = str;
      }
      else{
        found = str.find_first_of('.');
        str.erase(0, found);
        wireName = getWireName(d_circuit_p->getGate(i)->getType(), &str, &portChar, &output);
      }
      // Connect input wire to gate
      for(int j = 0; j < d_circuit_p->wireListSize(); ++j){
        if(wireName.compare(d_circuit_p->getWire(j)->getName()) == 0){
          d_circuit_p->getGate(i)->addInWire(d_circuit_p->getWire(j));                                                                          
          d_circuit_p->getWire(j)->addDestGate(d_circuit_p->getGate(i));
          break;
        }
        if(j == (d_circuit_p->wireListSize() - 1)){
          cout << __FILE__ << "::" << __LINE__ <<  ": Cannot find wire: " << wireName << endl;
          exit(1);
        }
      }
    }
    else{
      str = d_circuit_p->getGate(i)->getUtilityStr();
      found = str.find_first_of('.');
      str.erase(0, found);
      
      // Set wire connections
      while(str.length() != 0){
        output = false;
        wireName = getWireName(d_circuit_p->getGate(i)->getType(), &str, &portChar, &output);
        //assert(!wireName.empty());
        if(wireName.empty()){
            cout << "Warning !!! Empty port name in gate: " << d_circuit_p->getGate(i)->getUtilityStr() << endl;
            continue;
        }
        else if(wireName.compare("1'b0") == 0 || wireName.compare("1'b1") == 0){
            cout << "Warning !!! Constant port name in gate: " << d_circuit_p->getGate(i)->getUtilityStr() << endl;
            continue;
        }
        
        for(int j = 0; j < d_circuit_p->wireListSize(); ++j){
          if(wireName.compare(d_circuit_p->getWire(j)->getName()) == 0){
            if(output){
              d_circuit_p->getGate(i)->addOutWire(d_circuit_p->getWire(j));  
              if(portChar == 'Q' || portChar == 'N')
                d_circuit_p->getGate(i)->addDFFOutPort(portChar);
              d_circuit_p->getWire(j)->setSourceGate(d_circuit_p->getGate(i));
            }
            else{
              d_circuit_p->getGate(i)->addInWire(d_circuit_p->getWire(j));
              d_circuit_p->getWire(j)->addDestGate(d_circuit_p->getGate(i));
            }
            
            // Identify clk and rst 
            // if(d_circuit_p->getWire(j)->getType() == Wire::CLK || d_circuit_p->getWire(j)->getType() == Wire::RST){
            //   d_circuit_p->getGate(i)->setHasClkOrRst(true);
            // }
            break;
          }
          if(j == d_circuit_p->wireListSize() - 1){
            cout << __FILE__ << "::" << __LINE__ <<  ": Cannot find wire: " << wireName << endl;
            exit(1);
          }
        }//end for
      }//end while
      
      // Change wire type of PPI and PPO
      if(d_circuit_p->getGate(i)->getType() == Gate::DFF){
        for(int j = 0; j < d_circuit_p->getGate(i)->outputSize(); ++j)
          d_circuit_p->getGate(i)->getOutWire(j)->setWireType(Wire::PPI);
        if(d_circuit_p->getGate(i)->getInWire(0)->getType() != Wire::PI)
          d_circuit_p->getGate(i)->getInWire(0)->setWireType(Wire::PPO);
      }
    }
  }
}

Gate::gateType ParseCircuit::getGateType(string str){
    Gate::gateType type = Gate::UNKNOWN;
    map<string, Gate::gateType>::iterator it;
    size_t found;
  
    //cout << "Original string =\"" << str << "\"" << endl;
    if(str.substr(0, 6).compare("assign") == 0){
        if(str.find("'b0") != string::npos) // N'b0
            type = Gate::CONST0;
        else if(str.find("'b1") != string::npos) // N'b1
            type = Gate::CONST1;
        else
          type = Gate::BUF;
    }
    else if((found = str.find_last_of('X')) != string::npos){
        str.erase(found, str.length() - found);
        if((it = d_gateTypeMap.find(str)) != d_gateTypeMap.end()){
            //cout << "Add gate of type:" << str << endl;
            type = d_gateTypeMap[str];
        }
        else{ // Keep searching
            while((it = d_gateTypeMap.find(str)) == d_gateTypeMap.end()){
                found = str.find_last_of('X');
                if(found == string::npos){ // Cannot find anymore 'X', no such gate
                    cout << "Cannot find gate type: " << str << endl;
                    break;
                }
                else{
                    str.erase(found, str.length() - found);
                    //cout << "Identify gate type from string:\"" << str << "\"" << endl;
                }
            }
            //cout << "Add gate of type:" << str << endl;
            type = d_gateTypeMap[str];
        }
    }

    if(type == Gate::UNKNOWN)
        cout << __FILE__ << ":" << __LINE__ << ": Unknown gate type from spec" << str << endl;
      
    return type;
}

string ParseCircuit::getWireName(Gate::gateType gType, string* str_p, char* c_p, bool* output_p){
    string wireName, portName;
    size_t point, head, tail;
    
    (*c_p) = 'Y';
    head = str_p->find_first_of('(');
    if(head != string::npos){
        tail = str_p->find_first_of(')');
        wireName = str_p->substr(head + 1, tail - head - 1);
        
        point = str_p->find_first_of('.');
        portName = str_p->substr(point + 1, head - point - 1);
        
        str_p->erase(0, tail + 1);
    }
    
    // Check .Q and .QN
    if(gType == Gate::DFF){
        if(portName.compare("Q") == 0){
          (*c_p) = 'Q';
          (*output_p) = true;
          //cout << "Wire: " << wireName << ", port = Q" << endl;
        }
        else if(portName.compare("QN") == 0){
          (*c_p) = 'N';
          (*output_p) = true;
          //cout << "Wire: " << wireName << ", port = QN" << endl;
        }
    }
    
    // Check remain in/out
    head = str_p->find_first_of('(');
    if(head == string::npos){
        str_p->clear();
        (*output_p) = true;
    }
    
    return wireName;
}


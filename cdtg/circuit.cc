#include "circuit.h"
#include <iostream>
#include <stdlib.h>

// CREATORS
Circuit::Circuit(){}

Circuit::~Circuit(){
    for(int i=0;i<d_gateList.size();++i)
        delete d_gateList.at(i);
    for(int i=0;i<d_wireList.size();++i)
        delete d_wireList.at(i);
}

// MANIPULATORS
void Circuit::addGate(Gate* gate_p){
    d_gateList.push_back(gate_p);
  
    if(gate_p->getType() == Gate::DFF)
        d_dffList.push_back(gate_p);
}

void Circuit::addWire(Wire* wire_p){
    d_wireList.push_back(wire_p);
}

void Circuit::classifyWire(){
    // Add special type of wires to specific list
    for(int i = 0; i < d_wireList.size(); ++i){
        switch(d_wireList.at(i)->getType()){
            case Wire::PI:
                d_piList.push_back(d_wireList.at(i));
                d_str2PI[d_wireList.at(i)->getName()] = d_wireList.at(i);
                break;
            case Wire::PO:
                d_poList.push_back(d_wireList.at(i));
                break;
            case Wire::PPI:
                d_ppiList.push_back(d_wireList.at(i));
                break;
            case Wire::PPO:
                d_ppoList.push_back(d_wireList.at(i));
                d_str2PPO[d_wireList.at(i)->getName()] = d_wireList.at(i);
                break;
            case Wire::CLK:
                d_clkList.push_back(d_wireList.at(i));
                break;
            case Wire::RST:
                d_rstList.push_back(d_wireList.at(i));
                break;
            case Wire::NORMAL:
                break;
      }
    }
}

void Circuit::setName(string name){
    d_name = name; 
}

// ACCESSORS
string Circuit::getName(){
    return d_name;
}

Gate* Circuit::getGate(int idx){
    return d_gateList.at(idx);
}

Gate* Circuit::getDFF(int idx){
    return d_dffList.at(idx);
}

Wire* Circuit::getWire(int idx){
    return d_wireList.at(idx);
}

Wire* Circuit::getPI(int idx){
    return d_piList.at(idx);
}

Wire* Circuit::getPI(string name){
    return d_str2PI.find(name)->second;
}

Wire* Circuit::getPO(int idx){
    return d_poList.at(idx);
}

Wire* Circuit::getPPI(int idx){
    return d_ppiList.at(idx);
}

Wire* Circuit::getPPO(int idx){
    return d_ppoList.at(idx);
}

Wire* Circuit::getPPO(string name){
    return d_str2PPO.find(name)->second;
}

Wire* Circuit::getCLK(int idx){
    return d_clkList.at(idx);
}

Wire* Circuit::getRST(int idx){
    return d_rstList.at(idx);
}

int Circuit::gateListSize(){
    return d_gateList.size();
}

int Circuit::dffListSize(){
    return d_dffList.size();
}

int Circuit::wireListSize(){
    return d_wireList.size();
}

int Circuit::PIListSize(){
    return d_piList.size();
}

int Circuit::POListSize(){
    return d_poList.size();
}

int Circuit::PPIListSize(){
    return d_ppiList.size();
}

int Circuit::PPOListSize(){
    return d_ppoList.size();
}

int Circuit::CLKListSize(){
    return d_clkList.size();
}

int Circuit::RSTListSize(){
    return d_rstList.size();
}

// Print functions
void Circuit::printWireInfo(){
  cout << "\n================ Wire Information ================\n" << endl;
  for(int i=0;i<d_wireList.size();++i){
    cout << "Wire: " << d_wireList.at(i)->getName() << endl;
    d_wireList.at(i)->printType();
    //cout << "  CNF variable number = " << d_wireList.at(i)->getCnfVar() << endl;
    if(d_wireList.at(i)->getSourceGate() != NULL){
      cout << "  Source gate = "; 
      d_wireList.at(i)->getSourceGate()->printType();
      cout << endl;
    }
    else
      cout << "  Source gate = NULL" << endl;
    if(d_wireList.at(i)->fanoutSize()!= 0)
      for(int j=0;j<d_wireList.at(i)->fanoutSize();++j){
        cout << "  Destination gate = ";
        d_wireList.at(i)->getDestGate(j)->printType(); 
        cout << endl;
      }
    else
      cout << "  Destination gate = NULL" << endl;
      
    //if(d_wireList.at(i)->getType()==Wire::PI){
      //cout << "Wire: " << d_wireList.at(i)->getName() << endl;
      //d_wireList.at(i)->printType();
      cout << "  CNF variable number = " << d_wireList.at(i)->getCnfVar() << endl;
    //}
   /* 
    // Identify count register
    if(d_wireList.at(i)->getType()==Wire::PPI){
      string name = d_wireList.at(i)->getName();
      if(name.substr(0,5).compare("count")==0){
        cout << "Wire: " << d_wireList.at(i)->getName() << endl;
        d_wireList.at(i)->printType();
        cout << "  CNF variable number = " << d_wireList.at(i)->getCnfVar() << endl;
      }
    }*/
    
  }// End for
}

void Circuit::printGateInfo(){
  cout << "\n================ Gate Information ================\n" << endl;
  for(int i = 0; i < d_gateList.size(); ++i){
    cout << "Gate: "; 
    d_gateList.at(i)->printType(); 
    cout << endl;
//    cout << "Original string: " << d_gateList.at(i)->getUtilityStr() << endl;
      for(int j = 0; j < d_gateList.at(i)->outputSize(); ++j){
        cout << "  Output wire = " << d_gateList.at(i)->getOutWire(j)->getName();
        if(d_gateList.at(i)->getType() == Gate::DFF){
        	cout << ", port type = ";
        	if(d_gateList.at(i)->getDFFPort(j) == 'Q')
        		cout << "Q";
        	else if(d_gateList.at(i)->getDFFPort(j) == 'N')
        		cout << "QN";
        }
        cout << endl;
      }
      for(int j = 0; j < d_gateList.at(i)->inputSize(); ++j)
        cout << "  Input wire = "  << d_gateList.at(i)->getInWire(j)->getName() << endl;
  }
}

// For CNF initialization
void Circuit::setCNF(){
  Gate::gateType type;
  Gate* gate_p;
  for(int i = 0; i < d_gateList.size(); ++i){
    gate_p = d_gateList.at(i);
    //cout << "Generating CNF of gate: " << gate_p->getUtilityStr() << endl;
    type = gate_p->getType();
    switch(type){
    	case Gate::BUF:    genBUFCNF(gate_p);    break;
    	case Gate::INV:    genINVCNF(gate_p);    break;
    	case Gate::AND2:   genAND2CNF(gate_p);   break;
    	case Gate::AND3:   genAND3CNF(gate_p);   break;
    	case Gate::OR2:    genOR2CNF(gate_p);    break;
    	case Gate::OR3:    genOR3CNF(gate_p);    break;
    	case Gate::NAND2:  genNAND2CNF(gate_p);  break;
    	case Gate::NAND3:  genNAND3CNF(gate_p);  break;
    	case Gate::NAND4:  genNAND4CNF(gate_p);  break;
    	case Gate::NOR2:   genNOR2CNF(gate_p);   break;
    	case Gate::NOR3:   genNOR3CNF(gate_p);   break;
    	case Gate::XOR2:   genXOR2CNF(gate_p);   break;
    	case Gate::XNOR2:  genXNOR2CNF(gate_p);  break;
    	case Gate::MX2:    genMX2CNF(gate_p);    break;
    	case Gate::OAI21:  genOAI21CNF(gate_p);  break;
    	case Gate::AOI21:  genAOI21CNF(gate_p);  break;
    	case Gate::CONST0: genCONST0CNF(gate_p); break;
    	case Gate::CONST1: genCONST1CNF(gate_p); break;
    	case Gate::DFF:                          break;
    	case Gate::INPUT:                        break;
    	case Gate::OUTPUT:                       break;
    	default: cerr << "Gate type is not in the CNF library" << endl; 
    		       cerr << "Spec is: ";  gate_p->printType(); cerr << endl;
    		       exit(1);
    }
  }
}

void Circuit::genBUFCNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(),  Y->getCnfVar());
}

void Circuit::genINVCNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause(-A->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( A->getCnfVar(),  Y->getCnfVar());
}

void Circuit::genAND2CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( B->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(), -B->getCnfVar(), Y->getCnfVar());
}

void Circuit::genAND3CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* C = gate_p->getInWire(2);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( B->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( C->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(), -B->getCnfVar(), -C->getCnfVar(),
                        Y->getCnfVar());
}

void Circuit::genOR2CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause(-A->getCnfVar(), Y->getCnfVar());
  gate_p->addCNFClause(-B->getCnfVar(), Y->getCnfVar());
  gate_p->addCNFClause( A->getCnfVar(), B->getCnfVar(), -Y->getCnfVar());
}

void Circuit::genOR3CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* C = gate_p->getInWire(2);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause(-A->getCnfVar(), Y->getCnfVar());
  gate_p->addCNFClause(-B->getCnfVar(), Y->getCnfVar());
  gate_p->addCNFClause(-C->getCnfVar(), Y->getCnfVar());
  gate_p->addCNFClause( A->getCnfVar(), B->getCnfVar(), C->getCnfVar(),
                       -Y->getCnfVar());
}

void Circuit::genNAND2CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause( B->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(), -B->getCnfVar(), -Y->getCnfVar());
}

void Circuit::genNAND3CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* C = gate_p->getInWire(2);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause( B->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause( C->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(), -B->getCnfVar(), -C->getCnfVar(), 
                       -Y->getCnfVar());
}

void Circuit::genNAND4CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* C = gate_p->getInWire(2);
  Wire* D = gate_p->getInWire(3);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause( B->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause( C->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause( D->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(), -B->getCnfVar(), -C->getCnfVar(), 
                       -D->getCnfVar(), -Y->getCnfVar());
}

void Circuit::genNOR2CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause(-A->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-B->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( A->getCnfVar(),  B->getCnfVar(), Y->getCnfVar());
}

void Circuit::genNOR3CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* C = gate_p->getInWire(2);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause(-A->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-B->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-C->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( A->getCnfVar(),  B->getCnfVar(), C->getCnfVar(), 
                        Y->getCnfVar());
}

void Circuit::genXOR2CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(),  B->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( A->getCnfVar(), -B->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(),  B->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(), -B->getCnfVar(), -Y->getCnfVar()); 
}

void Circuit::genXNOR2CNF(Gate* gate_p){
  Wire* A = gate_p->getInWire(0);
  Wire* B = gate_p->getInWire(1);
  Wire* Y = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(),  B->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause( A->getCnfVar(), -B->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(),  B->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(), -B->getCnfVar(),  Y->getCnfVar());
}

void Circuit::genMX2CNF(Gate* gate_p){
  Wire* A  = gate_p->getInWire(0);
  Wire* B  = gate_p->getInWire(1);
  Wire* S0 = gate_p->getInWire(2);
  Wire* Y  = gate_p->getOutWire(0);
  gate_p->addCNFClause( A->getCnfVar(),  S0->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause( B->getCnfVar(), -S0->getCnfVar(), -Y->getCnfVar());
  gate_p->addCNFClause(-B->getCnfVar(), -S0->getCnfVar(),  Y->getCnfVar());
  gate_p->addCNFClause(-A->getCnfVar(),  S0->getCnfVar(),  Y->getCnfVar());
}

void Circuit::genOAI21CNF(Gate* gate_p){
  Wire* A0 = gate_p->getInWire(0);
  Wire* A1 = gate_p->getInWire(1);
  Wire* B0 = gate_p->getInWire(2);
  Wire* Y  = gate_p->getOutWire(0);
  gate_p->addCNFClause( A0->getCnfVar(),  A1->getCnfVar(),   Y->getCnfVar());
  gate_p->addCNFClause(-A1->getCnfVar(), -B0->getCnfVar(),  -Y->getCnfVar());
  gate_p->addCNFClause(-A0->getCnfVar(), -B0->getCnfVar(),  -Y->getCnfVar());
  gate_p->addCNFClause( B0->getCnfVar(),   Y->getCnfVar());
}

void Circuit::genAOI21CNF(Gate* gate_p){
  Wire* A0 = gate_p->getInWire(0);
  Wire* A1 = gate_p->getInWire(1);
  Wire* B0 = gate_p->getInWire(2);
  Wire* Y  = gate_p->getOutWire(0);
  gate_p->addCNFClause( A0->getCnfVar(),  B0->getCnfVar(),   Y->getCnfVar());
  gate_p->addCNFClause( A1->getCnfVar(),  B0->getCnfVar(),   Y->getCnfVar());
  gate_p->addCNFClause(-A0->getCnfVar(), -B0->getCnfVar(),  -Y->getCnfVar());  
  gate_p->addCNFClause(-B0->getCnfVar(),  -Y->getCnfVar());
}

void Circuit::genCONST0CNF(Gate* gate_p){
	Wire* Y  = gate_p->getOutWire(0);
	gate_p->addCNFClause(-Y->getCnfVar());
	//cout << "Constant 0 wire: " << Y->getName() << endl;
}

void Circuit::genCONST1CNF(Gate* gate_p){
	Wire* Y  = gate_p->getOutWire(0);
	gate_p->addCNFClause( Y->getCnfVar());
}

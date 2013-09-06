#include "coverVariable.h"
#include <iostream>
#include <stdlib.h>
#include <assert.h>

// CREATORS
CoverVariable::CoverVariable(string name, varType type, int bitwidth){
	d_name = name;
	d_type = type;
	d_bitwidth = bitwidth;
	for(int i = 0; i < d_bitwidth; ++i){
	  d_cnfVarList.push_back(0);
	  d_cnfVarList_origin.push_back(0);
	}
}

// MANIPULATORS
void CoverVariable::setCnfVar(int idx, int cnfVar){
	//cout << "Variable: " << d_name << " adding CNF var of bit: " << idx << endl;
	while(idx > d_cnfVarList.size())
	  d_cnfVarList.push_back(0);
	d_cnfVarList.at(idx) = cnfVar;
}

void CoverVariable::setCnfVar(int* cnfVar_p){
	for(int i = 0; i < d_bitwidth; ++i)
	  d_cnfVarList.at(i) = ++(*cnfVar_p);
}

void CoverVariable::incrCnfVar(int offset){
	for(int i = 0; i < d_bitwidth; ++i)
	  d_cnfVarList.at(i) += offset;
}

void CoverVariable::resetCnfVar(){
	d_cnfVarList = d_cnfVarList_origin;
}

void CoverVariable::setValue(int t, long int value){
	while((t+1) > d_valueList.size())
	  d_valueList.push_back(0);
	d_valueList.at(t) = value;
}

// ACCESSORS
string CoverVariable::getName() const{
	return d_name;
}

int CoverVariable::getBitwidth() const{
	return d_bitwidth;
}

long int CoverVariable::getValue(int t) const{
	return d_valueList.at(t);
}

int CoverVariable::getCnfVar(int idx) const{
	return d_cnfVarList.at(idx);
}

void CoverVariable::checkConsistence(){
	assert(d_bitwidth == d_cnfVarList.size());
	for(int i = 0; i < d_bitwidth; ++i){
	  if(d_cnfVarList.at(i) == 0){
	  	if(d_bitwidth == 1) 
	  		cout << __FILE__ << "::" << __LINE__ << ": Variable " << d_name << " has no CNF variable assigned." << endl;
	    else
	  	  cout << __FILE__ << "::" << __LINE__ << ": Variable " << d_name << "[" << i << "] has no CNF variable assigned." << endl;
	  	exit(1);
	  }
	}
	// Copy CNF var
	d_cnfVarList_origin = d_cnfVarList;
	
}

const vector<int>& CoverVariable::getCnfVarList(){
	return d_cnfVarList;
}

vector<int> CoverVariable::getCnfVarList(int msb, int lsb){
	vector<int> bitString;
	
	//cout << "Getting CNF var list ..." << endl;
	//cout << "Original list = ";
	//for(int j = d_cnfVarList.size() - 1; j>=0; --j) cout << "+" << d_cnfVarList.at(j) << "+";
	//cout << endl;
	
	if(msb == lsb){
		bitString.push_back(d_cnfVarList.at(lsb));
	}
	else{
	  for(int i = lsb; i <= msb; ++i)
	    bitString.push_back(d_cnfVarList.at(i));
	}
	
	//cout << "Return list[" << msb << ":" << lsb << "] = ";
	//for(int j = bitString.size() - 1; j>=0; --j) cout << "+" << bitString.at(j) << "+";
	//cout << endl;
	
	return bitString;
}

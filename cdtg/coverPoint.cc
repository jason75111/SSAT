#include "coverPoint.h"
#include "stateBin.h"
//#include "transBin.h"
#include <math.h>
#include <typeinfo>

// CREATORS
CoverPoint::CoverPoint(string name, int msb, int lsb, int AUTO_BIN_MAX, CoverVariable* var_p){
	d_name         = name;
	d_cnfVar       = -1;
	d_msb          = msb;
	d_lsb          = lsb;
	d_auto_bin_max = AUTO_BIN_MAX;
	d_var_p        = var_p;
}

CoverPoint::~CoverPoint(){
	for(int i=0;i<d_binList.size();++i){
		if(typeid(*(d_binList[i]))==typeid(StateBin)){
		  StateBin* ptr = static_cast<StateBin*>(d_binList[i]);
		  delete ptr;
		}
		/*else if(typeid(*(d_binList[i]))==typeid(TransBin)){
			TransBin* ptr = static_cast<TransBin*>(d_binList[i]);
			delete ptr;
		}*/
	}
}

// MANIPULATORS
void CoverPoint::addBin(BaseBin* bin_p){
	d_binList.push_back(bin_p);
}

void CoverPoint::setCnfVar(const int& cnfVar){
	d_cnfVar = cnfVar;
}

// ACCESSORS
const string& CoverPoint::getName() const{
  return d_name;	
}

const int& CoverPoint::getCnfVar() const{
	return d_cnfVar;
}

const int& CoverPoint::getMSB() const{
	return d_msb;
}

const int& CoverPoint::getLSB() const{
	return d_lsb;
}

const int CoverPoint::getAutoBinMax() const{
	return  d_auto_bin_max;
}

const int CoverPoint::getMaxVarValue() const{
	return (int) pow(2.0, d_var_p->getBitwidth()) - 1;
}

BaseBin* CoverPoint::getBin(const int& idx) const{
	return d_binList[idx];
}

const int CoverPoint::numOfBin() const{
	return d_binList.size();
}

CoverVariable* CoverPoint::getVar() const{
	return d_var_p;
}

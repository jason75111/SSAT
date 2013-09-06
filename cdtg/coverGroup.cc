#include "coverGroup.h"
#include <iostream>
#include <assert.h>

// CREATORS
CoverGroup::CoverGroup(){}

CoverGroup::~CoverGroup(){
    for(unsigned i=0;i<d_variableList.size();++i)
        delete d_variableList.at(i);
    for(unsigned i=0;i<d_coverPointList.size();++i)
        delete d_coverPointList.at(i);
    for(unsigned i=0;i<d_crossList.size();++i)
        delete d_crossList.at(i);
}

// MANIPULATORS
void CoverGroup::setName(const string& name){
    d_name = name;
}

void CoverGroup::setCnfVar(const int& cnfVar){
    d_cnfVar = cnfVar;
}

void CoverGroup::addVariable(CoverVariable* variable_p){
    d_variableList.push_back(variable_p);
}

void CoverGroup::addCoverPoint(CoverPoint* point_p){
    d_coverPointList.push_back(point_p);
}

void CoverGroup::addCross(Cross* cross_p){
    d_crossList.push_back(cross_p);
}

// ACCESSORS
string CoverGroup::getName() const{
    return d_name;
}

int CoverGroup::getCnfVar() const{
    return d_cnfVar;
}

BaseBin* CoverGroup::getBin(string binName) const{
    for(int i=0;i<d_coverPointList.size();++i){
        for(int j=0;j<d_coverPointList.at(i)->numOfBin();++j)
            if(d_coverPointList.at(i)->getBin(j)->getName().compare(binName)==0)
                return d_coverPointList.at(i)->getBin(j);
    }
    for(int i=0;i<d_crossList.size();++i){
        for(int j=0;j<d_crossList.at(i)->numOfBin();++j)
            if(d_crossList.at(i)->getBin(j)->getName().compare(binName)==0)
                return d_crossList.at(i)->getBin(j);
    }
    cout << "Can not find bin: " << binName << endl;
    exit(1);
}

CoverVariable* CoverGroup::getVariable(const string& varName) const{
    for(unsigned i=0;i<d_variableList.size();++i){
        if(varName.compare(d_variableList.at(i)->getName())==0)
            return d_variableList.at(i);
    }
    cout << "Can not find variable: " << varName << endl;
    exit(1);
}

CoverVariable* CoverGroup::getVariable(const int& idx) const{
    return d_variableList.at(idx);
}

CoverPoint* CoverGroup::getCoverPoint(const string& name) const{
    for(unsigned i=0;i<d_coverPointList.size();++i)
        if(d_coverPointList[i]->getName().compare(name)==0)
            return d_coverPointList[i];
    // if no matched coverpoint
    cout << "No coverpoint: " << name << " in cover group:" << d_name << endl;
    //exit(1);
    return NULL;
}

CoverPoint* CoverGroup::getCoverPoint(const int& idx) const{
    //cout << "In CoverGroup::getCoverPoint, idx = " << idx << endl;;
    return d_coverPointList.at(idx);
}

CoverPoint* CoverGroup::getLastCoverPoint() const{
    return d_coverPointList.back();
}

Cross* CoverGroup::getCross(const string& name) const{
    for(unsigned i=0;i<d_crossList.size();++i)
        if(d_crossList[i]->getName().compare(name)==0)
            return d_crossList[i];
    // if no matched cross
    cout << "No cross: " << name << " in cover group:" << d_name << endl;
    //exit(1);
    return NULL;
}

Cross* CoverGroup::getCross(const int& idx) const{
    return d_crossList.at(idx);
}

Cross* CoverGroup::getLastCross() const{
    return d_crossList.back();
}

int CoverGroup::numOfVariable() const{
    return d_variableList.size();
}

int CoverGroup::numOfCoverPoint() const{
    return d_coverPointList.size();
}

int CoverGroup::numOfCross() const{
    return d_crossList.size();
}

int CoverGroup::numOfBin() const{
    int num = 0;
    
    for(int i=0;i<d_coverPointList.size();++i)
        num += d_coverPointList.at(i)->numOfBin();
    for(int i=0;i<d_crossList.size();++i)
        num += d_crossList.at(i)->numOfBin();
    return num;
}

// Debug function
void CoverGroup::printVariableInfo(){
    cout << "==============================================================\n";
    cout << "Total " << d_variableList.size() << " variables" << endl;
    for(int i = 0; i < d_variableList.size(); ++i){
        cout << " variable " << d_variableList.at(i)->getName() << " : " << d_variableList.at(i)->getBitwidth() << "-bit" << endl;
        if(d_variableList.at(i)->getBitwidth() == 1){
            cout << "  CNF variable of " << d_variableList.at(i)->getName() << " = " << d_variableList.at(i)->getCnfVar(0) << endl;
        }
        else{
            for(int j = 0; j < d_variableList.at(i)->getBitwidth(); ++j){
                cout << "  CNF variable of " << d_variableList.at(i)->getName() << "[" << j << "] = " << d_variableList.at(i)->getCnfVar(j) << endl;
            }
        }
    }
}

void CoverGroup::printPointInfo(){
    cout << "==============================================================\n";
    cout << "Total " << d_coverPointList.size() << " coverpoint(s)" << endl;
    for(int i = 0; i < d_coverPointList.size(); ++i){
        cout << " coverpoint " << d_coverPointList.at(i)->getName() << "{" << endl;
        for(int j = 0; j < d_coverPointList.at(i)->numOfBin(); ++j){
            cout << "   bins " << d_coverPointList.at(i)->getBin(j)->getName() << " = ";
            d_coverPointList.at(i)->getBin(j)->print();
        }
        cout << " }\n";
    }
}

void CoverGroup::printCrossInfo(){
    CrossBin* crossBin_p;
    cout << "==============================================================\n";
    cout << "Total " << d_crossList.size() << " cross(es)" << endl;
    for(int i = 0; i < d_crossList.size(); ++i){
        cout << " cross " << d_crossList.at(i)->getName() << "{" << endl;
        for(int j = 0; j < d_crossList.at(i)->numOfBin(); ++j){
            crossBin_p = static_cast<CrossBin*>(d_crossList.at(i)->getBin(j));
            cout << "   bins " << crossBin_p->getName() << " = ";
            cout << crossBin_p->getSelecSpec() << " :\n";
            crossBin_p->print();
        }
        cout << " }\n";
    }
}


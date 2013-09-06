#include "stateBin.h"
#include "global.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace boost::icl;

// CREATORS
StateBin::StateBin(const string& name, const bool& isExcluded, const binType& type, const int& msb, const int& lsb, 
    const int& maxValue, CoverVariable* var_p){
    d_name       = name;
    d_cnfVar     = -1;
    d_var_p      = var_p;
    d_isExcluded = isExcluded;
    d_type       = type;
    d_hitCount   = 0;
    d_msb        = msb;
    d_lsb        = lsb;
    d_maxValue   = maxValue;
//  cout << "Start to build bin "  << d_name << endl;
}

// Following 2 constructors are for auto bin vector
StateBin::StateBin(const int& value, const int& msb, const int& lsb, const int& maxValue, CoverVariable* var_p){
    d_name        =  "auto";
    d_name        += '[';
    d_name        += Global::int2str(value);
    d_name        += ']';
    d_var_p       =  var_p;
    d_cnfVar      =  -1;
    d_isExcluded  =  false;
    d_type        =  AUTO_MULTI;
    d_hitCount    =  0;
    d_msb         =  msb;
    d_lsb         =  lsb;
    d_maxValue    =  maxValue;
    d_intervalSpec.insert(value);
//  cout << "Start to build bin "  << d_name << endl;
}

StateBin::StateBin(const int& lVal, const int& hVal, const int& msb, const int& lsb, const int& maxValue, CoverVariable* var_p){
    d_name         =  "auto";
    d_name         += '[';
    d_name         += Global::int2str(lVal);
    d_name         += ':';
    d_name         += Global::int2str(hVal);
    d_name         += ']';
    d_var_p        =  var_p;
    d_cnfVar       =  -1;
    d_isExcluded   =  false;
    d_type         =  AUTO_MULTI;
    d_hitCount     =  0;
    d_msb          =  msb;
    d_lsb          =  lsb;
    d_maxValue     =  maxValue;
    d_intervalSpec += interval<int>::closed(lVal, hVal);
//  cout << "Start to build bin "  << d_name << endl;
}

// Following 2 constructors are for fixed bin vector
StateBin::StateBin(const string& name, const int& value, const int& msb, const int& lsb, const int& maxValue, CoverVariable* var_p){
    d_name       =  name;
    d_name       += '[';
    d_name       += Global::int2str(value);
    d_name       += ']';
    d_var_p      =  var_p;
    d_cnfVar     =  -1;
    d_isExcluded =  false;
    d_type       =  AUTO_MULTI;
    d_hitCount   =  0;
    d_msb        =  msb;
    d_lsb        =  lsb;
    d_maxValue   =  maxValue;
    d_intervalSpec.insert(value);
//  cout << "Start to build bin "  << d_name << endl;
}

StateBin::StateBin(const string& name, const int& lVal, const int& hVal, const set<int>& multiValues, const int& msb, const int& lsb,
    const int& maxValue, CoverVariable* var_p){
    d_name       =  name;
    d_name       += '[';
    d_var_p      =  var_p;  
    d_cnfVar     =  -1;
    d_isExcluded =  false;
    d_type       =  AUTO_MULTI;
    d_hitCount   =  0;
    d_msb        =  msb;
    d_lsb        =  lsb;
    d_maxValue   =  maxValue;
    
    set<int>::iterator it = multiValues.begin();
    for(int i = 0; i < lVal; ++i, ++it);
    for(int i = lVal; i <= hVal; ++i, ++it){
        d_intervalSpec.insert(*it);
        d_name += Global::int2str(*it);
        if(i != hVal)
            d_name += ',';      
    }
    d_name += ']';
//cout << "Start to build bin "  << d_name << endl;
}

// MANIPULATORS
void StateBin::setSpec(string* binStr_p){
    string subStr1, subStr2;
    size_t find1, find2;
    int l,h;
    
    find1 = binStr_p->find_first_of('{');
    binStr_p->erase(0,find1+1);
    while(binStr_p->length()>2){
        find1 = binStr_p->find_first_of(',');
        if(find1==string::npos)
            find1 = binStr_p->find_first_of('}');
        
        if((*binStr_p)[0]=='['){// a range of values
            find1 = binStr_p->find_first_of(':');
            subStr1 = binStr_p->substr(1, find1-1);
            find2 = binStr_p->find_first_of(']');
            subStr2 = binStr_p->substr(find1+1, find2-find1-1);
            
            if(subStr1.compare("$")==0){// open range [$:v(h)]
                l = 0;
                h = Global::str2int(subStr2);
            }
            else if(subStr2.compare("$")==0){// open range [v(l):$]
                l = Global::str2int(subStr1);
                h = d_maxValue;
            }
            else{// [v(l):v(h)]
                l = Global::str2int(subStr1);
                h = Global::str2int(subStr2);
            }
            d_intervalSpec += interval<int>::closed(l, h);
            binStr_p->erase(0, find2+2);
        }// end a range of values
        else{// single value
            subStr1 = binStr_p->substr(0, find1);
            d_intervalSpec.insert(Global::str2int(subStr1));
            binStr_p->erase(0, find1+1);
        }
    }// end while
}

void StateBin::setCnfVar(const int cnfVar){
    d_cnfVar = cnfVar;
}

void StateBin::setMSB(int msb){
    d_msb = msb;
}

void StateBin::setLSB(int lsb){
    d_lsb = lsb;
}
    
void StateBin::genCnf(int* cnfVar_p){
    d_cnfClause.clear(); // Generate a new set of clauses
    interval_set<int>::iterator it = d_intervalSpec.begin();
    vector<int> intervalCnfVars;
    
    for(int i = 0; i < d_intervalSpec.iterative_size(); ++i, ++it){
        if(it->lower() == it->upper()){  // A single value
            //cout << "c Generating CNF clauses: " << it->lower()  << " of bin " << d_name << ", CNF variable = "<< d_cnfVar << endl;
            intervalCnfVars.push_back( ++(*cnfVar_p) );
            genEqualCnf(d_var_p->getCnfVarList(d_msb, d_lsb), it->lower(), *cnfVar_p);
        }
        else{  // A range of values
            //cout << "c Generating CNF clauses: [" << it->lower() << ":" << it->upper() << "] of bin " << d_name << ", CNF variable = "<< (*cnfVar_p)+1 << endl;
            intervalCnfVars.push_back( ++(*cnfVar_p) );
            genRangeCnf(d_var_p->getCnfVarList(d_msb, d_lsb), it->lower(), it->upper(), cnfVar_p);
        }
    }
    // OR CNF variables
    genOrCnf(intervalCnfVars, d_cnfVar);
    
    //cout << "CNF clauses of bin " << d_name << " is: " << endl;
    //printClause();
}

void StateBin::plusCount(){
    d_hitCount++;
}

// ACCESSORS
const string& StateBin::getName() const{
    return d_name;
}

const int& StateBin::getCnfVar() const{
    return d_cnfVar;
}

const int& StateBin::getHitCount() const{
    return d_hitCount;
}

void StateBin::print() const{
    interval_set<int>::iterator it=d_intervalSpec.begin();
    cout << "{ ";  
    for(int i=0;i<d_intervalSpec.iterative_size();++i,++it){
        if(it->lower() == it->upper()){ // single element interval
            cout << it->lower();
        }
        else{
            cout << *it;
        }
        if(i==d_intervalSpec.iterative_size()-1)
            cout << ' ';
        else
            cout << ", ";
    }
    cout << "};" << endl;
    //cout << d_intervalSpec << endl;
}

void StateBin::printClause() const{
    for(int i = 0; i < d_cnfClause.size(); ++i){
        if(d_cnfClause[i] == 0)
            cout << endl;
        else
          cout << d_cnfClause[i] << " ";
    }
}

const vector<int>& StateBin::getCNFClause() const{
    return d_cnfClause;
}

const bool StateBin::hasValue(int value) const{
    return contains(d_intervalSpec, value);
    // boost::icl function
}

const bool StateBin::hasValueLessOrEqualTo(int value) const{
    //return upper_less_equal(d_intervalSpec, value);
    // boost::icl function
    interval_set<int>::iterator it=d_intervalSpec.begin();
    if(it->lower() <= value){
//      cout << it->lower() << " <= " << value << endl;
        return true;
    }
    else
        return false;
}

const bool StateBin::hasValueGreaterOrEqualTo(int value) const{
    //return lower_less_equal(d_intervalSpec, value);
    // boost::icl function
    interval_set<int>::iterator it=d_intervalSpec.begin();
    for(int i=0;i<d_intervalSpec.iterative_size();++i,++it);
    --it;
    if(it->upper() >= value){
//cout << it->upper() << " >= " << value << endl;
        return true;
    }
    else
        return false;
}

// CLAUSE GENERATION FUNCTIONS
void StateBin::genEqualCnf(const vector<int> bitCnfVarList, const int& value, const int& cnfVar){
    vector<int> bitstring, tempVars;
    
    int2bitvector(value, &bitstring);
    
    // cout << "Gen equal CNF of value: " << value << " from CNF var list :";
    // for(int j = bitCnfVarList.size() - 1; j >=0; --j)
    //   cout << bitCnfVarList[j] << "+";
    // cout << endl;
    
    while(bitstring.size() < bitCnfVarList.size())
        bitstring.push_back(0);

    for(int i = 0; i < bitCnfVarList.size(); ++i){
        if(bitstring[i] == 1)
            tempVars.push_back(bitCnfVarList[i]);
        else
            tempVars.push_back(-bitCnfVarList[i]);
    }
    // AND all vars in tempVars
    genAndCnf(tempVars, cnfVar);
}

void StateBin::genRangeCnf(const vector<int> bitCnfVarList, const int& lBound, const int& uBound, int* cnfVar_p){
    const int finalOut = *cnfVar_p;
    vector<int> lBoundBitstring, uBoundBitstring;
    vector<int> tempClauseSet;
    
    int2bitvector(lBound, &lBoundBitstring);
    int2bitvector(uBound, &uBoundBitstring);
    while(lBoundBitstring.size()<bitCnfVarList.size())
        lBoundBitstring.push_back(0);
    while(uBoundBitstring.size()<bitCnfVarList.size())
        uBoundBitstring.push_back(0);

    tempClauseSet.clear();
    genLessCnf(bitCnfVarList, uBoundBitstring, bitCnfVarList.size() - 1, &tempClauseSet);  
    SopToPos(&tempClauseSet, cnfVar_p);
    const int lessCnfVar = *cnfVar_p;
    //cout << "c genLessCnf" << endl;
    for(int i=0;i<tempClauseSet.size();++i){
        d_cnfClause.push_back(tempClauseSet[i]);
    }
    
    tempClauseSet.clear();
    genLargerCnf(bitCnfVarList, lBoundBitstring, bitCnfVarList.size() - 1, &tempClauseSet);
    SopToPos(&tempClauseSet, cnfVar_p);
    const int largerCnfVar = *cnfVar_p;
    //cout << "c genLargerCnf" << endl;
    for(int i=0;i<tempClauseSet.size();++i){
        d_cnfClause.push_back(tempClauseSet[i]);
    }
    // AND the two comparators
    genAnd2Cnf(lessCnfVar, largerCnfVar, finalOut);
}

void StateBin::genLessCnf(const vector<int>& bitCnfVarList, const vector<int>& bitstring, const int i, vector<int>* clauseSet_p){
    if(i==0){
        if(bitstring[0]==0){
            clauseSet_p->push_back(-bitCnfVarList[0]);
            clauseSet_p->push_back(0);
        }
        else{
            clauseSet_p->push_back(bitCnfVarList[0]);
            clauseSet_p->push_back(-bitCnfVarList[0]);
            clauseSet_p->push_back(0);
        }
    }
    else{
        if(bitstring[i]==0){
            genLessCnf(bitCnfVarList, bitstring, i-1, clauseSet_p);
            clauseUnion(-bitCnfVarList[i], clauseSet_p);
        }
        else{
            genLessCnf(bitCnfVarList, bitstring, i-1, clauseSet_p);
            clauseDistrib(-bitCnfVarList[i], clauseSet_p);
        }
    }
}

void StateBin::genLargerCnf(const vector<int>& bitCnfVarList, const vector<int>& bitstring, const int i, vector<int>* clauseSet_p){
    if(i==0){
        if(bitstring[0]==0){
            clauseSet_p->push_back(bitCnfVarList[0]);
            clauseSet_p->push_back(-bitCnfVarList[0]);
            clauseSet_p->push_back(0);
        }
        else{
            clauseSet_p->push_back(bitCnfVarList[0]);
            clauseSet_p->push_back(0);
        }
    }
    else{
        if(bitstring[i]==0){
            genLargerCnf(bitCnfVarList, bitstring, i-1, clauseSet_p);
            clauseDistrib(bitCnfVarList[i], clauseSet_p);
        }
        else{
            genLargerCnf(bitCnfVarList, bitstring, i-1, clauseSet_p);
            clauseUnion(bitCnfVarList[i], clauseSet_p);
        }
    }
}

void StateBin::clauseUnion(int v, vector<int>* clauseSet_p){
    clauseSet_p->push_back(v);
    clauseSet_p->push_back(0);  
}

void StateBin::clauseDistrib(int v, vector<int>* clauseSet_p){
    vector<int> clauseSet;
    for(int i=0;i<clauseSet_p->size();++i){
        if(clauseSet_p->at(i)==0)
            clauseSet.push_back(v);
        clauseSet.push_back(clauseSet_p->at(i));
    }
    *clauseSet_p = clauseSet;
}

void StateBin::SopToPos(vector<int>* clauseSet_p, int* cnfVar_p){
    vector<int> totalClauseSet, tempClause, tempClauseSet, auxVars;
    // OR each clause's vars
    for(int i=0;i<clauseSet_p->size();++i){
        tempClause.push_back((*clauseSet_p)[i]);
        if((*clauseSet_p)[i]==0){
            tempClause.pop_back(); // Remove 0 
            genOrCnf(tempClause, ++(*cnfVar_p), &tempClauseSet);
            auxVars.push_back(*cnfVar_p);
            for(int j=0;j<tempClauseSet.size();++j){
                totalClauseSet.push_back(tempClauseSet[j]);
            }
            tempClause.clear();
            tempClauseSet.clear();
        }
    }
    // AND each clause
    genAndCnf(auxVars, ++(*cnfVar_p), &tempClauseSet);
    for(int i=0;i<tempClauseSet.size();++i)
        totalClauseSet.push_back(tempClauseSet[i]);
    *clauseSet_p = totalClauseSet;
}

void StateBin::genOrCnf(const vector<int>& cnfVarVec, int cnfVar, vector<int>* clauseSet_p){ // OR all vec vars and put in *clauseSet_p
    clauseSet_p->clear();
    for(int i=0;i<cnfVarVec.size();++i){
        clauseSet_p->push_back(-cnfVarVec[i]);
        clauseSet_p->push_back(cnfVar);
        clauseSet_p->push_back(0);
    }
    for(int i=0;i<cnfVarVec.size();++i){
        clauseSet_p->push_back(cnfVarVec[i]);
    }
    clauseSet_p->push_back(-cnfVar);
    clauseSet_p->push_back(0);
}

void StateBin::genOrCnf(const vector<int>& cnfVarVec, int cnfVar){ // OR all vec vars and output in d_cnfClause
    for(int i=0;i<cnfVarVec.size();++i){
        d_cnfClause.push_back(-cnfVarVec[i]);
        d_cnfClause.push_back(cnfVar);
        d_cnfClause.push_back(0);
    }
    for(int i=0;i<cnfVarVec.size();++i){
        d_cnfClause.push_back(cnfVarVec[i]);
    }
    d_cnfClause.push_back(-cnfVar);
    d_cnfClause.push_back(0);
}

void StateBin::genAndCnf(const vector<int>& cnfVarVec, int cnfVar, vector<int>* clauseSet_p){ // AND all vec vars and put in *clauseSet_p
    clauseSet_p->clear();
    for(int i=0;i<cnfVarVec.size();++i){
        clauseSet_p->push_back(cnfVarVec[i]);
        clauseSet_p->push_back(-cnfVar);
        clauseSet_p->push_back(0);
    }
    for(int i=0;i<cnfVarVec.size();++i)
        clauseSet_p->push_back(-cnfVarVec[i]);
    clauseSet_p->push_back(cnfVar);
    clauseSet_p->push_back(0);
}

void StateBin::genAndCnf(const vector<int>& cnfVarVec, int cnfVar){ // AND all vec vars and put in d_cnfClause
    for(int i=0;i<cnfVarVec.size();++i){
        d_cnfClause.push_back(cnfVarVec[i]);
        d_cnfClause.push_back(-cnfVar);
        d_cnfClause.push_back(0);
    }
    for(int i=0;i<cnfVarVec.size();++i){
        d_cnfClause.push_back(-cnfVarVec[i]);
    }
    d_cnfClause.push_back(cnfVar);
    d_cnfClause.push_back(0);
}

void StateBin::genAnd2Cnf(const int& in1,const int& in2, const int& cnfVar){// AND 2 vec vars
    d_cnfClause.push_back(in1);  d_cnfClause.push_back(-cnfVar);  d_cnfClause.push_back(0);
    d_cnfClause.push_back(in2);  d_cnfClause.push_back(-cnfVar);  d_cnfClause.push_back(0);
    d_cnfClause.push_back(-in1); d_cnfClause.push_back(-in2); d_cnfClause.push_back(cnfVar); d_cnfClause.push_back(0);
}

void StateBin::int2bitvector(int i, vector<int>* bitString_p){
    int r;
    bitString_p->clear();

    //cout << " Value " << i << " = ";
    if(i == 0){
        bitString_p->push_back(r);
        //cout << "1`b0" << endl;
        return;
    }
    
    while(i != 0){
        r = i - (i / 2) * 2;
        i/=2;
        bitString_p->push_back(r);
    }
    
    // cout << bitString_p->size() << "`b";
    // for(int j = bitString_p->size() - 1; j>=0; --j)
    //      cout << bitString_p->at(j);
    // cout << endl;
}

int StateBin::bitvector2int(const int& bitWidth, const vector<int>& bitString){
    int val=0;
    for(int i=0;i<bitWidth;i++)
        val += (int)(pow((double)2, i))*bitString[i];
    return val;
}


#include "crossBin.h"
#include "global.h"
#include <assert.h>
#include <limits.h>
#include <typeinfo>

using namespace std;

// CREATORS
CrossBin::CrossBin(const string& name, const bool& isExcluded, vector<CoverPoint*> crossList){
    d_name       = name;
    d_cnfVar     = -1;
    d_isExcluded = isExcluded;
    d_crossList  = crossList;
    d_hitCount   = 0;
}

CrossBin::~CrossBin(){}

// MANIPULATORS
void CrossBin::setSpec(string* binStr_p){
    size_t                    find1;
    bool                      isNegative=false;
    string                    selectStr, binsSelect;
    vector<size_t>            selectPosition;
    vector<vector<BaseBin*> > binsToCross;
    
    // Initialize binsToCross
    for(int i = 0; i < d_crossList.size(); ++i){
        vector<BaseBin*> temp;
        binsToCross.push_back(temp);
    }
    // Record selection spec in member "d_selectSpec"
    if(binStr_p->at(binStr_p->size() - 1) == ';')
        binStr_p->erase(binStr_p->size() - 1);
    d_selectSpec = *binStr_p;
    
    //////////////////////////////////////////////////////////////////
    // In the syntax of cross selection, 
    // operator "||" could be seen as a part of new selections
    // so we segment the spec string binStr_p based on "||"
    // Each iteration in the following while loop handles a segment
    
    while(!binStr_p->empty()){
        if((find1 = binStr_p->find("||")) != string::npos){
            // Find a "||", multiple part of selections
            selectStr = binStr_p->substr(0, find1);
            binStr_p->erase(0, find1 + 2);
        }
        else{
            // Just a single part of selection
            selectStr = *binStr_p;
            binStr_p->clear();
        }
        find1 = -1;
        selectPosition.clear();
        // Record the positions of "binsof" in this part of selection
        while((find1 = selectStr.find("binsof", find1 + 1)) != string::npos){     
            selectPosition.push_back(find1);
        }
    
        // Check if there are '!' and update select positions
        for(int i = 0; i < selectPosition.size(); ++i){
            if(selectPosition.at(i) != 0 && selectStr.at(selectPosition.at(i) - 1) == '!')
                selectPosition.at(i) -= (size_t) 1;
        }
        
        // Now select a subset of bins of a coverpoint
        if(selectPosition.size() == 1){
            isNegative = (selectStr.at(0) == '!') ? true : false;
            binsSelect = selectStr;
            selectBins(isNegative, &binsSelect, &binsToCross);      
        }
        else{
            for(int i = 0; i < selectPosition.size(); ++i){       
                isNegative = (selectStr.at(selectPosition.at(i)) == '!') ? true : false;
                int star = selectPosition.at(i);
                int length = ((i + 1) == selectPosition.size()) ?
                              selectPosition.size() - selectPosition[i] :
                              selectPosition.at(i + 1) - selectPosition.at(i) - 2;
                binsSelect = selectStr.substr(star, length);
                selectBins(isNegative, &binsSelect, &binsToCross); 
          }
        }
        
        // Add bins of coverpoints that not specified explictly
        for(int i = 0; i < binsToCross.size(); ++i){
            if(binsToCross.at(i).empty()){
                CoverPoint* point_p = d_crossList.at(i);
                for(int j = 0; j < point_p->numOfBin(); ++j){
                    binsToCross.at(i).push_back(point_p->getBin(j));
                }
            }
        }
        
        // Generate cross products
        genCrossProducts(&binsToCross);
        
        // Reset binsToCross
        for(int i = 0; i < d_crossList.size(); ++i)
            binsToCross.at(i).clear();
              
    }//end whle  

}

void CrossBin::setCnfVar(const int cnfVar){
    d_cnfVar = cnfVar;
}

void CrossBin::addCrossProduct(vector<BaseBin*> crossProduct){
    d_crossProductList.push_back(crossProduct);
}

void CrossBin::genCnf(int* cnfVar_p){
    d_cnfClause.clear(); // Generate a new set of clauses
    BaseBin* baseBin_p;
    vector<int> binCnfVars_v, productCnfVars_v;
    
    for(int i = 0; i < d_crossProductList.size(); ++i){
        for(int j = 0; j < d_crossProductList.at(i).size(); ++j){
            baseBin_p = d_crossProductList[i][j];
            if(baseBin_p->getCnfVar() != -1){
              binCnfVars_v.push_back(baseBin_p->getCnfVar());
            }
            else{// The bin has not been initial
                baseBin_p->setCnfVar(++(*cnfVar_p));
                binCnfVars_v.push_back(*cnfVar_p);
                baseBin_p->genCnf(cnfVar_p);
                /////////////////// !!!!!!!!!!!  Important !!!!!!!!! //////////////////////////////////////
                // Generated clauses need to be added to solver, now we put them to d_cnfClause ..
                const vector<int> binClause = baseBin_p->getCNFClause();
                for(int k = 0; k < binClause.size(); ++k)
                  d_cnfClause.push_back(binClause.at(k));
            }
        }
        // AND CNF variables
        productCnfVars_v.push_back(++(*cnfVar_p)); // push CNF variable of product term
        genAndCnf(binCnfVars_v, *cnfVar_p);
        binCnfVars_v.clear();
    }
    
    // OR cross product CNF variables
    genOrCnf(productCnfVars_v, d_cnfVar);
}

void CrossBin::plusCount(){
    d_hitCount++;
}

// ACCESSORS
const string& CrossBin::getName() const{
    return d_name;
}

const string& CrossBin::getSelecSpec() const{
    return d_selectSpec;
}

const int& CrossBin::getCnfVar() const{
    return d_cnfVar;
}

const int& CrossBin::getHitCount() const{
    return d_hitCount;
}

void CrossBin::print() const{
    //cout << "=============================================" << endl;
    //cout << "Selection spec: " << d_selectSpec << endl;
    for(int i = 0; i < d_crossProductList.size(); ++i){
        cout << "     <";
        for(int j = 0; j < d_crossProductList[i].size(); ++j){
            cout << d_crossProductList[i][j]->getName();
            if(j != d_crossProductList[i].size() - 1)               
                cout << ", ";
        }
        cout << ">" << endl;
    }   
}

const vector<int>& CrossBin::getCNFClause() const{
    return d_cnfClause;
}

void CrossBin::printClause() const{
    for(int i = 0; i < d_cnfClause.size(); ++i){
        if(d_cnfClause[i] == 0)
            cout << endl;
        else
          cout << d_cnfClause[i] << " ";
    }
}

// UTILITIES

// =============================================================================================
// Function selectBins()
// selects bins according to "string* binsSelect"
// and store they in "vector<vector<BaseBin*> >* binsToCross_p" 
// according to the order in cross list.
// For example: cross a,b,c
//              *binsSelect = binsof(c) intersect{[0:10]}
// then pointer of bins of coverpoint c that have value interseting [0:10]
// will be stored in (*binsToCross_p)[2].
// =============================================================================================
void CrossBin::selectBins(const bool& isNegative, string* binsSelect, vector<vector<BaseBin*> >* binsToCross_p){
    size_t find1, find2, find3;
    string pointName, binName, subStr1, subStr2, tempId;
    interval_set<int> intersectVal;
    int lessOrEuqal=-1, greaterOrEqual=INT_MAX;
    CoverPoint* point_p;
    BaseBin* baseBin_p;
    int idx=0;

    find1 = binsSelect->find_first_of('(');
    find2 = binsSelect->find_first_of('.');
    find3 = binsSelect->find_first_of(')');
//cout << "==========================================\nStart to select: " << *binsSelect << endl;   
    // Get id of of current selection
    if(find2!=string::npos){
        ointName = binsSelect->substr(find1+1, find2-find1-1);
        inName = binsSelect->substr(find2+1, find3-find2-1);
    }
    else{
        pointName = binsSelect->substr(find1+1, find3-find1-1);
    }
    
    // Get current coverpoint, here we assume a coverpoint name,
    // but not a variable name is specified.
    // In fact, if a variable is specified in the cross list,
    // a coverpoint is created for it, check Cross::setSpec(string*)
    point_p = NULL;
    for(idx=0;idx<d_crossList.size();++idx){
        if(d_crossList[idx]->getName().compare(pointName)==0){
            point_p = d_crossList[idx];
            break;
      }
    }
    // Check if coverpoint is not found
    assert(point_p!=NULL);
    
    // Check if the coverpoint has been selected before
    assert(binsToCross_p->at(idx).empty());
    
    if(!binName.empty()){ // A specific bin has been selected
        for(int i=0;i<point_p->numOfBin();++i){
            baseBin_p = point_p->getBin(i);
            tempId = baseBin_p->getName();          
            //if((find1=tempId.find_first_of('['))!=string::npos)
              //tempId.erase(find1);            
            if(tempId.compare(binName)==0){
//cout << "  Select bin: " << baseBin_p->getName() << endl;
                binsToCross_p->at(idx).push_back(baseBin_p);
          }
        }
    }
    else{ // A subset of bins are selected    
        if((find1=binsSelect->find_first_of('{'))!=string::npos){// "intersect" operation
            binsSelect->erase(0,find1+1);
            // Identify intersection value
            while(binsSelect->length()>0){
                find1 = binsSelect->find_first_of(',');
                if(find1==string::npos)
                    find1 = binsSelect->find_first_of('}');
                    //find1 = binsSelect->at(binsSelect->size()-1);
            
                if((*binsSelect)[0]=='['){// a range of values
                    find1 = binsSelect->find_first_of(':');
                    subStr1 = binsSelect->substr(1, find1-1);
                    find2 = binsSelect->find_first_of(']');
                    subStr2 = binsSelect->substr(find1+1, find2-find1-1);
              
                    if(subStr1.compare("$")==0){// open range [$:v(h)]
                        int t = Global::str2int(subStr2);
                        if(lessOrEuqal==-1 || t<lessOrEuqal)
                            lessOrEuqal = t;           
                    }
                    else if(subStr2.compare("$")==0){// open range [v(l):$]
                        int t = Global::str2int(subStr1);
                        if(greaterOrEqual==INT_MAX || t>greaterOrEqual)
                            greaterOrEqual = t;
                    }
                    else{// [v(l):v(h)]
                        int l = Global::str2int(subStr1);
                        int h = Global::str2int(subStr2);
                        intersectVal += interval<int>::closed(l, h);
                    }        
                    binsSelect->erase(0, find2+2);
                }// end a range of values
                else{// single value
                    subStr1 = binsSelect->substr(0, find1);
                    intersectVal.insert(Global::str2int(subStr1));
//                  cout << "Value "   << Global::str2int(subStr1) << " is selected" << endl;
                    binsSelect->erase(0, find1+1);
                }
            }// end while
      
            // Identify bins that meet intersection   
            for(int i=0;i<point_p->numOfBin();++i){
                StateBin* stateBin_p;
                baseBin_p = point_p->getBin(i);
                // Check binType, currently not support transition bin intersection 
                if(typeid(*(baseBin_p))==typeid(StateBin)){
                    stateBin_p = static_cast<StateBin*>(baseBin_p);
                }
                else 
                    continue;
            
                if(!isNegative){
                    if(stateBin_p->hasValueLessOrEqualTo(lessOrEuqal) || stateBin_p->hasValueGreaterOrEqualTo(greaterOrEqual)){
//                      cout << "  Select bin: " << baseBin_p->getName() << endl;
                        binsToCross_p->at(idx).push_back(stateBin_p);
                    }
                    else{
                        for(interval_set<int>::element_const_iterator it = elements_begin(intersectVal); it != elements_end(intersectVal); ++it){
                            if(stateBin_p->hasValue(*it)){
//                              cout << "  Select bin: " << baseBin_p->getName() << endl;
                                binsToCross_p->at(idx).push_back(stateBin_p);
                                break;
                            }
                        }
                    }
                }// end if(!isNegative)
                else{ // isNegative
                    if(!stateBin_p->hasValueLessOrEqualTo(lessOrEuqal) && !stateBin_p->hasValueGreaterOrEqualTo(greaterOrEqual)){
                        interval_set<int>::element_const_iterator it = elements_begin(intersectVal);
                        for(;it != elements_end(intersectVal); ++it){
                            if(stateBin_p->hasValue(*it))
                                break;
                        }
//                      cout << "  Select bin: " << baseBin_p->getName() << endl;
                        if(it==elements_end(intersectVal))
                        binsToCross_p->at(idx).push_back(stateBin_p);
                    }
                }
            }
        }
        else{ // Select all bins of coverpoint      
            for(int i=0;i<point_p->numOfBin();++i){
//              cout << "  Select bin: " << baseBin_p->getName() << endl;
                binsToCross_p->at(idx).push_back(baseBin_p);
            }
        }
    }
}

// =============================================================================================
// Function genCrossProducts()
// generates cross products of bins according to "binsToCross_p"
// and put results to "d_crossProductList".
// Note that duplicated cross products are not checked
// due to potential complexity.
// =============================================================================================
void CrossBin::genCrossProducts(vector<vector<BaseBin*> >* binsToCross_p){
    vector<int> loopTime;
    int max = 1;
    vector<BaseBin*> tempProduct;
  
    const int cross_size = binsToCross_p->size();
    for(int i = 0; i < cross_size; ++i)
        loopTime.push_back(binsToCross_p->at(i).size());
      
    for(int i = 0; i < loopTime.size(); ++i)
        max *= loopTime.at(i);
    
    for(int i = 0; i < max; ++i){
        int t = i;
        tempProduct.clear();
        for(int j = 0; j < loopTime.size(); ++j){
            int idx = t%loopTime.at(j);
            tempProduct.push_back(binsToCross_p->at(j).at(idx));      
            t /= loopTime.at(j);
        }
        d_crossProductList.push_back(tempProduct);
    }
}

// CLAUSE GENERATION FUNCTIONS

void CrossBin::genAndCnf(const vector<int>& cnfVarVec, int cnfVar){
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

void CrossBin::genOrCnf(const vector<int>& cnfVarVec, int cnfVar){
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


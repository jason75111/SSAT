#include "cross.h"

// CREATORS
Cross::Cross(const string& name, CoverGroup* belongGroup_p ){
    d_name          = name;
    d_cnfVar        = -1;
    d_belongGroup_p = belongGroup_p;
}

Cross::~Cross(){
    for(int i = 0; i < d_binList.size(); ++i)
        delete d_binList[i];
}

// MANIPULATORS
void Cross::setSpec(string* str_p){
    string         name;
    size_t         find;
    CoverPoint*    point_p;
    CoverVariable* var_p;
    BaseBin*       baseBin_p;
    int            msb, lsb;
  
    while(!str_p->empty()){
        if( (find = str_p->find_first_of(',')) != string::npos){
            name = str_p->substr(0, find);
        }
        else{
            find = str_p->at(str_p->size()-1);
            name = str_p->substr(0, find);
        }
        
        if((point_p = d_belongGroup_p->getCoverPoint(name))){// A cover point, push to cross list
            d_crossList.push_back(point_p);
        }
        else{// A variable which is not a coverpoint yet, create a coverpoint for it
            var_p = d_belongGroup_p->getVariable(name);
            msb = var_p->getBitwidth() - 1;
            lsb = 0;
            point_p = new CoverPoint(name, msb, lsb, Global::AUTO_BIN_MAX, var_p);
            
            ////////////////////////////////////////////////
            // Add auto bins for the coverpoint,
            // implictly create bins auto[0 : maxValue]
            
            const int maxValue = point_p->getMaxVarValue();
            if(maxValue<=point_p->getAutoBinMax()){
                // Check if # of bins less than AUTO_BIN_MAX
                // Create bins auto[0] - auto[maxValue]
                for(int i=0;i<maxValue;++i){
                    baseBin_p = new StateBin(i, msb, lsb, maxValue, var_p);
                    point_p->addBin(baseBin_p);
                }
            }
            else{
                // Create AUTO_BIN_MAX bins auto[0, ...] ~ auto[..., maxValue]
                int numOfValue = maxValue;
                int numOfBin   = point_p->getAutoBinMax();
                int rangeSize  = numOfValue / numOfBin;
                int l = 0, h = 0;
                if(numOfValue == (numOfBin * rangeSize) ){
                    for(int i=0;i<numOfBin;++i){
                        l = 0 + i * rangeSize;
                        h = l + rangeSize - 1;
                        baseBin_p = new StateBin(l, h, msb, lsb, maxValue, var_p);
                        point_p->addBin(baseBin_p);
                    }
                }
                else{
                    for(int i=0;i<numOfBin-1;++i){
                        l = 0 + i * rangeSize;
                        h = l + rangeSize - 1;
                        baseBin_p = new StateBin(l, h, msb, lsb, maxValue, var_p);
                        point_p->addBin(baseBin_p);
                    }
                    l += rangeSize;
                    h = maxValue - 1;
                    baseBin_p = new StateBin(l, h, msb, lsb, maxValue, var_p);
                    point_p->addBin(baseBin_p);
                }
            }// end else
        
            // Push the coverpoint to cross list
            d_crossList.push_back(point_p);
        }
        str_p->erase(0, find + 1);
    }// end while
    d_crossSize = d_crossList.size();
 
}

void Cross::addBin(CrossBin* bin_p){
    d_binList.push_back(bin_p);
}

void Cross::setCnfVar(const int& cnfVar){
    d_cnfVar = cnfVar;
}

// ACCESSORS
const string& Cross::getName() const{
    return d_name;    
}

const int& Cross::getCnfVar() const{
    return d_cnfVar;
}

vector<CoverPoint*> Cross::getCrossList() const{
    return d_crossList;
}

BaseBin* Cross::getBin(const int& idx) const{
    return d_binList[idx];
}

const int Cross::numOfBin() const{
    return d_binList.size();
}


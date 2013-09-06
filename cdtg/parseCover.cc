#include "parseCover.h"
#include <iostream>
#include <assert.h>
#include <typeinfo>

// CREATORS
ParseCover::ParseCover(Circuit* circuit_p, CoverGroup* coverGroup_p, char* coverFile, int* cnfVar_p){
  d_circuit_p = circuit_p;
  d_coverGroup_p = coverGroup_p;
  d_cnfVar_p = cnfVar_p;
  d_coverFile.open(coverFile, ifstream::in);
  if(!d_coverFile){
    cout << "File could not be opened" << endl;
    exit(1);
  }
}

// MANIPULATORS
// MAIN PARSING FUNCTION
void ParseCover::parse(){
  char   cStr[Global::STR_LENGTH];
  string str;
  
  while(d_coverFile.getline(cStr, Global::STR_LENGTH)){
    assert(d_coverFile.gcount() < Global::STR_LENGTH);
    str = string(cStr);
    Global::eraseBlanks(&str);
    if(!str.empty()){
      switch(parseTypeAnalysis(str)){
        case VARIABLE:
          parseVariable(&str);
          //cout << "After parse var\n";
          break;
        case COVERGROUP:
          parseGroup(&str);
          //cout << "After parse group\n";
          break;
        case COVERPOINT:        
          parsePoint(&str);
          //cout << "After parse point\n";
          break;
        case CROSS:       
          parseCross(&str);
          //cout << "After parse cross\n";
          break;
        default:
        	cout << "Wrong string : \"" << str <<"\"" << endl;
          ;
          //cout << "Pass string : \"" << str <<"\"" << endl;
      }//end switch 
    }
  }//end while
}

// UTILITIES
ParseCover::parseType ParseCover::parseTypeAnalysis(const string& str){
  size_t find;
  if(str.compare(0, 3, "bit") == 0)
    return VARIABLE;
  if(str.compare(0, 10, "covergroup") == 0)
    return COVERGROUP;
  if(str.compare(0, 10, "coverpoint") == 0)
    return COVERPOINT;
  if(str.compare(0, 5, "cross") == 0)
    return CROSS;
  
  if((find = str.find_first_of(':')) != string::npos)
    if(str.compare(find + 1, 10, "coverpoint") == 0)
      return COVERPOINT;
    else if(str.compare(find + 1, 5, "cross") == 0)
      return CROSS;
    else
      return OTHER;
  // else
  return OTHER;
}

void ParseCover::parseVariable(string* str_p){
  size_t find1, find2;
  string subStr, name, cname;
  int bitwidth, h, l;
  CoverVariable::varType type;
  	
  //////////////////////////////////////////////////////////////
  // Here we assume the variable descrption in the file is:
  // bit ([SIZE-1:0]) VARIABLE_NAME; // VARIABLE_TYPE
  // For example:
  // bit wr_en; // PI
  // bit [3:0] counter; // PPO
  
  // Identify variable type
  find1 = str_p->find_first_of('/'); 
  ++find1;
  find2 = str_p->length();
  subStr = str_p->substr(find1+1, find2-find1-1);
  if(subStr.compare("PI")==0) type = CoverVariable::PI;
  else if(subStr.compare("PPI")==0) type = CoverVariable::PPI;
  else if(subStr.compare("LOCAL")==0) type = CoverVariable::LOCAL;
  else{ cout << "Can not identify variable type from string:\"" << *str_p << "\"" << endl; exit(1);}
  
  // Identify bit width
  if((find1 = str_p->find_first_of('[')) == string::npos) {
  	bitwidth = 1;
  }
  else{
    find2  = str_p->find_first_of(':');
    subStr = str_p->substr(find1 + 1, find2 - find1 - 1);
    h      = Global::str2int(subStr) + 1;
    find1  = find2;
    find2  = str_p->find_first_of(']');
    subStr = str_p->substr(find1 + 1, find2 - find1 - 1);
    l      = Global::str2int(subStr) + 1;
    bitwidth = h - l + 1;
    if(l != 0) cout << "Warning! lsb is not 0" << endl;
  }
  
  // Identify variable name
  if((find1 = str_p->find_first_of(']')) == string::npos) 
  	find1 = 2; // after "bit", maybe need to modify
  find2 = str_p->find_first_of(';');
  name  = str_p->substr(find1 + 1, find2 - find1 - 1);
  
  // Construct variable object
  CoverVariable* var_p = new CoverVariable(name, type, bitwidth);
  
  // Set variable to be consistent with circuit signal
  int bitDigit;
  int cnfVar = -1;
  if(type == CoverVariable::PI){
    for(int i = 0; i < d_circuit_p->PIListSize(); ++i){
    	subStr = d_circuit_p->getPI(i)->getName();
    	if((find1 = subStr.find_first_of('[')) != string::npos)
    		cname = subStr.substr(0, find1);
    	else
    		cname = subStr;
      if(name.compare(cname) == 0){
        if((find1 = subStr.find_first_of('[')) == string::npos) 
        	bitDigit = 0; // 1-bit
        else{
          find2 = subStr.find_first_of(']');
          assert(find2 != string::npos);
          bitDigit = Global::str2int(subStr.substr(find1 + 1, find2 - find1 - 1));
        }
        cnfVar = d_circuit_p->getPI(i)->getCnfVar();
        var_p->setCnfVar(bitDigit, cnfVar);
//        cout << "Set CNF var of variable: " << name << ", CNF var = " << cnfVar << endl;
      }
    }
    if(cnfVar == -1){
    	cout << __FILE__ << ":" << __LINE__<< ": Cannot find PI: " << name << endl;
    	exit(1);
    }
  }
  else if(type == CoverVariable::PPI){
    for(int i = 0; i < d_circuit_p->PPIListSize(); ++i){
      subStr = d_circuit_p->getPPI(i)->getName();
      if((find1 = subStr.find_first_of('[')) != string::npos)
    		cname = subStr.substr(0, find1);
    	else
    		cname = subStr;
      if(name.compare(cname) == 0){
        if((find1 = subStr.find_first_of('[')) == string::npos) 
        	bitDigit = 0;  // 1-bit 
        else{
          find2 = subStr.find_first_of(']');
          assert(find2 != string::npos);
          bitDigit = Global::str2int(subStr.substr(find1 + 1, find2 - find1 - 1));
        }
        cnfVar = d_circuit_p->getPPI(i)->getCnfVar();
        var_p->setCnfVar(bitDigit, cnfVar);
//        cout << "Set CNF var of variable: " << name << ", CNF var = " << cnfVar << endl;
      }
    }
    if(cnfVar == -1){
    	cout << __FILE__ << ":" << __LINE__<< "Cannot find PPI: " << name << endl;
    	exit(1);
    }
  }
  else{ // type == CoverVariable::LOCAL
    var_p->setCnfVar(d_cnfVar_p);
  }
  
  // Check if any inconsistence between circuit signal and variable
  var_p->checkConsistence();
  d_coverGroup_p->addVariable(var_p);
}

void ParseCover::parseGroup(string* str_p){
  size_t find;
  string subStr;
  
  if((find = str_p->find_first_of('@')) == string::npos)
    find = str_p->find_first_of(';');
  subStr = str_p->substr(10, find - 10);
  d_coverGroup_p->setName(subStr);
}

void ParseCover::parsePoint(string* str_p){
  char           cStr[Global::STR_LENGTH];
  bool           noSpecifyBins = false;
  size_t         find;
  string         str, name, varName;
  CoverVariable* var_p;
  BaseBin*       baseBin_p;
  StateBin*      stateBin_p;
  
  //////////////////////////////////////////////////////////////////////
  // Parse a coverage point.
  // Here we assume the format of coverage points are
  // 1. single line (and no specified bins):
  //    NAME: coverpoint VARIABLE_NAME;
  // 2. single line (and follows the bins): 
  //    NAME: coverpoint VARIABLE_NAME{
  //       bins 1 = ...;
  //       bins 2 = ...;
  //    } // and ends with a single line with only a single'}'
  
  // Check if implicit (no bins specified)
  if(str_p->at(str_p->size()-1) == ';'){
    noSpecifyBins = true;
    str_p->erase(str_p->size() - 1);
  }
  
  // Recognize coverpoint_name & sampling variable of coverpoint
  if(str_p->compare(0, 10, "coverpoint") == 0){// No coverpoint_name is specified
    str_p->erase(0, 10);
    if(noSpecifyBins){
      name = varName = *str_p;
    }
    else if((find = str_p->find_first_of('{')) != string::npos){
      name = varName = str_p->substr(0, find);
    }
    else{
      cout << "Unknown error when parsing string " << *str_p << endl;
      exit(1);
    }
  }
  else{// A coverpoint_name is specified
    find = str_p->find_first_of(':');
    name = str_p->substr(0, find);
    str_p->erase(0, find + 11);
    if(noSpecifyBins){
      varName = *str_p;
    }
    else if((find = str_p->find_first_of('{'))!= string::npos){
      varName = str_p->substr(0, find);
    }
    else{
      cout << "Unknown error when parsing string " << *str_p << endl;
      exit(1);
    }
  }
  
  // Check if VARIABLE subexpression : VARIABLE[MSB:LSB]
  int find2, msb, lsb;
  if((find = varName.find_first_of('[')) != string::npos){ // partial assignment
  	if((find2 = varName.find_first_of(':')) != string::npos){
  	  msb     = Global::str2int( varName.substr(find + 1, find2 - find - 1) );
  	  find    = find2;
  	  find2   = varName.find_first_of(']');
  	  lsb     = Global::str2int( varName.substr(find + 1, find2 - find - 1) );
  	}
  	else{ // VARIABLE[IDX]
  		msb = lsb = Global::str2int( varName.substr(find + 1, 1) );
  	}
  	find    = varName.find_first_of('[');
  	varName = varName.substr(0, find);
  }
  else{ // whole assignment
  	var_p = d_coverGroup_p->getVariable(varName);
  	if(var_p->getBitwidth() == 1) { msb = lsb = 0; }
  	else { msb = var_p->getBitwidth() - 1; lsb = 0; }
  }
  assert(msb >= lsb);
  // Create coverpoint object
  var_p = d_coverGroup_p->getVariable(varName);
  CoverPoint* point_p = new CoverPoint(name, msb, lsb, Global::AUTO_BIN_MAX, var_p);
  // Add this cover point to the cover group
  d_coverGroup_p->addCoverPoint(point_p);

  ////////////////////////////////////////////////////////////////
  // Parse coverage bins
  // Here we assume the format of each coverage bin is
  // in a single line, e.q. bins a = { ... };
  
  string      subStr1, subStr2;
  vector<int> multiValues; // Used to record values of bin vectors
  const int   maxValue = point_p->getMaxVarValue();
  
  if(noSpecifyBins){// Implictly create bins auto[0:maxValue]
    if((maxValue + 1) <= point_p->getAutoBinMax()){
      // Check if # of bins less than AUTO_BIN_MAX
      // Create bins auto[0] ~ auto[maxValue]
      for(int i = 0; i <= maxValue; ++i){
        baseBin_p = new StateBin(i, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
        point_p->addBin(baseBin_p);
      }
    }
    else{
      // Create AUTO_BIN_MAX bins auto[0,...] ~ auto[...,maxValue]
      int numOfValue = maxValue + 1;
      int numOfBin   = point_p->getAutoBinMax();
      int rangeSize  = numOfValue / numOfBin;
      int l=0, h=0;
      if(numOfValue==numOfBin*rangeSize){
        for(int i=0;i<numOfBin;++i){
          l = 0 + i * rangeSize;
          h = l + rangeSize - 1;
          baseBin_p = new StateBin(l, h, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
          point_p->addBin(baseBin_p);
        }
      }
      else{
        for(int i=0;i<numOfBin-1;++i){
          l = 0 + i * rangeSize;
          h = l + rangeSize - 1;
          stateBin_p = new StateBin(l, h, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
          baseBin_p = stateBin_p;
          point_p->addBin(baseBin_p);
        }
        l += rangeSize;
        h = maxValue;
        baseBin_p = new StateBin(l, h, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
        point_p->addBin(baseBin_p);
      }
    }// end else  
  }
  else{// Parse user-specified bins
    while(1){
      d_coverFile.getline(cStr, Global::STR_LENGTH);
      assert(d_coverFile.gcount()<Global::STR_LENGTH);
      str = string(cStr);
      Global::eraseBlanks(&str);
      if(str[0] == '}') break;
      // Parsing bins
      parseBin(point_p, &str, var_p); // Call the bins-parsing function
    }
  }
}

void ParseCover::parseBin(CoverPoint* point_p, string* str_p, CoverVariable* var_p){
  size_t      find1, find2;
  string      name;
  bool        isWildcard = false, isExcluded = false;
  BaseBin*    baseBin_p  = NULL;
  const int   maxValue   = point_p->getMaxVarValue();
  set<int>    multiValues_s; // Used to record values of bin vectors
  vector<int> multiValues_v; // Used to record values of fix bin vectors
  
  if(str_p->compare(0, 8, "wildcard") == 0){
    isWildcard = true;
    str_p->erase(0, 8);
  }
  if(str_p->compare(0, 7, "ignore_") == 0){
    isExcluded = true;
    str_p->erase(0, 7);
  }
  else if(str_p->compare(0, 8, "illegal_") == 0){
    isExcluded = true;
    str_p->erase(0, 8);
  }
  
  if((find1 = str_p->find_first_of('{')) != string::npos){ // State bin
    find1 = str_p->find_first_of('[');
    find2 = str_p->find_first_of('=');
    
    if(find1 > find2){ // Single range
      name = str_p->substr(4, find2-4);
      str_p->erase(0, find2+1);
      if(isWildcard){ // to be construct
        //baseBin_p = new WildcardValueBin(name, isExcluded, BaseBin::SINGLE);
      }
      else{
        baseBin_p = new StateBin(name, isExcluded, BaseBin::SINGLE, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
      }
      baseBin_p->setSpec(str_p);  // Call the setSpec() function depends on individual bin type
      point_p->addBin(baseBin_p);
    }
    else{// Multirange
      // A bin vector is splited to multiple single bins
      find2 = str_p->find_first_of(']');
      if(find2==(find1+1)){// Auto multirange
        name = str_p->substr(4, find1-4);
        str_p->erase(0, find2+2);
        
        // Record values of bin vector
        recordValues(&multiValues_s, maxValue, str_p);
        
        if((int)multiValues_s.size()<=point_p->getAutoBinMax()){
          // Check if # of bins less than AUTO_BIN_MAX
          // Create bins bin_name[...] - bin_name[...]
          for(set<int>::iterator it=multiValues_s.begin();it!=multiValues_s.end();++it){
            if(isWildcard){
              // to be construct
              //baseBin_p = new WildcardValueBin(name, isExcluded, BaseBin::AUTO_MULTI);
            }
            else{
              baseBin_p = new StateBin(name, *it, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
            }
            point_p->addBin(baseBin_p);
          }
        }
        else{
          // Create AUTO_BIN_MAX bins bin_name[...] - bin_name[...]
          int numOfValue = multiValues_s.size();
          int numOfBin = point_p->getAutoBinMax();
          int rangeSize = numOfValue / numOfBin;
          int l=0, h=0;
          if(numOfValue==numOfBin*rangeSize){
            for(int i=0;i<numOfBin;++i){
              l = 0 + i* rangeSize;
              h = l + rangeSize - 1;
              if(isWildcard){
                // to be construct
                //baseBin_p = new WildcardValueBin(name, isExcluded, BaseBin::AUTO_MULTI);
              }
              else{
                baseBin_p = new StateBin(name, l, h, multiValues_s, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
              }
              point_p->addBin(baseBin_p);             
            }
          }
          else{
            for(int i=0;i<numOfBin-1;++i){
              l = 0 + i * rangeSize;
              h = l + rangeSize - 1;
              if(isWildcard){
                // to be construct
                //baseBin_p = new WildcardValueBin(name, isExcluded, BaseBin::AUTO_MULTI);
              }
              else{
                baseBin_p = new StateBin(name, l, h, multiValues_s, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
              }
              point_p->addBin(baseBin_p);
            }
            l += rangeSize;
            h = numOfValue - 1;
            if(isWildcard){
              // to be construct
              //baseBin_p = new WildcardValueBin(name, isExcluded, BaseBin::AUTO_MULTI);
            }
            else{
              baseBin_p = new StateBin(name, l, h, multiValues_s, point_p->getMSB(), point_p->getLSB(), maxValue, var_p);
            }
            point_p->addBin(baseBin_p);
          }
        }// end else
      }
      else{// Fixed multirange
        // to be construct
        /*name = str_p->substr(4, find1-4);        
        int numOfBin = Global::str2int(str_p->substr(find1+1,find2-find1-1));
        str_p->erase(0, find2+2);
        
        // Record values of bin vector
        recordValues(&multiValues_v, maxValue, str_p);
        
        // Create numOfBin bins BIN_NAME[...] ~ BIN_NAME[...]
        int numOfValue = multiValues_v.size();
        int rangeSize = numOfValue / numOfBin;
        int l=0, h=0;
        if(numOfValue==numOfBin*rangeSize){
          for(int i=0;i<numOfBin;++i){
            l = 0 + i* rangeSize;
            h = l + rangeSize - 1;
            if(isWildcard){
              // to be construct
              //baseBin_p = new FixMultiWildcardValueBin(name, isExcluded, BaseBin::FIXED_MULTI, numOfBin);
            }
            else{
              baseBin_p = new FixMultiValueBin(name, isExcluded, BaseBin::FIXED_MULTI, maxValue, l, h, multiValues_v);
            }
            point_p->addBin(baseBin_p);             
          }
        }
        else{
          for(int i=0;i<numOfBin-1;++i){
            l = 0 + i * rangeSize;
            h = l + rangeSize - 1;
            if(isWildcard){
              // to be construct
              //baseBin_p = new WildcardValueBin(name, isExcluded, BaseBin::AUTO_MULTI);
            }
            else{
              baseBin_p = new FixMultiValueBin(name, isExcluded, BaseBin::FIXED_MULTI, maxValue, l, h, multiValues_v);
            }
            point_p->addBin(baseBin_p);
          }
          l += rangeSize;
          h = numOfValue - 1;
          if(isWildcard){
            // to be construct
            //baseBin_p = new FixMultiWildcardValueBin(name, isExcluded, BaseBin::FIXED_MULTI, numOfBin);
          }
          else{
            baseBin_p = new FixMultiValueBin(name, isExcluded, BaseBin::FIXED_MULTI, maxValue, l, h, multiValues_v);
          }
          point_p->addBin(baseBin_p);
        }*/
      }
    }
  }// end of parsing a state bin
  else{// Transition bin, currently unsupport
    
  }
  assert(baseBin_p!=NULL);
}

void ParseCover::parseCross(string* str_p){
  char   cStr[Global::STR_LENGTH];
  bool   noSpecifyBins = false;
  size_t find;
  string str, tempStr, name, spec;  
  
  //////////////////////////////////////////////////////////////////////////
  // Parse a cross coverage
  // here we assume the format of cross coverages are
  // 1. single line (and no specified bins) :  (axb :) cross v_a, v_b;
  // 2. single line (and follows the bins) : 
  //    (axb :) cross v_a, v_b{
  //       bins 1 ...;
  //       bins 2 ...;
  //    } // and ends with a single line with only a single'}'
  
  // Check if implicit
  if(str_p->at(str_p->size()-1) == ';'){
    noSpecifyBins = true;
    str_p->erase(str_p->size() - 1);// Erase ';'
  }
  // Recognize name & cross variables(spec)
  find = str_p->find_first_of(':');
  if(find == string::npos){
    str_p->erase(0, 5);// Erase initial "cross"
    if(noSpecifyBins){
      name = spec = *str_p;
    }
    else if((find = str_p->find_first_of('{')) != string::npos){
      name = spec = str_p->substr(0, find);
    }
    else{
      cout << "Unknown error when parsing string" << *str_p << endl;
      exit(1);
    }
  }
  else{
    name = str_p->substr(0, find);
    str_p->erase(0, find+6);
    if(noSpecifyBins){
      spec = *str_p;
    }
    else if((find = str_p->find_first_of('{'))!= string::npos){
      spec = str_p->substr(0, find);
    }
    else{
      cout << "Unknown error when parsing string" << *str_p << endl;
      exit(1);
    }
  }
  
  // Create cross object
  Cross* cross_p = new Cross(name, d_coverGroup_p);
  cross_p->setSpec(&spec);
  // Add this cross to the cover group
  d_coverGroup_p->addCross(cross_p);
  
  //////////////////////////////////////////////////////////////////
  // Parse cross bins,
  // here we assume the format of cross bins are
  // 1. single line, e.q. bins c =  ...;
  // 2. multiple line :  
  //    bins c = { ...
  //      ...
  //      ...
  //    } // and ends with a single line with only a single'}'
  
  // Parse cross bins
  if(noSpecifyBins){
    // Implictly create cross bins
    // under-construction
    cout << __FILE__ <<":" << __LINE__ << ": No user-specified bins ..." << endl;
  }
  else{
    while(1){
      d_coverFile.getline(cStr, Global::STR_LENGTH);
      assert(d_coverFile.gcount() < Global::STR_LENGTH);
      str = string(cStr);
      Global::eraseBlanks(&str);
      if(str[0] == '}') break;      
      // Combine multiple line specification
      while(str[str.size()-1] != ';'){
        d_coverFile.getline(cStr, Global::STR_LENGTH);
        assert(d_coverFile.gcount() < Global::STR_LENGTH);
        tempStr = string(cStr);
        Global::eraseBlanks(&tempStr);
        str += tempStr;
      }
      parseCrossBin(cross_p, &str);     
    }
  }
}

void ParseCover::parseCrossBin(Cross* cross_p, string* str_p){
  size_t    find;
  string    name;
  bool      isExcluded = false;  
  CrossBin* crossBin_p;
  
  if(str_p->compare(0, 4, "bins")==0){
    isExcluded = false;
    str_p->erase(0, 4);
  }
  if(str_p->compare(0, 7, "ignore_")==0){
    isExcluded = true;
    str_p->erase(0, 11);
  }
  else if(str_p->compare(0, 8, "illegal_")==0){
    isExcluded = true;
    str_p->erase(0, 12);
  }
  
  find = str_p->find_first_of('=');
  name = str_p->substr(0, find);
  str_p->erase(0, find+1);
  
  crossBin_p = new CrossBin(name, isExcluded, cross_p->getCrossList());
  crossBin_p->setSpec(str_p);
  // Add this bin to current cross
  cross_p->addBin(crossBin_p);
}

void ParseCover::recordValues(set<int>* multiValues, int maxValue, string* binStr_p){
  string subStr1, subStr2;
  size_t find1, find2;
  int l,h;
  
  if((find1=binStr_p->find_first_of('{'))!=string::npos)
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
        h = maxValue;
      }
      else{// [v(l):v(h)]
        l = Global::str2int(subStr1);
        h = Global::str2int(subStr2);
      }
      for(int i=l;i<=h;i++)
        multiValues->insert(i);      
      binStr_p->erase(0, find2+2);
    }// end a range of values
    else{// single value
      subStr1 = binStr_p->substr(0, find1);      
      multiValues->insert(Global::str2int(subStr1));
      binStr_p->erase(0, find1+1);
    }
  }// end while
}

void ParseCover::recordValues(vector<int>* multiValues, int maxValue, string* binStr_p){
  string subStr1, subStr2;
  size_t find1, find2;
  int l,h;
  
  if((find1=binStr_p->find_first_of('{'))!=string::npos)
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
        h = maxValue;
      }
      else{// [v(l):v(h)]
        l = Global::str2int(subStr1);
        h = Global::str2int(subStr2);
      }
      for(int i=l;i<=h;i++)
        multiValues->push_back(i);      
      binStr_p->erase(0, find2+2);
    }// end a range of values
    else{// single value
      subStr1 = binStr_p->substr(0, find1);      
      multiValues->push_back(Global::str2int(subStr1));
      binStr_p->erase(0, find1+1);
    }
  }// end while
}

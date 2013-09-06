#ifndef INCLUDED_PARSE_COVER
#define INCLUDED_PARSE_COVER

#include <fstream>
#include <vector>
#include <set>
#include "global.h"
#include "coverGroup.h"
#include "circuit.h"

class Variable;
class BaseBin;

using namespace std;

class ParseCover{
  private:
    // DATA
    Circuit*    d_circuit_p;
    CoverGroup* d_coverGroup_p;
    ifstream    d_coverFile;
    int*        d_cnfVar_p; // Only used for local variables
    
  public:
    // CREATORS
    ParseCover(Circuit*, CoverGroup*, char*, int*);
    
    // MANIPULATORS
    // MAIN PARSING FUNCTION
    void parse();
    
  private:
    // UTILITIES
    enum parseType { VARIABLE, COVERGROUP, COVERPOINT, CROSS, COVERBIN, CROSSBIN, OTHER };
    
    parseType parseTypeAnalysis (const string&);
    void      parseVariable     (string*);
    void      parseGroup        (string*);
    void      parsePoint        (string*);   
    void      parseBin          (CoverPoint*, string*, CoverVariable*);
    void      parseCross        (string*);
    void      parseCrossBin     (Cross*, string*);
    void      recordValues      (set<int>*, int, string*);
    void      recordValues      (vector<int>*, int, string*);
};

#endif

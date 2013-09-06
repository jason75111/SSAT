#ifndef INCLUDED_COVER_GROUP
#define INCLUDED_COVER_GROUP

#include <string>
#include <vector>
#include "coverVariable.h"
#include "coverPoint.h"
#include "cross.h"
//#include "circuit.h"

class Cross;
using namespace std;

class CoverGroup{
    // DATA
    string                  d_name;
    int                     d_cnfVar;
    vector<CoverVariable*>  d_variableList;
    vector<CoverPoint*>     d_coverPointList;
    vector<Cross*>          d_crossList;
      
    public:
        // CREATORS
        CoverGroup();
        ~CoverGroup();
        
        // MANIPULATORS
        void setName(const string&);
        void setIdx(const int&);
        void setCnfVar(const int&);
        void addVariable(CoverVariable*);
        void addCoverPoint(CoverPoint*);
        void addCross(Cross*);
        
        // ACCESSORS
        string          getName() const;
        int             getCnfVar() const;
        BaseBin*        getBin(string) const;
        CoverVariable*  getVariable(const string&) const;
        CoverVariable*  getVariable(const int&) const;
        CoverPoint*     getCoverPoint(const string&) const;
        CoverPoint*     getCoverPoint(const int&) const;
        CoverPoint*     getLastCoverPoint() const;
        Cross*          getCross(const string&) const;
        Cross*          getCross(const int&) const;
        Cross*          getLastCross() const;

        int             numOfVariable() const;
        int             numOfCoverPoint() const;
        int             numOfCross() const;
        int             numOfBin() const;
        
        // Debug function
        void printVariableInfo();
        void printPointInfo();
        void printCrossInfo();
};

#endif

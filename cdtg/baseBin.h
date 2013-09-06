#ifndef INCLUDED_BASEBIN
#define INCLUDED_BASEBIN

#include <string>
#include <vector>
#include <iostream>

#include "coverVariable.h"

using namespace std;

class BaseBin{
    public:
        // CREATORS
        virtual ~BaseBin(){};
        
        // ENUM_TYPE DEFINITION
        enum binType {SINGLE, AUTO_MULTI, FIXED_MULTI};
      
        // MANIPULATORS
        virtual void    setSpec   (string*)=0;
        virtual void    setCnfVar (const int)=0;
        virtual void    genCnf    (int*)=0;
        virtual void    plusCount ()=0;
        
        // ACCESSORS
        virtual const string&       getName      () const=0;
        virtual const int&          getCnfVar    () const=0;
        virtual const int&          getHitCount  () const=0;
        virtual void                print        () const=0;
        virtual void                printClause  () const=0;
        virtual const vector<int>&  getCNFClause () const=0;
        
    protected:
      // DATA
      string            d_name;
      int               d_cnfVar;
      CoverVariable*    d_var_p;
      bool              d_isExcluded; // ignored or illegal bins
      binType           d_type;
      int               d_hitCount;
      vector<int>       d_cnfClause;
};

#endif

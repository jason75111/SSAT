#ifndef INCLUDED_CROSS
#define INCLUDED_CROSS

#include "global.h"
#include "coverVariable.h"
#include "coverGroup.h"
#include "coverPoint.h"
#include "crossBin.h"
#include "stateBin.h"

#include <string>
#include <vector>

class CoverGroup;

using namespace std;

class Cross{
    // DATA
    string                  d_name;
    int                     d_cnfVar;
    CoverGroup*             d_belongGroup_p;
    int                     d_crossSize;
    vector<CoverPoint*>     d_crossList;
    vector<CrossBin*>       d_binList;
    
    public:
        // CREATORS
        Cross(const string&, CoverGroup*);
        ~Cross();
        
        // MANIPULATORS     
        void    setSpec(string*);
        void    addBin(CrossBin*);
        void    setCnfVar(const int&);
        
        // ACCESSORS
        const string&           getName      ()           const;
        const int&              getCnfVar    ()           const;
        vector<CoverPoint*>     getCrossList ()           const;
        BaseBin*                getBin       (const int&) const;
        const int               numOfBin     ()           const;
};

#endif


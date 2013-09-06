#ifndef INCLUDED_COVER_VARIABLE
#define INCLUDED_COVER_VARIABLE

#include <string>
#include <vector>

using namespace std;

class CoverVariable{
    public:
        enum varType {PI, PPI, LOCAL};
    
    private:
        // DATA
        string           d_name;
        int              d_bitwidth;
        varType          d_type;
        vector<long int> d_valueList;
        vector<int>      d_cnfVarList;        // 0, 1, ... , M-1 (assume M bit)
        vector<int>      d_cnfVarList_origin; // used to keep time 0 CNF var, should not be modified
    
    public:
        // CREATORS
        CoverVariable(string, varType, int);
        
        // MANIPULATORS
        void setCnfVar   (int, int);
        void setCnfVar   (int*);
        void incrCnfVar  (int);                    // used for time frame expansion
        void resetCnfVar ();                       // used for non-incremental SAT to reset CNF var to time 0
        void setValue    (int t, long int value);
        
        // ACCESSORS
        string              getName          ()    const;
        int                 getBitwidth      ()    const;
        long int            getValue         (int) const;
        int                 getCnfVar        (int) const;
        
        vector<int>         getCnfVarList    (int msb, int lsb);
        const vector<int>&  getCnfVarList    ();
        
        void                checkConsistence ();
    
};

#endif

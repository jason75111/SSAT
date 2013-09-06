#ifndef INCLUDED_STATE_BIN
#define INCLUDED_STATE_BIN

#include <string>
#include <vector>
#include <set>
#include <boost/icl/interval_set.hpp>
#include "baseBin.h"

using namespace std;
using namespace boost::icl;

class StateBin : public BaseBin{
    // DATA
    
    // Inherited from BaseBin:
    // string         d_name;
    // int            d_cnfVar;
    // CoverVariable* d_var_p;
    // bool           d_isExcluded;  // ignored or illegal bins
    // binType        d_type;
    // int            d_hitCount;
    // vector<int>    d_cnfClause;
    
    int               d_msb;
    int               d_lsb;
    int               d_maxValue;     // The maximum value could be specified
    interval_set<int> d_intervalSpec;
    
    public:
        // CREATORS
        StateBin(const string& name, const bool& isExcluded, const binType& type,
                 const int& msb, const int&  lsb, const int& maxValue, 
                 CoverVariable* var_p);
        
        // Following 2 constructors are for auto bin vector
        StateBin(const int& value, const int& msb, const int& lsb, 
                 const int& maxValue, CoverVariable* var_p);
        StateBin(const int& lVal, const int& hVal, const int& msb, const int& lsb, 
                 const int& maxValue, CoverVariable* var_p);
        
        // Following 2 constructors are for fixed bin vector
        StateBin(const string& name, const int& value, const int& msb, const int& lsb, 
                 const int& maxValue, CoverVariable* var_p);
                 
        StateBin(const string& name, const int& lVal, const int& hVal, 
                 const set<int>& multiValues, const int& msb, const int& lsb, 
                 const int& maxValue, CoverVariable* var_p);
        
        // MANIPULATORS
        // The specString specifies a range of values
        // for example: "{0,2,[7:20],[100:$]}";
        void    setSpec     (string* specString);
        void    setCnfVar   (const int cnfVar);
        void    setMSB      (int msb);
        void    setLSB      (int lsb);
        void    genCnf      (int* cnfVar);
        void    plusCount   ();
        
        // ACCESSORS
        const string&      getName                  ()    const;
        const int&         getCnfVar                ()    const;
        const int&         getHitCount              ()    const;
        void               print                    ()    const;
        void               printClause              ()    const;
        const vector<int>& getCNFClause             ()    const;
                      
        const bool         hasValue                 (int) const;
        const bool         hasValueLessOrEqualTo    (int) const;
        const bool         hasValueGreaterOrEqualTo (int) const;
    
    private:
        // CLAUSE GENERATION FUNCTIONS
        void    genEqualCnf   (const vector<int>, const int&, const int&);
        void    genRangeCnf   (const vector<int>, const int&, const int&, int*);
        void    genLessCnf    (const vector<int>&, const vector<int>&, const int, vector<int>*);
        void    genLargerCnf  (const vector<int>&, const vector<int>&, const int, vector<int>*);
        void    clauseUnion   (int, vector<int>*);
        void    clauseDistrib (int, vector<int>*);
        void    SopToPos      (vector<int>*, int*);
        void    genOrCnf      (const vector<int>&, int, vector<int>*);
        void    genOrCnf      (const vector<int>&, int);
        void    genAndCnf     (const vector<int>&, int, vector<int>*);
        void    genAndCnf     (const vector<int>&, int);
        void    genAnd2Cnf    (const int&,const int&, const int&);
        void    int2bitvector (int, vector<int>*);
        int     bitvector2int (const int&, const vector<int>&);
};


#endif

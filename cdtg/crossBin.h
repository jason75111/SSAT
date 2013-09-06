#ifndef INCLUDED_CROSS_BIN
#define INCLUDED_CROSS_BIN

#include "baseBin.h"
#include "stateBin.h"
#include "coverPoint.h"
#include <string>
#include <vector>
#include <boost/icl/interval_set.hpp>

using namespace std;
using namespace boost::icl;

class CrossBin : public BaseBin{
    // DATA
      
    // Inherited from BaseBin
    // string                 d_name;
    // int                    d_cnfVar;
    // CoverVariable*         d_var_p;
    // bool                   d_isExcluded;         // ignored or illegal bins
    // binType                d_type;
    // int                    d_hitCount;
    // vector<int>            d_cnfClause;
    
    string                    d_selectSpec;
    vector<CoverPoint*>       d_crossList;          // Coverpoints in the cross list
    vector<vector<BaseBin*> > d_crossProductList;

    public:
        // CREATORS
        CrossBin(const string&, const bool&, vector<CoverPoint*>);
        ~CrossBin();
        
        // MANIPULATORS
        void setSpec(string* specString);
        void setCnfVar(const int cnfVar);
        void addCrossProduct(vector<BaseBin*> crossProduct);
        void genCnf(int*);
        void plusCount();
        
        // ACCESSORS
        const string&       getName      () const;
        const int&          getCnfVar    () const;
        const int&          getHitCount  () const;
        void                print        () const;
        void                printClause  () const;
        const vector<int>&  getCNFClause () const;
        const string&       getSelecSpec () const;
        
    private:
        // UTILITIES
        void selectBins(const bool&, string*, vector<vector<BaseBin*> >*);
        void genCrossProducts(vector<vector<BaseBin*> >*);
        
        // CLAUSE GENERATION FUNCTIONS
        void genAndCnf(const vector<int>&,int);
        void genOrCnf(const vector<int>&,int);
};

#endif

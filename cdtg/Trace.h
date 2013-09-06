#ifndef INCLUDED_TRACE
#define INCLUDED_TRACE

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "global.h"
#include "coverGroup.h"
#include "circuit.h"

#include "core/Solver.h"
#include "utils/System.h"
#include "utils/Options.h"

using namespace std;
using namespace Minisat;

class Trace{
    private:
        // DATA
        Circuit&                      d_circuit;
        CoverGroup*                   d_coverGroup_p;
        int*                          d_dimacsCnfVar_p;   // DIMACS CNF variable = (solver CNF variable + 1)
        const BoolOption&             d_group;            // Group bins by coverpoint
        const IntOption&              d_randGroup;        // Group bins randomly
        const BoolOption&             d_nR;               // Negative reset
        const IntOption&              d_covThr;           // Cover threshould
        const IntOption&              d_maxDepth;         // Maximum time frame
        Solver                        d_solver;
        Solver*                       d_solver_p;         // used for non-incremental SAT
        
        ifstream                      d_inUncoverBinFile;
                                     
        vector<BaseBin*>              d_uncoverBinList;
        vector<vector<BaseBin*> >     d_uncoverGroupBinList;
        vector<int>                   d_initialVar;       // d_initialVar[t] gives the first variable of frame t
        vector<map<BaseBin*, int> >   d_bin2var;          // bin2var[t][b] gives the variable of bin b in frame t
        map<int, BaseBin*>            d_var2bin;          // var2bin[v] gives the bin of variable v
                                     
        vector<int>                   d_incrSATConstr;    // d_incrSATConstr[t] gives the auxilary variable of incremental SAT solving of frame t
        vector<vector<int> >          d_allSATConstr;     // d_allSATConstr[t][i] gives the auxilary variable i of all SAT solving of frame t, 
                                                          //   used by addAllSATConstraint(int frame)
        vector<vector<int> >          d_groupSATConstr;
        vector<vector<vector<int> > > d_patterns;         // d_patterns[n][t] gives the n-th (PI) pattern of frame t, 
                                                          //   used by addAllSATConstraint_noIncrSAT (int frame);
    
    public:
        // CREATORS
        Trace(Circuit&, CoverGroup*, int*, const BoolOption&, const IntOption&, const BoolOption&, const IntOption&, const IntOption&);
  
        // MAIN FUNCTIONS
        void tpg           ();
        void tpg_grouped   ();
        void tpg_mspsat    ();
        void tpg_single    ();  // Note: don't use this with tpg() at the same Main()
        void tpg_noIncrSAT ();  // Note: don't use this with tpg() at the same Main()
    
    private:
        // UTILITIES
        // Incremental SAT solving functions, used by tpg()
        void insert                        (int frame);
        void addSpecConstraint             (int frame);
        void setBinCNF                     (int frame);
        void analyzeSolvedBins             (int frame);
        void addAllSATConstraint           (int frame);
        
        // Non-incremental SAT solving functions, used by tpg_noIncrSAT()
        void insert_noIncrSAT              (int frame, int maxframe);
        void addSpecConstraint_noIncrSAT   (int frame);
        void setBinCNF_noIncrSAT           (int frame);
        void analyzeSolvedBins_noIncrSAT   (int frame);
        void addAllSATConstraint_noIncrSAT (int frame);
        void recordPatterns                (int frame);
        
        // Single SAT solving functions, used by tpg_single()
        void insert_single                 (int frame, int binIdx);
        void setBinCNF_single              (int frame, int binIdx);
        
        // Utility functions
        void parseBinsToBeSolved           ();
        long bitvector2int                 (const int&, const vector<int>&);
        void genPatterns                   (int frame);
        void printStats                    ();
        void printStats_                   ();
};

#endif


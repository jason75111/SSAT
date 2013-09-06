#ifndef INCLUDED_COVER_POINT
#define INCLUDED_COVER_POINT

#include "baseBin.h"
#include "coverVariable.h"
#include<string>
#include<vector>

using namespace std;

class CoverPoint{
    // DATA
	  string           d_name;
	  int              d_cnfVar;
	  int              d_msb;
    int              d_lsb;
	  int              d_auto_bin_max; // Maximun # of automatically created bins
	  CoverVariable*   d_var_p;
	    // A cover point may sample a set of variables
	    // for example: "coverpoint v_a + v_b" samples variables v_a and v_b
	    // but we currently assume a single variable is sampled
	  vector<BaseBin*> d_binList;
	  
	public:
		// CREATORS
		CoverPoint(string name, int msb, int lsb, int AUTO_BIN_MAX, CoverVariable* var_p);
		~CoverPoint();
		
		// MANIPULATORS		
		void           addBin(BaseBin*);
		void           setCnfVar(const int&);
		
		// ACCESSORS
		const string&  getName()          const;
		const int&     getCnfVar()        const;
		const int&     getMSB()           const;
		const int&     getLSB()           const;
		const int      getAutoBinMax()    const;
		const int      getMaxVarValue()   const;
		BaseBin*       getBin(const int&) const;
		const int      numOfBin()         const;
		CoverVariable* getVar()           const;
};


#endif

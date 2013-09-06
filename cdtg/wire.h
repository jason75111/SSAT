#ifndef INCLUDED_WIRE
#define INCLUDED_WIRE

#include <string>
#include <vector>

class Gate;

using namespace std;

class Wire{
    public:
        enum wireType{ PI, PO, PPI, PPO, NORMAL, CLK, RST };
      
    private:
        // DATA
        string          d_name;
        wireType        d_type;
        int             d_cnfVar;     // CNF variable of SAT instance
        Gate*           d_sourceGate;
        vector<Gate*>   d_destGateList;
    
    public:
        // CREATORS
        Wire(string, wireType, int);
        //~Wire();
        
        // ACCESSORS
        string   getName();
        wireType getType();
        void     printType();
        int      getCnfVar();
        int      fanoutSize();
        Gate*    getSourceGate();
        Gate*    getDestGate(int);
        
        // MANIPULATORS
        void     setWireType(wireType);
        void     setSourceGate(Gate*);
        void     addDestGate(Gate*);
};

#endif

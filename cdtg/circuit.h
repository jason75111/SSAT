#ifndef INCLUDED_CIRCUIT
#define INCLUDED_CIRCUIT

#include <string>
#include <vector>
#include <map>
#include "gate.h"
#include "wire.h"

using   namespace   std;

class Circuit{
    private:
        // DATA
        string              d_name;
        vector<Gate*>       d_gateList;
        vector<Gate*>       d_dffList;
        vector<Wire*>       d_wireList;
        vector<Wire*>       d_piList;
        vector<Wire*>       d_poList;
        vector<Wire*>       d_ppiList;
        vector<Wire*>       d_ppoList;
        vector<Wire*>       d_clkList;
        vector<Wire*>       d_rstList;
        map<string, Wire*>  d_str2PI;
        map<string, Wire*>  d_str2PPO;
        
    public:
        // CREATORS
        Circuit();
        ~Circuit();
        
        // MANIPULATORS
        void    addGate         (Gate*);
        void    addWire         (Wire*);
        void    classifyWire    ();
        void    setName         (string);
        void    setCNF          ();
                
        // ACCESSORS
        string getName();
        
        // Get gates and wires by   indexing
        Gate*   getGate(int);
        Gate*   getDFF(int);
        Wire*   getWire(int);
        Wire*   getPI(int);
        Wire*   getPI(string);
        Wire*   getPO(int);
        Wire*   getPPI(int);
        Wire*   getPPO(int);
        Wire*   getPPO(string);
        Wire*   getCLK(int);
        Wire*   getRST(int);
        
        int     gateListSize();
        int     dffListSize();
        int     wireListSize();
        int     PIListSize();
        int     POListSize();
        int     PPIListSize();
        int     PPOListSize();
        int     CLKListSize();
        int     RSTListSize();
        
        // Print functions
        void    printWireInfo();
        void    printGateInfo();
        
    private:
        // For CNF initialization
        void    genBUFCNF   (Gate*);
        void    genINVCNF   (Gate*);
        void    genAND2CNF  (Gate*);
        void    genAND3CNF  (Gate*);
        void    genOR2CNF   (Gate*);
        void    genOR3CNF   (Gate*);
        void    genNAND2CNF (Gate*);
        void    genNAND3CNF (Gate*);
        void    genNAND4CNF (Gate*);
        void    genNOR2CNF  (Gate*);
        void    genNOR3CNF  (Gate*);
        void    genXOR2CNF  (Gate*);
        void    genXNOR2CNF (Gate*);
        void    genMX2CNF   (Gate*);  // (S0 B) | ((!S0) A)
        void    genOAI21CNF (Gate*);  // !((A0 | A1) B0)
        void    genAOI21CNF (Gate*);  // !((A0 A1) | B0)
        void    genCONST0CNF(Gate*);
        void    genCONST1CNF(Gate*);
};


#endif

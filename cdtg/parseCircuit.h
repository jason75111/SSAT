#ifndef INCLUDED_PARSE_CIRCUIT
#define INCLUDED_PARSE_CIRCUIT

#include "global.h"
#include "circuit.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>


using namespace std;

class ParseCircuit{
    private:
        // DATA
        Circuit*                    d_circuit_p;
        ifstream                    d_circuitFile;
        int*                        d_cnfVar_p; // Used for wires, increase variable before set
        map<string, Gate::gateType> d_gateTypeMap;
        
    public:
        // CREATORS
        ParseCircuit(Circuit*, char*, int*);
        
        // MANIPULATORS
        // MAIN PARSING FUNCTION
        void parse();
      
    private:
        // UTILITIES
        void            initializeGateTypeMap ();
        void            formalizeGateLine     (string*);
        void            parseWires            (string);
        void            parseGates            (string);
        void            createInoutGates      ();
        void            gateConnection        ();
        Gate::gateType  getGateType           (string);
        string          getWireName           (Gate::gateType, string*, char*, bool*);
};

#endif

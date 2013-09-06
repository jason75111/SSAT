#ifndef INCLUDED_GATE
#define INCLUDED_GATE

#include <string>
#include <vector>

class Wire;

using namespace std;

class Gate{
  public:
	enum gateType{ 
			BUF, INV, AND2, AND3, OR2, OR3, NAND2, NAND3, NAND4,
			OAI21, AOI21,
			NOR2, NOR3, XOR2, XNOR2, MX2, DFF, INPUT, OUTPUT, 
			CONST0, CONST1, UNKNOWN};
			// BUF: BUF, CLKBUF
			// DFF: DFFRHQ, DFFHQ, DFFS, DFFSR, DFFR, DFF, DFFTR
			
			// To add a new gate, also need to set 
			//   1. ParseCircuit::initializeGateTypeMap()
			//   2. Circuit::setCNF()
			//   3. Add new Circuit::genCNF() of new gate type
			//   4. Gate::printType()
		
  private:
   // DATA
    gateType      d_type;
    string        d_utilityString;
    int           d_id;
    //bool          d_hasClkOrRst;
    vector<Wire*> d_outputWireList;
    vector<Wire*> d_inputWireList;
    vector<char>  d_dffOutputPortList;  // Q | N(QN), used to track position of Q or QN
    vector<int>   d_cnfClause;
    
  public:
    // CREATORS
    Gate(gateType, string, int);
    Gate(gateType, int);
    //~Gate();
    
    // ACCESSORS
    string   getUtilityStr();
    int      getId();
    //bool     getHasClkOrRst();
    gateType getType();
    void     printType();
    int      outputSize();
    int      inputSize();
    int      dffOutSize();
    Wire*    getOutWire(int);
    Wire*    getInWire(int);
    char     getDFFPort(int);
    
    // CNF functions
    vector<int>& getCNFClause();
    void         printCNFClause();
    
    // MANIPULATORS
    //void setHasClkOrRst (bool);
    void addOutWire     (Wire*);
    void addInWire      (Wire*);
    void addDFFOutPort  (char);
    void addCNFClause   (int);
    void addCNFClause   (int, int);
    void addCNFClause   (int, int, int);
    void addCNFClause   (int, int, int, int);
    void addCNFClause   (int, int, int, int, int);
};

#endif

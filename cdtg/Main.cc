#include "parseCircuit.h"
#include "parseCover.h"
#include "Trace.h"
#include "utils/Options.h"

using namespace std;
using namespace Minisat;

int main(int argc, char** argv){

  int var = 0;
  double initial_time, parse_time;
  
  /////////////////////////////////////
  // Command options
  
  BoolOption incrSAT   ("MAIN", "i", "Incremental or Non-incremental SAT (-i for incremental SAT).", true);
  BoolOption group     ("MAIN", "g", "Group bins.", false);
  IntOption  randGroup ("MAIN", "r", "Random group bins, speficfy the number of literals of each clause.", 1, IntRange(2, INT32_MAX));
  BoolOption mspsat    ("MAIN", "msp", "MSPSAT solving by FMCAD'10.", false);
  BoolOption single    ("MAIN", "s", "Single SAT solving.", false);
  BoolOption nR        ("MAIN", "n", "Negative reset option (-n for negative reset).", false);
  IntOption  covThr    ("MAIN", "t", "Bins covered threshold.", 0, IntRange(0, INT32_MAX));
  IntOption  maxDepth  ("MAIN", "d", "Maximum time frame.", 50, IntRange(0, INT32_MAX));
  
  parseOptions(argc, argv, true);
  if(argc != 3){
    cout << "Error!! The command format is: ./[program] [options] [circuit_file] [covergroup_file]" << endl;
    exit(1);
  }
  
  /////////////////////////////////////
  // Start parsing
  
  Circuit circuit;
  ParseCircuit circuitParser(&circuit, argv[1], &var);
  initial_time = cpuTime();
  circuitParser.parse();
  parse_time = cpuTime();
  cout << "Circuit parsing time = " << parse_time - initial_time << endl;
  
  CoverGroup coverGroup;
  ParseCover coverageParser(&circuit, &coverGroup, argv[2], &var);
  initial_time = cpuTime();
  coverageParser.parse();
  parse_time = cpuTime();
  cout << "Covergroup parsing time = " << parse_time - initial_time << endl;
  
  /////////////////////////////////////
  // Print circuit information
  
  cout << "Total " << circuit.gateListSize() << " gates" << endl;
  //circuit.printWireInfo();
  //circuit.printGateInfo();
  
  /////////////////////////////////////
  // Print covergroup information
  
  cout << "Total " << coverGroup.numOfBin() << " bins" << endl;
  //coverGroup.printVariableInfo();
  //coverGroup.printPointInfo();
  //coverGroup.printCrossInfo();
  
  ////////////////////////////////////////////
  // Coverage-driven Test Generation
  
  Trace trace(circuit, &coverGroup, &var, group, randGroup, nR, covThr, maxDepth);
  
  // Default        : -i
  // Group          : -g
  // Random Group-k : -g -r=k
  // MSP            : -msp
  // Single         : -s
  
  if(single){
  	trace.tpg_single();
  }
  else if(incrSAT){
  	if(!group)
  		if(mspsat)
  			trace.tpg_mspsat();
  		else
        trace.tpg();
    else // group or rand_group
  	  trace.tpg_grouped();
  }
  else{
  	trace.tpg_noIncrSAT();
  }
  
  return 0;
}


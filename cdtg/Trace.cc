#include "Trace.h"
#include <set>
#include <iomanip>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

using namespace std;
using namespace Minisat;


// CREATORS
Trace::Trace(Circuit& circuit, CoverGroup* coverGroup_p, int* cnfVar_p, const BoolOption& group, const IntOption& randGroup, const BoolOption& nR, const IntOption& covThr, const IntOption& maxDepth)
	       : d_circuit(circuit), d_group(group), d_randGroup(randGroup), d_nR(nR), d_covThr(covThr), d_maxDepth(maxDepth){
    d_coverGroup_p   = coverGroup_p;
    d_dimacsCnfVar_p = cnfVar_p;
    // Set first variable of frame 0 to 0
    d_initialVar.clear();
    d_initialVar.push_back(0);
    // Initialization
    d_incrSATConstr.clear();
    d_allSATConstr.resize(1);
    d_patterns.clear();
}

// MAIN FUNCTIONS
void Trace::tpg(){
  vec<Lit> lits, assumps;
  vector<BaseBin*> currentUncoverBinList;
  int modelCount = 0;
  //////////////////////////////////////////////////////
  // Parse uncoverd bins
  parseBinsToBeSolved();
  double initial_time = cpuTime();
  for(int depth = 0; d_uncoverBinList.size() > 0 && depth <= d_maxDepth; ++depth){
  	double current_time = cpuTime();
    cout << "Current depth1 & time : " << depth << " " << current_time - initial_time << endl;
    cout << "Current depth2 & unsolved bins : " << depth << " " << d_uncoverBinList.size() << endl;
    cout << "Current conflicts : " << d_solver.conflicts  << endl;
    insert(depth);
    if(depth == 0)
      printStats();
    if(d_covThr != 0)
      d_allSATConstr.resize(depth + 2);
    assumps.clear();
    assumps.push( ~mkLit( d_incrSATConstr.at(depth) ) );
    while(d_solver.solve(assumps)){ // Search current frame until UNSAT
    	++modelCount;
      // Record current pattern
      //genPatterns(depth);
      // Update & add clause for unsolved bins
      lits.clear();
      lits.push( mkLit( d_incrSATConstr.at(depth) ) );
      currentUncoverBinList.clear();
      for(int i = 0; i < d_uncoverBinList.size(); ++i){
        int var = d_bin2var.at(depth)[d_uncoverBinList.at(i)];
        if(d_solver.modelValue(var) == l_True){
        	//assumps.push( ~mkLit(var) );
          d_uncoverBinList.at(i)->plusCount();
          if(d_covThr != 0 && d_uncoverBinList.at(i)->getHitCount() < d_covThr){
            lits.push( mkLit(var) );
            currentUncoverBinList.push_back(d_uncoverBinList.at(i));
          }
          cout << "**** Bin: " << d_uncoverBinList.at(i)->getName() << " hit at frame " << depth
               << ", solver var = " << var << ", CNF var = " << d_uncoverBinList.at(i)->getCnfVar() << endl;
        }
        else{
          lits.push( mkLit(var) );
          currentUncoverBinList.push_back(d_uncoverBinList.at(i));
        }
        
      }
      // Add new constraint clause if a bin is sufficientlly covered
      if(currentUncoverBinList.size() < d_uncoverBinList.size()){
        d_solver.addClause_(lits);
        d_uncoverBinList = currentUncoverBinList;
      }
      
      // Print value of variables
      analyzeSolvedBins(depth);
      
      // Add all SAT solving constraint 
      if(d_covThr != 0)    
        addAllSATConstraint(depth);
      
      // Change assumptions
      if(d_covThr != 0){
        assumps.clear();
        assumps.push( ~mkLit( d_incrSATConstr.at(depth) ) );
        for(int i = 0; i < d_allSATConstr.at(depth).size(); ++i)
          assumps.push( ~mkLit( d_allSATConstr.at(depth).at(i) ) );
      }
      // Terminate tpg() if all bins are covered
      if(d_uncoverBinList.size() == 0){
        cout << "All bins are covered ..." << endl;
        break;
      }
    }// End while

    //cout << "Conflict assumption clause: " << endl;
    //for(int i = 0; i < d_solver.conflict.size(); ++i){
    //   // cout << "Conflict bin = " << (d_var2bin[ toInt( d_solver.conflict[i] ) ])->getName()  << " ";
    //   int lit = toInt( d_solver.conflict[i] );
    //   if((lit%2)==0)
    //     cout << (lit/2) + 1 << " ";
    //   else
    //     cout << -((lit+1)/2) << " ";
    //   cout << endl;
    //} cout << endl;

    // Remove UNSAT constraint clause
    d_solver.addClause( mkLit( d_incrSATConstr.at(depth) ) );
    if(d_covThr != 0){
      for(int i = 0; i < d_allSATConstr.at(depth).size(); ++i)
        d_solver.addClause( mkLit( d_allSATConstr.at(depth).at(i) ) );
    }
  }// End for
  
  double tpg_time = cpuTime();
  cout << "Test pattern generation time = " << tpg_time - initial_time << endl;
  cout << "Number of models = " << modelCount << endl;
  if(d_uncoverBinList.size() != 0){
    cout << setfill('=') << setw(20); cout << "=" << endl;
    cout << "  Unsolved Bins :" << endl;
    for(int i = 0; i < d_uncoverBinList.size(); ++i){
      cout << "  ";
      cout << d_uncoverBinList.at(i)->getName() << endl;
    }
  }
  cout << "#### Ending status ####" << endl;
  printStats();
}

// Group or random group solving
void Trace::tpg_grouped(){
  vec<Lit> lits, assumps;
  vector<BaseBin*> currentUncoverBinList, currentUncoverGroup;
  int modelCount = 0;
  //////////////////////////////////////////////////////
  // Parse uncoverd bins
  parseBinsToBeSolved();
  double initial_time = cpuTime();
  for(int depth = 0; d_uncoverBinList.size() > 0 && depth <= d_maxDepth; ++depth){
    double current_time = cpuTime();
    cout << "Current depth1 & time : " << depth << " " << current_time - initial_time << endl;
    cout << "Current depth2 & unsolved bins : " << depth << " " << d_uncoverBinList.size() << endl;
    cout << "Current conflicts : " << d_solver.conflicts  << endl;
    insert(depth);
    if(depth == 0)
      printStats();
    if(d_covThr != 0) { d_allSATConstr.resize(depth + 2); }
    assumps.clear();
    assumps.push( ~mkLit( d_groupSATConstr.at(depth).at(0) ) );
    while(d_solver.solve(assumps)){ // Search current frame until UNSAT
      ++modelCount;
      // Record current pattern
      //genPatterns(depth);
      // Update & add clause for unsolved bins
      currentUncoverBinList.clear();
      for(int i = 0; i < d_uncoverGroupBinList.size(); ++i){
      	currentUncoverGroup.clear();
      	lits.clear();
      	//lits.push( ~mkLit(d_groupSATConstr.at(depth).at(i + 1)) );
      	int var = d_groupSATConstr.at(depth).at(i + 1);
      	//if(d_solver.modelValue(var) == l_True){ // a group is satisfied
      		lits.push( ~mkLit(var) );
      		for(int j = 0; j < d_uncoverGroupBinList.at(i).size(); ++j){
            var = d_bin2var.at(depth)[d_uncoverGroupBinList.at(i).at(j)];
            if(d_solver.modelValue(var) == l_True){
              d_uncoverGroupBinList.at(i).at(j)->plusCount();
              if(d_covThr != 0 && d_uncoverGroupBinList.at(i).at(j)->getHitCount() < d_covThr){
                lits.push( mkLit(var) );
                currentUncoverGroup.push_back(d_uncoverGroupBinList.at(i).at(j));
                currentUncoverBinList.push_back(d_uncoverGroupBinList.at(i).at(j));
              }
              cout << "**** Bin: " << d_uncoverGroupBinList.at(i).at(j)->getName() << " hit at frame " << depth
                   << ", solver var = " << var << ", CNF var = " << d_uncoverGroupBinList.at(i).at(j)->getCnfVar() << endl;
            }
            else{
              lits.push( mkLit(var) );
              currentUncoverGroup.push_back(d_uncoverGroupBinList.at(i).at(j));
              currentUncoverBinList.push_back(d_uncoverGroupBinList.at(i).at(j));
            }
          }
          // Add new constraint clause if a bin is sufficientlly covered
          if(currentUncoverGroup.size() < d_uncoverGroupBinList.at(i).size()){
          	d_solver.addClause_(lits);
          	d_uncoverGroupBinList.at(i) = currentUncoverGroup;
          }
      	//}
      	//else{
      	//	for(int j = 0; j < d_uncoverGroupBinList.at(i).size(); ++j)
      	//		currentUncoverBinList.push_back(d_uncoverGroupBinList.at(i).at(j));
      	//}
      }
      
      // Update total uncoverBinList
      if(currentUncoverBinList.size() < d_uncoverBinList.size()){
        d_uncoverBinList = currentUncoverBinList;
      }
      
      // Print value of variables
      analyzeSolvedBins(depth);
      
      // Add all SAT solving constraint 
      if(d_covThr != 0)    
        addAllSATConstraint(depth);
      
      // Change assumptions
      if(d_covThr != 0){
        assumps.clear();
        assumps.push( ~mkLit( d_groupSATConstr.at(depth).at(0) ) );
        for(int i = 0; i < d_allSATConstr.at(depth).size(); ++i)
          assumps.push( ~mkLit( d_allSATConstr.at(depth).at(i) ) );
      }
      // Terminate tpg() if all bins are covered
      if(d_uncoverBinList.size() == 0){
        cout << "All bins are covered ..." << endl;
        break;
      }
    }// End while

    // Remove UNSAT constraint clause
    d_solver.addClause( mkLit( d_groupSATConstr.at(depth).at(0) ) );
    //for(int i = 0; i < d_uncoverGroupBinList.size(); ++i)
    //  d_solver.addClause( ~mkLit( d_groupSATConstr.at(depth).at(i+1) ) );
    if(d_covThr != 0){
      for(int i = 0; i < d_allSATConstr.at(depth).size(); ++i)
        d_solver.addClause( mkLit( d_allSATConstr.at(depth).at(i) ) );
    }
  }// End for
  
  double tpg_time = cpuTime();
  cout << "Test pattern generation time = " << tpg_time - initial_time << endl;
  cout << "Number of models = " << modelCount << endl;
  if(d_uncoverBinList.size() != 0){
    cout << setfill('=') << setw(20); cout << "=" << endl;
    cout << "  Unsolved Bins :" << endl;
    for(int i = 0; i < d_uncoverBinList.size(); ++i){
      cout << "  ";
      cout << d_uncoverBinList.at(i)->getName() << endl;
    }
  }
  cout << "#### Ending status ####" << endl;
  printStats();
}

// MSPSAT algorithm
void Trace::tpg_mspsat(){
  vec<Lit> lits, assumps;
  vector<BaseBin*> currentUncoverBinList;
  vector<BaseBin*> currentUncheckBinList, newUncheckBinList;
  int modelCount = 0;
  //////////////////////////////////////////////////////
  // Parse uncoverd bins
  parseBinsToBeSolved();
  double initial_time = cpuTime();
  for(int depth = 0; d_uncoverBinList.size() > 0 && depth <= d_maxDepth; ++depth){
    double current_time = cpuTime();
    cout << "Current depth1 & time : " << depth << " " << current_time - initial_time << endl;
    cout << "Current depth2 & unsolved bins : " << depth << " " << d_uncoverBinList.size() << endl;
    cout << "Current conflicts : " << d_solver.conflicts  << endl;
    insert(depth);
    if(depth == 0)
      printStats();
    if(d_covThr != 0)
      d_allSATConstr.resize(depth + 2);
    
    currentUncheckBinList = d_uncoverBinList;
    while(currentUncheckBinList.size() != 0){
      // Randomly permutation for bin list
      srand (time(NULL));
      for(int i = 0; i < currentUncheckBinList.size()*3; ++i){
  	  	int b1 = rand() % currentUncheckBinList.size();
  	  	int b2 = rand() % currentUncheckBinList.size();
  	  	BaseBin* temp = currentUncheckBinList.at(b1);
  	  	currentUncheckBinList.at(b1) = currentUncheckBinList.at(b2);
  	  	currentUncheckBinList.at(b2) = temp;
  	  }
    	assumps.clear();
    	// choose last bin for ease of deletion
    	int var = d_bin2var.at(depth)[currentUncheckBinList.back()];
    	assumps.push( mkLit(var) );
    	if(d_solver.solve(assumps)){// assumption satisfied, one or multiple bins solved
    	  ++modelCount;
    	  // Record current pattern
    	  //genPatterns(depth);
    	  
    	  // Update check bin list
    	  newUncheckBinList.clear();
        for(int i = 0; i < currentUncheckBinList.size(); ++i){
          int var = d_bin2var.at(depth)[currentUncheckBinList.at(i)];
          if(d_solver.modelValue(var) == l_True){
            currentUncheckBinList.at(i)->plusCount();
            if(d_covThr != 0 && currentUncheckBinList.at(i)->getHitCount() < d_covThr){
              newUncheckBinList.push_back(currentUncheckBinList.at(i));
            }
            cout << "**** Bin: " << currentUncheckBinList.at(i)->getName() << " hit at frame " << depth
                 << ", solver var = " << var << ", CNF var = " << currentUncheckBinList.at(i)->getCnfVar() << endl;
          }
          else{
            newUncheckBinList.push_back(currentUncheckBinList.at(i));
          }
        }
        if(newUncheckBinList.size() < currentUncheckBinList.size())
          currentUncheckBinList = newUncheckBinList;
        
        // Update uncover bin list
        currentUncoverBinList.clear();
        for(int i = 0; i < d_uncoverBinList.size(); ++i){
          int var = d_bin2var.at(depth)[d_uncoverBinList.at(i)];
          if(d_solver.modelValue(var) == l_True){
          	//assumps.push( ~mkLit(var) );
            //d_uncoverBinList.at(i)->plusCount(); ******** don't count again
            if(d_covThr != 0 && d_uncoverBinList.at(i)->getHitCount() < d_covThr){
              currentUncoverBinList.push_back(d_uncoverBinList.at(i));
            }
            //cout << "**** Bin: " << d_uncoverBinList.at(i)->getName() << " hit at frame " << depth
            //     << ", solver var = " << var << ", CNF var = " << d_uncoverBinList.at(i)->getCnfVar() << endl;
          }
          else{
            currentUncoverBinList.push_back(d_uncoverBinList.at(i));
          }
        }
        if(currentUncoverBinList.size() < d_uncoverBinList.size())
          d_uncoverBinList = currentUncoverBinList;
        
        // Print value of variables
        analyzeSolvedBins(depth);
        
        // Add all SAT solving constraint 
        if(d_covThr != 0)    
          addAllSATConstraint(depth);
        
        // Change assumptions
        if(d_covThr != 0){
          assumps.clear();
          assumps.push( ~mkLit( d_incrSATConstr.at(depth) ) );
          for(int i = 0; i < d_allSATConstr.at(depth).size(); ++i)
            assumps.push( ~mkLit( d_allSATConstr.at(depth).at(i) ) );
        }
        // Terminate tpg() if all bins are covered
        if(d_uncoverBinList.size() == 0){
          cout << "All bins are covered ..." << endl;
          break;
        }
      }
      else{ // assumption failed, the assuming bins cannot be solved
      	// remove the choosed bin
      	currentUncheckBinList.pop_back();
      }
    }

    // Remove UNSAT constraint clause
    if(d_covThr != 0){
      for(int i = 0; i < d_allSATConstr.at(depth).size(); ++i)
        d_solver.addClause( mkLit( d_allSATConstr.at(depth).at(i) ) );
    }
  }// End for
  
  double tpg_time = cpuTime();
  cout << "Test pattern generation time = " << tpg_time - initial_time << endl;
  cout << "Number of models = " << modelCount << endl;
  if(d_uncoverBinList.size() != 0){
    cout << setfill('=') << setw(20); cout << "=" << endl;
    cout << "  Unsolved Bins :" << endl;
    for(int i = 0; i < d_uncoverBinList.size(); ++i){
      cout << "  ";
      cout << d_uncoverBinList.at(i)->getName() << endl;
    }
  }
  cout << "#### Ending status ####" << endl;
  printStats();
}

// Solve multiple bins simultaneously, but discard all learned clauses once gets a model
void Trace::tpg_noIncrSAT(){
  vec<Lit> lits;
  vector<BaseBin*> currentUncoverBinList;
  d_patterns.clear();
  int modelCount = 0;
  long int conflicts = 0;
  //////////////////////////////////////////////////////
  // Parse uncoverd bins
  parseBinsToBeSolved();
  double initial_time = cpuTime();
  for(int depth = 0; d_uncoverBinList.size() > 0 && depth <= d_maxDepth; ++depth){
    double current_time = cpuTime();
    cout << "Current depth & time : " << depth << " " << current_time - initial_time<< endl;
    for(;;){
      ////////////////////////////////////////////////////////
      // Create a new SAT instance of 0 to current depth
      d_solver_p = new Solver;
      for(int t = 0; t <= depth; ++t){
        insert_noIncrSAT(t, depth);
      }
      if(depth == 0)
      	printStats_();
      // Add all SAT solving constraint
      if(d_covThr != 0)
        addAllSATConstraint_noIncrSAT(depth);

      ////////////////////////////////////////////////////////
      //  If SAT, create a new SAT instance with same depth
      //  to keep searching uncovered bins,
      //  else goto next depth
      
      if(d_solver_p->solve()){
      	// Update statistics
      	++modelCount;
      	conflicts += d_solver_p->conflicts;
        // Update unsolved bins
        currentUncoverBinList.clear();
        for(int i = 0; i < d_uncoverBinList.size(); ++i){
          int var = d_bin2var.at(depth)[d_uncoverBinList.at(i)];
          if(d_solver_p->modelValue(var) == l_True){
            d_uncoverBinList.at(i)->plusCount();
            if(d_covThr != 0 && d_uncoverBinList.at(i)->getHitCount() < d_covThr){
              currentUncoverBinList.push_back(d_uncoverBinList.at(i));
            }
            cout << "**** Bin: " << d_uncoverBinList.at(i)->getName() << " hit at frame " << depth
                 << ", solver var = " << var << ", CNF var = " << d_uncoverBinList.at(i)->getCnfVar() << endl;
          }
          else{
            currentUncoverBinList.push_back(d_uncoverBinList.at(i));
          }
        }
        if(currentUncoverBinList.size() < d_uncoverBinList.size())
          d_uncoverBinList = currentUncoverBinList;
        
        // Print value of variables
        analyzeSolvedBins_noIncrSAT(depth);
        
        // Record patterns for all SAT solving
        if(d_covThr != 0)
          recordPatterns(depth);
        
        delete d_solver_p;
      }
      else{
      	conflicts += d_solver_p->conflicts;
        d_patterns.clear();
        delete d_solver_p;
        break; // goto next frame
      }
    } // end for(;;)
  }
  double tpg_time = cpuTime();
  cout << "Test pattern generation time = " << tpg_time - initial_time << endl;
  cout << "#### Ending status ####" << endl;
  cout << "Number of models = " << modelCount << endl;
  cout << "Conflicts : " << conflicts  << endl;
}

// Solve bins one by one
void Trace::tpg_single(){
  vec<Lit> lits, assumps;
  vector<BaseBin*> currentUncoverBinList;
  int modelCount = 0;
  long int conflicts = 0;
  //////////////////////////////////////////////////////
  // Parse uncoverd bins
  parseBinsToBeSolved();
  double initial_time = cpuTime();
  
  for(int i = 0; i < d_uncoverBinList.size(); ++i){
  	cout << "Current bin = " << d_uncoverBinList.at(i)->getName() << endl;
    d_solver_p = new Solver;
    for(int depth = 0; depth <= d_maxDepth; ++depth){
      cout << "Current depth = " << depth << endl;
      insert_single(depth, i);
      int var = d_bin2var.at(depth)[d_uncoverBinList.at(i)];
      if(d_solver_p->solve( mkLit(var) )){
      	++modelCount;
      	conflicts += d_solver_p->conflicts;
      	cout << "**** Bin: " << d_uncoverBinList.at(i)->getName() << " hit at frame " << depth << endl;     
        // Print value of variables
        analyzeSolvedBins_noIncrSAT(depth); // Reused noIncrSAT function
        delete d_solver_p;
      	break; // Solve next bin
      }
      else if(depth == d_maxDepth){
      	currentUncoverBinList.push_back(d_uncoverBinList.at(i));
      	delete d_solver_p;
      	break; // Solve next bin
      }
    }// End for
  }
  
  double tpg_time = cpuTime();
  cout << "Test pattern generation time = " << tpg_time - initial_time << endl;
  cout << "Number of models = " << modelCount << endl;
  if(currentUncoverBinList.size() != 0){
    cout << setfill('=') << setw(20); cout << "=" << endl;
    cout << "  Unsolved Bins :" << endl;
    for(int i = 0; i < currentUncoverBinList.size(); ++i){
      cout << "  ";
      cout << currentUncoverBinList.at(i)->getName() << endl;
    }
  }
  cout << "#### Ending status ####" << endl;
  cout << "Number of models = " << modelCount << endl;
  cout << "Conflicts : " << conflicts  << endl;
}

// UTILITIES

////////////////////////////////////////////////////////////////////////////////
// Incremental SAT solving functions, used by tpg()
void Trace::insert(int frame){
  int parsedLit, var, dffPortSize, PPI, PPI_N, prePPO;
  char portChar;
  vec<Lit> lits;
  
  //////////////////////////////////////////////////////
  // Add clauses from circuit
  for(int i = 0; i < d_circuit.gateListSize(); ++i){
    lits.clear();
    const vector<int> gateClause = d_circuit.getGate(i)->getCNFClause();
    if(gateClause.size() > 0){
      for(int j = 0; j < gateClause.size(); ++j){
        parsedLit = gateClause.at(j);
        if(parsedLit == 0){
          d_solver.addClause_(lits);
          lits.clear();
          continue;
        }
        parsedLit = (gateClause.at(j) > 0) ? (gateClause.at(j) + d_initialVar.at(frame)) : (gateClause.at(j) - d_initialVar.at(frame));
        var = abs(parsedLit) - 1;
        while(var >= d_solver.nVars()) { d_solver.newVar(); }
        lits.push( (parsedLit > 0) ? mkLit(var) : ~mkLit(var) );
      }
    }
    else if(d_circuit.getGate(i)->getType() == Gate::DFF && frame >= 1){ // Insert clauses of flip-flops
      dffPortSize = d_circuit.getGate(i)->dffOutSize();
      portChar    = d_circuit.getGate(i)->getDFFPort(0);
      //cout << "DFF port size = " << dffPortSize << ", port char = " << portChar << endl;
      prePPO  = d_circuit.getGate(i)->getInWire(0)->getCnfVar() - 1 + d_initialVar.at(frame - 1);
      while(prePPO >= d_solver.nVars()) { d_solver.newVar(); }
      if(dffPortSize == 1 && portChar == 'Q'){
        PPI = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI >= d_solver.nVars()) { d_solver.newVar(); }
        // Connect PPI(t) to PPO(t-1)
        d_solver.addClause( ~mkLit(PPI),  mkLit(prePPO) );
        d_solver.addClause(  mkLit(PPI), ~mkLit(prePPO) );
        // cout << "c Connect PPI(t) to PPO(t-1)" << endl;
        // cout << -(PPI + 1) << " " <<  (prePPO + 1) << endl;
        // cout <<  (PPI + 1) << " " << -(prePPO + 1) << endl;
      }
      else if(dffPortSize == 1 && portChar == 'N'){
        PPI_N = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI_N >= d_solver.nVars()) { d_solver.newVar(); }
        // Connect PPI_N(t) to PPO(t-1)
        d_solver.addClause( ~mkLit(PPI_N), ~mkLit(prePPO) );
        d_solver.addClause(  mkLit(PPI_N),  mkLit(prePPO) );
      }
      else if(dffPortSize == 2){
        PPI = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI >= d_solver.nVars()) { d_solver.newVar(); }
        PPI_N = d_circuit.getGate(i)->getOutWire(1)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI_N >= d_solver.nVars()) { d_solver.newVar(); }
        // Connect PPI(t) to PPO(t-1)
        d_solver.addClause( ~mkLit(PPI),    mkLit(prePPO) );
        d_solver.addClause(  mkLit(PPI),   ~mkLit(prePPO) );
        // Connect PPI_N(t) to PPO(t-1)
        d_solver.addClause( ~mkLit(PPI_N), ~mkLit(prePPO) );
        d_solver.addClause(  mkLit(PPI_N),  mkLit(prePPO) );
      }
      else{
        cout << "Unknown DFF port condition." << endl;
        cout << "DFF port size = " << dffPortSize << ", port char = " << portChar << endl;
        exit(1);
      }
    }
  }
  
  //////////////////////////////////////////////////////
  // Add spec constraint clauses
  addSpecConstraint(frame);
  
  //////////////////////////////////////////////////////
  // Add clauses of uncovered bins
  setBinCNF(frame);
  
  // for(int i = 0; i < d_circuit.PPIListSize(); ++i){
  //   if(d_circuit.getPPI(i)->getName().substr(0, 5).compare("count") == 0){
  //     cout << d_circuit.getPPI(i)->getName() << ": " << d_circuit.getPPI(i)->getCnfVar() + d_initialVar.at(frame) << endl;
  //   }
  // }
  // CoverVariable* var_p = d_coverGroup_p->getVariable((string)"count");
  // for(int i = 0; i < var_p->getBitwidth(); ++i){
  //   cout << i << " :" << var_p->getCnfVar(i) << endl;
  // }
  
  for(int i = 0; i < d_uncoverBinList.size(); ++i){
    //cout << "CNF Clause of bin: " << d_uncoverBinList.at(i)->getName() << endl;
    const vector<int> binClause = d_uncoverBinList.at(i)->getCNFClause();
    lits.clear();
    for(int j = 0; j < binClause.size(); ++j){
      parsedLit = binClause.at(j);
      if(parsedLit == 0){
        // for(int i = 0; i < lits.size(); ++i){
        //   int lit = toInt( lits[i] );
        //   if((lit%2)==0)
        //     cout << (lit/2) + 1 << " ";
        //   else
        //     cout << -((lit+1)/2) << " ";
        // }
        // cout << endl;
        d_solver.addClause_(lits);
        lits.clear();
        continue;
      }
      var = abs(parsedLit) - 1;
      while(var >= d_solver.nVars()) { d_solver.newVar(); }
      lits.push( (parsedLit > 0) ? mkLit(var) : ~mkLit(var) );
    }
  }
  
  //////////////////////////////////////////////////////
  // Create constraint variable & clauses
  if(!d_group){
    d_solver.newVar();
    d_incrSATConstr.resize(frame + 1);
    d_incrSATConstr.at(frame) = d_solver.nVars() - 1;
    
    // Add constraint clause
    lits.clear();
    lits.push( mkLit( d_incrSATConstr.at(frame) ) );
    for(int i = 0; i < d_uncoverBinList.size(); ++i)
      lits.push( mkLit( d_bin2var.at(frame)[d_uncoverBinList.at(i)] ) );
    d_solver.addClause_(lits);
    
    // cout << " Constraint clause" << endl;
    // for(int i = 0; i < lits.size(); ++i){
    //   int lit = toInt( lits[i] );
    //   if((lit%2)==0)
    //     cout << (lit/2) + 1 << " ";
    //   else
    //     cout << -((lit+1)/2) << " ";
    // }
    // cout << endl;
  }
  else{
  	vector<int> temp;
  	// Create variables for each group
  	for(int i = 0; i < d_uncoverGroupBinList.size(); ++i){
  		d_solver.newVar();
  		temp.push_back(d_solver.nVars() - 1);
  	}
  	d_solver.newVar(); // need group_size + 1 variables
  	temp.push_back(d_solver.nVars() - 1);
  	d_groupSATConstr.resize(frame + 1);
  	d_groupSATConstr.at(frame) = temp;
  	
  	// Add constraint clause
  	lits.clear();
  	for(int i = 0; i < temp.size(); ++i){
  		lits.push( mkLit( temp.at(i) ) );
  	}
  	d_solver.addClause_(lits); // top constraint
  	for(int i = 0; i < d_uncoverGroupBinList.size(); ++i){
  		lits.clear();
  		lits.push( ~mkLit( temp.at(i + 1) ) ); // Reserve the first variable in temp
  		for(int j = 0; j < d_uncoverGroupBinList.at(i).size(); ++j){
  		  lits.push( mkLit( d_bin2var.at(frame)[d_uncoverGroupBinList.at(i).at(j)] ) );
  		}
  		d_solver.addClause_(lits); // single group constraint
  	}
  }
  // Update first variable of next frame
  d_initialVar.resize(frame + 2);
  d_initialVar.at(frame + 1) = d_solver.nVars(); // Note: nVars() == (max var + 1)
}

void Trace::addSpecConstraint(int frame){
  int var;
  //////////////////////////////////////////////////////
  // General constraints
  if(frame == 0){  // Initial conditions
    // Set all PI to 0
    for(int i = 0; i < d_circuit.PIListSize(); ++i)
      d_solver.addClause( ~mkLit( d_circuit.getPI(i)->getCnfVar() - 1) );
    // Set all PPI to 0
    for(int i = 0; i < d_circuit.PPIListSize(); ++i)
      d_solver.addClause( ~mkLit( d_circuit.getPPI(i)->getCnfVar() - 1) );
      
    if(d_nR){ // Set all RST to 0
      for(int i = 0; i < d_circuit.RSTListSize(); ++i)
        d_solver.addClause( ~mkLit( d_circuit.getRST(i)->getCnfVar() - 1) );
    }
    else{ // Set all RST to 1
      for(int i = 0; i < d_circuit.RSTListSize(); ++i)
        d_solver.addClause( mkLit( d_circuit.getRST(i)->getCnfVar() - 1) );
    }
  }
  else{
    if(d_nR){ // Set all RST to 1
      for(int i = 0; i < d_circuit.RSTListSize(); ++i){
        var = d_circuit.getRST(i)->getCnfVar() - 1 + d_initialVar.at(frame);
        d_solver.addClause( mkLit(var) );
      }
    }
    else{ // Set all RST to 0
      for(int i = 0; i < d_circuit.RSTListSize(); ++i){
        var = d_circuit.getRST(i)->getCnfVar() - 1 + d_initialVar.at(frame);
        d_solver.addClause( ~mkLit(var) );
      }
    }
  }
  
  //////////////////////////////////////////////////////
  // Spec specific constraints
  
}

void Trace::setBinCNF(int frame){
  int var;
  // Reset all CNF variables of state bins
  for(int i = 0; i < d_coverGroup_p->numOfCoverPoint(); ++i)
    for(int j = 0; j < d_coverGroup_p->getCoverPoint(i)->numOfBin(); ++j)
      d_coverGroup_p->getCoverPoint(i)->getBin(j)->setCnfVar(-1);
  
  // Reset all CNF variables of cross bins
  for(int i = 0; i < d_coverGroup_p->numOfCross(); ++i)
    for(int j = 0; j < d_coverGroup_p->getCross(i)->numOfBin(); ++j)
      d_coverGroup_p->getCross(i)->getBin(j)->setCnfVar(-1);

  // Set CNF variable of coverVariables to consist with circuit CNF variable
  if(frame > 0)
    for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i)
      d_coverGroup_p->getVariable(i)->incrCnfVar(d_initialVar.at(frame) - d_initialVar.at(frame - 1));

  // Set d_dimacsCnfVar_p to new max variable
  (*d_dimacsCnfVar_p) = d_solver.nVars();

  // Set CNF variable of uncovered bins at frame t
  for(int i = 0; i < d_uncoverBinList.size(); ++i){
    d_uncoverBinList.at(i)->setCnfVar( ++(*d_dimacsCnfVar_p) );
    //cout << "  @ Timeframe " << frame << ": CNF var of bin " << d_uncoverBinList.at(i)->getName() << " = " << *d_dimacsCnfVar_p << endl;
    var = (*d_dimacsCnfVar_p) - 1;
    d_bin2var.resize(frame + 1);
    d_bin2var.at(frame)[d_uncoverBinList.at(i)] = var;
    d_var2bin[var] = d_uncoverBinList.at(i);
  }
  
  // Generate CNF clauses of uncovered bins
  for(int i = 0; i < d_uncoverBinList.size(); ++i)
    d_uncoverBinList.at(i)->genCnf(d_dimacsCnfVar_p); // Call genCnf() of each bin
}

void Trace::analyzeSolvedBins(int frame){
  vector<int> bitVector;
  int var = 0;
  long int value = 0;
  
  //////////////////////////////////////////////////////
  // Identify solve value of variables
  for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i){
    CoverVariable* var_p = d_coverGroup_p->getVariable(i);
    for(int t = frame; t >= 0; --t){
      bitVector.resize(var_p->getBitwidth());
      // Identify var CNF because they had been changed during CNF bin generation
      for(int j = 0; j < var_p->getBitwidth(); ++j){
        var = var_p->getCnfVar(j) - 1;
        if(t < frame)
          var -= (d_initialVar.at(frame) - d_initialVar.at(t));
        bitVector.at(j) = (d_solver.modelValue(var) == l_True) ? 1 : 0;
      }
      value = bitvector2int(var_p->getBitwidth(), bitVector);
      var_p->setValue(t, value);
      
      // cout << "#### @ Timeframe " << t << ": Variable: " << var_p->getName() << " = " << var_p->getBitwidth() << "`b";
      // for(int j = bitVector.size() - 1 ; j >=0; --j)  cout << bitVector.at(j);
      // cout << ", integer value = " << value << ",  CNF var = " << var + 1 << endl;
    }
  }
  
  //////////////////////////////////////////////////////
  // Print value of variable
  // Format printing
  cout << setfill('=') << setw(15 + 5 * (frame + 2)); cout << "=" << endl;
  cout << setfill(' ') << setw(15); cout << "Depth:";
  for(int t = 0; t <= frame; ++t){
    cout << setfill(' ') << setw(5);
    cout << t;
  }
  cout << endl;
  
  // Print RST
  for(int i = 0; i < d_circuit.RSTListSize(); ++i){
    cout << setfill(' ') << setw(14);
    cout << d_circuit.getRST(i)->getName() << ":";
    for(int t = 0; t <= frame; ++t){
      var = d_circuit.getRST(i)->getCnfVar() - 1 + d_initialVar.at(t);
      cout << setfill(' ') << setw(5);
      var = (d_solver.modelValue(var) == l_True) ? 1 : 0;
      cout << var;
    }
    cout << endl;
  }
  
  // Print variable
  for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i){
    cout << setfill(' ') << setw(14);
    cout << d_coverGroup_p->getVariable(i)->getName() << ":";
    for(int t = 0; t <= frame; ++t){
      cout << setfill(' ') << setw(5);
      cout << d_coverGroup_p->getVariable(i)->getValue(t);
    }
    cout << endl;
  }
  // Format printing
  cout << setfill('=') << setw(15 + 5 * (frame + 2)); cout << "=" << endl;
}

void Trace::addAllSATConstraint(int frame){
  vec<Lit> lits;
  int var;
  
  for(int t = 0; t < frame; ++t){
    lits.clear();
    for(int i = 0; i < d_circuit.PIListSize(); ++i){
      var = d_circuit.getPI(i)->getCnfVar() - 1 + d_initialVar.at(t);
      if(d_solver.modelValue(var) == l_True)
        lits.push( ~mkLit(var) );
      else
        lits.push( mkLit(var) );
    }
  }
  
  //////////////////////////////////////////////////////
  // Create constraint variable
  d_solver.newVar();
  var = d_solver.nVars() - 1;
  
  lits.push( mkLit(var) );
  d_allSATConstr.at(frame).push_back(var);
   
  d_solver.addClause_(lits);
  
  // Update first variable of next frame
  d_initialVar.resize(frame + 2);
  d_initialVar.at(frame + 1) = d_solver.nVars();
}

////////////////////////////////////////////////////////////////////////////////
// Non-incremental SAT solving functions, used by tpg_noIncrSAT()
void Trace::insert_noIncrSAT(int frame, int maxframe){
  int parsedLit, var, dffPortSize, PPI, PPI_N, prePPO;
  char portChar;
  vec<Lit> lits;
  
  //////////////////////////////////////////////////////
  // Add clauses from circuit
  for(int i = 0; i < d_circuit.gateListSize(); ++i){
    lits.clear();
    const vector<int> gateClause = d_circuit.getGate(i)->getCNFClause();
    if(gateClause.size() > 0){
      for(int j = 0; j < gateClause.size(); ++j){
        parsedLit = gateClause.at(j);
        if(parsedLit == 0){
          d_solver_p->addClause_(lits);
          lits.clear();
          continue;
        }
        parsedLit = (gateClause.at(j) > 0) ? (gateClause.at(j) + d_initialVar.at(frame)) : (gateClause.at(j) - d_initialVar.at(frame));
        var = abs(parsedLit) - 1;
        while(var >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        lits.push( (parsedLit > 0) ? mkLit(var) : ~mkLit(var) );
      }
    }
    else if(d_circuit.getGate(i)->getType() == Gate::DFF && frame >= 1){ // Insert clauses of flip-flops
      dffPortSize = d_circuit.getGate(i)->dffOutSize();
      portChar    = d_circuit.getGate(i)->getDFFPort(0);
      //cout << "DFF port size = " << dffPortSize << ", port char = " << portChar << endl;
      prePPO  = d_circuit.getGate(i)->getInWire(0)->getCnfVar() - 1 + d_initialVar.at(frame - 1);
      while(prePPO >= d_solver_p->nVars()) { d_solver_p->newVar(); }
      if(dffPortSize == 1 && portChar == 'Q'){
        PPI = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        // Connect PPI(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI),  mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI), ~mkLit(prePPO) );
        // cout << "c Connect PPI(t) to PPO(t-1)" << endl;
        // cout << -(PPI + 1) << " " <<  (prePPO + 1) << endl;
        // cout <<  (PPI + 1) << " " << -(prePPO + 1) << endl;
      }
      else if(dffPortSize == 1 && portChar == 'N'){
        PPI_N = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI_N >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        // Connect PPI_N(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI_N), ~mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI_N),  mkLit(prePPO) );
      }
      else if(dffPortSize == 2){
        PPI = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        PPI_N = d_circuit.getGate(i)->getOutWire(1)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI_N >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        // Connect PPI(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI),    mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI),   ~mkLit(prePPO) );
        // Connect PPI_N(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI_N), ~mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI_N),  mkLit(prePPO) );
      }
      else{
        cout << "Unknown DFF port condition." << endl;
        cout << "DFF port size = " << dffPortSize << ", port char = " << portChar << endl;
        exit(1);
      }
    }
  }
  
  //////////////////////////////////////////////////////
  // Add spec constraint clauses
  addSpecConstraint_noIncrSAT(frame);
  
  //////////////////////////////////////////////////////
  // Add clauses of uncovered bins
  setBinCNF_noIncrSAT(frame);
  
  for(int i = 0; i < d_uncoverBinList.size(); ++i){
    const vector<int> binClause = d_uncoverBinList.at(i)->getCNFClause();
    lits.clear();
    for(int j = 0; j < binClause.size(); ++j){
      parsedLit = binClause.at(j);
      if(parsedLit == 0){
        d_solver_p->addClause_(lits);
        lits.clear();
        continue;
      }
      var = abs(parsedLit) - 1;
      while(var >= d_solver_p->nVars()) { d_solver_p->newVar(); }
      lits.push( (parsedLit > 0) ? mkLit(var) : ~mkLit(var) );
    }
  }
  
  // Add bin-solving constraint clauses
  if(frame == maxframe){
    lits.clear();
    for(int i = 0; i < d_uncoverBinList.size(); ++i)
      lits.push( mkLit( d_bin2var.at(frame)[d_uncoverBinList.at(i)] ) );
    d_solver_p->addClause_(lits);
  }
 
  //////////////////////////////////////////////////////
  // Update first variable of next frame
  d_initialVar.resize(frame + 2);
  d_initialVar.at(frame + 1) = d_solver_p->nVars(); // Note: nVars() == (max var + 1)
}

void Trace::addSpecConstraint_noIncrSAT(int frame){
  int var;
  //////////////////////////////////////////////////////
  // General constraints
  if(frame == 0){  // Initial conditions
    // Set all PI to 0
    for(int i = 0; i < d_circuit.PIListSize(); ++i)
      d_solver_p->addClause( ~mkLit( d_circuit.getPI(i)->getCnfVar() - 1) );
    // Set all PPI to 0
    for(int i = 0; i < d_circuit.PPIListSize(); ++i)
      d_solver_p->addClause( ~mkLit( d_circuit.getPPI(i)->getCnfVar() - 1) );
    
    if(d_nR){ // Set all RST to 0
      for(int i = 0; i < d_circuit.RSTListSize(); ++i)
        d_solver_p->addClause( ~mkLit( d_circuit.getRST(i)->getCnfVar() - 1) );
    }
    else{ // Set all RST to 1
      for(int i = 0; i < d_circuit.RSTListSize(); ++i)
        d_solver_p->addClause( mkLit( d_circuit.getRST(i)->getCnfVar() - 1) );
    }
  }
  else{
    if(d_nR){ // Set all RST to 1
      for(int i = 0; i < d_circuit.RSTListSize(); ++i){
        var = d_circuit.getRST(i)->getCnfVar() - 1 + d_initialVar.at(frame);
        d_solver_p->addClause( mkLit(var) );
      }
    }
    else{ // Set all RST to 0
      for(int i = 0; i < d_circuit.RSTListSize(); ++i){
        var = d_circuit.getRST(i)->getCnfVar() - 1 + d_initialVar.at(frame);
        d_solver_p->addClause( ~mkLit(var) );
      }
    }
  }
  
  //////////////////////////////////////////////////////
  // Spec specific constraints
  
}

void Trace::setBinCNF_noIncrSAT(int frame){
  int var;
  // Reset all CNF variables of state bins
  for(int i = 0; i < d_coverGroup_p->numOfCoverPoint(); ++i)
    for(int j = 0; j < d_coverGroup_p->getCoverPoint(i)->numOfBin(); ++j)
      d_coverGroup_p->getCoverPoint(i)->getBin(j)->setCnfVar(-1);
  
  // Reset all CNF variables of cross bins
  for(int i = 0; i < d_coverGroup_p->numOfCross(); ++i)
    for(int j = 0; j < d_coverGroup_p->getCross(i)->numOfBin(); ++j)
      d_coverGroup_p->getCross(i)->getBin(j)->setCnfVar(-1);

  // Set CNF variable of coverVariables to consist with circuit CNF variable
  if(frame > 0){
    for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i)
      d_coverGroup_p->getVariable(i)->incrCnfVar(d_initialVar.at(frame) - d_initialVar.at(frame - 1));
  }
  else{ // frame = 0, reset CNF variable list to original
    for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i)
      d_coverGroup_p->getVariable(i)->resetCnfVar();
  }
  
  // Set d_dimacsCnfVar_p to new max variable
  (*d_dimacsCnfVar_p) = d_solver_p->nVars();

  // Set CNF variable of uncovered bins at frame t
  for(int i = 0; i < d_uncoverBinList.size(); ++i){
    d_uncoverBinList.at(i)->setCnfVar( ++(*d_dimacsCnfVar_p) );
    //cout << "  @ Timeframe " << frame << ": CNF var of bin " << d_uncoverBinList.at(i)->getName() << " = " << *d_dimacsCnfVar_p << endl;
    var = (*d_dimacsCnfVar_p) - 1;
    d_bin2var.resize(frame + 1);
    d_bin2var.at(frame)[d_uncoverBinList.at(i)] = var;
    d_var2bin[var] = d_uncoverBinList.at(i);
  }
  
  // Generate CNF clauses of uncovered bins
  for(int i = 0; i < d_uncoverBinList.size(); ++i)
    d_uncoverBinList.at(i)->genCnf(d_dimacsCnfVar_p); // Call genCnf() of each bin
}

void Trace::analyzeSolvedBins_noIncrSAT(int frame){
  vector<int> bitVector;
  int var = 0;
  long int value = 0;
  
  //////////////////////////////////////////////////////
  // Identify solve value of variables
  for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i){
    CoverVariable* var_p = d_coverGroup_p->getVariable(i);
    for(int t = frame; t >= 0; --t){
      bitVector.resize(var_p->getBitwidth());
      // Identify var CNF because they had been changed during CNF bin generation
      for(int j = 0; j < var_p->getBitwidth(); ++j){
        var = var_p->getCnfVar(j) - 1;
        if(t < frame)
          var -= (d_initialVar.at(frame) - d_initialVar.at(t));
        bitVector.at(j) = (d_solver_p->modelValue(var) == l_True) ? 1 : 0;
      }
      value = bitvector2int(var_p->getBitwidth(), bitVector);
      var_p->setValue(t, value);
      
      // cout << "#### @ Timeframe " << t << ": Variable: " << var_p->getName() << " = " << var_p->getBitwidth() << "`b";
      // for(int j = bitVector.size() - 1 ; j >=0; --j)  cout << bitVector.at(j);
      // cout << ", integer value = " << value << ",  CNF var = " << var + 1 << endl;
    }
  }
  
  //////////////////////////////////////////////////////
  // Print value of variable
  // Format printing
  cout << setfill('=') << setw(15 + 5 * (frame + 2)); cout << "=" << endl;
  cout << setfill(' ') << setw(15); cout << "Depth:";
  for(int t = 0; t <= frame; ++t){
    cout << setfill(' ') << setw(5);
    cout << t;
  }
  cout << endl;
  
  // Print RST
  for(int i = 0; i < d_circuit.RSTListSize(); ++i){
    cout << setfill(' ') << setw(14);
    cout << d_circuit.getRST(i)->getName() << ":";
    for(int t = 0; t <= frame; ++t){
      var = d_circuit.getRST(i)->getCnfVar() - 1 + d_initialVar.at(t);
      cout << setfill(' ') << setw(5);
      var = (d_solver_p->modelValue(var) == l_True) ? 1 : 0;
      cout << var;
    }
    cout << endl;
  }
  
  // Print variable
  for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i){
    cout << setfill(' ') << setw(14);
    cout << d_coverGroup_p->getVariable(i)->getName() << ":";
    for(int t = 0; t <= frame; ++t){
      cout << setfill(' ') << setw(5);
      cout << d_coverGroup_p->getVariable(i)->getValue(t);
    }
    cout << endl;
  }
  // Format printing
  cout << setfill('=') << setw(15 + 5 * (frame + 2)); cout << "=" << endl;
}

void Trace::addAllSATConstraint_noIncrSAT(int frame){
  vec<Lit> lits;
  int var;
  
  if(d_patterns.size() == 0)
    return;
  
  if(frame == 0){
    lits.clear();
    for(int j = 0; j < d_circuit.PIListSize(); ++j){
      var = d_circuit.getPI(j)->getCnfVar() - 1;
      lits.push( mkLit(var) );
    }
    d_solver_p->addClause_(lits);
  }
  else{
    for(int i = 0; i < d_patterns.size(); ++i){
      vector<vector<int> >& pattern = d_patterns.at(i);
      lits.clear();
      for(int t = 0; t < frame; ++t){
        vector<int>& tPattern = pattern.at(t);
        for(int j = 0; j < d_circuit.PIListSize(); ++j){
          var = d_circuit.getPI(j)->getCnfVar() - 1 + d_initialVar.at(t);
          if(tPattern.at(j) == 1)
            lits.push( ~mkLit(var) );
          else
            lits.push( mkLit(var) );
        }
      }
      // Add a constraint clause for each pattern
      d_solver_p->addClause_(lits);
    }      
  }
}

void Trace::recordPatterns(int frame){
  int var;
  vector<int> tPattern;
  vector<vector<int> > pattern;
  
  if(frame == 0){
    tPattern.clear();
    for(int i = 0; i < d_circuit.PIListSize(); ++i){
      tPattern.push_back(0);
    }
    pattern.push_back(tPattern);
    d_patterns.push_back(pattern);
  }
  else{
    for(int t = 0; t < frame; ++t){
      tPattern.clear();
      for(int i = 0; i < d_circuit.PIListSize(); ++i){
        var = d_circuit.getPI(i)->getCnfVar() - 1 + d_initialVar.at(t);
        if(d_solver_p->modelValue(var) == l_True)
          tPattern.push_back(1);
        else
          tPattern.push_back(0);
      }
      pattern.push_back(tPattern);
    }
    d_patterns.push_back(pattern);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Single SAT solving functions, used by tpg_single()
void Trace::insert_single(int frame, int binIdx){
  int parsedLit, var, dffPortSize, PPI, PPI_N, prePPO;
  char portChar;
  vec<Lit> lits;
  
  //////////////////////////////////////////////////////
  // Add clauses from circuit
  for(int i = 0; i < d_circuit.gateListSize(); ++i){
    lits.clear();
    const vector<int> gateClause = d_circuit.getGate(i)->getCNFClause();
    if(gateClause.size() > 0){
      for(int j = 0; j < gateClause.size(); ++j){
        parsedLit = gateClause.at(j);
        if(parsedLit == 0){
          d_solver_p->addClause_(lits);
          lits.clear();
          continue;
        }
        parsedLit = (gateClause.at(j) > 0) ? (gateClause.at(j) + d_initialVar.at(frame)) : (gateClause.at(j) - d_initialVar.at(frame));
        var = abs(parsedLit) - 1;
        while(var >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        lits.push( (parsedLit > 0) ? mkLit(var) : ~mkLit(var) );
      }
    }
    else if(d_circuit.getGate(i)->getType() == Gate::DFF && frame >= 1){ // Insert clauses of flip-flops
      dffPortSize = d_circuit.getGate(i)->dffOutSize();
      portChar    = d_circuit.getGate(i)->getDFFPort(0);
      //cout << "DFF port size = " << dffPortSize << ", port char = " << portChar << endl;
      prePPO  = d_circuit.getGate(i)->getInWire(0)->getCnfVar() - 1 + d_initialVar.at(frame - 1);
      while(prePPO >= d_solver_p->nVars()) { d_solver_p->newVar(); }
      if(dffPortSize == 1 && portChar == 'Q'){
        PPI = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        // Connect PPI(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI),  mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI), ~mkLit(prePPO) );
        // cout << "c Connect PPI(t) to PPO(t-1)" << endl;
        // cout << -(PPI + 1) << " " <<  (prePPO + 1) << endl;
        // cout <<  (PPI + 1) << " " << -(prePPO + 1) << endl;
      }
      else if(dffPortSize == 1 && portChar == 'N'){
        PPI_N = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI_N >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        // Connect PPI_N(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI_N), ~mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI_N),  mkLit(prePPO) );
      }
      else if(dffPortSize == 2){
        PPI = d_circuit.getGate(i)->getOutWire(0)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        PPI_N = d_circuit.getGate(i)->getOutWire(1)->getCnfVar() - 1 + d_initialVar.at(frame);
        while(PPI_N >= d_solver_p->nVars()) { d_solver_p->newVar(); }
        // Connect PPI(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI),    mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI),   ~mkLit(prePPO) );
        // Connect PPI_N(t) to PPO(t-1)
        d_solver_p->addClause( ~mkLit(PPI_N), ~mkLit(prePPO) );
        d_solver_p->addClause(  mkLit(PPI_N),  mkLit(prePPO) );
      }
      else{
        cout << "Unknown DFF port condition." << endl;
        cout << "DFF port size = " << dffPortSize << ", port char = " << portChar << endl;
        exit(1);
      }
    }
  }
  
  //////////////////////////////////////////////////////
  // Add spec constraint clauses
  addSpecConstraint_noIncrSAT(frame); // Reused noIncrSAT function
  
  //////////////////////////////////////////////////////
  // Add clauses of a single uncovered bin
  setBinCNF_single(frame, binIdx);
  
  const vector<int> binClause = d_uncoverBinList.at(binIdx)->getCNFClause();
  lits.clear();
  for(int j = 0; j < binClause.size(); ++j){
    parsedLit = binClause.at(j);
    if(parsedLit == 0){
      d_solver_p->addClause_(lits);
      lits.clear();
      continue;
    }
    var = abs(parsedLit) - 1;
    while(var >= d_solver_p->nVars()) { d_solver_p->newVar(); }
    lits.push( (parsedLit > 0) ? mkLit(var) : ~mkLit(var) );
  }
 
  //////////////////////////////////////////////////////
  // Update first variable of next frame
  d_initialVar.resize(frame + 2);
  d_initialVar.at(frame + 1) = d_solver_p->nVars(); // Note: nVars() == (max var + 1)
}

void Trace::setBinCNF_single(int frame, int binIdx){
  int var;
  // Reset the CNF variable of the bin
  d_uncoverBinList.at(binIdx)->setCnfVar(-1);

  // Set CNF variable of coverVariables to consist with circuit CNF variable
  if(frame > 0){
    for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i)
      d_coverGroup_p->getVariable(i)->incrCnfVar(d_initialVar.at(frame) - d_initialVar.at(frame - 1));
  }
  else{ // frame = 0, reset CNF variable list to original
    for(int i = 0; i < d_coverGroup_p->numOfVariable(); ++i)
      d_coverGroup_p->getVariable(i)->resetCnfVar();
  }
  
  // Set d_dimacsCnfVar_p to new max variable
  (*d_dimacsCnfVar_p) = d_solver_p->nVars();

  // Set the CNF variable of the uncovered bin at frame t
  d_uncoverBinList.at(binIdx)->setCnfVar( ++(*d_dimacsCnfVar_p) );
  var = (*d_dimacsCnfVar_p) - 1;
  d_bin2var.resize(frame + 1);
  d_bin2var.at(frame)[d_uncoverBinList.at(binIdx)] = var;
  d_var2bin[var] = d_uncoverBinList.at(binIdx);
  
  // Generate CNF clauses of the uncovered bin
  d_uncoverBinList.at(binIdx)->genCnf(d_dimacsCnfVar_p);
}

////////////////////////////////////////////////////////////////////////////////
// Utility functions
void Trace::parseBinsToBeSolved(){
  CoverPoint* coverPoint_p;
  Cross*      cross_p;
  d_uncoverBinList.clear();
  d_uncoverGroupBinList.clear();
  d_inUncoverBinFile.open("fsdfds");
  vector<BaseBin*> binGroup;
  
  if(!d_inUncoverBinFile){
    //File not specified, record all bins by default
    cout << "Warning!! Can not open bin_file ... " << endl;
    cout << "Parse all bins by default ..." << endl;
    for(int j = 0; j < d_coverGroup_p->numOfCoverPoint(); ++j){
      coverPoint_p = d_coverGroup_p->getCoverPoint(j);
//      cout << "Parsing coverpoint: " << coverPoint_p->getName() << endl;
//      cout << "There are " << coverPoint_p->numOfBin() << " bins" << endl;
      if(d_group) binGroup.clear();
      for(int k = 0; k < coverPoint_p->numOfBin(); ++k){
//        cout << "  Parsing bin: " << coverPoint_p->getBin(k)->getName() << endl;
        d_uncoverBinList.push_back(coverPoint_p->getBin(k));
        if(d_group) binGroup.push_back(coverPoint_p->getBin(k));
//        cout << "   Finish parsing bin ..." << endl;
      }
      if(d_group) d_uncoverGroupBinList.push_back(binGroup);
    }
//    cout << "After parsing coverpoints ..." << endl;
    for(int j = 0; j <d_coverGroup_p->numOfCross(); ++j){
      cross_p = d_coverGroup_p->getCross(j);
      if(d_group) binGroup.clear();
      for(int k = 0; k < cross_p->numOfBin(); ++k){
        d_uncoverBinList.push_back(cross_p->getBin(k));
        if(d_group) binGroup.push_back(cross_p->getBin(k));
      }
      if(d_group) d_uncoverGroupBinList.push_back(binGroup);
    }
//    cout << "After parsing crosses ..." << endl;
  }
  else{
    // Parse bins specified in the file
    // For ease of use, the file just list name of bins, single name a line,
    // (assume no duplicated names in different coverpoint/cross)
    // Note: currently not support group identification
    char cStr[Global::STR_LENGTH];
    string str;
    while(d_inUncoverBinFile.getline(cStr, Global::STR_LENGTH)){
      str = string(cStr);
      Global::eraseBlanks(&str);
      d_uncoverBinList.push_back(d_coverGroup_p->getBin(str));
    }
    d_inUncoverBinFile.close();
  }
  
  // Random grouping
  if(d_randGroup > 1){
  	d_uncoverGroupBinList.clear();
  	const int groupSize = d_randGroup;
  	int binNum = d_uncoverBinList.size();
  	//int groupNum = (binNum%groupSize)>0 ? (binNum/groupSize)+1 : (binNum/groupSize);
  	// Random permutation
  	srand (time(NULL));
  	for(int i = 0; i < binNum*3; ++i){
  		int b1 = rand() % binNum;
  		int b2 = rand() % binNum;
  		BaseBin* temp = d_uncoverBinList.at(b1);
  		d_uncoverBinList.at(b1) = d_uncoverBinList.at(b2);
  		d_uncoverBinList.at(b2) = temp;
  	}
  	
  	int i = 0;
  	while(i < binNum){
  		binGroup.clear();
  		for(int j = 0; j < groupSize && i < binNum; ++j, ++i){
  			binGroup.push_back(d_uncoverBinList.at(i));
  		}
  		d_uncoverGroupBinList.push_back(binGroup);
  	}
  }
}

long int Trace::bitvector2int(const int& bitWidth, const vector<int>& bitString){
  long int val = 0;
  for(int i = 0; i < bitWidth; ++i){
    val += (long int)( pow( (double)2, i) ) * bitString.at(i);
  }
  return val;
}

void Trace::genPatterns(int frame){
  
  static ofstream p("patterns.p");
  static int pnum = 1;
  string CLK = d_circuit.getCLK(0)->getName();
  string RST = d_circuit.getRST(0)->getName();
  string temp, temp2, name;
  Wire* w;
  vector<int> bitString;
  int var, find1, find2;
  long value;
  
  p << "Pattern # " << pnum++ << endl;
  for(int t = 0; t <= frame; ++t){
    p << "@(negedge " << CLK << ")" << endl;
    for(int i = 0; i < d_circuit.PIListSize(); ++i){
      w = d_circuit.getPI(i);
      temp = w->getName();
      //p << "temp = " << temp << endl;
      bitString.clear();
      if((find1 = temp.find_first_of('[')) == string::npos){ // single bit
        name = temp;
        var  = w->getCnfVar() + d_initialVar.at(t) - 1;
        bitString.push_back(0);
        bitString.at(0) = (d_solver.modelValue(var) == l_True) ? 1 : 0;
      }
      else{ // multi bit
        name = temp.substr(0, find1);
        var  = w->getCnfVar() + d_initialVar.at(t) - 1;
        bitString.push_back(0);
        bitString.at(0) = (d_solver.modelValue(var) == l_True) ? 1 : 0;
        int idx = 1;
        for(int j = i + 1; j < d_circuit.PIListSize(); ++i, ++j){
          w = d_circuit.getPI(j);
          temp2 = w->getName();
          //p << "temp2 = " << temp2 << endl;
          if((find2 = temp2.find_first_of('[')) != string::npos){
            if(name.compare( temp2.substr(0, find2) ) == 0){
              var  = w->getCnfVar() + d_initialVar.at(t) - 1;
              bitString.push_back(0);
              bitString.at(idx++) = (d_solver.modelValue(var) == l_True) ? 1 : 0;
            }
            else break;
          }
          else break;
        }
      }
      //p << "bitString = " << bitString.size() << "'d";
      //for(int k = 0; k < bitString.size(); ++k)
      //  p << bitString.at(k);
      //p << endl;
      value = bitvector2int(bitString.size(), bitString);
      p << name << " = " << bitString.size() << "'d" << value << ";" << endl;
    }
  }
}

void Trace::printStats(){
	cout << "Clauses   : " << d_solver.nClauses() << endl;
	cout << "Variables : " << d_solver.nVars()    << endl;
	cout << "Learns    : " << d_solver.nLearnts() << endl;
	cout << "Conflicts : " << d_solver.conflicts  << endl;
}

void Trace::printStats_(){
	cout << "Clauses   : " << d_solver_p->nClauses() << endl;
	cout << "Variables : " << d_solver_p->nVars()    << endl;
	cout << "Learns    : " << d_solver_p->nLearnts() << endl;
	cout << "Conflicts : " << d_solver_p->conflicts  << endl;
}


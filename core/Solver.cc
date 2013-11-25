/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <math.h>
#include "mtl/Sort.h"
#include "core/Solver.h"

#include "utils/System.h"

//#define UIP_TRAIL

using namespace Minisat;

//=================================================================================================
// Options:

static const char* _cat = "CORE";

static DoubleOption  opt_var_decay         (_cat, "var-decay",   "The variable activity decay factor",            0.95,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay      (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq   (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed       (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode        (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption     opt_phase_saving      (_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act      (_cat, "rnd-init",    "Randomize the initial activity", false);
static BoolOption    opt_luby_restart      (_cat, "luby",        "Use the Luby restart sequence", true);
static IntOption     opt_restart_first     (_cat, "rfirst",      "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption  opt_restart_inc       (_cat, "rinc",        "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption  opt_garbage_frac      (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.20, DoubleRange(0, false, HUGE_VAL, false));

// Options for diversekSet algorithms:
static BoolOption    opt_rnd_pol           (_cat, "p-rand",      "Randomize the polarty selection", false);
static BoolOption    opt_guide_pol         (_cat, "p-guide",     "Guide the polarty selection according to potential", false);
static IntOption     opt_BCP_t             (_cat, "bcp",         "Threshold value of BCPGuide hueristic", 0, IntRange(0, INT32_MAX));
static BoolOption    opt_vrnd_pol          (_cat, "p-vrandLoc",  "Randomize the CBH polarty selection", false);
static BoolOption    opt_vguide_pol        (_cat, "p-vguideLoc", "Guide the CBH polarty selection", false);

//=================================================================================================
// Constructor/Destructor:


Solver::Solver():

    // Parameters (user settable):
    //
    verbosity        (0)
  , var_decay        (opt_var_decay)
  , clause_decay     (opt_clause_decay)
  , random_var_freq  (opt_random_var_freq)
  , random_seed      (opt_random_seed)
  , luby_restart     (opt_luby_restart)
  , ccmin_mode       (opt_ccmin_mode)
  , phase_saving     (opt_phase_saving)
  , rnd_pol          (opt_rnd_pol)
  , rnd_init_act     (opt_rnd_init_act)
  , garbage_frac     (opt_garbage_frac)
  , required_models  (1)
  , restart_first    (opt_restart_first)
  , restart_inc      (opt_restart_inc)
    // Parameters (the rest):
    //
  , learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

    // Parameters (experimental):
    //
  , learntsize_adjust_start_confl (100)
  , learntsize_adjust_inc         (1.5)

  , BCPGuide_T(opt_BCP_t)       // opt in Main()
    // Statistics: (formerly in 'SolverStats')
    //
  , solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
  , dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)
  , num_models(0)
  , bcp_conflict(0) 

  , ok                 (true)
  , cla_inc            (1)
  , var_inc            (1)
  , watches            (WatcherDeleted(ca))
  , qhead              (0)
  , simpDB_assigns     (-1)
  , simpDB_props       (0)
  , order_heap         (VarOrderLt(activity))
  , progress_estimate  (0)
  , remove_satisfied   (false)
  , okBCP              (false)
  , BCPCounts          (0)
  , vbs                (opt_vrnd_pol || opt_vguide_pol)
  , CBH_order_heap     (VarOrderLt(iosv))
 
    // Resource constraints:
    //
  , conflict_budget    (-1)
  , propagation_budget (-1)
  , asynch_interrupt   (false)
{}

Solver::~Solver(){}

//=================================================================================================
// Minor methods:

// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar){
    int v = nVars();
    VarStatCell stat;
    watches     .init(mkLit(v, false));
    watches     .init(mkLit(v, true ));
    assigns     .push(l_Undef);
    vardata     .push(mkVarData(CRef_Undef, 0));
    activity    .push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen        .push(0);
    polarity    .push(sign);
    decision    .push();
    trail       .capacity(v+1);
    setDecisionVar(v, dvar);
    varStat     .push(stat);
    fromClause  .push();
    iosv        .push(0); 
    ios_n       .push(0);
    ios_p       .push(0);
    igs_n       .push(0);
    igs_p       .push(0); 
    ils_n       .push(0);
    ils_p       .push(0);
    
    lcv         .push(0);
    lcl_n       .push(0);
    lcl_p       .push(0);
    gcv         .push(0);
    gcl_n       .push(0);
    gcl_p       .push(0);

    return v;
}

bool Solver::addClause_(vec<Lit>& ps){
    assert(decisionLevel() == 0);
    if(!ok) return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p; 
    int i, j;
    
    for(i = j = 0, p = lit_Undef; i < ps.size(); i++){
        if(value(ps[i]) == l_True || ps[i] == ~p){
            return true;
        }
        else if(value(ps[i]) != l_False && ps[i] != p){
            ps[j++] = p = ps[i];

        }
    }

    ps.shrink(i - j);

    if(ps.size() == 0) 
        return ok = false;
    else if(ps.size() == 1){
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
    }
    else{
        CRef cr = ca.alloc(ps, false);
        clauses.push(cr);
        attachClause(cr);

        if(vbs){
            for(i = 0; i < ps.size(); i++){
                p = ps[i];
                fromClause[var(p)].push(cr);
                // Initialize CBH global scores
                if(sign(p)){
                    igs_p[var(p)]++;
                    gcl_p[var(p)]++;
                }
                else{
                    igs_n[var(p)]++;
                    gcl_n[var(p)]++;
                }
            }
        }

    }

    return true;
}

void Solver::attachClause(CRef cr){
    const Clause& c = ca[cr];
    assert(c.size() > 1);
    watches[~c[0]].push(Watcher(cr, c[1]));
    watches[~c[1]].push(Watcher(cr, c[0]));
    if(c.learnt()) learnts_literals += c.size();
    else           clauses_literals += c.size(); 
}

void Solver::detachClause(CRef cr, bool strict){
    const Clause& c = ca[cr];
    assert(c.size() > 1);
    
    if(strict){
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
    }
    else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
    }

    if(c.learnt()) learnts_literals -= c.size();
    else           clauses_literals -= c.size(); 
}

void Solver::removeClause(CRef cr){
    // Remove clause from CBH clause list
    if(vbs){
        std::list<CRef>::iterator it;
        for(it = CBH_list.begin(); it != CBH_list.end(); ++it){
            if((*it) == cr){
                CBH_list.erase(it);
                break;
            }
        }
    }

    Clause& c = ca[cr];
    detachClause(cr);
    // Don't leave pointers to free'd memory!
    if(locked(c)) vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1); 
    ca.free(cr);

    for(std::list<CRef>::iterator it = CBH_list.begin(); it != CBH_list.end(); ++it){
        assert((*it) < ca.size());
    }
//    printf("removeClause: CBH_list = %d, clauses = %d, learnts = %d\n", CBH_list.size(), clauses.size(), learnts.size());
}

bool Solver::satisfied(const Clause& c) const{
    for(int i = 0; i < c.size(); i++)
        if(value(c[i]) == l_True)
            return true;
    return false; 
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level){
//    printf("decisionLevel = %d, cancel level = %d\n", decisionLevel(), level);
    if(decisionLevel() > level){
        for(int c = trail.size() - 1; c >= trail_lim[level]; c--){
            Var x  = var(trail[c]);
            assigns [x] = l_Undef;
            if(phase_saving > 1 || (phase_saving == 1) && c > trail_lim.last())
                polarity[x] = sign(trail[c]);
            insertVarOrder(x);
            if(vbs)
                insertCBHVarOrder(x);
        }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    }
}


//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit(){
    Var next = var_Undef;
    bool pol = false;

    // Variable-based CBH decision
    if(opt_vrnd_pol){
//        if(num_models > 0)  printf("$$$$$$$$$$$\n");
        double initial_time = cpuTime();
        return vRandLoc();
        double end_time = cpuTime();
        vRandLoc_time += (end_time - initial_time);
    }
    else if(opt_vguide_pol)
        return vGuideLoc();

    // Random decision:
    //if(drand(random_seed) < random_var_freq && !order_heap.empty()){
    //    next = order_heap[irand(random_seed,order_heap.size())];
    //    if(value(next) == l_Undef && decision[next])
    //        rnd_decisions++; 
    //}

    // Activity based decision:
    while(next == var_Undef || value(next) != l_Undef || !decision[next]){
        if(order_heap.empty()){
            next = var_Undef;
            break;
        }
        else
            next = order_heap.removeMin();
    }
 
    // Polarity selection heuristics:

    assert( opt_rnd_pol ^ opt_guide_pol ^ opt_vrnd_pol ^ opt_vguide_pol );

    if(next != var_Undef){
        pol = ((BCPGuide_T > 0) && (bcp_conflict < BCPGuide_T)) ? ( (varStat[next][0] > varStat[next][1]) ? 1 : 0 )
        //pol = ((BCPGuide_T > 0) && (BCPCounts < BCPGuide_T)) ? ( (varStat[next][0] > varStat[next][1]) ? 1 : 0 )
            : opt_rnd_pol ? (drand(random_seed) < 0.5)
            : opt_guide_pol ? pGuide(next)
            : polarity[next];
    }

    //return next == var_Undef ? lit_Undef : mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
    return next == var_Undef ? lit_Undef : mkLit(next, pol);
}

int Solver::absPotential(Var var){
    int pos = varStat[var][1], neg = varStat[var][0];
    return (pos > neg) ? (pos - neg) : (neg - pos);
}

bool Solver::pGuide(Var var){

    int pos = varStat[var][1], neg = varStat[var][0];
    if(pos > neg){
        //if(var >=20 && var <=30)
        //    printf("GUIDE var = %d, varStat[var][1] = %d, varStat[var][0] = %d, select neg\n", var, varStat[var][1], varStat[var][0]);
        //return false;
        return true;
    }
    else if(pos < neg){
        //if(var >=20 && var <=30)
        //    printf("GUIDE var = %d, varStat[var][1] = %d, varStat[var][0] = %d, select pos\n", var, varStat[var][1], varStat[var][0]);
        //return true;
        return false;
    }
    else{
        //if(var >=20 && var <=30)
        //    printf("GUIDE var = %d, varStat[var][1] = %d, varStat[var][0] = %d, select rand\n", var, varStat[var][1], varStat[var][0]);
        return drand(random_seed) < 0.5; 
    }
}

Lit Solver::vGuideLoc(){

    Lit p = lit_Undef;

    for(std::list<CRef>::iterator it = CBH_list.begin(); it != CBH_list.end(); ++it){
        const Clause& c = ca[*it];
        p = lit_Undef;
        if(!satisfied(c)){
            for(int i = 0; i < c.size(); ++i){
                if(value(c[i]) == l_Undef){
                    if(p == lit_Undef) 
                        p = c[i];
                    else if( absPotential(var(p)) > absPotential(var(c[i])) )
                        ;
                    else if( absPotential(var(p)) == absPotential(var(c[i])) ){
                        if( lcv[var(p)] > lcv[var(c[i])])
                            ;
                        else if( lcv[var(p)] == lcv[var(c[i])] ){
                            if( gcv[var(p)] > gcv[var(c[i])])
                                ;
                            else if( gcv[var(p)] == gcv[var(c[i])])
                                p = (var(p) < var(c[i])) ? p : c[i];
                            else
                                p = c[i];
                        }
                    }
                    else
                        p = c[i];
                }
            }
            break;
        }
        //assert(*it != CBH_list.back()); // All clauses satisfied
    }

    return p == lit_Undef ? lit_Undef : mkLit( var(p), pGuide(var(p))); // if all clauses satisfied, p == lit_Undef
}

Lit Solver::vRandLoc(){
    Lit p = lit_Undef;
//    if(num_models > 0)  printf("Size of list = %d, size of clauses = %d, size of learned clauses = %d, size of ca = %d\n", CBH_list.size(), clauses.size(), learnts.size(), ca.size());

    for(std::list<CRef>::iterator it = CBH_list.begin(); it != CBH_list.end(); ++it){
        assert((*it) != CRef_Undef);
        const Clause& c = ca[(*it)];
        p = lit_Undef;
//        if(num_models > 0) printf("???\n");
        if(!satisfied(c)){
            for(int i = 0; i < c.size(); ++i){
//                if(num_models > 0)  printf("    Literal # %d\n", i);
                if(value(c[i]) == l_Undef){
                    if(p == lit_Undef) 
                        p = c[i];
                    else
                        p = (drand(random_seed) < 0.5) ? p : c[i];
                }
            }
            break;
        }
        //assert((*it) != CBH_list.back()); // All clauses satisfied
    }

    if(p == lit_Undef){  // All clauses satisfied
        for(int i = 0; i < nVars(); ++i){
            if(value(i) == l_Undef || !decision[i])
                return mkLit(i, pGuide(i));
        }
    }

    return p == lit_Undef ? lit_Undef : mkLit( var(p), pGuide(var(p))); // if all clauses satisfied, p == lit_Undef
}

double Solver::quality(){
    assert(required_models == num_models);
    double sum = 0;
    for(int i = 0; i < nVars(); ++i){
        //printf("Var %d, negative = %d, positive = %d\n", i,  varStat[i][0], varStat[i][1]);
        sum += (varStat[i][0] * varStat[i][1]);
    }

    if(required_models == 1) 
        return 0;

    double div = num_models * (num_models - 1) * nVars();
    return (2 * sum / div) ;  // sum / ( C(m,2) * nVars() )
}

void Solver::printVarStat(){
    for(int i = 0; i < nVars(); ++i)
        if(varStat[i][0] != 0 && varStat[i][1] != 0)
            printf("var = %d, negative = %d, positive = %d\n", i,  varStat[i][0], varStat[i][1]);
}

double Solver::BCPQuality(){
        //printf("Partial Var %d, negative = %d, positive = %d\n", i,  varStat[i][0], varStat[i][1]);
    double score = 0;
    for(int i = 0; i < BCPGuide_partal_assigns.size(); ++i){
        Lit p = BCPGuide_partal_assigns[i];
        Var v = var(p);
        //int pol = sign(p) ? 1 : 0;
        //score += ( varStat[v][0] + ~pol ) * (  varStat[v][1] + pol );
        //printf("Partial Var %d, negative = %d, positive = %d\n", i,  varStat[i][0], varStat[i][1]);
        if(!sign(p))
            score += (varStat[v][0] * (varStat[v][1] + 1));
        else
            score += ((varStat[v][0] + 1) * varStat[v][1]);
    }

    return (double) score / (double) BCPGuide_partal_assigns.size(); // average score
}

void Solver::CBH_init(){

    // Initialize scores
    for(int i = 0; i < nVars(); ++i){
        // lis are updated during clause list construction
        ils_n[i] = 0;
        ils_p[i] = 0;
        // igs are initialize in :addClause_() and never be changed after that 
        ios_n[i] = igs_n[i] + ils_n[i];
        ios_p[i] = igs_p[i] + ils_p[i];
        iosv [i] = ios_p[i] + ios_n[i] + 3 * (ios_p[i] < ios_n[i] ? ios_p[i] : ios_n[i]);

        lcl_n[i] = 0;
        lcl_p[i] = 0;
        lcv  [i] = 0;  // lcv[i] = lcl_p[i] + lcl_n[i] + 3 * (lcl_p[i] < lcl_n[i] ? lcl_p[i] : lcl_n[i]);
        // gcl are initialize in :addClause_() and never be changed after that 
        gcv  [i] = gcl_p[i] + gcl_n[i] + 3 * (gcl_p[i] < gcl_n[i] ? gcl_p[i] : gcl_n[i]);
    }

    // Construct CBH_order_heap
    for(int i = 0; i < nVars(); ++i)
        insertCBHVarOrder(i);

    // Construct CBH_list
    CBH_list.clear();
    Var max_osv = var_Undef;

    for(int numAppended = 0;;){
        if(CBH_order_heap.empty()){
            assert(CBH_list.size() == clauses.size());
            break;
        }
        else{
            max_osv = CBH_order_heap.removeMin();
            numAppended = 0;
        }

        // Append clauses containing max_osv
        vec<CRef>& clauseVec = fromClause[max_osv];
        for(int i = 0; i < clauseVec.size(); ++i){
            CRef cr = clauseVec[i];
            Clause& c = ca[cr];
            if(c.appended())
                continue;
            // Append the clause into CBH clause list
            CBH_list.push_back(cr);
            c.append(1);

            // Update ils and ios for each literal in the clause
            for(int k = 0; k < c.size(); ++k){
                 iosvUpdate(c[k]);
                 CBH_order_heap.update(var(c[k]));
            }
        }

        //for(int i = 0; i < clauses.size(); ++i){
        //    CRef cr = clauses[i];
        //    Clause& c = ca[cr];

        //    // Skip if already appended
        //    if(c.appended()){
        //        numAppended++;
        //        continue;
        //    }

        //    for(int j = 0; j < c.size(); ++j){
        //        Lit p = c[j];
        //        if(max_osv == var(p)){

        //            // Append the clause into CBH clause list
        //            CBH_list.push_back(cr);
        //            c.append(1);

        //            // Update ils and ios for each literal in the clause
        //            for(int k = 0; k < c.size(); ++k){
        //                iosvUpdate(c[k]);
        //                CBH_order_heap.update( var(c[k]) );
        //            }
        //        }
        //    }
        //}

        // All clauses appended, done constructing list
        //if(numAppended == clauses.size())
        //    break;
    }
//    printf("CBH_init: CBH_list = %d, clauses = %d, learnts = %d\n", CBH_list.size(), clauses.size(), learnts.size());
    for(std::list<CRef>::iterator it = CBH_list.begin(); it != CBH_list.end(); ++it){
        assert((*it) < ca.size());
    }
}

void Solver::CBH_reloc(CRef cr, CRef newCr){
    double initial_time = cpuTime();
    for(std::list<CRef>::iterator it = CBH_list.begin(); it != CBH_list.end(); ++it){
        if((*it) == cr){
            (*it) = newCr;
            break;
        }
    }
    double end_time = cpuTime();
    CBH_reloc_time += (end_time - initial_time);
}

void Solver::CBH_update(CRef confl){
    double initial_time = cpuTime();
    std::list<CRef> temp_list; // Contains learnt clause and conflict-responsible clauses
    std::list<CRef>::iterator it;

//    printf("CBH_update begin: CBH_list = %d, clauses = %d, learnts = %d\n", CBH_list.size(), clauses.size(), learnts.size());
    assert(confl < ca.size());

    temp_list.push_back(confl);
    for(int i = 0; i < respons_clauses.size(); ++i){
        CRef cr = respons_clauses[i];
        temp_list.push_front(cr);

        // Remove conflict-responsible clauses from original list
        for(it = CBH_list.begin(); it != CBH_list.end(); ++it){
            assert((*it) != CRef_Undef);
            assert(cr != CRef_Undef);
            if((*it) == cr){
                CBH_list.erase(it);
                break;
            }
            assert((*it) != CBH_list.back() ); // Somehow clause not found
        }
    }

    // Copy temp_list to the head of CBH_list
    // note: confl is push at the head
    for(it = temp_list.begin(); it != temp_list.end(); ++it){
        CBH_list.push_front((*it));
    }

//    printf("CBH_update end: CBH_list = %d, clauses = %d, learnts = %d\n", CBH_list.size(), clauses.size(), learnts.size());
    double end_time = cpuTime();
    CBH_update_time += (end_time - initial_time);
}

void Solver::iosvUpdate(Lit p){
    Var v = var(p);

    if(sign(p)){
        ils_p[v]++;
        ios_p[v] = igs_p[v] + ils_p[v];
    }
    else{
        ils_n[v]++;
        ios_n[v] = igs_n[v] + ils_n[v];
    }

    iosv[v] = ios_p[v] + ios_n[v] + 3 * (ios_p[v] < ios_n[v] ? ios_p[v] : ios_n[v]);
}

void Solver::lcvUpdate(Lit p){
    Var v = var(p);

    if(sign(p))
        lcl_p[v]++;
    else
        lcl_n[v]++;

    lcv[v] = lcl_p[v] + lcl_n[v] + 3 * (lcl_p[v] < lcl_n[v] ? lcl_p[v] : lcl_n[v]);
}

void Solver::gcvUpdate(Lit p){
    Var v = var(p);

    if(sign(p))
        gcl_p[v]++;
    else
        gcl_n[v]++;

    gcv[v] = gcl_p[v] + gcl_n[v] + 3 * (gcl_p[v] < gcl_n[v] ? gcl_p[v] : gcl_n[v]);
}

void Solver::lclDecay(){    // Half the scores of lcl every 100 conflicts
    if( (conflicts % 100 == 0) && (conflicts / 100 == (lcl_decay_counts + 1)) ){
        for(int i = 0; i < nVars(); ++i){
            lcl_p[i] /= 2;
            lcl_n[i] /= 2;
        }
        lcl_decay_counts++;
    }
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|  
|  Description:
|    Analyze conflict and produce a reason clause.
|  
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|  
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the 
|        rest of literals. There may be others from the same level though.
|  
|________________________________________________________________________________________________@*/

void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel){
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;

    respons_clauses.clear();  // VGUIDE
    respons_clauses.push(confl);  // VGUIDE

#ifdef UIP_TRAIL
    printf("Start analyze() ...\n");
#endif    
    do{
        assert(confl != CRef_Undef); // (otherwise should be UIP)
        Clause& c = ca[confl];

        if(c.learnt())
            claBumpActivity(c);

        for(int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
            Lit q = c[j];
#ifdef UIP_TRAIL
            printf("  Var %c' s level = %d; Current decision level = %d\n",  v2c(var(q)), level(var(q)), decisionLevel());
#endif            
            if(!seen[var(q)] && level(var(q)) > 0){
#ifdef UIP_TRAIL
                printf("    Looking %c%c ...\n", sign(q)? '-' : ' ', v2c(var(q)));
#endif                
                varBumpActivity(var(q));
                seen[var(q)] = 1;
                if(level(var(q)) >= decisionLevel()){
                    pathC++;
#ifdef UIP_TRAIL
                    printf("    pathC++ = %d\n", pathC);
#endif                    
                }
                else{
                    out_learnt.push(q);
                    if(vbs){
                        CRef from = reason(var(q));
                        if(from != CRef_Undef){ // Desicion variable
                            // Check duplicattion
                            //for(int i = 0; i < respons_clauses.size(); ++i){
                            //    if(from == respons_clauses[i])
                            //        break;
                            //    if(i == respons_clauses.size() - 1)
                            //        respons_clauses.push(from);
                            //}
                            respons_clauses.push(from);
                        }
                    }
#ifdef UIP_TRAIL
                    printf("    Put %c%c in out_learnt.\n", sign(q)? '-' : ' ', v2c(var(q)));
                    printf("    %c%c is implied from clause = {", sign(q)? '-' : ' ', v2c(var(q)));
                    const Clause& fromc = ca[from];
                    for(int i = 0; i < fromc.size(); ++i){
                        Lit k = fromc[i];
                        printf(" %c%c", sign(k)? '-' : ' ', v2c(var(k)));
                        if(i != fromc.size() - 1)
                            printf(",");
                        else
                            printf(" }\n");
                    }
#endif                    
                }
            }
        }
        
        // Select next clause to look at:
        while(!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

        if(vbs){
            if(confl != CRef_Undef){
                // Check duplication
                //for(int i = 0; i < respons_clauses.size(); ++i){
                //    if(confl == respons_clauses[i])
                //        break;
                //    if(i == respons_clauses.size() - 1)
                //        respons_clauses.push(confl);
                //}
                respons_clauses.push(confl);
            }
        }
#ifdef UIP_TRAIL
        printf("    pathC-- = %d\n", pathC);
        printf("Select %c%c from clause = {", sign(p)? '-' : ' ', v2c(var(p)));
        const Clause& conflc = ca[confl];
        for(int i = 0; i < conflc.size(); ++i){
            Lit k = conflc[i];
            printf(" %c%c", sign(k)? '-' : ' ', v2c(var(k)));
            if(i != conflc.size() - 1)
                printf(",");
            else
                printf(" }\n");
        }
#endif
    }while(pathC > 0);
    out_learnt[0] = ~p;
#ifdef UIP_TRAIL
    printf("Put %c%c in out_learnt.\n", sign(~p)? '-' : ' ', v2c(var(~p)));
#endif

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if(ccmin_mode == 2){
        uint32_t abstract_level = 0;
        for(i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for(i = j = 1; i < out_learnt.size(); i++)
            if(reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
        
    }
    else if(ccmin_mode == 1){
        for(i = j = 1; i < out_learnt.size(); i++){
            Var x = var(out_learnt[i]);

            if(reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else{
                Clause& c = ca[reason(var(out_learnt[i]))];
                for(int k = 1; k < c.size(); k++)
                    if(!seen[var(c[k])] && level(var(c[k])) > 0){
                        out_learnt[j++] = out_learnt[i];
                        break; 
                    }
            }
        }
    }
    else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if(out_learnt.size() == 1)
        out_btlevel = 0;
    else{
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for(int i = 2; i < out_learnt.size(); i++)
            if(level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level(var(p));
    }

    for(int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels){
    analyze_stack.clear(); analyze_stack.push(p);
    int top = analyze_toclear.size();
    while(analyze_stack.size() > 0){
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause& c = ca[reason(var(analyze_stack.last()))]; analyze_stack.pop();

        for(int i = 1; i < c.size(); i++){
            Lit p  = c[i];
            if(!seen[var(p)] && level(var(p)) > 0){
                if(reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }
                else{
                    for(int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}

/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|  
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/

void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict){
    out_conflict.clear();
    out_conflict.push(p);

    if(decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for(int i = trail.size()-1; i >= trail_lim[0]; i--){
        Var x = var(trail[i]);
        if(seen[x]){
            if(reason(x) == CRef_Undef){
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }
            else{
                Clause& c = ca[reason(x)];
                for(int j = 1; j < c.size(); j++)
                    if(level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}

void Solver::uncheckedEnqueue(Lit p, CRef from){
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool( !sign(p) );
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);
//    printf("uncheckedEnqueue literal %c%d from clause %d\n", sign(p) ? '-' : ' ', var(p), from);
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|  
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|  
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/

CRef Solver::propagate(){
    CRef    confl     = CRef_Undef;
    int     num_props = 0;
    watches.cleanAll();

//    if(okBCP)  printf("Start propagation() ...\n");

    while(qhead < trail.size()){
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Watcher>&  ws  = watches[p];
        Watcher        *i, *j, *end;
        num_props++;

//        if(okBCP) printf("  Propagating %c%d ...\n", sign(p)? '-' : ' ', var(p));
#ifdef UIP_TRAIL        
        printf("  Propagating %c%c ...\n", sign(p)? '-' : ' ', v2c(var(p)));
#endif        
        for(i = j = (Watcher*)ws, end = i + ws.size();  i != end;){
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if(value(blocker) == l_True){
                *j++ = *i++; 
                continue; 
            }

            // Make sure the false literal is data[1]:
            CRef     cr        = i->cref;
            Clause&  c         = ca[cr];
            Lit      false_lit = ~p;
            if(c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit     first = c[0];
            Watcher w     = Watcher(cr, first);
            if(first != blocker && value(first) == l_True){
                *j++ = w; 
                continue; 
            }

            // Look for a new literal to watch:
            for(int k = 2; k < c.size(); k++){
                if(value(c[k]) != l_False){
                    c[1] = c[k]; 
                    c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause; 
                }
            }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if(value(first) == l_False){
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while(i < end)
                    *j++ = *i++;
            }
            else{
                uncheckedEnqueue(first, cr);
                //if((int)conflicts < BCPGuide_T)
                if(okBCP)
                    BCPGuide_partal_assigns.push(first);
            }

            NextClause:;
        }

        ws.shrink(i - j);
    }

    propagations += num_props;
    simpDB_props -= num_props;
#ifdef UIP_TRAIL
    if(confl != CRef_Undef){
        printf("  propagate() return confl = {");
        const Clause& conflc = ca[confl];
        for(int i = 0; i < conflc.size(); ++i){
            Lit p = conflc[i];
            printf(" %c%c", sign(p)? '-' : ' ', v2c(var(p)));
            if(i != conflc.size() - 1)
                printf(",");
            else
                printf(" }\n");
        }
    }
#endif
    return confl;
}

/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|  
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/

struct reduceDB_lt{ 
    ClauseAllocator& ca;
    reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) { 
        return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); 
    }
};

void Solver::reduceDB(){
	printf("Calling reduceDB() ...\n");
    int     i, j;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

    sort(learnts, reduceDB_lt(ca));
    // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
    // and clauses with activity smaller than 'extra_lim':
    for(i = j = 0; i < learnts.size(); i++){
        Clause& c = ca[learnts[i]];
        if(c.size() > 2 && !locked(c) && (i < learnts.size() / 2 || c.activity() < extra_lim))
            removeClause(learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
    checkGarbage();
}

void Solver::removeSatisfied(vec<CRef>& cs){
    int i, j;
    for(i = j = 0; i < cs.size(); i++){
        Clause& c = ca[cs[i]];
        if(satisfied(c))
            removeClause(cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}

void Solver::rebuildOrderHeap(){
    vec<Var> vs;
    for(Var v = 0; v < nVars(); v++)
        if(decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}

/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|  
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/

bool Solver::simplify(){
    assert(decisionLevel() == 0);

    if(!ok || propagate() != CRef_Undef)
        return ok = false;

    if(nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if(remove_satisfied)        // Can be turned off.
        removeSatisfied(clauses);
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    return true;
}

/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|  
|  Description:
|    Search for a model the specified number of conflicts. 
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|  
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/

lbool Solver::search(int nof_conflicts){
    assert(ok);
    int         backtrack_level;
    int         conflictC = 0;
    vec<Lit>    learnt_clause;
    starts++;

    bool        initPropagate   = true;     // Determine the first propagation
    bool        propagateLearnt = false;    // Determine whether the propagation is caused by conflict learning
    Lit         gLit = lit_Undef;
    int         BCPlevel = -1; 
    double      score1 = -1.0, score2 = -1.0;

    okBCP       = false;

    for(;;){

        okBCP = (!initPropagate && !propagateLearnt && (num_models >= 1) && (bcp_conflict < BCPGuide_T));
        //okBCP = (!initPropagate && !propagateLearnt && (num_models >= 1) && (BCPCounts < BCPGuide_T));

        if(okBCP){
            gLit = trail.last();
            BCPlevel = decisionLevel() - 1;         // Record level for cancel
            BCPGuide_partal_assigns.clear();
            BCPGuide_partal_assigns.push(gLit);     // Partial assigns will be updated during propagate()
        }

        CRef confl = propagate();

        if(okBCP && (confl == CRef_Undef)){ // No conflict
//            printf("BCP Guide, measuring literal %c%d ...\n", sign(gLit)? ' ' : '-', var(gLit));
            score1 = BCPQuality();
//            int score = 0;
//            printf("Size of BCPGuide_partal_assigns = %d\n", BCPGuide_partal_assigns.size());
//            for(int t = 0; t < BCPGuide_partal_assigns.size();t++){
//                Var r  = var(BCPGuide_partal_assigns[t]);
//                bool f = sign(BCPGuide_partal_assigns[t]);
//                score +=  f ? ((varStat[r][0] + 1) * varStat[r][1]) : (varStat[r][0] * (varStat[r][1] + 1)) ;
//                printf("var %d assigned %d, previous pos = %d, neg = %d, score = %d, partial quality = %d\n", r, f ? 0 : 1, varStat[r][1], varStat[r][0],
//                         f ? ((varStat[r][0] + 1) * varStat[r][1]) : (varStat[r][0] * (varStat[r][1] + 1)), score);
//            }
//
//            printf("score1 = %f\n", score1);
            cancelUntil(BCPlevel);
//            for(int t = 0; t < BCPGuide_partal_assigns.size(); t++)
//                assert(assigns[var(BCPGuide_partal_assigns[t])] == l_Undef);
            // Check opposite polarity
            newDecisionLevel();
            uncheckedEnqueue(~gLit);

            BCPlevel = decisionLevel() - 1;
            BCPGuide_partal_assigns.clear();
            BCPGuide_partal_assigns.push(~gLit);

            confl = propagate();
            if(confl == CRef_Undef){ // No conflict
                score2 = BCPQuality();
//                printf("Size of BCPGuide_partal_assigns = %d\n", BCPGuide_partal_assigns.size());
                if(score1 > score2){ // Revert to first polarity
//                    printf("==================================================\n");
//                    printf("%c%d score1 = %f, %c%d score2 = %f\n",  sign(gLit)? ' ' : '-', var(gLit), score1,  sign(~gLit)? ' ' : '-', var(~gLit), score2);
//                    //for(int t=0;t<BCPGuide_partal_assigns.size();t++){
//                    //    Var r  = var(BCPGuide_partal_assigns[t]);
//                    //    bool f = sign(BCPGuide_partal_assigns[t]);
//                    //    printf("varStat[%d][0] = %d, varStat[%d][1] = %d, choose %c%d\n", r, varStat[r][0], r, varStat[r][1], f ? '-' : ' ', r);
//                    //}
//                    printf("==================================================\n");
                    cancelUntil(BCPlevel);
                    newDecisionLevel();
                    uncheckedEnqueue(gLit);
                    confl = propagate();
                    BCPCounts++;
                    assert(confl == CRef_Undef);
                }
            }
        }

        if(confl != CRef_Undef){  // CONFLICT
            propagateLearnt = true;
            if(!initPropagate)  bcp_conflict++;
            //printf("CONFLICT!\n");
            conflicts++; conflictC++;
            if(decisionLevel() == 0) 
      	        return l_False;

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level);
            cancelUntil(backtrack_level);

#ifdef UIP_TRAIL 
            printf("learnt_clause = {");
            for(int i = 0; i < learnt_clause.size(); ++i){
                Lit k = learnt_clause[i];
                printf(" %c%c", sign(k)? '-' : ' ', v2c(var(k)));
                if(i != learnt_clause.size() - 1)
                    printf(",");
                else
                    printf(" }\n");
            }
#endif            
            // Update lcv, gcv
            if(vbs){
                for(int i = 0; i < respons_clauses.size(); ++i){
                    const Clause& c = ca[respons_clauses[i]];
                    for(int j = 0; j < c.size(); ++j){
                        Lit lt = c[j];
                        lcvUpdate(lt);
                        gcvUpdate(lt);
                    }
                }
            }

            if(learnt_clause.size() == 1){
                uncheckedEnqueue(learnt_clause[0]);
            }
            else{
                CRef cr = ca.alloc(learnt_clause, true);  // CRef alloc(const Lits& ps, bool learnt = false)
                learnts.push(cr);
                attachClause(cr);
                claBumpActivity(ca[cr]);
                uncheckedEnqueue(learnt_clause[0], cr);
                
                // Update CBH_list
                if(vbs) CBH_update(cr);
            }

            varDecayActivity();
            claDecayActivity();
            if(vbs)  lclDecay();

            if(--learntsize_adjust_cnt == 0){
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt    = (int)learntsize_adjust_confl;
                max_learnts             *= learntsize_inc;

                if(verbosity >= 1)
                    printf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n", 
                          (int)conflicts, 
                          (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals, 
                          (int)max_learnts, nLearnts(), (double)learnts_literals/nLearnts(), progressEstimate()*100);
            }

        }
        else{  // NO CONFLICT
            propagateLearnt = false;
            initPropagate   = false;

            if(nof_conflicts >= 0 && conflictC >= nof_conflicts || !withinBudget()){
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef; 
            }

            // Simplify the set of problem clauses:
            if(decisionLevel() == 0 && !simplify())
                return l_False;

            if(learnts.size()-nAssigns() >= max_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();

            Lit next = lit_Undef;
            while(decisionLevel() < assumptions.size()){
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if(value(p) == l_True){
                    // Dummy decision level:
                    newDecisionLevel();
                }
                else if(value(p) == l_False){
                    analyzeFinal(~p, conflict);
                    return l_False;
                }
                else{
                    next = p;
                    break;
                }
            }

            if(next == lit_Undef){
                // New variable decision:
                decisions++;
#ifdef UIP_TRAIL
                if(decisions==1)
                    next = mkLit(8, true);
                else if(decisions==2)
                    next = mkLit(0, true);
                else if(decisions==3)
                    next = mkLit(3, true);
                else
#endif                        
                next = pickBranchLit();

                if(next == lit_Undef)
                    // Model found:
                    return l_True;
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}

double Solver::progressEstimate() const{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for(int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

void Solver::varStatUpdate(){
//    printf("varStatUpdate() ... \n");
    for(int i = 0; i < nVars(); i++){
        assert( varStat[i][1] >= 0 && varStat[i][1] < INT_MAX);
        assert( varStat[i][0] >= 0 && varStat[i][0] < INT_MAX);
        assert(model[i] != l_Undef);
        if(model[i] == l_True) varStat[i][1] =  varStat[i][1] + 1;
        else varStat[i][0] =  varStat[i][0] + 1;
    }
    num_models++;
}

/*
  Finite subsequences of the Luby-sequence:

  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...


 */

static double luby(double y, int x){
    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for(size = 1, seq = 0; size < x+1; seq++, size = 2*size+1)
        ;

    while(size-1 != x){
        size = (size-1)>>1;
        seq--;
        x = x % size;
    }

    return pow(y, seq);
}

// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_(){
    model.clear();
    conflict.clear();
    if(!ok) return l_False;

    solves++;

    max_learnts             = nClauses() * learntsize_factor;
    learntsize_adjust_confl = learntsize_adjust_start_confl;
    learntsize_adjust_cnt   = (int)learntsize_adjust_confl;
    lbool status            = l_Undef;

    conflicts               = 0;
    BCPCounts               = 0; 
    bcp_conflict            = 0;
    lcl_decay_counts        = 0;

    CBH_build_time          = 0;
    CBH_reloc_time          = 0;
    CBH_update_time         = 0;
    vRandLoc_time           = 0;

    double initial_time = cpuTime();
    if(vbs && (num_models == 0)) CBH_init();
    double init_end_time = cpuTime();

    printf("Solving model #%d\n", (int) num_models);

    if(verbosity > 0){
        printf("============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
    }

    // Search:
    int curr_restarts = 0;
    while(status == l_Undef){
        double rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
        status = search((int)rest_base * restart_first);
        if(!withinBudget()) break;
        curr_restarts++;
    }

    if(verbosity > 0)
        printf("===============================================================================\n");


    if(status == l_True){
        // Extend & copy model:
        model.growTo(nVars());
        for(int i = 0; i < nVars(); i++) model[i] = value(i);
        // Update var polarity statistics
        varStatUpdate();
    }
    else if(status == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);

//  printf("Number of BCPCounts = %d\n", BCPCounts);

    double end_time = cpuTime();

    printf("    Model #%d solving time = %.2f\n", (int) num_models, end_time - initial_time);
    if(vbs && (num_models == 1)) printf("    CBH_init_time = %.2f\n", init_end_time - initial_time);
    if(vbs) printf("    CBH_reloc_time = %.10f\n", CBH_reloc_time);
    if(vbs) printf("    CBH_update_time = %.10f\n", CBH_update_time);
    if(vbs) printf("    vRandLoc_time = %.10f\n", vRandLoc_time);

    return status;
}

//=================================================================================================
// Writing CNF to DIMACS:
// 
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max){
    if(map.size() <= x || map[x] == -1){
        map.growTo(x+1, -1);
        map[x] = max++;
    }
    return map[x];
}

void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max){
    if(satisfied(c)) return;

    for(int i = 0; i < c.size(); i++)
        if(value(c[i]) != l_False)
            fprintf(f, "%s%d", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max)+1);
    fprintf(f, "0\n");
}

void Solver::toDimacs(const char *file, const vec<Lit>& assumps){
    FILE* f = fopen(file, "wr");
    if(f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}

void Solver::toDimacs(FILE* f, const vec<Lit>& assumps){
    // Handle case when solver is in contradictory state:
    if(!ok){
      fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
      return; 
    }

    vec<Var> map; Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for(int i = 0; i < clauses.size(); i++)
      if(!satisfied(ca[clauses[i]]))
        cnt++;
        
    for(int i = 0; i < clauses.size(); i++)
      if(!satisfied(ca[clauses[i]])){
        Clause& c = ca[clauses[i]];
          for(int j = 0; j < c.size(); j++)
            if(value(c[j]) != l_False)
              mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for(int i = 0; i < assumptions.size(); i++){
      assert(value(assumptions[i]) != l_False);
      fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max)+1);
    }

    for(int i = 0; i < clauses.size(); i++)
      toDimacs(f, ca[clauses[i]], map, max);

    if(verbosity > 0)
      printf("Wrote %d clauses with %d variables.\n", cnt, max);
}

//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to){
    // All watchers:
    //
    // for(int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    for(int v = 0; v < nVars(); v++)
        for(int s = 0; s < 2; s++){
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher>& ws = watches[p];
            for(int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
        }

    // All reasons:
    //
    for(int i = 0; i < trail.size(); i++){
        Var v = var(trail[i]);

        if(reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for(int i = 0; i < learnts.size(); i++){
        CRef cr = learnts[i];
        ca.reloc(learnts[i], to);
        CRef newCr = learnts[i];
        if(vbs) CBH_reloc(cr, newCr);
    }
    // All original:
    //
    for(int i = 0; i < clauses.size(); i++){
        CRef cr = clauses[i];
        ca.reloc(clauses[i], to);
        CRef newCr = clauses[i];
        if(vbs) CBH_reloc(cr, newCr);
    }
}

void Solver::garbageCollect(){
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    printf("\ngarbageCollect()!\n\n");
    ClauseAllocator to(ca.size() - ca.wasted()); 

    relocAll(to);
    if(verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n", 
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

char Solver::v2c(Var v){
    switch(v){
        case(0): return 'p';
        case(1): return 'q';
        case(2): return 'r';
        case(3): return 's';
        case(4): return 't';
        case(5): return 'v';
        case(6): return 'w';
        case(7): return 'x';
        case(8): return 'y';
    }
    return ' ';
}

#ifndef INCLUDED_GLOBAL
#define INCLUDED_GLOBAL

#include <string>
#include <sstream>

using namespace std;

class Global{
    private:
        // NOT IMPLEMENTED
        Global();
      
    public:
        // GLOBAL PARAMETERS
        enum {STR_LENGTH = 1024, AUTO_BIN_MAX = 100000, AT_LEAST = 10};
        
        // ltNumber is used to represent literal of SAT instance
        // it is recorded in Wire and CoverVariable
        //int ltNumber;
        //int clauseNumber;
        
        static void eraseBlanks(string* str_p){
            size_t found;
            string::iterator it;
      
            while((found = str_p->find(' ')) != string::npos){
                it = str_p->begin() + found;
                str_p->erase(it);
            }
        }

        static string int2str(const int i){
            stringstream ss;
            ss << i;
            return ss.str();
        }

        static int str2int(const string& str){
            int i;
            stringstream ss(str);
            ss >> i;
            return i;
        }
};

#endif

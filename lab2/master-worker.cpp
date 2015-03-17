#include <iostream>
#include <vector>
#include <mpi.h>
#include <math.h>
#include <gmpxx.h>
#include "master-worker.h"

using namespace std;

#define PRE_ASSIGN false
#define NUM 10000000000000000
#define GRAN 100000
#define NUM_SIZE 32

struct FACTOR_WORK {
    char start[NUM_SIZE];
    int startLen, offset;
};

struct FACTOR_RESULT {
    char data[GRAN][NUM_SIZE];
    int size;
};

class MW: public MasterWorker<FACTOR_WORK, FACTOR_RESULT> {
public:
    
    MW(int c, char **v, int gran, mpz_class pNum): MasterWorker<FACTOR_WORK, FACTOR_RESULT>(c, v, gran) {
        num_ = pNum;
    }

    void createWork() {
        mpz_class start = 2, end = sqrt(num_);
        cout << "num: " << num_ << endl;
        
        if (PRE_ASSIGN) {
            cout << "pre-assign: yes" << endl;
            mpz_class len = end/(size_-1);
            for (int i=0; i<(size_-1); i++) {
                FACTOR_WORK work;
            
                string startStr = start.get_str();
                for (int i=0; i<startStr.length(); i++) {
                    work.start[i] = startStr[i];
                }
                work.start[startStr.length()] = '\0';
                work.startLen = startStr.length();
                work.offset = -1;

                workPool_.push_back(work);
                // debugOutputWork(work);
                
                start += len;
            }
            
        } else {
            cout << "pre-assign: no" << endl;
            cout << "gran: " << granularity_ << endl;
            
            mpz_class remainMpz = (end-start + 1) % granularity_;
            int remain = (int) remainMpz.get_si();
            while (start <= end) {
                FACTOR_WORK work;
            
                string startStr = start.get_str();
                for (int i=0; i<startStr.length(); i++) {
                    work.start[i] = startStr[i];
                }
                work.start[startStr.length()] = '\0';
                work.startLen = startStr.length();
            
                work.offset = granularity_;
                if (remain > 0) {
                    work.offset++;
                    remain--;
                }
                start += work.offset;
                workPool_.push_back(work);
                // debugOutputWork(work);
            }
        }
        cout << "try: " << 2 << ' ' << end << endl;
    }
    
    FACTOR_RESULT compute(FACTOR_WORK work) {
        FACTOR_RESULT ret;
        int retSize = 0;
        mpz_class start, end;

        if (PRE_ASSIGN) {
            start.set_str(work.start, 10);
            mpz_class partSize(sqrt(num_)/(size_-1));
            if (start + partSize > sqrt(num_))  {
                end = sqrt(num_)+1;
            } else {
                end = start + partSize;
            }
            // cout << "compute " << start << ' ' << end << endl;
            
        } else {
            start.set_str(work.start, 10);
            end = start + work.offset;
        }
        
        for (; start < end; start++) {
            if (num_ % start == 0) {

                string startStr = start.get_str();
                for (int i=0; i<startStr.length(); i++) {
                    ret.data[retSize][i] = startStr[i];
                }
                ret.data[retSize][startStr.length()] = '\0';
                
                retSize++;
            }
        }
        ret.size = retSize;
        // debugOutputResult(ret);
        return ret;
    }

    void result(vector<FACTOR_RESULT> resultData) {
        for (int i=0; i<resultData.size(); i++) {
            for (int j=0; j<resultData[i].size; j++) {
                mpz_class ans;
                ans.set_str(resultData[i].data[j], 10);
                // cout << ans << ' ' << num_ / ans << endl;
                // cout << num_ / ans << endl;
            }
            // debugOutputResult(resultData[i]);
        }
        cout << "Done" << endl;
    }

    // for debug
    void debugOutputWork(FACTOR_WORK work) {
        cout << "work: ";
        for (int i=0; i<work.startLen; i++) {
            cout << work.start[i];
        }
        cout << ' ' << work.offset << endl;
    }

    // for debug
    void debugOutputResult(FACTOR_RESULT result) {
        for (int i=0; i<result.size; i++) {
            int j = 0;
            while (result.data[i][j] != '\0') {
                cout << result.data[i][j++];
            }
            cout << endl;
        }
    }
    
private:
    mpz_class num_;
};

int main(int argc, char** argv) {
    mpz_class num(NUM);
    MW mw(argc, argv, GRAN, num) ;
    mw.run();
    return 0;
}

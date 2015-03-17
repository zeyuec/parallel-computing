#ifndef MASTER_WORKER_H

#define MASTER_WORKER_H

using namespace std;

template <typename T_WORK,typename T_RESULT>
class MasterWorker {
    
public:
    
    // constructor
    MasterWorker(int pArgc, char** pArgv, int pGranularity) {
        argc_ = pArgc;
        argv_ = pArgv;
        granularity_ = pGranularity;
    }

    // run
    void run() {

        // MPI init
        MPI::Init(argc_, argv_);
        size_ = MPI::COMM_WORLD.Get_size();
        rank_ = MPI::COMM_WORLD.Get_rank();
        
        // set rank
        rankMaster_ = 0;
        rankFirstWorker_ = 1;
        rankLastWorker_ = size_-1;

        // create work and broadcast workPoolSize_
        if (rank_ == rankMaster_) {
            createWork();
            workPoolSize_ = workPool_.size();
        }
        MPI::COMM_WORLD.Bcast(&workPoolSize_, 1, MPI::INT, rankMaster_);

        // common status
        int stop = 0;
        MPI::Status status;
        
        // send work to workers
        if (rank_ == rankMaster_) {

            // send the initial round
            int curWorkIndex = 0;
            for (int dest=rankFirstWorker_; dest<=rankLastWorker_; dest++) {
                MPI::COMM_WORLD.Send(&stop, 1, MPI::INT, dest, 1);
                MPI::COMM_WORLD.Send(&workPool_[curWorkIndex++], 1*sizeof(T_WORK), MPI::BYTE, dest, 0);
            }

            vector<T_RESULT> collectedResults;

            // dynamic send another rounds
            int curDoneWorkIndex = 0;
            while (curDoneWorkIndex < workPoolSize_) {
                
                // get result
                T_RESULT result;
                MPI::COMM_WORLD.Recv(&result, 1*sizeof(T_RESULT), MPI::BYTE, MPI::ANY_SOURCE, 0, status);
                collectedResults.push_back(result);
                curDoneWorkIndex++;
                
                int source = status.Get_source();
                if (curWorkIndex < workPoolSize_) {
                    stop = 0;
                    MPI::COMM_WORLD.Send(&stop, 1, MPI::INT, source, 1);
                    MPI::COMM_WORLD.Send(&workPool_[curWorkIndex++], 1*sizeof(T_WORK), MPI::BYTE, source, 0);
                } else {
                    stop = 1;
                    MPI::COMM_WORLD.Send(&stop, 1, MPI::INT, source, 1);
                }
            }
            result(collectedResults);
        } else {
            while (1) {
                // check if need stop
                stop = 0;
                MPI::COMM_WORLD.Recv(&stop, 1, MPI::INT, rankMaster_, 1, status);
                if (stop == 1) {
                    // cout << rank_ << " stopped" << endl;
                    break;
                }

                // receive work
                T_WORK receivedWork;
                MPI::COMM_WORLD.Recv(&receivedWork, 1*sizeof(T_WORK), MPI::BYTE, rankMaster_, 0, status);

                // send back result
                T_RESULT result = compute(receivedWork);
                MPI::COMM_WORLD.Send(&result, 1*sizeof(T_RESULT), MPI::BYTE, rankMaster_, 0);
            }
        }

        MPI::Finalize();
    }

    // create work
    virtual void createWork() = 0;

    // compute each piece of work
    virtual T_RESULT compute(T_WORK data) = 0; 

    // get final result
    virtual void result(vector<T_RESULT>) = 0;

private:
    // init args
    int argc_;
    char** argv_;
    
    // rank
    int rankMaster_, rankFirstWorker_, rankLastWorker_;

protected:
    vector<T_WORK> workPool_;
    int workPoolSize_;
    int granularity_;
    int size_, rank_;
};

#endif

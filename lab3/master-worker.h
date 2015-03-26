#ifndef MASTER_WORKER_H

#define MASTER_WORKER_H

// if open debug
#define DEBUG_BUILD

// threshold
#define MASTER_FAILURE_THRESHOLD 0.01
#define WORKER_FAILURE_THRESHOLD 0.001

// millisecond
#define MASTER_REQUEST_WAIT_TIME 1 
#define BACKUP_MASTER_REQUEST_WAIT_TIME 1


#include <thread>
#include "data-stack.h"
#include "debug-helper.h"

using namespace std;

template <typename T_JOB,typename T_RESULT>
class MasterWorker {
    
public:
    
    // constructor
    MasterWorker(int pArgc, char** pArgv) {
        argc_ = pArgc;
        argv_ = pArgv;
    }

    // run
    void run() {

        // MPI init
        MPI::Init(argc_, argv_);
        size_ = MPI::COMM_WORLD.Get_size();
        rank_ = MPI::COMM_WORLD.Get_rank();

        // set rank
        rankMaster_ = 0;
        rankMasterBackup_ = 1;

        // create work and broadcast jobSize_
        if (rank_ == rankMaster_ || rank_ == rankMasterBackup_) {
            createWork();
            jobSize_ = jobPool_.size();
        }
        MPI::COMM_WORLD.Bcast(&jobSize_, 1, MPI::INT, rankMaster_);

        // send work to workers
        if (rank_ == rankMaster_ || rank_ == rankMasterBackup_) {
            DBGMSG(cout, jobSize_ << " job generated. ");
            startMaster();
        } else {
            startWorker();
        }
        
        MPI::Finalize();
    }

    // create work
    virtual void createWork() = 0;

    // compute each piece of work
    virtual T_RESULT compute(T_JOB data) = 0; 

    // get final result
    virtual void result(vector<T_RESULT>) = 0;

private:
    // init args
    int argc_;
    char** argv_;
    
    // rank
    int rankMaster_,  rankMasterBackup_, finishedCount_, stoppedWorkerCount_;
    vector<T_RESULT> collectedResults_;

    // struct for the worker status
    struct WorkerStatus {
        WorkerStatus(int r, int c, unsigned long l, bool i): rank(r), curJob(c), lastResponseTime(l), isAlive(i) {}
        int rank, curJob;
        unsigned long lastResponseTime;
        bool isAlive;
    };
    
    // data stack for the available worker and job
    DataStack<int> workerStack_;
    DataStack<int> jobStack_;

    // status
    vector<WorkerStatus> workerStatus_;

    /**
     * Get milliseconds since epoch
     * @return unsigned long milliseconds since epoch
     * @date 03/25/2015
     */
    unsigned long getCurrentMilliseconds() {
        return std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    }

    /**
     * Send a signal to let the worker stop
     * @prarm worker's rank
     * @date 03/25/2015
     */
    void killWorker(int rank) {
        int stop = 1;
        MPI::COMM_WORLD.Isend(&stop, 1, MPI::INT, rank, 1);
    }

    /**
     * Init some private vars for the master and master backup node
     * @date 03/25/2015
     */
    void initMasterData() {
        // init worker queue
        int *workerArray = (int *)malloc(sizeof(int) * size_);
        workerStack_.init(workerArray, size_);
        for (int i=0; i<size_; i++) {
            if (i != rankMaster_ && i != rankMasterBackup_) {
                workerStack_.push(i);
            }
        }
        
        // init job queue
        int *jobArray = (int*)malloc(sizeof(int) * jobSize_);
        jobStack_.init(jobArray, jobSize_);
        for (int i=0; i<jobSize_; i++) {
            jobStack_.push(i);
        }

        // init worker status vector
        for (int i=0; i<size_; i++) {
            WorkerStatus ws(i, -1, 0, true);
            workerStatus_.push_back(ws);
        }

        // init results
        T_RESULT emptyResult;
        for (int i=0; i<jobSize_; i++) {
            collectedResults_.push_back(emptyResult);
        }
    }

    /**
     * Send the status of the master node to backup-master node, call by master node
     * @date 03/25/2015
     */
    void backupMasterSend() {
        int h, *data;

        // send worker stack        
        h = workerStack_.getHeight();
        data = workerStack_.getData();
        MPI::COMM_WORLD.Isend(&h, 1, MPI::INT, rankMasterBackup_, 800);
        MPI::COMM_WORLD.Isend(data, size_, MPI::INT, rankMasterBackup_, 801);

        // send job stack
        h = jobStack_.getHeight();
        data = jobStack_.getData();
        MPI::COMM_WORLD.Isend(&h, 1, MPI::INT, rankMasterBackup_, 802);
        MPI::COMM_WORLD.Isend(data, jobSize_, MPI::INT, rankMasterBackup_, 803);

        // send worker status
        MPI::COMM_WORLD.Isend(&workerStatus_[0], 1*sizeof(WorkerStatus)*size_, MPI::BYTE, rankMasterBackup_, 804);

        // send resultCount
        MPI::COMM_WORLD.Isend(&finishedCount_, 1, MPI::INT, rankMasterBackup_, 805);

        // send result
        MPI::COMM_WORLD.Isend(&collectedResults_[0], 1*sizeof(T_RESULT)*jobSize_, MPI::BYTE, rankMasterBackup_, 806);

        // send stoppedWorkerCount
        MPI::COMM_WORLD.Isend(&stoppedWorkerCount_, 1, MPI::INT, rankMasterBackup_, 807);
    }

    /**
     * Receive status from master node, call by backup-master node
     * @return 03/25/2015
     */
    void backupMasterReceive() {
        MPI::Request reqWorkerHeight, reqWorkerData, reqJobHeight,
            reqJobData, reqWorkerStatusData, reqFinishedCount, reqCollectedResults, reqStoppedWorkerCount;

        unsigned long lastBackupTime = 0;
        bool masterFail = false;
        while (!masterFail) {
            reqWorkerHeight = MPI::COMM_WORLD.Irecv(&(workerStack_.h_), 1, MPI::INT, rankMaster_, 800);
            reqWorkerData = MPI::COMM_WORLD.Irecv(workerStack_.data_, size_, MPI::INT, rankMaster_, 801);

            reqJobHeight = MPI::COMM_WORLD.Irecv(&(jobStack_.h_), 1, MPI::INT, rankMaster_, 802);
            reqJobData = MPI::COMM_WORLD.Irecv(jobStack_.data_, jobSize_, MPI::INT, rankMaster_, 803);

            reqWorkerStatusData = MPI::COMM_WORLD.Irecv(&workerStatus_[0], 1*sizeof(WorkerStatus)*size_, MPI::BYTE, rankMaster_, 804);
            
            reqFinishedCount = MPI::COMM_WORLD.Irecv(&finishedCount_, 1, MPI::INT, rankMaster_, 805);
            reqCollectedResults = MPI::COMM_WORLD.Irecv(&collectedResults_[0], 1*sizeof(T_RESULT)*jobSize_, MPI::BYTE, rankMaster_, 806);

            reqStoppedWorkerCount = MPI::COMM_WORLD.Irecv(&stoppedWorkerCount_, 1, MPI::INT, rankMaster_, 807);

            while (!reqWorkerHeight.Test() || !reqWorkerData.Test() ||
                   !reqJobHeight.Test() || !reqJobData.Test() ||
                   !reqWorkerStatusData.Test() ||  
                   !reqFinishedCount.Test() || !reqCollectedResults.Test() ||
                   !reqStoppedWorkerCount.Test()
                ) {
                
                std::chrono::milliseconds waitime(BACKUP_MASTER_REQUEST_WAIT_TIME); 
                std::this_thread::sleep_for(waitime);
                if (lastBackupTime != 0 && getCurrentMilliseconds() - lastBackupTime > 1000) {
                    DBGMSG(cout, "Switching to backup-master node");
                    masterFail = true;
                    break;
                }
            }
            lastBackupTime = getCurrentMilliseconds();
        }
        
        // @TODO make sure every in-computing job can be computed, this will increase some duplicate computations, can be better 
        for (int i=0; i<size_; i++) {
            if (i != rankMaster_ && i != rankMasterBackup_ && workerStatus_[i].isAlive) {
                if (workerStatus_[i].curJob != -1) {
                    jobStack_.push(workerStatus_[i].curJob);
                    workerStack_.push(i);
                    workerStatus_[i].curJob = -1;
                }
            }
        }
        
    }

    /**
     * Master and backup-master node's work
     *
     * For the master node, it will try to backup the status with backupMasterSend() function,
     * for the backup-master node, it will try to receive status.
     * Once it found the master is not sending, the backup-master can keep handling the process
     *
     * @date 03/25/2015
     */
    void startMaster() {

        // init random seed
        srand(getCurrentMilliseconds());

        // some common vars
        MPI::Status status;
        int stop = 0;

        // init data
        initMasterData();
        
        // start loop
        finishedCount_ = 0;
        stoppedWorkerCount_ = 0;

        // loop starts
        bool masterAlreadyFail = false;
        while (finishedCount_ < jobSize_) {

            if (rank_ == rankMaster_) {
                backupMasterSend();
                if (finishedCount_ > 500 && randomFail(MASTER_FAILURE_THRESHOLD*5)) {
                    return;
                }
            } if (rank_ == rankMasterBackup_ && !masterAlreadyFail) { // make sure this only run once
                backupMasterReceive();
                masterAlreadyFail = true;
            }
            
            T_RESULT result;
            MPI::Request request = MPI::COMM_WORLD.Irecv(&result, 1*sizeof(T_RESULT), MPI::BYTE, MPI::ANY_SOURCE, 0);
            MPI::Status status;
            while (!request.Test(status)) {
                if (!workerStack_.isEmpty() && !jobStack_.isEmpty()) {
                    int workerRank = workerStack_.pop();
                    int jobId = jobStack_.pop();
                    // DBGMSG(cout, "[send] " << jobId << " => " << workerRank);
                    
                    MPI::COMM_WORLD.Send(&stop, 1, MPI::INT, workerRank, 1);
                    MPI::COMM_WORLD.Send(&jobPool_[jobId], 1*sizeof(T_JOB), MPI::BYTE, workerRank, 0);
                
                    workerStatus_[workerRank].curJob = jobId;
                    workerStatus_[workerRank].lastResponseTime = getCurrentMilliseconds();
                }
        
                for (int i=0; i<size_; i++) {
                    int diff = getCurrentMilliseconds() - workerStatus_[i].lastResponseTime;
                    if (i != rankMaster_ && i != rankMasterBackup_ && workerStatus_[i].isAlive && workerStatus_[i].curJob != -1 && diff > 3000) {
                        DBGMSG(cout, "[test] worker " << i << " is offline, not reponding for " << diff << " , push back its job " << workerStatus_[i].curJob);
                        stoppedWorkerCount_++;
                        killWorker(i);
                        if (stoppedWorkerCount_ == size_-2) {
                            DBGMSG(cout, "[Summary] Computation is not finished. All workers are died.");
                            return;
                        }
                        jobStack_.push(workerStatus_[i].curJob);
                        workerStatus_[i].isAlive = false;
                    }
                }
                
                std::chrono::milliseconds waitime(MASTER_REQUEST_WAIT_TIME); 
                std::this_thread::sleep_for(waitime);
            }

            int sourceWorker = status.Get_source();
            // DBGMSG(cout, "[receive] result from " << sourceWorker << " did " << workerStatus_[sourceWorker].curJob);
            if (workerStatus_[sourceWorker].isAlive) {
                collectedResults_[workerStatus_[sourceWorker].curJob] = result;
                workerStack_.push(sourceWorker);
                workerStatus_[sourceWorker].lastResponseTime = getCurrentMilliseconds();
                workerStatus_[sourceWorker].curJob = -1;
                finishedCount_++;
            } else {
                DBGMSG(cout, "[receive] abnormal computing time, throw result");
            }
        }
        
        cout << "[Summary] " << stoppedWorkerCount_ << " workers are died " << endl;
        
        for (int i=0; i<size_; i++) {
            if (i != rankMaster_ && i!= rankMasterBackup_ && workerStatus_[i].isAlive) {
                killWorker(i);
            }
        }
        
        result(collectedResults_);
    }

    /**
     * Worker's work
     * 
     * A Worker will try to receive a stop sign every time, once it receives a work,
     * it will do the computation and send the result back to the source node(can be the master or backup-master node)
     *
     * @date 03/25/2015
     */
    void startWorker() {
        srand(getCurrentMilliseconds()*rank_*997);
        int stop = 0;
        MPI::Status status;
        
        while (1) {
            // try receive stop sign
            stop = 0;
            MPI::COMM_WORLD.Recv(&stop, 1, MPI::INT, MPI::ANY_SOURCE, 1, status);
            if (stop == 1) {
                return;
            }
            
            // receive work
            T_JOB receivedWork;
            MPI::COMM_WORLD.Recv(&receivedWork, 1*sizeof(T_JOB), MPI::BYTE, MPI::ANY_SOURCE, 0, status);

            // send back result
            int start = getCurrentMilliseconds();
            T_RESULT result = compute(receivedWork);
            int end = getCurrentMilliseconds();
            // cout << "[worker] " << rank_ << " used " << end-start << endl;
            
            if (randomFail(WORKER_FAILURE_THRESHOLD)) {
                DBGMSG(cout, "[Fail] worker " << rank_);
                return;
            } else {
                MPI::COMM_WORLD.Isend(&result, 1*sizeof(T_RESULT), MPI::BYTE, status.Get_source(), 0);
            }
        }
    }

    /**
     * Random fail function
     * @return if the random number is in the threshold
     * @date 03/25/2015
     */
    inline bool randomFail(double threshold) {
        double p = rand() / (double)RAND_MAX;
        if (p < threshold) {
            return true;
        } else {
            return false;
        }
    }


protected:

    // size and rank
    int size_, rank_;
    
    // data holder
    vector<T_JOB> jobPool_;
    int jobSize_;

};

#endif

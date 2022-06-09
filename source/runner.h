#pragma once

#include <mutex>
#include <queue>
#include <functional>

#include "protocol/protocol.h"

struct Result
{
    std::string res;
    enum Priority
    {
        S, A, B, C
    }priority;
};

class runner
{
    std::mutex _ContextMutex, _ResultMutex;
    std::priority_queue < Context*, std::vector<Context*>,
        decltype([](Context* left, Context* right) {return left->priority < right->priority; }) > ContextConsumer;

    std::priority_queue < Result, std::vector<Result>,
        decltype([](Result& left, Result& right) {return left.priority < right.priority; }) > ResultProducer;

    void MainThread();
    std::condition_variable _cv;
    std::mutex _cv_m;


    std::function<int()> _worker;
    std::function<void()> _noticer;
    volatile bool isNetworkModuleRunning;

public:
    runner(std::function<int()>&& worker);
    ~runner();

};
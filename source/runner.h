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
    std::priority_queue < std::unique_ptr<Context>, std::vector<std::unique_ptr<Context>>,
        decltype([](const std::unique_ptr<Context>& left, const std::unique_ptr<Context>& right) {return std::less<int>{}(left->priority, right->priority); }) > ContextConsumer;

    std::priority_queue < Result, std::vector<Result>,
        decltype([](const Result& left, const  Result& right) {return std::less<int>{}(left.priority, right.priority); }) > ResultProducer;

    void MainThread();
    std::condition_variable _cv;
    std::mutex _cv_m;

    std::function<int(const std::unique_ptr<Context>&)> _worker;
    std::function<void()> _noticer;
    volatile bool isNetworkModuleRunning;

public:
    runner(std::function<int(const std::unique_ptr<Context>&)>&& worker);
    ~runner();

    void AddToConsumer(const Context& ctx);
    void LockConsumer();
    void UnlockConsumer();

};
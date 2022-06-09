#include <thread>
#include <mutex>

#include "runner.h"

runner::runner(std::function<int()>&& worker)
    : _worker(worker)
    , isNetworkModuleRunning(true)
{
    std::thread(&runner::MainThread, this).detach();
}

runner::~runner()
{
    isNetworkModuleRunning = false;
    _cv.notify_all();
}

void runner::MainThread()
{
    std::unique_lock<std::mutex> lock(_cv_m);
    while (isNetworkModuleRunning)
    {
        Context* ctx = nullptr;
        {
            std::scoped_lock<std::mutex> lock(_ContextMutex);
            ctx = ContextConsumer.top();
            ContextConsumer.pop();
        }

        if (ctx != nullptr)
        {
            Result res;
            //alloc_res(ctx);
            int err = _worker();
            //res.set(err);

            //ResultProducer.emplace(res);

            std::scoped_lock<std::mutex> lock(_ResultMutex);
            ResultProducer.emplace(std::move(res));
        }

        _cv.wait(lock,
            [this]()->bool
            {
                return !isNetworkModuleRunning
                    || !ContextConsumer.empty();
            });
    }
}
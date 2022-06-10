#include <thread>
#include <mutex>

#include "runner.h"

runner::runner(std::function<int(const std::unique_ptr<Context>&)>&& worker)
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
        std::unique_ptr<Context> ctx;
        {
            std::scoped_lock<std::mutex> lock(_ContextMutex);
            ctx = std::move(const_cast<std::unique_ptr<Context>&>(ContextConsumer.top()));
            ContextConsumer.pop();
        }

        if (ctx != nullptr)
        {
            Result res;
            //alloc_res(ctx);
            
            //std::count<<"transfer in "<< reinterpret_cast<std::unique_ptr<DLNAContext>>(ctx->priv_data).
            int err = _worker(ctx);
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

void runner::LockConsumer()
{
    _ContextMutex.lock();
}

void runner::UnlockConsumer()
{
    _ContextMutex.unlock();
}

void runner::AddToConsumer(const Context& ctx)
{
    std::scoped_lock<std::mutex> lock(_ContextMutex);
    ContextConsumer.emplace(std::make_unique<Context>(ctx));
}


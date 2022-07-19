#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <concepts>
#include <iostream>

#include "protocol/protocol.h"

struct Result
{
    std::string res;
    enum Priority
    {
        S, A, B, C
    }priority;
};


template <typename Fn, typename ...Args>
    requires std::invocable<Fn, Args...>&& std::disjunction_v<std::is_same<Context*, Args>... >
class runner
{
    using Task = std::tuple<Args...>;

public:
    runner(Fn&& worker)
        : _worker(worker)
        , isNetworkModuleRunning(true)
    {
        std::thread(&runner::MainThread, this).detach();
    }

    runner(const runner&) = delete;
    runner& operator=(const runner&) = delete;

    runner(runner&&) = delete;


    ~runner()
    {
        isNetworkModuleRunning = false;
        _cv.notify_all();
    }

    Task CreateTask(Args&&... args)
    {
        if constexpr (!std::conjunction_v<std::is_copy_constructible<Args>...>)
        {
            Task dst;
            [&] <std::size_t... N>(std::index_sequence<N...>)
            {
                ((std::get<N>(dst) = std::move(std::get<N>(std::forward_as_tuple(args...)))), ...);
            }(std::make_index_sequence<sizeof...(Args)>{});
            return dst;
        }
        else
        {
            return std::make_tuple(args...);
        }
    }

    void AddToConsumer(Args&&... args)
    {
        std::scoped_lock<std::mutex> lock(_ContextMutex);
        TaskConsumer.emplace(std::forward_as_tuple(args...));
        _cv.notify_all();
    }

    bool IsInitialized()
    {
        return isNetworkModuleRunning;
    }

private:

    void MainThread()
    {
        std::unique_lock<std::mutex> lock(_cv_m);
        while (isNetworkModuleRunning)
        {
            bool hasCurrentTask = false;
            Task currentTask;
            {
                std::scoped_lock<std::mutex> lock(_ContextMutex);
                if (!TaskConsumer.empty())
                {
                    currentTask = std::move(TaskConsumer.top());
                    hasCurrentTask = true;
                    TaskConsumer.pop();
                }
            }

            if (hasCurrentTask)
            {
                Result res;

                std::invoke_result_t<Fn, Args...> err = std::apply(_worker, currentTask);

                std::scoped_lock<std::mutex> lock(_ResultMutex);
                ResultProducer.emplace(std::move(res));
            }

            _cv.wait(lock,
                [this]()->bool
                {
                    return !isNetworkModuleRunning
                        || !TaskConsumer.empty();
                });
        }
    }


private:
    std::mutex _ContextMutex, _ResultMutex;
    std::priority_queue < Task, std::vector<Task>,
        decltype([](const Task& left, const Task& right) {return std::less<int>{}(std::get<Context*>(left)->priority, std::get<Context*>(right)->priority); }) > TaskConsumer;

    std::priority_queue < Result, std::vector<Result>,
        decltype([](const Result& left, const Result& right) {return std::less<int>{}(left.priority, right.priority); }) > ResultProducer;

    std::condition_variable _cv;
    std::mutex _cv_m;

    Fn _worker;
    std::function<void()> _noticer;
    volatile bool isNetworkModuleRunning;
};
#pragma once
#include <string>
#include <map>
#include <memory>
#include <functional>

struct Protocol
{
    const std::string name;

    std::function<int()> init;
    std::function<int()> finish;
    std::function<int(std::shared_ptr<void>)> discover;
    std::function<int(std::shared_ptr<void>)> browse;
};

struct Context
{
    const struct Protocol proto;
    std::shared_ptr<void> priv_data;
    enum Priority
    {
        S, A, B, C
    }priority;

    Context operator=(const Context& other) { return other; }
};

Protocol FindProtocol(const std::string& name);

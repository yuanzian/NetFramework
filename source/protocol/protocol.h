#pragma once
#include <string>
#include <map>
#include <memory>

struct Protocol;
struct Context;

struct Protocol
{
    const std::string name;
    int (*init)();
    int (*finish)();
    int (*discover)(Context* ctx);
    int (*browse)(Context* ctx);
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

#pragma once
#include <string>
#include <map>

struct Protocol
{
    const std::string name;
    int (*init)();
    int (*finish)();
    int (*discover)();
    int (*browse)();
};

struct Context
{
    const struct Protocol proto;
    void* priv_data;
    enum Priority
    {
        S, A, B, C
    }priority;

    Context operator=(const Context& other) { return other; }
};

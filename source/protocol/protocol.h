#pragma once
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <optional>

struct Protocol
{
    const std::string name;

    std::function<int()> init;
    std::function<int()> finish;
    std::function<int(std::shared_ptr<void>)> discover;
    std::function<int(std::shared_ptr<void>)> browse;
};

/*!
* @struct Context
* @brief indispensable component of the Task handled by runner.
*
* @param proto: current context protocol, must discard the context while being nullopt.
* @param priv_data: pointer to a structure of the detail informations according to current context protocol.
*/
struct Context
{
    std::optional<const Protocol> proto;
    std::shared_ptr<void> priv_data;
    enum Priority
    {
        S, A, B, C
    }priority;
};

void InitProtocols();
void FinishProtocols();

std::optional<const Protocol> FindProtocol(const std::string& name);

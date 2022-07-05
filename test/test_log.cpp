#include <iostream>

#include "source/logger.h"

using namespace logger;
using enum logLevel;

int main()
{
    std::cout << "test\n";
    logger::Log(Trace, "hello {}{}", 5, "");
    system("pause");
}
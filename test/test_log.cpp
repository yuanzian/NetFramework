#include <iostream>

#include "source/logger.h"

int main()
{
    using enum logger::logLevel;
    std::cout << std::boolalpha << logger::SetLogFile(R"(./test.log)") << std::endl;
    std::cout << std::boolalpha << logger::OpenFile() << std::endl;

    logger::Log(Trace, "hello {}{}", 5, "");

    system("pause");
}
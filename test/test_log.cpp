#include <iostream>

#include "source/logger.h"
//#include "source/logger_inl.h"

using namespace logger;
using enum logger::logLevel;

int main()
{
    std::cout << std::boolalpha << logger::SetLogFile(R"(./test.log)") << std::endl;
    std::cout << std::boolalpha << logger::OpenFile() << std::endl;

    logger::Log(Trace, "hello {}{}", 5, "");
    //logger::Log(Trace, "hello {}{}", 5, "", std::cout);

    system("pause");
}
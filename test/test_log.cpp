#include <iostream>

#include "source/logger.h"

int main()
{
    std::cout << "test\n";
    logger::Log("hello {}", 5);
}
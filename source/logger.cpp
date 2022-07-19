#include "logger.h"

namespace logger {

    const std::filesystem::path logFile;
    std::ofstream logStream;

    bool SetLogFile(const std::string_view& path)
    {
        return std::filesystem::exists(const_cast<std::filesystem::path&>(logFile) = std::filesystem::u8path(path));//TODO: change all string to u8string.
    }

    bool OpenFile()
    {
        if (logStream.is_open())
            return false;
        else
            logStream.open(logFile, std::ios::out | std::ios::app);

        return true;
    }

    bool CloseFile()
    {
        logStream.close();
        return true;
    }

}// namespace logger

#include <mutex>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <source_location>

namespace logger {

    extern const std::filesystem::path logFile;
    extern std::ofstream logStream;

    bool SetLogFile(const std::string_view& path);
    bool OpenFile();

    enum class logLevel : char
    {
        Trace = 'T',
        Debug = 'D',
        Info = 'I',
        Warning = 'W',
        Error = 'E',
        Fatal = 'F'
    };

    template <typename... Args>
    struct Log
    {
        Log(logLevel level, const char* format, Args&&... args,
            const std::source_location& location = std::source_location::current())
        {
            time_t current = std::time(nullptr);
            std::ostream* tmp = logStream.is_open() ? &logStream : &std::cout;
            *tmp << std::left
                << std::put_time(std::localtime(&current), "%F %T ")
                << "[" << static_cast<char>(level) << "]"
                << "[thread " << std::this_thread::get_id() << "] "
                << location.file_name()
                << "(" << location.line() << ":" << location.column() << ") "
                << std::quoted(location.function_name()) << ": "
                << std::vformat(std::string_view{ format }, std::make_format_args(args...)) << "\n";
        }
    };

    template <typename... Args>
    Log(logLevel, const char*, Args&&...)->Log<Args...>;
};

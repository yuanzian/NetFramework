#include <mutex>
#include <fstream>
#include <filesystem>
#include <source_location>

namespace logger {
    std::mutex logger_mutex;
    std::filesystem::path logFile;

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
            std::ofstream outLogFileStream{ logFile, std::ios::out | std::ios::app };
            if (!outLogFileStream.is_open())
                return;

            time_t current = std::time(nullptr);
            outLogFileStream << std::left
                << std::put_time(std::localtime(&current), "%F %T ")
                << "[" << static_cast<char>(level) << "]"
                << "[thread " << std::this_thread::get_id() << "]"
                << location.file_name()
                << "(" << location.line() << ":" << location.column() << ")"
                << std::quoted(location.function_name()) << ": "
                << std::vformat(std::string_view{ format }, std::make_format_args(args...)) << "\n";
            outLogFileStream.close();
        }
    };

    template <typename... Args>
    Log(logLevel, const char*, Args&&...)->Log<Args...>;
}

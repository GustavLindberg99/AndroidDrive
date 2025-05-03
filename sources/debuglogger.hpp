#ifndef DEBUGLOGGER_HPP
#define DEBUGLOGGER_HPP

#include <QFile>
#include <QString>

#include <format>
#include <memory>
#include <source_location>

template<>
struct std::formatter<QString>: public std::formatter<std::string> {
    auto format(const QString &str, std::format_context &ctx) const {
        return std::formatter<std::string>::format(str.toStdString(), ctx);
    }
};

template<typename T>
constexpr bool isNotTupleHelper = true;

template<typename... Args>
constexpr bool isNotTupleHelper<std::tuple<Args...>> = false;

template<typename T>
concept IsNotTuple = isNotTupleHelper<T>;

class DebugLogger final {
public:
    /**
     * Gets the instance of the singleton class.
     *
     * @return The instance.
     */
    static DebugLogger& getInstance();

    /**
     * Not possible to copy a singleton.
     */
    DebugLogger(const DebugLogger&) = delete;
    DebugLogger &operator=(const DebugLogger&) = delete;

    /**
     * Starts recording debug logs.
     */
    bool start();

    /**
     * Saves the debug logs to a file.
     */
    void stop();

    /**
     * If logging is enabled, logs the message, otherwise does nothing.
     *
     * @param message   The message to log.
     * @param location  The location at which this function is being called at. Intended to always use the default argument, see documentation for std::source_location.
     */
    void log(const QString &message, std::source_location location = std::source_location::current());

    /**
     * If logging is enabled, logs the message, otherwise does nothing. Should be called the same way as std::format, but with the arguments in a tuple to be able to use std::source_location as an optional parameter.
     *
     * @param fmt       The format string.
     * @param args      Arguments to be formatted.
     * @param location  The location at which this function is being called at. Intended to always use the default argument, see documentation for std::source_location.
     */
    template<typename... Args>
    void log(const std::format_string<Args...> &fmt, const std::tuple<Args...> &args, std::source_location location = std::source_location::current()){
        if(this->_file == nullptr){
            return;
        }
        const std::string message = std::vformat(
            fmt.get(),
            std::apply([](auto&&... it){return std::make_format_args(it...);}, args)    //Passes the content of the tuple args to std::make_format_args (see https://stackoverflow.com/a/37100646/4284627)
        );
        this->log(QString::fromStdString(message), location);
    }

    /**
     * Shorthand for log(fmt, std::make_tuple(arg)).
     *
     * Uses a concept to disallow T from being a tuple, otherwise calls to log(fmt, std::make_tuple(arg1, arg2, ...)) would be ambiguous.
     *
     * @param fmt       The format string, containing one placeholder.
     * @param arg       Argument to be formatted.
     * @param location  The location at which this function is being called at. Intended to always use the default argument, see documentation for std::source_location.
     */
    template<IsNotTuple T>
    void log(const std::format_string<T> &fmt, const T &arg, std::source_location location = std::source_location::current()){
        this->log(fmt, std::make_tuple(arg), location);
    }

    /**
     * Checks if debug logs are currently being recorded.
     *
     * @return True if they're currently being recorded, false otherwise.
     */
    bool isRecording() const;

    /**
     * Gets the absolute path of the log file currently being written to.
     *
     * @return The absolute path of the log file with backslashes as directory separators. If debug logs aren't currently being recorded, returns an empty string.
     */
    QString logFilePath() const;

private:
    DebugLogger() = default;

    std::unique_ptr<QFile> _file = nullptr;
};

#endif // DEBUGLOGGER_HPP

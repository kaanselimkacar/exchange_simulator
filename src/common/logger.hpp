#pragma once

#include <source_location>
#include <string>

class Logger {
  public:
    Logger() = default;
    virtual ~Logger() = default;

    Logger(const Logger &) = delete;
    auto operator=(const Logger &) -> Logger & = delete;

    Logger(Logger &&) noexcept = default;
    auto operator=(Logger &&) noexcept -> Logger & = default;

    static auto instance() -> Logger &;

    void debug_impl(const std::string &msg, const std::source_location &loc) {
        debug_impl_internal(msg, loc);
    }

    void info_impl(const std::string &msg, const std::source_location &loc) {
        info_impl_internal(msg, loc);
    }

    void warn_impl(const std::string &msg, const std::source_location &loc) {
        warn_impl_internal(msg, loc);
    }

    void error_impl(const std::string &msg, const std::source_location &loc) {
        error_impl_internal(msg, loc);
    }

  protected:
    virtual void debug_impl_internal(const std::string &msg, const std::source_location &loc) = 0;
    virtual void info_impl_internal(const std::string &msg, const std::source_location &loc) = 0;
    virtual void warn_impl_internal(const std::string &msg, const std::source_location &loc) = 0;
    virtual void error_impl_internal(const std::string &msg, const std::source_location &loc) = 0;
};

#ifdef DEBUG_BUILD
#define LOG_DEBUG(fmt, ...)                                                                        \
    Logger::instance().debug_impl(std::format(fmt, ##__VA_ARGS__), std::source_location::current())

#define LOG_INFO(fmt, ...)                                                                         \
    Logger::instance().info_impl(std::format(fmt, ##__VA_ARGS__), std::source_location::current())
#else
#define LOG_DEBUG(fmt, ...) (void)sizeof(fmt), (void)sizeof(##__VA_ARGS__)
#define LOG_INFO(fmt, ...) (void)sizeof(fmt), (void)sizeof(##__VA_ARGS__)
#endif

#define LOG_WARN(fmt, ...)                                                                         \
    Logger::instance().warn_impl(std::format(fmt, ##__VA_ARGS__), std::source_location::current())

#define LOG_ERROR(fmt, ...)                                                                        \
    Logger::instance().error_impl(std::format(fmt, ##__VA_ARGS__), std::source_location::current())

#pragma once

#include <format>
#include <source_location>
#include <string>

#ifdef DEBUG_BUILD
inline constexpr bool kDebugBuild = true;
#else
inline constexpr bool kDebugBuild = false;
#endif

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

namespace detail {
template <typename... Args>
void LogDebugImpl(std::source_location loc, std::format_string<Args...> fmt, Args &&...args) {
    if constexpr (kDebugBuild) {
        Logger::instance().debug_impl(std::format(std::move(fmt), std::forward<Args>(args)...),
                                      loc);
    }
}

template <typename... Args>
void LogInfoImpl(std::source_location loc, std::format_string<Args...> fmt, Args &&...args) {
    if constexpr (kDebugBuild) {
        Logger::instance().info_impl(std::format(std::move(fmt), std::forward<Args>(args)...), loc);
    }
}

template <typename... Args>
void LogWarnImpl(std::source_location loc, std::format_string<Args...> fmt, Args &&...args) {
    Logger::instance().warn_impl(std::format(std::move(fmt), std::forward<Args>(args)...), loc);
}

template <typename... Args>
void LogErrorImpl(std::source_location loc, std::format_string<Args...> fmt, Args &&...args) {
    Logger::instance().error_impl(std::format(std::move(fmt), std::forward<Args>(args)...), loc);
}
} // namespace detail

template <typename... Args> void LogDebug(std::format_string<Args...> fmt, Args &&...args) {
    detail::LogDebugImpl(std::source_location::current(), std::move(fmt),
                         std::forward<Args>(args)...);
}

template <typename... Args> void LogInfo(std::format_string<Args...> fmt, Args &&...args) {
    detail::LogInfoImpl(std::source_location::current(), std::move(fmt),
                        std::forward<Args>(args)...);
}

template <typename... Args> void LogWarn(std::format_string<Args...> fmt, Args &&...args) {
    detail::LogWarnImpl(std::source_location::current(), std::move(fmt),
                        std::forward<Args>(args)...);
}

template <typename... Args> void LogError(std::format_string<Args...> fmt, Args &&...args) {
    detail::LogErrorImpl(std::source_location::current(), std::move(fmt),
                         std::forward<Args>(args)...);
}

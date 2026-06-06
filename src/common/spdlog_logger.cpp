#include "spdlog_logger.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <format>
#include <memory>
#include <source_location>
#include <stdexcept>

auto Logger::instance() -> Logger & {
    static SpdlogLogger logger;
    return logger;
}

SpdlogLogger::SpdlogLogger() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        spdlog_instance_ = std::make_shared<spdlog::logger>("app", console_sink);

        spdlog::register_logger(spdlog_instance_);

#ifdef DEBUG_BUILD
        spdlog_instance_->set_level(spdlog::level::debug);
#else
        spdlog_instance_->set_level(spdlog::level::warn);
#endif

        spdlog_instance_->flush_on(spdlog::level::err);

    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("spdlog initialization failed: ") + e.what());
    }
}

namespace {
auto format_location(const std::source_location &loc) -> std::string {
    return std::format("{}:{} in {}", loc.file_name(), loc.line(), loc.function_name());
}
} // namespace

void SpdlogLogger::debug_impl_internal(const std::string &msg, const std::source_location &loc) {
    spdlog_instance_->debug("[{}] {}", format_location(loc), msg);
}

void SpdlogLogger::info_impl_internal(const std::string &msg, const std::source_location &loc) {
    spdlog_instance_->info("[{}] {}", format_location(loc), msg);
}

void SpdlogLogger::warn_impl_internal(const std::string &msg, const std::source_location &loc) {
    spdlog_instance_->warn("[{}] {}", format_location(loc), msg);
}

void SpdlogLogger::error_impl_internal(const std::string &msg, const std::source_location &loc) {
    spdlog_instance_->error("[{}] {}", format_location(loc), msg);
}

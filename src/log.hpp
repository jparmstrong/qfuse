/*
    qfuse - virtual file system for KDB/Q
    Copyright (C) 2025 JP Armstrong <jp@armstrong.sh>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

*/

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <fmt/core.h>

inline std::string _logTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

#ifdef DEBUG_LOGS
#define DEBUG(fmt_str, ...) fmt::print("{} DEBUG | " fmt_str "\n", _logTime(), __VA_ARGS__)
#else
#define DEBUG(fmt_str, ...) ((void)0)
#endif

#ifdef TRACE_LOGS
#define TRACE(fmt_str, ...) fmt::print("{} TRACE | " fmt_str "\n", _logTime(), __VA_ARGS__)
#else
#define TRACE(fmt_str, ...) ((void)0)
#endif

#ifdef INFO_LOGS
#define INFO(fmt_str, ...) fmt::print("{} INFO  | " fmt_str "\n", _logTime(), __VA_ARGS__)
#else
#define INFO(fmt_str, ...) ((void)0)
#endif

#ifdef WARN_LOGS
#define WARN(fmt_str, ...) fmt::print("{} WARN  | " fmt_str "\n", _logTime(), __VA_ARGS__)
#else
#define WARN(fmt_str, ...) ((void)0)
#endif

#define ERROR(fmt_str, ...) fmt::print("{} ERROR | " fmt_str "\n", _logTime(), __VA_ARGS__)

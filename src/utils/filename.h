#pragma once

#include "utils/filesystem.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace filename_utils {

namespace fs = std::filesystem;

inline std::string parse_pattern(
    const std::string& pattern, const std::string& filename) {
    if (pattern.empty()) {
        return "";
    }
    fs::path p(filename);
    SPDLOG_TRACE("filename components: {stem}", fmt::arg("stem", p.stem().string()));
    auto parsed = fmt::format(pattern, fmt::arg("stem", p.stem().string()));
    return fs::absolute(parsed).string();
}

inline std::string create_dir_if_not_exist(
    const std::string& dirname)  {
    auto p = fs::absolute(dirname);
    if (fs::is_directory(p)) {
        SPDLOG_TRACE("use existing dir {}", p);
    } else if (fs::exists(p)) {
        throw std::runtime_error(fmt::format(
            "path {} exist and is not dir", p));
    } else {
        SPDLOG_TRACE("create dir {}", p);
        fs::create_directories(p);
    }
    return p.string();
}

}  // namespace

#pragma once
#include "utils/logging.h"
#include <map>
#include <sstream>
#include <variant>

namespace config {

/**
 * @class Config
 * @brief A simple configuration class
 * @ingroup config
 */
class Config {
public:
    using key_t = std::string;
    using value_t = std::variant<bool, int, double, const char *, std::string>;
    using storage_t = std::map<key_t, value_t>;

    Config() = default;
    Config(storage_t config) : m_config(std::move(config)) {}
    Config(const std::initializer_list<storage_t::value_type> &config)
        : Config(storage_t{config}) {}

    template <typename T> inline T get(const std::string &key) const {
        SPDLOG_TRACE("get config key={} value={}", key, this->m_config.at(key));
        return std::get<T>(this->m_config.at(key));
    }

    template <typename T> inline T get_as(const std::string &key) const {
        SPDLOG_TRACE("get config key={} value={}", key, this->m_config.at(key));
        std::stringstream ss;
        std::visit([&](auto &&arg) { ss << arg; }, this->m_config.at(key));
        T out;
        ss >> out;
        SPDLOG_TRACE("get config key={} value={} got={}", key,
                     this->m_config.at(key), out);
        return out;
    }

    template <typename T> inline void set(const key_t &k, const T &v) {
        this->m_config[k] = v;
    }

    std::string pprint() const {
        std::stringstream ss;
        ss << "{";
        for (const auto &p : this->m_config) {
            ss << fmt::format("\n   {}: {}", p.first, p.second);
        }
        ss << "\n}";
        return ss.str();
    };

private:
    storage_t m_config{};
};

} // namespace config

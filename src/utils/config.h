#pragma once
#include "utils/logging.h"
#include "utils/formatter/container.h"
#include <sstream>
#include <iomanip>

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

    template <typename T> inline T get_typed(const std::string &key) const {
        SPDLOG_TRACE("get config key={} value={}", key, at(key));
        return std::get<T>(this->m_config.at(key));
    }

    template <typename T> inline T get(const std::string &key) const {
        std::stringstream ss;
        std::visit([&](auto &&arg) { ss << arg; }, at(key));
        T out;
        ss >> out;
        SPDLOG_TRACE("get config key={} value={} got={}", key,
                     at(key), out);
        return out;
    }
    inline std::string get_str(const std::string & key) const {
        return get<std::string>(key);
    }

    template <typename T> inline void set(const key_t &k, const T &v) {
        this->m_config[k] = v;
        SPDLOG_TRACE("set config key={} value={}", k, at(k));
    }

    inline bool has(const key_t &k) {
        return this->m_config.find(k) != m_config.end();
    }

    std::string pprint() const {
        // compute width
        std::size_t key_width = 0;
        auto it = std::max_element(this->m_config.begin(), this->m_config.end(),
                         [](const auto& a, const auto& b) {
            return a.first.size() < b.first.size();
        });
        if (it != this->m_config.end()) {
            key_width = it->first.size();
        }
        std::stringstream ss;
        ss << "{";
        for (const auto &p : this->m_config) {
            ss << "\n   " << std::setw(key_width) << std::right << p.first;
            ss << fmt::format(": {}", p.second);
        }
        ss << "\n}";
        return ss.str();
    }

private:
    storage_t m_config{};

    const value_t& at(const std::string & key) const {
        try {
            return this->m_config.at(key);
        } catch (const std::out_of_range & e) {
            SPDLOG_ERROR("invalid key: {} in config {}", key, pprint());
            throw;
        }
    }
};

} // namespace config

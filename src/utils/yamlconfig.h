#pragma once
#include "logging.h"
#include "meta.h"
#include <yaml-cpp/yaml.h>

namespace config {

/**
 * @brief The YamlConfig struct
 * This is a thin wrapper around YAML::Node.
 */
struct YamlConfig {
    using key_t = std::string;
    using value_t = YAML::Node;
    using storage_t = YAML::Node;
    YamlConfig() = default;
    explicit YamlConfig(storage_t node) : m_node(std::move(node)) {}
    std::string dump_to_str() const { return fmt::format("{}", m_node); }
    static auto load_from_str(std::string s) {
        return YamlConfig(YAML::Load(s));
    }
    friend std::ostream &operator<<(std::ostream &os,
                                    const YamlConfig &config) {
        return os << config.dump_to_str();
    }

    template <typename T> auto get_typed(const key_t &key) const {
        return m_node[key].as<T>(key);
    }
    template <typename T> auto get_typed(const key_t &key, T &&defval) const {
        decltype(auto) node = m_node[key];
        if (node.IsDefined() && !node.IsNull()) {
            return node.as<T>(key);
        }
        return FWD(defval);
    }

    auto get_str(const key_t &key) const { return get_typed<std::string>(key); }
    auto get_str(const key_t &key, const std::string &defval) const {
        return get_typed<std::string>(key, std::string(defval));
    }

    decltype(auto) operator[](const key_t &key) const { return m_node[key]; }

    bool has(const key_t &key) const { return m_node[key].IsDefined(); }
    template <typename T> bool has_typed(const key_t &key) const {
        if (!has(key)) {
            return false;
        }
        try {
            get_typed<T>(key);
        } catch (YAML::BadConversion) {
            return false;
        }
        return true;
    }

    std::string pformat() const { return dump_to_str(); }

private:
    storage_t m_node;
};

} // namespace config

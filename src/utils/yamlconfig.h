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

    template <typename U>
    using is_validate_key_t =
        std::is_invocable<decltype((YAML::Node(YAML::Node::*)(U)) &
                                   YAML::Node::operator[]),
                          YAML::Node, U>;
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

    template <typename key_t> decltype(auto) operator[](key_t &&key) const {
        auto multiget_node_impl = meta::y_combinator(
            [](auto &&get_node, auto &&node, auto &&x, auto &&...rest) {
                if constexpr (sizeof...(rest) == 0) {
                    return FWD(node)[FWD(x)];
                } else {
                    return get_node(FWD(node)[FWD(x)], rest...);
                }
            });
        if constexpr (meta::is_instance<key_t, std::tuple>::value) {
            return std::apply(multiget_node_impl,
                              std::tuple_cat(std::tuple{m_node}, FWD(key)));
        } else if constexpr (is_validate_key_t<key_t>::value) {
            return std::apply(multiget_node_impl,
                              std::make_tuple(m_node, FWD(key)));
        }
    }

    template <typename key_t> bool has(key_t &&key) const {
        return operator[](FWD(key)).IsDefined();
    }

    template <typename T, typename key_t> bool has_typed(key_t &&key) const {
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

    template <typename T, typename key_t> auto get_typed(key_t &&key) const {
        return operator[](FWD(key)).template as<T>();
    }
    template <typename T, typename key_t>
    auto get_typed(key_t &&key, T &&defval) const {
        decltype(auto) node = operator[](FWD(key));
        if (node.IsDefined() && !node.IsNull()) {
            return node.template as<T>();
        }
        return FWD(defval);
    }

    template <typename... Args> auto get_str(Args &&...args) const {
        return get_typed<std::string>(FWD(args)...);
    }

    std::string pformat() const { return dump_to_str(); }

    static auto from_filepath(const std::string &filepath) {
        return YamlConfig(YAML::LoadFile(filepath));
    }

private:
    storage_t m_node;
};

} // namespace config

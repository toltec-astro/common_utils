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

    template <typename T, typename KT> auto get_typed(KT &&key) const {
        return multiget_node(FWD(key)).template as<T>();
    }
    template <typename T, typename KT>
    auto get_typed(KT &&key, T &&defval) const {
        decltype(auto) node = m_node[key];
        if (node.IsDefined() && !node.IsNull()) {
            return node.template as<T>();
        }
        return FWD(defval);
    }

    template <typename KT> auto get_str(const KT &key) const {
        return get_typed<std::string, KT>(key);
    }
    template <typename KT>
    auto get_str(const KT &key, const std::string &defval) const {
        return get_typed<std::string, KT>(key, std::string(defval));
    }

    template <typename KT> decltype(auto) operator[](KT &&key) const {
        return m_node[FWD(key)];
    }

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

    template <typename KT> decltype(auto) multiget_node(KT &&key) {
        auto multiget_node_impl = meta::y_combinator(
            [](auto &&get_node, auto &&node, auto &&x, auto &&...rest) {
                if constexpr (sizeof...(rest) == 0) {
                    return FWD(node)[FWD(x)];
                } else {
                    return get_node(FWD(node)[FWD(x)], rest...);
                }
            });
        if constexpr (meta::is_instance<KT, std::tuple>::value) {
            return std::apply(multiget_node_impl,
                              std::tuple_cat(std::tuple{m_node}, FWD(key)));
        } else {
            return std::apply(multiget_node_impl, std::tuple{m_node, FWD(key)});
        }
    }
};

} // namespace config

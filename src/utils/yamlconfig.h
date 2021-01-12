#pragma once
#include "filesystem.h"
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
    template <typename Path>
    explicit YamlConfig(storage_t node, Path filepath)
        : m_node(std::move(node)), m_filepath{std::move(filepath)} {}
    template <typename key_t> decltype(auto) get_node(key_t &&key) const {
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
        decltype(auto) node = get_node(FWD(key));
        return node.IsDefined();
    }

    template <typename key_t> bool has_list(key_t &&key) const {
        decltype(auto) node = get_node(FWD(key));
        return node.IsDefined() && node.IsSequence();
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
        return get_node(FWD(key)).template as<T>();
    }
    template <typename T, typename key_t>
    auto get_typed(key_t &&key, T &&defval) const {
        decltype(auto) node = get_node(FWD(key));
        if (node.IsDefined() && !node.IsNull()) {
            return node.template as<T>();
        }
        return FWD(defval);
    }

    template <typename... Args> auto get_str(Args &&...args) const {
        return get_typed<std::string>(FWD(args)...);
    }

    template <typename... Args> auto get_filepath(Args &&...args) const {
        decltype(auto) s = get_typed<std::string>(FWD(args)...);
        if (m_filepath.has_value()) {
            return (m_filepath.value().parent_path() / s).string();
        }
        return s;
    }

    template <typename key_t> auto get_config(key_t &&key) {
        return YamlConfig{get_node(FWD(key)), this->filepath()};
    }

    const std::optional<std::filesystem::path> &filepath() const {
        return m_filepath;
    }

    ///@brief Create instance from Yaml filepath.
    static auto from_filepath(const std::string &filepath) {
        return YamlConfig(YAML::LoadFile(filepath), filepath);
    }
    ///@brief Create instance from Yaml string.
    static auto from_str(const std::string &s) {
        return YamlConfig(YAML::Load(s));
    }

    std::string to_str() const { return fmt::format("{}", m_node); }
    std::string pformat() const { return to_str(); }
    template <typename OStream>
    friend auto operator<<(OStream &os, const YamlConfig &config)
        -> decltype(auto) {
        auto pformat_opt_filepath =
            [](auto &&opt_filepath) -> std::optional<std::string> {
            if (!opt_filepath.has_value()) {
                return std::nullopt;
            }
            return opt_filepath.value().string();
        };
        return os << fmt::format("{}\n<config filepath: {}>", config.pformat(),
                                 pformat_opt_filepath(config.filepath()));
    }

private:
    storage_t m_node{};
    std::optional<std::filesystem::path> m_filepath{std::nullopt};
};

} // namespace config

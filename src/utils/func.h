#pragma once
#include "meta.h"
#include "logging.h"

/*
namespace func_helper {

template<typename, typename=void, template<typeanme >typename ...>
struct is_signature : std::false_type {};

template<typename F, template <> typename... Args>
struct is_signature {
    using value =
};

struct is_return_optional;

struct is_return_optional_tuple;

struct is_return_iterable;

}  // namespace func_helper

namespace func_adpter {
*/

/**
 * @brief Adapt to return a tuple.
 */
/*
template<typename F>
struct Tup {
    Tup(F f_): f(std::move(f_)) {}
    F f;
    template <typename ...Args>
    decltype(auto) operator () (Args ...args)
    {
        using RT = typename std::invoke_result<F, Args...>::type;
        if constexpr (
            meta::is_instance<
                    RT, std::tuple
                    >::value
                    ) {
            return std::invoke(this->f, FWD(args)...);
        } else {
            SPDLOG_TRACE("wrap return in tuple");
            return std::make_tuple(std::invoke(this->f, FWD(args)...));
        }
    }
};
*/

/**
 * @brief Adapt functor to return an optional.
 * @brief
 */
/// template<typename F>
/// struct OptF {
///     OptF(F&& f_): f(FWD(f_)) {};
///     F&& f;
///     template <typename ...Args>
///     decltype(auto) operator () (Args ...args)
///     {
///         using RT = typename std::result_of<F(Args...)>::type;
///         if constexpr (
///             meta::is_instance<
///                     RT, std::optional
///                     >::value
///                     ) {
///             return FWD(f)(FWD(args)...);
///         } else if constexpr (
///              meta::is_sized<
///                     RT>::value
///                     )
///         {
///             SPDLOG_DEBUG("adapt functor of sized return type return an optioanl based on size");
///             auto&& ret = FWD(f)(FWD(args)...);
///             if (ret.size() > 0) {
///                 return std::optional(ret);
///             }
///             return std::optional<decltype(ret)>{};
///         } else
///         {
///             SPDLOG_DEBUG("adapt general functor to return an optional");
///             return std::optional(FWD(f)(FWD(args)...));
///         }
///     }
/// };
///
/// /**
///  * @brief Adapt functor of return type tuple and/or optional to return an optional tuple
///  * Fail the the original
///  */
/// template<typename F>
/// struct OptTupF {
///     OptTupF(F&& f_): f(FWD(f_)) {};
///     F&& f;
///     template <typename ...Args>
///     decltype(auto) operator () (Args ...args)
///     {
///         using RT = typename std::result_of<F(Args...)>::type;
///         if constexpr(
///             meta::is_nested_instance<
///                     RT, std::optional, std::tuple
///                     >::value
///                     )
///         {
///             return FWD(f)(FWD(args)...);
///         } else if constexpr (
///             meta::is_instance<
///                     RT, std::optional
///                     >::value
///                     ) {
///             SPDLOG_DEBUG("adapt functor of return type optional to return optional tuple");
///             auto&& ret = FWD(f)(FWD(args)...);
///             if (ret.has_value()) {
///                 // wrap the value in tuple
///                 return std::optional(std::make_tuple(std::move(ret.value())));
///             }
///             // empty
///             return std::optional<std::tuple<typename RT::value_type>>{};
///         } else if constexpr (
///             meta::is_instance<
///                     RT, std::tuple
///                     >::value
///                     ) {
///             SPDLOG_DEBUG("adapt functor of return type tuple to return optional tuple");
///             return std::optional(FWD(f)(FWD(args)...));
///         } else if constexpr (
///                     meta::is_sized<
///                     RT>::value
///                     ) {
///             SPDLOG_DEBUG("adapt functor of sized return type return an optional tuple based on size");
///             auto&& ret = FWD(f)(FWD(args)...);
///             if (ret.size() > 0) {
///                 return std::optional(std::make_tuple(ret));
///             }
///             return std::optional<std::tuple<decltype(ret)>>{};
///         } else {
///             SPDLOG_DEBUG("adapt general functor to return optional tuple");
///             return std::optional(std::make_tuple(FWD(f)(FWD(args)...)));
///         }
///     }
/// };
///
/// template<typename F>
/// struct IterF {
///     IterF(F f_): f(std::move(f_)) {};
///     F f;
///     template <typename ... Args>
///     decltype(auto) operator() (Args&& ... args) {
///         using RT = typename std::invoke_result<F, Args...>::type;
///         if constexpr (
///              meta::is_iterable<RT>::value
///                     ) {
///             return std::invoke(this->f, FWD(args)...);
///         } else {
///             static_assert (meta::always_false<RT>::value, "UNABLE TO ADAPT FUNCTOR RETURN TYPE TO ITERATOR");
///         }
///     }
/// };



}  // namespace func

/*
template<typename F, template <typename ...> typename T=std::void_t, template <typename ...> typename U=std::void_t>
struct adapt {};

// any to tuple
template <typename F>
struct adapt<F, std::tuple> {
    template<typename ...Fs>
    decltype(auto) operator()(Fs&&...from) {
        static_assert(sizeof...(Fs) > 1, "CALLED WITH TOO MANY ARGUMENTS");
        constexpr bool dryrun = sizeof...(Fs) == 0;
        // already tuple nop
        if constexpr(
                    meta::is_instance<Fs..., std::tuple>::value) {
            if constexpr (dryrun) {
                // no message
                return;
            } else {
                return FWD(from)...;
            }
        // wrap as tuple
        } else {
            if constexpr (dryrun) {
                SPDLOG_DEBUG("adapt {} to std::tuple", typeid(Fs...).name());
                return;
            } else {
                return std::make_tuple(FWD(from)...);
            }
        }
    }
};

// any/sized to optional
template <typename F>
struct adapt<F, std::optional> {
    template<typename ... Fs>
    decltype(auto) operator()(Fs&&... from) {
        static_assert(sizeof...(Fs) > 1, "CALLED WITH TOO MANY ARGUMENTS");
        constexpr bool dryrun = sizeof...(Fs) == 0;
        // already optional, nop
        if constexpr(
                    meta::is_instance<F, std::optional>::value) {
            if constexpr (dryrun) {
                // no message
                return;
            } else {
                return FWD(from);
            }
        // sized, to optional
        } else if constexpr (
                    meta::is_sized<F>::value
                    ){
            if constexpr (dryrun) {
                SPDLOG_DEBUG("adapt sized type {} to std::optional", typeid(F).name());
                return;
            } else {
                if (from.size() > 0) {
                    return std::optional(FWD(from));
                }
                return std::optional<decltype(from)>{};
            }
        // wrap as optional
        } else {
            if constexpr (dryrun) {
                SPDLOG_DEBUG("adapt type {} to std::optional", typeid(F).name());
                return;
            } else {
                return std::optional(FWD(from));
            }
        }
    }
};

// tuple and/or optional to optional/tuple
template<typename F>
struct adapt<F, std::optional, std::tuple> {
    template<typename ...Args>
    decltype(auto) operator(Args...args) {
        static_assert(sizeof...(Types) > 1, "CALLED WITH TOO MANY ARGUMENTS");
        constexpr bool dryrun = sizeof...(Types) == 0;
        // already optional tuple, nop
        if constexpr(
            meta::is_nested_instance<
                    F, std::optional, std::tuple
                    >::value
                    )
        {
            if constexpr (dryrun) {
                return;
            } else {
                return FWD(args)...;
            }

        // optional, wrap value as tuple
        } else if constexpr (
            meta::is_instance<
                    F, std::optional
                    >::value
                    ) {
            if constexpr (dryrun) {
                SPDLOG_DEBUG("adapt optional type {} to optioanl tuple", typeid(F).name());
                return;
            } else {
                if (from.has_value()) {
                    return std::optional(std::make_tuple(FWD(from).value()));
                }
                // empty
                return std::optional<std::tuple<typename F::value_type>>{};
            }
        // size, to optional tuple
        } else if constexpr (
                    meta::is_sized<
                    F>::value
                    ) {
            if constexpr (dryrun) {
                SPDLOG_DEBUG("adapt sized type {} to optional tuple", typeid(F).name());
                return;
            } else {
                if (from.size() > 0) {
                    return std::optional(std::make_tuple(FWD(from)));
                }
                return std::optional<std::tuple<decltype(from)>>{};
            }
        // tuple, to optional
        } else if constexpr (
                meta::is_instance<
                    F, std::tuple
                    >::value
                    ) {
            if constexpr (dryrun) {
                SPDLOG_DEBUG("adapt tuple type {} to optional tuple", typeid(F).name());
                return;
            } else {
                return std::optional(FWD(from));
            }
        // nothing wrap as optional tuple
        } else {
            if constexpr (dryrun) {
                SPDLOG_DEBUG("adapt type {} to optional tuple", typeid(F).name());
                return;
            } else {
                return std::optional(std::make_tuple(FWD(from)));
            }
        }
    }
};
*/

/*
 *  @brief Functor to adapt return types
template<typename F, typename Args>
struct adapter {
    template<typename ...Args>
    struct traits {
        using return_t = typename std::invoke_result<F, Args...>::type;
    }
    template<typename From, typename To, bool DryRun=false>
    decltype(auto) adapt(From&& from) {
        if constexpr(
            meta::is_instance<From, To>
                    ) {
            if constexpr(DryRun) {
                // same
            } else {
                return FWD(From);
            }
        } else if constexpr(
            meta::is_instance<From, std::tuple>::value
                    ) {
            if constexpr(To)
        }
    }
}
 */


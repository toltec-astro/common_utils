#include "enum.h"
#include "formatter/ptr.h"
#include "formatter/utils.h"
#include "logging.h"
#include <Eigen/Core>
#include <mxx/comm.hpp>
#include <mxx/env.hpp>
#include <mxx/utils.hpp>
#include <sstream>
#include <string>

namespace mpi_utils {

inline auto logging_init(const mxx::comm &comm) {
    logging::init();
    auto pattern =
        fmt::format("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#:%!] [{}/{}] %v",
                    comm.rank(), comm.size());
    spdlog::set_pattern(pattern);
}

struct env : mxx::env {
    META_ENUM(WinMemoryModel, int, Unified, Separate, Unknown, NotSupported);
    template <typename... Args> env(Args... args) : mxx::env(args...) {
        // get memory model
        int *attr_val;
        int attr_flag;
        MPI_Win win;
        MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        /* this function will always return flag=false in MPI-2 */
        MPI_Win_get_attr(win, MPI_WIN_MODEL, &attr_val, &attr_flag);
        MPI_Win_free(&win);
        if (attr_flag) {
            if ((*attr_val) == MPI_WIN_UNIFIED) {
                memory_model = WinMemoryModel::Unified;
            } else if ((*attr_val) == MPI_WIN_SEPARATE) {
                memory_model = WinMemoryModel::Separate;
            } else {
                memory_model = WinMemoryModel::Unknown;
            }
        } else {
            memory_model = WinMemoryModel::NotSupported;
        }
        // api version
        MPI_Get_version(&api_version.first, &api_version.second);
        // lib info
        {
            char lib_info_[MPI_MAX_LIBRARY_VERSION_STRING];
            int lib_info_len;
            MPI_Get_library_version(lib_info_, &lib_info_len);
            lib_info = lib_info_;
        }
    }
    ~env();
    std::pair<int, int> api_version{0, 0};
    std::string lib_info{""};
    WinMemoryModel memory_model{WinMemoryModel::Unknown};
};
REGISTER_META_ENUM(env::WinMemoryModel);

env::~env() = default;

template <typename Func>
auto pprint_node_ranks(const mxx::comm &comm, int rank, Func &&func) {
    char node_name[MPI_MAX_PROCESSOR_NAME];
    int node_len;
    MPI_Get_processor_name(node_name, &node_len);
    // gather all processor names to master
    std::vector<char> node_names_raw =
        mxx::gatherv(node_name, SIZET(MPI_MAX_PROCESSOR_NAME), rank, comm);
    if (rank == comm.rank()) {
        std::vector<std::string> node_names;
        for (std::size_t i = 0; i < SIZET(comm.size()); ++i) {
            node_names.emplace_back(node_names_raw.data() +
                                        i * MPI_MAX_PROCESSOR_NAME,
                                    MPI_MAX_PROCESSOR_NAME);
        }
        // SPDLOG_TRACE("node names: {}", node_names);
        std::unordered_map<std::string, std::vector<std::size_t>> node_ranks_;
        for (std::size_t i = 0; i < SIZET(comm.size()); ++i) {
            node_ranks_[node_names[i]].push_back(i);
        }
        std::vector<std::pair<std::string, std::vector<std::size_t>>>
            node_ranks;
        for (auto &[node, ranks] : node_ranks_) {
            std::sort(ranks.begin(), ranks.end());
            node_ranks.emplace_back(node, ranks);
        }
        // sort the vector of node names by first rank
        std::sort(node_ranks.begin(), node_ranks.end(),
                  [](const auto &lhs, const auto &rhs) {
                      return lhs.second.front() < rhs.second.front();
                  });

        std::stringstream ss;
        ss << fmt::format("MPI comm layout:"
                          "\n  n_procs: {}\n  n_nodes: {}",
                          comm.size(), node_ranks.size());
        for (std::size_t i = 0; i < node_ranks.size(); ++i) {
            auto &[node, ranks] = node_ranks[i];
            ss << fmt::format("\n  {}: {}\n      ranks: {}", i, node, ranks);
        }
        FWD(func)(ss.str());
    }
}

/**
 * @brief The Span struct
 * This provides contexts to a continuous block of memory.
 */
template <typename T> struct Span {
    using index_t = MPI_Aint;
    using data_t = T;
    using disp_unit_t = int;
    static constexpr disp_unit_t disp_unit{sizeof(data_t)};
    Span(data_t *data_, index_t size_) : data(data_), size(size_) {}
    data_t *data{nullptr};
    index_t size{0};

    auto asvec() {
        using PlainObject = Eigen::Matrix<data_t, Eigen::Dynamic, 1>;
        return Eigen::Map<PlainObject>(data, size);
    }

    /**
     * @brief Create span as MPI shared memory.
     * @param rank The rank at which the memory is allocated
     * @param size The size of the memory block
     * @param comm The MPI comm across which the memory is shared
     * @param p_win The RMA context.
     */
    template <typename size_t>
    static auto allocate_shared(int rank, size_t size, const mxx::comm &comm,
                                MPI_Win *p_win) {
        Span arr(nullptr, meta::size_cast<Span::index_t>(size));
        // allocate
        MPI_Win_allocate_shared(comm.rank() == rank ? arr.size : 0,
                                arr.disp_unit, MPI_INFO_NULL, comm, &arr.data,
                                p_win);
        if (comm.rank() == rank) {
            SPDLOG_TRACE("shared memory allocated {}", arr);
        } else {
            // update the pointer to point to the shared memory on the
            // creation rank
            Span::index_t _size;
            Span::disp_unit_t _disp_unit;
            MPI_Win_shared_query(*p_win, rank, &_size, &_disp_unit, &arr.data);
            assert(_size == arr.size);
            assert(_disp_unit == arr.disp_unit);
            SPDLOG_TRACE("shared memroy connected from rank={} {}", comm.rank(),
                         arr);
        }
        return arr;
    }
};
} // namespace mpi_utils

namespace fmt {

template <>
struct formatter<mpi_utils::env, char> : fmt_utils::nullspec_formatter_base {
    template <typename FormatContext>
    auto format(const mpi_utils::env &e, FormatContext &ctx)
        -> decltype(ctx.out()) {
        return format_to(
            ctx.out(),
            "MPI environment:\n  MPI-API v{}.{}\n  {}\n  Memory model: {}",
            e.api_version.first, e.api_version.second, e.lib_info,
            e.memory_model);
    }
};

template <typename T>
struct formatter<mpi_utils::Span<T>, char>
    : fmt_utils::nullspec_formatter_base {
    template <typename FormatContext>
    auto format(const mpi_utils::Span<T> &arr, FormatContext &ctx)
        -> decltype(ctx.out()) {
        return format_to(ctx.out(), "@{:z} size={} disp_unit={}",
                         fmt_utils::ptr(arr.data), arr.size, arr.disp_unit);
    }
};
} // namespace fmt

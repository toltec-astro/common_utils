#pragma once

#include "logging.h"
#include "meta_enum.h"
#include <fstream>

namespace datatable {

/// Default Index type from Eigen.
using Index = Eigen::Index;

/// @brief Throw when there is an error in parsing data table.
struct ParseError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
#ifdef DOXYGEN
/**
 * @enum Format
 * @brief The format of data table.
 * @var Ascii
 *  Delim-separated Ascii table.
 * @var Memdump
 *  Raw C-style array memory dump.
 */
enum class Format : int { Ascii, Memdump };
#else
meta_enum_class(Format, int, Ascii, Memdump);
#endif

/**
 * @brief Data table IO handler.
 */
template <Format format> struct IO {
    /**
     * @brief Parse input stream as the specified format.
     * To be implemented for each format specilization.
     * @see IO<Format::Ascii>::parse, IO<Format::Memdump>::parse
     * @tparam Scalar The numeric type of data table.
     * @tparam IStream The input stream type, e.g., std::ifstream.
     */
    template <typename Scalar, typename IStream> static void parse(IStream) {}
};

/**
 * @brief Ascii table IO.
 */
template <> struct IO<Format::Ascii> {
    /**
     * @brief Parse as Ascii table.
     * @param is Input stream to be parsed.
     * @param usecols The indexes of columns to include in the result.
     *  Python-style negative indexes are supported.
     * @param delim Delimiter characters. Default is space or tab.
     */
    template <typename Scalar, typename IStream>
    static decltype(auto) parse(IStream &is,
                                const std::vector<Index> &usecols = {},
                                const std::string &delim = " \t") {
        SPDLOG_TRACE("parse as ascii, usecols={} delim=\"{}\"", usecols, delim);
        // is.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        std::string line;
        std::string strnum;
        std::vector<std::vector<Scalar>> data;
        // clear first
        data.clear();
        // parse line by line
        while (std::getline(is, line)) {
            data.emplace_back(std::vector<double>());
            for (auto i = line.begin(); i != line.end(); ++i) {
                if (!isascii(*i)) {
                    throw ParseError("not an ascii file");
                }
                // If i is not a delim, then append it to strnum
                if (delim.find(*i) == std::string::npos) {
                    strnum += *i;
                    // continue to next char unless this is the last one
                    if (i + 1 != line.end())
                        continue;
                }
                // if strnum is still empty, it means the previous char is also
                // a delim (several delims appear together). Ignore this char.
                if (strnum.empty())
                    continue;
                // If we reach here, we got a number. Convert it to double.
                Scalar number;
                std::istringstream(strnum) >> number;
                data.back().push_back(number);
                strnum.clear();
            }
        }
        // convert to Eigen matrix
        auto nrows = static_cast<Index>(data.size());
        auto ncols = static_cast<Index>(data[0].size());
        auto ncols_use = ncols;
        SPDLOG_TRACE("shape of table ({}, {})", nrows, ncols);
        // get the usecols
        if (!usecols.empty()) {
            for (auto i : usecols) {
                if ((i < -ncols) || (i >= ncols))
                    throw ParseError(fmt::format(
                        "invalid column index {} for table of ncols={}", i,
                        ncols));
            }
            ncols_use = usecols.size();
            SPDLOG_TRACE("using {} cols out of {}", ncols_use, ncols);
        }
        using Eigen::Dynamic;
        using Eigen::Map;
        using Eigen::Matrix;
        Matrix<Scalar, Dynamic, Dynamic> ret(nrows, ncols_use);
        for (Index i = 0; i < nrows; ++i) {
            if (usecols.empty()) {
                ret.row(i) =
                    Map<Matrix<Scalar, Dynamic, 1>>(&data[i][0], ncols_use);
            } else {
                for (std::size_t j = 0; j < usecols.size(); ++j) {
                    auto v = usecols[j];
                    if (v < 0)
                        v += ncols;
                    // SPDLOG_TRACE("get table data {} {} to return {} {}", i,
                    // v, i, j);
                    ret(i, j) = data[i][v];
                }
            }
            data[i].clear();
        }
        return ret;
    }
};

/**
 * @brief C-style array memory dump IO.
 */
template <> struct IO<Format::Memdump> {

    /**
     * @brief Prase as C-style array memory dump.
     * @param is Input stream to be parsed.
     * @param nrows The number of rows of the table.
     * Default is Eigen::Dynamic, in which case the number of rows is
     * determined from the size of the data and \p ncols.
     * @param ncols The number of columns of the table.
     * Default is Eigen::Dynamic, in which case, the number of columns is
     * determined from the size of the data and \p nrows if \p nrows is not
     * Eigen::Dynamic, or 1 if nrows is Eigen::Dynamic.
     * @param order The storage order of the memdump.
     * Default is Eigen::ColMajor.
     */
    template <typename Scalar, typename IStream>
    static decltype(auto) parse(IStream &is, Index nrows = Eigen::Dynamic,
                                Index ncols = Eigen::Dynamic,
                                Eigen::StorageOptions order = Eigen::ColMajor) {
        using Eigen::Dynamic;
        using Eigen::Matrix;
        using Eigen::RowMajor;
        // throw everything
        is.exceptions(std::ios_base::failbit | std::ios_base::badbit |
                      std::ios_base::eofbit);
        // get file size
        is.seekg(0, std::ios_base::end);
        auto filesize = is.tellg();
        auto size = filesize / sizeof(Scalar);
        SPDLOG_TRACE("memdump size={} nelem={}", filesize, size);
        // validate based on nrows and ncols
        if (ncols == Dynamic) {
            ncols = 1;
        } else if (size % ncols != 0) {
            throw ParseError(fmt::format(
                "memdump size {} inconsistent with ncols={}", size, ncols));
        }
        if (nrows == Dynamic) {
            nrows = size / ncols;
        } else if (nrows * ncols != size) {
            throw ParseError(fmt::format(
                "memdump size {} inconsistent with nrows={} ncols={}", size,
                nrows, ncols));
        }
        SPDLOG_TRACE("memdump shape ({}, {})", nrows, ncols);
        // this is colmajor
        Matrix<Scalar, Dynamic, Dynamic> ret(nrows, ncols);
        is.seekg(0, std::ios_base::beg);
        is.read(reinterpret_cast<char *>(ret.data()), filesize);
        if (order & Eigen::RowMajor) {
            ret = Eigen::Map<Matrix<Scalar, Dynamic, Dynamic, RowMajor>>(
                ret.data(), nrows, ncols);
        }
        return ret;
    }
};

/**
 * @brief Read data table file.
 * @tparam Scalar The numeric type of the data.
 * @tparam Format The expected file format from \p Format.
 * @param filepath The path to the table file.
 * @param args The arguments forwarded to call \ref IO<format>::parse().
 */
template <typename Scalar, Format format, typename... Args>
decltype(auto) read(const std::string &filepath, Args &&... args) {
    SPDLOG_TRACE("read data from {}", filepath);
    std::ifstream fo;
    fo.open(filepath, std::ios_base::binary);
    return IO<format>::template parse<Scalar>(
        fo, std::forward<decltype(args)>(args)...);
}

} // namespace datatable

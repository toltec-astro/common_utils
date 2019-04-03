#pragma once


#if defined(__GNUC__) && !defined(__clang__)
#include <experimental/filesystem>
namespace std {
    namespace filesystem = ::std::experimental::filesystem;
} // namespace std
#else
#include <filesystem>
#endif

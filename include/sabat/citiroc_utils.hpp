/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include <array>
#include <istream>

namespace spark::citiroc::utils
{

template<size_t N>
auto read_n_bytes(std::istream& source) -> std::array<std::byte, N>
{
    std::array<std::byte, N> ret {};
    source.read(reinterpret_cast<char*>(ret.data()), N);
    return ret;
}

template<typename T>
auto read_n_bytes(int n, std::istream& source) -> T
{
    T ret {0};
    source.read(reinterpret_cast<char*>(&ret), n);
    return ret;
}

}  // namespace spark::citiroc::utils

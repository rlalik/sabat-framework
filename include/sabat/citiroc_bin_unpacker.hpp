// @(#)lib/fibers:$Id$
// Author: Rafal Lalik  18/11/2017

/*************************************************************************
 * Copyright (C) 2017-2018, Rafał Lalik.                                 *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $SiFiSYS/LICENSE.                         *
 * For the list of contributors see $SiFiSYS/README/CREDITS.             *
 *************************************************************************/

#pragma once

#include "citiroc_types.hpp"

#include <spark/core/unpacker.hpp>
#include <spark/spark_export.hpp>

#include <cstddef>      // for size_t
#include <cstdint>      // for uint16_t
#include <fstream>
#include <optional>
#include <vector>
#include <sys/types.h>  // for ulong

namespace spark
{

// class category;
// class SFibersLookupTable;

// class SPARK_EXPORT bin_unpacker : public unpacker
// {
// private:
//     SCategory* cat_fibers_raw {nullptr};
//     SCategory* cat_fibers_cal {nullptr};
//     SFibersLookupTable* p_lookup {nullptr};
//
// public:
//     bin_unpacker() = default;
//
//     bin_unpacker(const bin_unpacker&) = delete;
//     bin_unpacker(bin_unpacker&&) = delete;
//
//     auto operator=(const bin_unpacker&) -> bin_unpacker& = delete;
//     auto operator=(bin_unpacker&&) -> bin_unpacker& = delete;
//
//     virtual ~bin_unpacker() = default;
//
//     auto init() -> bool override;
//     auto execute(uint64_t event, uint64_t seq_number, uint16_t subevent, void* buffer, size_t length) -> bool
//     override;
// };

class category;
class sparksys;

namespace citiroc
{

class SPARK_EXPORT bin_unpacker : public unpacker
{
private:
    category* cat_fibers_raw {nullptr};
    // category* cat_fibers_cal {nullptr};
    // SFibersLookupTable* p_lookup {nullptr};

public:
    explicit bin_unpacker(sparksys* sprk)
        : unpacker(sprk)
    {
    }

    bin_unpacker(const bin_unpacker&) = delete;
    bin_unpacker(bin_unpacker&&) = delete;

    auto operator=(const bin_unpacker&) -> bin_unpacker& = delete;
    auto operator=(bin_unpacker&&) -> bin_unpacker& = delete;

    ~bin_unpacker() override = default;

    auto init() -> bool override;

    auto execute(uint64_t event, uint64_t seq_number, uint16_t subevent, std::istream& source, size_t length)
        -> bool override;
};

}  // namespace citiroc

}  // namespace spark

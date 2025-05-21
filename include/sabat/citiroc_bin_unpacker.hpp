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

#include "sabat/citiroc_utils.hpp"
#include "sabat/sabat_categories.hpp"

#include <spark/core/unpacker.hpp>
#include <spark/parameters/database.hpp>
#include <spark/spark.hpp>
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

namespace
{

}  // namespace

template<typename LookupTable>
class SPARK_EXPORT bin_unpacker : public unpacker
{
private:
    category* cat_fibers_raw {nullptr};
    // category* cat_fibers_cal {nullptr};
    spark::container_wrapper<LookupTable> p_lookup;

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

    auto init() -> bool override
    {
        unpacker::init();

        cat_fibers_raw = spark()->build_category<SabatRaw>(SabatCategories::SabatRaw);

        if (cat_fibers_raw != nullptr) {
            spdlog::critical("[{}] No SabatRaw category", __PRETTY_FUNCTION__);
            return false;
        }

        // catFibersCal = sifi()->buildCategory(SCategory::CatFibersCal); FIXME

        // if (!cat_fibers_cal) {
        //     spdlog::critical("No CatFibersCal category");
        //     return false;
        // }

        p_lookup = spark()->parameters().get_container<LookupTable>("SabatPMLookup");  // FIXME
        p_lookup->print();

        return true;
    }

    auto execute(uint64_t event, uint64_t seq_number, uint16_t subevent, std::istream& source, size_t length)
        -> bool override
    {
        read_event(source);

        // FIXME
        // SFibersChannel* lc = dynamic_cast<SFibersChannel*>(p_lookup->getAddress(0x1000, hit->ch));
        // if (!lc) {
        //     std::cerr << "No associated channel<->fiber value in " << pLookUp->GetName() << std::endl;
        //     std::cerr << "The container(params.txt) might be empty or the channel, fiber information doesn't exist."
        //               << std::endl;
        //     exit(0);
        // }
        // SLocator loc(3);
        // loc[0] = lc->m;  // mod;
        // loc[1] = lc->l;  // lay;
        // loc[2] = lc->s;  // fib;
        // char side = lc->side;

        return true;
    }

private:
    auto read_hit(int n, std::istream& source) -> void
    {
        auto channel = utils::read_n_bytes<uint8_t>(1, source);
        auto datatype = utils::read_n_bytes<uint8_t>(1, source);

        std::optional<int> toa {};
        std::optional<int> tot {};

        if (datatype & 0x10u) {
            toa = utils::read_n_bytes<uint32_t>(4, source);
        }
        if (datatype & 0x20u) {
            tot = utils::read_n_bytes<uint16_t>(2, source);
        }

        spdlog::debug("  Hit {:4d}  Channel {:3d}  DataType {:#04x}  ToA {:10s}  ToT {:10s}",
                      n,
                      channel,
                      datatype,
                      toa ? std::to_string(toa.value()) : "-",
                      tot ? std::to_string(tot.value()) : "-");

        fmt::print("Addr: {}\n", std::tuple{0, channel});

        auto row = p_lookup->get({0, channel});

        fmt::print("Row: {}\n", row);

        // auto obj = cat_fibers_raw->get_object<SabatRaw>(row); // FIXME use tuples?
        auto obj = cat_fibers_raw->get_object<SabatRaw>({std::get<0>(row), std::get<1>(row)}); // FIXME use tuples?
        if (!obj) {
            obj = cat_fibers_raw->make_object_unsafe<SabatRaw>({std::get<0>(row), std::get<1>(row)});
        }

        obj->board = 0;
        obj->channel = channel;
        obj->toa = toa.value_or(-1);
        obj->tot = tot.value_or(-1);
    }

    auto read_event(std::istream& source) -> void
    {
        auto evsize = utils::read_n_bytes<uint16_t>(2, source);
        auto brd = utils::read_n_bytes<uint8_t>(1, source);
        auto trgts = utils::read_n_bytes<uint64_t>(8, source);
        auto nhits = utils::read_n_bytes<uint16_t>(2, source);

        spdlog::debug(" Event :  Size {:#06x}  Board {:3d}  trgTS {:#018x}  Nhits {:4d}", evsize, brd, trgts, nhits);

        for (int i = 0; i < nhits; ++i) {
            read_hit(i, source);
        }
    }
};

}  // namespace citiroc

}  // namespace spark

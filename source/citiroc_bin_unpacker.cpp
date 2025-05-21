// @(#)lib/fibers:$Id$
// Author: Rafal Lalik  18/11/2017

/*************************************************************************
 * Copyright (C) 2017-2018, Rafał Lalik.                                 *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $SiFiSYS/LICENSE.                         *
 * For the list of contributors see $SiFiSYS/README/CREDITS.             *
 *************************************************************************/

#include "sabat/citiroc_bin_unpacker.hpp"

#include "sabat/citiroc_utils.hpp"
#include "sabat/sabat_categories.hpp"

#include <spark/core/unpacker.hpp>
#include <spark/spark.hpp>

// #include "SDatabase.h" // for pm, SDatabase
// #include "SFibersLookup.h"
// #include "SLookup.h"  // for SLookupChannel, SLookupTable

#include <cstddef>
#include <cstdint>

#include <spdlog/spdlog.h>

namespace spark::citiroc
{

namespace
{

auto read_hit(category* cat, int n, std::istream& source) -> void
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

    auto obj = cat->get_object<SabatRaw>({0, channel});
    if (!obj) {
        obj = cat->make_object_unsafe<SabatRaw>({0, channel});
    }

    obj->board = 0;
    obj->channel = channel;
    obj->toa = toa.value_or(-1);
    obj->tot = tot.value_or(-1);
}

auto read_event(category* cat, std::istream& source) -> void
{
    auto evsize = utils::read_n_bytes<uint16_t>(2, source);
    auto brd = utils::read_n_bytes<uint8_t>(1, source);
    auto trgts = utils::read_n_bytes<uint64_t>(8, source);
    auto nhits = utils::read_n_bytes<uint16_t>(2, source);

    spdlog::debug(" Event :  Size {:#06x}  Board {:3d}  trgTS {:#018x}  Nhits {:4d}", evsize, brd, trgts, nhits);

    for (int i = 0; i < nhits; ++i) {
        read_hit(cat, i, source);
    }
}

}  // namespace

auto bin_unpacker::init() -> bool
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

    // pLookUp = dynamic_cast<SFibersLookupTable*>(pm()->getLookupContainer("FibersPMILookupTable")); FIXME
    //     pLookUp->print();

    return true;
}

auto bin_unpacker::execute(
    uint64_t /*event*/, uint64_t /*seq_number*/, uint16_t /*subevent*/, std::istream& source, size_t /*length*/) -> bool
{
    read_event(cat_fibers_raw, source);

    // FIXME
    // SFibersChannel* lc = dynamic_cast<SFibersChannel*>(pLookUp->getAddress(0x1000, hit->ch));
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

}  // namespace spark::citiroc

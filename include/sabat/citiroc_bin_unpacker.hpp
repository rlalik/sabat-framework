/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include "sabat/sabat_export.hpp"

#include "sabat/citiroc_types.hpp"
#include "sabat/citiroc_utils.hpp"
#include "sabat/sabat_categories.hpp"
#include "sabat/sabat_definitions.hpp"

#include <spark/core/unpacker.hpp>
#include <spark/parameters/database.hpp>
#include <spark/spark.hpp>

#include <cstddef>      // for size_t
#include <cstdint>      // for uint16_t
#include <fstream>
#include <optional>
#include <vector>
#include <sys/types.h>  // for ulong

namespace spark
{

class category;
class sparksys;

namespace citiroc
{

template<typename LookupTable>
class SABAT_EXPORT bin_unpacker : public unpacker
{
public:
    using unpacker::unpacker;

    bin_unpacker(const bin_unpacker&) = delete;
    bin_unpacker(bin_unpacker&&) = delete;

    auto operator=(const bin_unpacker&) -> bin_unpacker& = delete;
    auto operator=(bin_unpacker&&) -> bin_unpacker& = delete;

    ~bin_unpacker() override = default;

    auto init() -> bool override
    {
        unpacker::init();

        cat_sipm_raw = model()->template build_category<SiPMRaw>(SabatCategories::SiPMRaw);

        if (cat_sipm_raw == nullptr) {
            spdlog::critical("[{}] No SiPMRaw category", __PRETTY_FUNCTION__);
            return false;
        }

        sabat_lookup = db()->template get_container<LookupTable>("SabatLookup");

        return true;
    }

    auto execute(uint64_t /*event*/,
                 uint64_t /*seq_number*/,
                 uint16_t /*subevent*/,
                 std::istream& source,
                 size_t /*length*/) -> bool override
    {
        // spdlog::debug(" Unpack Event :  {}  SeqNim {}  SubEVT {}  Length {}", event, seq_number, subevent, length);
        return read_event(source);
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

        auto [mod, sipm] = sabat_lookup->get({0, channel});

        auto obj = cat_sipm_raw->get_object<SiPMRaw>({mod, sipm});  // FIXME use tuples?
        if (!obj) {
            obj = cat_sipm_raw->make_object_unsafe<SiPMRaw>({mod, sipm});
        }

        obj->board = mod;
        obj->channel = sipm;
        obj->toa = toa.value_or(-1);
        if (obj->toa != -1) {  // as the 1 LSB = 0.5 ns, do conversion to ns
            obj->toa *= 0.5;
        }
        obj->tot = tot.value_or(-1);
        if (obj->tot != -1) {  // as the 1 LSB = 0.5 ns, do conversion to ns
            obj->tot *= 0.5;
        }
    }

    auto read_event(std::istream& source) -> bool
    {
        auto evsize = utils::read_n_bytes<uint16_t>(2, source);

        if (evsize == 0) {
            return false;
        }

        auto brd = utils::read_n_bytes<uint8_t>(1, source);
        auto trgts = utils::read_n_bytes<uint64_t>(8, source);
        auto nhits = utils::read_n_bytes<uint16_t>(2, source);

        spdlog::debug(" Event :  Size {:#06x}  Board {:3d}  trgTS {:#018x}  Nhits {:4d}", evsize, brd, trgts, nhits);

        for (int i = 0; i < nhits; ++i) {
            read_hit(i, source);
        }

        return true;
    }

private:
    category* cat_sipm_raw {nullptr};
    spark::container_wrapper<LookupTable> sabat_lookup;
};

}  // namespace citiroc

}  // namespace spark

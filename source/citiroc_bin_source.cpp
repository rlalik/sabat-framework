// @(#)lib/base/datasources:$Id$
// Author: Rafal Lalik  18/11/2017

/*************************************************************************
 * Copyright (C) 2017-2018, Rafał Lalik.                                 *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $SiFiSYS/LICENSE.                         *
 * For the list of contributors see $SiFiSYS/README/CREDITS.             *
 *************************************************************************/

#include "sabat/citiroc_bin_source.hpp"

#include "sabat/citiroc_types.hpp"
#include "sabat/citiroc_utils.hpp"

#include <spark/core/unpacker.hpp>

// #include <spark/data_struct/SLookup.hpp>
// #include <SFibersLookup.h>

#include <cstdint>
#include <fstream>
#include <ios>
#include <istream>

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

namespace spark::citiroc
{

namespace
{
auto read_file_header(std::istream& source) -> types::file_header
{
    types::file_header fheader;

    fheader.firmware_ver = utils::read_n_bytes<2>(source);
    fheader.janus_rel = utils::read_n_bytes<3>(source);
    fheader.board_id = utils::read_n_bytes<uint16_t>(2, source);
    fheader.run = utils::read_n_bytes<uint16_t>(2, source);
    fheader.acq_mode = utils::read_n_bytes<uint8_t>(1, source);
    fheader.e_hists_nbins = utils::read_n_bytes<uint16_t>(2, source);
    fheader.toa_tot_unit = utils::read_n_bytes<uint8_t>(1, source);
    fheader.time_lsb = utils::read_n_bytes<uint32_t>(4, source);
    fheader.run_timestamp = utils::read_n_bytes<uint64_t>(8, source);

    return fheader;
}
}  // namespace

auto bin_source::open() -> bool
{
    source = std::ifstream(file, std::ios_base::binary);

    if (!source) {
        spdlog::critical("Invalid source {}", file.string());
        return false;
        // throw std::runtime_error(file.string() + ": " + std::strerror(errno));
    }

    spdlog::info("Citiroc bin file open: {:s}", file.string());

    fheader = read_file_header(source);

    spdlog::info(
        " Firmware: {:#06x}  Janus: {:#08x}:  Board {:#06x}:  Run {:#06x}:  AcqMode {:#04x} "
        " EHnB {:#06x}  Tunit " "{:#04x}  Tlsb {:#010x}  TS {:#018x}",
        spdlog::to_hex(fheader.firmware_ver), spdlog::to_hex(fheader.janus_rel),
        fheader.board_id, fheader.run, fheader.acq_mode, fheader.e_hists_nbins,
        fheader.toa_tot_unit, fheader.time_lsb, fheader.run_timestamp);

    hwid = fheader.board_id;
    hwid <<= 16u;

    vaddr = get_vadrr(hwid);

    spdlog::info("Board ID {} -> vaddr {}", hwid, vaddr);

    return true;
}

auto bin_source::read_current_event() -> bool
{
    spdlog::debug("Read Citiroc event");

    auto* unp = get_unpacker(vaddr);

    // spdlog::info("Unpacker = {:p} for {}", (void*)unp, vaddr);
    unp->execute(0, 0, vaddr, source, 0);

    return true;

    return source.operator bool();
}

auto bin_source::get_n_events() -> int64_t
{
    auto cur_pos = source.tellg();

    source.seekg(0);

    read_file_header(source);

    int64_t nevents {0};

    while (true) {
        // auto search_pos = source.tellg();

        auto evsize = utils::read_n_bytes<uint16_t>(2, source);
        if (!source) {
            break;
        }

        source.seekg(evsize - 2, std::ios::cur);
        // if (source.eof()) break;

        // auto pos = source.tellg();
        //
        // if (pos % (1024*1024) == 0)
        //     fmt::print("pos = {:.2f} MB\n", pos/(1024.*1024.));

        nevents++;
    }

    source.clear();
    source.seekg(cur_pos);

    return nevents;
}

auto bin_source::skip_to_event(int64_t) -> void {}

}  // namespace spark::citiroc

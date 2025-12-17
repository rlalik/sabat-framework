/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#include "sabat/citiroc_bin_source.hpp"

#include "sabat/citiroc_types.hpp"
#include "sabat/citiroc_utils.hpp"

#include <spark/core/unpacker.hpp>

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

    fheader.firmware_ver = std::byteswap(utils::read_n_bytes<uint16_t>(2, source));
    fheader.janus_rel = std::byteswap(utils::read_n_bytes<uint32_t>(3, source)) >> 8;
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
    if (fheader.firmware_ver > 0) {
        return true;  // already open
    }

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
        fheader.firmware_ver, fheader.janus_rel, fheader.board_id, fheader.run, fheader.acq_mode,
        fheader.e_hists_nbins, fheader.toa_tot_unit, fheader.time_lsb, fheader.run_timestamp);

    hwid = fheader.board_id << 16u;

    vaddr = get_vadrr(hwid);

    spdlog::info("Board ID {:#x} -> vaddr {}", hwid, vaddr);

    if (spdlog::get_level() == spdlog::level::debug) {
        spdlog::debug("Number of events in file: {}", get_n_events());
    }
    return true;
}

auto bin_source::read_current_event() -> bool
{
    spdlog::debug("Read Citiroc event {} for vadrr = {}", get_current_event(), vaddr);

    auto* unp = get_unpacker(vaddr);

    spdlog::debug("Unpacker = {:p} for {} event {}", (void*)unp, vaddr, get_current_event());
    return unp->execute(get_current_event(), get_current_event(), vaddr, source, 0);

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

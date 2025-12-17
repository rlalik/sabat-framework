/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace spark::citiroc::types
{

struct timing_hit
{
    uint8_t channel {0};
    uint8_t datatype {0};
    std::optional<int> toa;
    std::optional<int> tot;
};

struct event_header
{
    uint16_t evsize {0};
    uint8_t brd {0};
    uint64_t trgts {0};
    uint16_t nhits {0};
};

template<typename T>
struct event
{
    event_header header;
    std::vector<T> hits;

    auto prepare_hits() -> void { hits.reserve(header.nhits); }
};

struct file_header
{
    uint16_t firmware_ver {};
    uint32_t janus_rel {};
    uint16_t board_id {};
    uint16_t run {};
    uint8_t acq_mode {};
    uint16_t e_hists_nbins {};
    uint8_t toa_tot_unit {};
    uint32_t time_lsb {};
    uint64_t run_timestamp {};
};

}  // namespace spark::citiroc::types

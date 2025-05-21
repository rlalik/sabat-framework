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

#include <spark/core/data_source.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

namespace spark::citiroc
{

/**
 * Extends SDataSource to read data from the Citiroc setup.
 */
class SABAT_EXPORT bin_source : public data_source
{
public:
    bin_source() = default;

    auto read_current_event() -> bool override;

    /**
     * Set input for the source.
     *
     * \param filename input file name
     * \param length length of buffer to read
     */
    virtual auto set_input(const std::filesystem::path& filepath) -> void { file = filepath; }

    auto open() -> bool override;

    auto close() -> bool override { return true; }

    auto skip_to_event(int64_t new_event) -> void;

private:
    auto get_n_events() -> int64_t;

    std::filesystem::path file;  ///< file name

    std::ifstream source;        ///< input file stream
    types::file_header fheader;  ///< file header
    uint32_t hwid {0};
    uint16_t vaddr {0};
};

}  // namespace spark::citiroc

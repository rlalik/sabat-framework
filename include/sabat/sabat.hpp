/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include "sabat/sabat_export.hpp"

#include "sabat/sabat_categories.hpp"
#include "sabat/sabat_detector.hpp"

#include <spark/spark.hpp>

namespace sabat
{

template<typename DetectorVariant, typename Categories = SabatCategories>
struct SABAT_EXPORT sabatsys : public spark::sparksys
{
    sabatsys()
        : spark::sparksys(std::in_place_type_t<Categories> {})
    {
        spdlog::info("..:: SETUP SABAT SYSTEM ::..");
        system().template make_detector<DetectorVariant>("SabatEye");
    }

    auto init(size_t runid = 0) -> void
    {
        spdlog::info("..:: INIT SABAT SYSTEM ::..");
        pardb().init_containers(runid);
    }
};

using SabatMain = sabat::sabatsys<sabat_detector>;

}  // namespace sabat

/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include "sabat/sabat_export.hpp"

#include "sabat/sabat_definitions.hpp"
#include "sabat/sabat_task_calibration.hpp"
#include "sabat/sabat_task_clustering.hpp"

#include <spark/core/detector.hpp>
#include <spark/core/task_manager.hpp>

class SABAT_EXPORT sabat_detector : public spark::detector
{
public:
    using detector::detector;

    auto setup_categories(spark::category_manager& cat_mgr) -> void override
    {
        cat_mgr.register_category(SabatCategories::SiPMRaw, "SiPMRaw", {2, 64}, false);
        cat_mgr.register_category(SabatCategories::SiPMCal, "SiPMCal", {2, 64}, false);
        cat_mgr.register_category(SabatCategories::PhotonHit, "PhotonHit", {2, 10}, false);
    }

    auto setup_containers(spark::database& rundb) -> void override
    {
        rundb.register_container<SabatLookup>("SabatLookup", 0x0000, 0x1000, 64, "{:x} {}", "{} {}");
        rundb.register_container<SiPMCalPar>("SiPMCalPar", "{:x} {}", "{} {} {}");
    }

    auto setup_tasks(spark::task_manager& task_mgr) -> void override
    {
        task_mgr.add_task<sabat_calibration>();
        task_mgr.add_task<sabat_clustering, sabat_calibration>();
    }
};

#pragma once

#include "sabat_definitions.hpp"
#include "sabat_task_calibration.hpp"
#include "sabat_task_clustering.hpp"

#include <spark/core/detector.hpp>
#include <spark/core/task_manager.hpp>
#include <spark/spark_export.hpp>

class SPARK_EXPORT sabat_detector : public spark::detector
{
public:
    using detector::detector;

    auto init_categories(spark::category_manager& cat_mgr) -> bool override
    {
        cat_mgr.reg(SabatCategories::SabatRaw, "SabatRaw", {2, 64}, true);
        cat_mgr.reg(SabatCategories::SabatCal, "SabatCal", {2, 64}, false);
        return true;
    }

    auto init_containers(spark::database& rundb) -> bool override
    {
        rundb.register_container<SabatPMLookup>("SabatPMLookup", 0x0000, 0x1000, 64, "{:x} {}", "{} {}");
        rundb.register_container<SabatPMCal>("SabatPMCal", "{:x} {}", "{} {} {}");
        return true;
    }

    auto setup_tasks(spark::task_manager& task_mgr) -> bool override
    {
        task_mgr.add_task<sabat_calibration>();
        task_mgr.add_task<sabat_clustering, sabat_calibration>();
        return true;
    }
};

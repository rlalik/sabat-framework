#pragma once

#include <spark/core/detector.hpp>
#include <spark/core/task_manager.hpp>
#include <spark/spark_export.hpp>

#include "sabat_task_calibration.hpp"
#include "sabat_task_clustering.hpp"

class SPARK_EXPORT sabat_detector : public spark::detector
{
public:
    using detector::detector;

    auto init_categories() -> bool override { return true; }

    auto init_containers() -> bool override { return true; }

    auto setup_tasks(spark::task_manager& task_mgr) -> bool override
    {
        task_mgr.add_task<sabat_calibration>();
        task_mgr.add_task<sabat_clustering, sabat_calibration>();
        return true;
    }
};

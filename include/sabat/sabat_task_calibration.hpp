#pragma once

#include <spark/core/task.hpp>
#include <spark/spark.hpp>

#include "citiroc_types.hpp"
#include "sabat_categories.hpp"
#include "sabat_definitions.hpp"

class sabat_calibration : public spark::task
{
public:
    using task::task;

    auto init() -> bool override
    {
        cat_fibers_raw = spark()->get_category(SabatCategories::SabatRaw);

        if (cat_fibers_raw == nullptr) {
            spdlog::critical("[{}] No SabatRaw category", __PRETTY_FUNCTION__);
            return false;
        }

        cat_fibers_cal = spark()->build_category<SabatCal>(SabatCategories::SabatCal);

        if (cat_fibers_cal == nullptr) {
            spdlog::critical("[{}] Cannot build SabatCal category", __PRETTY_FUNCTION__);
            return false;
        }

        pm_cal = spark()->parameters().get_container<SabatPMCal>("SabatPMCal");

        return true;
    }

    auto execute() -> bool override
    {
        auto n_objs = cat_fibers_raw->get_entries();

        for (int i = 0; i < n_objs; ++i) {
            auto loc = cat_fibers_raw->get_locator(i);

            auto raw_obj = cat_fibers_raw->get_object<SabatRaw>(i);

            auto new_cal_obj = cat_fibers_cal->make_object_unsafe<SabatCal>(loc);

            new_cal_obj->board = raw_obj->board;
            new_cal_obj->channel = raw_obj->channel;

            new_cal_obj->toa = raw_obj->toa * 0.515;
            new_cal_obj->energy = raw_obj->tot * 0.123 + 456.;
        }

        // std::ranges::for_each(*cat_fibers_cal,
        //                       [&](auto& raw_obj)
        //                       {
        //                           // FIXME
        //                       });

        return true;
    }

private:
    spark::category* cat_fibers_raw {nullptr};
    spark::category* cat_fibers_cal {nullptr};

    spark::container_wrapper<SabatPMCal> pm_cal;
};

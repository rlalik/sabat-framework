/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include <spark/core/task.hpp>
#include <spark/spark.hpp>

#include "sabat/citiroc_types.hpp"
#include "sabat/sabat_categories.hpp"
#include "sabat/sabat_definitions.hpp"

class sabat_calibration : public spark::task
{
public:
    using task::task;

    auto init() -> bool override
    {
        cat_sipm_raw = model()->get_category(SabatCategories::SiPMRaw);

        if (cat_sipm_raw == nullptr) {
            spdlog::critical("[{}] No SiPMRaw category", __PRETTY_FUNCTION__);
            return false;
        }

        cat_sipm_cal = model()->build_category<SiPMCal>(SabatCategories::SiPMCal);

        if (cat_sipm_cal == nullptr) {
            spdlog::critical("[{}] Cannot build SiPMCal category", __PRETTY_FUNCTION__);
            return false;
        }

        pm_cal = db()->get_container<SiPMCalPar>("SiPMCalPar");
        pm_cal->print();

        return true;
    }

    auto execute() -> bool override
    {
        auto n_objs = cat_sipm_raw->get_entries();

        for (int i = 0; i < n_objs; ++i) {
            auto loc = cat_sipm_raw->get_locator(i);

            auto raw_obj = cat_sipm_raw->get_object<SiPMRaw>(i);

            auto new_cal_obj = cat_sipm_cal->make_object_unsafe<SiPMCal>(loc);

            new_cal_obj->board = raw_obj->board;
            new_cal_obj->channel = raw_obj->channel;

            new_cal_obj->toa = raw_obj->toa;
            new_cal_obj->energy = raw_obj->tot;
        }

        return true;
    }

private:
    spark::category* cat_sipm_raw {nullptr};
    spark::category* cat_sipm_cal {nullptr};

    spark::container_wrapper<SiPMCalPar> pm_cal;
};

/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include <spark/core/task.hpp>

class sabat_clustering : public spark::task
{
public:
    using task::task;

    auto init() -> bool override
    {
        cat_sipm_cal = model()->get_category(SabatCategories::SiPMCal);

        if (cat_sipm_cal == nullptr) {
            spdlog::critical("[{}] No SiPMCal category", __PRETTY_FUNCTION__);
            return false;
        }

        cat_photon_hit = model()->build_category<PhotonHit>(SabatCategories::PhotonHit);

        if (cat_photon_hit == nullptr) {
            spdlog::critical("[{}] Cannot build PhotonHit category", __PRETTY_FUNCTION__);
            return false;
        }

        return true;
    }

    auto execute() -> bool override
    {
        auto n_objs = cat_sipm_cal->get_entries();

        float energy_sum = 0.0;
        int count {0};

        for (int i = 0; i < n_objs; ++i) {
            auto loc = cat_sipm_cal->get_locator(i);

            auto cal_obj = cat_sipm_cal->get_object<SiPMCal>(i);

            if (cal_obj->board == 0) {
                energy_sum += cal_obj->energy;
                count++;
            }
        }

        auto new_hit_obj = cat_photon_hit->make_object_unsafe<PhotonHit>({0, 0});
        new_hit_obj->board = 0;
        new_hit_obj->energy = energy_sum;
        new_hit_obj->mult = count;

        return true;
    }

private:
    spark::category* cat_sipm_cal {nullptr};
    spark::category* cat_photon_hit {nullptr};
};

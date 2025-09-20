/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include <spark/core/category.hpp>

#include <TObject.h>

enum class SabatCategories : std::uint8_t
{
    GeantTrack = 0,    ///< geant track
    GeantSiPMRaw = 1,  ///< SiPM geant hit
    SiPMRaw = 20,      ///< SiPM raw data
    SiPMCal = 21,      ///< SiPM cal data
    PhotonHit = 22,    ///< hit
};

struct SiPMRaw : public TObject
{
    SiPMRaw() = default;

    int board {-1};
    int channel {-1};
    float toa {0};
    float tot {0};

    ClassDef(SiPMRaw, 1)
};

struct SiPMCal : public TObject
{
    SiPMCal() = default;

    int board {-1};
    int channel {-1};
    float toa {0};
    float energy {0};

    ClassDef(SiPMCal, 1)
};

struct PhotonHit : public TObject
{
    PhotonHit() = default;

    int board {-1};
    float x {-1};
    float y {0};
    float energy {0.0};
    int mult {0};

    ClassDef(PhotonHit, 1)
};

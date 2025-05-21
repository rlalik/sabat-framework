#pragma once

#include <spark/core/category.hpp>

// #include <array>

#include <TObject.h>

// template<size_t N>
// struct GeantTrack : public spark::basic_category<N>
// {
//     GeantTrack() = default;
//
//     int a {13};  ///<
//
//     ClassDef(GeantTrack, 1)
// };

enum class SabatCategories : std::uint8_t
{
    GeantTrack = 0,     ///< geant track
    GeantSabatRaw = 1,  ///< fibers hit

    // helpers

    // SiPMs
    SabatRaw = 20,   ///< fibers stack raw data
    SabatCal = 21,   ///< fibers cal data
    SabatHit = 22,   ///< fibers hit
    SabatClus = 23,  ///< fibers cluster
};

struct SabatRaw : public TObject
{
    SabatRaw() = default;

    SabatRaw(int _board, int _channel)
        : board {_board}
        , channel {_channel}
    {
    }

    int board {-1};
    int channel {-1};
    int toa {0};
    int tot {0};

    ClassDef(SabatRaw, 1)
};

struct SabatCal : public TObject
{
    SabatCal() = default;

    int board {-1};
    int channel {-1};
    float toa {0};
    float energy {0};

    ClassDef(SabatCal, 1)
};

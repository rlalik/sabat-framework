/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#pragma once

#include <spark/parameters/lookup.hpp>
#include <spark/parameters/tabular.hpp>

/*** Containers ***/

using SabatLookup = spark::lookup_table<std::tuple<uint8_t, uint8_t>, std::tuple<uint8_t, uint8_t>>;
using SiPMCalPar = spark::tabular_par<std::tuple<uint8_t, uint8_t>, std::tuple<int, int, double>>;

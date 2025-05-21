#pragma once

#include <spark/parameters/lookup.hpp>
#include <spark/parameters/tabular.hpp>

/*** Containers ***/

using SabatPMLookup = spark::lookup_table<std::tuple<size_t, size_t>, std::tuple<size_t, size_t>>;
using SabatPMCal = spark::tabular_par<std::tuple<size_t, size_t>, std::tuple<int, int, double>>;

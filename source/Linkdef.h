/*************************************************************************
 * Copyright (C) 2025, Rafał Lalik <rafal.lalik@uj.edu.pl>               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see LICENSE file.                             *
 * For the list of contributors see README.md file.                      *
 *************************************************************************/

#ifdef __CLING__

// clang-format off

// #pragma link off all globals;
// #pragma link off all classes;
// #pragma link off all functions;

// #pragma link C++ nestedclasses;
// #pragma link C++ nestedtypedefs;

#pragma link C++ class SiPMRaw+;
#pragma link C++ class SiPMCal+;
#pragma link C++ class PhotonHit+;

// containers

#pragma link C++ class SabatLookup+;
#pragma link C++ class SiPMCalPar+;

// obsolete
#pragma link C++ class SabatPixelLookup+;

// clang-format on

#endif

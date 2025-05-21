# ---- Developer mode ----

# Developer mode enables targets and code paths in the CMake scripts that are
# only relevant for the developer(s) of sabat-framework
# Targets necessary to build the project must be provided unconditionally, so
# consumers can trivially build and package the project
if(PROJECT_IS_TOP_LEVEL)
  option(sabat-framework_DEVELOPER_MODE "Enable developer mode" OFF)
  option(BUILD_SHARED_LIBS "Build shared libs." OFF)
endif()

# ---- Suppress C4251 on Windows ----

# A note about the MSVC warning C4251:
# This warning should be suppressed for private data members of the project's
# exported classes, because there are too many ways to work around it and all
# involve some kind of trade-off (increased code complexity requiring more
# developer time, writing boilerplate code, longer compile times), but those
# solutions are very situational and solve things in slightly different ways,
# depending on the requirements of the project.
# That is to say, there is no general solution.
#
# What can be done instead is understand where issues could arise where this
# warning is spotting a legitimate bug. I will give the general description of
# this warning's cause and break it down to make it trivial to understand.
#
# C4251 is emitted when an exported class has a non-static data member of a
# non-exported class type.
#
# The exported class in our case is a class below (exported_class), which
# has a non-static data member (m_name) of a non-exported class type e.g.
# (std::string).
#
# The rationale here is that the user of the exported class could attempt to
# access (directly, or via an inline member function) a static data member or
# a non-inline member function of the data member, resulting in a linker
# error.
# Inline member function above means member functions that are defined (not
# declared) in the class definition.
#
# Since this exported class never makes these non-exported types available to
# the user, we can safely ignore this warning. It's fine if there are
# non-exported class types as private member variables, because they are only
# accessed by the members of the exported class itself.
#
# For example, the name() method returns a pointer to the stored null-terminated
# string as a fundamental type (char const), so this is safe to use anywhere.
# The only downside is that you can have dangling pointers if the pointer
# outlives the class instance which stored the string.
#
# Shared libraries are not easy, they need some discipline to get right, but
# they also solve some other problems that make them worth the time invested.

set(pragma_suppress_c4251 "
/* This needs to suppress only for MSVC */
#if defined(_MSC_VER) && !defined(__ICL)
#  define SABAT_FRAMEWORK_SUPPRESS_C4251 _Pragma(\"warning(suppress:4251)\")
#else
#  define SABAT_FRAMEWORK_SUPPRESS_C4251
#endif
")

# ---- Warning guard ----

# target_include_directories with the SYSTEM modifier will request the compiler
# to omit warnings from the provided paths, if the compiler supports that
# This is to provide a user experience similar to find_package when
# add_subdirectory or FetchContent is used to consume this project
set(warning_guard "")
if(NOT PROJECT_IS_TOP_LEVEL)
  option(
      sabat-framework_INCLUDES_WITH_SYSTEM
      "Use SYSTEM modifier for sabat-framework's includes, disabling warnings"
      ON
  )
  mark_as_advanced(sabat-framework_INCLUDES_WITH_SYSTEM)
  if(sabat-framework_INCLUDES_WITH_SYSTEM)
    set(warning_guard SYSTEM)
  endif()
endif()

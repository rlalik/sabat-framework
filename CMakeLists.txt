cmake_minimum_required(VERSION 3.25)

include(cmake/prelude.cmake)

project(
    sabat-framework
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

option(BUILD_SHARED_LIBS "Build as shared lib" ON)

# ---- Dependencies ----

set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE OPT_IN)

include(FetchContent)

FetchContent_Declare(
  spark
  GIT_REPOSITORY https://github.com/rlalik/spark
  GIT_TAG        master
  FIND_PACKAGE_ARGS 0.1.1
)

FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG        4160d259d961cd393fd8d67590a8c7d210207348
  FIND_PACKAGE_ARGS 2.5.0
)

FetchContent_MakeAvailable(CLI11 spark)

# find ROOT
find_package(ROOT QUIET REQUIRED COMPONENTS Core ROOTNTuple)

# ---- Declare library ----

add_library(
    sabat
    source/citiroc_bin_source.cpp
)
add_library(sabat::sabat ALIAS sabat)

target_link_libraries(sabat
  PUBLIC
    spark::spark
)

include(GenerateExportHeader)
generate_export_header(
    sabat
    BASE_NAME sabat
    EXPORT_FILE_NAME export/sabat/sabat_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(sabat PUBLIC SABAT_FRAMEWORK_STATIC_DEFINE)
endif()

set_target_properties(
    sabat PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME sabat
    OUTPUT_NAME sabat
)

target_include_directories(
    sabat ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    sabat SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(sabat PUBLIC cxx_std_23)

ROOT_GENERATE_DICTIONARY(G__sabat_cc
    ${PROJECT_SOURCE_DIR}/include/sabat/sabat_categories.hpp
    ${PROJECT_SOURCE_DIR}/include/sabat/sabat_definitions.hpp
    MODULE sabat
    LINKDEF source/Linkdef.h
    NO_CXXMODULE
)

# ---- Declare executables ----

add_executable(sabat_analysis_exe tools/sabat_analysis.cpp)
add_executable(sabat::sabat_analysis ALIAS sabat_analysis_exe)

set_property(TARGET sabat_analysis_exe PROPERTY OUTPUT_NAME sabat-analysis)

#target_compile_features(sabat_analysis_exe PRIVATE cxx_std_23)

target_link_libraries(sabat_analysis_exe
    PRIVATE
        sabat
        CLI11::CLI11
)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT sabat-framework_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of sabat-framework"
  )
endif()

include(cmake/dev-mode.cmake)

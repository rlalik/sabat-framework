find_package(spark @spark_VERSION@ REQUIRED HINTS @CMAKE_INSTALL_PREFIX@)

@PACKAGE_INIT@

message(STATUS "sabat-framework found: @sabat_framework_VERSION@")
include("${CMAKE_CURRENT_LIST_DIR}/sabat-framework-targets.cmake")

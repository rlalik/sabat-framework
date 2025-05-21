if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/sabat-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package sabat-framework)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT sabat-framework_Development
)

#install(
#    TARGETS sabat sabat_analysis_exe
#    DESTINATION ${CMAKE_INSTALL_BINDIR}
#    RUNTIME COMPONENT sabat-framework_Runtime
#)

install(
    TARGETS sabat sabat_analysis_exe
    EXPORT sabat-framework-targets
    RUNTIME #
    COMPONENT spark_Runtime
    LIBRARY #
    COMPONENT spark_Runtime
    NAMELINK_COMPONENT spark_Development
    ARCHIVE #
    COMPONENT spark_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}-config-version.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    sabat-framework_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE sabat-framework_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(sabat-framework_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${sabat-framework_INSTALL_CMAKEDIR}"
    RENAME "${package}-config.cmake"
    COMPONENT sabat-framework_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}-config-version.cmake"
    DESTINATION "${sabat-framework_INSTALL_CMAKEDIR}"
    COMPONENT sabat-framework_Development
)

install(
    EXPORT sabat-framework-targets
    NAMESPACE sabat::
    DESTINATION "${sabat-framework_INSTALL_CMAKEDIR}"
    COMPONENT sabat-framework_Development
)

install(
    FILES
        ${PROJECT_BINARY_DIR}/libsabat_rdict.pcm
        ${PROJECT_BINARY_DIR}/libsabat.rootmap
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    FILES
        ${PROJECT_SOURCE_DIR}/tools/draw_hists.C
    DESTINATION ${CMAKE_INSTALL_DATADIR}
)

configure_file(profile.sh.in ${PROJECT_BINARY_DIR}/sabat_profile.sh @ONLY)
install(
    FILES
        ${PROJECT_BINARY_DIR}/sabat_profile.sh
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()

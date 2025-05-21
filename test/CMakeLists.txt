# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(sabat-frameworkTests LANGUAGES CXX)

# ---- Tests ----

add_executable(sabat-framework_test source/sabat-framework_test.cpp)
target_link_libraries(sabat-framework_test PRIVATE sabat)
target_compile_features(sabat-framework_test PRIVATE cxx_std_20)

add_test(NAME sabat-framework_test COMMAND sabat-framework_test)

# ---- End-of-file commands ----

add_folders(Test)

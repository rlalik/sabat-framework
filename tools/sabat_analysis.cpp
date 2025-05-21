#include <sabat/citiroc_bin_source.hpp>
#include <sabat/citiroc_bin_unpacker.hpp>
#include <sabat/sabat_categories.hpp>
#include <sabat/sabat_detector.hpp>

#include <spark/spark.hpp>

#include <cstdint>
#include <memory>

#include <CLI/CLI.hpp>
#include <fmt/core.h>

// #include <magic_enum/magic_enum.hpp>

auto main(int argc, char** argv) -> int
{
    CLI::App app {"App description"};
    argv = app.ensure_utf8(argv);

    int64_t n_events_to_process {0};
    app.add_option("-e,--events", n_events_to_process, "Number of events")->check(CLI::PositiveNumber);

    std::string input_file {};
    app.add_option("input_file", input_file, "file to process")->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    // spdlog::set_level(spdlog::level::debug);

    auto sprk = spark::sparksys::create<SabatCategories>("output_sabat.root", "T");

    /*** DETECTORS ****/
    auto* sabat_det = sprk.make_detector<sabat_detector>("SabatEye");

    sprk.register_category(SabatCategories::SabatRaw, "SabatRaw", {2, 64}, true);
    sprk.register_category(SabatCategories::SabatCal, "SabatCal", {2, 64}, false);

    auto citiroc_unp = sprk.make_unpacker<spark::citiroc::bin_unpacker>();

    auto citiroc_src = std::make_shared<spark::citiroc::bin_source>();
    citiroc_src->register_hw_address(0x14520000, 0x1000);

    // citiroc_src->set_input("/tmp/Run15_list.dat");
    citiroc_src->set_input(input_file);

    citiroc_src->add_unpacker(citiroc_unp, 0x1000);
    // citiroc_src->add_unpacker(citiroc_unp, spark::make_address_range<0x1000, 0x1009>());

    sprk.add_source(citiroc_src.get());

    // auto n_events = citiroc_src->get_no_events();

    sprk.book();

    citiroc_unp->init();

    sprk.process_data(n_events_to_process);

    return 0;
}

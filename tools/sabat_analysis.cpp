#include <sabat/citiroc_bin_source.hpp>
#include <sabat/citiroc_bin_unpacker_timing.hpp>
#include <sabat/citiroc_bin_unpacker_spectroscopy.hpp>
#include <sabat/sabat.hpp>
#include <sabat/sabat_categories.hpp>
#include <sabat/sabat_detector.hpp>

#include <spark/core/writer_tree.hpp>
#include <spark/parameters/parameters_ascii_source.hpp>
#include <spark/spark.hpp>

#include <cstdint>
#include <memory>

#include <CLI/CLI.hpp>
#include <fmt/core.h>

auto main(int argc, char** argv) -> int
{
    CLI::App app {"Sabat DST application"};
    argv = app.ensure_utf8(argv);

    int64_t first_event {0};
    app.add_option("-f,--first", first_event, "number of events to skip")->check(CLI::PositiveNumber);

    int64_t n_events_to_process {0};
    app.add_option("-e,--events", n_events_to_process, "number of events to analyze")->check(CLI::PositiveNumber);

    std::string ascii_par {"sabat_pars.txt"};
    app.add_option("-a,--ascii", ascii_par, "ascii parameters file")->check(CLI::ExistingFile);

    std::string input_file {};
    app.add_option("input_file", input_file, "file to process")->check(CLI::ExistingFile);

    std::string output_file {"output_sabat.root"};
    app.add_option("-o,--output", output_file, "output file");

    bool debug_mode {false};
    app.add_flag("-d", debug_mode, "debug mode");

    CLI11_PARSE(app, argc, argv);

    if (debug_mode) {
        spdlog::set_level(spdlog::level::debug);
    }

    //******************//
    // SPARK/SABAT part //
    //******************//

    auto sabat = sabat::SabatMain {};

    /*** Parameters and sources ***/
    auto ascii_source = std::make_unique<spark::parameters_ascii_source>(ascii_par);
    sabat.pardb().add_source(ascii_source.get());

    auto citiroc_src = std::make_shared<spark::citiroc::bin_source>();
    citiroc_src->register_hw_address(0x14520000, 0x0000);
    citiroc_src->set_input(input_file);

    if (citiroc_src->open()) {
        auto hdr = citiroc_src->header();

        switch (hdr->acq_mode) {
            case 0x01: {
                auto citiroc_unp =
                sabat.tasks().make_unpacker<spark::citiroc::bin_unpacker_spectroscopy<SabatLookup>>("CitirocBinSpectroscopyUnpacker");
                citiroc_src->add_unpacker(citiroc_unp, 0x0000);
            } break;
            case 0x02: {
                auto citiroc_unp =
                    sabat.tasks().make_unpacker<spark::citiroc::bin_unpacker_timing<SabatLookup>>("CitirocBinTimingUnpacker");
                citiroc_src->add_unpacker(citiroc_unp, 0x0000);
            } break;
            default: {
                spdlog::error("Citiroc Acquisition Mode not supported by any unpacker");
                std::exit(2);
            }
        }
    }

    sabat.add_source(citiroc_src.get());

    sabat.init();

    auto writer = sabat.create_writer<spark::writer::tree>("T", output_file, 0);
    writer.process_data(n_events_to_process);

    return 0;
}

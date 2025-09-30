#include <spark/spark_config.hpp>

#include <sabat/sabat.hpp>
#include <sabat/sabat_categories.hpp>
#include <spark/core/reader_tree.hpp>

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include <filesystem>

namespace fs = std::filesystem;

const auto n_chan_mod_0 = 52;
const auto n_pixel_mod_0 = 64;
const int pixel_map_mod_0[n_pixel_mod_0] = {2,  3,  4,  5,  9,  10, 11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 23,
                                            24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
                                            42, 43, 44, 45, 46, 47, 49, 50, 51, 52, 53, 54, 58, 59, 60, 61};

const auto n_chan_mod_1 = 12;
const auto n_pixel_mod_1 = 12;
const int pixel_map_mod_1[n_pixel_mod_1] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

void draw_hists(const char* file = "output_sabat.root")
{
    const auto input_path = fs::path(file);
    auto pathname = input_path.parent_path();
    if (pathname.empty()) {
        pathname = fs::path(".");
    }

    const auto out_file = pathname / (fs::path(std::string("hist_") + input_path.filename().c_str()));
    fmt::print("Input file : {}\nOutput file: {}\n", file, out_file.c_str());

    auto sabat = sabat::SabatMain {};
    sabat.init();

    auto reader = sabat.create_reader<spark::reader::tree>("T");
    reader.add_file(file);

    auto* chain = reader.chain();
    // chain->Print();

    reader.set_input({SabatCategories::SiPMRaw});

    auto cat_fibers_raw = reader.model().get_category(SabatCategories::SiPMRaw);

    auto outfile = TFile::Open(out_file.c_str(), "RECREATE");

    auto* h_toa_mod_0 = new TH2I("h_toa_mod_0", "", 64, 0, 64, 512, 0, 256);
    auto* h_toa_mod_1 = new TH2I("h_toa_mod_1", "", 64, 0, 64, 512, 0, 256);

    TH1I* h_tot_mod_0[n_chan_mod_0];
    TH2I* h_tot_toa_mod_0[n_chan_mod_0];

    for (int i = 0; i < n_chan_mod_0; ++i) {
        h_tot_mod_0[i] = new TH1I(std::format("h_tot_mod_0_{:02d}", i).c_str(),
                                  std::format("tot mod 0 chan {:02d};ToT [ns];counts", i).c_str(),
                                  512,
                                  0,
                                  256);
        h_tot_toa_mod_0[i] = new TH2I(std::format("h_tot_toa_mod_0_{:02d}", i).c_str(),
                                      std::format("tot vs toa mod 0 chan {:02d};ToA [ns];ToT [ns]", i).c_str(),
                                      512,
                                      0,
                                      256,
                                      512,
                                      0,
                                      256);
    }

    TH1I* h_tot_mod_1[n_chan_mod_1];
    TH2I* h_tot_toa_mod_1[n_chan_mod_1];
    for (int i = 0; i < n_chan_mod_1; ++i) {
        h_tot_mod_1[i] = new TH1I(std::format("h_tot_mod_1_{:02d}", i).c_str(),
                                  std::format("tot mod 1 chan {:02d};ToT [ns];counts", i).c_str(),
                                  512,
                                  0,
                                  256);
        h_tot_toa_mod_1[i] = new TH2I(std::format("h_tot_toa_mod_1_{:02d}", i).c_str(),
                                      std::format("tot vs toa mod 1 chan {:02d};ToA [ns];ToT [ns]", i).c_str(),
                                      512,
                                      0,
                                      256,
                                      512,
                                      0,
                                      256);
    }

    for (auto i = 0l; i < chain->GetEntries(); ++i) {
        // printf("Entry: %ld\n", i);
        reader.get_entry(i);
        // cat_fibers_raw->print();

        auto n = cat_fibers_raw->get_entries();

        for (int j = 0; j < n; ++j) {
            auto raw_hit = cat_fibers_raw->get_object<SiPMRaw>(j);

            if (raw_hit->board == 0) {
                h_toa_mod_0->Fill(raw_hit->channel, raw_hit->toa);
                h_tot_mod_0[raw_hit->channel]->Fill(raw_hit->tot);
                h_tot_toa_mod_0[raw_hit->channel]->Fill(raw_hit->toa, raw_hit->tot);
            } else {
                h_toa_mod_1->Fill(raw_hit->channel, raw_hit->toa);
                h_tot_mod_1[raw_hit->channel]->Fill(raw_hit->tot);
                h_tot_toa_mod_1[raw_hit->channel]->Fill(raw_hit->toa, raw_hit->tot);
            }
        }
    }

    auto* can = new TCanvas("can", "can", 800, 400);
    can->Divide(2, 1);

    can->cd(1);
    h_toa_mod_0->Draw();

    can->cd(2);
    h_toa_mod_1->Draw();

    auto* can_energy_0 = new TCanvas("can_energy_0", "can_energy_0", 800, 800);
    can_energy_0->DivideSquare(n_chan_mod_0);

    for (int i = 0; i < n_chan_mod_0; ++i) {
        can_energy_0->cd(1 + i);
        h_tot_mod_0[i]->Draw();
    }

    auto* can_energy_1 = new TCanvas("can_energy_1", "can_energy_1", 800, 800);
    can_energy_1->DivideSquare(n_chan_mod_1);

    for (int i = 0; i < n_chan_mod_1; ++i) {
        can_energy_1->cd(1 + i);
        h_tot_mod_1[i]->Draw();
    }

    auto* can_energy_map_0 = new TCanvas("can_energy_map_0", "can_energy_map_0", 800, 800);
    can_energy_map_0->DivideSquare(n_pixel_mod_0);

    for (int i = 0; i < n_chan_mod_0; ++i) {
        can_energy_map_0->cd(1 + pixel_map_mod_0[i]);
        h_tot_mod_0[i]->Draw();
    }

    auto* can_energy_map_1 = new TCanvas("can_energy_map_1", "can_energy_map_1", 800, 800);
    can_energy_map_1->DivideSquare(n_pixel_mod_1);

    for (int i = 0; i < n_chan_mod_1; ++i) {
        can_energy_map_1->cd(1 + pixel_map_mod_1[i]);
        h_tot_mod_1[i]->Draw();
    }

    auto* can_tot_toa_0 = new TCanvas("can_tot_toa_0", "can_tot_toa_0", 800, 800);
    can_tot_toa_0->DivideSquare(n_chan_mod_0);

    for (int i = 0; i < n_chan_mod_0; ++i) {
        // can_tot_toa_0->cd(1 + pixel_map_mod_0[i]);
        can_tot_toa_0->cd(1 + i);
        h_tot_toa_mod_0[i]->Draw("colz");
        gPad->SetLogz();
    }

    auto* can_tot_toa_1 = new TCanvas("can_tot_toa_1", "can_tot_toa_1", 800, 800);
    can_tot_toa_1->DivideSquare(n_pixel_mod_1);

    for (int i = 0; i < n_chan_mod_1; ++i) {
        can_tot_toa_1->cd(1 + pixel_map_mod_1[i]);
        h_tot_toa_mod_1[i]->Draw("colz");
        gPad->SetLogz();
    }

    can_energy_0->Write();
    can_energy_1->Write();
    can_energy_map_0->Write();
    can_energy_map_1->Write();
    can_tot_toa_0->Write();
    can_tot_toa_1->Write();

    outfile->Write();
}

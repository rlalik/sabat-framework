#include <sabat/sabat.hpp>
#include <sabat/sabat_categories.hpp>

#include <spark/core/reader_tree.hpp>

#include <RQ_OBJECT.h>
#include <TEveBoxSet.h>
#include <TEveBrowser.h>
#include <TEveEventManager.h>
#include <TEveGeoNode.h>
#include <TEveGeoShape.h>
#include <TEveManager.h>
#include <TEveProjectionManager.h>
#include <TEveScene.h>
#include <TEveViewer.h>
#include <TGColorSelect.h>
#include <TGComboBox.h>
#include <TGButtonGroup.h>
#include <TGLLightSet.h>
#include <TGLViewer.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGSlider.h>
#include <TGTab.h>
#include <TGeoManager.h>
#include <TCanvas.h>
#include <TF2.h>
#include <TGeoMedium.h>
#include <TGeoVolume.h>
#include <TPaletteAxis.h>
#include <TROOT.h>
#include <TRootEmbeddedCanvas.h>
#include <TStyle.h>
#include <TSystem.h>

#include <array>
#include <vector>

#include <spdlog/spdlog.h>

constexpr auto mm = 1e-1;
constexpr auto cm = 1e+0;

constexpr auto LaBr3Z = 5.08 * cm;

constexpr auto SiPM_X = 6.0 * mm;
constexpr auto SiPM_Y = 6.0 * mm;
constexpr auto SiPM_Z = 0.5 * mm;

constexpr auto BGO_X = 6.0 * mm;
constexpr auto BGO_Y = 6.0 * mm;
constexpr auto BGO_Z = 60.0 * mm;

namespace
{
auto f_clear_node = [](TGeoNode* node) { node->GetVolume()->SetLineColor(kWhite); };
auto f_clear_subnode = [](TGeoNode* node) { node->GetVolume()->GetNode(0)->GetVolume()->SetLineColor(kWhite); };
}  // namespace

class event_viewer : public TObject
{
    RQ_OBJECT("event_viewer")

    sabat::SabatMain sabat {};
    spark::reader::tree reader {&sabat, "T"};

    long long event_id {0};    ///< Current event id.
    const Int_t jumps_no {5};  ///< Number of JumpNav fields

    int sipms_node_alpha {0};     ///< active fibers color
    int bgos_node_alpha {0};      ///< active fibers color

    int sipm_raw_toa_color {28};  ///< deposited charge color
    int sipm_raw_toa_alpha {95};

    int bgo_raw_toa_color {41};   ///< deposited charge color
    int bgo_raw_toa_alpha {95};

    int toa_scale {25};           ///< toa scale is common for SiPMs and BGos

    TGLabel* l_evt {nullptr};     ///< Displays current event number
    TGLabel* l_all {nullptr};     ///< Displays all events number

    TGRadioButton* btn_sipm_raw {nullptr};
    TGRadioButton* btn_sipm_cal {nullptr};

    static constexpr size_t n_sipms {52};
    std::vector<TGeoNode*> SiPM_nodes {n_sipms};

    static constexpr size_t n_bgos {28};
    std::vector<TGeoNode*> BGO_nodes {n_bgos};

    static constexpr size_t n_bgos_clusters {13};
    std::array<std::vector<size_t>, n_bgos_clusters> BGO_clusters {};

    static constexpr std::pair<int, int> toa_range {0, 256};
    static constexpr std::pair<int, int> tot_range {0, 256};
    static constexpr std::pair<int, int> energy_range {-100, 5000};

    enum class SiPM_input
    {
        RAW,
        CAL,
    } sipm_selected_input {SiPM_input::RAW};

    TRootEmbeddedCanvas* embedded_canvas;
    TPaletteAxis* palette {nullptr};

public:
    bool block_event_reload {false};  // with that set, the event won't be reload, useful on the first configuration

public:
    event_viewer(const char* file,
                 const char* ascii_params = "sabat_params.txt",
                 const char* sabat_geometry = "sabat.gdml")
        : TObject()
    {
        BGO_clusters[0] = {0, 1};
        BGO_clusters[1] = {2, 3};
        BGO_clusters[2] = {4, 5};
        BGO_clusters[3] = {6, 7, 8};
        BGO_clusters[4] = {9, 10};
        BGO_clusters[5] = {11, 12};
        BGO_clusters[6] = {13, 14};
        BGO_clusters[7] = {15, 16};
        BGO_clusters[8] = {17, 18};
        BGO_clusters[9] = {19, 20};
        BGO_clusters[10] = {21, 22, 23};
        BGO_clusters[11] = {24, 25};
        BGO_clusters[12] = {26, 27};

        spdlog::info("Init Eve Manager");

        TEveManager::Create();

        spdlog::info("Init Sabat");
        sabat.init();
        sabat.init_reader_system();

        if (file) {
            // input file can be passed by macro parameter
            reader.add_file(file);
        } else {
            // or taking already open files
            TSeqCollection* files = gROOT->GetListOfFiles();
            for (int i = 0; i < files->GetEntries(); ++i) {
                reader.add_file(((TFile*)(files->At(i)))->GetName());
            }
        }

        reader.set_input({SabatCategories::SiPMRaw, SabatCategories::SiPMCal, SabatCategories::PhotonHit});

        /**** GEOMETRY ****/
        std::string params_file(ascii_params);

        spdlog::info("Init Geometry");
        auto* aGeom = TGeoManager::Import(sabat_geometry);
        if (!aGeom) {
            spdlog::critical("Geometry could not be imported from {:s}.", sabat_geometry);
            std::exit(1);
        }

        auto* topEveNode = new TEveGeoTopNode(aGeom, aGeom->GetTopNode());
        gEve->AddGlobalElement(topEveNode);

        /* Hide volumes which are not essential to display the event. */
        auto hide_volume = [&](const char* name) { aGeom->GetVolume(name)->SetVisibility(kFALSE); };

        hide_volume("Reflector");
        hide_volume("Reflectorface");
        hide_volume("lhousing");
        hide_volume("lLaBr3");
        hide_volume("Teflon");
        hide_volume("Glass_window");
        hide_volume("optgel");

        /* As the geometry file doesn't have any tree-like structure, but rather almost all nodes and nodes are on the
         * same level, one needs to iterate over all nodes, check whether they are SiPMs or BGOs, check their copy
         * number and store the pointers for the future use. SiPMs are accessed directly from the second nodes level,
         * but BGOs are inside BGOW nodes. */
        auto tnode = aGeom->GetTopNode();
        auto n_daughters = tnode->GetNdaughters();

        for (int i = 0; i < n_daughters; ++i) {
            auto node = tnode->GetDaughter(i);
            TString node_name = node->GetName();

            if (node_name.BeginsWith("Physi_SiPM")) {
                node->SetVolume(dynamic_cast<TGeoVolume*>(node->GetVolume()->Clone()));
                node->GetVolume()->SetLineColor(kWhite);
                SiPM_nodes[node->GetNumber()] = node;
            } else if (node_name.BeginsWith("BGOW")) {
                node->SetVolume(dynamic_cast<TGeoVolume*>(node->GetVolume()->Clone()));
                node->GetVolume()->SetLineColor(kWhite);
                BGO_nodes[node->GetNumber()] = node;
            }
        }

        spdlog::info(" SiPM volumes: {:d}", SiPM_nodes.size());
        spdlog::info(" BGO  volumes: {:d}", BGO_nodes.size());

        spdlog::info("Init View");

        /**** VIEW ****/
        // auto gMultiView = new MultiView;
        // gMultiView->f3DView->GetGLViewer()->SetStyle(TGLRnrCtx::kOutline);
        // gMultiView->SetDepth(-10);
        // gMultiView->ImportGeomRPhi(topNode);
        // gMultiView->ImportGeomRhoZ(topNode);
        // gMultiView->SetDepth(0);

        gEve->FullRedraw3D(kTRUE);

        /**** GL Viewer ****/
        auto v = gEve->GetDefaultGLViewer();
        //     v->GetClipSet()->SetClipType(TGLClip::EType(1));
        v->SetGuideState(TGLUtil::kAxesOrigin, kTRUE, kFALSE, 0);
        v->RefreshPadEditor(v);
        v->GetLightSet()->SetLight(TGLLightSet::kLightLeft, kTRUE);
        v->GetLightSet()->SetLight(TGLLightSet::kLightRight, kTRUE);
        v->GetLightSet()->SetLight(TGLLightSet::kLightTop, kTRUE);
        v->GetLightSet()->SetLight(TGLLightSet::kLightBottom, kTRUE);
        v->GetLightSet()->SetLight(TGLLightSet::kLightFront, kTRUE);
        v->DoDraw();

        /**** GUI ****/
        gEve->GetViewers()->SwitchColorSet();
        gEve->GetDefaultGLViewer()->SetStyle(TGLRnrCtx::kOutline);

        TEveBrowser* browser = gEve->GetBrowser();

        browser->GetTabRight()->SetTab(0);
        browser->StartEmbedding(TRootBrowser::kLeft);
        make_gui();
        browser->StopEmbedding();
        browser->SetTabTitle("Event Control", 0);

        load_event();
    }

    virtual ~event_viewer() {}

    auto make_gui() -> TGMainFrame*
    {
        block_event_reload = true;

        // Create minimal GUI for event navigation.
        TGMainFrame* main_frame = new TGMainFrame(gClient->GetRoot(), 1000, 600);
        main_frame->SetWindowName("Sabat GUI");
        main_frame->SetCleanup(kDeepCleanup);

        // For displaying palette axis
        embedded_canvas = new TRootEmbeddedCanvas("ECanvas", main_frame, 10, 40, kChildFrame);
        embedded_canvas->GetCanvas()->SetFillColor(18);
        palette = new TPaletteAxis(0.10, 0.40, 0.90, 0.95, 0, 100);
        palette->SetNdivisions(508);
        palette->SetLabelFont(43);
        palette->SetTitleFont(43);
        palette->SetLabelSize(15);
        palette->SetTitleSize(15);
        palette->SetTitle("");
        palette->Draw();

        TGGroupFrame* group_navi = new TGGroupFrame(main_frame, "Basic Navigation", kHorizontalFrame);
        group_navi->SetTitlePos(TGGroupFrame::kLeft);
        {
            //         TString icondir(Form("%s/icons/", gSystem->Getenv("ROOTSYS")));
            //         TGPictureButton* b = 0;
            //         EvNavHandler* fh = new EvNavHandler;
            //
            //         b = new TGPictureButton(groupFrame1, gClient->GetPicture(icondir +
            //         "GoBack.gif")); groupFrame1->AddFrame(b); b->Connect("Clicked()", "event_viewer::EvNavHandler",
            //         fh, "Bck()");
            //
            //         b = new TGPictureButton(groupFrame1, gClient->GetPicture(icondir +
            //         "GoForward.gif")); groupFrame1->AddFrame(b); b->Connect("Clicked()",
            //         "event_viewer::EvNavHandler", fh, "Fwd()");

            auto b_prev = new TGTextButton(group_navi, new TGHotString("&Prev"));
            group_navi->AddFrame(b_prev, new TGLayoutHints(kLHintsExpandY, 5, 5, 5, 5));
            b_prev->Connect("Clicked()", "event_viewer", this, "Prev()");

            TGVerticalFrame* group_event_info = new TGVerticalFrame(group_navi);

            TGHorizontalFrame* group_event_current = new TGHorizontalFrame(group_event_info);

            TGLabel* l = new TGLabel(group_event_current, "Event:");
            group_event_current->AddFrame(l, new TGLayoutHints(kLHintsExpandY));

            l_evt = new TGLabel(group_event_current, "");
            group_event_current->AddFrame(l_evt, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

            TGHorizontalFrame* group_event_all = new TGHorizontalFrame(group_event_info);

            l = new TGLabel(group_event_all, "Total:");
            group_event_all->AddFrame(l, new TGLayoutHints(kLHintsExpandY));

            l_all = new TGLabel(group_event_all, "");
            group_event_all->AddFrame(l_all, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

            group_event_info->AddFrame(group_event_current, new TGLayoutHints(kLHintsExpandX));
            group_event_info->AddFrame(group_event_all, new TGLayoutHints(kLHintsExpandX));

            group_navi->AddFrame(group_event_info, new TGLayoutHints(kLHintsExpandX));

            auto b_next = new TGTextButton(group_navi, new TGHotString("&Next"));
            group_navi->AddFrame(b_next, new TGLayoutHints(kLHintsExpandY, 5, 5, 5, 5));
            b_next->Connect("Clicked()", "event_viewer", this, "Next()");
        }
        main_frame->AddFrame(group_navi, new TGLayoutHints(kLHintsExpandX));

        TGGroupFrame* frame_quick_nav = new TGGroupFrame(main_frame, "Quick Navigation", kVerticalFrame);
        frame_quick_nav->SetTitlePos(TGGroupFrame::kLeft);
        for (int i = 0; i < jumps_no; ++i) {
            auto hf = new TGHorizontalFrame(frame_quick_nav);
            {
                TGTextButton* b1 = new TGTextButton(hf, "Load");
                hf->AddFrame(b1, new TGLayoutHints(kLHintsExpandX));

                TGLabel* l = new TGLabel(hf, "  Selected ");
                hf->AddFrame(l, new TGLayoutHints(kLHintsCenterY));

                TGNumberEntry* l_evt_jump = new TGNumberEntry(hf,
                                                              0,
                                                              5,
                                                              -1,
                                                              TGNumberFormat::kNESInteger,
                                                              TGNumberFormat::kNEAAnyNumber,
                                                              TGNumberFormat::kNELLimitMinMax,
                                                              0,
                                                              reader.get_entries() - 1);
                hf->AddFrame(l_evt_jump);

                QuickJumpNavHandler* fhj = new QuickJumpNavHandler(this, l_evt_jump);

                TGTextButton* b2 = new TGTextButton(hf, TString::Format("Jump &%d", i + 1));
                hf->AddFrame(b2, new TGLayoutHints(kLHintsExpandX, 5, 0, 0, 0));

                b1->Connect("Clicked()", "event_viewer::QuickJumpNavHandler", fhj, "Load()");
                b2->Connect("Clicked()", "event_viewer::QuickJumpNavHandler", fhj, "Jump()");
            }
            frame_quick_nav->AddFrame(hf, new TGLayoutHints(kLHintsExpandX, 0, 0, 2, 2));
        }
        main_frame->AddFrame(frame_quick_nav, new TGLayoutHints(kLHintsExpandX));

        // Input selection
        auto* gr_input_selection = new TGButtonGroup(main_frame, "Input selection", kHorizontalFrame);
        gr_input_selection->SetTitlePos(TGGroupFrame::kLeft);
        {
            btn_sipm_raw = new TGRadioButton(gr_input_selection, new TGHotString("&RAW"));
            btn_sipm_raw->SetTextJustify(36);

            btn_sipm_cal = new TGRadioButton(gr_input_selection, new TGHotString("&CAL"));
            btn_sipm_cal->SetTextJustify(36);

            gr_input_selection->SetLayoutHints(new TGLayoutHints(kLHintsExpandX), btn_sipm_raw);
            gr_input_selection->SetLayoutHints(new TGLayoutHints(kLHintsExpandX), btn_sipm_cal);

            btn_sipm_raw->Connect("Clicked()", "event_viewer", this, "SelectInput(=0)");
            btn_sipm_cal->Connect("Clicked()", "event_viewer", this, "SelectInput(=1)");

            switch (sipm_selected_input) {
                case SiPM_input::RAW:
                    btn_sipm_raw->SetState(kButtonDown, kTRUE);
                    break;
                case SiPM_input::CAL:
                    btn_sipm_cal->SetState(kButtonDown, kTRUE);
                    break;
            }
        }

        main_frame->AddFrame(gr_input_selection, new TGLayoutHints(kLHintsExpandX));

        // Visual properties
        TGGroupFrame* gr_visual_properties = new TGGroupFrame(main_frame, "Visual Properties", kVerticalFrame);
        gr_visual_properties->SetTitlePos(TGGroupFrame::kLeft);
        {
            VisualHandler* vish = new VisualHandler(this, embedded_canvas);

            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);
                TGLabel* l = new TGLabel(hf, "Color Palette");
                hf->AddFrame(l, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                auto* hcbpalette = new TGComboBox(hf, 50);

                auto add_palette_to_cb = [&](auto id, auto name) { hcbpalette->AddEntry(name, id); };
                add_palette_to_cb(kDeepSea, "Deap Sea");
                add_palette_to_cb(kGreyScale, "Grey Scale");
                add_palette_to_cb(kDarkBodyRadiator, "Dark Body Radiator");
                add_palette_to_cb(kBlueYellow, "Blue Yellow");
                add_palette_to_cb(kRainBow, "Rain Bow");
                add_palette_to_cb(kInvertedDarkBodyRadiator, "Inverted Dark Body Radiator");
                add_palette_to_cb(kBird, "Bird");
                add_palette_to_cb(kCubehelix, "Cube helix");
                add_palette_to_cb(kGreenRedViolet, "Green Red Violet");
                add_palette_to_cb(kBlueRedYellow, "Blue Red Yellow");
                add_palette_to_cb(kOcean, "Ocean");
                add_palette_to_cb(kColorPrintableOnGrey, "Color Printable On Grey");
                add_palette_to_cb(kAlpine, "Alpine");
                add_palette_to_cb(kAquamarine, "Aquamarine");
                add_palette_to_cb(kArmy, "Army");
                add_palette_to_cb(kAtlantic, "Atlantic");
                add_palette_to_cb(kAurora, "Aurora");
                add_palette_to_cb(kAvocado, "Avocado");
                add_palette_to_cb(kBeach, "Beach");
                add_palette_to_cb(kBlackBody, "Black Body");
                add_palette_to_cb(kBlueGreenYellow, "Blue Green Yellow");
                add_palette_to_cb(kBrownCyan, "Brown Cyan");
                add_palette_to_cb(kCMYK, "CMYK");
                add_palette_to_cb(kCandy, "Candy");
                add_palette_to_cb(kCherry, "Cherry");
                add_palette_to_cb(kCoffee, "Coffee");
                add_palette_to_cb(kDarkRainBow, "Dark Rain Bow");
                add_palette_to_cb(kDarkTerrain, "Dark Terrain");
                add_palette_to_cb(kFall, "Fall");
                add_palette_to_cb(kFruitPunch, "Fruit Punch");
                add_palette_to_cb(kFuchsia, "Fuchsia");
                add_palette_to_cb(kGreyYellow, "Grey Yellow");
                add_palette_to_cb(kGreenBrownTerrain, "Green Brown Terrain");
                add_palette_to_cb(kGreenPink, "Green Pink");
                add_palette_to_cb(kIsland, "Island");
                add_palette_to_cb(kLake, "Lake");
                add_palette_to_cb(kLightTemperature, "Light Temperature");
                add_palette_to_cb(kLightTerrain, "Light Terrain");
                add_palette_to_cb(kMint, "Mint");
                add_palette_to_cb(kNeon, "Neon");
                add_palette_to_cb(kPastel, "Pastel");
                add_palette_to_cb(kPearl, "Pearl");
                add_palette_to_cb(kPigeon, "Pigeon");
                add_palette_to_cb(kPlum, "Plum");
                add_palette_to_cb(kRedBlue, "Red Blue");
                add_palette_to_cb(kRose, "Rose");
                add_palette_to_cb(kRust, "Rust");
                add_palette_to_cb(kSandyTerrain, "Sandy Terrain");
                add_palette_to_cb(kSienna, "Sienna");
                add_palette_to_cb(kSolar, "Solar");
                add_palette_to_cb(kSouthWest, "South West");
                add_palette_to_cb(kStarryNight, "Starry Night");
                add_palette_to_cb(kSunset, "Sunset");
                add_palette_to_cb(kTemperatureMap, "Temperature Map");
                add_palette_to_cb(kThermometer, "Thermometer");
                add_palette_to_cb(kValentine, "Valentine");
                add_palette_to_cb(kVisibleSpectrum, "Visible Spectrum");
                add_palette_to_cb(kWaterMelon, "Water Melon");
                add_palette_to_cb(kCool, "Cool");
                add_palette_to_cb(kCopper, "Copper");
                add_palette_to_cb(kGistEarth, "Gist Earth");
                add_palette_to_cb(kViridis, "Viridis");
                add_palette_to_cb(kCividis, "Cividis");

                hcbpalette->Connect("Selected(Int_t)", "event_viewer::VisualHandler", vish, "PaletteChanged(Int_t)");
                hf->AddFrame(hcbpalette, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

                hcbpalette->Select(kBird, kTRUE);

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }

            // SiPMs Properties
            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);
                TGLabel* l = new TGLabel(hf, "SiPMs Alpha");
                hf->AddFrame(l, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                TGHSlider* hslider = new TGHSlider(hf, 100, kSlider1 | kScaleDownRight, 1);
                hslider->SetRange(0, 100);
                hslider->SetPosition(sipms_node_alpha);
                hf->AddFrame(hslider, new TGLayoutHints(kLHintsExpandX));

                auto l_alpha = new TGLabel(hf, "000");
                hf->AddFrame(l_alpha, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                auto* sipms_nth = new NodeAlphaHandler(this, &SiPM_nodes, &sipms_node_alpha, l_alpha);
                hslider->Connect(
                    "PositionChanged(Int_t)", "event_viewer::NodeAlphaHandler", sipms_nth, "ChangeAlpha(Int_t)");
                sipms_nth->ChangeAlpha(sipms_node_alpha);

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }

            // BGOs Properties
            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);
                TGLabel* l = new TGLabel(hf, "BGOs Alpha");
                hf->AddFrame(l, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                TGHSlider* hslider = new TGHSlider(hf, 100, kSlider1 | kScaleDownRight, 1);
                hslider->SetRange(0, 100);
                hslider->SetPosition(bgos_node_alpha);
                hf->AddFrame(hslider, new TGLayoutHints(kLHintsExpandX));

                auto l_alpha = new TGLabel(hf, "000");
                hf->AddFrame(l_alpha, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                auto* bgos_nth = new NodeAlphaHandler(this, &BGO_nodes, &bgos_node_alpha, l_alpha);
                hslider->Connect(
                    "PositionChanged(Int_t)", "event_viewer::NodeAlphaHandler", bgos_nth, "ChangeAlpha(Int_t)");
                bgos_nth->ChangeAlpha(bgos_node_alpha);

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }

            // SiPM
            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);
                TGLabel* l = new TGLabel(hf, "SiPM Raw ToA Box Color");
                hf->AddFrame(l, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

                TGColorSelect* csel = new TGColorSelect(hf, TColor::Number2Pixel(sipm_raw_toa_color), 100);
                hf->AddFrame(csel, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 2, 2, 2));

                auto sipm_raw_toa_ch = new BoxColorHandler(this, &sipm_raw_toa_color);
                csel->Connect(
                    "ColorSelected(Pixel_t)", "event_viewer::BoxColorHandler", sipm_raw_toa_ch, "SelectColor(Pixel_t)");

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }

            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);
                TGLabel* l = new TGLabel(hf, "SiPM ToA Alpha");
                hf->AddFrame(l, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                TGHSlider* hslider = new TGHSlider(hf, 100, kSlider1 | kScaleDownRight, 1);
                hslider->SetRange(0, 100);
                hslider->SetPosition(sipm_raw_toa_alpha);
                hf->AddFrame(hslider, new TGLayoutHints(kLHintsExpandX));

                auto l_alpha = new TGLabel(hf, "000");
                hf->AddFrame(l_alpha, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                auto sipm_raw_toa_ah = new BoxAlphaHandler(this, &sipm_raw_toa_alpha, l_alpha);
                hslider->Connect(
                    "PositionChanged(Int_t)", "event_viewer::BoxAlphaHandler", sipm_raw_toa_ah, "ChangeAlpha(Int_t)");

                sipm_raw_toa_ah->ChangeAlpha(sipm_raw_toa_alpha);

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }

            // BGO
            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);
                TGLabel* l = new TGLabel(hf, "BGO Raw ToA Box Color");
                hf->AddFrame(l, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

                TGColorSelect* csel = new TGColorSelect(hf, TColor::Number2Pixel(bgo_raw_toa_color), 100);
                hf->AddFrame(csel, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 5, 2, 2, 2));

                auto bgo_raw_toa_ch = new BoxColorHandler(this, &bgo_raw_toa_color);
                csel->Connect(
                    "ColorSelected(Pixel_t)", "event_viewer::BoxColorHandler", bgo_raw_toa_ch, "SelectColor(Pixel_t)");

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }

            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);

                TGLabel* l = new TGLabel(hf, "BGO ToA Alpha");
                hf->AddFrame(l, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                TGHSlider* hslider = new TGHSlider(hf, 100, kSlider1 | kScaleDownRight, 1);
                hslider->SetRange(0, 100);
                hslider->SetPosition(bgo_raw_toa_alpha);
                hf->AddFrame(hslider, new TGLayoutHints(kLHintsExpandX));

                auto l_alpha = new TGLabel(hf, "000");
                hf->AddFrame(l_alpha, new TGLayoutHints(kLHintsRight | kLHintsCenterY));

                auto bgo_raw_toa_ah = new BoxAlphaHandler(this, &bgo_raw_toa_alpha, l_alpha);
                hslider->Connect(
                    "PositionChanged(Int_t)", "event_viewer::BoxAlphaHandler", bgo_raw_toa_ah, "ChangeAlpha(Int_t)");

                bgo_raw_toa_ah->ChangeAlpha(bgo_raw_toa_alpha);

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }

            {
                auto hf = new TGHorizontalFrame(gr_visual_properties);
                TGLabel* l = new TGLabel(hf, "ToA Scale");
                hf->AddFrame(l, new TGLayoutHints(kLHintsLeft | kLHintsCenterY));

                TGHSlider* hslider = new TGHSlider(hf, 100, kSlider1 | kScaleDownRight, 1);
                hslider->SetRange(1, 100);
                hslider->SetPosition(toa_scale);
                hf->AddFrame(hslider, new TGLayoutHints(kLHintsExpandX));

                auto l_scale = new TGLabel(hf, "000");
                hf->AddFrame(l_scale, new TGLayoutHints(kLHintsRight | kLHintsCenterY));

                auto common_sh = new BoxScaleHandler(this, &toa_scale, l_scale);

                hslider->Connect(
                    "PositionChanged(Int_t)", "event_viewer::BoxScaleHandler", common_sh, "ChangeScale(Int_t)");

                common_sh->ChangeScale(toa_scale);

                gr_visual_properties->AddFrame(hf, new TGLayoutHints(kLHintsExpandX));
            }
        }
        main_frame->AddFrame(gr_visual_properties, new TGLayoutHints(kLHintsExpandX));

        {
            switch (sipm_selected_input) {
                case SiPM_input::RAW:
                    palette->GetAxis()->SetWmin(tot_range.first);
                    palette->GetAxis()->SetWmax(tot_range.second);
                    break;
                case SiPM_input::CAL:
                    palette->GetAxis()->SetWmin(energy_range.first);
                    palette->GetAxis()->SetWmax(energy_range.second);
                    break;
            }
        }
        main_frame->AddFrame(embedded_canvas, new TGLayoutHints(kLHintsExpandX));

        main_frame->MapSubwindows();
        main_frame->Resize();
        main_frame->MapWindow();

        block_event_reload = false;

        return main_frame;
    }

    //______________________________________________________________________________
    void load_event()
    {
        if (block_event_reload) {
            return;
        }

        // Load event specified in global event_id.
        // The contents of previous event are removed.

        std::print("# Event: {:d} of {:d} total.\n", event_id, reader.get_entries() - 1);

        l_evt->SetText(TString::Format("%5lld", event_id).Data());
        l_all->SetText(TString::Format("%5lld", reader.get_entries()).Data());

        gEve->GetViewers()->DeleteAnnotations();
        reader.get_entry(event_id);

        TEveElement* top = gEve->GetCurrentEvent();  // TODO Why this?
        delete top;
        gEve->SetCurrentEvent(nullptr);

        std::for_each(SiPM_nodes.begin(), SiPM_nodes.end(), f_clear_node);
        std::for_each(BGO_nodes.begin(), BGO_nodes.end(), f_clear_subnode);

        switch (sipm_selected_input) {
            case SiPM_input::RAW:
                sabat_read_sipm_raw();
                break;
            case SiPM_input::CAL:
                sabat_read_sipm_cal();
                break;
        };

        gEve->FullRedraw3D(kFALSE);
    }

    void sabat_read_sipm_raw()
    {
        auto cat = reader.model().get_category(SabatCategories::SiPMRaw);
        auto n = cat->get_entries();

        auto sipm_raw_hits_toa = new TEveBoxSet("sipm_raw_hits_toa");
        sipm_raw_hits_toa->UseSingleColor();
        sipm_raw_hits_toa->SetMainColor(sipm_raw_toa_color);
        sipm_raw_hits_toa->SetMainAlpha(sipm_raw_toa_alpha * 0.01);
        sipm_raw_hits_toa->Reset(TEveBoxSet::kBT_AABox, kFALSE, 64);

        auto bgo_raw_hits_toa = new TEveBoxSet("bgo_raw_hits_toa");
        bgo_raw_hits_toa->UseSingleColor();
        bgo_raw_hits_toa->SetMainColor(bgo_raw_toa_color);
        bgo_raw_hits_toa->SetMainAlpha(bgo_raw_toa_alpha * 0.01);
        bgo_raw_hits_toa->Reset(TEveBoxSet::kBT_AABox, kFALSE, 64);

        auto palette = TColor::GetPalette();

        for (int j = 0; j < n; ++j) {
            auto hit = cat->get_object<SiPMRaw>(j);

            auto board = hit->board;
            auto chan = hit->channel;
            auto toa = hit->toa;
            auto tot = hit->tot;

            std::print("[{:02d}]  board: {:d}  sipm: {:2d}  toa: {:6.2f}  tot: {:6.2f}\n", j, board, chan, toa, tot);

            if (board == 0) {
                auto node = SiPM_nodes[chan];
                auto translation = node->GetMatrix()->GetTranslation();

                sipm_raw_hits_toa->AddBox(translation[0] - SiPM_X / 8,
                                          translation[1] - SiPM_Y / 8,
                                          BGO_Z / 2,  // align SIPM times with BGO
                                          SiPM_X / 4,
                                          SiPM_Y / 4,
                                          toa * toa_scale * 0.001);

                SiPM_nodes[chan]->GetVolume()->SetLineColor(palette[0] + tot * palette.GetSize() / tot_range.second);
            } else {
                for (const auto id : BGO_clusters[chan]) {
                    auto node = BGO_nodes[id];
                    auto translation = node->GetMatrix()->GetTranslation();

                    bgo_raw_hits_toa->AddBox(translation[0] - BGO_X / 8,
                                             translation[1] - BGO_Y / 8,
                                             BGO_Z / 2,
                                             BGO_X / 4,
                                             BGO_Y / 4,
                                             toa * toa_scale * 0.001);

                    BGO_nodes[id]->GetVolume()->GetNode(0)->GetVolume()->SetLineColor(
                        palette[0] + tot * palette.GetSize() / tot_range.second);
                }
            }
        }

        gEve->AddElement(sipm_raw_hits_toa);
        gEve->AddElement(bgo_raw_hits_toa);
    }

    void sabat_read_sipm_cal()
    {
        auto cat = reader.model().get_category(SabatCategories::SiPMCal);
        auto n = cat->get_entries();

        auto sipm_raw_hits_toa = new TEveBoxSet("sipm_raw_hits_toa");
        sipm_raw_hits_toa->UseSingleColor();
        sipm_raw_hits_toa->SetMainColor(sipm_raw_toa_color);
        sipm_raw_hits_toa->SetMainAlpha(sipm_raw_toa_alpha * 0.01);
        sipm_raw_hits_toa->Reset(TEveBoxSet::kBT_AABox, kFALSE, 64);

        auto bgo_raw_hits_toa = new TEveBoxSet("bgo_raw_hits_toa");
        bgo_raw_hits_toa->UseSingleColor();
        bgo_raw_hits_toa->SetMainColor(bgo_raw_toa_color);
        bgo_raw_hits_toa->SetMainAlpha(bgo_raw_toa_alpha * 0.01);
        bgo_raw_hits_toa->Reset(TEveBoxSet::kBT_AABox, kFALSE, 64);

        auto palette = TColor::GetPalette();

        for (int j = 0; j < n; ++j) {
            auto hit = cat->get_object<SiPMCal>(j);

            auto board = hit->board;
            auto chan = hit->channel;
            auto toa = hit->toa;
            auto energy = hit->energy;

            std::print(
                "[{:02d}]  board: {:d}  sipm: {:2d}  toa: {:6.2f}  energy: {:6.2f}\n", j, board, chan, toa, energy);

            if (board == 0) {
                auto node = SiPM_nodes[chan];
                auto translation = node->GetMatrix()->GetTranslation();

                sipm_raw_hits_toa->AddBox(translation[0] - SiPM_X / 8,
                                          translation[1] - SiPM_Y / 8,
                                          BGO_Z / 2,  // align SIPM times with BGO
                                          SiPM_X / 4,
                                          SiPM_Y / 4,
                                          toa * toa_scale * 0.001);

                SiPM_nodes[chan]->GetVolume()->SetLineColor(palette[0] + energy * palette.GetSize() / tot_range.second);
            } else {
                for (const auto id : BGO_clusters[chan]) {
                    auto node = BGO_nodes[id];
                    auto translation = node->GetMatrix()->GetTranslation();

                    bgo_raw_hits_toa->AddBox(translation[0] - BGO_X / 8,
                                             translation[1] - BGO_Y / 8,
                                             BGO_Z / 2,
                                             BGO_X / 4,
                                             BGO_Y / 4,
                                             toa * toa_scale * 0.001);

                    BGO_nodes[id]->GetVolume()->GetNode(0)->GetVolume()->SetLineColor(
                        palette[0] + energy * palette.GetSize() / tot_range.second);
                }
            }
        }

        gEve->AddElement(sipm_raw_hits_toa);
        gEve->AddElement(bgo_raw_hits_toa);
    }

    /******************************************************************************/
    // GUI
    /******************************************************************************/
    void print_geom_tree(TGeoNode* node, int indent = 0)
    {
        if (indent == 0) {
            printf("%*c%s\n", indent, ' ', node->GetName());
        }

        TObjArray* objs = node->GetNodes();
        if (!objs) {
            return;
        }

        for (int i = 0; i < objs->GetEntries(); ++i) {
            TGeoNode* n = dynamic_cast<TGeoNode*>(objs->At(i));
            if (n) {
                printf("%*c[%d]  %s\n", indent + 4, ' ', i, n->GetName());
                print_geom_tree(n, indent + 4);
            }
        }
    }

    TGeoMaterial* matFiber {nullptr};

    auto Next() -> void
    {
        if (event_id == reader.get_entries() - 1) {
            std::print("Already at the last event: {:d} of {:d}.\n", event_id, reader.get_entries() - 1);
        } else {
            ++(event_id);
            load_event();
        }
    }

    auto Prev() -> void
    {
        if (event_id == 0) {
            std::print("Already at the first event: {:d} of {:d}.\n", event_id, reader.get_entries() - 1);
        } else {
            --(event_id);
            load_event();
        }
    }

    auto SelectInput(int button)
    {
        auto old = sipm_selected_input;
        switch (button) {
            case 0:
                sipm_selected_input = SiPM_input::RAW;
                if (palette) {
                    palette->GetAxis()->SetWmin(tot_range.first);
                    palette->GetAxis()->SetWmax(tot_range.second);
                }
                break;
            case 1:
                sipm_selected_input = SiPM_input::CAL;
                if (palette) {
                    palette->GetAxis()->SetWmin(energy_range.first);
                    palette->GetAxis()->SetWmax(energy_range.second);
                }
                break;
        }

        if (old != sipm_selected_input) {
            if (embedded_canvas and palette) {
                embedded_canvas->GetCanvas();
                palette->Draw();
                gPad->Modified();
                gPad->Update();
            }

            load_event();
        }
    }

    class QuickJumpNavHandler
    {
        event_viewer* evi {nullptr};

    public:
        QuickJumpNavHandler(event_viewer* eviewer, TGNumberEntry* widget)
            : evi {eviewer}
            , l_evt_jump(widget)
        {
        }

        void Load() { l_evt_jump->SetIntNumber(evi->event_id); }

        void Jump()
        {
            if (evi->event_id == l_evt_jump->GetIntNumber()) {
                std::print("Already at the event {:d} of {:d}.\n", evi->event_id, evi->reader.get_entries() - 1);
                return;
            }

            evi->event_id = l_evt_jump->GetIntNumber();
            if (evi->event_id >= 0 and evi->event_id < evi->reader.get_entries()) {
                evi->load_event();
            }
        }

    private:
        TGNumberEntry* l_evt_jump {nullptr};
    };

    class VisualHandler
    {
        event_viewer* evi {nullptr};
        TRootEmbeddedCanvas* ecanv {nullptr};

    public:
        VisualHandler(event_viewer* eviewer, TRootEmbeddedCanvas* embedded_canvas)
            : evi {eviewer}
            , ecanv {embedded_canvas}
        {
        }

        void PaletteChanged(Int_t n)
        {
            gStyle->SetPalette(n);
            if (ecanv) {
                auto can = ecanv->GetCanvas();
                if (can) {
                    can->Draw();
                    can->Modified();
                    can->Update();
                }
            }
            evi->load_event();
        }
    };

    class MaterialPropHandler
    {
        event_viewer* evi {nullptr};

    public:
        MaterialPropHandler(event_viewer* eviewer, TGeoMaterial* material)
            : evi {eviewer}
            , material(material)
        {
        }

        void PositionChanged(Int_t n)
        {
            if (!material) {
                return;
            }

            material->SetTransparency(n);
            if (alpha_label) {
                alpha_label->SetText(TString::Format("% 3d", n));
            }
            gEve->FullRedraw3D(kFALSE);
        }

        void SetAlphaLabel(TGLabel* l) { alpha_label = l; }

    private:
        TGeoMaterial* material;
        TGLabel* alpha_label;
    };

    /// This class controls the transaprency of the TGeoNode element
    class NodeAlphaHandler
    {
    private:
        event_viewer* evi {nullptr};
        std::vector<TGeoNode*>* nodes_array {nullptr};
        int* alpha {nullptr};
        TGLabel* label {nullptr};

    public:
        NodeAlphaHandler(event_viewer* eviewer, std::vector<TGeoNode*>* arr, int* _alpha, TGLabel* _label = nullptr)
            : evi {eviewer}
            , nodes_array {arr}
            , alpha {_alpha}
            , label {_label}
        {
        }

        void ChangeAlpha(Int_t n)
        {
            *alpha = n;
            auto make_transparent = [&](TGeoNode* node)
            {
                if (node->GetVolume()->GetNodes()) {
                    node->GetVolume()->GetNode(0)->GetVolume()->SetTransparency(*alpha);
                } else {
                    node->GetVolume()->SetTransparency(*alpha);
                }
            };
            std::for_each(nodes_array->begin(), nodes_array->end(), make_transparent);
            gEve->FullRedraw3D(kFALSE);

            if (label) {
                label->SetText(TString::Format("% 3d", n));
            }
        }
    };

    class BoxColorHandler
    {
    private:
        event_viewer* evi {nullptr};
        int* color;

    public:
        BoxColorHandler(event_viewer* eviewer, int* _color)
            : evi {eviewer}
            , color {_color}
        {
        }

        void SelectColor(Pixel_t c)
        {
            *color = TColor::GetColor(c);
            evi->load_event();
        }
    };

    class BoxAlphaHandler
    {
    private:
        event_viewer* evi {nullptr};
        int* alpha;
        TGLabel* label;

    public:
        BoxAlphaHandler(event_viewer* eviewer, int* _alpha, TGLabel* _label = nullptr)
            : evi {eviewer}
            , alpha {_alpha}
            , label {_label}
        {
        }

        void ChangeAlpha(Int_t n)
        {
            *alpha = n;
            if (label) {
                label->SetText(TString::Format("% 3d", n));
            }
            evi->load_event();
        }
    };

    class BoxScaleHandler
    {
    private:
        event_viewer* evi {nullptr};
        int* scale;
        TGLabel* label;

    public:
        BoxScaleHandler(event_viewer* eviewer, int* _scale, TGLabel* _label = nullptr)
            : evi {eviewer}
            , scale(_scale)
            , label(_label)
        {
        }

        void ChangeScale(Int_t n)
        {
            *scale = n;
            if (label) {
                label->SetText(TString::Format("% 3d", n));
            }
            evi->load_event();
        }
    };

    ClassDef(event_viewer, 0);
};

void event_display(const char* file = nullptr,
                   const char* ascii_params = "sabat_pars.txt",
                   const char* sabat_geometry = "sabat.gdml")
{
    spdlog::info("Starting event_display...");

    new event_viewer(file, ascii_params, sabat_geometry);
}

#if !defined(__ROOTCLING__)

#    include <TApplication.h>

#    include <CLI/CLI.hpp>
#    include <fmt/core.h>

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

    std::string geom_file {"sabat.gdml"};
    app.add_option("-g,--geom", geom_file, "geometry file")->check(CLI::ExistingFile);

    std::string input_file {};
    app.add_option("input_file", input_file, "file to process")->check(CLI::ExistingFile);

    bool debug_mode {false};
    app.add_flag("-d", debug_mode, "debug mode");

    CLI11_PARSE(app, argc, argv);

    if (debug_mode) {
        spdlog::set_level(spdlog::level::debug);
    }

    std::print("Args:: input file: {:s}  ascii pars: {:s}  geom file: {:s}\n", input_file, ascii_par, geom_file);

    TApplication rapp("app", &argc, argv);

    event_display(input_file.c_str(), ascii_par.c_str(), geom_file.c_str());

    rapp.Run();
}

#endif /* !defined(__ROOTCLING__) */

// HB Core demo: renders an HTML page with the CPU renderer and writes a PNG.
// Usage: hb_demo_render [input.html|file-url] [output.png] [width] [height]
#include <hbcore/hb_core.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

static const char* kHello =
    "<!DOCTYPE html>\n<html>\n<head>\n    <title>HB Core Test</title>\n</head>\n<body>\n"
    "    <h1>Hello HB Core</h1>\n    <p>Independent browser engine.</p>\n</body>\n</html>\n";

int main(int argc, char** argv) {
    std::string input = argc > 1 ? argv[1] : "";
    std::string output = argc > 2 ? argv[2] : "hb_core_demo.png";
    int width = argc > 3 ? std::atoi(argv[3]) : 640;
    int height = argc > 4 ? std::atoi(argv[4]) : 240;

    HB::Core core;
    HB::CoreConfig config;
    config.logLevel = 3;
    if (!core.initialize(config).ok()) { std::fprintf(stderr, "initialize failed\n"); return 1; }

    HB::ViewConfig vc;
    vc.width = width;
    vc.height = height;
    HB::View view(core, vc);
    view.setEventHandler([](const HB::ViewEvent& e) {
        if (e.type == HB::ViewEventType::TitleChanged) std::printf("title: %s\n", e.text.c_str());
        if (e.type == HB::ViewEventType::NavigationFailed) std::printf("navigation failed: %s\n", e.text.c_str());
    });

    HB::Error err;
    if (input.empty()) err = view.loadHTML(kHello);
    else if (input.find("://") != std::string::npos) err = view.loadURL(input);
    else {
        std::string abs = std::filesystem::absolute(input).generic_string();
        err = view.loadURL("file://" + std::string(abs.empty() || abs[0] != '/' ? "/" : "") + abs);  // C:/x -> file:///C:/x
    }
    if (!err.ok()) { std::fprintf(stderr, "load failed: %s\n", err.message.c_str()); return 2; }

    err = view.savePNG(output);
    if (!err.ok()) { std::fprintf(stderr, "save failed: %s\n", err.message.c_str()); return 3; }
    std::printf("HB Core %s rendered \"%s\" (%d x %d, document height %.0f px) -> %s\n",
                HB::Core::version(), view.title().c_str(), width, height, view.documentHeight(), output.c_str());
    return 0;
}

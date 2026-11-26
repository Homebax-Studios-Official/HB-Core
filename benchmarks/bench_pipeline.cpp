// Measures the 0.1.0 pipeline on a generated document. Numbers are machine-specific; compare only on one machine.
#include "engine/page.hpp"
#include "html/parser.hpp"
#include <chrono>
#include <cstdio>
#include <string>

using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }

int main() {
    std::string html = "<!DOCTYPE html><html><head><title>bench</title><style>p{margin:4px 0} .a{color:#336699} li{padding:1px}</style></head><body>";
    for (int i = 0; i < 2000; ++i) {
        html += "<h2>Heading " + std::to_string(i) + "</h2><p class=\"a\">The quick brown fox jumps over the lazy dog. <b>Bold</b> and <i>italic</i> words keep the line breaker busy.</p><ul><li>one</li><li>two</li></ul>";
    }
    html += "</body></html>";

    auto t0 = Clock::now();
    auto doc = HB::parseHtml(html);
    auto t1 = Clock::now();
    HB::Page page;
    page.setViewport(1024, 768, 1.0f);
    page.setSource(html, {});
    auto t2 = Clock::now();
    float h = page.documentHeight();
    auto t3 = Clock::now();
    page.render();
    auto t4 = Clock::now();

    std::printf("input: %.1f KiB\n", html.size() / 1024.0);
    std::printf("parse only      : %8.2f ms\n", ms(t0, t1));
    std::printf("parse + sheets  : %8.2f ms\n", ms(t1, t2));
    std::printf("style + layout  : %8.2f ms (document height %.0f px)\n", ms(t2, t3), h);
    std::printf("paint + raster  : %8.2f ms (1024x768 viewport)\n", ms(t3, t4));
    return 0;
}

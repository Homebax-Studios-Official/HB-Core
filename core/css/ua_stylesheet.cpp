// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
// Default (user-agent) stylesheet. Written for HB Core; values follow common browser defaults.
#include "css/style.hpp"

namespace HB {

static const char* kUserAgentCss = R"CSS(
html, body, div, p, h1, h2, h3, h4, h5, h6, ul, ol, dl, dt, dd, pre, blockquote, hr, form, fieldset,
header, footer, section, article, nav, main, aside, figure, figcaption, address, details, summary,
center, table, caption, thead, tbody, tfoot, tr, menu { display: block; }
li { display: list-item; }
head, title, meta, link, style, script, base, template, noscript, area, param, source, track { display: none; }
body { margin: 8px; }
h1 { font-size: 2em; margin: 0.67em 0; font-weight: bold; }
h2 { font-size: 1.5em; margin: 0.83em 0; font-weight: bold; }
h3 { font-size: 1.17em; margin: 1em 0; font-weight: bold; }
h4 { margin: 1.33em 0; font-weight: bold; }
h5 { font-size: 0.83em; margin: 1.67em 0; font-weight: bold; }
h6 { font-size: 0.67em; margin: 2.33em 0; font-weight: bold; }
p, dl, pre, blockquote, figure, menu { margin: 1em 0; }
ul, ol { margin: 1em 0; padding-left: 40px; }
ul ul, ul ol, ol ul, ol ol { margin: 0; }
ul { list-style-type: disc; }
ol { list-style-type: decimal; }
dd { margin-left: 40px; }
blockquote { margin: 1em 40px; }
pre { white-space: pre; }
code, kbd, samp, tt, pre { font-family: monospace; }
img { display: inline; }
b, strong, th { font-weight: bold; }
i, em, cite, var, dfn, address { font-style: italic; }
u, ins { text-decoration: underline; }
s, strike, del { text-decoration: line-through; }
small { font-size: 0.83em; }
big { font-size: 1.17em; }
a { color: #0000ee; text-decoration: underline; }
mark { background-color: #ffff00; color: #000000; }
hr { margin: 0.5em 0; border-top-width: 1px; border-top-style: solid; border-top-color: #808080; height: 0; }
center { text-align: center; }
th { text-align: center; }
)CSS";

const Stylesheet& userAgentStylesheet() {
    static const Stylesheet sheet = parseStylesheet(kUserAgentCss);
    return sheet;
}

} // namespace HB

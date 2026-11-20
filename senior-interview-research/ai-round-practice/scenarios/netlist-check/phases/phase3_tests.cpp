// Phase 3 tests: production boards. Each test writes its netlist and layout to
// data/generated/ (open them: their shapes differ), parses them, and then times
// load_board plus check_board. Parsing is not timed.
#include <string>
#include <utility>
#include <vector>

#include "check.hpp"
#include "connectivity.hpp"
#include "layout.hpp"
#include "netlist.hpp"

namespace {

std::uint64_t result_hash(const CheckResult& r) {
    std::string text;
    for (const Open& o : r.opens) {
        text += "O " + o.net + ":";
        for (const auto& island : o.islands) {
            for (const std::string& p : island) text += p + ",";
            text += "|";
        }
        text += "\n";
    }
    for (const Short& s : r.shorts) {
        text += "S ";
        for (const std::string& net : s.nets) text += net + ",";
        text += "\n";
    }
    return check::fnv1a(text);
}

std::string pin(const std::string& part, int p) { return part + "." + std::to_string(p); }

// An LED panel: 160,000 addressable LEDs in one serpentine strip. Pin 1 VDD, 2 DOUT, 3 GND,
// 4 DIN. Power and ground are routed pin to pin along the strip, and each LED's DOUT feeds
// the next LED's DIN.
std::pair<std::string, std::string> led_panel() {
    check::Rng rng(701);
    const int leds = 160000;
    auto d = [](int k) { return "D" + std::to_string(k); };
    std::string net = "part J1 pins=4 PANEL-IN\n";
    for (int k = 1; k <= leds; ++k) net += "part " + d(k) + " pins=4 WS2812\n";
    std::string vdd = "net VDD J1.1", gnd = "net GND J1.3";
    for (int k = 1; k <= leds; ++k) {
        vdd += " " + pin(d(k), 1);
        gnd += " " + pin(d(k), 3);
    }
    net += vdd + "\n" + gnd + "\nnet DIN J1.2 D1.4\n";
    for (int k = 1; k < leds; ++k) net += "net DATA" + std::to_string(k) + " " + pin(d(k), 2) + " " + pin(d(k + 1), 4) + "\n";

    std::string lay = "trace J1.1 D1.1\ntrace J1.3 D1.3\ntrace J1.2 D1.4\n";
    for (int k = 1; k < leds; ++k) {
        lay += "trace " + pin(d(k), 1) + " " + pin(d(k + 1), 1) + "\n";
        if (k != 80000) lay += "trace " + pin(d(k), 3) + " " + pin(d(k + 1), 3) + "\n";  // a cracked ground trace
        if (rng.range(0, 39999) != 0) lay += "trace " + pin(d(k), 2) + " " + pin(d(k + 1), 4) + "\n";
    }
    for (int i = 0; i < 6; ++i) {  // solder bridges between neighbouring pads
        int k = (int)rng.range(1, leds);
        int p = (int)rng.range(1, 3);
        lay += "trace " + pin(d(k), p) + " " + pin(d(k), p + 1) + "\n";
    }
    return {net, lay};
}

// A dense digital board: 20,000 parts, 60,000 nets routed through 60,000 vias,
// with a few hundred missing traces and solder bridges.
std::pair<std::string, std::string> dense_board() {
    check::Rng rng(702);
    std::string net, lay;
    std::vector<std::string> pins;
    for (int i = 1; i <= 20000; ++i) {
        long long kind = rng.range(0, 9);
        std::string des = (kind < 4 ? "R" : kind < 7 ? "C" : kind < 9 ? "U" : "J") + std::to_string(i);
        int count = kind < 7 ? 2 : kind < 9 ? (int)rng.range(16, 144) : (int)rng.range(10, 40);
        net += "part " + des + " pins=" + std::to_string(count) + "\n";
        for (int p = 1; p <= count; ++p) pins.push_back(pin(des, p));
    }
    for (std::size_t i = pins.size() - 1; i > 0; --i) std::swap(pins[i], pins[rng.range(0, (long long)i)]);
    std::size_t next = 0;
    int vias = 0;
    for (int k = 0; k < 60000; ++k) {
        std::size_t size = k < 2 ? 4000 : (std::size_t)rng.range(2, 10);
        if (next + size > pins.size()) break;
        net += "net " + (k == 0 ? std::string("GND") : k == 1 ? std::string("VCC") : "SIG" + std::to_string(k));
        for (std::size_t j = 0; j < size; ++j) net += " " + pins[next + j];
        net += "\n";
        for (std::size_t j = 1; j < size; ++j) {
            if (rng.range(0, 499) == 0) continue;  // a missing trace
            const std::string& a = pins[next + j];
            const std::string& b = pins[next + rng.range(0, (long long)j - 1)];
            if (rng.range(0, 1) == 0) {
                std::string via = "V" + std::to_string(++vias);
                lay += "trace " + a + " " + via + "\ntrace " + via + " " + b + "\n";
            } else {
                lay += "trace " + a + " " + b + "\n";
            }
        }
        next += size;
    }
    for (int k = 0; k < 400; ++k) {
        const std::string& a = pins[rng.range(0, (long long)pins.size() - 1)];
        const std::string& b = pins[rng.range(0, (long long)pins.size() - 1)];
        if (a != b) lay += "trace " + a + " " + b + "\n";
    }
    return {net, lay};
}

// A board with a ground plane: a 600 x 400 grid of stitching vias, named by the layout tool,
// joined to their neighbours. A slot cuts the plane in two.
std::pair<std::string, std::string> ground_plane() {
    check::Rng rng(703);
    const int W = 600, H = 400;
    auto via = [&](int x, int y) {
        std::string s = std::to_string(y * W + x);
        return "GND_VIA_" + std::string(6 - s.size(), '0') + s;
    };
    std::string net, lay;
    std::vector<std::string> gnd_pins, other;
    for (int i = 1; i <= 3000; ++i) {
        bool ic = rng.range(0, 2) == 0;
        std::string des = (ic ? "U" : "C") + std::to_string(i);
        int count = ic ? (int)rng.range(8, 64) : 2;
        net += "part " + des + " pins=" + std::to_string(count) + "\n";
        for (int p = 1; p <= count; ++p) (p % 4 == 2 ? gnd_pins : other).push_back(pin(des, p));
    }
    net += "net GND";
    for (const std::string& p : gnd_pins) net += " " + p;
    net += "\n";
    for (std::size_t k = 0; k + 1 < other.size(); k += 2) {
        net += "net S" + std::to_string(k / 2 + 1) + " " + other[k] + " " + other[k + 1] + "\n";
        lay += "trace " + other[k] + " " + other[k + 1] + "\n";
    }
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            if (x + 1 < W && x != 299) lay += "trace " + via(x, y) + " " + via(x + 1, y) + "\n";
            if (y + 1 < H) lay += "trace " + via(x, y) + " " + via(x, y + 1) + "\n";
        }
    auto random_via = [&] {
        int x = (int)rng.range(0, W - 1);
        int y = (int)rng.range(0, H - 1);
        return via(x, y);
    };
    for (const std::string& p : gnd_pins)
        if (rng.range(0, 999) != 0) lay += "trace " + p + " " + random_via() + "\n";
    for (int k = 0; k < 20; ++k) {  // stray copper from signal pins to the plane
        const std::string& p = other[rng.range(0, (long long)other.size() - 1)];
        lay += "trace " + p + " " + random_via() + "\n";
    }
    return {net, lay};
}

void run_large(const std::string& name, const std::pair<std::string, std::string>& board, double limit_ms,
               std::size_t opens, std::size_t shorts, std::uint64_t hash) {
    Netlist n = parse_netlist(check::read_file(check::write_generated(name + ".net", board.first)));
    Layout layout = parse_layout(check::read_file(check::write_generated(name + ".layout", board.second)));
    CheckResult r;
    check::measure(limit_ms, [&] {
        Board b = load_board(n, layout);
        r = check_board(n, b);
    });
    EXPECT_EQ(r.opens.size(), opens);
    EXPECT_EQ(r.shorts.size(), shorts);
    EXPECT_EQ(result_hash(r), hash);
}

}  // namespace

TEST("large-1 LED panel: one long serpentine strip", 3) { run_large("large-1-led-panel", led_panel(), 1500, 4, 2, 17766195132495264427ULL); }

TEST("large-2 dense digital board: 60,000 nets", 3) { run_large("large-2-dense", dense_board(), 1500, 582, 329, 15503151672582297338ULL); }

TEST("large-3 ground plane stitched with 240,000 vias", 3) { run_large("large-3-ground-plane", ground_plane(), 1500, 1, 2, 17522752046235130171ULL); }

// Phase 2 tests: check_board.
#include <string>
#include <utility>
#include <vector>

#include "check.hpp"
#include "connectivity.hpp"
#include "layout.hpp"
#include "netlist.hpp"

namespace {

CheckResult run(const std::string& netlist, const std::string& layout) {
    Netlist n = parse_netlist(netlist);
    Board b = load_board(n, parse_layout(layout));
    return check_board(n, b);
}

// "OPEN GND [C1.2 C2.2] [LED1.2]" and "SHORT A B", one per entry.
std::vector<std::string> brief(const CheckResult& r) {
    std::vector<std::string> out;
    for (const Open& o : r.opens) {
        std::string s = "OPEN " + o.net;
        for (const auto& island : o.islands) {
            s += " [";
            for (std::size_t i = 0; i < island.size(); ++i) s += (i ? " " : "") + island[i];
            s += "]";
        }
        out.push_back(s);
    }
    for (const Short& sh : r.shorts) {
        std::string s = "SHORT";
        for (const std::string& net : sh.nets) s += " " + net;
        out.push_back(s);
    }
    return out;
}

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

// A random board: nets are routed as trees, some traces are missing, some pins are bridged.
std::pair<std::string, std::string> random_board(int parts, int nets, std::uint64_t seed) {
    check::Rng rng(seed);
    const char* kinds = "RCUDQJ";
    std::string netlist, layout;
    std::vector<std::string> pins;
    for (int i = 0; i < parts; ++i) {
        char kind = kinds[rng.range(0, 5)];
        int count = kind == 'U' ? (int)rng.range(8, 48) : kind == 'J' ? (int)rng.range(2, 10) : kind == 'Q' ? 3 : 2;
        std::string des = std::string(1, kind) + std::to_string(i + 1);
        netlist += "part " + des + " pins=" + std::to_string(count) + "\n";
        for (int p = 1; p <= count; ++p) pins.push_back(des + "." + std::to_string(p));
    }
    for (std::size_t i = pins.size() - 1; i > 0; --i) std::swap(pins[i], pins[rng.range(0, (long long)i)]);
    std::size_t next = 0;
    int vias = 0;
    for (int k = 0; k < nets && next < pins.size(); ++k) {
        std::size_t size = std::min<std::size_t>((std::size_t)rng.range(1, 6), pins.size() - next);
        std::string name = k == 0 ? "GND" : k == 1 ? "VCC" : "N" + std::to_string(k);
        netlist += "net " + name;
        for (std::size_t j = 0; j < size; ++j) netlist += " " + pins[next + j];
        netlist += "\n";
        for (std::size_t j = 1; j < size; ++j) {
            if (rng.range(0, 11) == 0) continue;  // a missing trace
            const std::string& a = pins[next + j];
            const std::string& b = pins[next + rng.range(0, (long long)j - 1)];
            if (rng.range(0, 3) == 0) {
                std::string via = "V" + std::to_string(++vias);
                layout += "trace " + a + " " + via + "\ntrace " + via + " " + b + "\n";
            } else {
                layout += "trace " + a + " " + b + "\n";
            }
        }
        next += size;
    }
    for (int k = 0; k < nets / 12; ++k) {  // solder bridges, some through unused pins
        const std::string& a = pins[rng.range(0, (long long)pins.size() - 1)];
        const std::string& b = pins[rng.range(0, (long long)pins.size() - 1)];
        if (a != b) layout += "trace " + a + " " + b + "\n";
    }
    return {netlist, layout};
}

}  // namespace

TEST("the sensor board has one open and one short", 2) {
    CheckResult r = run(check::read_file("data/sensor-board.net"), check::read_file("data/sensor-board.layout"));
    EXPECT_EQ(brief(r), (std::vector<std::string>{"OPEN GND [C1.2 C2.2 J1.4 U1.8 U11.4] [LED1.2]", "SHORT UART_RX UART_TX"}));
}

TEST("a chain of traces and vias is one connection", 2) {
    CheckResult r = run("part R1 pins=2\npart R2 pins=2\nnet A R1.1 R2.2\n",
                        "trace R1.1 V1\ntrace V1 V2\ntrace V3 V2\ntrace V3 R2.2\n");
    EXPECT_EQ(brief(r), std::vector<std::string>{});
}

TEST("islands and opens are in natural order", 2) {
    CheckResult r = run(
        "part U2 pins=12\npart U10 pins=4\n"
        "net LED10 U2.10 U2.9\n"
        "net LED2 U10.1 U2.11 U2.2 U10.2 U2.1\n",
        "trace U2.11 U10.2\ntrace U2.2 U2.1\n");
    EXPECT_EQ(brief(r), (std::vector<std::string>{"OPEN LED2 [U2.1 U2.2] [U2.11 U10.2] [U10.1]",
                                                  "OPEN LED10 [U2.9] [U2.10]"}));
}

TEST("nets bridged together form one short, reported once", 2) {
    CheckResult r = run(
        "part R1 pins=2\npart R2 pins=2\npart R3 pins=2\npart R4 pins=2\npart R5 pins=2\n"
        "net C R1.1 R1.2\nnet B R2.1 R2.2\nnet A R3.1 R3.2\n"
        "net Q R4.1 R5.1\nnet P R4.2 R5.2\n",
        "trace R1.1 R1.2\ntrace R2.1 R2.2\ntrace R3.1 R3.2\n"
        "trace R1.2 R2.1\ntrace R2.2 R3.1\n"   // C to B, B to A
        "trace R4.1 R4.2\ntrace R5.1 R5.2\n");  // Q to P, twice
    EXPECT_EQ(brief(r), (std::vector<std::string>{"OPEN P [R4.2] [R5.2]", "OPEN Q [R4.1] [R5.1]", "SHORT A B C",
                                                  "SHORT P Q"}));
}

TEST("pins on no net do not cause shorts", 2) {
    CheckResult r = run("part R1 pins=2\npart R2 pins=2\npart U1 pins=8\nnet A R1.1 R1.2\nnet B R2.1\n",
                        "trace R1.1 U1.5\ntrace U1.5 R1.2\ntrace U1.6 U1.7\ntrace U1.8 R2.1\n");
    EXPECT_EQ(brief(r), std::vector<std::string>{});
}

TEST("single-pin nets are never open; an empty layout opens the rest", 2) {
    CheckResult r = run("part J1 pins=3\npart TP1 pins=1\nnet TP TP1.1\nnet SIG J1.1 J1.3 J1.2\n", "");
    EXPECT_EQ(brief(r), (std::vector<std::string>{"OPEN SIG [J1.1] [J1.2] [J1.3]"}));
}

TEST("medium random boards", 2) {
    struct Case { int parts; int nets; std::uint64_t seed; std::size_t opens; std::size_t shorts; std::uint64_t hash; };
    const Case cases[] = {
        {60, 80, 71, 12, 1, 9397528175051752519ULL},
        {800, 1500, 72, 264, 89, 4831240997066824418ULL},
    };
    for (const Case& c : cases) {
        auto [netlist, layout] = random_board(c.parts, c.nets, c.seed);
        CheckResult r = run(netlist, layout);
        EXPECT_EQ(r.opens.size(), c.opens);
        EXPECT_EQ(r.shorts.size(), c.shorts);
        EXPECT_EQ(result_hash(r), c.hash);
    }
}

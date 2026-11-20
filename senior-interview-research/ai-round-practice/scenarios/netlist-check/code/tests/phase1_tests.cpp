// Existing tests for netcheck. Do not edit them to make them pass.
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "check.hpp"
#include "layout.hpp"
#include "netlist.hpp"
#include "pinref.hpp"
#include "pinreport.hpp"

namespace {

template <class E, class F>
std::string error_of(F&& f) {
    try {
        f();
    } catch (const E& e) {
        return e.what();
    }
    return "(no error)";
}

Netlist sensor_board() { return parse_netlist(check::read_file("data/sensor-board.net")); }

}  // namespace

TEST("parse_pin reads the designator and the pin number", 1) {
    PinRef p = parse_pin("U1.12");
    EXPECT_EQ(p.part, std::string("U1"));
    EXPECT_EQ(p.pin, 12);
    EXPECT_TRUE(parse_pin("LED3.2") == (PinRef{"LED3", 2}));
    EXPECT_EQ(format_pin({"TP1", 1}), std::string("TP1.1"));
    for (const char* bad : {"U1", "U1.", "U1.0", "u1.2", "U.2", "1.2", "U1.x", "U1.12345", "U1.02"}) {
        bool threw = false;
        try { parse_pin(bad); } catch (const PinError&) { threw = true; }
        if (!threw) check::fail(__FILE__, __LINE__, std::string("accepted bad pin \"") + bad + "\"");
    }
}

TEST("natural order of designators and pins", 1) {
    std::vector<std::string> v{"R10", "U1.10", "C9", "R2", "U1.9", "LED1", "R1"};
    std::sort(v.begin(), v.end(), [](const std::string& a, const std::string& b) { return natural_less(a, b); });
    EXPECT_EQ(v, (std::vector<std::string>{"C9", "LED1", "R1", "R2", "R10", "U1.9", "U1.10"}));
    EXPECT_TRUE(natural_less("U1.12", "U11.2"));
    EXPECT_TRUE(!natural_less("U11.2", "U1.12"));
    EXPECT_TRUE(!natural_less("R7", "R7"));
}

TEST("parse_netlist reads parts and nets", 1) {
    Netlist n = sensor_board();
    EXPECT_EQ(n.parts.size(), std::size_t(10));
    EXPECT_EQ(n.nets.size(), std::size_t(11));
    EXPECT_EQ(n.parts[0].value, std::string("MCU"));
    EXPECT_EQ(n.parts[1].pins, 8);
    EXPECT_EQ(n.nets[5].name, std::string("SDA"));
    EXPECT_TRUE(n.nets[5].pins.size() == 3 && n.nets[5].pins[1] == (PinRef{"U11", 5}));
    EXPECT_EQ(n.find_part("LED1"), 8);
    EXPECT_EQ(n.find_part("X1"), -1);
}

TEST("parse_netlist reports the failing line", 1) {
    auto err = [](const char* text) { return error_of<NetlistError>([&] { parse_netlist(text); }); };
    EXPECT_EQ(err("part U1 pins=4\npart U1 pins=8\n"), std::string("line 2: part U1 is declared twice"));
    EXPECT_EQ(err("part U1 pins=4\nnet A U1.5\n"), std::string("line 2: U1 has 4 pins, not 5"));
    EXPECT_EQ(err("part U1 pins=4\nnet A U2.1\n"), std::string("line 2: unknown part 'U2'"));
    EXPECT_EQ(err("part U1 pins=4\nnet A U1.1\nnet B U1.2 U1.1\n"), std::string("line 3: pin U1.1 is already on net A"));
    EXPECT_EQ(err("part U1 pins=4\nnet A U1.1 U1.1\n"), std::string("line 2: pin U1.1 is already on net A"));
    EXPECT_EQ(err("part U1 pins=0\n"), std::string("line 1: expected pins=<n>, got 'pins=0'"));
    EXPECT_EQ(err("# rev A\nwire U1.1 U1.2\n"), std::string("line 2: expected 'part' or 'net', got 'wire'"));
}

TEST("load_board turns pins and vias into nodes", 1) {
    Netlist n = parse_netlist("part R1 pins=2\npart R2 pins=2\npart U1 pins=4\n");
    Board b = load_board(n, parse_layout("trace R1.2 V1\ntrace V1 U1.3\ntrace R2.1 V2   # to the pad\ntrace V1 V2\n"));
    EXPECT_EQ(b.pin_nodes, 8);
    EXPECT_EQ(b.vias, (std::vector<std::string>{"V1", "V2"}));
    EXPECT_EQ(b.edges, (std::vector<std::pair<int, int>>{{1, 8}, {8, 6}, {2, 9}, {8, 9}}));
    EXPECT_EQ(node_name(n, b, 0), std::string("R1.1"));
    EXPECT_EQ(node_name(n, b, 6), std::string("U1.3"));
    EXPECT_EQ(node_name(n, b, 9), std::string("V2"));
}

TEST("load_board reports bad traces", 1) {
    Netlist n = parse_netlist("part R1 pins=2\n");
    auto err = [&](const char* text) { return error_of<LayoutError>([&] { load_board(n, parse_layout(text)); }); };
    EXPECT_EQ(err("trace R9.1 V1\n"), std::string("line 1: unknown part 'R9'"));
    EXPECT_EQ(err("trace R1.3 V1\n"), std::string("line 1: R1 has no pin 3"));
    EXPECT_EQ(err("\ntrace V1 V1\n"), std::string("line 2: trace from V1 to itself"));
    EXPECT_EQ(err("trace R1.1\n"), std::string("line 1: expected: trace <node> <node>"));
    EXPECT_EQ(err("trace R1.x V1\n"), std::string("line 1: bad pin number in 'R1.x'"));
    EXPECT_EQ(err("trace R1.1 V$\n"), std::string("line 1: bad via name 'V$'"));
}

TEST("unconnected pins of the EEPROM", 1) {
    Netlist n = sensor_board();
    EXPECT_EQ(unconnected_pins(n, "U11"), (std::vector<int>{1, 2, 3, 7}));
    EXPECT_EQ(unconnected_pins(n, "LED1"), std::vector<int>{});
    EXPECT_EQ(error_of<NetlistError>([&] { unconnected_pins(n, "U99"); }), std::string("unknown part 'U99'"));
}

TEST("pin report for the sensor board", 1) {
    EXPECT_EQ(pin_report(sensor_board()), std::string(
        "Parts: 10, nets: 11, pins on nets: 31 of 57\n"
        "  C1     2 pins, all on nets\n"
        "  C2     2 pins, all on nets\n"
        "  J1     4 pins, all on nets\n"
        "  LED1   2 pins, all on nets\n"
        "  R1     2 pins, all on nets\n"
        "  R2     2 pins, all on nets\n"
        "  R10    2 pins, all on nets\n"
        "  TP1    1 pin, all on nets\n"
        "  U1    32 pins, 22 not on any net: 2-3, 5-7, 9, 14-19, 21-29, 32\n"
        "  U11    8 pins, 4 not on any net: 1-3, 7\n"
        "Single-pin nets: SWO\n"));
}

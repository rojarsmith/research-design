# netcheck

`netcheck` is the hardware team's board review tool. Today it reads the schematic netlist and
prints a pin report, so that floating pins are caught before a board is ordered. Next it needs
to compare the schematic with the copper from the layout tool and find opens and shorts.

## Netlist (from the schematic)

```
# part <designator> pins=<n> [value]
part U1   pins=32 MCU
part R1   pins=2  4k7
# net <name> <pin> <pin> ...
net SDA   U1.12 R1.2
```

- A designator is capital letters followed by digits: `R1`, `U12`, `LED3`, `TP2`.
- A pin is written `<designator>.<number>`; pins are numbered from 1 up to the part's pin count.
- A net lists at least one pin. A pin may be on at most one net. Pins on no net are allowed (unused pins).

## Layout (from the layout tool)

```
trace U1.12 V7
trace V7    R1.2
```

- `trace <node> <node>`: a piece of copper that connects two nodes.
- A node is a pin (`U1.12`) or a via. A via is any name without a `.` (`V7`, `TV3`, `GND_VIA_12`);
  vias exist only in the layout.

## Layout of the code

| Path | What it does |
| --- | --- |
| `src/pinref.*` | Parse and format pin references, natural order of designators |
| `src/netlist.*` | Parse the netlist, check parts, pins and nets |
| `src/pinreport.*` | Pin report for the schematic review |
| `src/layout.*` | Parse the layout and turn it into a graph of nodes (`Board`) |
| `src/connectivity.*` | Compare the schematic with the board (not implemented yet) |
| `app/main.cpp` | `netcheck report <netlist>` and `netcheck check <netlist> <layout>` |
| `tests/` | Tests. Run them with the drill tool or the VS Code test task. |
| `data/` | The sensor board: netlist and layout |

## Conventions

- Reports list parts, nets and pins in natural order (`natural_less`): `R2` before `R10`, `U1.9` before `U1.10`.
- Errors throw `NetlistError` or `LayoutError` with a message that starts with `line N:`.

## Phase 2: find opens and shorts (about 25 minutes)

Implement `check_board(netlist, board)` in `src/connectivity.cpp`. The tests are in `tests/phase2_tests.cpp`.

**Rules**

1. Two nodes are *connected* if a chain of traces joins them. Vias are nodes like any other.
2. A net is *open* if its pins are not all connected to each other. Report it as an `Open` with its *islands*:
   the net's pins split into groups that are connected to each other. List the pins of an island in natural
   order (`format_pin`, compared with `natural_less`), and the islands by their first pin.
3. A net with one pin is never open.
4. Every group of connected nodes that contains pins of two or more nets is a *short*. Report it as a `Short`
   with those nets in natural order. If several groups touch exactly the same nets, report that set once.
5. Pins that are on no net belong to no net: copper may connect them to anything without causing a short.
6. `opens` are in natural order of net name. `shorts` are in natural order of their first net, then their second, and so on.

**Before you prompt the AI**, say out loud how you will find which nodes are connected, how you will turn that
into opens and shorts, and the time complexity you expect. Then use the AI for the implementation, and read
every line it gives you before you run it.

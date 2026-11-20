# teammatch

`teammatch` runs the internship program's team matching. Interns list the teams they want, in
order; each team interviews some of its applicants and its panel gives marks. Today the tool
reads the roster and prints a demand report for the program managers. Next it needs to place
interns on teams.

## Roster

```
team payments cap=2
team search   cap=1
intern ava  applied=2026-09-01T09:30  prefs=payments,search
intern ben  applied=2026-09-03T14:10  prefs=payments
marks payments ava 4.1,4.3
marks payments ben 3.6,4.8
```

- `team <id> cap=<n>`: how many interns the team can take. `cap=0` is allowed (a hiring freeze).
- `intern <id> applied=<time> prefs=<team>,<team>,...`: teams in order of preference, most wanted first,
  each at most once. `prefs=-` means none. `applied` is when the application arrived, `YYYY-MM-DDTHH:MM`.
- `marks <team> <intern> <mark>,<mark>,...`: one mark per interviewer, 1.0 to 5.0 with one decimal.
  A team gives marks only to interns who listed it, and at most one set per intern.
- Ids use lowercase letters, digits, `-` and `_`. Teams are declared before interns list them, and
  interns before their marks.

## How a team ranks its applicants

A team ranks the interns it interviewed: higher average mark first; equal averages by the earlier
application; then by intern id. Averages are compared exactly; the two decimals in reports are only
for printing.

## Layout

| Path | What it does |
| --- | --- |
| `src/marks.*` | Parse and average interview marks |
| `src/roster.*` | Parse the roster: teams, interns, marks |
| `src/ranking.*` | How a team ranks its applicants |
| `src/demand.*` | Demand report for the program managers |
| `src/matcher.*` | Place interns on teams (not implemented yet) |
| `app/main.cpp` | `teammatch report <roster>` and `teammatch match <roster>` |
| `tests/` | Tests. Run them with the drill tool or the VS Code test task. |
| `data/` | The spring cohort |

## Conventions

- Team and intern indices follow the order of lines in the roster.
- Errors in the roster throw `RosterError("line N: ...")`.

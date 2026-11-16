## Phase 2: place the interns (about 25 minutes)

Implement `match_interns(roster)` in `src/matcher.cpp`. The tests are in `tests/phase2_tests.cpp`.

**Rules**

1. A team ranks the interns it interviewed, in the order of `rank_applicants`. It never takes an intern it
   did not interview, and a team with `cap=0` takes nobody.
2. While some intern is unplaced and still has teams on their list they have not applied to, that intern
   applies to the next team on their list.
3. The team then holds the best interns it can fit, up to its capacity, among the interns it already holds
   plus the new applicant, and turns the rest away. An intern who is turned away, including one the team
   held until now, is unplaced again and continues with the next team on their list.
4. When no intern can apply any more, every held intern is placed on the team that holds them.
5. `team_of[i]` is the team intern `i` is placed on, or -1.

The result does not depend on which unplaced intern applies first. You may rely on that, and a test checks it.

**Before you prompt the AI**, say out loud what you will keep for each team and for each intern, how a team
decides whom to turn away, and the time complexity you expect. Then use the AI for the implementation, and
read every line it gives you before you run it.

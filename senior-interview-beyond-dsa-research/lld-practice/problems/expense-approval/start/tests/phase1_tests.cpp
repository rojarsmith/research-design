// Phase 1 tests: the core workflow. They only use the public interface in expense_workflow.hpp.
#include <string>
#include <vector>

#include "check.hpp"
#include "expense_workflow.hpp"

namespace {

using Lines = std::vector<std::string>;

// mia runs engineering and has no manager; bob reports to mia; ann and raj report to bob.
// fay and fin work in finance.
void add_team(ExpenseWorkflow& w) {
    w.addPerson("mia", "employee", "");
    w.addPerson("bob", "employee", "mia");
    w.addPerson("ann", "employee", "bob");
    w.addPerson("raj", "employee", "bob");
    w.addPerson("fay", "finance", "mia");
    w.addPerson("fin", "finance", "fay");
}

}  // namespace

TEST("new reports start as draft, with ids in creation order", 1) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(w.createReport("ann", 1200), std::string("R1"));
    EXPECT_EQ(w.createReport("zed", 1200), std::string(""));
    EXPECT_EQ(w.createReport("ann", 0), std::string(""));
    EXPECT_EQ(w.createReport("raj", 5000), std::string("R2"));
    EXPECT_EQ(w.state("R1"), std::string("draft"));
    EXPECT_EQ(w.state("R2"), std::string("draft"));
    EXPECT_EQ(w.history("R1"), Lines{});
}

TEST("submit, approve, pay", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 4200);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("submitted"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("approved"));
    EXPECT_EQ(w.pay(r, "fin"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("paid"));
    EXPECT_EQ(w.history(r), (Lines{"submit by ann: draft -> submitted", "approve by bob: submitted -> approved",
                                   "pay by fin: approved -> paid"}));
}

TEST("reject with and without a reason, then resubmit", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("raj", 900);
    EXPECT_EQ(w.submit(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.reject(r, "bob", "missing receipt"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("rejected"));
    EXPECT_EQ(w.submit(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.reject(r, "bob", ""), std::string("ok"));
    EXPECT_EQ(w.submit(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.history(r), (Lines{"submit by raj: draft -> submitted", "reject by bob: submitted -> rejected (missing receipt)",
                                   "submit by raj: rejected -> submitted", "reject by bob: submitted -> rejected",
                                   "submit by raj: rejected -> submitted", "approve by bob: submitted -> approved"}));
}

TEST("withdraw returns a submitted report to draft", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.withdraw(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("draft"));
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.history(r), (Lines{"submit by ann: draft -> submitted", "withdraw by ann: submitted -> draft",
                                   "submit by ann: draft -> submitted"}));
}

TEST("actions that are not possible from the current state", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    // draft
    EXPECT_EQ(w.withdraw(r, "ann"), std::string("invalid-transition"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("invalid-transition"));
    EXPECT_EQ(w.reject(r, "bob", "no"), std::string("invalid-transition"));
    EXPECT_EQ(w.pay(r, "fin"), std::string("invalid-transition"));
    // submitted
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.submit(r, "ann"), std::string("invalid-transition"));
    EXPECT_EQ(w.pay(r, "fin"), std::string("invalid-transition"));
    // approved
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.submit(r, "ann"), std::string("invalid-transition"));
    EXPECT_EQ(w.withdraw(r, "ann"), std::string("invalid-transition"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("invalid-transition"));
    EXPECT_EQ(w.reject(r, "bob", "late"), std::string("invalid-transition"));
    // paid
    EXPECT_EQ(w.pay(r, "fin"), std::string("ok"));
    EXPECT_EQ(w.pay(r, "fin"), std::string("invalid-transition"));
    EXPECT_EQ(w.submit(r, "ann"), std::string("invalid-transition"));
    EXPECT_EQ(w.state(r), std::string("paid"));
}

TEST("only the right person may act", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(r, "raj"), std::string("not-allowed"));  // not the owner
    EXPECT_EQ(w.submit(r, "bob"), std::string("not-allowed"));  // the manager cannot submit for her
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.withdraw(r, "bob"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(r, "ann"), std::string("not-allowed"));  // the owner
    EXPECT_EQ(w.approve(r, "raj"), std::string("not-allowed"));  // a peer
    EXPECT_EQ(w.approve(r, "mia"), std::string("not-allowed"));  // the manager's manager
    EXPECT_EQ(w.approve(r, "fin"), std::string("not-allowed"));  // finance does not approve here
    EXPECT_EQ(w.reject(r, "fay", "no"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.pay(r, "bob"), std::string("not-allowed"));  // not finance
    EXPECT_EQ(w.pay(r, "ann"), std::string("not-allowed"));
    EXPECT_EQ(w.pay(r, "fay"), std::string("ok"));
}

TEST("finance cannot pay their own report", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("fay", 7000);
    EXPECT_EQ(w.submit(r, "fay"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "mia"), std::string("ok"));
    EXPECT_EQ(w.pay(r, "fay"), std::string("not-allowed"));
    EXPECT_EQ(w.pay(r, "fin"), std::string("ok"));
}

TEST("unknown reports and unknown people", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.submit("R9", "ann"), std::string("unknown-report"));
    EXPECT_EQ(w.submit("R9", "zed"), std::string("unknown-report"));
    EXPECT_EQ(w.submit(r, "zed"), std::string("unknown-person"));
    EXPECT_EQ(w.approve(r, "zed"), std::string("unknown-person"));
    EXPECT_EQ(w.state("R9"), std::string(""));
    EXPECT_EQ(w.history("R9"), Lines{});
}

TEST("invalid-transition comes before not-allowed", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.approve(r, "raj"), std::string("invalid-transition"));  // draft, and raj is not her manager
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.pay(r, "bob"), std::string("invalid-transition"));  // submitted, and bob is not finance
    EXPECT_EQ(w.submit(r, "raj"), std::string("invalid-transition"));
}

TEST("a failed action changes nothing", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "raj"), std::string("not-allowed"));
    EXPECT_EQ(w.reject(r, "zed", "x"), std::string("unknown-person"));
    EXPECT_EQ(w.pay(r, "fin"), std::string("invalid-transition"));
    EXPECT_EQ(w.state(r), std::string("submitted"));
    EXPECT_EQ(w.history(r), (Lines{"submit by ann: draft -> submitted"}));
}

TEST("nobody can approve for a person without a manager", 1) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("mia", 2500);
    EXPECT_EQ(w.submit(r, "mia"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(r, "mia"), std::string("not-allowed"));
    EXPECT_EQ(w.reject(r, "fay", "x"), std::string("not-allowed"));
    EXPECT_EQ(w.withdraw(r, "mia"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("draft"));
}

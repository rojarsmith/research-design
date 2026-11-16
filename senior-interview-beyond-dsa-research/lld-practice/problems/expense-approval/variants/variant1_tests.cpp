// Variant 1 tests: finance review for large reports.
#include <string>
#include <vector>

#include "check.hpp"
#include "expense_workflow.hpp"

namespace {

using Lines = std::vector<std::string>;

void add_team(ExpenseWorkflow& w) {
    w.addPerson("mia", "employee", "");
    w.addPerson("bob", "employee", "mia");
    w.addPerson("ann", "employee", "bob");
    w.addPerson("raj", "employee", "bob");
    w.addPerson("fay", "finance", "mia");
    w.addPerson("fin", "finance", "fay");
}

}  // namespace

TEST("large reports go through finance review", 2) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 640000);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("finance-review"));
    EXPECT_EQ(w.approve(r, "fin"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("approved"));
    EXPECT_EQ(w.pay(r, "fay"), std::string("ok"));
    EXPECT_EQ(w.history(r), (Lines{"submit by ann: draft -> submitted", "approve by bob: submitted -> finance-review",
                                   "approve by fin: finance-review -> approved", "pay by fay: approved -> paid"}));
}

TEST("the threshold is 500000 cents", 2) {
    ExpenseWorkflow w;
    add_team(w);
    std::string small = w.createReport("ann", 499999);
    std::string large = w.createReport("ann", 500000);
    EXPECT_EQ(w.submit(small, "ann"), std::string("ok"));
    EXPECT_EQ(w.submit(large, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(small, "bob"), std::string("ok"));
    EXPECT_EQ(w.approve(large, "bob"), std::string("ok"));
    EXPECT_EQ(w.state(small), std::string("approved"));
    EXPECT_EQ(w.state(large), std::string("finance-review"));
}

TEST("only finance, and not the owner, decides in finance review", 2) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 800000);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("not-allowed"));  // the manager already did his part
    EXPECT_EQ(w.reject(r, "bob", "x"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(r, "ann"), std::string("not-allowed"));
    EXPECT_EQ(w.state(r), std::string("finance-review"));

    std::string own = w.createReport("fay", 900000);  // fay is finance; her manager is mia
    EXPECT_EQ(w.submit(own, "fay"), std::string("ok"));
    EXPECT_EQ(w.approve(own, "mia"), std::string("ok"));
    EXPECT_EQ(w.state(own), std::string("finance-review"));
    EXPECT_EQ(w.approve(own, "fay"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(own, "fin"), std::string("ok"));
    EXPECT_EQ(w.state(own), std::string("approved"));
}

TEST("finance can reject and the owner can withdraw during finance review", 2) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("raj", 510000);
    EXPECT_EQ(w.submit(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.reject(r, "fay", "over budget"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("rejected"));
    EXPECT_EQ(w.submit(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.withdraw(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.state(r), std::string("draft"));
    EXPECT_EQ(w.history(r), (Lines{"submit by raj: draft -> submitted", "approve by bob: submitted -> finance-review",
                                   "reject by fay: finance-review -> rejected (over budget)",
                                   "submit by raj: rejected -> submitted", "approve by bob: submitted -> finance-review",
                                   "withdraw by raj: finance-review -> draft"}));
}

TEST("other actions in finance review are invalid transitions", 2) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 700000);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(w.pay(r, "fin"), std::string("invalid-transition"));
    EXPECT_EQ(w.pay(r, "bob"), std::string("invalid-transition"));
    EXPECT_EQ(w.submit(r, "ann"), std::string("invalid-transition"));
    EXPECT_EQ(w.state(r), std::string("finance-review"));
}

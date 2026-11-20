// Variant 3 tests: notifications.
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
    w.addPerson("carl", "employee", "mia");
}

template <class W>
Lines inbox(const W& w, const std::string& person) {
    if constexpr (requires { w.inbox(person); }) return w.inbox(person);
    else return check::missing<Lines>("std::vector<std::string> ExpenseWorkflow::inbox(const std::string& personId) const");
}

template <class W>
std::string delegate(W& w, const std::string& manager, const std::string& to) {
    if constexpr (requires { w.delegate(manager, to); }) return w.delegate(manager, to);
    else return check::missing<std::string>("std::string ExpenseWorkflow::delegate(const std::string& managerId, const std::string& delegateId)");
}

}  // namespace

TEST("submitting notifies the manager", 4) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(inbox(w, "bob"), (Lines{"R1 needs you"}));
    EXPECT_EQ(inbox(w, "ann"), Lines{});
    EXPECT_EQ(inbox(w, "mia"), Lines{});
}

TEST("approving notifies the owner, then finance", 4) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "bob"), std::string("ok"));
    EXPECT_EQ(inbox(w, "ann"), (Lines{"R1 is now approved"}));
    EXPECT_EQ(inbox(w, "fay"), (Lines{"R1 needs you"}));
    EXPECT_EQ(inbox(w, "fin"), (Lines{"R1 needs you"}));
    EXPECT_EQ(inbox(w, "bob"), (Lines{"R1 needs you"}));
    EXPECT_EQ(w.pay(r, "fin"), std::string("ok"));
    EXPECT_EQ(inbox(w, "ann"), (Lines{"R1 is now approved", "R1 is now paid"}));
    EXPECT_EQ(inbox(w, "fay"), (Lines{"R1 needs you"}));
}

TEST("rejecting, resubmitting and withdrawing", 4) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("raj", 300);
    EXPECT_EQ(w.submit(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.reject(r, "bob", "missing receipt"), std::string("ok"));
    EXPECT_EQ(w.submit(r, "raj"), std::string("ok"));
    EXPECT_EQ(w.withdraw(r, "raj"), std::string("ok"));
    EXPECT_EQ(inbox(w, "raj"), (Lines{"R1 is now rejected"}));
    EXPECT_EQ(inbox(w, "bob"), (Lines{"R1 needs you", "R1 needs you"}));
}

TEST("finance review notifies finance, but never the owner or the actor", 4) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("fay", 900000);
    EXPECT_EQ(w.submit(r, "fay"), std::string("ok"));
    EXPECT_EQ(w.approve(r, "mia"), std::string("ok"));
    EXPECT_EQ(inbox(w, "mia"), (Lines{"R1 needs you"}));
    EXPECT_EQ(inbox(w, "fay"), (Lines{"R1 is now finance-review"}));
    EXPECT_EQ(inbox(w, "fin"), (Lines{"R1 needs you"}));
    EXPECT_EQ(w.approve(r, "fin"), std::string("ok"));
    EXPECT_EQ(inbox(w, "fay"), (Lines{"R1 is now finance-review", "R1 is now approved"}));
    EXPECT_EQ(inbox(w, "fin"), (Lines{"R1 needs you"}));  // fin did it, and fay owns it
}

TEST("the manager's delegate is notified too", 4) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(delegate(w, "bob", "carl"), std::string("ok"));
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(r, "ann"), std::string("ok"));
    EXPECT_EQ(inbox(w, "bob"), (Lines{"R1 needs you"}));
    EXPECT_EQ(inbox(w, "carl"), (Lines{"R1 needs you"}));
    EXPECT_EQ(w.approve(r, "carl"), std::string("ok"));
    EXPECT_EQ(inbox(w, "ann"), (Lines{"R1 is now approved"}));
}

TEST("failed actions notify nobody", 4) {
    ExpenseWorkflow w;
    add_team(w);
    std::string r = w.createReport("ann", 300);
    EXPECT_EQ(w.approve(r, "bob"), std::string("invalid-transition"));
    EXPECT_EQ(w.submit(r, "raj"), std::string("not-allowed"));
    for (const char* p : {"mia", "bob", "ann", "raj", "fay", "fin", "carl"}) EXPECT_EQ(inbox(w, p), Lines{});
    EXPECT_EQ(inbox(w, "zed"), Lines{});
}

TEST("messages for several reports arrive in order", 4) {
    ExpenseWorkflow w;
    add_team(w);
    std::string a = w.createReport("ann", 300);
    std::string b = w.createReport("raj", 300);
    EXPECT_EQ(w.submit(b, "raj"), std::string("ok"));
    EXPECT_EQ(w.submit(a, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(a, "bob"), std::string("ok"));
    EXPECT_EQ(w.approve(b, "bob"), std::string("ok"));
    EXPECT_EQ(inbox(w, "bob"), (Lines{"R2 needs you", "R1 needs you"}));
    EXPECT_EQ(inbox(w, "fin"), (Lines{"R1 needs you", "R2 needs you"}));
}

// Variant 2 tests: delegation while a manager is away.
#include <string>
#include <vector>

#include "check.hpp"
#include "expense_workflow.hpp"

namespace {

using Lines = std::vector<std::string>;

// carl and dana report to mia, like bob.
void add_team(ExpenseWorkflow& w) {
    w.addPerson("mia", "employee", "");
    w.addPerson("bob", "employee", "mia");
    w.addPerson("ann", "employee", "bob");
    w.addPerson("raj", "employee", "bob");
    w.addPerson("fay", "finance", "mia");
    w.addPerson("fin", "finance", "fay");
    w.addPerson("carl", "employee", "mia");
    w.addPerson("dana", "employee", "mia");
}

// The methods this variant adds. They compile whether or not your facade has them yet.
template <class W>
std::string delegate(W& w, const std::string& manager, const std::string& to) {
    if constexpr (requires { w.delegate(manager, to); }) return w.delegate(manager, to);
    else return check::missing<std::string>("std::string ExpenseWorkflow::delegate(const std::string& managerId, const std::string& delegateId)");
}

template <class W>
std::string end_delegation(W& w, const std::string& manager) {
    if constexpr (requires { w.endDelegation(manager); }) return w.endDelegation(manager);
    else return check::missing<std::string>("std::string ExpenseWorkflow::endDelegation(const std::string& managerId)");
}

}  // namespace

TEST("a delegate approves and rejects for the manager", 3) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(delegate(w, "bob", "carl"), std::string("ok"));
    std::string a = w.createReport("ann", 300);
    std::string b = w.createReport("raj", 400);
    EXPECT_EQ(w.submit(a, "ann"), std::string("ok"));
    EXPECT_EQ(w.submit(b, "raj"), std::string("ok"));
    EXPECT_EQ(w.approve(a, "carl"), std::string("ok"));
    EXPECT_EQ(w.reject(b, "carl", "no receipt"), std::string("ok"));
    EXPECT_EQ(w.history(a), (Lines{"submit by ann: draft -> submitted", "approve by carl for bob: submitted -> approved"}));
    EXPECT_EQ(w.history(b), (Lines{"submit by raj: draft -> submitted", "reject by carl for bob: submitted -> rejected (no receipt)"}));
}

TEST("the manager can still act while delegating", 3) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(delegate(w, "bob", "carl"), std::string("ok"));
    std::string a = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(a, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(a, "bob"), std::string("ok"));
    EXPECT_EQ(w.history(a), (Lines{"submit by ann: draft -> submitted", "approve by bob: submitted -> approved"}));
}

TEST("ending and replacing a delegation", 3) {
    ExpenseWorkflow w;
    add_team(w);
    std::string a = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(a, "ann"), std::string("ok"));
    EXPECT_EQ(delegate(w, "bob", "carl"), std::string("ok"));
    EXPECT_EQ(end_delegation(w, "bob"), std::string("ok"));
    EXPECT_EQ(w.approve(a, "carl"), std::string("not-allowed"));
    EXPECT_EQ(delegate(w, "bob", "carl"), std::string("ok"));
    EXPECT_EQ(delegate(w, "bob", "dana"), std::string("ok"));
    EXPECT_EQ(w.approve(a, "carl"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(a, "dana"), std::string("ok"));
    EXPECT_EQ(w.history(a).back(), std::string("approve by dana for bob: submitted -> approved"));
    EXPECT_EQ(end_delegation(w, "ann"), std::string("ok"));  // ann has no delegate: still ok
}

TEST("delegation is not transitive", 3) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(delegate(w, "bob", "carl"), std::string("ok"));
    EXPECT_EQ(delegate(w, "carl", "dana"), std::string("ok"));
    std::string a = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(a, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(a, "dana"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(a, "carl"), std::string("ok"));
}

TEST("a delegate never decides on their own report", 3) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(delegate(w, "bob", "ann"), std::string("ok"));
    std::string own = w.createReport("ann", 300);
    std::string other = w.createReport("raj", 300);
    EXPECT_EQ(w.submit(own, "ann"), std::string("ok"));
    EXPECT_EQ(w.submit(other, "raj"), std::string("ok"));
    EXPECT_EQ(w.approve(own, "ann"), std::string("not-allowed"));
    EXPECT_EQ(w.reject(own, "ann", "x"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(other, "ann"), std::string("ok"));
    EXPECT_EQ(w.history(other).back(), std::string("approve by ann for bob: submitted -> approved"));
}

TEST("finance permissions are not delegated", 3) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(delegate(w, "bob", "carl"), std::string("ok"));
    EXPECT_EQ(delegate(w, "fin", "raj"), std::string("ok"));
    std::string big = w.createReport("ann", 600000);
    EXPECT_EQ(w.submit(big, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(big, "carl"), std::string("ok"));
    EXPECT_EQ(w.state(big), std::string("finance-review"));
    EXPECT_EQ(w.history(big).back(), std::string("approve by carl for bob: submitted -> finance-review"));
    EXPECT_EQ(w.approve(big, "carl"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(big, "raj"), std::string("not-allowed"));
    EXPECT_EQ(w.approve(big, "fin"), std::string("ok"));
    EXPECT_EQ(w.pay(big, "raj"), std::string("not-allowed"));
    EXPECT_EQ(w.pay(big, "fay"), std::string("ok"));
}

TEST("delegation errors", 3) {
    ExpenseWorkflow w;
    add_team(w);
    EXPECT_EQ(delegate(w, "bob", "zed"), std::string("unknown-person"));
    EXPECT_EQ(delegate(w, "zed", "carl"), std::string("unknown-person"));
    EXPECT_EQ(delegate(w, "bob", "bob"), std::string("not-allowed"));
    EXPECT_EQ(end_delegation(w, "zed"), std::string("unknown-person"));
    std::string a = w.createReport("ann", 300);
    EXPECT_EQ(w.submit(a, "ann"), std::string("ok"));
    EXPECT_EQ(w.approve(a, "carl"), std::string("not-allowed"));  // the failed calls set up nothing
}

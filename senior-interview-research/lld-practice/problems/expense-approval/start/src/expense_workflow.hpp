// expense_workflow.hpp: the public interface the tests use.
// Keep these signatures. Everything behind them is your design: add members here, and add your own
// classes in new files under src/.
#pragma once

#include <string>
#include <vector>

class ExpenseWorkflow {
public:
    // role is "employee" or "finance". managerId is "" when the person has no manager.
    void addPerson(const std::string& id, const std::string& role, const std::string& managerId);

    // A new report in state "draft". Returns "R1", "R2", ... in creation order, or "" (and creates
    // nothing) if the owner is unknown or the amount is not positive.
    std::string createReport(const std::string& ownerId, long long amountCents);

    // Each action returns "ok" or an error code (see TASK.md).
    std::string submit(const std::string& reportId, const std::string& actorId);
    std::string withdraw(const std::string& reportId, const std::string& actorId);
    std::string approve(const std::string& reportId, const std::string& actorId);
    std::string reject(const std::string& reportId, const std::string& actorId, const std::string& reason);
    std::string pay(const std::string& reportId, const std::string& actorId);

    // "draft", "submitted", ...; "" for an unknown report.
    std::string state(const std::string& reportId) const;
    // One line per successful action, oldest first; empty for an unknown report.
    std::vector<std::string> history(const std::string& reportId) const;
};

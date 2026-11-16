#include "expense_workflow.hpp"

#include <stdexcept>

void ExpenseWorkflow::addPerson(const std::string&, const std::string&, const std::string&) {
    throw std::logic_error("not implemented: ExpenseWorkflow::addPerson");
}

std::string ExpenseWorkflow::createReport(const std::string&, long long) {
    throw std::logic_error("not implemented: ExpenseWorkflow::createReport");
}

std::string ExpenseWorkflow::submit(const std::string&, const std::string&) {
    throw std::logic_error("not implemented: ExpenseWorkflow::submit");
}

std::string ExpenseWorkflow::withdraw(const std::string&, const std::string&) {
    throw std::logic_error("not implemented: ExpenseWorkflow::withdraw");
}

std::string ExpenseWorkflow::approve(const std::string&, const std::string&) {
    throw std::logic_error("not implemented: ExpenseWorkflow::approve");
}

std::string ExpenseWorkflow::reject(const std::string&, const std::string&, const std::string&) {
    throw std::logic_error("not implemented: ExpenseWorkflow::reject");
}

std::string ExpenseWorkflow::pay(const std::string&, const std::string&) {
    throw std::logic_error("not implemented: ExpenseWorkflow::pay");
}

std::string ExpenseWorkflow::state(const std::string&) const {
    throw std::logic_error("not implemented: ExpenseWorkflow::state");
}

std::vector<std::string> ExpenseWorkflow::history(const std::string&) const {
    throw std::logic_error("not implemented: ExpenseWorkflow::history");
}

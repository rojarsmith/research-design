#pragma once

#include <vector>

#include "event.hpp"

// One timeline sorted by timestamp. Events with equal timestamps keep file order,
// then line order. A file does not have to be sorted itself.
std::vector<Event> merge_logs(const std::vector<LogFile>& files);

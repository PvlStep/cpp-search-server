#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <utility>

using namespace std::string_literals;

std::vector<std::string> SplitIntoWords(const std::string& text);

std::vector<std::string_view> SplitIntoWords(const std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

template<typename Type>
void MakeUniqueVector(std::vector<Type>& data_) {
    std::sort(data_.begin(), data_.end());
    auto last = std::unique(data_.begin(), data_.end());
    data_.erase(last, data_.end());
}

template<typename Type, typename ExecutionPolicy>
void MakeUniqueVector(const ExecutionPolicy& policy, std::vector<Type>& data_) {
    std::sort(policy, data_.begin(), data_.end());
    auto last = std::unique(policy, data_.begin(), data_.end());
    data_.erase(last, data_.end());
}
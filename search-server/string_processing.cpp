#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;
    
    std::string_view delimiter = " ";
    size_t first = 0;

    while (first < text.size()) {
        const auto second = text.find_first_of(delimiter, first);

        if (first != second) {
            words.emplace_back(text.substr(first, second - first));
        }

        if (second == std::string_view::npos) {
            break;
        }

        first = second + 1;
    }
   
    return words;
}

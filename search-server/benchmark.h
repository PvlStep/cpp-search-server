#pragma once

#include "search_server.h"
#include "log_duration.h"

#include <iostream>
#include <string>
#include <vector>
#include <random>

std::string GenerateWord(std::mt19937& generator, int max_length) {
    const int8_t length = std::uniform_int_distribution(1, max_length)(generator);
    std::string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(static_cast<char>(std::uniform_int_distribution(97, 122)(generator)));
    }
    return word;
}

std::vector<std::string> GenerateDictionary(std::mt19937& generator, int word_count, int max_length) {
    std::vector<std::string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}
std::string GenerateQuery(std::mt19937& generator, const std::vector<std::string>& dictionary, int max_word_count) {
    const int word_count = std::uniform_int_distribution(1, max_word_count)(generator);
    std::string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        query += dictionary[std::uniform_int_distribution<int>(0, static_cast<int>(dictionary.size()) - 1)(generator)];
    }
    return query;
}
std::vector<std::string> GenerateQueries(std::mt19937& generator, const std::vector<std::string>& dictionary, int query_count, int max_word_count) {
    std::vector<std::string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename QueriesProcessor>
void Test(std::string_view mark, QueriesProcessor processor, const SearchServer& search_server, const std::vector<std::string>& queries) {
    LOG_DURATION(mark);
    const auto documents_lists = processor(search_server, queries);
}

#define TEST(processor) Test(#processor, processor, search_server, queries)

template <typename Func>
void BenchMark(Func function) {
    std::mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 2'000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);
    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }
    const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
    TEST(function);
}
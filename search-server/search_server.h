#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <deque>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <cmath>
#include <regex> 
#include <execution>
#include <stdlib.h>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "concurrent_map.h"


using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double INACCURACY = 1e-6;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string_view stop_words_text);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin();

    std::set<int>::const_iterator end();

    void RemoveDocument(int document_id);

    template <typename ExecutionPolicy>
    void RemoveDocument(const ExecutionPolicy& policy, int document_id);


    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
        int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy,
        const std::string_view raw_query, int document_id) const;

    //template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy,
        const std::string_view raw_query, int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::map<std::string_view, double> freq;
    };
    std::deque<std::string> documents_storage;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;


    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct QueryNew {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    static bool IsInvalidQuery(const std::string& text);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;

    QueryNew ParseQuery(const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const QueryNew& query,
        DocumentPredicate document_predicate) const;
};

void RemoveDuplicates(SearchServer& search_server);

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    auto query = ParseQuery(raw_query);

    MakeUniqueVector(query.minus_words);
    MakeUniqueVector(query.plus_words);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < INACCURACY) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const QueryNew& query,
    DocumentPredicate document_predicate) const {
    //std::map<int, double> document_to_relevance;

    ConcurrentMap<int, double> document_to_relevance(10);

    /*
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [this, &policy, &document_to_relevance, &document_predicate]
    (const std::string_view word) {
        if (word_to_document_freqs_.count(word) == 1) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            std::for_each(policy, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), 
                [this, policy, &document_to_relevance, &inverse_document_freq, &document_predicate]
            (const std::pair<int, double> term_freq) {
                    const auto& document_data = documents_.at(term_freq.first);
                    if (document_predicate(term_freq.first, document_data.status, document_data.rating)) {
                        document_to_relevance[term_freq.first] += term_freq.second * inverse_document_freq;
                    }
                });
        }
        });
*/


std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [this, &policy, &document_to_relevance, &document_predicate]
(const std::string_view word) {
    if (word_to_document_freqs_.count(word) == 1) {
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        std::for_each(policy, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
            [this, policy, &document_to_relevance, &inverse_document_freq, &document_predicate]
        (const std::pair<int, double> term_freq) {
                const auto& document_data = documents_.at(term_freq.first);
                if (document_predicate(term_freq.first, document_data.status, document_data.rating)) {
                    document_to_relevance[term_freq.first].ref_to_value += term_freq.second * inverse_document_freq;
                }
            });
    }
    });


    /*
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    */
/*
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    */

    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&policy, this, &document_to_relevance](const std::string_view word) {
        if (word_to_document_freqs_.count(word) == 1) {
            std::for_each(policy, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                [&policy, this, &document_to_relevance](const std::pair<int, double> docs_freq) {
                    document_to_relevance.erase(docs_freq.first);
                });
        }
        });

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
void SearchServer::RemoveDocument(const ExecutionPolicy& policy, int document_id) {
    if (!documents_.count(document_id)) {
        return;
    }
    std::vector<const std::string*> temp(documents_[document_id].freq.size());

    std::transform(policy, documents_[document_id].freq.begin(), documents_[document_id].freq.end(), temp.begin(), [](const std::pair<const std::string&, double> word_f) {
        return &word_f.first;
        });

    std::for_each(policy, temp.begin(), temp.end(), [&](const std::string* word) {
        word_to_document_freqs_.at(*word).erase(document_id); });

    documents_.erase(document_id);
    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
}


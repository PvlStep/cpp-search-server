#include "search_server.h"


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(
        SplitIntoWords(std::string(stop_words_text)))
{
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query);
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }

    documents_storage.push_back(std::string(document));
    const auto words = SplitIntoWordsNoStop(documents_storage.back());

    const double inv_word_count = 1.0 / words.size();
    std::map<std::string_view, double> word_freq;
    for (const std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_freq[word] = inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, word_freq });
    document_ids_.insert(document_id);
}


int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}


std::set<int>::const_iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::const_iterator  SearchServer::end() {
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    const static std::map<std::string_view, double> empty_map{};
    if (documents_.count(document_id)) {
        return documents_.at(document_id).freq;
    }
    else {
        return empty_map;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    for (auto [word, rating] : documents_[document_id].freq) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    documents_.erase(document_id);
    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query,
    int document_id) const {
    QueryNew query = SearchServer::ParseQuery(raw_query);

    MakeUniqueVector(query.minus_words);
    MakeUniqueVector(query.plus_words);

    std::vector<std::string_view> matched_words;
    bool galya_cansel = false;

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            galya_cansel = true;
            break;
        }
    }

    if (!galya_cansel) {
        for (const std::string_view word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, const std::string_view raw_query,
    int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, const std::string_view raw_query,
    int document_id) const {
    QueryNew query = SearchServer::ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;

    bool galya_cansel = std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [&](const auto& word) {
        return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
        });

    if (!galya_cansel) {
        matched_words.resize(query.plus_words.size());
        auto last = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
            [&](const std::string_view word) {
                return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
            });
        std::sort(policy, matched_words.begin(), last);
        auto last2 = std::unique(policy, matched_words.begin(), last);
        matched_words.resize(distance(matched_words.begin(), last2));
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

bool SearchServer::IsInvalidQuery(const std::string& text) {

    bool double_minus = std::regex_match(text, std::regex("(--)(.*)"));
    bool last_char_is_minus = (!text.empty() && text[text.size() - 1] == '-');

    return double_minus || last_char_is_minus;

}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(std::string_view(text))) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::QueryNew SearchServer::ParseQuery(const std::string_view text) const {
    QueryNew result;
    for (const auto word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    return result;
}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
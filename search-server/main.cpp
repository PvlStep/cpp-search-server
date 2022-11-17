#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename PredicateDoc>
    vector<Document> FindTopDocuments(const string& raw_query,
        PredicateDoc predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance
                    || (abs(lhs.relevance - rhs.relevance) < 1e-6 && lhs.rating > rhs.rating);
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }


    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log((GetDocumentCount() * 1.0) / word_to_document_freqs_.at(word).size());

    }

    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query,
        Predicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};



//Шаблон для векторов
template <typename Data>
ostream& operator<<(ostream& out, const vector<Data>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template <typename Data>
ostream& operator<<(ostream& out, const set<Data>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}


template <typename Key, typename Value>
ostream& operator<<(ostream& out, const pair<Key, Value>& container) {
    out << container.first << ": "s << container.second;
    return out;
}

template <typename Output>
ostream& Print(ostream& out, const Output container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
    return out;
}



template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImp(const TestFunc& func, const string& func_name) {
    func();
    cerr << func_name << " OK"s << endl;
}

#define  RUN_TEST(func) RunTestImp(func, #func);



// -------- Начало модульных тестов поисковой системы ----------



// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

/*
Разместите код остальных тестов здесь
*/

// Добавление документов.Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddDocument() {
    const int doc_id = 43;
    const string content = "поисковый запрос для тестирования поискового сервера"s;
    const vector<int> ratings = { 5, 4, 3, 5 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("запрос сервера"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);

    }

}

//Поддержка минус - слов.Документы, содержащие минус - слова поискового запроса, не должны включаться в результаты поиска.

void TestMinusWords() {
    const int doc_id = 44;
    const string content = "поисковый запрос для минус слов"s;
    const vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const string content2 = "поисковый запрос для минус слов второй"s;
    const vector<int> ratings2 = { 5, 3, 5 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("минус -второй"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);

    }

}

//Матчинг документов.При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе.Если есть соответствие хотя бы по одному минус - слову, должен возвращаться пустой список слов.

void TestMatching() {
    const int doc_id = 46;
    const string content = "matching test for search server"s;
    const vector<int> ratings = { 5, 3, 5 };
    const vector<string> request = { "test"s, "server"s };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto match_docs = server.MatchDocument("test server"s, doc_id);
        const auto match_words = get<vector<string>>(match_docs);
        ASSERT(count(match_words.begin(), match_words.end(), request[0]));
        ASSERT(count(match_words.begin(), match_words.end(), request[1]));

    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto match_docs = server.MatchDocument("test -server"s, doc_id);
        const auto match_words = get<vector<string>>(match_docs);
        ASSERT(match_words.empty());

    }
}

//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.

void TestRelevance() {
    const int doc_id = 44;
    const string content = "поисковый запрос для релевантности"s;
    const vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const string content2 = "поисковый запрос для релевантности поискового сервера"s;
    const vector<int> ratings2 = { 5, 3, 5 };

    const int doc_id3 = 46;
    const string content3 = "поисковый запрос для релевантности поискового сервера для яндекс практикума"s;
    const vector<int> ratings3 = { 5, 3, 5 };

    const vector<int> revelance = { 46, 45, 44 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto found_docs = server.FindTopDocuments("запрос сервера яндекс"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, revelance[0]);
        const Document& doc2 = found_docs[2];
        ASSERT_EQUAL(doc2.id, revelance[2]);


    }

}

//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.

void TestRating() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 5, 4, 5, 3 };
    const double average_r = 4.25;
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        const Document& doc0 = found_docs[0];
        ASSERT((doc0.rating - average_r) < 1e-6);
    }
}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.

void TestPredicate() {

    {
        SearchServer server;
        server.AddDocument(50, "поисковый запрос тест статус"s, DocumentStatus::ACTUAL, { 1, 1, 2 });
        server.AddDocument(51, "для тест запроса по статусу"s, DocumentStatus::ACTUAL, { 1, 3, 1 });
        server.AddDocument(52, "поисковый сервер тест"s, DocumentStatus::ACTUAL, { 5, 4, 5 });
        server.AddDocument(53, "удар"s, DocumentStatus::BANNED, { 5, 5, 5 });
        server.AddDocument(54, "статус статус тест тест раз раз раз"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        const auto found_docs = server.FindTopDocuments("раз"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 54);
        const auto found_docs2 = server.FindTopDocuments("статус тест"s, [](int document_id, DocumentStatus status, int rating) { return rating >= 4; });
        ASSERT_EQUAL(found_docs2.size(), 2);
        ASSERT_EQUAL(found_docs2[0].id, 54);
        ASSERT_EQUAL(found_docs2[1].id, 52);
    }

}

//Поиск документов, имеющих заданный статус.

void TestSearchStatus() {
    const int doc_id = 44;
    const string content = "поисковый запрос для релевантности"s;
    const vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const string content2 = "поисковый запрос для релевантности поискового сервера"s;
    const vector<int> ratings2 = { 5, 3, 5 };

    const int doc_id3 = 46;
    const string content3 = "поисковый запрос для релевантности поискового сервера для яндекс практикума"s;
    const vector<int> ratings3 = { 5, 3, 5 };

    const vector<int> revelance = { 46, 45, 44 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, ratings3);
        const auto found_docs = server.FindTopDocuments("запрос сервера яндекс"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 44);
        const auto found_docs2 = server.FindTopDocuments("запрос сервера яндекс"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs2.size(), 2);
        ASSERT_EQUAL(found_docs2[0].id, 46);
    }

}





//Корректное вычисление релевантности найденных документов.

void TestCompRevelance() {
    const double rev = 0;
    const double rev2 = 0.173286;
    //0.173287

    {
        SearchServer server;
        server.AddDocument(50, "статус количество запрос текст"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        server.AddDocument(51, "сервер статус запрос текст"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        const auto found_docs = server.FindTopDocuments("сервер статус"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT((abs(found_docs[0].relevance) - rev2) < 1e-6);
        ASSERT((abs(found_docs[1].relevance) - rev) < 1e-5);
    }

}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatching);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestRating);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestSearchStatus);
    RUN_TEST(TestCompRevelance);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
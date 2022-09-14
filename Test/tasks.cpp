#include <iostream>
#include <istream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>


using namespace std;

// Глобальные константы
int MAX_RESULT_DOCUMENT_COUNT = 5; // Количество отображаемых документов с макс.релевантностью

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

// Функция "SplitIntoWords" разделяет строку на отдельные слова по пробелу
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

//Структура Document хранит значения id и релевантности найденных документов
struct Document {
    int id;
    double relevance;
    int rating;
    DocumentStatus status;
};


// Функция считывающая строку
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}


// Функция, считывающая количество строк
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}


//Класс SearchServer
class SearchServer {

    //Метка "public" используется для обозначения функций (методов), к которым могут обращаться "внешние" источники
public:


    //Метод "AddDocument" добавляет в словарь слов: само слово и словарь, включающий id документов, где встречается это слово и его релевантность (tf).
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int> ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        
        document_data[document_id] = { ComputeAverageRating(ratings), status };
        //document_rating[document_id] = ComputeAverageRating(ratings);
        ++document_count_;
        const double count_word = 1.0 / words.size();
        for (const string& word : words) {
            word_to_documents_tf[word][document_id] += count_word;
        }

    }

    //Метод "SetStopWords" получает список стоп-слов
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words.insert(word);
        }
    }

    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);
        vector<Document> actual_documents;

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance;
            });

        for (Document doc : matched_documents) {
            if (doc.status == status) {
                actual_documents.push_back(doc);
            }
        }

        if (actual_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            actual_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return actual_documents;
    }


    //Метка "private" используется для обозначения функций и полей, не доступных к редактированию извне
private:

    struct InternalData {
        int rating;  
        DocumentStatus status;
    };

    //Стуктура для хранения "плюс"-слов и минус-слов
    struct Query {
        set<string> plus;
        set<string> minus;
    };

    //Ключи контейнера word_to_documents — слова из добавленных документов, а значения — id документов, в которых это слово встречается, 
 //Множество "stop_words" хранит стоп-слова
    map<string, map<int, double>> word_to_documents_tf;
    InternalData int_data;
    map<int, InternalData> document_data;
    set<string> stop_words;
    int document_count_ = 0;

    // Функция, воспринимающая запрос
    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                if (!stop_words.count(word.substr(1))) {
                    query_words.minus.insert(word.substr(1));
                }
            }
            else {
                query_words.plus.insert(word);
            }

        }
        return query_words;
    }

 

    // Функция "SplitIntoWordsNoStop" разделяет текст из потока слова и возвращает массив словю
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (stop_words.count(word) == 0) {
                words.push_back(word);
            }
        }
        return words;
    }

      //Функция расчитывающая средний рейтинг
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.size() == 0) {
            return 0;
        }
        int average = 0;
        for (const int& r : ratings) {
            average += r;
        }
        return average / static_cast<int>(ratings.size());
    };

    // Для каждого документа возвращает его id и релевантность 
    vector<Document> FindAllDocuments(const Query& query_words) const {
        map<int, double> documents_to_relevance; //хранит id документа и его релевантность
        for (const string& word : query_words.plus) {

            if (word_to_documents_tf.count(word)) {
                double itf = log(document_count_ * 1.0 / word_to_documents_tf.at(word).size());
                for (const auto& tf : word_to_documents_tf.at(word)) {
                    documents_to_relevance[tf.first] += tf.second * itf;
                }
            }
        }
        for (const string& word : query_words.minus) {
            if (word_to_documents_tf.count(word)) {
                for (const auto& tf : word_to_documents_tf.at(word)) {
                    documents_to_relevance.erase(tf.first);
                }
            }
        }
        vector<Document> matched_documents;
        for (const auto& dock : documents_to_relevance) {
            matched_documents.push_back({ dock.first, dock.second, document_data.at(dock.first).rating, document_data.at(dock.first).status});
        }
        return matched_documents;
    }
};

// Функция создающая поисковый сервер 
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const string document = ReadLine();

        int status;
        cin >> status;

        int ratings_size;
        cin >> ratings_size;

        // создали вектор размера ratings_size из нулей
        vector<int> ratings(ratings_size, 0);

        // считали каждый элемент с помощью ссылки
        for (int& rating : ratings) {
            cin >> rating;
        }

        search_server.AddDocument(document_id, document, static_cast<DocumentStatus>(status), ratings);
        ReadLine();
    }
    return search_server;
}



int main() {
    const SearchServer server = CreateSearchServer();
    const string query = ReadLine();
    const DocumentStatus status = static_cast<DocumentStatus>(ReadLineWithNumber());
    for (Document& document : server.FindTopDocuments(query, status)) {
        cout << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s << endl;
    }
    return 0;
}

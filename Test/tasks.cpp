#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>


using namespace std;

// ���������� ���������
int MAX_RESULT_DOCUMENT_COUNT = 5; // ���������� ������������ ���������� � ����.��������������

// ������� "SplitIntoWords" ��������� ������ �� ��������� ����� �� �������
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

//��������� Document ������ �������� id � ������������� ��������� ����������
struct Document {
    int id;
    double relevance;
    int rating;
};


// ������� ����������� ������
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}


// �������, ����������� ���������� �����
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}


//����� SearchServer
class SearchServer {

    //����� "public" ������������ ��� ����������� ������� (�������), � ������� ����� ���������� "�������" ���������
public:


    //����� "AddDocument" ��������� � ������� ����: ���� ����� � �������, ���������� id ����������, ��� ����������� ��� ����� � ��� ������������� (tf).
    void AddDocument(int document_id, const string& document, const vector<int> ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        document_rating[document_id] = ComputeAverageRating(ratings);
        ++document_count_;
        const double count_word = 1.0 / words.size();
        for (const string& word : words) {
            word_to_documents_tf[word][document_id] += count_word;
        }

    }

    //����� "SetStopWords" �������� ������ ����-����
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words.insert(word);
        }
    }

    // ���������� ���-5 ����� ����������� ���������� � ���� ���: {id, �������������}
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }


    //����� "private" ������������ ��� ����������� ������� � �����, �� ��������� � �������������� �����
private:

    //�������� ��� �������� "����"-���� � �����-����
    struct Query {
        set<string> plus;
        set<string> minus;
    };

    //����� ���������� word_to_documents � ����� �� ����������� ����������, � �������� � id ����������, � ������� ��� ����� �����������, 
 //��������� "stop_words" ������ ����-�����
    map<string, map<int, double>> word_to_documents_tf;
    map<int, int> document_rating;
    set<string> stop_words;
    int document_count_ = 0;

    // �������, �������������� ������
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

 

    // ������� "SplitIntoWordsNoStop" ��������� ����� �� ������ ����� � ���������� ������ �����
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (stop_words.count(word) == 0) {
                words.push_back(word);
            }
        }
        return words;
    }

      //������� ������������� ������� �������
    int ComputeAverageRating(const vector<int>& ratings) const {
        if (ratings.size() == 0) {
            return 0;
        }
        int average = 0;
        for (const int& r : ratings) {
            average += r;
        }
        return average / static_cast<int>(ratings.size());
    }

    // ��� ������� ��������� ���������� ��� id � ������������� 
    vector<Document> FindAllDocuments(const Query& query_words) const {
        map<int, double> documents_to_relevance; //������ id ��������� � ��� �������������
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
            matched_documents.push_back({ dock.first, dock.second, document_rating.at(dock.first)});
        }
        return matched_documents;
    }
};

// ������� ��������� ��������� ������ 
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const string document = ReadLine();
        int ratings_size;
        cin >> ratings_size;

        // ������� ������ ������� ratings_size �� �����
        vector<int> ratings(ratings_size, 0);

        // ������� ������ ������� � ������� ������
        for (int& rating : ratings) {
            cin >> rating;
        }

        search_server.AddDocument(document_id, document, ratings);
        ReadLine();
    }
    return search_server;
}



int main() {
    const SearchServer server = CreateSearchServer();
    const string query = ReadLine();
    for (Document& document : server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s << endl;
    }
    return 0;
}

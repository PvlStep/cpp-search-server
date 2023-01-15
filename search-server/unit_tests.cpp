#include "unit_tests.h"

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

// Добавление документов.Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddDocument() {
    const int doc_id = 43;
    const std::string content = "поисковый запрос для тестирования поискового сервера"s;
    const std::vector<int> ratings = { 5, 4, 3, 5 };

    {
        SearchServer server("test"s);
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
    const std::string content = "поисковый запрос для минус слов"s;
    const std::vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const std::string content2 = "поисковый запрос для минус слов второй"s;
    const std::vector<int> ratings2 = { 5, 3, 5 };
    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("минус -второй"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);

    }

}

/*Матчинг документов.При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе.
Если есть соответствие хотя бы по одному минус - слову, должен возвращаться пустой список слов.*/

void TestMatching() {
    const int doc_id = 46;
    const std::string content = "matching test for search server"s;
    const std::vector<int> ratings = { 5, 3, 5 };
    const std::vector<std::string> request = { "test"s, "server"s };

    {
        SearchServer server("cpp"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto match_docs = server.MatchDocument("test server"s, doc_id);
        const auto match_words = std::get<std::vector<std::string>>(match_docs);
        ASSERT(count(match_words.begin(), match_words.end(), request[0]));
        ASSERT(count(match_words.begin(), match_words.end(), request[1]));

    }

    {
        SearchServer server("cpp"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto match_docs = server.MatchDocument("test -server"s, doc_id);
        const auto match_words = std::get<std::vector<std::string>>(match_docs);
        ASSERT(match_words.empty());

    }
}

//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.

void TestRelevance() {
    const int doc_id = 44;
    const std::string content = "поисковый запрос для релевантности"s;
    const std::vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const std::string content2 = "поисковый запрос для релевантности поискового сервера"s;
    const std::vector<int> ratings2 = { 5, 3, 5 };

    const int doc_id3 = 46;
    const std::string content3 = "поисковый запрос для релевантности поискового сервера для яндекс практикума"s;
    const std::vector<int> ratings3 = { 5, 3, 5 };

    {
        SearchServer server("cpp"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto found_docs = server.FindTopDocuments("запрос сервера яндекс"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL(doc0.id, doc_id3);
        const Document& doc2 = found_docs[2];
        ASSERT(doc0.relevance > doc1.relevance && doc1.relevance > doc2.relevance);
    }
}

//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.

void TestRating() {
    const std::string content1 = "cat in the city"s;
    const std::string content2 = "dog in the village"s;
    const std::string content3 = "capybara in the bar"s;
    const std::vector<int> ratings_positive = { 5, 4, 5, 3 };
    const std::vector<int> ratings_negative = { -5, -5, -5, -5, -4 };
    const std::vector<int> ratings_mix = { -5, 4, 5, 5 };
    const int average_rating1 = 4;
    const int average_rating2 = -4;
    const int average_rating3 = 2;
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server("test"s);
        server.AddDocument(42, content1, DocumentStatus::ACTUAL, ratings_positive);
        server.AddDocument(43, content2, DocumentStatus::ACTUAL, ratings_negative);
        server.AddDocument(44, content3, DocumentStatus::ACTUAL, ratings_mix);
        const auto found_docs1 = server.FindTopDocuments("cat"s);
        const Document& doc1 = found_docs1[0];
        ASSERT(abs(doc1.rating - average_rating1) < INACCURACY);

        const auto found_docs2 = server.FindTopDocuments("dog"s);
        const Document& doc2 = found_docs2[0];
        ASSERT(abs(doc2.rating - average_rating2) < INACCURACY);

        const auto found_docs3 = server.FindTopDocuments("bar"s);
        const Document& doc3 = found_docs3[0];
        ASSERT(abs(doc3.rating - average_rating3) < INACCURACY);
    }
}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.

void TestPredicate() {
    {
        SearchServer server("test"s);
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
    const std::string content = "поисковый запрос для релевантности"s;
    const std::vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const std::string content2 = "поисковый запрос для релевантности поискового сервера"s;
    const std::vector<int> ratings2 = { 5, 3, 5 };

    const int doc_id3 = 46;
    const std::string content3 = "поисковый запрос для релевантности поискового сервера для яндекс практикума"s;
    const std::vector<int> ratings3 = { 5, 3, 5 };

    const std::vector<int> revelance = { 46, 45, 44 };

    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, ratings3);
        server.AddDocument(47, "Дополнительный документ", DocumentStatus::IRRELEVANT, ratings2);
        server.AddDocument(48, "Удаленный документ", DocumentStatus::REMOVED, ratings);
        const auto found_docs = server.FindTopDocuments("запрос сервера яндекс"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 44);
        const auto found_docs2 = server.FindTopDocuments("запрос сервера яндекс"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs2.size(), 2);
        ASSERT_EQUAL(found_docs2[0].id, 46);
        const auto found_docs3 = server.FindTopDocuments("документ"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs3.size(), 1);
        ASSERT_EQUAL(found_docs3[0].id, 47);
        const auto found_docs4 = server.FindTopDocuments("документ"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs4.size(), 1);
        ASSERT_EQUAL(found_docs4[0].id, 48);
    }
}

//Корректное вычисление релевантности найденных документов.

void TestCompRevelance() {
    const double rev = 0;
    const double rev2 = 0.173286;

    {
        SearchServer server("test"s);
        server.AddDocument(50, "статус количество запрос текст"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        server.AddDocument(51, "сервер статус запрос текст"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        const auto found_docs = server.FindTopDocuments("сервер статус"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT((abs(found_docs[0].relevance) - rev2) < INACCURACY);
        ASSERT((abs(found_docs[1].relevance) - rev) < INACCURACY);
    }

}

void TestRemoveDuplicates() {
    int before_ = 9;
    int after_ = 5;

    {
        SearchServer search_server("and with"s);

        search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // дубликат документа 2, будет удалён
        search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // отличие только в стоп-словах, считаем дубликатом
        search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // множество слов такое же, считаем дубликатом документа 1
        search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // добавились новые слова, дубликатом не является
        search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
        search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

        // есть не все слова, не является дубликатом
        search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // слова из разных документов, не является дубликатом
        search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        ASSERT(search_server.GetDocumentCount() == before_);
        RemoveDuplicates(search_server);
        ASSERT(search_server.GetDocumentCount() == after_);
    }
}

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
    RUN_TEST(TestRemoveDuplicates);
    std::cout << "Search server testing finished"s << std::endl;
}
#include "unit_tests.h"

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    // ������� ����������, ��� ����� �����, �� ��������� � ������ ����-����,
    // ������� ������ ��������
    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // ����� ����������, ��� ����� ����� �� �����, ��������� � ������ ����-����,
    // ���������� ������ ���������
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

// ���������� ����������.����������� �������� ������ ���������� �� ���������� �������, ������� �������� ����� �� ���������.
void TestAddDocument() {
    const int doc_id = 43;
    const std::string content = "��������� ������ ��� ������������ ���������� �������"s;
    const std::vector<int> ratings = { 5, 4, 3, 5 };

    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("������ �������"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);

    }

}

//��������� ����� - ����.���������, ���������� ����� - ����� ���������� �������, �� ������ ���������� � ���������� ������.
void TestMinusWords() {
    const int doc_id = 44;
    const std::string content = "��������� ������ ��� ����� ����"s;
    const std::vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const std::string content2 = "��������� ������ ��� ����� ���� ������"s;
    const std::vector<int> ratings2 = { 5, 3, 5 };
    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("����� -������"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);

    }

}

/*������� ����������.��� �������� ��������� �� ���������� ������� ������ ���� ���������� ��� ����� �� ���������� �������, �������������� � ���������.
���� ���� ������������ ���� �� �� ������ ����� - �����, ������ ������������ ������ ������ ����.*/

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

//���������� ��������� ���������� �� �������������. ������������ ��� ������ ���������� ���������� ������ ���� ������������� � ������� �������� �������������.

void TestRelevance() {
    const int doc_id = 44;
    const std::string content = "��������� ������ ��� �������������"s;
    const std::vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const std::string content2 = "��������� ������ ��� ������������� ���������� �������"s;
    const std::vector<int> ratings2 = { 5, 3, 5 };

    const int doc_id3 = 46;
    const std::string content3 = "��������� ������ ��� ������������� ���������� ������� ��� ������ ����������"s;
    const std::vector<int> ratings3 = { 5, 3, 5 };

    {
        SearchServer server("cpp"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto found_docs = server.FindTopDocuments("������ ������� ������"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL(doc0.id, doc_id3);
        const Document& doc2 = found_docs[2];
        ASSERT(doc0.relevance > doc1.relevance && doc1.relevance > doc2.relevance);
    }
}

//���������� �������� ����������. ������� ������������ ��������� ����� �������� ��������������� ������ ���������.

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
    // ������� ����������, ��� ����� �����, �� ��������� � ������ ����-����,
    // ������� ������ ��������
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

//���������� ����������� ������ � �������������� ���������, ����������� �������������.

void TestPredicate() {
    {
        SearchServer server("test"s);
        server.AddDocument(50, "��������� ������ ���� ������"s, DocumentStatus::ACTUAL, { 1, 1, 2 });
        server.AddDocument(51, "��� ���� ������� �� �������"s, DocumentStatus::ACTUAL, { 1, 3, 1 });
        server.AddDocument(52, "��������� ������ ����"s, DocumentStatus::ACTUAL, { 5, 4, 5 });
        server.AddDocument(53, "����"s, DocumentStatus::BANNED, { 5, 5, 5 });
        server.AddDocument(54, "������ ������ ���� ���� ��� ��� ���"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        const auto found_docs = server.FindTopDocuments("���"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 54);
        const auto found_docs2 = server.FindTopDocuments("������ ����"s, [](int document_id, DocumentStatus status, int rating) { return rating >= 4; });
        ASSERT_EQUAL(found_docs2.size(), 2);
        ASSERT_EQUAL(found_docs2[0].id, 54);
        ASSERT_EQUAL(found_docs2[1].id, 52);
    }
}

//����� ����������, ������� �������� ������.

void TestSearchStatus() {
    const int doc_id = 44;
    const std::string content = "��������� ������ ��� �������������"s;
    const std::vector<int> ratings = { 5, 3, 5 };

    const int doc_id2 = 45;
    const std::string content2 = "��������� ������ ��� ������������� ���������� �������"s;
    const std::vector<int> ratings2 = { 5, 3, 5 };

    const int doc_id3 = 46;
    const std::string content3 = "��������� ������ ��� ������������� ���������� ������� ��� ������ ����������"s;
    const std::vector<int> ratings3 = { 5, 3, 5 };

    const std::vector<int> revelance = { 46, 45, 44 };

    {
        SearchServer server("test"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, ratings3);
        server.AddDocument(47, "�������������� ��������", DocumentStatus::IRRELEVANT, ratings2);
        server.AddDocument(48, "��������� ��������", DocumentStatus::REMOVED, ratings);
        const auto found_docs = server.FindTopDocuments("������ ������� ������"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 44);
        const auto found_docs2 = server.FindTopDocuments("������ ������� ������"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs2.size(), 2);
        ASSERT_EQUAL(found_docs2[0].id, 46);
        const auto found_docs3 = server.FindTopDocuments("��������"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs3.size(), 1);
        ASSERT_EQUAL(found_docs3[0].id, 47);
        const auto found_docs4 = server.FindTopDocuments("��������"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs4.size(), 1);
        ASSERT_EQUAL(found_docs4[0].id, 48);
    }
}

//���������� ���������� ������������� ��������� ����������.

void TestCompRevelance() {
    const double rev = 0;
    const double rev2 = 0.173286;

    {
        SearchServer server("test"s);
        server.AddDocument(50, "������ ���������� ������ �����"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        server.AddDocument(51, "������ ������ ������ �����"s, DocumentStatus::ACTUAL, { 5, 5, 5 });
        const auto found_docs = server.FindTopDocuments("������ ������"s);
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

        // �������� ��������� 2, ����� �����
        search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // ������� ������ � ����-������, ������� ����������
        search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

        // ��������� ���� ����� ��, ������� ���������� ��������� 1
        search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // ���������� ����� �����, ���������� �� ��������
        search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // ��������� ���� ����� ��, ��� � id 6, �������� �� ������ �������, ������� ����������
        search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

        // ���� �� ��� �����, �� �������� ����������
        search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

        // ����� �� ������ ����������, �� �������� ����������
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
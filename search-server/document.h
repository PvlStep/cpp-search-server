#pragma once


enum class DocumentStatus {
    ACTUAL = 0,
    IRRELEVANT = 1,
    BANNED = 3,
    REMOVED = 4,
};

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};


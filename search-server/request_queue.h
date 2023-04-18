#pragma once
#include <stack>
#include <vector>
#include <string>

#include "document.h"
#include "search_server.h"

class RequestQueue {
public:

    RequestQueue(const SearchServer& search_server)
        :server(search_server)
    {
    }
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {

        std::vector<Document> doc = server.FindTopDocuments(raw_query, document_predicate);
        requests_.push_back({ doc.size(), raw_query });
        DequeRequests();
        return doc;     
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        std::vector<Document> doc = server.FindTopDocuments(raw_query, status);
        requests_.push_back({ doc.size(), raw_query});
        DequeRequests();
        return doc;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        std::vector<Document> doc = server.FindTopDocuments(raw_query);
        requests_.push_back({ doc.size(), raw_query });
        DequeRequests();
        return doc;
    }

    int GetNoResultRequests() const;

    int TotalRequets() const;


private:
    struct QueryResult {
        size_t responce_;
        std::string request;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server;

    void DequeRequests();
};


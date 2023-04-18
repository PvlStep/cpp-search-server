#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string_view>> docs;
    std::set<int> duplicate_docs;
    for (int document_id : search_server) {
        std::set<std::string_view> docs_;
        for (auto [word, freq] : search_server.GetWordFrequencies(document_id)) {
            docs_.insert(word);
        }
        if (docs.count(docs_)) {
            duplicate_docs.insert(document_id);
        }
        else {
            docs.insert(docs_);
        }
        docs_.clear();
    }
    for (int id : duplicate_docs) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }

}
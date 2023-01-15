#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> docs;
    std::set<int> duplicate_docs;
    for (int test : search_server) {
        std::set<std::string> docs_;
        for (auto [word, freq] : search_server.GetWordFrequencies(test)) {
            docs_.insert(word);
        }
        if (docs.count(docs_)) {
            duplicate_docs.insert(test);
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
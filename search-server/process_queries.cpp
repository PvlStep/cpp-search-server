#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string& query) {
        return search_server.FindTopDocuments(query);
        });
        
    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::list<Document> result;

    std::vector<std::vector<Document>> q = ProcessQueries(search_server, queries);

    for (std::vector<Document> v : q) {
        for (Document d : v) {
            result.push_back(d);
        }
    }

    
    return result;

}
#pragma once

#include "search_server.h"
#include "document.h"

#include <numeric>
#include <execution>
#include <vector>
#include <string>
#include <algorithm>
#include <list>


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);

/*Ёта задача показывает многообразие возможных решений. —амое простое Ч последовательно объединить векторы из результата. 
Ётого достаточно. —амое необычное Ч использовать контейнер list и в reduce-стадии объедин€ть списки за O(1). 
—амое честное Ч написать обЄртку над вектором векторов со своим итератором, который позвол€ет последовательно перебрать все элементы всех векторов. 
ѕопробуйте на досуге. */
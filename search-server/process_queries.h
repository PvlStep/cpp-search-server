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

/*��� ������ ���������� ������������ ��������� �������. ����� ������� � ��������������� ���������� ������� �� ����������. 
����� ����������. ����� ��������� � ������������ ��������� list � � reduce-������ ���������� ������ �� O(1). 
����� ������� � �������� ������ ��� �������� �������� �� ����� ����������, ������� ��������� ��������������� ��������� ��� �������� ���� ��������. 
���������� �� ������. */
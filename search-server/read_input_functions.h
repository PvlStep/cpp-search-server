#pragma once

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "document.h"
#include "paginator.h"

using namespace std::string_literals;

std::string ReadLine();

int ReadLineWithNumber();

std::ostream& operator<<(std::ostream& out, const Document& doc);

template <typename Iterator>
std::ostream & operator<<(std::ostream & out, const IteratorRange<Iterator> iterator_) {
    for (auto it = iterator_.begin(); it != iterator_.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}
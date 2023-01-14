#pragma once
#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <iostream>

template <typename Iterator>
class IteratorRange {
public:

    IteratorRange(Iterator begin, Iterator end)
        : page_begin(begin), page_end(end) {
    }

    auto begin() const {
        return page_begin;
    }

    auto end() const {
        return page_end;
    }

    auto size() const {
        return size_;
    }

private:
    Iterator page_begin;
    Iterator page_end;
    size_t size_ = page_end - page_begin;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        int number_of_pages = std::ceil(static_cast<double>(distance(begin, end)) / static_cast<double>(page_size));
        if (number_of_pages > 1) {
            for (int page = 1; page != number_of_pages; ++page) {
                if (distance(begin, end) > page_size) {
                    auto page_end = begin + page_size;
                    pages.push_back(IteratorRange<Iterator>(begin, page_end));
                    begin = page_end;
                }
                if (distance(begin, end) <= page_size) {
                    pages.push_back(IteratorRange<Iterator>(begin, end));
                }
            }
        }
        else {
            pages.push_back(IteratorRange<Iterator>(begin, end));
        }
    }


    auto begin() const {
        return pages.begin();
    }

    auto end() const {
        return pages.end();
    }

    auto size() const {
        return pages.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator> iterator_) {
    for (auto it = iterator_.begin(); it != iterator_.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}

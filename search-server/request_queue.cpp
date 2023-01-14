#include "request_queue.h"

    int RequestQueue::GetNoResultRequests() const {
        int empty_requests = 0;
        for (QueryResult result_ : requests_) {
            if (result_.responce_.empty()) {
                ++empty_requests;
            }
        }
        return empty_requests;
    }

    int RequestQueue::TotalRequets() const {
        return requests_.size();
    }

    void RequestQueue::DequeRequests() {
        if (requests_.size() > min_in_day_) {
            requests_.pop_front();
        }

    }
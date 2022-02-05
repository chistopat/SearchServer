#pragma once

#include <algorithm>
#include <vector>


template<class Iterator>
class IteratorRange {
  public:
    IteratorRange() = default;

    IteratorRange(Iterator first, Iterator last)
        : first_(first)
          , last_(last) {
    }

    auto begin() const {
        return first_;
    }

    auto end() const {
        return last_;
    }

    std::size_t size() const {
        return std::distance(first_, last_);
    }

  private:
    Iterator first_;
    Iterator last_;
};

template<typename Iterator>
class Paginator {
  public:
    Paginator(Iterator first, Iterator last, size_t page_size)
        : first_(first)
        , last_(last)
        , page_size_(page_size)
        , pages_(CalculatePagesCount()) {

        auto it = first;
        for (auto& page : pages_) {
            page = IteratorRange{it, next(it, std::min(page_size, static_cast<size_t>(last - it)))};
            it += page_size;
        }
    };

    auto begin() const {
        return pages_.begin();
    };

    auto end() const {
        return pages_.end();
    };

    size_t size() const {
        return pages_.size();
    }

  private:

    size_t CalculatePagesCount() {
        size_t len = std::distance(first_, last_);
        return (len % page_size_) ? len / page_size_ + 1 : len / page_size_;
    }

  private:
    Iterator first_;
    Iterator last_;
    size_t page_size_;
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <class Iterator>
std::ostream& operator<< (std::ostream& os, const IteratorRange<Iterator>& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        os << *it;
    }
    return os;
}

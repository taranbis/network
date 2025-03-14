#ifndef _BYTEARRAY_HEADER_HPP_
#define _BYTEARRAY_HEADER_HPP_ 1
#pragma once

#include <iostream>

#include <assert.h>

namespace rmg
{
namespace util
{

    inline bool contains(std::string_view s, std::string_view pattern)
    {
        if (s.find(pattern) != std::string::npos) return true;
        return false;
    }
}

// template <class Iter = std::random_access_iterator<char>, class Allocator = std::allocator<char> >
class ByteArray
{
public:
    using value_type = char;
    using size_type = std::size_t;
    // using difference_type = std::ptrdiff_t;
    using pointer = char*;
    using const_pointer = const char*;
    using reference = char&;
    using const_reference = const char&;
    using iterator = std::vector<char>::iterator;
    using const_iterator = std::vector<char>::const_iterator;
    using reverse_iterator = std::vector<char>::const_reverse_iterator;
    using const_reverse_iterator = std::vector<char>::const_reverse_iterator;

    ByteArray(ByteArray&& other) = default;
    ByteArray(const ByteArray& other) = default;
    ByteArray() = default;
    ByteArray& operator=(ByteArray&& other) = default;
    ByteArray& operator=(const ByteArray& other) = default;

    ByteArray(std::size_t size, char ch) {}

    ByteArray(const char* str, std::size_t size = -1)
    {
        if (size != -1) {
            data_.reserve(size);
            for (std::size_t i = 0; i < size; i++) data_.emplace_back(str[i]);

        } else {
            for (const char* t = str; *t != '\0'; t++) data_.emplace_back(*t);
        }
    }

    ByteArray(std::vector<char> ba)
    {
        this->data_ = std::move(ba);
    }

    ByteArray(const std::string& str)
    {
        std::copy(str.begin(), str.end(), std::back_inserter(data_));
    }

    std::string toStdString() const
    {
        std::string rv(data_.begin(), data_.end());
        return rv;
    }

    const char* data() const
    {
        return data_.data();
    }

    char* data()
    {
        return data_.data();
    }

    ByteArray& fill(char ch, std::size_t size = -1)
    {
        data_.reserve(size);
        for (int i = 0; i < size; ++i) data_.emplace_back(ch);

        return *this;
    }

    ByteArray& append(const ByteArray& other)
    {
        data_.insert(data_.end(), other.begin(), other.end());
        return *this;
    }

    ByteArray& append(const std::vector<char>& data)
    {
        data_.insert(data_.end(), data.begin(), data.end());
        return *this;
    }

    ByteArray& append(char ch)
    {
        data_.emplace_back(ch);
        return *this;
    }

    ByteArray& append(std::size_t count, char ch)
    {
        for (int i = 0; i < count; ++i) data_.emplace_back(ch);
        return *this;
    }

    ByteArray& append(const char* str)
    {
        for (const char* t = str; *t != '\0'; t++) data_.emplace_back(*t);
        return *this;
    }

    ByteArray& append(const char* str, std::size_t len)
    {
        for (int i = 0; i < len; ++i) data_.emplace_back(str[i]);
        return *this;
    }

    ByteArray& append(ByteArray other)
    {
        data_.insert(data_.end(), other.begin(), other.end());
        return *this;
    }

    char at(std::size_t i) const
    {
        return data_.at(i);
    }

    char back() const
    {
        return data_.back();
    }

    char& back()
    {
        return data_.back();
    }

    ByteArray::iterator begin()
    {
        return data_.begin();
    }

    ByteArray::const_iterator begin() const
    {
        return data_.begin();
    }

    std::size_t capacity() const
    {
        return data_.capacity();
    }

    ByteArray::const_iterator cbegin() const
    {
        return data_.cbegin();
    }

    ByteArray::const_iterator cend() const
    {
        return data_.cend();
    }

    void chop(std::size_t n)
    {
        for (int i = 0; i < n; ++i) data_.pop_back();
    }

    ByteArray chopped(std::size_t len) const
    {
        // The behavior is undefined if len is negative or greater than size().
        ByteArray rv;
        for (int i = 0; i < len; ++i) rv.append(data_[i]);
        return rv;
    }

    void clear()
    {
        data_.clear();
    }

    // int compare(ByteArray bv, Qt::CaseSensitivity cs = Qt::CaseSensitive) const {}

    ByteArray::const_iterator constBegin() const
    {
        return data_.cbegin();
    }

    const char* constData() const
    {
        return data_.data();
    }

    ByteArray::const_iterator constEnd() const
    {
        return data_.cend();
    }

    bool contains(ByteArray other) const
    {
        return util::contains(this->toStdString(), other.toStdString());
    }

    bool contains(char ch) const
    {
        const std::string s(1, ch);
        return util::contains(this->toStdString(), s);
    }

    std::size_t count(const ByteArray& bv) const
    {
        assert(true && "Not YET IMPLEMENTED");
        return 0;
    }

    std::size_t count(char ch) const
    {
        return std::count(data_.begin(), data_.end(), ch);
    }

    std::size_t count() const
    {
        return data_.size();
    }

    ByteArray::const_reverse_iterator crbegin() const
    {
        return data_.crbegin();
    }

    ByteArray::const_reverse_iterator crend() const
    {
        return data_.crend();
    }

    ByteArray::iterator end()
    {
        return data_.end();
    }

    ByteArray::const_iterator end() const
    {
        return data_.end();
    }

    bool endsWith(ByteArray bv) const
    {
        std::string s1 = this->toStdString();
        std::string s2 = bv.toStdString();

        std::reverse(s1.begin(), s1.end());
        std::reverse(s2.begin(), s2.end());

        return s1.starts_with(s2);
    }

    bool endsWith(char ch) const
    {
        return data_.back() == ch;
    }

    ByteArray::iterator erase(ByteArray::const_iterator first, ByteArray::const_iterator last)
    {
        return data_.erase(first, last);
    }

    /**
     * @brief The behavior is undefined when n < 0 or n > size().
     *
     * @param n
     * @return ByteArray
     */
    ByteArray first(std::size_t n) const
    {
        if (n > data_.size()) return ByteArray();

        ByteArray rv;
        for (std::size_t i = 0; i < n; ++i) rv.append(this->at(i));

        return rv;
    }

    char front() const
    {
        return *data_.cbegin();
    }
    char& front()
    {
        return *data_.begin();
    }

    std::size_t indexOf(ByteArray bv, std::size_t from = 0) const {}

    std::size_t indexOf(char ch, std::size_t from = 0) const {}

    /**
     * @brief If i is beyond the end of the array, the array is first extended with space characters to reach this
     * i.
     *
     * @param i
     * @param s
     * @return ByteArray&
     */
    ByteArray& insert(std::size_t i, const char* str)
    {
        std::size_t runningIdx = 0;
        for (const char* t = str; *t != '\0'; t++) {
            this->insert(i + runningIdx, *t);
            runningIdx++;
        }
        return *this;
    }

    ByteArray& insert(std::size_t i, const ByteArray& other)
    {
        std::size_t runningIdx = 0;
        for (auto it = other.cbegin(); it != other.cend(); ++it) {
            this->insert(i + runningIdx, *it);
            runningIdx++;
        }
        return *this;
    }

    ByteArray& insert(std::size_t i, std::size_t count, char ch)
    {
        std::size_t runningIdx = 0;
        for (std::size_t i = 0; i < count; i++) {
            this->insert(i + runningIdx, ch);
            runningIdx++;
        }
        return *this;
    }

    ByteArray& insert(std::size_t i, char ch)
    {
        if (i >= data_.size() - 1) {
            while (i < data_.size() - 1) { data_.push_back(' '); }
            data_.push_back(ch);
        } else {
            std::size_t idx = 0;
            auto it = data_.begin();
            for (; it != data_.end(); ++it) {
                if (idx == i) break;
                idx++;
            }
            data_.insert(it, ch);
        }
        return *this;
    }

    ByteArray& insert(std::size_t i, const char* data, std::size_t len)
    {
        std::size_t runningIdx = 0;
        for (std::size_t i = 0; i < len; i++) {
            this->insert(i + runningIdx, data[i]);
            runningIdx++;
        }
        return *this;
    }

    bool isEmpty() const
    {
        return data_.empty();
    }

    bool isLower() const
    {
        for (auto it = data_.begin(); it != data_.end(); ++it) {
            if (!std::islower(*it)) return false;
        }
        return true;
    }

    bool isNull() const
    {
        const char* p = data_.data();
        if (!p) return true;
        return false;
    }

    bool isUpper() const
    {
        for (auto it = data_.begin(); it != data_.end(); ++it) {
            if (!std::isupper(*it)) return false;
        }
        return true;
    }

    bool isValidUtf8() const {}

    ByteArray last(std::size_t n) const {}

    std::size_t lastIndexOf(ByteArray bv, std::size_t from) const {}
    std::size_t lastIndexOf(char ch, std::size_t from = -1) const {}
    std::size_t lastIndexOf(ByteArray bv) const {}

    ByteArray left(std::size_t len) const {}
    ByteArray leftJustified(std::size_t width, char fill = ' ', bool truncate = false) const {}

    std::size_t length() const
    {
        return data_.size();
    }

    ByteArray mid(std::size_t pos, std::size_t len = -1) const
    {
        if (len != -1) return this->sliced(pos, len);

        ByteArray rv;
        for (std::size_t i = pos; i < this->size(); i++) rv.append(this->at(i));
        return rv;
    }

    ByteArray& prepend(char ch)
    {
        return this->insert(0, ch);
    }

    ByteArray& prepend(std::size_t count, char ch)
    {
        return this->insert(0, count, ch);
    }

    ByteArray& prepend(const char* str)
    {
        return this->insert(0, str);
    }

    ByteArray& prepend(const char* str, std::size_t len)
    {
        return this->insert(0, str, len);
    }

    ByteArray& prepend(const ByteArray& other)
    {
        return this->insert(0, other);
    }

    void push_back(const ByteArray& other)
    {
        data_.insert(data_.end(), other.begin(), other.end());
    }

    void push_back(char ch)
    {
        data_.push_back(ch);
    }

    void push_back(const char* str)
    {
        for (const char* t = str; *t != '\0'; t++) data_.emplace_back(*t);
    }

    void push_front(const ByteArray& other)
    {
        this->insert(0, other);
    }

    void push_front(char ch)
    {
        this->insert(0, ch);
    }

    void push_front(const char* str)
    {
        this->insert(0, str);
    }

    ByteArray::reverse_iterator rbegin()
    {
        return data_.rbegin();
    }

    ByteArray::const_reverse_iterator rbegin() const
    {
        return data_.rbegin();
    }

    /**
     * @brief
     * If pos is out of range, nothing happens.
     * If pos is valid, but pos + len is larger than the size of the array, the array is truncated at position pos.
     *
     * @param pos
     * @param len
     * @return ByteArray&
     */
    ByteArray& remove(std::size_t pos, std::size_t len)
    {
        if (pos > this->size()) return *this;

        if (pos + len > this->size()) {
            for (std::size_t i = pos; i < this->size(); ++i) data_.pop_back();

            return *this;
        }

        ByteArray rv;
        for (std::size_t i = 0; i < pos; ++i) rv.append(this->at(i));
        for (std::size_t i = pos + len; i < this->size(); ++i) rv.append(this->at(i));

        data_ = std::move(rv.data_);

        return *this;
    }

    // ByteArray& removeIf(Predicate pred) {}

    ByteArray::reverse_iterator rend()
    {
        return data_.rend();
    }

    ByteArray::const_reverse_iterator rend() const
    {
        return data_.crend();
    }

    ByteArray repeated(std::size_t times) const
    {
        ByteArray rv;
        for (int i = 0; i < times; ++i) { rv << data_; }

        return rv;
    }

    ByteArray& replace(std::size_t pos, std::size_t len, ByteArray after) {}
    ByteArray& replace(std::size_t pos, std::size_t len, const char* after, std::size_t alen) {}
    ByteArray& replace(char before, ByteArray after) {}
    ByteArray& replace(const char* before, std::size_t bsize, const char* after, std::size_t asize) {}
    ByteArray& replace(ByteArray before, ByteArray after) {}
    ByteArray& replace(char before, char after) {}

    void reserve(std::size_t size)
    {
        data_.reserve(size);
    }

    void resize(std::size_t size)
    {
        data_.resize(size);
    }

    ByteArray right(std::size_t len) const {}
    ByteArray rightJustified(std::size_t width, char fill = ' ', bool truncate = false) const {}

    /**
     * @brief Represent the whole number n as text.
     *
     * Sets this byte array to a string representing n in base base (ten by default) and returns a reference to
     * this byte array. Bases 2 through 36 are supported, using letters for digits beyond 9; A is ten, B is eleven
     * and so on.
     *
     * Example:
     *
     * QByteArray ba
     * int n = 63
     * ba.setNum(n);           // ba == "63
     * ba.setNum(n, 16);       // ba == "3f"
     */
    ByteArray& setNum(int n, int base = 10) {}
    ByteArray& setNum(short n, int base = 10) {}
    ByteArray& setNum(long n, int base = 10) {}
    ByteArray& setNum(float n, char format = 'g', int precision = 6) {}
    ByteArray& setNum(double n, char format = 'g', int precision = 6) {}

    /**
     * @brief Set the Raw Data object
     *
     * Resets the ByteArray to use the first size bytes of the data array. The bytes ARE copied.
     */
    ByteArray& setRawData(const char* data, std::size_t size)
    {
        data_.clear();

        for (std::size_t i = 0; i < size; ++i) data_.emplace_back(data[i]);

        return *this;
    }

    void shrink_to_fit()
    {
        data_.shrink_to_fit();
    }

    ByteArray simplified() const {}

    std::size_t size() const
    {
        return data_.size();
    }

    ByteArray sliced(std::size_t pos, std::size_t n) const
    {
        ByteArray rv;

        if (pos + n > this->size()) return ByteArray();

        for (std::size_t i = pos; i < pos + n; ++i) rv.append(this->at(i));

        return rv;
    }

    ByteArray sliced(std::size_t pos) const
    {
        ByteArray rv;

        if (pos > this->size()) return ByteArray();

        for (std::size_t i = pos; i < data_.size(); ++i) rv.append(this->at(i));

        return rv;
    }

    // TODO: push_back here is not correct
    std::vector<ByteArray> split(char sep) const
    {
        std::vector<ByteArray> rv;

        std::string str = this->toStdString();

        std::size_t current, previous = 0;
        current = str.find_first_of(sep);
        while (current != std::string::npos) {
            rv.push_back(str.substr(previous, current - previous));
            previous = current + 1;
            current = str.find_first_of(sep, previous);
        }
        rv.push_back(str.substr(previous, current - previous));

        return rv;
    }

    void squeeze()
    {
        data_.shrink_to_fit();
    }

    bool startsWith(ByteArray bv) const
    {
        return this->toStdString().starts_with(bv.toStdString());
    }

    bool startsWith(char ch) const
    {
        const std::string s(1, ch);
        return this->toStdString().starts_with(s);
    }

    void swap(ByteArray& other)
    {
        std::swap(this->data_, other.data_);
    }

    // ByteArray toBase64(ByteArray::Base64Options options = Base64Encoding) const {}
    // CFDataRef toCFData() const {}W
    double toDouble(bool* ok = nullptr) const {}
    float toFloat(bool* ok = nullptr) const {}
    ByteArray toHex(char separator = '\0') const {}
    int toInt(bool* ok = nullptr, int base = 10) const {}
    long toLong(bool* ok = nullptr, int base = 10) const {}
    uint64_t toLongLong(bool* ok = nullptr, int base = 10) const {}

    ByteArray toLower() const
    {
        ByteArray rv;
        for (const auto& val : data_) rv.append(std::tolower(val));
        return rv;
    }

    // NSData* toNSData() const {}
    ByteArray toPercentEncoding(const ByteArray& exclude = ByteArray(), const ByteArray& include = ByteArray(),
                                char percent = '%') const
    {
    }

    // CFDataRef toRawCFData() const {}
    // NSData* toRawNSData() const {}
    short toShort(bool* ok = nullptr, int base = 10) const {}
    uint32_t toULong(bool* ok = nullptr, int base = 10) const {}
    uint64_t toULongLong(bool* ok = nullptr, int base = 10) const {}

    ByteArray toUpper() const
    {
        ByteArray rv;
        for (const auto& val : data_) rv.append(std::toupper(val));
        return rv;
    }

    ByteArray trimmed() const
    {
        ByteArray rv;
        for (const auto& val : data_) {
            if (!std::isspace(val)) { rv.append(val); }
        }

        while (std::isspace(rv.data_.back())) rv.data_.pop_back();

        return rv;
    }

    void truncate(std::size_t pos)
    {
        if (pos >= data_.size()) return;

        std::vector<char> tmp;
        for (std::size_t i = 0; i < pos; i++) tmp.push_back(this->at(i));
        data_ = std::move(tmp);
    }

    bool operator!=(const std::string& str) const
    {
        return this->toStdString() != str;
    }

    ByteArray& operator+=(const ByteArray& other)
    {
        data_.insert(data_.end(), other.begin(), other.end());
        return *this;
    }

    ByteArray& operator+=(char ch)
    {
        data_.emplace_back(ch);
        return *this;
    }

    ByteArray& operator+=(const char* str)
    {
        for (const char* t = str; *t != '\0'; t++) data_.emplace_back(*t);
        return *this;
    }

    bool operator<(const std::string& str) const
    {
        return this->toStdString() < str;
    }

    bool operator<=(const std::string& str) const
    {
        return this->toStdString() <= str;
    }

    ByteArray& operator=(const char* str)
    {
        this->data_.clear();
        for (const char* t = str; *t != '\0'; t++) data_.emplace_back(*t);
        return *this;
    }

    bool operator==(const std::string& str) const
    {
        return this->toStdString() == str;
    }

    bool operator>(const std::string& str) const
    {
        return this->toStdString() > str;
    }

    bool operator>=(const std::string& str) const
    {
        return this->toStdString() >= str;
    }

    // auto operator<=>(const ByteArray&) const = default;

    char& operator[](std::size_t i)
    {
        return data_[i];
    }

    char operator[](std::size_t i) const
    {
        return data_[i];
    }

    friend std::ostream& operator<<(std::ostream& os, const ByteArray& other)
    {
        for (std::size_t i = 0; i < other.data_.size(); i++) os << other.data_[i];
        return os;
    }

    ByteArray& operator<<(const ByteArray& other)
    {
        data_.insert(data_.end(), other.begin(), other.end());
        return *this;
    }

    ByteArray& operator<<(const std::vector<char>& data)
    {
        data_.insert(data_.end(), data.begin(), data.end());
        return *this;
    }

private:
    std::vector<char> data_;
    // bool isBinary_ {false};
};

} // namespace rmg

#endif //!_BYTEARRAY_HEADER_HPP_
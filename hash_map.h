#pragma once

#include <iostream>
#include <list>
#include <vector>
#include <memory>

// We use Hopscotch hashing as an internal algorithm for the HashMap class
// You can read more about it here: http://mcg.cs.tau.ac.il/papers/disc2008-hopscotch.pdf

namespace {
    const size_t NEXT = 32;
    const size_t INITIAL_SIZE = 32;
    const long double MIN_LOAD_FACTOR = 0.1;
    const long double MAX_LOAD_FACTOR = 0.5;

    template<class KeyType, class ValueType>
    class Bucket {
        using StorageType = std::pair<const KeyType, ValueType>;
    public:
        Bucket() = default;

        explicit Bucket(const StorageType &pair) : pair_(pair), is_occupied_(true) {};

        Bucket(const Bucket &other) {
            operator=(other);
        }

        Bucket &operator=(const Bucket &other) {
            if (other.is_occupied_) {
                auto ref = RootRef();
                ref.first = other.pair_.first;
                ref.second = other.pair_.second;
            }
            is_occupied_ = other.is_occupied_;
            return *this;
        }

        bool IsOccupied() const {
            return is_occupied_;
        }

        bool HasKey(const KeyType &other_key) const {
            return IsOccupied() && pair_.first == other_key;
        }

        void Set(const std::pair<KeyType, ValueType> &new_pair) {
            operator=(Bucket({new_pair.first, new_pair.second}));
        }

        void Erase() {
            is_occupied_ = false;
        }

        StorageType &GetRef() {
            return pair_;
        }

        const StorageType &GetRef() const {
            return pair_;
        }

    private:
        auto RootRef() {
            return std::pair<KeyType &, ValueType &>(const_cast<KeyType &>(pair_.first), pair_.second);
        }

        StorageType pair_;
        bool is_occupied_ = false;
    };

    template<class KeyType, class ValueType, class Hash>
    class BucketArray {
        using PairType = std::pair<const KeyType, ValueType>;
    public:
        using BArrayType = std::vector<Bucket<KeyType, ValueType>>;

        BucketArray(size_t size, Hash hash) : array_(size + NEXT - 1), pairs_count_(0), hash_func_(hash) {};

        BucketArray &operator=(const BucketArray &other) {  // with the same hash func as before, only for Reconstruct()
            array_ = other.array_;
            pairs_count_ = other.pairs_count_;
            return *this;
        }

        auto FirstOccupiedBucket(typename BArrayType::iterator iter) {
            for (auto it = iter; it != End(); ++it) {
                if (it->IsOccupied()) return it;
            }
            return End();
        }

        auto FirstOccupiedBucket(typename BArrayType::const_iterator iter) const {
            for (auto it = iter; it != End(); ++it) {
                if (it->IsOccupied()) return it;
            }
            return End();
        }

        size_t GetIndex(const KeyType &key) const {
            size_t length = array_.size() - NEXT + 1;
            return hash_func_(key) % length;
        }

        std::pair<bool, PairType &> Insert(const PairType &pair) {
            PairType fake_pair;
            std::pair<bool, PairType &> result = {false, fake_pair};
            size_t arr_index = GetIndex(pair.first);
            size_t empty_bucket = arr_index;
            for (; empty_bucket < array_.size(); ++empty_bucket) {
                if (!array_[empty_bucket].IsOccupied()) {
                    break;
                }
            }
            if (empty_bucket == array_.size()) {
                return result;
            } else {
                bool swapped = true;
                while (arr_index + NEXT <= empty_bucket) {
                    swapped = false;
                    for (size_t pot_bucket = empty_bucket - NEXT + 1; pot_bucket < empty_bucket; ++pot_bucket) {
                        size_t ideal_index = GetIndex(array_[pot_bucket].GetRef().first);
                        if (empty_bucket < ideal_index + NEXT) {
                            std::swap(array_[pot_bucket], array_[empty_bucket]);
                            empty_bucket = pot_bucket;
                            swapped = true;
                        }
                    }
                    if (!swapped) {
                        break;
                    }
                }
                if (swapped) {
                    array_[empty_bucket].Set(pair);
                    ++pairs_count_;
                    return {true, array_[empty_bucket].GetRef()};
                } else {
                    return result;
                }
            }
        }

        bool Erase(const KeyType &key) {
            auto it = Find(key);
            if (it != array_.end()) {
                it->Erase();
                --pairs_count_;
                return true;
            } else {
                return false;
            }
        }

        auto Find(const KeyType &key) const {
            size_t arr_index = GetIndex(key);
            for (size_t i = arr_index; i < arr_index + NEXT; ++i) {
                if (array_[i].HasKey(key)) {
                    return array_.begin() + i;
                }
            }
            return array_.end();
        }

        auto Find(const KeyType &key) {
            size_t arr_index = GetIndex(key);
            for (size_t i = arr_index; i < arr_index + NEXT; ++i) {
                if (array_[i].HasKey(key)) {
                    return array_.begin() + i;
                }
            }
            return array_.end();
        }

        size_t PairsCount() const {
            return pairs_count_;
        }

        size_t ArraySize() const {
            return array_.size();
        }

        auto Begin() {
            return array_.begin();
        }

        auto End() {
            return array_.end();
        }

        auto Begin() const {
            return array_.begin();
        }

        auto End() const {
            return array_.end();
        }

        long double LoadFactor() {
            return static_cast<long double>(pairs_count_) / array_.size();
        }

    private:
        BArrayType array_;
        size_t pairs_count_;
        Hash hash_func_;
    };
}

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
    using PairType = std::pair<const KeyType, ValueType>;
    using BucketType = Bucket<KeyType, ValueType>;
    using BArray = BucketArray<KeyType, ValueType, Hash>;
    using ListType = std::vector<BucketType>;
    using BucketArrayIterator = typename BArray::BArrayType::iterator;
    using BucketArrayConstIterator = typename BArray::BArrayType::const_iterator;
    using ListIterator = typename ListType::iterator;
    using ListConstIterator = typename ListType::const_iterator;
public:

    explicit HashMap(const Hash &hash = Hash()) : hash_func_(hash), b_array_(INITIAL_SIZE, hash) {};

    HashMap(std::initializer_list<PairType> init_list, const Hash &hash = Hash())
            : HashMap(init_list.begin(), init_list.end(), hash) {};

    template<class InputIt>
    HashMap(InputIt first, InputIt second, const Hash &hash = Hash()) : hash_func_(hash), b_array_(INITIAL_SIZE, hash) {
        for (auto it = first; it != second; ++it) {
            insert(*it);
        }
    };

    size_t size() const {
        return b_array_.PairsCount() + list_.size();
    }

    bool empty() const {
        return size() == 0;
    }

    auto hash_function() const {
        return hash_func_;
    }

    void insert(const PairType &pair) {
        if (find(pair.first) == end()) {
            ForceInsert(pair);
        }
    }

    void erase(const KeyType &key) {
        if (!b_array_.Erase(key)) {
            for (auto it = list_.begin(); it != list_.end(); ++it) {
                if (it->GetRef().first == key) {
                    list_.erase(it);
                    return;
                }
            }
        }
    }

    template<bool IsConst>
    class RawIterator {
        using ReturnType = std::conditional_t<IsConst, const PairType, PairType>;
        using BArrayIter = std::conditional_t<IsConst, BucketArrayConstIterator, BucketArrayIterator>;
        using ListIter = std::conditional_t<IsConst, ListConstIterator, ListIterator>;
    public:
        RawIterator(BArrayIter array_begin, BArrayIter array_end, ListIter list_iter, ListIter list_end)
                : b_array_iter_(
                array_begin), b_array_end_(array_end), list_iter_(list_iter), list_end_(list_end) {};

        RawIterator() = default;

        auto operator++() {
            if (b_array_iter_ != b_array_end_) {
                while (b_array_iter_ != b_array_end_) {
                    ++b_array_iter_;
                    if (b_array_iter_ != b_array_end_ && b_array_iter_->IsOccupied()) {
                        return *this;
                    }
                }
                return *this;
            } else {
                ++list_iter_;
            }
            return *this;
        }

        auto operator++(int) {
            const RawIterator before = *this;
            operator++();
            return before;
        }

        friend bool operator==(const RawIterator &first, const RawIterator &second) {
            return first.b_array_iter_ == second.b_array_iter_ && first.list_iter_ == second.list_iter_;
        }

        friend bool operator!=(const RawIterator &first, const RawIterator &second) {
            return !operator==(first, second);
        }

        ReturnType &operator*() {
            if (b_array_iter_ != b_array_end_) {
                return b_array_iter_->GetRef();
            } else {
                return list_iter_->GetRef();
            }
        }

        ReturnType *operator->() {
            return &operator*();
        }

    private:
        BArrayIter b_array_iter_;
        BArrayIter b_array_end_;
        ListIter list_iter_;
        ListIter list_end_;
    };

    using iterator = RawIterator<false>;
    using const_iterator = RawIterator<true>;

    iterator begin() {
        return {b_array_.FirstOccupiedBucket(b_array_.Begin()), b_array_.End(), list_.begin(), list_.end()};
    }

    iterator end() {
        return {b_array_.End(), b_array_.End(), list_.end(), list_.end()};
    }

    const_iterator begin() const {
        return {b_array_.FirstOccupiedBucket(b_array_.Begin()), b_array_.End(), list_.begin(), list_.end()};
    }

    const_iterator end() const {
        return const_iterator(b_array_.End(), b_array_.End(), list_.end(), list_.end());
    }

    iterator find(const KeyType &key) {
        BucketArrayIterator it = b_array_.Find(key);
        if (it != b_array_.End()) {
            return iterator(it, b_array_.End(), list_.begin(), list_.end());
        } else {
            for (auto iter = list_.begin(); iter != list_.end(); ++iter) {
                if (iter->GetRef().first == key) {
                    return iterator(b_array_.End(), b_array_.End(), iter, list_.end());
                }
            }
            return iterator(b_array_.End(), b_array_.End(), list_.end(), list_.end());
        }
    }

    const_iterator find(const KeyType &key) const {
        auto it = b_array_.Find(key);
        if (it != b_array_.End()) {
            return {it, b_array_.End(), list_.begin(), list_.end()};
        } else {
            for (auto iter = list_.begin(); iter != list_.end(); ++iter) {
                if (iter->GetRef().first == key) {
                    return {b_array_.End(), b_array_.End(), iter, list_.end()};
                }
            }
            return {b_array_.End(), b_array_.End(), list_.end(), list_.end()};
        }
    }

    ValueType &operator[](const KeyType &key) {
        auto it = find(key);
        if (it == end()) {
            auto new_pair = std::make_pair(key, ValueType());
            return ForceInsert(new_pair).second;
        } else {
            return it->second;
        }
    }

    const ValueType &at(const KeyType &key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Key was not found");
        } else {
            return it->second;
        }
    }

    void clear() {
        Reconstruct(false, true);
    }

private:
    PairType &ForceInsert(const PairType &pair) {
        auto load_factor = b_array_.LoadFactor();
        if (load_factor > MAX_LOAD_FACTOR || load_factor < MIN_LOAD_FACTOR) {
            Reconstruct();
        }
        auto [success, ref] = b_array_.Insert(pair);
        if (!success) {
            return list_.emplace_back(pair).GetRef();
        } else {
            return ref;
        }
    }

    void Reconstruct(bool change_size = true, bool clear = false) {
        size_t new_size = b_array_.ArraySize() - NEXT + 1;
        if (change_size) {
            if (b_array_.LoadFactor() < MIN_LOAD_FACTOR) {
                new_size /= 2;
            } else {
                new_size *= 2;
            }
        }
        new_size = std::max(new_size, INITIAL_SIZE);
        BucketArray<KeyType, ValueType, Hash> tmp_map(new_size, hash_func_);
        ListType tmp_list;
        if (!clear) {
            for (const auto &bucket: *this) {
                if (!tmp_map.Insert(bucket).first) {
                    tmp_list.emplace_back(bucket);
                }
            }
        }
        b_array_ = tmp_map;
        list_ = tmp_list;
    }

    Hash hash_func_;
    BucketArray<KeyType, ValueType, Hash> b_array_;
    ListType list_;
};

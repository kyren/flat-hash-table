#pragma once

#include <vector>
#include <utility>

namespace flat_hash {

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
struct hash_table {
private:
  static size_t const NPos = (size_t)-1;
  static size_t const EmptyHashValue = 0;
  static size_t const EndHashValue = 1;
  static size_t const FilledHashBit = (size_t)1 << (sizeof(size_t) * 8 - 1);

  struct Bucket {
    Bucket();
    ~Bucket();

    Bucket(Bucket const& rhs);
    Bucket(Bucket&& rhs);

    Bucket& operator=(Bucket const& rhs);
    Bucket& operator=(Bucket&& rhs);

    void setFilled(size_t hash, Value value);
    void setEmpty();
    void setEnd();

    Value const* valuePtr() const;
    Value* valuePtr();
    bool isEmpty() const;
    bool isEnd() const;

    size_t hash;
    union {
      Value value;
    };
  };

  typedef std::vector<Bucket, typename Allocator::template rebind<Bucket>::other> Buckets;

public:
  struct const_iterator {
    bool operator==(const_iterator const& rhs) const;
    bool operator!=(const_iterator const& rhs) const;

    const_iterator& operator++();
    const_iterator operator++(int);

    Value const& operator*() const;
    Value const* operator->() const;

    typename Buckets::const_iterator current;
  };

  struct iterator {
    bool operator==(iterator const& rhs) const;
    bool operator!=(iterator const& rhs) const;

    iterator& operator++();
    iterator operator++(int);

    Value& operator*() const;
    Value* operator->() const;

    operator const_iterator() const;

    typename Buckets::iterator current;
  };

  hash_table(size_t bucketCount, GetKey const& getKey, Hash const& hash, Equals const& equal, Allocator const& alloc);

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  size_t empty() const;
  size_t size() const;
  void clear();

  std::pair<iterator, bool> insert(Value const& value);
  std::pair<iterator, bool> insert(Value&& value);

  iterator erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);

  const_iterator find(Key const& key) const;
  iterator find(Key const& key);

  double maxFillLevel() const;
  void setMaxFillLevel(double maxFillLevel);

  void reserve(size_t capacity);
  Allocator getAllocator() const;

  bool operator==(hash_table const& rhs) const;
  bool operator!=(hash_table const& rhs) const;

private:
  static constexpr size_t MinCapacity = 8;
  static constexpr double DefaultMaxFillLevel = 0.7;

  // Scans for the next bucket value that is non-empty
  static typename Buckets::iterator scanIterator(typename Buckets::iterator i);
  static typename Buckets::const_iterator scanIterator(typename Buckets::const_iterator i);

  size_t hashBucket(size_t hash) const;
  size_t bucketError(size_t current, size_t target) const;
  void checkCapacity(size_t additionalCapacity);
  std::pair<iterator, bool> doInsert(Value value);

  Buckets m_buckets;
  double m_maxFillLevel;
  size_t m_filledCount;

  GetKey m_getKey;
  Hash m_hash;
  Equals m_equals;
};

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::Bucket() {
  this->hash = EmptyHashValue;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::~Bucket() {
  if (auto s = valuePtr())
    s->~Value();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::Bucket(Bucket const& rhs) {
  this->hash = rhs.hash;
  if (auto o = rhs.valuePtr())
    new (&this->value) Value(*o);
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::Bucket(Bucket&& rhs) {
  this->hash = rhs.hash;
  if (auto o = rhs.valuePtr())
    new (&this->value) Value(std::move(*o));
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::operator=(Bucket const& rhs) -> Bucket& {
  if (auto o = rhs.valuePtr()) {
    if (auto s = valuePtr())
      *s = *o;
    else
      new (&this->value) Value(*o);
  } else {
    if (auto s = valuePtr())
      s->~Value();
  }
  this->hash = rhs.hash;
  return *this;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::operator=(Bucket&& rhs) -> Bucket& {
  if (auto o = rhs.valuePtr()) {
    if (auto s = valuePtr())
      *s = std::move(*o);
    else
      new (&this->value) Value(std::move(*o));
  } else {
    if (auto s = valuePtr())
      s->~Value();
  }
  this->hash = rhs.hash;
  return *this;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
void hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::setFilled(size_t hash, Value value) {
  if (auto s = valuePtr())
    *s = std::move(value);
  else
    new (&this->value) Value(std::move(value));
  this->hash = hash | FilledHashBit;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
void hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::setEmpty() {
  if (auto s = valuePtr())
    s->~Value();
  this->hash = EmptyHashValue;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
void hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::setEnd() {
  if (auto s = valuePtr())
    s->~Value();
  this->hash = EndHashValue;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
Value const* hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::valuePtr() const {
  if (hash & FilledHashBit)
    return &this->value;
  return nullptr;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
Value* hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::valuePtr() {
  if (hash & FilledHashBit)
    return &this->value;
  return nullptr;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::isEmpty() const {
  return this->hash == EmptyHashValue;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::Bucket::isEnd() const {
  return this->hash == EndHashValue;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::const_iterator::operator==(const_iterator const& rhs) const {
  return current == rhs.current;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::const_iterator::operator!=(const_iterator const& rhs) const {
  return current != rhs.current;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::const_iterator::operator++() -> const_iterator& {
  current = hash_table::scanIterator(++current);
  return *this;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::const_iterator::operator++(int) -> const_iterator {
  const_iterator copy(*this);
  operator++();
  return copy;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::const_iterator::operator*() const -> Value const& {
  return *operator->();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::const_iterator::operator->() const -> Value const* {
  return current->valuePtr();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::iterator::operator==(iterator const& rhs) const {
  return current == rhs.current;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::iterator::operator!=(iterator const& rhs) const {
  return current != rhs.current;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::iterator::operator++() -> iterator& {
  current = hash_table::scanIterator(++current);
  return *this;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::iterator::operator++(int) -> iterator {
  iterator copy(*this);
  operator++();
  return copy;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::iterator::operator*() const -> Value& {
  return *operator->();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::iterator::operator->() const -> Value* {
  return current->valuePtr();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::iterator::operator typename hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::const_iterator() const {
  return const_iterator{current};
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::hash_table(size_t bucketCount,
    GetKey const& getKey, Hash const& hash, Equals const& equal, Allocator const& alloc)
  : m_buckets(alloc), m_maxFillLevel(DefaultMaxFillLevel), m_filledCount(0), m_getKey(getKey),
    m_hash(hash), m_equals(equal) {
  if (bucketCount != 0)
    checkCapacity(bucketCount);
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::begin() -> iterator {
  if (m_buckets.empty())
    return end();
  return iterator{scanIterator(m_buckets.begin())};
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::end() -> iterator {
  return iterator{--m_buckets.end()};
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::begin() const -> const_iterator {
  return const_cast<hash_table*>(this)->begin();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::end() const -> const_iterator {
  return const_cast<hash_table*>(this)->end();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
size_t hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::empty() const {
  return m_filledCount == 0;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
size_t hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::size() const {
  return m_filledCount;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
void hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::clear() {
  if (m_buckets.empty())
    return;

  for (size_t i = 0; i < m_buckets.size() - 1; ++i)
    m_buckets[i].setEmpty();
  m_filledCount = 0;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::insert(Value const& value) -> std::pair<iterator, bool> {
  return doInsert(Value(value));
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::insert(Value&& value) -> std::pair<iterator, bool> {
  return doInsert(std::move(value));
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::erase(const_iterator pos) -> iterator {
  size_t bucketIndex = pos.current - m_buckets.begin();
  size_t currentBucketIndex = bucketIndex;
  auto currentBucket = &m_buckets[currentBucketIndex];

  while (true) {
    size_t nextBucketIndex = hashBucket(currentBucketIndex + 1);
    auto nextBucket = &m_buckets[nextBucketIndex];
    if (auto nextPtr = nextBucket->valuePtr()) {
      if (bucketError(nextBucketIndex, nextBucket->hash) > 0) {
        currentBucket->hash = nextBucket->hash;
        *currentBucket->valuePtr() = std::move(*nextPtr);
        currentBucketIndex = nextBucketIndex;
        currentBucket = nextBucket;
      } else {
        break;
      }
    } else {
      break;
    }
  }

  m_buckets[currentBucketIndex].setEmpty();
  --m_filledCount;

  return iterator{scanIterator(m_buckets.begin() + bucketIndex)};
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::erase(const_iterator first, const_iterator last) -> iterator {
  while (first != last)
    first = erase(first);
  return iterator{m_buckets.begin() + (first.current - m_buckets.begin())};
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::find(Key const& key) const -> const_iterator {
  return const_cast<hash_table*>(this)->find(key);
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::find(Key const& key) -> iterator {
  if (m_buckets.empty())
    return end();

  size_t hash = m_hash(key) | FilledHashBit;
  size_t targetBucket = hashBucket(hash);
  size_t currentBucket = targetBucket;
  while (true) {
    auto& bucket = m_buckets[currentBucket];
    if (auto value = bucket.valuePtr()) {
      if (bucket.hash == hash && m_equals(m_getKey(*value), key))
        return iterator{m_buckets.begin() + currentBucket};

      size_t entryError = bucketError(currentBucket, bucket.hash);
      size_t findError = bucketError(currentBucket, targetBucket);

      if (findError > entryError)
        return end();

      currentBucket = hashBucket(currentBucket + 1);

    } else {
      return end();
    }
  }
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
double hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::maxFillLevel() const {
  return m_maxFillLevel;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
void hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::setMaxFillLevel(double maxFillLevel) {
  m_maxFillLevel = maxFillLevel;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
void hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::reserve(size_t capacity) {
  if (capacity > m_filledCount)
    checkCapacity(capacity - m_filledCount);
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
Allocator hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::getAllocator() const {
  return m_buckets.get_allocator();
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::operator==(hash_table const& rhs) const {
  if (size() != rhs.size())
    return false;

  auto i = begin();
  auto j = rhs.begin();
  auto e = end();

  while (i != e) {
    if (*i != *j)
      return false;
    ++i;
    ++j;
  }

  return true;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
bool hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::operator!=(hash_table const& rhs) const {
  return !operator==(rhs);
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
constexpr size_t hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::MinCapacity;

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
constexpr double hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::DefaultMaxFillLevel;

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::scanIterator(typename Buckets::iterator i) -> typename Buckets::iterator {
  while (i->isEmpty())
    ++i;
  return i;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::scanIterator(typename Buckets::const_iterator i) -> typename Buckets::const_iterator {
  while (i->isEmpty())
    ++i;
  return i;
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
size_t hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::hashBucket(size_t hash) const {
  return hash & (m_buckets.size() - 2);
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
size_t hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::bucketError(size_t current, size_t target) const {
  return hashBucket(current - target);
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
void hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::checkCapacity(size_t additionalCapacity) {
  size_t newSize;
  if (!m_buckets.empty())
    newSize = m_buckets.size() - 1;
  else
    newSize = MinCapacity;

  while ((double)(m_filledCount + additionalCapacity) / (double)newSize > m_maxFillLevel)
    newSize *= 2;

  if (newSize == m_buckets.size() - 1)
    return;

  Buckets oldBuckets;
  swap(m_buckets, oldBuckets);

  // Leave an extra end entry when allocating buckets, so that iterators are
  // simpler and can simply iterate until they find something that is not an
  // empty entry.
  m_buckets.resize(newSize + 1);
  m_buckets[newSize].setEnd();

  m_filledCount = 0;

  for (auto& entry : oldBuckets) {
    if (auto ptr = entry.valuePtr())
      insert(std::move(*ptr));
  }
}

template <typename Value, typename Key, typename GetKey, typename Hash, typename Equals, typename Allocator>
auto hash_table<Value, Key, GetKey, Hash, Equals, Allocator>::doInsert(Value value) -> std::pair<iterator, bool> {
  checkCapacity(1);

  size_t hash = m_hash(m_getKey(value)) | FilledHashBit;
  size_t targetBucket = hashBucket(hash);
  size_t currentBucket = targetBucket;
  size_t insertedBucket = NPos;

  while (true) {
    auto& target = m_buckets[currentBucket];
    if (auto entryValue = target.valuePtr()) {
      if (target.hash == hash && m_equals(m_getKey(*entryValue), m_getKey(value)))
        return std::make_pair(iterator{m_buckets.begin() + currentBucket}, false);

      size_t entryTargetBucket = hashBucket(target.hash);
      size_t entryError = bucketError(currentBucket, entryTargetBucket);
      size_t addError = bucketError(currentBucket, targetBucket);
      if (addError > entryError) {
        if (insertedBucket == NPos)
          insertedBucket = currentBucket;

        std::swap(value, *entryValue);
        std::swap(hash, target.hash);
        targetBucket = entryTargetBucket;
      }
      currentBucket = hashBucket(currentBucket + 1);

    } else {
      target.setFilled(hash, std::move(value));
      ++m_filledCount;
      if (insertedBucket == NPos)
        insertedBucket = currentBucket;

      return std::make_pair(iterator{m_buckets.begin() + insertedBucket}, true);
    }
  }
}

}

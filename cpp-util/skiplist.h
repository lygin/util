/* Rocksdb skiplist */
// thread safty:
// Writes require external synchronization, most likely a mutex.
// Reads require a guarantee that the SkipList will not be destroyed
// while the read is in progress.  Apart from that, reads progress
// without any internal locking or synchronization.
#pragma once
#include <assert.h>
#include <stdlib.h>
#include <atomic>

template<typename K, class Cmp>
class SkipList {
public:
	class Iterator;
	explicit SkipList(Cmp cmp,int32_t max_height=12, int32_t branching_factor=4):
		kMaxHeight_(max_height), 
		kBranching_(branching_factor),
		kScaledInverseBranching_(2147483647L/kBranching_),
		compare_(cmp),
		head_(NewNode(0, max_height)),
		max_height_(1),
		prev_height_(1) {
			assert(max_height > 0 && kMaxHeight_ == static_cast<uint32_t>(max_height));
			assert(branching_factor > 0 && kBranching_ == static_cast<uint32_t>(branching_factor));
			assert(kScaledInverseBranching_ > 0);
			prev_ = (Node**)malloc(sizeof(Node*) * kMaxHeight_);
			for(int i=0; i<kMaxHeight_; i++) {
				head_->SetNext(i, nullptr);
				prev_[i] = head_;
			}
	}
		
	SkipList(const SkipList& other) = delete;
	void operator=(const SkipList& other) = delete;

	void Insert(const K& key) {
		// fast path for sequential insertion
		if (!KeyIsAfterNode(key, prev_[0]->NoBarrier_Next(0)) &&
				(prev_[0] == head_ || KeyIsAfterNode(key, prev_[0]))) {
			assert(prev_[0] != head_ || (prev_height_ == 1 && GetMaxHeight() == 1));

			for (int i = 1; i < prev_height_; i++) {
				prev_[i] = prev_[0];
			}
		} else {
			FindLessThan(key, prev_);
		}

		// Our data structure does not allow duplicate insertion
		assert(prev_[0]->Next(0) == nullptr || !Equal(key, prev_[0]->Next(0)->key));

		int height = RandomHeight();
		if (height > GetMaxHeight()) {
			for (int i = GetMaxHeight(); i < height; i++) {
				prev_[i] = head_;
			}

			// It is ok to mutate max_height_ without any synchronization
			// with concurrent readers. 
			max_height_.store(height, std::memory_order_relaxed);
		}

		Node* x = NewNode(key, height);
		for (int i = 0; i < height; i++) {
			x->NoBarrier_SetNext(i, prev_[i]->NoBarrier_Next(i));
			prev_[i]->SetNext(i, x);
		}
		prev_[0] = x;
		prev_height_ = height;
	}

	bool Contains(const K& key) const {
		Node* x = FindGreaterOrEqual(key);
		if (x != nullptr && Equal(key, x->key)) {
			return true;
		} else {
			return false;
		}
	}

	uint64_t EstimateCount(const K& key) const {
		uint64_t count = 0;

		Node* x = head_;
		int level = GetMaxHeight() - 1;
		while (true) {
			assert(x == head_ || compare_(x->key, key) < 0);
			Node* next = x->Next(level);
			if (next == nullptr || compare_(next->key, key) >= 0) {
				if (level == 0) {
					return count;
				} else {
					// Switch to next list
					count *= kBranching_;
					level--;
				}
			} else {
				x = next;
				count++;
			}
		}
	}
	private:
	struct Node;
	const uint16_t kMaxHeight_;
	const uint16_t kBranching_;
	const uint32_t kScaledInverseBranching_;

	Cmp const compare_;
	Node* const head_;

	std::atomic<int> max_height_;  // Height of the entire list
	// Used for optimizing sequential insert patterns.
	Node** prev_;
	int32_t prev_height_;

	Node* NewNode(const K& key, int height) {
		char* mem = (char*)malloc(sizeof(Node) + sizeof(std::atomic<Node*>)*(height - 1));
		return new (mem) Node(key);
	}

	int RandomHeight() {
		int height = 1;
		while(height < kMaxHeight_ && rand() < kScaledInverseBranching_) height++;
		assert(height > 0);
		assert(height < kMaxHeight_);
		return height;
	}
	bool KeyIsAfterNode(const K& key, Node* n) const {
		// nullptr n is considered infinite
		return (n != nullptr) && (compare_(n->key, key) < 0);
	}

	inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
  }

	bool Equal(const K& a, const K& b) const { return (compare_(a, b) == 0); }
	bool LessThan(const K& a, const K& b) const {
    return (compare_(a, b) < 0);
  }

	// Returns the earliest node with a key >= key.
  // Return nullptr if there is no such node.
	Node *FindGreaterOrEqual(const K& key) const {
		Node* x = head_;
		int level = GetMaxHeight() - 1;
		Node* last_bigger = nullptr;
		while (true) {
			assert(x != nullptr);
			Node* next = x->Next(level);
			// Make sure the lists are sorted
			assert(x == head_ || next == nullptr || KeyIsAfterNode(next->key, x));
			// Make sure we haven't overshot during our search
			assert(x == head_ || KeyIsAfterNode(key, x));
			int cmp = (next == nullptr || next == last_bigger) ? 1 : compare_(next->key, key);
			if (cmp == 0 || (cmp > 0 && level == 0)) {
				return next;
			} else if (cmp < 0) {
				// Keep searching in this list
				x = next;
			} else {
				// Switch to next list, reuse compare_() result
				last_bigger = next;
				level--;
			}
		}
	}

	// Return the latest node with a key < key.
  // Return head_ if there is no such node.
	Node *FindLessThan(const K& key, Node** prev=nullptr) const {
		Node* x = head_;
		int level = GetMaxHeight() - 1;
		// KeyIsAfter(key, last_not_after) is definitely false
		Node* last_not_after = nullptr;
		while (true) {
			assert(x != nullptr);
			Node* next = x->Next(level);
			assert(x == head_ || next == nullptr || KeyIsAfterNode(next->key, x));
			assert(x == head_ || KeyIsAfterNode(key, x));
			if (next != last_not_after && KeyIsAfterNode(key, next)) {
				// Keep searching in this list
				x = next;
			} else {
				if (prev != nullptr) {
					prev[level] = x;
				}
				if (level == 0) {
					return x;
				} else {
					// Switch to next list, reuse KeyIUsAfterNode() result
					last_not_after = next;
					level--;
				}
			}
		}
	}

	Node *FindLast() const {
		Node* x = head_;
		int level = GetMaxHeight() - 1;
		while (true) {
			Node* next = x->Next(level);
			if (next == nullptr) {
				if (level == 0) {
					return x;
				} else {
					// Switch to next list
					level--;
				}
			} else {
				x = next;
			}
		}
	}
};

template<typename K, class Cmp>
struct SkipList<K, Cmp>::Node {
	K const key;
	explicit Node(const K& k): key(k) {}

	Node *Next(int n) {
		assert(n >= 0);
		// Use an 'acquire load' so that we observe a fully initialized
    // version of the returned Node.
		return next_[n].load(std::memory_order_acquire);
	}
	void SetNext(int n, Node *next) {
		assert(n >= 0);
		// Use a 'release store' so that anybody who reads through this
    // pointer observes a fully initialized version of the inserted node
		next_[n].store(next, std::memory_order_release);
	}
	// No-barrier variants that can be safely used in a few locations.
	Node* NoBarrier_Next(int n) {
		assert(n >= 0);
		return next_[n].load(std::memory_order_relaxed);
	}
	void NoBarrier_SetNext(int n, Node* x) {
		assert(n >= 0);
		next_[n].store(x, std::memory_order_relaxed);
	}
	private:
	// Array of length equal to the node height.  next_[0] is lowest level link.
	std::atomic<Node*> next_[1];
};
template<typename K, class Cmp>
class SkipList<K, Cmp>::Iterator {
	public:
	// Initialize an iterator over the specified list.
	// The returned iterator is not valid.
	explicit Iterator(const SkipList* list) {
		SetList(list);
	}

	void SetList(const SkipList* list) {
		list_ = list;
		node_ = nullptr;
	}

	bool Valid() const {
		return node_ != nullptr;
	}

	// Returns the key at the current position.
	const K& key() const {
		assert(Valid());
		return node_->key;
	}

	void Next() {
		assert(Valid());
  		node_ = node_->Next(0);
	}

	void Prev() {
		assert(Valid());
		node_ = list_->FindLessThan(node_->key);
		if (node_ == list_->head_) {
			node_ = nullptr;
		}
	}

	// Advance to the first entry with a key >= target
	void Seek(const K& target) {
		node_ = list_->FindGreaterOrEqual(target);
	}

	// Retreat to the last entry with a key <= target
	void SeekForPrev(const K& target) {
		Seek(target);
  		if (!Valid()) {
			SeekToLast();
		}
		while (Valid() && list_->LessThan(target, key())) {
			Prev();
		}
	}

	void SeekToFirst() {
		node_ = list_->head_->Next(0);
	}

	void SeekToLast() {
		node_ = list_->FindLast();
		if (node_ == list_->head_) {
			node_ = nullptr;
		}
	}

	private:
	const SkipList* list_;
	Node* node_;
	// Intentionally copyable
};
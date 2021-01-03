#pragma once

#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <atomic>
#include <limits>
#include <algorithm>
#include <shared_mutex>

namespace died
{
	template<
		class Key,
		class T,
		unsigned int N = UINT_MAX - 1>
	class circle_map final
	{
		static_assert(N > 0, "Map size must be greater than 0");
		static_assert(N < UINT_MAX, "Map size must be less than UINT_MAX");

	public:
		using key_type = Key;
		using mapped_type = T;
		using size_type = unsigned int;
		using con_vec = Concurrency::concurrent_vector<mapped_type>;
		using con_map = Concurrency::concurrent_unordered_map<key_type, size_type>;
		using reference = mapped_type&;
		using const_reference = const mapped_type&;

	private:
		class clear_map_condition
		{
		public:
			size_type increase() noexcept
			{
				return exceed() ? MAX_SIZE : ++mSize;
			}

			size_type get() const noexcept
			{
				return mSize.load(std::memory_order_relaxed);
			}

			bool changed(size_type val) const noexcept
			{
				return val != get();
			}

			bool exceed() const noexcept
			{
				return get() > MAX_SIZE;
			}

			void clear()
			{
				mSize = 0;
			}

		private:
			const size_type MAX_SIZE = std::max<size_type>(N, 10240);
			std::atomic<size_type> mSize{};
		};

	public:
		constexpr size_type size() const noexcept 
		{ 
			return N;
		}

		bool empty() const noexcept
		{
			return mEmpty.load(std::memory_order_relaxed);
		}

		const_reference find(key_type const& key) const
		{
			auto pos = find_internal(key);
			return (INVALID_INDEX != pos) ? mData[pos] : EMPTY_ITEM;
		}

		template<class Predicate>
		const_reference find_if(Predicate pre) const
		{
			// empty map
			if (empty()) {
				return EMPTY_ITEM;
			}

			// not empty
			bool found = false;
			size_type pos = mPopIndex.load(std::memory_order_relaxed);
			for (size_type i = 0; i < N; ++i) { // circle search
				size_type idx = (pos + i) % N;
				const auto& item = mData[idx];
				if (item && pre(item)) {
					found = true;
					pos = idx;
					break;
				}
			}

			return found ? mData[pos] : EMPTY_ITEM;
		}

		template<class Predicate>
		const_reference rfind_if(Predicate pre) const
		{
			// empty map
			if (empty()) {
				return EMPTY_ITEM;
			}

			// not empty
			bool found = false;
			size_type pos = mPopIndex.load(std::memory_order_relaxed) + 1; // start with current pos
			for (size_type i = 0; i < N; ++i) { // circle search
				const auto& item = mData[--pos];
				if (item && pre(item)) {
					found = true;
					break;
				}
				if (0 == pos) pos = N;
			}

			return found ? mData[pos] : EMPTY_ITEM;
		}

		template<class Func>
		void loop_all(Func invoke) const
		{
			// empty map
			if (empty()) {
				return;
			}
			size_type pos = mPopIndex.load(std::memory_order_relaxed);
			for (size_type i = 0; i < N; ++i) { // circle search
				size_type idx = (pos + i) % N;
				const auto& item = mData[idx];
				if (item) {
					invoke(item);
				}
			}
		}

		template<class Func>
		void rloop_all(Func invoke) const
		{
			// empty map
			if (empty()) {
				return;
			}
			size_type pos = mPopIndex.load(std::memory_order_relaxed) + 1;
			for (size_type i = 0; i < N; ++i) { // circle search
				const auto& item = mData[--pos];
				if (item) {
					invoke(item);
				}
				if (0 == pos) pos = N;
			}
		}

		reference operator[](key_type const& key)
		{
			std::shared_lock<std::shared_mutex> slk(mSyncClear);
			mClearCond.increase();

			// This function is considered as add item to map
			// Whenever it is called should change the empty state
			update_empty(false);

			size_type pos = find_internal(key);
			if (INVALID_INDEX != pos) {
				return mData[pos];
			}

			// Not found => create new pair
			size_type curIndex = next_push_index();
			mKeys[key] = curIndex;
			return mData[curIndex];
		}

		void erase(key_type const& key)
		{
			auto found = mKeys.find(key);

			// Not valid
			if (std::cend(mKeys) == found || INVALID_INDEX == found->second) {
				return;
			}

			// reset data
			mData[found->second] = mapped_type{};
			found->second = INVALID_INDEX;
		}

		const_reference front() const
		{
			return mData[get_pop_index()];
		}

		size_type next_available_item()
		{
			// Empty data
			if (empty()) {
				return mPopIndex.load(std::memory_order_relaxed);
			}

			auto oldSize = mClearCond.get();
			size_type old = mPopIndex.load(std::memory_order_relaxed);
			size_type next = old;
			for (size_type i = 0; i < N; ++i) { // Circle search
				next = (next + 1) % N;
				if (mData[next]) {
					break;
				}
			}

			// No more data
			if (next == old) {
				if (!mData[next]) {
					// Mark as empty map
					update_empty(true);
					if (!mClearCond.changed(oldSize)) {
						clear();
					}
				}
				else { // The 'old' is already processed => should ignore it
					next = (old + 1) % N;
				}
			}

			// avaible item => update to atomic
			while (!mPopIndex.compare_exchange_weak(old, next, std::memory_order_release, std::memory_order_relaxed)) {}
			return next;
		}

	private:
		size_type find_internal(key_type const& key) const
		{
			// Empty map
			if (empty()) {
				return INVALID_INDEX;
			}

			// not empty
			auto found = mKeys.find(key);
			if (std::cend(mKeys) == found || INVALID_INDEX == found->second) {
				return INVALID_INDEX;
			}
			return found->second;
		}

		size_type next_push_index() noexcept 
		{ 
			return mPushIndex++ % N; 
		}

		size_type get_pop_index() const noexcept 
		{ 
			return mPopIndex.load(std::memory_order_relaxed) % N; 
		}

		bool update_empty(bool val)
		{
			bool old = mEmpty.load(std::memory_order_relaxed);
			while (!mEmpty.compare_exchange_weak(old, val, std::memory_order_release, std::memory_order_relaxed)) {}
			return old;
		}

		void clear()
		{
			if (mClearCond.exceed()) {
				std::lock_guard<std::shared_mutex> lk(mSyncClear);
				mKeys.clear();
				mClearCond.clear();
			}
		}

	private:
		std::atomic_bool mEmpty{ true };
		std::atomic<size_type> mPushIndex{ 0u };
		std::atomic<size_type> mPopIndex{ 0u };
		con_vec mData{ N };
		con_map mKeys;
		const size_type		INVALID_INDEX = N + 1;
		const mapped_type	EMPTY_ITEM{};
		std::shared_mutex	mSyncClear;
		clear_map_condition mClearCond;
	};
}
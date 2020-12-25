#pragma once

#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <atomic>
#include <limits>
#include <algorithm>
#include "gsl\gsl_assert"

namespace died
{
	template<
		class KEY,
		class T,
		unsigned int N = std::numeric_limits<unsigned int>::max() - 1>
	class circle_map final
	{
	public:
		using key_type = KEY;
		using mapped_type = T;
		using size_type = unsigned int;
		using con_vec = Concurrency::concurrent_vector<mapped_type>;
		using con_map = Concurrency::concurrent_unordered_map<key_type, size_type>;
		using reference = mapped_type&;
		using const_reference = const mapped_type&;

	public:
		circle_map() noexcept(std::is_nothrow_constructible_v<con_vec> && std::is_nothrow_constructible_v<con_map>) = default;
		~circle_map() noexcept = default;

		// copyable
		circle_map(circle_map const& other) :
			mPushIndex{ other.mPushIndex.load(std::memory_order_relaxed) },
			mPopIndex{ other.mPopIndex.load(std::memory_order_relaxed) },
			mData{ other.mData },
			mKeys{ other.mKeys }
		{}

		circle_map& operator=(circle_map const& other)
		{
			auto tmp(other);
			*this = std::move(tmp);
			return *this;
		}

		// movable
		circle_map(circle_map&& other) noexcept:
			mPushIndex{ other.mPushIndex.load(std::memory_order_relaxed) },
			mPopIndex{ other.mPopIndex.load(std::memory_order_relaxed) },
			mData{ std::exchange(other.mData,  con_vec{}) },
			mKeys{ std::exchange(other.mKeys, con_map{}) }
		{
			other.mPushIndex.store(0, std::memory_order_relaxed);
			other.mPopIndex.store(0, std::memory_order_relaxed);
		}

		circle_map& operator=(circle_map&& other) noexcept
		{
			if (this != &other) {
				mPushIndex.store(other.mPushIndex.load(std::memory_order_relaxed));
				mPopIndex.store(other.mPopIndex.load(std::memory_order_relaxed));
				mData = std::exchange(other.mData, con_vec{});
				mKeys = std::exchange(other.mKeys, con_map{});
				other.mPushIndex.store(0, std::memory_order_relaxed);
				other.mPopIndex.store(0, std::memory_order_relaxed);
			}
			return *this;
		}

		constexpr size_type max_size() const noexcept { return N; }
		size_type size() const noexcept { return mData.size(); }
		bool empty() const noexcept
		{
			Ensures(mPushIndex.load() >= mPopIndex.load());
			return mPushIndex.load(std::memory_order_relaxed) == mPopIndex.load(std::memory_order_relaxed);
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
				pos = (pos + i) % N;
				const auto& item = mData[pos];
				if (item && pre(item)) {
					found = true;
					break;
				}
			}

			return found ? mData[pos] : EMPTY_ITEM;
		}

		reference operator[](key_type const& key)
		{
			size_type pos = find_internal(key);
			if (INVALID_INDEX != pos) {
				return mData[pos];
			}

			// Not found => create new pair
			size_type curIndex = next_push_index();
			mKeys[key] = curIndex;
			return mData[curIndex];
		}

		reference operator[](key_type&& key)
		{
			size_type pos = find_internal(key);
			if (INVALID_INDEX != pos) {
				return mData[pos];
			}

			// Not found => create new pair
			size_type curIndex = next_push_index();
			mKeys[std::move(key)] = curIndex;
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

			bool found = false;
			size_type old = mPopIndex.load(std::memory_order_relaxed);
			size_type next = old;
			for (size_type i = 0; i < N; ++i) { // Circle search
				if (mData[++next % N]) {
					found = true;
					break;
				}
			}

			// No available data
			if (!found) {
				// Mark as empty map by rest 'pop index' = 'push index'
				next = mPushIndex.load(std::memory_order_relaxed);
			}

			// Always 'pop index' <= 'push index'
			Ensures(next <= mPushIndex.load());

			// avaible item => update to atomic
			while (!mPopIndex.compare_exchange_weak(old, next, std::memory_order_release, std::memory_order_relaxed)) {}
			return next % N;
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

		size_type next_push_index() noexcept { return mPushIndex++ % N; }
		size_type get_pop_index() const noexcept { return mPopIndex.load(std::memory_order_relaxed) % N; }

	private:
		std::atomic<size_type> mPushIndex{ 0u };
		std::atomic<size_type> mPopIndex{ 0u };
		con_vec mData{ N };
		con_map mKeys; // Doesn't have reserve()
		const unsigned int	INVALID_INDEX = N + 1;
		const mapped_type	EMPTY_ITEM{};
	};
}
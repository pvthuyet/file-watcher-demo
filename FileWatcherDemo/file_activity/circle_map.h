#pragma once

#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <atomic>
#include <limits>
#include <algorithm>

namespace died
{
	template<
		class KEY,
		class T,
		unsigned int N = std::numeric_limits<unsigned int>::max() - 1>
	class circle_map final
	{
		static constexpr unsigned int INVALID_POS = N + 1;
	public:
		using key_type = KEY;
		using mapped_type = T;
		using size_type = unsigned int;
		using con_vec = Concurrency::concurrent_vector<mapped_type>;
		using con_map = Concurrency::concurrent_unordered_map<key_type, size_type>;

	public:
		circle_map() noexcept(std::is_nothrow_constructible_v<con_vec> && std::is_nothrow_constructible_v<con_map>) = default;
		~circle_map() noexcept = default;

		// copyable
		circle_map(circle_map const& other) :
			mPosPush{ other.mPosPush.load(std::memory_order_relaxed) },
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
			mPosPush{ other.mPosPush.load(std::memory_order_relaxed) },
			mData{ std::exchange(other.mData,  con_vec{}) },
			mKeys{ std::exchange(other.mKeys, con_map{}) }
		{
			other.mPosPush.store(0, std::memory_order_relaxed);
		}

		circle_map& operator=(circle_map&& other) noexcept
		{
			if (this != &other) {
				mPosPush.store(other.mPosPush.load(std::memory_order_relaxed));
				mData = std::exchange(other.mData, con_vec{});
				mKeys = std::exchange(other.mKeys, con_map{});
				other.mPosPush.store(0, std::memory_order_relaxed);
			}
			return *this;
		}

		constexpr size_type max_size() const noexcept { return N; }
		constexpr size_type size() const noexcept { return mKeys.size(); }

		mapped_type find(key_type const& key) const
		{
			auto pos = find_internal(key);
			if (INVALID_POS != pos) {
				return mData[pos];
			}

			// return empty element
			return mapped_type{};
		}

		template<class Predicate>
		mapped_type find_if(Predicate pre) const
		{
			auto found = std::find_if(mData.cbegin(), mData.cend(), pre);
			if (mData.cend() != found) {
				return *found;
			}

			// return empty element
			return mapped_type{};
		}

		mapped_type& operator[](key_type const& key)
		{
			size_type found = find_internal(key);
			if (INVALID_POS != found) {
				return mData[found];
			}

			// Not found => create new pair
			size_type pos = next_push_pos();
			mKeys[key] = pos;
			return mData[pos];
		}

		void erase(key_type const& key)
		{
			auto found = mKeys.find(key);

			// Not valid
			if (std::cend(mKeys) == found || INVALID_POS == found->second) {
				return;
			}

			// reset data
			mData[found->second] = mapped_type{};
			found->second = INVALID_POS;
		}

		mapped_type front() const
		{
			// get current pos_pop
			auto pos = get_pop_pos();
			return mData[pos];
		}

		size_type next_available_item() noexcept
		{
			// Empty data
			if (mKeys.size() == 0) {
				return INVALID_POS;
			}

			bool found = false;
			size_type old = mPosPop.load(std::memory_order_relaxed);
			size_type next = old;
			for (size_type i = 0; i < N; ++i) { // Circle search
				next = (next + 1) % N;
				if (mData[next]) {
					found = true;
					break;
				}
			}

			// No available data
			if (!found) {
				return INVALID_POS;
			}

			// avaible item => update to atomic
			while (!mPosPop.compare_exchange_weak(old, next, std::memory_order_release, std::memory_order_relaxed)) {}
			return next;
		}

	private:
		size_type find_internal(key_type const& key) const
		{
			auto found = mKeys.find(key);
			if (std::cend(mKeys) == found || INVALID_POS == found->second) {
				return INVALID_POS;
			}
			return found->second;
		}

		size_type next_push_pos() noexcept { return mPosPush++ % N; }
		size_type get_pop_pos() const noexcept { return mPosPop.load(std::memory_order_relaxed) % N; }

	private:
		std::atomic<size_type> mPosPush{ 0ul };
		std::atomic<size_type> mPosPop{ 0ul };
		con_vec mData{ N };
		con_map mKeys; // Doesn't have reserve()
	};
}
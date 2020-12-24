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
	public:
		using key_type = KEY;
		using mapped_type = T;
		using size_type = unsigned int;
		using con_vec = Concurrency::concurrent_vector<mapped_type>;
		using con_map = Concurrency::concurrent_unordered_map<key_type, size_type>;
		using reference = mapped_type&;
		using const_reference = const mapped_type&;

	private:
		const unsigned int		INVALID_POS = N + 1;
		const mapped_type		EMPTY_MAPPED_TYPE{};

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
		constexpr size_type size() const noexcept { return mKeys.size(); }

		const_reference find(key_type const& key) const
		{
			auto pos = find_internal(key);
			return (INVALID_POS != pos) ? mData[pos] : EMPTY_MAPPED_TYPE;
		}

		template<class Predicate>
		const_reference find_if(Predicate pre) const
		{
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

			return found ? mData[pos] : EMPTY_MAPPED_TYPE;
		}

		reference operator[](key_type const& key)
		{
			size_type pos = find_internal(key);
			if (INVALID_POS != pos) {
				return mData[pos];
			}

			// Not found => create new pair
			size_type next = next_push_index();
			mKeys[key] = next;
			return mData[next];
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

		const_reference front() const
		{
			return mData[get_pop_index()];
		}

		size_type next_available_item() noexcept
		{
			// Empty data
			if (mKeys.size() == 0) {
				return INVALID_POS;
			}

			bool found = false;
			size_type old = mPopIndex.load(std::memory_order_relaxed);
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
			while (!mPopIndex.compare_exchange_weak(old, next, std::memory_order_release, std::memory_order_relaxed)) {}
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

		size_type next_push_index() noexcept { return mPushIndex++ % N; }
		size_type get_pop_index() const noexcept { return mPopIndex.load(std::memory_order_relaxed) % N; }

	private:
		std::atomic<size_type> mPushIndex{ 0u };
		std::atomic<size_type> mPopIndex{ 0u };
		con_vec mData{ N };
		con_map mKeys; // Doesn't have reserve()
	};
}
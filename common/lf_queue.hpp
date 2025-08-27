#pragma once

#include <atomic>
#include <vector>

#include "common/macros.hpp"

namespace Common {

	template<typename T>
	class LFQueue final {

		std::vector<T> m_store;

		std::atomic<size_t> m_next_write_index = 0;
		std::atomic<size_t> m_next_read_index = 0;
		std::atomic<size_t> m_num_elements = 0;

		LFQueue() = delete;
		LFQueue(const LFQueue&) = delete;
		LFQueue(const LFQueue&&) = delete;
		LFQueue& operator=(const LFQueue&) = delete;
		LFQueue& operator=(const LFQueue&&) = delete;

	public:
		LFQueue(size_t num_elems) : m_store(num_elems, T()) {}

		auto getNextToWriteTo() noexcept {
			return &m_store[m_next_write_index];
		}

		auto updateWriteIndex() noexcept {
			m_next_write_index = (m_next_write_index + 1) % m_store.size();
			m_num_elements++;
		}

		const T* getNextToRead() const noexcept {
			return (m_next_read_index == m_next_write_index) ? 
				nullptr : &m_store[m_next_read_index];
		}

		auto updateReadIndex() noexcept {
			m_next_read_index = (m_next_read_index + 1) % m_store.size();
			ASSERT(m_num_elements != 0, "Read and invalid element in: " +
				std::to_string(pthread_self()));
			m_num_elements--;
		}

		auto size() const noexcept {
			return m_num_elements.load();
		}
	};
}
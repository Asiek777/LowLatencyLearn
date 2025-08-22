#pragma once

#include <vector>
#include <string>

#include "common/macros.hpp"

namespace Common {
	template<typename T>
	class MemPool final {
		struct ObjectBlock {
			T m_object;
			bool m_is_free = true;
		};

		std::vector<ObjectBlock> m_store;
		size_t m_next_free_index = 0;

		MemPool() = delete;
		MemPool(const MemPool&) = delete;
		MemPool(const MemPool&&) = delete;
		MemPool& operator=(const MemPool&) = delete;
		MemPool& operator=(const MemPool&&) = delete;

		void updateNextFreeIndex() noexcept {
			const auto initial_free_index = m_next_free_index;
			while (!m_store[m_next_free_index].m_is_free) {
				++m_next_free_index;
				if (m_next_free_index == m_store.size()) [[unlikely]] {
					m_next_free_index = 0;
				}
				if (m_next_free_index == initial_free_index) {
					ASSERT(m_next_free_index != initial_free_index,
						"Memory Pool out of space.");
				}
			}
		}

	public:
		explicit MemPool(size_t num_elems) : m_store(num_elems, { T(), true }) {
			ASSERT(reinterpret_cast<const ObjectBlock*>(&(m_store[0].m_object)) == &(m_store[0]),
				"T object should be first member of ObjectBlock.");
		}

		template<typename... Args>
		T* allocate(Args... args) noexcept {
			auto obj_block = &(m_store[m_next_free_index]);
			ASSERT(obj_block->m_is_free, "Expected free ObjectBlock at index: " + 
				std::to_string(m_next_free_index));
			T* ret = &(obj_block->m_object);
			ret = new(ret) T(args...);
			obj_block->m_is_free = false;
			updateNextFreeIndex();

			return ret;
		}

		auto deallocate(const T* elem) noexcept {
			const auto elem_index = (reinterpret_cast<const ObjectBlock*>(elem)) - &m_store[0];
			ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index) < m_store.size(),
				"Element being deallocated does not belong to this memory pool.");
			ASSERT(!m_store[elem_index].m_is_free, 
				"Expected in-use ObjectoBlock at index: " + std::to_string(elem_index));
			m_store[elem_index].m_is_free = true;
		}
	};
}
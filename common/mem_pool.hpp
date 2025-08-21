#include "common/macros.hpp"

#include <vector>
#include <string>

namespace Common {
	template<typename T>
	class MemPool final {
		struct ObjectBlock {
			T object_;
			bool is_free_ = true;
		};

		std::vector<ObjectBlock> store_;
		size_t next_free_index_ = 0;

		MemPool() = delete;
		MemPool(const MemPool&) = delete;
		MemPool(const MemPool&&) = delete;
		MemPool& operator=(const MemPool&) = delete;
		MemPool& operator=(const MemPool&&) = delete;

	public:
		explicit MemPool(size_t num_elems) : store_(num_elems, { T(), true }) {
			ASSERT(reinterpret_cast<const ObjectBlock*>(&(store_[0].object_)) == &(store_[0]),
				"T object should be first member of ObjectBlock.");
		}

	};
}
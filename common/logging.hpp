#pragma once
#include <string>
#include <fstream>
#include <thread>

#include "common/lf_queue.hpp"
#include "common/thread_utils.hpp"



namespace Common {

	constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

	enum class LogType : int8_t {
		CHAR = 0,
		INTEGER = 1, LONG_INTEGER = 2, LONG_LONG_INTEGER = 3,
		UNSIGNED_INTEGER = 4, UNSIGNED_LONG_INTEGER = 5,
		UNSIGNED_LONG_LONG_INTEGER = 6,
		FLOAT = 7, DOUBLE = 8,
	};

	struct LogElement {
		LogType m_type = LogType::CHAR;
		union {
			char c;
			int i; long l; long long ll;
			unsigned u; unsigned long ul; unsigned long long ull;
			float f; double d;
		} m_u;
	};

	class Logger final {
		const std::string m_file_name;
		std::ofstream m_file;
		LFQueue<LogElement> m_queue;
		std::atomic<bool> m_running = true;
		std::thread* m_logger_thread = nullptr;

		Logger() = delete;
		Logger(const Logger&) = delete;
		Logger(const Logger&&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger& operator=(const Logger&&) = delete;

	public:

		auto flushQueue() noexcept {
			while (m_running) {
				for (auto next = m_queue.getNextToRead();
					m_queue.size() && next; next = m_queue.getNextToRead()) {
					switch (next->m_type)
					{
					case LogType::CHAR:
						m_file << next->m_u.c;
						break;
					case LogType::INTEGER:
						m_file << next->m_u.i;
						break;
					case LogType::LONG_INTEGER:
						m_file << next->m_u.l;
						break;
					case LogType::LONG_LONG_INTEGER:
						m_file << next->m_u.ll;
						break;
					case LogType::UNSIGNED_INTEGER:
						m_file << next->m_u.u;
						break;
					case LogType::UNSIGNED_LONG_INTEGER:
						m_file << next->m_u.ul;
						break;
					case LogType::UNSIGNED_LONG_LONG_INTEGER:
						m_file << next->m_u.ull;
						break;
					case LogType::FLOAT:
						m_file << next->m_u.f;
						break;
					case LogType::DOUBLE:
						m_file << next->m_u.d;
						break;
					}
					m_queue.updateReadIndex();
				}
				using namespace std::literals::chrono_literals;
				std::this_thread::sleep_for(1ms);
			}
		}
		explicit Logger(const std::string& file_name) :
			m_file_name(file_name), m_queue(LOG_QUEUE_SIZE) {
			m_file.open(m_file_name);
			m_logger_thread = createAndStartThread(-1, "Common/Logger",
				[this]() { flushQueue(); });
			ASSERT(m_logger_thread != nullptr, "Failed to start Logger thread.");
		}

		~Logger() {
			std::cerr << "Flushing and closing Logger for " << m_file_name << std::endl;
			while (m_queue.size()) {
				using namespace std::literals::chrono_literals;
				std::this_thread::sleep_for(1s);
			}
			m_running = false;
			m_logger_thread->join();

			m_file.close();
		}

		auto pushValue(const LogElement& log_element) noexcept {
			*(m_queue.getNextToWriteTo()) = log_element;
			m_queue.updateWriteIndex();
		}

		auto pushValue(const char value) noexcept {
			pushValue(LogElement{ LogType::CHAR, {.c = value} });
		}

		auto pushValue(const char* value) noexcept {
			while (*value) {
				pushValue(*value);
				value++;
			}
		}

		auto pushValue(const std::string& value) noexcept {
			pushValue(value.c_str());
		}

		auto pushValue(const int value) noexcept {
			pushValue(LogElement{ LogType::INTEGER, {.i = value} });
		}

		auto pushValue(const long value) noexcept {
			pushValue(LogElement{ LogType::LONG_INTEGER, {.l = value} });
		}

		auto pushValue(const long long value) noexcept {
			pushValue(LogElement{ LogType::LONG_LONG_INTEGER, {.ll = value} });
		}

		auto pushValue(const unsigned value) noexcept {
			pushValue(LogElement{ LogType::UNSIGNED_INTEGER, {.u = value} });
		}

		auto pushValue(const unsigned long value) noexcept {
			pushValue(LogElement{ LogType::UNSIGNED_LONG_INTEGER, {.ul = value} });
		}

		auto pushValue(const unsigned long long value) noexcept {
			pushValue(LogElement{ LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull = value} });
		}

		auto pushValue(const float value) noexcept {
			pushValue(LogElement{ LogType::FLOAT, {.f = value} });
		}

		auto pushValue(const double value) noexcept {
			pushValue(LogElement{ LogType::DOUBLE, {.d = value} });
		}

		template<typename T, typename... A>
		auto log(const char* s, const T& value, A... args) noexcept {
			while (*s) {
				if (*s == '%') {
					if (*(s + 1) == '%') [[unlikely]] {
						s++;
					}
					else {
						pushValue(value);
						log(s + 1, args...);
						return;
					}
				}
				pushValue(*s++);
			}
			FATAL("Extra arguments provided to log()");
		}
		auto log(const char* s) noexcept {
			while (*s) {
				if (*s == '%') {
					if (*(s + 1) == '%') [[unlikely]] {
						s++;
					}
					else {
						FATAL("missing arguments to log()");
					}
				}
				pushValue(*s++);
			}
		}

	};

}
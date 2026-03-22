#ifndef LOG_FILE_WRITER_H
#define LOG_FILE_WRITER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace godot {

class LogFileWriter : public Object {

    GDCLASS(LogFileWriter, Object);

public:

    // Keep the enum for internal C++ use
    enum LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4 };
    // Internal C++ macro-friendly log
    void _log_internal(LogLevel p_level, const String &p_msg, const char *p_file, int p_line);

    static LogFileWriter* get_singleton() { return singleton; }

    // Bound methods for GDScript (using int for compatibility)
    void set_min_level(int p_level);
    void log_gd(int p_level, const String &p_msg);

    LogFileWriter();
    ~LogFileWriter();

    void info(const String &p_msg) { log_gd(INFO, p_msg); }
    void warn(const String &p_msg) { log_gd(WARN, p_msg); }
    void error(const String &p_msg) { log_gd(ERROR, p_msg); }

protected:
    static void _bind_methods();

private:

    static inline LogFileWriter *singleton = nullptr;
    std::atomic<int> min_level{0};
    
    // Async logic
    struct LogEntry {
        int level;
        String message;
        String timestamp;
        const char *file;
        int line;
    };

    std::queue<LogEntry> log_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::thread worker_thread;
    std::atomic<bool> should_exit{false};

    void process_logs();
    String get_timestamp();
};

};

// THIS IS THE KEY: Use the fully qualified name here
VARIANT_ENUM_CAST(godot::LogFileWriter::LogLevel);

// C++ Helper Macros
#define LOG_INFO(m) LogFileWriter::get_singleton()->_log_internal(LogFileWriter::INFO, m, __FILE__, __LINE__)
#define LOG_WARN(m) LogFileWriter::get_singleton()->_log_internal(LogFileWriter::WARN, m, __FILE__, __LINE__)
#define LOG_ERR(m)  LogFileWriter::get_singleton()->_log_internal(LogFileWriter::ERROR, m, __FILE__, __LINE__)

#endif
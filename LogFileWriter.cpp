#include "LogFileWriter.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace godot;

static const char* LOG_FILENAME = "game_session.log";

LogFileWriter::LogFileWriter() {
    singleton = this;
    worker_thread = std::thread(&LogFileWriter::process_logs, this);
}

LogFileWriter::~LogFileWriter() {
    should_exit = true;
    cv.notify_all();
    if (worker_thread.joinable()) worker_thread.join();
    // Al cerrar el juego, limpiamos el puntero
    if (singleton == this) singleton = nullptr;
}

void LogFileWriter::set_min_level(int p_level) { min_level = p_level; }

void LogFileWriter::log_gd(int p_level, const String &p_msg) {
    _log_internal(static_cast<LogLevel>(p_level), p_msg, "GDScript", 0);
}

void LogFileWriter::_log_internal(LogLevel p_level, const String &p_msg, const char *p_file, int p_line) {
    if ((int)p_level < min_level.load()) return;

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        log_queue.push({(int)p_level, p_msg, get_timestamp(), p_file, p_line});
    }
    cv.notify_one();
}

void LogFileWriter::process_logs() {
    String godot_path = String("user://") + LOG_FILENAME;
    String abs_path = ProjectSettings::get_singleton()->globalize_path(godot_path);
    
    std::ofstream file(abs_path.utf8().get_data(), std::ios::app);
    const char* levels[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

    while (true) {
        LogEntry entry;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { return !log_queue.empty() || should_exit; });
            if (should_exit && log_queue.empty()) break;
            entry = std::move(log_queue.front());
            log_queue.pop();
        }

        String output = vformat("[%s] [%s] [%s:%d] %s", 
            entry.timestamp, levels[entry.level], entry.file, entry.line, entry.message);

        // Print to Godot output
        if (entry.level >= ERROR) UtilityFunctions::printerr(output);
        else UtilityFunctions::print(output);

        if (file.is_open()) {
            file << output.utf8().get_data() << std::endl;
            file.flush();
        }
    }
}

String LogFileWriter::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%H:%M:%S");
    return String(ss.str().c_str());
}

void LogFileWriter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_min_level", "level"), &LogFileWriter::set_min_level);
    ClassDB::bind_method(D_METHOD("log", "level", "message"), &LogFileWriter::log_gd);

    ClassDB::bind_method(D_METHOD("info", "message"), &LogFileWriter::info);
    ClassDB::bind_method(D_METHOD("warn", "message"), &LogFileWriter::warn);
    ClassDB::bind_method(D_METHOD("error", "message"), &LogFileWriter::error);

    // Use the constant name, but it maps to Logger::LogLevel
    BIND_ENUM_CONSTANT(DEBUG);
    BIND_ENUM_CONSTANT(INFO);
    BIND_ENUM_CONSTANT(WARN);
    BIND_ENUM_CONSTANT(ERROR);
    BIND_ENUM_CONSTANT(FATAL);
}
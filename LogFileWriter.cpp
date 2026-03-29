#include "LogFileWriter.h"

// It is used to obtain the absolute path of //users of godot
#include <godot_cpp/classes/project_settings.hpp>

// C++ Libraries

// C++ base tool for reading and writing data to physical files on the hard drive.
#include <fstream>

// standard C++ high-precision time management library.
#include <chrono>

// sstream is used to "read/write" within a String as if it were a file or a console.
#include <sstream>

// aesthetic and professional formatting of the data.
#include <iomanip>

// have access to std::cerr and std:.cout
#include <iostream>


using namespace godot;

// Log file name defined as constant
static const char* LOG_FILENAME = "game_session.log";

// Translates your internal enum values into human-readable text for your log file.
// It is used to generate the string that will be stored with respect to the enum
const char* levels[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

// Constructor
LogFileWriter::LogFileWriter() {

    // Setting this object as singleton
    singleton = this;

    // Launching a background thread that runs the process_logs member function when he is woken up
    worker_thread = std::thread(&LogFileWriter::process_logs, this);
}

LogFileWriter::~LogFileWriter() {

    // we set the should_exit flag
    should_exit = true;

    // Awaken all the threads at once.
    cv.notify_all();

    // He blocks the main thread (Godot's) and says:
    // "Wait a moment, don't close the application yet; we have to wait for the worker thread to finish what it's doing and close properly."
    if (worker_thread.joinable()) worker_thread.join();

    // When we close the game, we also clear the pointer
    if (singleton == this) singleton = nullptr;
}


// Function that sets the minimum level from which logs will be saved
void LogFileWriter::set_min_level(int p_level) { min_level = p_level; }


// Method called from GDSCRIPT to register LogEntry in FIFO and wake up file writing
// Also called from the functions debug() info() warn() error() and fatal()
void LogFileWriter::log_gd(int p_level, const String &p_msg, const String &p_file, int p_line, bool isStdOutput) {

    // This line is the communication bridge between the world of GDScript (dynamic and flexible) and your C++ logging engine (strict and fast).
    _log_internal(static_cast<LogLevel>(p_level), std::string(p_msg.utf8().get_data(), p_msg.utf8().length()), std::string(p_file.utf8().get_data(), p_file.utf8().length()), p_line, isStdOutput);
}


// C++ method to put an item in the FIFO of LogEntry and wake up the process_logs method to write the item
void LogFileWriter::_log_internal(LogLevel p_level, const std::string &p_msg, const std::string& p_file, int p_line, bool isStdOutput) {

    // Their job is to prevent the system from wasting time processing messages that the user has decided to ignore.
    if ((int)p_level < min_level.load()) return;

    // code block
    {
        // Mutex management
        std::lock_guard<std::mutex> lock(queue_mutex);

        // Insert LogEntry object into FIFO
        log_queue.push({(int)p_level, p_msg, get_timestamp(), p_file, p_line, isStdOutput});
    }

    // This line is the "bell" that wakes up the writing thread (process_log) so that it is not consuming CPU 100% of the time
    cv.notify_one();
}


// Method for writing to the destination file
void LogFileWriter::process_logs() {

    // These lines are exclusive to Godot
    // Telling Godot to save the log file in the persistent user data folder.
    // String godot_path = String("user://") + LOG_FILENAME;
    // Converting that virtual path into a real, absolute OS path
    // String path = ProjectSettings::get_singleton()->globalize_path(godot_path);
    // Officially moved from Godot’s built-in wrappers to Standard C++ File I/O.
    // std::ofstream file(path.utf8().get_data(), std::ios::app);

    // Version C++ only. Just creating the log file by LOG_FILENAME
    std::string path = LOG_FILENAME;

    // Version C++ only
    std::ofstream file(path, std::ios::trunc);


    // We want the thread to be available throughout the entire life of the game.
    while (true) {

        LogEntry entry;

        // In multithreading programming, the curly braces {} limit the "scope" of the mutex
        {
            // the worker thread has exclusive access to log_queue
            std::unique_lock<std::mutex> lock(queue_mutex);

            // cv.wait puts the thread into a deep sleep, freeing up processor resources until something interesting happens
            cv.wait(lock, [this] { return !log_queue.empty() || should_exit; });

            // This is your safe exit protocol.
            if (should_exit && log_queue.empty()) break;

            // performing a "transfer of ownership" instead of an expensive copy.
            entry = std::move(log_queue.front());

            // The node you just "emptied" with std::move is permanently removed from the queue's memory, making room for the next messages.
            log_queue.pop();
        }

        // The String we want to display in the file is created in C++
        std::ostringstream ss;
        ss << "[" << entry.timestamp << "] "
        << "[" << levels[entry.level] << "] "
        << "[" << entry.file << ":" << entry.line << "] "
        << entry.message;
        std::string output = ss.str();

        // Print to output error or fatal messages, other levels depends on entry.isStdOutput, only if entry.isStdOutput is true is printed      
        if (entry.level >= ERROR) std::cerr << output << std::endl;
        else if (entry.isStdOutput) std::cout << output << std::endl;

        // Everything is Writing to file
        if (file.is_open()) {
            file << output << std::endl;
            file.flush();
        }
    }
}

// Obtain the current date professionally
std::string LogFileWriter::get_timestamp() {

    // Using the modern C++ way to handle time
    // 1. Get the current time point
    auto now = std::chrono::system_clock::now();

    // 1. Get miliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 2. Convert to time_t for formatting
    auto t = std::chrono::system_clock::to_time_t(now);
    
    // 3. Thread-safe conversion to local time
    std::basic_ostringstream<char> ss;

    std::tm lt;
    #ifdef _WIN32
        localtime_s(&lt, &t); 
    #else
        localtime_r(&t, &lt);
    #endif

    // 4. The line you provided: format and "pipe" into the stream
    ss << std::put_time(&lt, "%H:%M:%S")<< '.' << std::setfill('0') << std::setw(3) << ms.count();

    // 5. Return as a std::string
    return std::string(ss.str());    
}

// Only for Godot to make the bindings
void LogFileWriter::_bind_methods() {

    // Record of methods for Godot to see.
    ClassDB::bind_method(D_METHOD("set_min_level", "level"), &LogFileWriter::set_min_level);
    ClassDB::bind_method(D_METHOD("log", "level", "message"), &LogFileWriter::log_gd, "GDSCRIPT", 0, true);

    ClassDB::bind_method(D_METHOD("info", "message"), &LogFileWriter::info, "GDSCRIPT", 0, true);
    ClassDB::bind_method(D_METHOD("warn", "message"), &LogFileWriter::warn, "GDSCRIPT", 0,  true);
    ClassDB::bind_method(D_METHOD("error", "message"), &LogFileWriter::error, "GDSCRIPT", 0,  true);

    // It exposes a C++ enum value to the Godot Editor and GDScript 
    // so that you can use it by name (e.g., MyClass.DEBUG) instead of a raw integer.
    BIND_ENUM_CONSTANT(DEBUG);
    BIND_ENUM_CONSTANT(INFO);
    BIND_ENUM_CONSTANT(WARN);
    BIND_ENUM_CONSTANT(ERROR);
    BIND_ENUM_CONSTANT(FATAL);
}

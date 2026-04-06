#ifndef LOG_FILE_WRITER_H
#define LOG_FILE_WRITER_H

// That header (class_db.hpp) is essential for registering your classes in the engine
// Needed for using namespace godot;
#include <godot_cpp/core/class_db.hpp>

// Needed for the INFO_PRINT macro
#include <godot_cpp/variant/utility_functions.hpp>

//Including <queue> gives you access to std::queue, which is a FIFO (First-In, First-Out) container adapter
#include <queue>

// Needed by linux g++ compiler
#include <string>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace godot {

    // The class inherits from Object because it is a utility class
    class LogFileWriter : public Object {

        // Macro to register the class with Godot's type system.
        GDCLASS(LogFileWriter, Object);

    public:

        // Keep the enum for internal C++ use
        enum LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, FATAL = 4 };

        // Generic log function and the log_level setter exported to GDSCRIPT
        // Sets the minimum log level below which logs are not stored
        void set_min_level(int p_level);

        // Perform a log file reset.
        void resetLogFile();

        // LogFileWriter is a singleton; this is the method that returns the single instance of the object.
        // If it is nullptr, it is instantiated.
        // This method should be used instead of the constructor to get the object
        static LogFileWriter* get_singleton() { 
            if (singleton == nullptr) {
                singleton = new LogFileWriter(); 
            }
            return singleton;
        }

        // Constructor & Destructor
        LogFileWriter();
        ~LogFileWriter();

        // Internal C++ macro-friendly log. For use in log_gd (gdscript) and LOG_XXX macros in C++
        // It is called by log_gd which is called from GDSCRIPT
        // __FILE__ returns a literal string char*, but the parameter p_file is std::string&
        void _log_internal(LogLevel p_level, const std::string& p_msg, const std::string& p_file, int p_line, bool isStdOutput);

        // This function would add a log to the FIFO structure of LogEntry from GDSCRIPT
        // Call _log_internal to do the job in C++
        // Although it can be used independently, it acts as a bridge for the info(), warn(), and error() functions.
        // The parameters are of type GDSCRIPT since this function is only used from within GDSCRIPT.
        // The parameters p_file and p_line have a default value, but when calling it from GDSCRIPT, you could pass it the file and the line where the error occurred.
        void log_gd(int p_level, const String& p_msg, const String& p_file = "GDScript", int p_line = 0, bool isStdOutput = true);

        // Final specific log functions to display info, warning, or error messages from GDSCRIPT
        // The parameters are of type GDSCRIPT since this function is only used from within GDSCRIPT.
        // The parameters p_file and p_line have a default value, but when calling it from GDSCRIPT, you could pass it the file and the line where the error occurred.
        void debug(const String& p_msg, const String& p_file = "GDScript", int p_line = 0, bool isStdOutput = true) { log_gd(DEBUG, p_msg, p_file, p_line, isStdOutput); }        
        void info(const String& p_msg, const String& p_file = "GDScript", int p_line = 0, bool isStdOutput = true) { log_gd(INFO, p_msg, p_file, p_line, isStdOutput); }
        void warn(const String& p_msg, const String& p_file = "GDScript", int p_line = 0, bool isStdOutput = true) { log_gd(WARN, p_msg, p_file, p_line, isStdOutput); }
        void error(const String& p_msg, const String& p_file = "GDScript", int p_line = 0, bool isStdOutput = true) { log_gd(ERROR, p_msg, p_file, p_line, isStdOutput); }
        void fatal(const String& p_msg, const String& p_file = "GDScript", int p_line = 0, bool isStdOutput = true) { log_gd(FATAL, p_msg, p_file, p_line, isStdOutput); }

    protected:

        // Method to bind properties and methods to Godot   
        static void _bind_methods();

    private:

        // Log input object structure
        struct LogEntry {
            int level;                  // LOG Level
            std::string message;        // LOG message
            std::string timestamp;      // LOG timestamp
            std::string file;           // File that throw the message
            int line;                   // File's line where the message was thrown
            bool isStdOutput;           // the message should appear in the standard output
        };

        // Singleton pointer
        static inline LogFileWriter* singleton = nullptr;

        // Private methods
        // Method that manages the writing of a log when it occurs
        void process_logs();

        // a method that provides us with the date professionally
        std::string get_timestamp();



        // Set of objects required for the logging procedure of this class

        //std::atomic<int> ensures that the operation happens as a single, indivisible unit.
        std::atomic<int> min_level{0};
    
        // FIFO queue of LogEntry objects
        std::queue<LogEntry> log_queue;

        // Building a Thread-Safe Queue.
        std::mutex queue_mutex;

        // Final piece to the puzzle for a high-performance Producer-Consumer system!
        std::condition_variable cv;

        // object that actually runs your code in parallel.
        std::thread worker_thread;

        //std::atomic<int> ensures that the operation happens as a single, indivisible unit.
        std::atomic<bool> should_exit{false};
    };
};

// This macro is the "bridge" that tells Godot's Variant system that your custom C++ enum should be treated as a first-class citizen.
VARIANT_ENUM_CAST(godot::LogFileWriter::LogLevel);

// C++ Helper Macros. To use in pure C++
#define LOG_DEBUG(m, file, line, isStdOutput) \
    ((void)(LogFileWriter::get_singleton()->_log_internal( \
        LogFileWriter::LogLevel::DEBUG, (m), file, line, isStdOutput)))

#define LOG_INFO(m, file, line, isStdOutput) \
    ((void)(LogFileWriter::get_singleton()->_log_internal( \
        LogFileWriter::LogLevel::INFO, (m), file, line, isStdOutput)))

#define LOG_WARN(m, file, line, isStdOutput) \
    ((void)(LogFileWriter::get_singleton()->_log_internal( \
        LogFileWriter::LogLevel::WARN, (m), file, line, isStdOutput)))

#define LOG_ERROR(m, file, line, isStdOutput) \
    ((void)(LogFileWriter::get_singleton()->_log_internal( \
        LogFileWriter::LogLevel::ERROR, (m), file, line, isStdOutput)))

#define LOG_FATAL(m, file, line, isStdOutput) \
    ((void)(LogFileWriter::get_singleton()->_log_internal( \
        LogFileWriter::LogLevel::FATAL, (m), file, line, isStdOutput)))

// My own INFO_PRINT macro
#define INFO_PRINT(m_msg) \
    godot::UtilityFunctions::print(m_msg);

#define WARNING_PRINT(m_msg) \
    godot::UtilityFunctions::print(m_msg);
    
#define ERROR_PRINT(m_msg) \
    godot::UtilityFunctions::::printerr(m_msg);

#endif

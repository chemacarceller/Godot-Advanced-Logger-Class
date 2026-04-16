#ifndef LOGFILEWRITER_REGISTER_TYPES_H
#define LOGFILEWRITER_REGISTER_TYPES_H

// That header (class_db.hpp) is essential for registering your classes in the engine
// Needed for using namespace godot;
#include <godot_cpp/core/class_db.hpp>


using namespace godot;

void initialize_LogFileWriter(ModuleInitializationLevel p_level);
void uninitialize_LogFileWriter(ModuleInitializationLevel p_level);

#endif 
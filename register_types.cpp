#include "register_types.h"
#include "LogFileWriter.h"

// Needed for Engine::...
// It contains the specific definition of the Engine class.
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void initialize_LogFileWriter(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // El nombre que pongas aquí es el que GDScript reconocerá como TIPO
    ClassDB::register_class<LogFileWriter>(); 
        
    LogFileWriter* logger = memnew(LogFileWriter);
    Engine::get_singleton()->register_singleton("MyLogger", logger);
}

void uninitialize_LogFileWriter(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        
        // 1. Remove it from the engine
        Engine::get_singleton()->unregister_singleton("MyLogger");
    }
}


// Moving into the entry point of your GDExtension library! This is where your C++ binary shakes hands with the Godot engine.
// Typically, you are setting up the gdextension_registration_init function. Here is the standard boilerplate that follows that opening line
extern "C" {
    GDExtensionBool GDE_EXPORT
    LogFileWriter_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization*r_initialization) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address,p_library, r_initialization);
        init_obj.register_initializer(initialize_LogFileWriter);
        init_obj.register_terminator(uninitialize_LogFileWriter);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
        return init_obj.init();
    }
}
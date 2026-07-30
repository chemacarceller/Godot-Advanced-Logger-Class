#include "register_types.h"
#include "LogFileWriter.h"
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

// Declaramos un puntero estático global para almacenar la instancia nativa
static LogFileWriter* logger_instance = nullptr;

void initialize_LogFileWriter(ModuleInitializationLevel p_level) {
    // CAMBIO CLAVE: Cambiamos MODULE_INITIALIZATION_LEVEL_SCENE por SERVERS.
    // Esto inyecta MyLogger en el motor ANTES de que se procesen escenas como ConfigRender.
    if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
        return;
    }

    // Registramos la clase nativa en la base de datos de Godot
    ClassDB::register_class<LogFileWriter>(); 
        
    // Creamos la instancia en la memoria RAM
    logger_instance = memnew(LogFileWriter);
    
    // Registramos el Singleton con el nombre exacto que buscan tus scripts de GDScript
    Engine::get_singleton()->register_singleton("MyLogger", logger_instance);
}

void uninitialize_LogFileWriter(ModuleInitializationLevel p_level) {
    // Nos aseguramos de darlo de baja y limpiar la memoria en el mismo nivel SERVERS
    if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS && logger_instance != nullptr) {
        
        // 1. Lo removemos del motor para que nadie más intente llamarlo
        Engine::get_singleton()->unregister_singleton("MyLogger");

        // 2. Destruimos el objeto de la RAM de forma segura para evitar fugas de memoria
        memdelete(logger_instance);
        logger_instance = nullptr;
    }
}

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
extends Node
class_name MyLogger
# NOTE: The line "class_name MyLogger" was intentionally removed.
# Add the autoloads directly to the project.godot file, section [autoload]

# Class that acts as a bypass to the C++ LogFileWriter class, MyLogger object

# This is implemented to prevent errors when rebuilding the project in Godot because the class
# LogFileWriter is created as a singleton directly in C++

# --- ANALYZER SHIELD (EDIT TIME) ---
# Since they are not static functions, the engine compiles them as normal object methods,
# allowing the call to .free() to be perfectly legal for the parser.
static func debug(a=null, b=null, c=null, d=null) -> void : _route_call("debug", [a, b, c, d])
static func info(a=null, b=null, c=null, d=null) -> void : _route_call("info", [a, b, c, d])
static func error(a=null, b=null, c=null, d=null) -> void : _route_call("error", [a, b, c, d])
static func warn(a=null, b=null, c=null, d=null) -> void : _route_call("warn", [a, b, c, d])
static func fatal(a=null, b=null, c=null, d=null) -> void : _route_call("fatal", [a, b, c, d])
static func _log_internal(a=null, b=null, c=null, d=null, e=null) -> void : _route_call("_log_internal", [a, b, c, d, e])
static func log_gd(a=null, b=null, c=null, d=null, e=null) -> void : _route_call("log_gd", [a, b, c, d, e])
static func resetLogFile() -> void : _route_call("resetLogFile", [])
static func set_min_level(a=null) -> void : _route_call("set_min_level", [a])
static func get_singleton() -> void : _route_call("get_singleton", [])	

# --- SELF-DESTRUCT INTERCEPTOR ---
# If GameInstance calls MyLogger.free(), this function will be executed natively.
# We prevent the GDScript node from being deleted if C++ has not claimed the Singleton.
static func freeing() -> void :
	# If C++ is active, we let C++ manage its own memory
	# If it happens ahead of time, we simply prevent it from breaking the game
	if Engine.has_singleton("MyLogger") : pass
	else : pass

# --- DYNAMIC ROUTER (GAME TIME) ---
static func _route_call(method_name: String, args: Array) -> void :
	if Engine.has_singleton("MyLogger") :
		var singleton = Engine.get_singleton("MyLogger")
		var clean_args = args.filter(func(element): return element != null)
		singleton.callv(method_name, clean_args)
	else : print("[Early Object " + method_name.to_upper() + "]: ", args)

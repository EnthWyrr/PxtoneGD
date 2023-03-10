/*
---------------------
Pxtone playback for Godot Engine
Module by EnthWyrr
Pxtone by Studio Pixel

Released under the MIT License
---------------------
*/

#include "register_types.h"

#include "core/object/class_db.h"
#include "pxtone_gd.h"

void initialize_pxtone_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<PxtoneGD>();
}

void uninitialize_pxtone_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// Nothing to do here, probably.
}

/*
---------------------
Pxtone playback for Godot Engine
Module by GFXKazos
Pxtone by Studio Pixel

Released under the MIT License
---------------------
*/

#include "register_types.h"

#include "core/class_db.h"
#include "pxtone_gd.h"

void register_pxtone_types() {
	ClassDB::register_class<PxtoneGD>();
}

void unregister_pxtone_types() {
	// Uhh, should I do something in here?
}

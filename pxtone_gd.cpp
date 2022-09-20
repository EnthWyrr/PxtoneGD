/*
---------------------
PxtoneGD
Pxtone playback for Godot Engine
Module by GFXKazos
Pxtone by Studio Pixel

Released under the MIT License
---------------------
*/

// ゴドートエンジンのピストンモジュール

#include "pxtone_gd.h"
#include "servers/audio_server.h"
#include "core/math/math_funcs.h"
#include "core/print_string.h"
#include "core/os/file_access.h"

// I/O Callbacks
static bool pxtn_read(void* user, void* destination, int32_t size, int32_t num) {
	FileAccess* file_to_read_from = (FileAccess*)user;
	int desc_size = size * num;
	void* desc = memalloc(desc_size);
	file_to_read_from->get_buffer((uint8_t*)desc, desc_size);
	memcpy(destination,desc,desc_size);
	memfree(desc);
	return true;
}

static bool pxtn_seek(void* user, int mode, int size)
{
	FileAccess* file_to_seek_from = (FileAccess*)user;
	int file_size = file_to_seek_from->get_len();
	switch (mode)
	{
		case 0:	// SEEK_SET
			file_to_seek_from->seek(size);
			break;
		case 1:	// SEEK_CUR
			file_to_seek_from->seek(file_to_seek_from->get_position() + size);
			break;
		case 2:	// SEEK_END
			file_to_seek_from->seek_end(size);
			break;
		default:
			break;
	}
	return true;
}

static bool pxtn_pos(void* user, int32_t* p_pos)
{
	FileAccess* file_to_get_pos_from = (FileAccess*)user;
	int file_pos = file_to_get_pos_from->get_position();
	if (file_pos < 0) return false;
	*p_pos = file_pos;
	return true;
}

// Class methods begin here
PxtoneGD::PxtoneGD() {
	pxtn = memnew(pxtnService(pxtn_read, NULL, pxtn_seek, pxtn_pos));
	playing = false;
	looping = true;
	volume = 1;
	if (pxtn->init() != pxtnOK) {
		ERR_FAIL_MSG("Couldn't initialize pxtone");
	}
	// Configure pxtone quality
	channels = 2;
	mix_rate = AudioServer::get_singleton()->get_mix_rate();
	pxtn->set_destination_quality(channels,mix_rate);
	// AudioServer setup
	AudioServer::get_singleton()->lock();
	mix_buffer.resize(AudioServer::get_singleton()->thread_get_mix_buffer_size());
	AudioServer::get_singleton()->unlock();
	AudioServer::get_singleton()->add_callback(_mix_audios, this);
}

PxtoneGD::~PxtoneGD() {
	if (is_playing()){
	stop();
	}
	if (pxtn) {
		memdelete(pxtn);
	}
	AudioServer::get_singleton()->remove_callback(_mix_audios, this);
}

bool PxtoneGD::mix(AudioFrame* p_buffer, int p_frames){
	p_frames *= channels;

	Vector<int16_t>pxtn_buffer;
	pxtn_buffer.resize(p_frames);

	int16_t *pt_buffer = pxtn_buffer.ptrw();
	int pt_size = pxtn_buffer.size();


	if (!pxtn->Moo(pt_buffer,pt_size*2)){
		return false;
	}

	// Mix channels
	switch (channels)
	{
		case 1:	// Mono
			for (int i=0;i<p_frames;i++){
				p_buffer[i] += (pt_buffer[i] / 32767.0);
			}
			break;
		case 2:	// Stereo
			for (int i=0;i<p_frames;i+=2){
				p_buffer[i].l = (pt_buffer[i] / 32767.0);
				p_buffer[i].r = (pt_buffer[i+1] / 32767.0);
				p_buffer--;
			}
			break;
		default:
			print_error("Invalid channel count.");
			return false;
	}
	return true;
}

// Mix audio in audio thread
void PxtoneGD::_mix_audio() {
	if (playing) {
		AudioFrame *buffer = mix_buffer.ptrw();
		int buffer_size = mix_buffer.size();
		memset(buffer,0,buffer_size*sizeof(AudioFrame));

		if (!mix(buffer,buffer_size)){
			stop();
			emit_signal("finished");
			return;
		}

		// Output to AudioServer
		int bus_index = AudioServer::get_singleton()->thread_find_bus_index(bus);
		int channels = AudioServer::get_singleton()->get_channel_count();
		for (int c = 0; c < channels; c++) {
			AudioFrame *target = AudioServer::get_singleton()->thread_get_channel_mix_buffer(bus_index, c);
			for (int i = 0; i < buffer_size; i++) {
				target[i] += buffer[i];
			}
		}
	}
}

Error PxtoneGD::load_tune(const String file_path) {
	Error load_error = OK;
	pxtnERR pxtn_err = pxtnERR_VOID;
	stop();
	pxtn->tones_clear();
	// Ensure that the file can be loaded
	FileAccess *tune_file = FileAccess::open(file_path, FileAccess::READ, &load_error);
	if (load_error != OK)
	{
		goto finish;
	}
	else
	{
		pxtn_err = pxtn->read(tune_file);
		if (pxtn_err != pxtnOK)
		{
			load_error = FAILED;
			ERR_PRINT("PxTone Error: " + String(pxtnError_get_string(pxtn_err)));
			goto finish;
		}
		pxtn_err = pxtn->tones_ready();
		if (pxtn_err != pxtnOK)
		{
			load_error = FAILED;
			ERR_PRINT("PxTone Error: " + String(pxtnError_get_string(pxtn_err)));
			goto finish;
		}

	}
finish:
	if (tune_file)
	{
		memdelete(tune_file);
	}
	return load_error;
}

void PxtoneGD::start() {
	pxtnVOMITPREPARATION prep = { 0 };
	prep.flags |= (looping ? pxtnVOMITPREPFLAG_loop : 0);
	prep.start_pos_float = 0;
	prep.master_volume = volume;
	if (!pxtn->moo_preparation(&prep)) {	// Turns out, that having this function inside a error macro doesn't work...it just crashes.
		playing = false;
		print_error("Couldn't prepare playback!");
		return;
	}
	playing = true;
}

void PxtoneGD::stop() {
	playing = false;
}

void PxtoneGD::set_loop(const bool set_loop) {
	looping = set_loop;
	pxtn->moo_set_loop(looping);
}

bool PxtoneGD::get_loop() const {
	return looping;
}

void PxtoneGD::set_volume(const float new_volume) {
	volume = new_volume;
	pxtn->moo_set_master_volume(volume);
}

float PxtoneGD::get_volume() const {
	return volume;
}

void PxtoneGD::set_channels(const int new_ch) {
	ERR_FAIL_COND_MSG(new_ch > 2 || new_ch < 1,"Invalid number of channels.");	// Above 2 or below 1.
	channels = new_ch;
	pxtn->set_destination_quality(channels, mix_rate);
}

int PxtoneGD::get_channels() const {
	return channels;
}

bool PxtoneGD::is_playing() const {
	return playing;
}

String PxtoneGD::get_tune_name() const {
	return String(pxtn->text->get_name_buf(0));
}

String PxtoneGD::get_tune_comment() const {
	return String(pxtn->text->get_comment_buf(0));
}

void PxtoneGD::fadeout_tune(const float secs) {
	pxtn->moo_set_fade(-1, secs);	// -1 fades out the tune
}

// Taken from AudioStreamPlayer (audio_stream_player.cpp)

void PxtoneGD::set_bus(const StringName &p_bus) {
	//if audio is active, must lock this
	AudioServer::get_singleton()->lock();
	bus = p_bus;
	AudioServer::get_singleton()->unlock();
}
StringName PxtoneGD::get_bus() const {
	for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
		if (AudioServer::get_singleton()->get_bus_name(i) == bus) {
			return bus;
		}
	}
	return "Master";
}

void PxtoneGD::_validate_property(PropertyInfo &property) const {
	if (property.name == "bus") {

		String options;
		for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
			if (i > 0)
				options += ",";
			String name = AudioServer::get_singleton()->get_bus_name(i);
			options += name;
		}
		property.hint_string = options;
	}
}

void PxtoneGD::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_tune","tune_path"), &PxtoneGD::load_tune);
	ClassDB::bind_method(D_METHOD("play"), &PxtoneGD::start); // play() is less confusing
	ClassDB::bind_method(D_METHOD("stop"), &PxtoneGD::stop);
	ClassDB::bind_method(D_METHOD("set_loop", "loop"), &PxtoneGD::set_loop);
	ClassDB::bind_method(D_METHOD("get_loop"), &PxtoneGD::get_loop);
	ClassDB::bind_method(D_METHOD("set_volume", "volume"), &PxtoneGD::set_volume);
	ClassDB::bind_method(D_METHOD("get_volume"), &PxtoneGD::get_volume);
	ClassDB::bind_method(D_METHOD("set_channels", "channels"), &PxtoneGD::set_channels);
	ClassDB::bind_method(D_METHOD("get_channels"), &PxtoneGD::get_channels);
	ClassDB::bind_method(D_METHOD("is_playing"), &PxtoneGD::is_playing);
	ClassDB::bind_method(D_METHOD("get_tune_name"), &PxtoneGD::get_tune_name);
	ClassDB::bind_method(D_METHOD("get_tune_comment"), &PxtoneGD::get_tune_comment);
	ClassDB::bind_method(D_METHOD("fadeout_tune","fadeout_secs"), &PxtoneGD::fadeout_tune);
	ClassDB::bind_method(D_METHOD("set_bus", "bus"), &PxtoneGD::set_bus);
	ClassDB::bind_method(D_METHOD("get_bus"), &PxtoneGD::get_bus);
	// Properties
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "looping", PROPERTY_HINT_NONE), "set_loop", "get_loop");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "volume", PROPERTY_HINT_RANGE, "0,1"), "set_volume", "get_volume");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "channels", PROPERTY_HINT_RANGE, "1,2"), "set_channels", "get_channels");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "bus", PROPERTY_HINT_ENUM, ""), "set_bus", "get_bus");
	// Signals
	ADD_SIGNAL(MethodInfo("finished"));
}

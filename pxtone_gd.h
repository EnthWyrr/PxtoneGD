/*
---------------------
PxtoneGD
Pxtone playback for Godot Engine
Module by GFXKazos
Pxtone by Studio Pixel

Released under the MIT License
---------------------
*/

#ifndef PXTONE_GD_H
#define PXTONE_GD_H

#include "core/reference.h"
#include "scene/main/node.h"
#include "core/math/audio_frame.h"
#include "pxtone/pxtnService.h"

class PxtoneGD : public Node
{
	GDCLASS(PxtoneGD,Node)
	PxtoneGD();
	~PxtoneGD();
	public:
		Error load_tune(const String file_path);
		void start();
		void stop();
		void set_loop(const bool set_loop);
		bool get_loop() const;
		void set_volume(const float new_volume);
		float get_volume() const;
		void set_channels(const int new_ch);
		int get_channels() const;
		bool is_playing() const;
		String get_tune_name() const;
		String get_tune_comment() const;
		void fadeout_tune(const float secs);

	private:
		static void _mix_audios(void *self) { reinterpret_cast<PxtoneGD *>(self)->_mix_audio(); }
		bool mix(AudioFrame* p_buffer, int p_frames);
		void _mix_audio();
		pxtnService *pxtn;
		Vector<AudioFrame>mix_buffer;
		int buffer_size;
		int channels;
		int mix_rate;
		bool playing;
		bool looping;
		float volume;

	protected:
		static void _bind_methods();
};

#endif

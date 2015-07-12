#ifndef YASSPFC_ANIME_H
#define YASSPFC_ANIME_H

#include "yasspfc_ssbp.h"
#include "yasspfc_partstate.h"


namespace yasspfc {

struct SSFrameData;

struct SSAnime {
	SSBP* m_data;

	const SSBPAnimePack* m_anime_pack;
	const SSBPAnime* m_anime;

	const SSPartState* m_parent_part_state;
	SSPartState* m_part_state;
	SSAnime* m_instance;
	int16_t m_part_num;
	int16_t m_instance_num;

	int32_t m_elapse_frame_count;
	float m_delta_frame_frac;

	float m_time_scale;
	int16_t m_key_frame_index;
	int16_t m_start_frame_index;
	int16_t m_end_frame_index;
	int16_t m_loop_num;
	int16_t m_loop_flags;
	int16_t m_frame_index;
	bool m_instance_enabled;

	void extract_frame_data(SSStream16& strm, SSFrameData& frame_data);
	void update_parts();
	void update_part(const SSFrameData& frame_data);
	void update_part_uv(const SSFrameData& frame_data, SSPartState* part_state);
	void update_transform();

	void release_part();
	void parse_part();
	void reset();

	void dtor();
	void ctor(SSBP* data);
	void ctor_instance(SSBP* data, const SSPartState* part_state, const char* reference_name);

	void set_anime(const SSBPAnimePack* anime_pack, const char* anime_name);
	void set_anime(const char* anime_pack_name, const char* anime_name);

	void set_frame(int16_t index);
	void set_frame_abs(int32_t elapse_frame_count);
	void set_frame_instance(int32_t parent_frame_index);

	void update(float dt);
	void update_instance(float delta_frame);

#if COCOS2D_VERSION >= 0x00030000
	void draw(const cocos2d::Mat4& transform, uint32_t color);
#else
	void draw(const kmMat4& transform, uint32_t color);
#endif
	void draw_parts(uint32_t color);

	void set_loop(bool loop);
};

}

#endif

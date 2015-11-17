#ifndef YASSPFC_ANIME_H
#define YASSPFC_ANIME_H

#include "yasspfc_ssbp.h"
#include "yasspfc_partstate.h"


namespace yasspfc {

class SSPlayer;

struct SSFrameData {
	uint32_t flags;
	int16_t part_index;
	int16_t cell_index;

	int16_t x;
	int16_t y;
	float anchor_x;
	float anchor_y;
	float rotation_z;
	float scale_x;
	float scale_y;
	float size_x;
	float size_y;

	float uv_move_x;
	float uv_move_y;
	float uv_rotation;
	float uv_scale_x;
	float uv_scale_y;

	int16_t vt_lt_x;
	int16_t vt_lt_y;
	int16_t vt_lb_x;
	int16_t vt_lb_y;
	int16_t vt_rt_x;
	int16_t vt_rt_y;
	int16_t vt_rb_x;
	int16_t vt_rb_y;

	uint8_t opacity;
	uint8_t color_blend_type;
	uint8_t color_blend_vertex_flags;
	// pad

	SSColor lt_color;
	float lt_color_blend_rate;
	SSColor lb_color;
	float lb_color_blend_rate;
	SSColor rt_color;
	float rt_color_blend_rate;
	SSColor rb_color;
	float rb_color_blend_rate;

	int16_t instance_key_frame_index;
	int16_t instance_start_frame_index;
	int16_t instance_end_frame_index;
	// pad
	float instance_time_scale;
	int16_t instance_loop_num;
	int16_t instance_loop_flags;

	uint32_t color;// for customization. same as color and opacity of Node.
	uint8_t alpha_blend_type;// for customization.
};

struct SSAnime {
	SSBP* m_data;
	SSPlayer* m_player;

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
	bool m_end;
	bool m_first_frame;

	void extract_frame_data(SSStream16& strm, SSFrameData& frame_data);
	void update_parts();
	void update_part(const SSFrameData& frame_data);
	void update_part_uv(const SSFrameData& frame_data, SSPartState* part_state);
	void update_transform();

	void release_part();
	void parse_part();
	void reset();

	void dtor();
	void ctor(SSPlayer* player, SSBP* data);
	void ctor_instance(SSPlayer* player, SSBP* data, const SSPartState* part_state, const char* reference_name);

	void set_anime(const char* reference_name);
	void set_anime(const SSBPAnimePack* anime_pack, const char* anime_name);
	void set_anime(const char* anime_pack_name, const char* anime_name);

	void set_frame(bool force_update, int16_t index);
	void set_frame_abs(bool force_update, int32_t elapse_frame_count);
	void set_frame_instance(bool force_update, int32_t parent_frame_index);

	void update(float dt);
	void update_instance(bool force_update, float delta_frame);

	void draw(const YASSPFC_CC_MAT4& transform);
	void draw_parts();

	void set_loop(int8_t loop);
    void rewind();
};

}

#endif

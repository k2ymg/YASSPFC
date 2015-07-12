#include "yasspfc_anime.h"
#include "yasspfc_renderer.h"


using namespace yasspfc;


#define SSBP_SIG 0x42505353
#define SSBP_VER 1

#define PART_FLAG_INVISIBLE         (1 <<  0)
#define PART_FLAG_FLIP_H            (1 <<  1)
#define PART_FLAG_FLIP_V            (1 <<  2)
#define PART_FLAG_CELL_INDEX        (1 <<  3)
#define PART_FLAG_POSITION_X        (1 <<  4)
#define PART_FLAG_POSITION_Y        (1 <<  5)
#define PART_FLAG_POSITION_Z        (1 <<  6)
#define PART_FLAG_ANCHOR_X          (1 <<  7)
#define PART_FLAG_ANCHOR_Y          (1 <<  8)
#define PART_FLAG_ROTATIONX         (1 <<  9)
#define PART_FLAG_ROTATIONY         (1 << 10)
#define PART_FLAG_ROTATIONZ         (1 << 11)
#define PART_FLAG_SCALE_X           (1 << 12)
#define PART_FLAG_SCALE_Y           (1 << 13)
#define PART_FLAG_OPACITY           (1 << 14)
#define PART_FLAG_COLOR_BLEND       (1 << 15)
#define PART_FLAG_VERTEX_TRANSFORM  (1 << 16)
#define PART_FLAG_SIZE_X            (1 << 17)
#define PART_FLAG_SIZE_Y            (1 << 18)
#define PART_FLAG_U_MOVE            (1 << 19)
#define PART_FLAG_V_MOVE            (1 << 20)
#define PART_FLAG_UV_ROTATION       (1 << 21)
#define PART_FLAG_U_SCALE           (1 << 22)
#define PART_FLAG_V_SCALE           (1 << 23)
#define PART_FLAG_BOUNDINGRADIUS    (1 << 24)
#define PART_FLAG_INSTANCE_KEYFRAME (1 << 25)
#define PART_FLAG_INSTANCE_START    (1 << 26)
#define PART_FLAG_INSTANCE_END      (1 << 27)
#define PART_FLAG_INSTANCE_SPEED    (1 << 28)
#define PART_FLAG_INSTANCE_LOOP     (1 << 29)
#define PART_FLAG_INSTANCE_LOOP_FLG (1 << 30)

#define INSTANCE_LOOP_FLAG_INFINITY    (1 << 0)
#define INSTANCE_LOOP_FLAG_REVERSE     (1 << 1)
#define INSTANCE_LOOP_FLAG_PINGPONG    (1 << 2)
#define INSTANCE_LOOP_FLAG_INDEPENDENT (1 << 3)

#define VERTEX_FLAG_LT  (1 << 0)
#define VERTEX_FLAG_RT  (1 << 1)
#define VERTEX_FLAG_LB  (1 << 2)
#define VERTEX_FLAG_RB  (1 << 3)
#define VERTEX_FLAG_ONE (1 << 4)

#define PARTTYPE_INVALID -1
#define PARTTYPE_NULL     0
#define PARTTYPE_NORMAL   1
//#define PARTTYPE_TEXT     2 unused/reserved
#define PARTTYPE_INSTANCE 3

#define BLEND_MIX 0
#define BLEND_MUL 1
#define BLEND_ADD 2
#define BLEND_SUB 3


void SSAnime::release_part()
{
	SSAnime* instance;

	instance = m_instance;
	if(instance != 0){
		int16_t i;

		i = m_instance_num;
		while(i--)
			instance[i].dtor();
		free(instance);

		m_instance = 0;
	}

	if(m_part_state != 0){
		free(m_part_state);
		m_part_state = 0;
	}
}

void SSAnime::parse_part()
{
	const char* data;
	int16_t part_num;
	const SSBPPart* parts;
	int16_t instance_num;

	// part state
	m_part_state = (SSPartState*)malloc(sizeof(SSPartState) * m_anime_pack->part_num);

	// instance
	data = m_data->ptr();
	parts = (const SSBPPart*)(data + m_anime_pack->part_offset);

	instance_num = 0;
	part_num = m_anime_pack->part_num;
	for(int16_t i = 0; i < part_num; i++){
		const SSBPPart* part;

		part = &parts[i];
		if(part->type == PARTTYPE_INSTANCE){
			instance_num++;
		}else{
			m_part_state[i].instance = 0;
		}

		m_part_state[i].blend = (uint8_t)part->alpha_blend_type;
	}

	m_instance_num = instance_num;
	if(instance_num > 0){
		m_instance = (SSAnime*)malloc(sizeof(SSAnime) * instance_num);

		instance_num = 0;
		for(int16_t i = 0; i < part_num; i++){
			const SSBPPart* part;

			part = &parts[i];
			if(part->type == PARTTYPE_INSTANCE){
				const char* name;

				m_part_state[i].instance = &m_instance[instance_num];

				name = (const char*)(data + part->rename_offset);
				m_instance[instance_num].ctor_instance(m_data, &m_part_state[i], name);
				instance_num++;
			}
		}
	}
}

void SSAnime::reset()
{
	m_anime_pack = 0;
	m_anime = 0;

	m_parent_part_state = 0;
	m_part_state = 0;
	m_instance = 0;
	m_part_num = 0;
	m_instance_num = 0;

	m_elapse_frame_count = 0;
	m_delta_frame_frac = 0.f;

	m_time_scale = 1.f;
	m_key_frame_index = 0;
	m_start_frame_index = 0;
	m_end_frame_index = 0;
	m_loop_num = 1;
	m_loop_flags = INSTANCE_LOOP_FLAG_INDEPENDENT;
	m_frame_index = 0;
	m_instance_enabled = false;
}

void SSAnime::dtor()
{
	release_part();
	m_data->release();
}

void SSAnime::ctor(SSBP* data)
{
	data->retain();
	m_data = data;

	reset();
}

void SSAnime::ctor_instance(SSBP* data, const SSPartState* part_state, const char* reference_name)
{
	ctor(data);
	m_parent_part_state = part_state;

	std::string anime_pack_name;
	const char* anime_name;

	anime_name = strchr(reference_name, '/');
	anime_pack_name.assign(reference_name, anime_name - reference_name);
	anime_name++;

	set_anime(anime_pack_name.c_str(), anime_name);
}

void SSAnime::set_anime(const char* anime_pack_name, const char* anime_name)
{
	const SSBPAnimePack* anime_pack;

	release_part();
	if(m_anime_pack != 0)
		reset();

	anime_pack = m_data->find_anime_pack(anime_pack_name);
	if(anime_pack == 0)
		return;

	set_anime(anime_pack, anime_name);
}

void SSAnime::set_anime(const SSBPAnimePack* anime_pack, const char* anime_name)
{
	const SSBPAnime* anime;

	anime = m_data->find_anime(anime_pack, anime_name);
	if(anime == 0)
		return;

	m_anime_pack = anime_pack;
	m_anime = anime;

	m_end_frame_index = anime->frame_num - 1;

	parse_part();

	set_frame(m_start_frame_index);
}

struct yasspfc::SSFrameData {
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
};

void SSAnime::extract_frame_data(
	SSStream16& strm,
	SSFrameData& frame_data)
{
	uint32_t flags;
	const SSBPAnimeInitialData* ini;

	{
		int16_t part_index;

		part_index = strm.i16();
		frame_data.part_index = part_index;

		ini = (const SSBPAnimeInitialData*)
			(m_data->ptr() + m_anime->default_offset) + part_index;
	}

	flags = strm.i32();
	frame_data.flags = flags;

	frame_data.cell_index = (flags & PART_FLAG_CELL_INDEX) ? strm.i16() : ini->cell_index;

	frame_data.x = (flags & PART_FLAG_POSITION_X) ? strm.i16() : ini->position_x;
	frame_data.y = (flags & PART_FLAG_POSITION_Y) ? strm.i16() : ini->position_y;
	if(flags & PART_FLAG_POSITION_Z)
		strm.i16();// notsupport

	frame_data.anchor_x = (flags & PART_FLAG_ANCHOR_X) ? strm.f32() : ini->anchor_x;
	frame_data.anchor_y = (flags & PART_FLAG_ANCHOR_Y) ? strm.f32() : ini->anchor_y;

	if(flags & PART_FLAG_ROTATIONX)
		strm.f32();// notsupport
	if(flags & PART_FLAG_ROTATIONY)
		strm.f32();// notsupport
	frame_data.rotation_z = (flags & PART_FLAG_ROTATIONZ) ? strm.f32() : ini->rotation_z;

	frame_data.scale_x = (flags & PART_FLAG_SCALE_X) ? strm.f32() : ini->scale_x;
	frame_data.scale_y = (flags & PART_FLAG_SCALE_Y) ? strm.f32() : ini->scale_y;

	frame_data.opacity = (uint8_t)((flags & PART_FLAG_OPACITY) ? strm.i16() : ini->opacity);

	frame_data.size_x = (flags & PART_FLAG_SIZE_X) ? strm.f32() : ini->size_x;
	frame_data.size_y = (flags & PART_FLAG_SIZE_Y) ? strm.f32() : ini->size_y;

	frame_data.uv_move_x = (flags & PART_FLAG_U_MOVE) ? strm.f32() : ini->uv_move_x;
	frame_data.uv_move_y = (flags & PART_FLAG_V_MOVE) ? strm.f32() : ini->uv_move_y;
	frame_data.uv_rotation = (flags & PART_FLAG_UV_ROTATION) ? strm.f32() : ini->uv_rotation;
	frame_data.uv_scale_x = (flags & PART_FLAG_U_SCALE) ? strm.f32() : ini->uv_scale_x;
	frame_data.uv_scale_y = (flags & PART_FLAG_V_SCALE) ? strm.f32() : ini->uv_scale_y;

	if(flags & PART_FLAG_BOUNDINGRADIUS)
		strm.f32();// notsupport

	if(flags & PART_FLAG_VERTEX_TRANSFORM){
		int16_t flg = strm.i16();

		if(flg & VERTEX_FLAG_LT){
			frame_data.vt_lt_x = strm.i16();
			frame_data.vt_lt_y = strm.i16();
		}else{
			frame_data.vt_lt_x = 0;
			frame_data.vt_lt_y = 0;
		}
		if(flg & VERTEX_FLAG_RT){
			frame_data.vt_rt_x = strm.i16();
			frame_data.vt_rt_y = strm.i16();
		}else{
			frame_data.vt_rt_x = 0;
			frame_data.vt_rt_y = 0;
		}
		if(flg & VERTEX_FLAG_LB){
			frame_data.vt_lb_x = strm.i16();
			frame_data.vt_lb_y = strm.i16();
		}else{
			frame_data.vt_lb_x = 0;
			frame_data.vt_lb_y = 0;
		}
		if(flg & VERTEX_FLAG_RB){
			frame_data.vt_rb_x = strm.i16();
			frame_data.vt_rb_y = strm.i16();
		}else{
			frame_data.vt_rb_x = 0;
			frame_data.vt_rb_y = 0;
		}
	}

	if(flags & PART_FLAG_COLOR_BLEND){
		uint16_t flg = (uint16_t)strm.i16();
		uint8_t blend_type = (uint8_t)(flg);
		uint8_t vertex_flag = (uint8_t)(flg >> 8);

		frame_data.color_blend_type = blend_type;
		frame_data.color_blend_vertex_flags = vertex_flag;

		if(vertex_flag & VERTEX_FLAG_ONE){
			uint32_t c;

			frame_data.lt_color_blend_rate = strm.f32();
			c = strm.i32();
			frame_data.lt_color.rgba = (c << 8) | (c >> 24);
		}else{
			// Note: not need to check each flag. always contain all vertex. faq.
			uint32_t c;

			// VERTEX_FLAG_LT
			frame_data.lt_color_blend_rate = strm.f32();
			c = strm.i32();
			frame_data.lt_color.r = (c >> 16) & 0xff;// ARGB -> ABGR
			frame_data.lt_color.g = (c >>  8) & 0xff;
			frame_data.lt_color.b = (c >>  0) & 0xff;
			frame_data.lt_color.a = (c >> 24) & 0xff;

			// VERTEX_FLAG_RT
			frame_data.rt_color_blend_rate = strm.f32();
			c = strm.i32();
			frame_data.rt_color.r = (c >> 16) & 0xff;
			frame_data.rt_color.g = (c >>  8) & 0xff;
			frame_data.rt_color.b = (c >>  0) & 0xff;
			frame_data.rt_color.a = (c >> 24) & 0xff;

			// VERTEX_FLAG_LB
			frame_data.lb_color_blend_rate = strm.f32();
			c = strm.i32();
			frame_data.lb_color.r = (c >> 16) & 0xff;
			frame_data.lb_color.g = (c >>  8) & 0xff;
			frame_data.lb_color.b = (c >>  0) & 0xff;
			frame_data.lb_color.a = (c >> 24) & 0xff;

			// VERTEX_FLAG_RB
			frame_data.rb_color_blend_rate = strm.f32();
			c = strm.i32();
			frame_data.rb_color.r = (c >> 16) & 0xff;
			frame_data.rb_color.g = (c >>  8) & 0xff;
			frame_data.rb_color.b = (c >>  0) & 0xff;
			frame_data.rb_color.a = (c >> 24) & 0xff;
		}
	}

	if(flags & PART_FLAG_INSTANCE_KEYFRAME){
		// Note: again, always contain all info. faq.
		frame_data.instance_key_frame_index = strm.i16();
		frame_data.instance_start_frame_index = strm.i16();
		frame_data.instance_end_frame_index = strm.i16();
		frame_data.instance_time_scale = strm.f32();
		frame_data.instance_loop_num = strm.i16();
		frame_data.instance_loop_flags = strm.i16();
	}
}


static void make_center(SSPartState* part_state)
{
#if 1
	// simple
	{
		float x = (part_state->lt.x + part_state->lb.x + part_state->rt.x + part_state->rb.x) * 0.25f;
		float y = (part_state->lt.y + part_state->lb.y + part_state->rt.y + part_state->rb.y) * 0.25f;
		part_state->cc.x = x;
		part_state->cc.y = y;
	}
#else
	// exact
	// See: SpriteStudio5-SDK/ssplayer_animedecode.cpp/CoordinateGetDiagonalIntersection function
	{
		float x0 = (part_state->lt.x + part_state->rt.x) * 0.5f;
		float y0 = (part_state->lt.y + part_state->rt.y) * 0.5f;
		float x1 = (part_state->lt.x + part_state->lb.x) * 0.5f;
		float y1 = (part_state->lt.y + part_state->lb.y) * 0.5f;
		float x2 = (part_state->rb.x + part_state->rt.x) * 0.5f;
		float y2 = (part_state->rb.y + part_state->rt.y) * 0.5f;
		float x3 = (part_state->rb.x + part_state->lb.x) * 0.5f;
		float y3 = (part_state->rb.y + part_state->lb.y) * 0.5f;


		float c1 = (y1 - y2) * (x1 - x0) - (x1 - x2) * (y1 - y0);
		float c2 = (x3 - x0) * (y1 - y0) - (y3 - y0) * (x1 - x0);
		float c3 = (x3 - x0) * (y1 - y2) - (y3 - y0) * (x1 - x2);

		float x = 0;
		float y = 0;
		if(!(c3 <= 0 && c3 >=0)){
			float ca = c1 / c3;
			float cb = c2 / c3;

			if(
				((0.f <= ca) && (ca <= 1.f)) &&
				((0.f <= cb) && (cb <= 1.f))
			){
				x = x0 + ca * (x3 - x0);
				y = y0 + ca * (y3 - y0);
			}
		}
		part_state->cc.x = x;
		part_state->cc.y = y;
	}
#endif

	// uv
	{
		float u = (part_state->lt.uv.u + part_state->lb.uv.u + part_state->rt.uv.u + part_state->rb.uv.u) * 0.25f;
		float v = (part_state->lt.uv.v + part_state->lb.uv.v + part_state->rt.uv.v + part_state->rb.uv.v) * 0.25f;
		part_state->cc.uv.u = u;
		part_state->cc.uv.v = v;
	}

	// color-blend
	if(part_state->color_blend == SSPartBlendPerVertex){
		uint32_t c0 = part_state->lt.color.rgba;
		uint32_t c1 = part_state->lb.color.rgba;
		uint32_t c2 = part_state->rt.color.rgba;
		uint32_t c3 = part_state->rb.color.rgba;

		uint32_t rb = ((
			((c0 >> 8) & 0xff00ff) + ((c1 >> 8) & 0xff00ff) + 
			((c2 >> 8) & 0xff00ff) + ((c3 >> 8) & 0xff00ff)) >> 2) & 0xff00ff;
		uint32_t ga = ((
			(c0 & 0xff00ff) + (c1 & 0xff00ff) +
			(c2 & 0xff00ff) + (c3 & 0xff00ff)) >> 2) & 0xff00ff;
		part_state->cc.color.rgba = (rb << 8) | ga;

		part_state->cc.color_blend_rate = (
			part_state->lt.color_blend_rate + 
			part_state->lb.color_blend_rate + 
			part_state->rt.color_blend_rate +
			part_state->rb.color_blend_rate) * 0.25;
	}
}

void SSAnime::update_part(const SSFrameData& frame)
{
	uint32_t flags;
	const char* data;
	const SSBPHeader* header;
	SSPartState* part_state;
	bool need_center;

	data = m_data->ptr();
	header = (const SSBPHeader*)data;

	part_state = &m_part_state[frame.part_index];

	flags = frame.flags;
	part_state->flags = 0;
	if(flags & PART_FLAG_INVISIBLE)
		part_state->flags |= SSPartFlagInvisible;

	part_state->opacity = frame.opacity;

	// transform
#if COCOS2D_VERSION >= 0x00030000
	part_state->transform = cocos2d::AffineTransformMakeIdentity();
	part_state->transform = cocos2d::AffineTransformTranslate(
		part_state->transform, frame.x / 10.0f, frame.y / 10.0f);
	if(frame.rotation_z != 0){
		float rz = CC_DEGREES_TO_RADIANS(frame.rotation_z);
		part_state->transform = cocos2d::AffineTransformRotate(part_state->transform, rz);
	}
	if(frame.scale_x != 1.f || frame.scale_y != 1.f){
		part_state->transform = cocos2d::AffineTransformScale(part_state->transform,
		frame.scale_x, frame.scale_y);
	}
#else
	part_state->transform = cocos2d::CCAffineTransformMakeIdentity();
	part_state->transform = cocos2d::CCAffineTransformTranslate(
		part_state->transform, frame.x / 10.0f, frame.y / 10.0f);
	if(frame.rotation_z != 0){
		float rz = CC_DEGREES_TO_RADIANS(frame.rotation_z);
		part_state->transform = cocos2d::CCAffineTransformRotate(part_state->transform, rz);
	}
	if(frame.scale_x != 1.f || frame.scale_y != 1.f){
		part_state->transform = cocos2d::CCAffineTransformScale(part_state->transform,
		frame.scale_x, frame.scale_y);
	}
#endif

	part_state->cell_index = frame.cell_index;
	if(frame.cell_index < 0){
		if(part_state->instance){
			SSAnime* instance = part_state->instance;

			if(frame.instance_key_frame_index >= m_frame_index)
				instance->m_instance_enabled = true;
			instance->m_key_frame_index = frame.instance_key_frame_index;
			instance->m_start_frame_index = frame.instance_start_frame_index;
			instance->m_end_frame_index = frame.instance_end_frame_index;
			instance->m_time_scale = frame.instance_time_scale;
			instance->m_loop_num = frame.instance_loop_num;
			instance->m_loop_flags = frame.instance_loop_flags;
		}
		return;// NULL or Instance part.
	}

	update_part_uv(frame, part_state);

	need_center = false;

	// vertex color
	if(flags & PART_FLAG_COLOR_BLEND){
		part_state->color_blend_type = frame.color_blend_type;

		if(frame.color_blend_vertex_flags & VERTEX_FLAG_ONE){
			part_state->color_blend = SSPartBlendSingle;

			part_state->lt.color.rgba = frame.lt_color.rgba;
			part_state->lt.color_blend_rate = frame.lt_color_blend_rate;
		}else{
			part_state->color_blend = SSPartBlendPerVertex;
			need_center = true;

			part_state->lt.color.rgba = frame.lt_color.rgba;
			part_state->lt.color_blend_rate = frame.lt_color_blend_rate;
			part_state->lb.color.rgba = frame.lb_color.rgba;
			part_state->lb.color_blend_rate = frame.lb_color_blend_rate;
			part_state->rt.color.rgba = frame.rt_color.rgba;
			part_state->rt.color_blend_rate = frame.rt_color_blend_rate;
			part_state->rb.color.rgba = frame.rb_color.rgba;
			part_state->rb.color_blend_rate = frame.rb_color_blend_rate;
		}
	}else{
		part_state->color_blend = SSPartBlendNone;
	}

	// vertex position
	{
		float width, height;
		float l, t, r, b;

		width = frame.size_x;
		height = frame.size_y;

		l = -width  * frame.anchor_x;
		t =  height * frame.anchor_y;
		r = l + width;
		b = t - height;

		part_state->lt.x = l;
		part_state->lt.y = t;
		part_state->lb.x = l;
		part_state->lb.y = b;
		part_state->rt.x = r;
		part_state->rt.y = t;
		part_state->rb.x = r;
		part_state->rb.y = b;
	}
	if(flags & PART_FLAG_VERTEX_TRANSFORM){
		need_center = true;

		part_state->lt.x += frame.vt_lt_x;
		part_state->lt.y += frame.vt_lt_y;
		part_state->lb.x += frame.vt_lb_x;
		part_state->lb.y += frame.vt_lb_y;
		part_state->rt.x += frame.vt_rt_x;
		part_state->rt.y += frame.vt_rt_y;
		part_state->rb.x += frame.vt_rb_x;
		part_state->rb.y += frame.vt_rb_y;
	}

	if(need_center){
		part_state->flags |= SSPartFlagVertexTransform;
		make_center(part_state);
	}
}

void SSAnime::update_part_uv(const SSFrameData& frame, SSPartState* part_state)
{
	const SSBPCell* cell;
	float x, y, w, h;
#if COCOS2D_VERSION >= 0x00030000
	cocos2d::Size tex_size;
#else
	cocos2d::CCSize tex_size;
#endif
	SSUV p0, p1, p2, p3;

	{
		const char* data;
		const SSBPHeader* header;
		const SSBPCellMap* cell_map;
#if COCOS2D_VERSION >= 0x00030000
		cocos2d::Texture2D* tex;
#else
		cocos2d::CCTexture2D* tex;
#endif

		data = m_data->ptr();
		header = (const SSBPHeader*)data;
		cell = (const SSBPCell*)(m_data->ptr() + header->cell_offset) + frame.cell_index;
		cell_map = (const SSBPCellMap*)(data + cell->cell_map_offset);
		tex = m_data->get_texture(cell_map->index);
		if(tex == 0){
			return;
		}
		tex_size = tex->getContentSizeInPixels();
	}


	{
		float m = frame.uv_move_x;
		float s = frame.uv_scale_x;

		x = cell->x;
		w = cell->width;
		if(m != 0.f)
			x += m * tex_size.width;
		if(s != 1.f){
			x += -s * (0.5f * w) + (0.5f * w);
			w *= s;
		}
	}
	{
		float m = frame.uv_move_y;
		float s = frame.uv_scale_y;

		y = cell->y;
		h = cell->height;
		if(m != 0.f)
			y += m * tex_size.height;
		if(s != 1.f){
			y += -s * (0.5f * h) + (0.5f * h);
			h *= s;
		}
	}

	{
		float r = frame.uv_rotation;

		if(r != 0.f){
			// 0 - 2      -y
			// | C |    -x + x
			// 1 - 3       y
			// R = clockwise
			r = CC_DEGREES_TO_RADIANS(r);
			float sa = sinf(r);
			float ca = cosf(r);

			float cx = w * 0.5f;
			float cy = h * 0.5f;

			float x_ca = cx * ca;
			float x_sa = cx * sa;
			float y_sa = cy * sa;
			float y_ca = cy * ca;

			float x3 = x_ca - y_sa;
			float y3 = x_sa + y_ca;
			float x2 = x_ca + y_sa;
			float y2 = x_sa - y_ca;
			float x0 = -x3;
			float y0 = -y3;
			float x1 = -x2;
			float y1 = -y2;

			cx += x;
			cy += y;

			p0.u = (int16_t)(x0 + cx);
			p0.v = (int16_t)(y0 + cy);
			p1.u = (int16_t)(x1 + cx);
			p1.v = (int16_t)(y1 + cy);
			p2.u = (int16_t)(x2 + cx);
			p2.v = (int16_t)(y2 + cy);
			p3.u = (int16_t)(x3 + cx);
			p3.v = (int16_t)(y3 + cy);
		}else{
			int16_t l = (int16_t)x;
			int16_t t = (int16_t)y;
			int16_t r = (int16_t)(x + w);
			int16_t b = (int16_t)(y + h);

			p0.u = l;
			p0.v = t;
			p3.u = r;
			p3.v = b;
			p1.u = l;
			p1.v = b;
			p2.u = r;
			p2.v = t;
		}
	}

	switch(frame.flags & (PART_FLAG_FLIP_H | PART_FLAG_FLIP_V)){
	case 0:
		part_state->lt.uv = p0;
		part_state->lb.uv = p1;
		part_state->rt.uv = p2;
		part_state->rb.uv = p3;
		break;

	case PART_FLAG_FLIP_H:
		part_state->lt.uv = p2;
		part_state->lb.uv = p3;
		part_state->rt.uv = p0;
		part_state->rb.uv = p1;
		break;

	case PART_FLAG_FLIP_V:
		part_state->lt.uv = p1;
		part_state->lb.uv = p0;
		part_state->rt.uv = p3;
		part_state->rb.uv = p2;
		break;

	case PART_FLAG_FLIP_H | PART_FLAG_FLIP_V:
		part_state->lt.uv = p3;
		part_state->lb.uv = p2;
		part_state->rt.uv = p1;
		part_state->rb.uv = p0;
		break;
	}
}

void SSAnime::update_transform()
{
	const char* data;
	int16_t part_num;
	const SSBPPart* parts;

	if(m_anime == 0)
		return;

	data = m_data->ptr();
	parts = (const SSBPPart*)(data + m_anime_pack->part_offset);

	part_num = m_anime_pack->part_num;
	for(int16_t i = 0; i < part_num; i++){
		const SSBPPart* part;
		SSPartState* part_state;

		part_state = &m_part_state[i];
		part = &parts[i];

		if(part->parent_index < 0){
			if(m_parent_part_state != 0){
#if COCOS2D_VERSION >= 0x00030000
				m_part_state[i].transform = cocos2d::AffineTransformConcat(
					m_part_state[i].transform,
					m_parent_part_state->transform);
#else
				m_part_state[i].transform = cocos2d::CCAffineTransformConcat(
					m_part_state[i].transform,
					m_parent_part_state->transform);
#endif
			}
		}else{
#if COCOS2D_VERSION >= 0x00030000
			m_part_state[i].transform = cocos2d::AffineTransformConcat(
				m_part_state[i].transform,
				m_part_state[part->parent_index].transform);
#else
			m_part_state[i].transform = cocos2d::CCAffineTransformConcat(
				m_part_state[i].transform,
				m_part_state[part->parent_index].transform);
#endif
		}

#if COCOS2D_VERSION >= 0x00030000
		cocos2d::Point pos;
#else
		cocos2d::CCPoint pos;
#endif
		pos.x = part_state->lt.x;
		pos.y = part_state->lt.y;
#if COCOS2D_VERSION >= 0x00030000
		pos = cocos2d::PointApplyAffineTransform(pos, part_state->transform);
#else
		pos = cocos2d::CCPointApplyAffineTransform(pos, part_state->transform);
#endif
		part_state->lt.x = (int16_t)pos.x;
		part_state->lt.y = (int16_t)pos.y;

		pos.x = part_state->lb.x;
		pos.y = part_state->lb.y;
#if COCOS2D_VERSION >= 0x00030000
		pos = cocos2d::PointApplyAffineTransform(pos, part_state->transform);
#else
		pos = cocos2d::CCPointApplyAffineTransform(pos, part_state->transform);
#endif
		part_state->lb.x = (int16_t)pos.x;
		part_state->lb.y = (int16_t)pos.y;

		pos.x = part_state->rt.x;
		pos.y = part_state->rt.y;
#if COCOS2D_VERSION >= 0x00030000
		pos = cocos2d::PointApplyAffineTransform(pos, part_state->transform);
#else
		pos = cocos2d::CCPointApplyAffineTransform(pos, part_state->transform);
#endif
		part_state->rt.x = (int16_t)pos.x;
		part_state->rt.y = (int16_t)pos.y;

		pos.x = part_state->rb.x;
		pos.y = part_state->rb.y;
#if COCOS2D_VERSION >= 0x00030000
		pos = cocos2d::PointApplyAffineTransform(pos, part_state->transform);
#else
		pos = cocos2d::CCPointApplyAffineTransform(pos, part_state->transform);
#endif
		part_state->rb.x = (int16_t)pos.x;
		part_state->rb.y = (int16_t)pos.y;

		if(part_state->flags & SSPartFlagVertexTransform){
			pos.x = part_state->cc.x;
			pos.y = part_state->cc.y;
#if COCOS2D_VERSION >= 0x00030000
			pos = cocos2d::PointApplyAffineTransform(pos, part_state->transform);
#else
			pos = cocos2d::CCPointApplyAffineTransform(pos, part_state->transform);
#endif
			part_state->cc.x = (int16_t)pos.x;
			part_state->cc.y = (int16_t)pos.y;
		}
	}
}

void SSAnime::update_parts()
{
	const char* data;
	const int32_t* frame_offset;
	int16_t part_num;
	SSFrameData frame_data;

	data = m_data->ptr();
	frame_offset = (const int32_t*)(data + m_anime->frame_offset_offset);
	SSStream16 strm(data + frame_offset[m_frame_index]);

	part_num = m_anime_pack->part_num;
	for(int16_t i = 0; i < part_num; i++){
		extract_frame_data(strm, frame_data);

		m_part_state[i].rendering_order = frame_data.part_index;

		update_part(frame_data);
	}
}

void SSAnime::set_frame(int16_t frame_index)
{
	m_frame_index = frame_index;

	update_parts();
	update_transform();

	{
		int16_t n = m_instance_num;
		for(int16_t i = 0; i < n; i++){
			m_instance[i].set_frame_instance(m_frame_index);
		}
	}
}

void SSAnime::set_frame_abs(int32_t elapse_frame_count)
{
	// MEMO: 
	// max frame: 9999
	// max loop :   16
	// max speed:   16
	// 9999 * 16 = 159984
	// int16     =  32767 

	int32_t span;
	int32_t loop_num;
	int32_t loop_count;

	span = m_end_frame_index - m_start_frame_index + 1;
	loop_num = m_loop_num;
	if(m_loop_flags & INSTANCE_LOOP_FLAG_PINGPONG)
		loop_num <<= 1;

	if(m_loop_flags & INSTANCE_LOOP_FLAG_INFINITY){
		int32_t length;

		length = span * loop_num;
		if(elapse_frame_count >= length)
			elapse_frame_count %= length;
	}else{
		int32_t length;

		length = span * loop_num;
		if(elapse_frame_count >= length)
			elapse_frame_count = length - 1;
	}

	m_elapse_frame_count = elapse_frame_count;

	loop_count = elapse_frame_count / span;
	elapse_frame_count %= span;

	bool reverse = (m_loop_flags & INSTANCE_LOOP_FLAG_REVERSE) != 0;
	if((m_loop_flags & INSTANCE_LOOP_FLAG_PINGPONG) && (loop_count & 1))
		reverse = !reverse;

	int16_t frame_index;
	if(reverse)
		frame_index = (int16_t)(m_end_frame_index - elapse_frame_count);
	else
		frame_index = (int16_t)(m_start_frame_index + elapse_frame_count);

	if(m_frame_index != frame_index)
		set_frame(frame_index);
}

void SSAnime::set_frame_instance(int32_t parent_frame_index)
{
	if(!m_instance_enabled)
		return;

	if(m_loop_flags & INSTANCE_LOOP_FLAG_INDEPENDENT)
		return;

	int32_t elapse_frame_count;

	if(parent_frame_index < m_key_frame_index){
		m_frame_index = -1;
		return;
	}

	elapse_frame_count = (int32_t)((parent_frame_index - m_key_frame_index) * m_time_scale);

	set_frame_abs(elapse_frame_count);
}

void SSAnime::update(float dt)
{
	const SSBPAnime* anime;
	float delta_frame;

	anime = (const SSBPAnime*)m_anime;
	if(anime == 0)
		return;

	delta_frame = (dt * m_anime->fps * m_time_scale) + m_delta_frame_frac;
	m_delta_frame_frac = modff(delta_frame, &delta_frame);

	if(delta_frame > 0)
		set_frame_abs(m_elapse_frame_count + (int32_t)delta_frame);

	{
		int16_t num = m_instance_num;
		for(int16_t i = 0; i < num; i++){
			m_instance[i].update_instance(delta_frame);
		}
	}
}

void SSAnime::update_instance(float delta_frame)
{
	if(m_anime == 0)
		return;

	if(!m_instance_enabled)
		return;

	if(!(m_loop_flags & INSTANCE_LOOP_FLAG_INDEPENDENT))
		return;

	delta_frame = (delta_frame * m_time_scale) + m_delta_frame_frac;
	m_delta_frame_frac = modff(delta_frame, &delta_frame);

	if(delta_frame > 0)
		set_frame_abs(m_elapse_frame_count + (int32_t)delta_frame);

	{
		int16_t num = m_instance_num;
		for(int16_t i = 0; i < num; i++){
			m_instance[i].update_instance(delta_frame);
		}
	}
}

void SSAnime::draw_parts(uint32_t color)
{
	const char* data;
	const SSBPHeader* header;
	const SSBPCell* cells;
	int16_t part_num;

	if(m_anime == 0)
		return;

	data = m_data->ptr();
	header = (const SSBPHeader*)(data);
	cells = (const SSBPCell*)(data + header->cell_offset);

	part_num = m_anime_pack->part_num;
	for(int16_t i = 0; i < part_num; i++){
		int16_t part_index;
		SSPartState* part_state;
#if COCOS2D_VERSION >= 0x00030000
		cocos2d::Texture2D* texture;
#else
		cocos2d::CCTexture2D* texture;
#endif
		const SSBPCellMap* cell_map;

		part_index = m_part_state[i].rendering_order;
		part_state = &m_part_state[part_index];

		if(part_state->flags & SSPartFlagInvisible)
			continue;

		if(part_state->instance != 0){
			if(part_state->instance->m_instance_enabled)
				part_state->instance->draw_parts(color);
			continue;
		}

		if(part_state->cell_index < 0)
			continue;

		cell_map = (const SSBPCellMap*)(data + cells[part_state->cell_index].cell_map_offset);
		texture = m_data->get_texture(cell_map->index);
		if(texture == 0)
			continue;

		SSRenderer::draw(part_state, texture, color);
	}
}

#if COCOS2D_VERSION >= 0x00030000
void SSAnime::draw(const cocos2d::Mat4& transform, uint32_t color)
#else
void SSAnime::draw(const kmMat4& transform, uint32_t color)
#endif
{
	if(m_anime == 0)
		return;

	SSRenderer::begin(transform);
	draw_parts(color);
	SSRenderer::end();
}


void SSAnime::set_loop(bool loop)
{
	if(loop)
		m_loop_flags |= INSTANCE_LOOP_FLAG_INFINITY;
	else
		m_loop_flags &= ~INSTANCE_LOOP_FLAG_INFINITY;
}

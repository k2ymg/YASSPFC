#ifndef YASSPFC_SSBP_H
#define YASSPFC_SSBP_H

#include "cocos2d.h"


namespace yasspfc {

struct SSBPHeader {
	uint32_t sig;
	uint32_t ver;
	uint32_t flags;
	int32_t  image_base_dir_offset;
	int32_t  cell_offset;
	int32_t  anime_pack_offset;
	int16_t  cell_num;
	int16_t  anime_pack_num;
};

struct SSBPCell {
	int32_t  name_offset;
	int32_t  cell_map_offset;
	int16_t  index_in_cell_map;
	int16_t  x;
	int16_t  y;
	uint16_t width;
	uint16_t height;
	int16_t  unused__padding__;
};

struct SSBPCellMap {
	int32_t name_offset;
	int32_t image_path_offset;
	int16_t index;
	int16_t unused__padding__;
};

struct SSBPAnimePack {
	int32_t name_offset;
	int32_t part_offset;
	int32_t anime_offset;
	int16_t part_num;
	int16_t anime_num;
};

struct SSBPPart {
	int32_t name_offset;
	int16_t index;
	int16_t parent_index;
	int16_t type;
	int16_t bounds_type;
	int16_t alpha_blend_type;
	int16_t unused__padding__;
	int32_t rename_offset;
};

struct SSBPAnime {
	int32_t name_offset;
	int32_t default_offset;
	int32_t frame_offset_offset;
	int32_t user_data_offset;
	int32_t label_offset;
	int16_t frame_num;
	int16_t fps;
	int16_t label_num;
	int16_t unused__padding__;
};

struct SSBPAnimeInitialData {
	int16_t  index;
	int16_t  unused_padding__1;
	uint32_t flags;
	int16_t  cell_index;
	int16_t  position_x;
	int16_t  position_y;
	int16_t  position_z;
	uint16_t opacity;
	int16_t  unused__padding__2;
	float    anchor_x;
	float    anchor_y;
	float    rotation_x;
	float    rotation_y;
	float    rotation_z;
	float    scale_x;
	float    scale_y;
	float    size_x;
	float    size_y;
	float    uv_move_x;
	float    uv_move_y;
	float    uv_rotation;
	float    uv_scale_x;
	float    uv_scale_y;
	float    bounding_radius;
};

#if COCOS2D_VERSION >= 0x00030000
class SSBP : public cocos2d::Ref {
#else
class SSBP : public cocos2d::CCObject {
#endif
	char* m_data;
	size_t m_size;
	struct CM {
		const SSBPCellMap* cell_map;
		#if COCOS2D_VERSION >= 0x00030000
		cocos2d::Texture2D* texture;
		#else
		cocos2d::CCTexture2D* texture;
		#endif
	};
	CM* m_cm;
	uint16_t m_cell_map_num;

	~SSBP();
	SSBP();
	void init_with_file(const std::string& path);
	void init_with_data(char* data, size_t size);
	void parse_cell_map(const char* data, size_t size);
#if COCOS2D_VERSION >= 0x00030000
	cocos2d::Texture2D* load_texture(int index);
#else
	cocos2d::CCTexture2D* load_texture(int index);
#endif

public:
	static SSBP* create(const std::string& path);

	const char* ptr() const { return m_data; }

	void load_textures();
	void unload_textures();
#if COCOS2D_VERSION >= 0x00030000
	cocos2d::Texture2D* get_texture(int index);
#else
	cocos2d::CCTexture2D* get_texture(int index);
#endif
	const SSBPAnimePack* find_anime_pack(const char* anime_pack_name) const;
	const SSBPAnime* find_anime(const SSBPAnimePack* anime_pack, const char* anime_name) const;

	void dump();
};


class SSStream16 {
	const uint16_t* m_data;

public:
	SSStream16(const char* data) : m_data((uint16_t*)data) {}

	int16_t i16()
	{
		return *m_data++;
	}

	int32_t i32()
	{
		uint32_t l = *m_data++;
		uint32_t u = *m_data++;
		return (u << 16) | l;
	}

	float f32()
	{
		union {
			float f;
			int32_t i;
		} a;
		a.i = i32();
		return a.f;
	}
};

}

#endif

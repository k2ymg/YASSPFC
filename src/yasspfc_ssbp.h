#ifndef YASSPFC_SSBP_H
#define YASSPFC_SSBP_H

#include "cocos2d.h"
#include "yasspfc_ccc.h"


namespace yasspfc {

enum {
    SSBP_SIG = 0x42505353,
    SSBP_VER = 1,
};

enum {
    PART_FLAG_INVISIBLE         = (1 <<  0),
    PART_FLAG_FLIP_H            = (1 <<  1),
    PART_FLAG_FLIP_V            = (1 <<  2),
    PART_FLAG_CELL_INDEX        = (1 <<  3),
    PART_FLAG_POSITION_X        = (1 <<  4),
    PART_FLAG_POSITION_Y        = (1 <<  5),
    PART_FLAG_POSITION_Z        = (1 <<  6),
    PART_FLAG_ANCHOR_X          = (1 <<  7),
    PART_FLAG_ANCHOR_Y          = (1 <<  8),
    PART_FLAG_ROTATIONX         = (1 <<  9),
    PART_FLAG_ROTATIONY         = (1 << 10),
    PART_FLAG_ROTATIONZ         = (1 << 11),
    PART_FLAG_SCALE_X           = (1 << 12),
    PART_FLAG_SCALE_Y           = (1 << 13),
    PART_FLAG_OPACITY           = (1 << 14),
    PART_FLAG_COLOR_BLEND       = (1 << 15),
    PART_FLAG_VERTEX_TRANSFORM  = (1 << 16),
    PART_FLAG_SIZE_X            = (1 << 17),
    PART_FLAG_SIZE_Y            = (1 << 18),
    PART_FLAG_U_MOVE            = (1 << 19),
    PART_FLAG_V_MOVE            = (1 << 20),
    PART_FLAG_UV_ROTATION       = (1 << 21),
    PART_FLAG_U_SCALE           = (1 << 22),
    PART_FLAG_V_SCALE           = (1 << 23),
    PART_FLAG_BOUNDINGRADIUS    = (1 << 24),
    PART_FLAG_INSTANCE_KEYFRAME = (1 << 25),
    PART_FLAG_INSTANCE_START    = (1 << 26),
    PART_FLAG_INSTANCE_END      = (1 << 27),
    PART_FLAG_INSTANCE_SPEED    = (1 << 28),
    PART_FLAG_INSTANCE_LOOP     = (1 << 29),
    PART_FLAG_INSTANCE_LOOP_FLG = (1 << 30),
};

enum {
    INSTANCE_LOOP_FLAG_INFINITY    = (1 << 0),
    INSTANCE_LOOP_FLAG_REVERSE     = (1 << 1),
    INSTANCE_LOOP_FLAG_PINGPONG    = (1 << 2),
    INSTANCE_LOOP_FLAG_INDEPENDENT = (1 << 3),
};

enum {
    VERTEX_FLAG_LT  = (1 << 0),
    VERTEX_FLAG_RT  = (1 << 1),
    VERTEX_FLAG_LB  = (1 << 2),
    VERTEX_FLAG_RB  = (1 << 3),
    VERTEX_FLAG_ONE = (1 << 4),
};

enum {
    PARTTYPE_INVALID  = -1,
    PARTTYPE_NULL     = 0,
    PARTTYPE_NORMAL   = 1,
    //PARTTYPE_TEXT     = 2, unused/reserved
    PARTTYPE_INSTANCE = 3,
};

enum {
    BLEND_MIX = 0,
    BLEND_MUL = 1,
    BLEND_ADD = 2,
    BLEND_SUB = 3,
};

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

struct CellMapInfo {
	const char* name;
	const char* image_path;
};

class SSBP : public YASSPFC_CC_OBJECT {
	char* m_data;
	size_t m_size;
	struct CM {
		const SSBPCellMap* cell_map;
		YASSPFC_CC_TEXTURE2D* texture;
	};
	CM* m_cm;
	uint16_t m_cell_map_num;
	bool m_disable_texture;

protected:
	virtual ~SSBP();
	SSBP(bool disalbe_texture);

	void init_with_file(const std::string& path);
	void init_with_data(char* data, size_t size);
	void parse_cell_map(const char* data, size_t size);
	YASSPFC_CC_TEXTURE2D* load_texture(int index);

public:
	static SSBP* create(const std::string& path, bool do_not_load_texture = false);

	const char* ptr() const { return m_data; }
	const int16_t cell_map_num() const { return (int16_t)m_cell_map_num; }
    const SSBPCell* cell(int16_t index);
    const SSBPCellMap* cell_map(const SSBPCell* cell);

	void load_textures();
	void unload_textures();
	YASSPFC_CC_TEXTURE2D* get_texture(int cell_map_index);

	const SSBPAnimePack* find_anime_pack(const char* anime_pack_name, int16_t* index) const;
	const SSBPAnime* find_anime(const SSBPAnimePack* anime_pack, const char* anime_name, int16_t* index) const;
    const SSBPPart* find_part(const SSBPAnimePack* anime_pack, const char* name) const;
    const SSBPCell* find_cell(const char* name, int16_t* index) const;

	void get_cell_map_info(int16_t index, CellMapInfo* cell_map_info);

    // debug
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

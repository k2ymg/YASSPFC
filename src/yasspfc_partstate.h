#ifndef YASSPFC_PARTSTATE_H
#define YASSPFC_PARTSTATE_H

#include "cocos2d.h"


namespace yasspfc {

struct SSAnime;

enum {
	SSPartFlagInvisible = 1,
	SSPartFlagVertexTransform = 2,
};

enum {// same as SSBP.BLEND_x
	SSPartBlendTypeMix = 0,
	SSPartBlendTypeMul,
	SSPartBlendTypeAdd,
	SSPartBlendTypeSub,
};
enum {
	SSPartBlendNone = 0,
	SSPartBlendSingle,
	SSPartBlendPerVertex,
};
struct SSUV {
	int16_t u;
	int16_t v;
};
struct SSColor {
	union {
		struct {
			int8_t r, g, b, a;
		};
		uint32_t rgba;
	};
};
struct SSPartVertex {
	int16_t x;
	int16_t y;
	SSUV uv;
	SSColor color;
	float color_blend_rate;
};

struct SSPartState {
	SSAnime* instance;

	uint8_t flags;
	uint8_t blend;
	uint8_t color_blend;
	uint8_t color_blend_type;

	int16_t cell_index;
	int16_t rendering_order;// i know, i know...

	uint8_t opacity;
	// pad[3]

	SSPartVertex lt;
	SSPartVertex lb;
	SSPartVertex rt;
	SSPartVertex rb;
	SSPartVertex cc;// center. for the vertex transform

#if COCOS2D_VERSION >= 0x00030000
	cocos2d::AffineTransform transform;
#else
	cocos2d::CCAffineTransform transform;
#endif
};

}

#endif

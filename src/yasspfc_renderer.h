#ifndef YASSPFC_RENDERER_H
#define YASSPFC_RENDERER_H

#include "cocos2d.h"
#include "yasspfc_partstate.h"


namespace yasspfc {

class SSRenderer {
	static void flush();
	static void flush_n();

public:
	static void load_shaders();
	static void unload_shaders();

#if COCOS2D_VERSION >= 0x00030000
	static void begin(const cocos2d::Mat4& transform);
#else
	static void begin(const kmMat4& transform);
#endif

	static void end();
#if COCOS2D_VERSION >= 0x00030000
	static void draw(const SSPartState* part_state, cocos2d::Texture2D* texture, uint32_t color);
#else
	static void draw(const SSPartState* part_state, cocos2d::CCTexture2D* texture, uint32_t color);
#endif
};

}

#endif

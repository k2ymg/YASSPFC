#ifndef YASSPFC_RENDERER_H
#define YASSPFC_RENDERER_H

#include "cocos2d.h"
#include "yasspfc_ccc.h"
#include "yasspfc_partstate.h"


namespace yasspfc {

class SSRenderer {
	static void flush();
	static void flush_n();

public:
	static void load_shaders();
	static void unload_shaders();

	static void begin(const YASSPFC_CC_MAT4& transform);
	static void end();
	static void draw(const SSPartState* part_state);
};

}

#endif

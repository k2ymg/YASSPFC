#ifndef YASSPFC_GLOBAL_H
#define YASSPFC_GLOBAL_H

#include "cocos2d.h"
#include "yasspfc_ccc.h"


namespace yasspfc {

class SSBP;

typedef YASSPFC_CC_TEXTURE2D*(*TextureLoaderFunc)(SSBP* ssbp, const char* base_dir, const char* image_path);

class SSGlobal {
	static TextureLoaderFunc m_user_tex_loader;

public:
	static YASSPFC_CC_TEXTURE2D* load_texture(SSBP* ssbp, const char* base_dir, const char* image_path);
	static void set_texture_loader(TextureLoaderFunc func);

	static void load_shaders();
	static void unload_shaders();
};

}

#endif

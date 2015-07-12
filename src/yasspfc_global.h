#ifndef YASSPFC_GLOBAL_H
#define YASSPFC_GLOBAL_H

#include "cocos2d.h"


namespace yasspfc {
#if COCOS2D_VERSION >= 0x00030000
typedef cocos2d::Texture2D*(*TextureLoaderFunc)(const char* base_dir, const char* image_path);
#else
typedef cocos2d::CCTexture2D*(*TextureLoaderFunc)(const char* base_dir, const char* image_path);
#endif

class SSGlobal {
	static TextureLoaderFunc m_user_tex_loader;

public:
#if COCOS2D_VERSION >= 0x00030000
	static cocos2d::Texture2D* load_texture(const char* base_dir, const char* image_path);
#else
	static cocos2d::CCTexture2D* load_texture(const char* base_dir, const char* image_path);
#endif
	static void set_texture_loader(TextureLoaderFunc func);

	static void load_shaders();
	static void unload_shaders();
};

}

#endif

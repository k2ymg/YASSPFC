#include "yasspfc_global.h"
#include "yasspfc_renderer.h"


using namespace yasspfc;


TextureLoaderFunc SSGlobal::m_user_tex_loader;

#if COCOS2D_VERSION >= 0x00030000
cocos2d::Texture2D* SSGlobal::load_texture(const char* base_dir, const char* image_path)
#else
cocos2d::CCTexture2D* SSGlobal::load_texture(const char* base_dir, const char* image_path)
#endif
{
	if(m_user_tex_loader != 0)
		return m_user_tex_loader(base_dir, image_path);

#if COCOS2D_VERSION >= 0x00030000
	cocos2d::TextureCache* tc = cocos2d::Director::getInstance()->getTextureCache();
	cocos2d::Texture2D* tex;
#else
	cocos2d::CCTextureCache* tc = cocos2d::CCTextureCache::sharedTextureCache();
	cocos2d::CCTexture2D* tex;
#endif
	std::string path;
	if(base_dir != 0){
		path = base_dir;
		path += "/";
	}
	path += image_path;

#if COCOS2D_VERSION >= 0x00030000
	tex = tc->addImage(path);
#else
	tex = tc->addImage(path.c_str());
#endif

	return tex;
}

void SSGlobal::set_texture_loader(TextureLoaderFunc func)
{
	m_user_tex_loader = func;
}

void SSGlobal::load_shaders()
{
	SSRenderer::load_shaders();
}

void SSGlobal::unload_shaders()
{
	SSRenderer::unload_shaders();
}

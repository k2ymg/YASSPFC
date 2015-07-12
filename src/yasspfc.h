#ifndef YASSPFC_H_
#define YASSPFC_H_

#include "cocos2d.h"
#if COCOS2D_VERSION >= 0x00030000
#include "renderer/CCCustomCommand.h"
#endif
#include "yasspfc_anime.h"
#include "yasspfc_global.h"


namespace yasspfc {

#if COCOS2D_VERSION >= 0x00030000
class SSPlayer : public cocos2d::Node {
#else
class SSPlayer : public cocos2d::CCNodeRGBA {
#endif
	SSAnime m_anime;
#if COCOS2D_VERSION >= 0x00030000
	cocos2d::CustomCommand m_render_command;
#endif
	bool m_play;
	bool m_repeat;

	virtual ~SSPlayer();
	SSPlayer();

#if COCOS2D_VERSION >= 0x00030000
	void custom_draw(const cocos2d::Mat4& transform);
#else
#endif

public:
	static SSPlayer* create(SSBP* data);
	void set_anime(const char* anime_pack_name, const char* anime_name);
	void set_loop(bool loop);
	void play(bool play);

	// Node overrides
public:
	void onEnter();
	void onExit();
	void update(float delta);
#if COCOS2D_VERSION >= 0x00030000
	void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4& transform, uint32_t flags) override;
#else
	void draw(void);
#endif

};

}

#endif

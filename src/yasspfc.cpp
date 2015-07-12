#include "yasspfc.h"


using namespace yasspfc;


SSPlayer::~SSPlayer()
{
	m_anime.dtor();
}

SSPlayer::SSPlayer()
{
	m_play = false;
	m_repeat = false;
}

#if COCOS2D_VERSION >= 0x00030000
void SSPlayer::custom_draw(const cocos2d::Mat4& transform)
{
	auto& projection = cocos2d::Director::getInstance()->getMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
	cocos2d::Mat4 modelview_projection = projection * transform;

	cocos2d::Color3B rgb = getColor();
	GLubyte opacity = getOpacity();
	uint32_t color = (rgb.r << 24) | (rgb.g << 16) | (rgb.b << 8) | opacity;

	m_anime.draw(modelview_projection, color);
}
#endif

SSPlayer* SSPlayer::create(SSBP* data)
{
	SSPlayer* o = new(std::nothrow) SSPlayer;
	if(o && o->init()){
		o->autorelease();
		o->m_anime.ctor(data);
		return o;
	}
	CC_SAFE_RELEASE(o);
	return 0;
}

void SSPlayer::set_anime(const char* anime_pack_name, const char* anime_name)
{
	m_anime.set_anime(anime_pack_name, anime_name);
	if(m_repeat)
		m_anime.set_loop(true);
}

void SSPlayer::play(bool play)
{
	m_play = play;
	if(isRunning()){
		if(play)
			scheduleUpdate();
		else
			unscheduleUpdate();
	}
}

void SSPlayer::set_loop(bool loop)
{
	m_repeat = loop;
	m_anime.set_loop(loop);
}

// Node overrides
void SSPlayer::onEnter()
{
	if(m_play){
		scheduleUpdate();
	}
#if COCOS2D_VERSION >= 0x00030000
	Node::onEnter();
#else
	CCNode::onEnter();
#endif
}

void SSPlayer::onExit()
{
	if(m_play){
		unscheduleUpdate();
	}
#if COCOS2D_VERSION >= 0x00030000
	Node::onExit();
#else
	CCNode::onExit();
#endif
}

void SSPlayer::update(float delta)
{
	m_anime.update(delta);
}

#if COCOS2D_VERSION >= 0x00030000
void SSPlayer::draw(cocos2d::Renderer *renderer, const cocos2d::Mat4& transform, uint32_t flags)
{
	m_render_command.init(_globalZOrder, transform, flags);
	m_render_command.func = CC_CALLBACK_0(SSPlayer::custom_draw, this, transform);

	renderer->addCommand(&m_render_command);
}
#else
void SSPlayer::draw()
{
	kmMat4 p;
	kmMat4 mv;
	kmMat4 mvp;

	kmGLGetMatrix(KM_GL_PROJECTION, &p);
	kmGLGetMatrix(KM_GL_MODELVIEW, &mv);

	kmMat4Multiply(&mvp, &p, &mv);


	cocos2d::ccColor3B rgb = getColor();
	GLubyte opacity = getOpacity();
	uint32_t color = (rgb.r << 24) | (rgb.g << 16) | (rgb.b << 8) | opacity;

	m_anime.draw(mvp, color);
}
#endif

#include "yasspfc.h"


using namespace yasspfc;


SSPlayer::~SSPlayer()
{
	m_anime.dtor();
	if(m_data)
		m_data->release();
}

SSPlayer::SSPlayer()
{
	m_data = 0;
	m_play = false;
	m_loop = 1;
}

#if COCOS2D_VERSION >= 0x00030000
void SSPlayer::custom_draw(const cocos2d::Mat4& transform)
{
	auto& projection = cocos2d::Director::getInstance()->getMatrix(cocos2d::MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
	cocos2d::Mat4 modelview_projection = projection * transform;

	m_anime.draw(modelview_projection);
}
#endif

SSPlayer* SSPlayer::create(SSBP* data)
{
	SSPlayer* o = new(std::nothrow) SSPlayer;
	if(o && o->init_with_data(data)){
		o->autorelease();
		return o;
	}
	CC_SAFE_RELEASE(o);
	return 0;
}

bool SSPlayer::init_with_data(SSBP* data)
{
	if(!this->init())
		return false;
	m_data = data;
	data->retain();
	m_anime.ctor(this, data);
	return true;
}

void SSPlayer::set_anime(const char* anime_pack_name, const char* anime_name)
{
	m_anime.set_anime(anime_pack_name, anime_name);
	m_anime.set_loop(m_loop);
}

void SSPlayer::set_anime(const char* reference_name)
{
	m_anime.set_anime(reference_name);
	m_anime.set_loop(m_loop);
}

void SSPlayer::set_loop(int8_t loop)
{
	if(loop == 0 || loop > 16)
		return;

	m_loop = loop;
	m_anime.set_loop(loop);
}

void SSPlayer::play(bool play)
{
	if(m_play == play)
		return;

	m_play = play;
	if(isRunning()){
		if(play)
			scheduleUpdate();
		else
			unscheduleUpdate();
	}
}

void SSPlayer::rewind()
{
	m_anime.rewind();
}

void SSPlayer::get_part_pos(int16_t part_no, float& x, float& y)
{
	const SSPartState* s = &m_anime.m_part_state[part_no];

	x = s->transform.tx;
	y = s->transform.ty;
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
	int32_t f0, f1;
	bool e0, e1;

	if(!m_play)
		return;

	f0 = m_anime.m_elapse_frame_count;
	e0 = m_anime.m_end;
	m_anime.update(delta);
	f1 = m_anime.m_elapse_frame_count;
	e1 = m_anime.m_end;

	if(f0 != f1){
		on_frame_updated();
	}

	if(e1 && (e0 != e1)){
		on_play_ended();
	}
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

	m_anime.draw(mvp);
}
#endif

void SSPlayer::on_play_ended()
{
}

void SSPlayer::on_frame_updated()
{
}

YASSPFC_CC_TEXTURE2D* SSPlayer::get_texture(int16_t cell_map_index)
{
	return m_data->get_texture(cell_map_index);
}

void SSPlayer::get_cell(int16_t cell_index, yasspfc::SSPartStateCell* cell_data)
{
	const SSBPCell* cell;
	const SSBPCellMap* cell_map;
    
	cell = m_data->cell(cell_index);
	cell_map = m_data->cell_map(cell);
	cell_data->tex = get_texture(cell_map->index);
	cell_data->x = cell->x;
	cell_data->y = cell->y;
	cell_data->width = cell->width;
	cell_data->height = cell->height;
}

void SSPlayer::pre_update_frame(int16_t part_index, int16_t frame_no, yasspfc::SSFrameData* frame)
{
}

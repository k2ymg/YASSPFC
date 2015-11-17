#ifndef YASSPFC_H_
#define YASSPFC_H_

#include "cocos2d.h"
#if COCOS2D_VERSION >= 0x00030000
#include "renderer/CCCustomCommand.h"
#endif
#include "yasspfc_anime.h"
#include "yasspfc_global.h"


namespace yasspfc {

class SSPlayer : public YASSPFC_CC_NODERGBA {
	SSBP* m_data;
	SSAnime m_anime;
#if COCOS2D_VERSION >= 0x00030000
	cocos2d::CustomCommand m_render_command;
#endif

	bool m_play;
	int8_t m_loop;

#if COCOS2D_VERSION >= 0x00030000
	void custom_draw(const cocos2d::Mat4& transform);
#endif

public:
	virtual ~SSPlayer();
	SSPlayer();

	SSBP* data() const { return m_data; }
	const SSBPAnimePack* current_anime_pack() const { return m_anime.m_anime_pack; }

	static SSPlayer* create(SSBP* data);
	bool init_with_data(SSBP* data);
	void set_anime(const char* anime_pack_name, const char* anime_name);
	void set_anime(const char* reference_name);// for "name/name" style
	void set_loop(int8_t loop);
	void play(bool play);
	bool is_playing() const { return m_play; }
	void rewind();

	void get_part_pos(int16_t part_no, float& x, float& y);

	// Node overrides
public:
#if COCOS2D_VERSION >= 0x00030000
	void onEnter() override;
	void onExit() override;
	void update(float delta) override;
	void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4& transform, uint32_t flags) override;
#else
	void onEnter();
	void onExit();
	void update(float delta);
	void draw(void);
#endif

	// overrides for custimization
public:
	virtual void on_play_ended();
	virtual void on_frame_updated();
	virtual YASSPFC_CC_TEXTURE2D* get_texture(int16_t cell_map_index);
	virtual void get_cell(int16_t cell_index, yasspfc::SSPartStateCell* cell);
	virtual void pre_update_frame(int16_t part_index, int16_t frame_no, yasspfc::SSFrameData* frame);
};

}

#endif

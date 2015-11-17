#include "yasspfc_ssbp.h"
#include "yasspfc_global.h"


using namespace yasspfc;


SSBP::~SSBP()
{
	if(m_cm != 0){
		int n = m_cell_map_num;
		while(n--){
			if(m_cm[n].texture != 0)
				m_cm[n].texture->release();
		}
		delete[] m_cm;
	}

	if(m_data != 0)
		free(m_data);
}

SSBP::SSBP(bool disable_texture)
{
	m_data = 0;
	m_size = 0;
	m_cm = 0;
	m_cell_map_num = 0;
	m_disable_texture = disable_texture;
}

void SSBP::init_with_file(const std::string& path)
{
	char* data;
	size_t size;

#if COCOS2D_VERSION >= 0x00030000
	cocos2d::Data d = cocos2d::FileUtils::getInstance()->getDataFromFile(path);
	data = (char*)d.getBytes();
	size = (size_t)d.getSize();
	d.fastSet(0, 0);// detach
#else
	unsigned long sz;
	data = (char*)cocos2d::CCFileUtils::sharedFileUtils()->getFileData(path.c_str(), "rb", &sz);
	size = (size_t)sz;
#endif

	if(data != 0){
		init_with_data(data, size);
	}
}

void SSBP::init_with_data(char* data, size_t size)
{
	parse_cell_map(data, size);

	m_data = data;
	m_size = size;
}

void SSBP::parse_cell_map(const char* data, size_t size)
{
	// It cannot access to CellMap directly... faq!!
	// CellMap[0]
	//   Cells...
	// CellMap[1]
	//   Cells...
	const SSBPHeader* header;
	const SSBPCell* cells;
	uint16_t i;
	uint16_t cell_num;
	uint16_t last_index;

	header = (const SSBPHeader*)data;
	cells = (const SSBPCell*)(data + header->cell_offset);

	// step 1. count CellMap
	last_index = 0;
	cell_num = header->cell_num;
	for(i = 0; i < cell_num; i++){
		const SSBPCell* cell;
		const SSBPCellMap* cell_map;

		cell = &cells[i];
		cell_map = (const SSBPCellMap*)(data + cell->cell_map_offset);
		if(cell_map->index > last_index)
			last_index = cell_map->index;
	}

	m_cell_map_num = last_index + 1;
	m_cm = new CM[m_cell_map_num];

	// step 2. save CellMap address
	for(i = 0; i < cell_num; i++){
		const SSBPCell* cell;
		const SSBPCellMap* cell_map;

		cell = &cells[i];
		cell_map = (const SSBPCellMap*)(data + cell->cell_map_offset);
		m_cm[cell_map->index].cell_map = cell_map;
		m_cm[cell_map->index].texture = 0;
	}
}

YASSPFC_CC_TEXTURE2D* SSBP::load_texture(int index)
{
	const char* data;
	const SSBPHeader* header;
	const SSBPCellMap* cell_map;
	const char* base_dir;
	const char* image_path;
	YASSPFC_CC_TEXTURE2D* tex;

	cell_map = m_cm[index].cell_map;

	data = m_data;
	header = (const SSBPHeader*)data;
	if(header->image_base_dir_offset != 0)
		base_dir = (const char*)(data + header->image_base_dir_offset);
	else
		base_dir = 0;
	image_path = (const char*)(data + cell_map->image_path_offset);

	tex = SSGlobal::load_texture(this, base_dir, image_path);

	if(tex != 0){
		tex->retain();
		m_cm[index].texture = tex;
	}

	return tex;
}

SSBP* SSBP::create(const std::string& path, bool disable_texture)
{
	SSBP* o = new(std::nothrow) SSBP(disable_texture);
	if(o != 0){
		o->init_with_file(path);
		o->autorelease();
	}
	return o;
}

const SSBPCell* SSBP::cell(int16_t index)
{
	const SSBPHeader* header;
	const SSBPCell* cell;

	header = (const SSBPHeader*)m_data;
	cell = (const SSBPCell*)(m_data + header->cell_offset) + index;
	return cell;
}

const SSBPCellMap* SSBP::cell_map(const SSBPCell* cell)
{
	const SSBPCellMap* cell_map;

	cell_map = (const SSBPCellMap*)(m_data + cell->cell_map_offset);
	return cell_map;
}

void SSBP::load_textures()
{
	uint16_t cell_map_num;

	if(m_disable_texture)
		return;

	cell_map_num = m_cell_map_num;
	for(uint16_t i = 0; i < cell_map_num; i++){
		if(m_cm[i].texture == 0)
			load_texture(i);
	}
}

void SSBP::unload_textures()
{
	uint16_t i;

	i = m_cell_map_num;
	while(i--){
		if(m_cm[i].texture != 0){
			m_cm[i].texture->release();
			m_cm[i].texture = 0;
		}
	}
}

YASSPFC_CC_TEXTURE2D* SSBP::get_texture(int cell_map_index)
{
	YASSPFC_CC_TEXTURE2D* tex;

	if(m_disable_texture)
		return 0;

	tex = m_cm[cell_map_index].texture;
	if(tex == 0)
		tex = load_texture(cell_map_index);

	return tex;
}

const SSBPAnimePack* SSBP::find_anime_pack(const char* anime_pack_name, int16_t* index) const
{
	const char* data;
	const SSBPHeader* header;
	const SSBPAnimePack* anime_pack;
	int16_t anime_pack_num;

	data = m_data;
	header = (const SSBPHeader*)data;
	anime_pack = (const SSBPAnimePack*)(data + header->anime_pack_offset);
	anime_pack_num = header->anime_pack_num;
	for(int16_t i = 0; i < anime_pack_num; i++){
		const char* name;

		name = (const char*)(data + anime_pack[i].name_offset);
		if(strcmp(name, anime_pack_name) == 0){
			if(index != 0)
				*index = i;
			return &anime_pack[i];
		}
	}

	return 0;
}

const SSBPAnime* SSBP::find_anime(const SSBPAnimePack* anime_pack, const char* anime_name, int16_t* index) const
{
	const char* data;
	const SSBPAnime* anime;
	int16_t anime_num;

	data = m_data;
	anime = (const SSBPAnime*)(data + anime_pack->anime_offset);
	anime_num = anime_pack->anime_num;
	for(int16_t i = 0; i < anime_num; i++){
		const char* name;

		name = (const char*)(data + anime[i].name_offset);
		if(strcmp(name, anime_name) == 0){
			if(index != 0)
				*index = i;
			return &anime[i];
		}
	}

	return 0;
}

const SSBPPart* SSBP::find_part(const SSBPAnimePack* anime_pack, const char* name) const
{
	const char* data;
	const SSBPPart* part;
	int16_t num;

	data = m_data;
	part = (const SSBPPart*)(data + anime_pack->part_offset);
	num = anime_pack->part_num;
	for(; num > 0; num--, part++){
		const char* n;

		n = (const char*)(data + part->name_offset);
		if(strcmp(n, name) == 0)
			return part;
	}

	return 0;
}

const SSBPCell* SSBP::find_cell(const char* name, int16_t* index) const
{
    const char* data;
	const SSBPHeader* header;
	const SSBPCell* cells;
	uint16_t i;
	uint16_t cell_num;

	data = m_data;
	header = (const SSBPHeader*)data;
	cells = (const SSBPCell*)(data + header->cell_offset);

	cell_num = header->cell_num;
	for(i = 0; i < cell_num; i++){
		const SSBPCell* cell;
		const char* n;

		cell = &cells[i];
		n = (data + cell->name_offset);
		if(strcmp(n, name) == 0){
			if(index != 0)
				*index = i;
			return cell;
		}
	}

	return 0;
}

void SSBP::get_cell_map_info(int16_t index, CellMapInfo* cell_map_info)
{
	const char* data;
	const SSBPCellMap* cell_map;
	const char* name;
	const char* image_path;

	cell_map = m_cm[index].cell_map;

	data = m_data;
	name = (const char*)(data + cell_map->name_offset);
	image_path = (const char*)(data + cell_map->image_path_offset);

	cell_map_info->name = name;
	cell_map_info->image_path = image_path;
}

void SSBP::dump()
{
	const char* data;
	const SSBPHeader* header;
	const char* image_base_dir;
	const SSBPAnimePack* anime_pack;
	int16_t anime_pack_num;

	data = m_data;
	header = (const SSBPHeader*)data;
	if(header->image_base_dir_offset != 0)
		image_base_dir = (const char*)(data + header->image_base_dir_offset);
	else
		image_base_dir = "(unspecified)";
	CCLOG("----------");
	CCLOG("Header:");
	CCLOG("Sig          : %08x", header->sig);
	CCLOG("Ver          : %d", header->ver);
	CCLOG("Flags        : %08x", header->flags);
	CCLOG("ImageBaseDir : %s", image_base_dir);
	CCLOG("Cell num     : %d", header->cell_num);
	CCLOG("AnimePack num: %d", header->anime_pack_num);

	anime_pack = (const SSBPAnimePack*)(data + header->anime_pack_offset);
	anime_pack_num = header->anime_pack_num;

	for(int16_t i = 0; i < anime_pack_num; i++){
		const char* anime_pack_name;

		
		anime_pack_name = (const char*)(data + anime_pack[i].name_offset);
		CCLOG("----------");
		CCLOG("AnimePack: %d", i);
		CCLOG("Name     : %s", anime_pack_name);
		CCLOG("Part num : %d", anime_pack[i].part_num);
		CCLOG("Anime num: %d", anime_pack[i].anime_num);

		{
			const SSBPPart* part;
			int16_t part_num;

			part = (const SSBPPart*)(data + anime_pack[i].part_offset);
			part_num = anime_pack[i].part_num;
			for(int16_t j = 0; j < part_num; j++){
				const char* part_name;

				part_name = (const char*)(data + part[j].name_offset);
			}
		}

		{
			const SSBPAnime* anime;
			int16_t anime_num;

			anime = (const SSBPAnime*)(data + anime_pack[i].anime_offset);
			anime_num = anime_pack[i].anime_num;
			for(int16_t j = 0; j < anime_num; j++){
				const char* anime_name;

				anime_name = (const char*)(data + anime[j].name_offset);

				CCLOG("-----");
				CCLOG(" Anime    : %d", j);
				CCLOG(" Name     : %s", anime_name);
				CCLOG(" Frame num: %d", anime[j].frame_num);
				CCLOG(" FPS      : %d", anime[j].fps);
				CCLOG(" Label num: %d", anime[j].label_num);
			}
		}
	}
}

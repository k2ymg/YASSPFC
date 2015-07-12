#include "yasspfc_renderer.h"


// Note:
// OpenGL ES 3.x have textureSize() function
// sampler2DRect reserved.
// Apple support EXT_separate_shader_objects


using namespace yasspfc;

struct VertexNormal {
	GLshort x;
	GLshort y;
	SSUV uv;
};

struct VertexColor {
	GLshort x;
	GLshort y;
	SSUV uv;
	GLuint color;
	GLfloat blend_rate;
};
union VertexUnion {
	VertexNormal a;
	VertexColor b;
};




static const char* s_vs_Normal = "\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
attribute vec2 a_pos;\n\
attribute vec2 a_uv;\n\
\n\
varying vec2 v_uv;\n\
\n\
uniform mat4 u_proj;\n\
uniform vec2 u_tex_size;\n\
\n\
void main()\n\
{\n\
	gl_Position = u_proj * vec4(a_pos, 0, 1);\n\
	v_uv = a_uv * u_tex_size;\n\
}";

static const char* s_vs_VertexBlend = "\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
attribute vec2 a_pos;\n\
attribute vec2 a_uv;\n\
attribute vec4 a_blend;\n\
attribute float a_blend_rate;\n\
\n\
varying vec2 v_uv;\n\
varying vec4 blend;\n\
varying float blend_rate_inv;\n\
\n\
uniform mat4 u_proj;\n\
uniform vec2 u_tex_size;\n\
\n\
void main()\n\
{\n\
	gl_Position = u_proj * vec4(a_pos, 0, 1);\n\
	v_uv = a_uv * u_tex_size;\n\
	blend = a_blend * a_blend_rate;\n\
	blend_rate_inv = 1 - a_blend_rate;\n\
}";


static const char* s_fs_Normal = "\
#ifdef GL_ES \n\
precision lowp float;\n\
#endif \n\
varying vec2 v_uv;\
\
uniform sampler2D u_tex;\
\
void main()\
{\
	gl_FragColor = texture2D(u_tex, v_uv);\
}\
";

static const char* s_fs_Color = "\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif \n\
varying vec2 v_uv;\n\
\n\
uniform sampler2D u_tex;\n\
uniform vec4 u_color;\n\
\n\
void main()\n\
{\n\
	gl_FragColor = texture2D(u_tex, v_uv) * u_color;\n\
}";

static const char* s_fs_blend = "\
#ifdef GL_ES\n\
precision lowp float;\n\
#endif\n\
varying vec2 v_uv;\n\
\n\
uniform sampler2D u_tex;\n\
#if SS_VERTEX\n\
varying vec4 blend;\n\
varying float blend_rate_inv;\n\
#else\n\
uniform vec4 blend;\n\
uniform float blend_rate_inv;\n\
#endif\n\
#if SS_COLOR\n\
uniform vec4 u_color;\n\
#endif\n\
\n\
void main()\n\
{\n\
	vec4 t = texture2D(u_tex, v_uv);\n\
\n\
#if SS_BLEND == 0\n\
	vec3 c = (t.rgb * blend_rate_inv) + blend.rgb;\n\
#elif SS_BLEND == 1\n\
	vec3 c = (t.rgb * blend_rate_inv) + (blend.rgb * t.rgb);\n\
#elif SS_BLEND == 2\n\
	vec3 c = t.rgb + blend.rgb;\n\
#else\n\
	vec3 c = t.rgb - blend.rgb;\n\
#endif\n\
\n\
#if SS_PMA\n\
#if SS_COLOR\n\
	gl_FragColor = vec4(c * u_color.rgb, t.a) * (blend.a * u_color.a);\n\
#else\n\
	gl_FragColor = vec4(c, t.a) * blend.a;\n\
#endif\n\
#else\n\
#if SS_COLOR\n\
	gl_FragColor = vec4(c, t.a * blend.a) * u_color;\n\
#else\n\
	gl_FragColor = vec4(c, t.a * blend.a);\n\
#endif\n\
#endif\n\
}";

static const char* s_define_fmt = "\
#define SS_BLEND  %d\n\
#define SS_PMA    %d\n\
#define SS_VERTEX %d\n\
#define SS_COLOR  %d\n\
";


static void compile_shader(GLuint shader, const char* defs, const char* source)
{
	const GLchar* src[2];
	GLint len[2];
	GLint compiled;
	GLint cnt;

	cnt = 0;
	if(defs){
		src[cnt] = defs;
		len[cnt] = strlen(defs);
		cnt++;
	}
	src[cnt] = source;
	len[cnt] = strlen(source);
	cnt++;
	glShaderSource(shader, cnt, src, len);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(compiled == GL_FALSE){
		GLint log_len;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		if(log_len > 1){
			char* log = (char*)malloc(log_len);
			glGetShaderInfoLog(shader, log_len, 0, log);
			CCLOG("glCompileShader failed: %s", log);
			free(log);
		}
	}
}

struct SSVertexShader {
	const char* m_vs_source;
	GLuint m_shader;
	GLint m_attrib_num;
	bool m_have_blend;
	bool m_compiled;

	void init(GLint num, const char* vs_source, bool have_blend)
	{
		m_attrib_num = num;
		m_compiled = false;
		m_have_blend = have_blend;
		m_vs_source = vs_source;
	}

	GLint attrib_num() const
	{
		return m_attrib_num;
	}

	GLuint handle() const
	{
		return m_shader;
	}

	void compile()
	{
		if(!m_compiled){
			m_compiled = true;
			m_shader = glCreateShader(GL_VERTEX_SHADER);

			compile_shader(m_shader, 0, m_vs_source);
		}
	}

	void release()
	{
		if(m_compiled){
			m_compiled = false;
			glDeleteShader(m_shader);
		}
	}

	void bind_location(GLuint program)
	{
		glBindAttribLocation(program, 0, "a_pos");
		glBindAttribLocation(program, 1, "a_uv");
		if(m_have_blend){
			glBindAttribLocation(program, 2, "a_blend");
			glBindAttribLocation(program, 3, "a_blend_rate");
		}
	}

	void set_ptr(char* ptr)
	{
		GLsizei size;

		size = m_have_blend ? sizeof(VertexColor) : sizeof(VertexNormal);
		glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, size, ptr + 0);
		glVertexAttribPointer(1, 2, GL_SHORT, GL_FALSE, size, ptr + 4);
		if(m_have_blend){
			glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, size, ptr + 8);// normalized
			glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, ptr + 12);
		}
	}
};


#define PROGRAM_OPTION_BLEND  0x1
#define PROGRAM_OPTION_PMA    0x2
#define PROGRAM_OPTION_VERTEX 0x4
#define PROGRAM_OPTION_COLOR  0x8


struct SSShaderProgram {
	GLuint m_program;
	bool m_compiled;
	uint8_t m_options;
	uint8_t m_blend_mode;

	GLint m_uniform_projection;
	GLint m_uniform_texture;
	GLint m_uniform_tex_size;
	GLint m_uniform_color;
	GLint m_uniform_blend;
	GLint m_uniform_blend_rate_inv;

	SSVertexShader* m_vertex_shader;
	const char* m_fs_source;

	void uniform_location(GLuint program)
	{
		m_uniform_projection = glGetUniformLocation(program, "u_proj");
		m_uniform_texture = glGetUniformLocation(program, "u_tex");
		m_uniform_tex_size = glGetUniformLocation(program, "u_tex_size");
		if(m_options & PROGRAM_OPTION_COLOR)
			m_uniform_color = glGetUniformLocation(program, "u_color");
		if(m_options & PROGRAM_OPTION_BLEND){
			m_uniform_blend = glGetUniformLocation(program, "blend");
			m_uniform_blend_rate_inv = glGetUniformLocation(program, "blend_rate_inv");
		}
	}

	void compile_and_link(const char* fragment_shader_source)
	{
		GLuint vertex_shader;
		GLuint fragment_shader;
		GLuint program;
		GLint linked;
		

		m_vertex_shader->compile();
		vertex_shader = m_vertex_shader->handle();

		fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

		if(m_options & PROGRAM_OPTION_BLEND){
			char defs[128];

			sprintf(defs, s_define_fmt,
				m_blend_mode,
				(m_options & PROGRAM_OPTION_PMA) ? 1 : 0,
				(m_options & PROGRAM_OPTION_VERTEX) ? 1 : 0,
				(m_options & PROGRAM_OPTION_COLOR) ? 1 : 0
				);
			compile_shader(fragment_shader, defs, fragment_shader_source);
		}else{
			compile_shader(fragment_shader, 0, fragment_shader_source);
		}


		program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, fragment_shader);

		glDeleteShader(fragment_shader);

		m_vertex_shader->bind_location(program);

		glLinkProgram(program);

		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if(linked == GL_FALSE){
			GLint log_len;

			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
			if(log_len > 1){
				char* log = (char*)malloc(log_len);
				glGetProgramInfoLog(program, log_len, 0, log);
				CCLOG("glLinkProgram failed: %s", log);
				free(log);
			}
		}

		uniform_location(program);

		m_program = program;
	}

	GLuint handle() const
	{
		return m_program;
	}

	bool compiled() const
	{
		return m_compiled;
	}

	SSVertexShader* vertex_shader() const
	{
		return m_vertex_shader;
	}


	void init(SSVertexShader* vs, const char* fs_source, uint8_t blend_mode, uint8_t options)
	{
		m_compiled = false;
		m_options = options;
		m_vertex_shader = vs;
		m_fs_source = fs_source;
		m_blend_mode = blend_mode;
	}

	void release()
	{
		if(m_compiled){
			m_compiled = false;
			glDeleteProgram(m_program);
		}
	}

	void compile()
	{
		if(!m_compiled){
			m_compiled = true;
			compile_and_link(m_fs_source);
		}
	}

#if COCOS2D_VERSION >= 0x00030000
	void set_common_uniform(const cocos2d::Mat4* transform)
#else
	void set_common_uniform(const kmMat4* transform)
#endif
	{
#if COCOS2D_VERSION >= 0x00030000
		glUniformMatrix4fv(m_uniform_projection, 1, GL_FALSE, transform->m);
#else
		glUniformMatrix4fv(m_uniform_projection, 1, GL_FALSE, transform->mat);
#endif
		glUniform1i(m_uniform_texture, 0);
	}

	void set_texture_size(GLfloat* inv_size)
	{
		glUniform2fv(m_uniform_tex_size, 1, inv_size);
	}

	void set_color(uint32_t color)
	{
		if((m_options & PROGRAM_OPTION_COLOR) == 0)
			return;

		GLfloat v[4];
		GLfloat r;

		r = 1.0f / 255.0f;
		v[0] = ((color >> 24) & 0xff) * r;
		v[1] = ((color >> 16) & 0xff) * r;
		v[2] = ((color >>  8) & 0xff) * r;
		v[3] = ((color >>  0) & 0xff) * r;
		glUniform4fv(m_uniform_color, 1, v);
	}

	void set_blend(uint32_t color, float rate)
	{
		if((m_options & PROGRAM_OPTION_BLEND) == 0)
			return;

		GLfloat v[4];
		GLfloat r;

		r = (1.0f / 255.0f) * rate;
		v[0] = ((color >> 24) & 0xff) * r;
		v[1] = ((color >> 16) & 0xff) * r;
		v[2] = ((color >>  8) & 0xff) * r;
		v[3] = ((color >>  0) & 0xff) * r;
		glUniform4fv(m_uniform_blend, 1, v);

		glUniform1f(m_uniform_blend_rate_inv, -rate);
	}
};



static SSVertexShader s_va_Normal;
static SSVertexShader s_va_Blend;

#define PROGRAM_NUM (2 + 32)
static bool s_init_programs;
static SSShaderProgram s_programs[PROGRAM_NUM];


//-----------------------------------------------
#define MAX_BATCH_COUNT 32

enum ProgramType {
	ProgramTypeUndef = 0,
	ProgramTypeNormal,
	ProgramTypeColor,
	ProgramTypeMix,
	ProgramTypeMixColor,
	ProgramTypeVertexMix,
	ProgramTypeVertexMixColor,
};

enum PrimitiveType {
	PrimitiveTypeStrip,
	PrimitiveTypeTriangle,
};

enum {
	DirtyColor           = (1 << 0),
	DirtyTexture         = (1 << 1),
	DirtyProgram         = (1 << 2),
	DirtyPrimitiveType   = (1 << 3),
	DirtyColorBlend      = (1 << 4),
	DirtyColorBlendType  = (1 << 5),
	DirtyColorBlendColor = (1 << 6),
	DirtyBlend           = (1 << 7),
};

struct RenderState {
	uint32_t dirty;
	GLuint color;
#if COCOS2D_VERSION >= 0x00030000
	cocos2d::Texture2D* texture;
#else
	cocos2d::CCTexture2D* texture;
#endif
	uint8_t color_blend;
	uint8_t color_blend_type;
	uint8_t primitive_type;
	uint8_t blend;
	uint32_t color_blend_color;
	float color_blend_rate;
};


static char* s_vertex_buffer;
static GLubyte* s_index_buffer;
static RenderState s_render_state[2];// 0 next, 1 current
static int32_t s_draw_count;
static void* s_vertex_buffer_cur;
#if COCOS2D_VERSION >= 0x00030000
static const cocos2d::Mat4* s_transform;
#else
static const kmMat4* s_transform;
#endif
static SSShaderProgram* s_program;
static GLenum s_blend_src_factor;
static GLenum s_blend_dst_factor;
static GLenum s_blend_equation;

static void setup_shader_table()
{
	if(s_init_programs)
		return;
	s_init_programs = true;

	s_va_Normal.init(2, s_vs_Normal, false);
	s_va_Blend.init(4, s_vs_VertexBlend, true);

	s_programs[0].init(&s_va_Normal, s_fs_Normal, 0, 0);
	s_programs[1].init(&s_va_Normal, s_fs_Color, 0, PROGRAM_OPTION_COLOR);

	int i = 2;
	for(int mode = 0; mode < 4; mode++, i += 8){
		s_programs[i + 0].init(&s_va_Normal, s_fs_blend, mode, PROGRAM_OPTION_BLEND);
		s_programs[i + 1].init(&s_va_Normal, s_fs_blend, mode, PROGRAM_OPTION_BLEND | PROGRAM_OPTION_COLOR);
		s_programs[i + 2].init(&s_va_Blend, s_fs_blend, mode, PROGRAM_OPTION_BLEND | PROGRAM_OPTION_VERTEX);
		s_programs[i + 3].init(&s_va_Blend, s_fs_blend, mode, PROGRAM_OPTION_BLEND | PROGRAM_OPTION_VERTEX | PROGRAM_OPTION_COLOR);
		s_programs[i + 4].init(&s_va_Normal, s_fs_blend, mode, PROGRAM_OPTION_BLEND | PROGRAM_OPTION_PMA);
		s_programs[i + 5].init(&s_va_Normal, s_fs_blend, mode, PROGRAM_OPTION_BLEND | PROGRAM_OPTION_PMA | PROGRAM_OPTION_COLOR);
		s_programs[i + 6].init(&s_va_Blend, s_fs_blend, mode, PROGRAM_OPTION_BLEND | PROGRAM_OPTION_PMA | PROGRAM_OPTION_VERTEX);
		s_programs[i + 7].init(&s_va_Blend, s_fs_blend, mode, PROGRAM_OPTION_BLEND | PROGRAM_OPTION_PMA | PROGRAM_OPTION_VERTEX | PROGRAM_OPTION_COLOR);
	}
}

void SSRenderer::load_shaders()
{
	setup_shader_table();

	for(int i = 0; i < PROGRAM_NUM; i++)
		s_programs[i].compile();
}

void SSRenderer::unload_shaders()
{
	if(s_init_programs){
		for(int i = 0; i < PROGRAM_NUM; i++)
			s_programs[i].release();
		s_va_Normal.release();
		s_va_Blend.release();
	}

	if(s_vertex_buffer != 0){
		free(s_vertex_buffer);
		free(s_index_buffer);
	}
}

void SSRenderer::flush()
{
	const RenderState* cur;
	SSShaderProgram* program;
	int32_t draw_count;
	int32_t vertex_count;

	draw_count = s_draw_count;
	//CCASSERT(draw_count != 0, "");

	cur = &s_render_state[1];
	uint32_t dirty = cur->dirty;

	{
		int program_index;

		bool pma = cur->texture->hasPremultipliedAlpha();

		switch(cur->color_blend){
		case SSPartBlendNone:
			program_index = 0;
			break;
		case SSPartBlendSingle:
			program_index = 2 + cur->color_blend_type * 8 + (pma ? 4 : 0) + 0;
			break;
		case SSPartBlendPerVertex:
			program_index = 2 + cur->color_blend_type * 8 + (pma ? 4 : 0) + 2;
			break;
		}
		if(~cur->color)
			program_index++;

		program = &s_programs[program_index];

		//CCASSERT(program_index < PROGRAM_NUM, "");
	}

	if(s_program != program){
		GLint a, b;

		if(!program->compiled())
			program->compile();

		glUseProgram(program->handle());

		if(s_program != 0)
			a = s_program->vertex_shader()->attrib_num();
		else
			a = 0;
		b = program->vertex_shader()->attrib_num();
		if(a < b){
			for(; a < b; a++){
				glEnableVertexAttribArray(a);
			}
		}else if(a > b){
			for(; b < a; b++)
				glDisableVertexAttribArray(b);
		}

		program->vertex_shader()->set_ptr(s_vertex_buffer);
		program->set_common_uniform(s_transform);

		s_program = program;
		dirty |= DirtyProgram;
	}

	if(dirty & DirtyTexture){
		glBindTexture(GL_TEXTURE_2D, cur->texture->getName());
	}

	if(dirty & (DirtyTexture | DirtyProgram)){
#if COCOS2D_VERSION >= 0x00030000
		cocos2d::Size size = cur->texture->getContentSizeInPixels();
#else
		cocos2d::CCSize size = cur->texture->getContentSizeInPixels();
#endif
		GLfloat inv_size[2];
		inv_size[0] = 1 / size.width;
		inv_size[1] = 1 / size.height;

		program->set_texture_size(inv_size);
	}

	if(dirty & (DirtyColor | DirtyProgram)){
		program->set_color(cur->color);
	}

	if(cur->color_blend == SSPartBlendSingle){
		if(dirty & (/*DirtyColorBlend | DirtyColorBlendType | */DirtyProgram | DirtyColorBlendColor)){
			program->set_blend(cur->color_blend_color, cur->color_blend_rate);
		}
	}

	if(dirty & (DirtyTexture | DirtyBlend)){
		GLenum s, d, e;
		bool pma;

		pma = cur->texture->hasPremultipliedAlpha();

		switch(cur->blend){
		case SSPartBlendTypeMix:
			if(pma){
				s = GL_ONE;
				d = GL_ONE_MINUS_SRC_ALPHA;
			}else{
				s = GL_SRC_ALPHA;
				d = GL_ONE_MINUS_SRC_ALPHA;
			}
			e = GL_FUNC_ADD;
			break;

		case SSPartBlendTypeMul:
			if(pma){
				s = GL_DST_COLOR;
				d = GL_ZERO;
			}else{
				s = GL_DST_COLOR;// wrong
				d = GL_ZERO;
			}
			e = GL_FUNC_ADD;
			break;

		case SSPartBlendTypeAdd:
			if(pma){
				s = GL_ONE;
				d = GL_ONE;
			}else{
				s = GL_SRC_ALPHA;
				d = GL_ONE;
			}
			e = GL_FUNC_ADD;
			break;

		case SSPartBlendTypeSub:
			if(pma){
				s = GL_ONE;
				d = GL_ONE;
			}else{
				s = GL_SRC_ALPHA;
				d = GL_ONE;
			}
			e = GL_FUNC_REVERSE_SUBTRACT;
			break;
		}

		if(s != s_blend_src_factor || d != s_blend_dst_factor){
			s_blend_src_factor = s;
			s_blend_dst_factor = d;
			glBlendFunc(s, d);
		}

		if(e != s_blend_equation){
			s_blend_equation = e;
			glBlendEquation(e);
		}
	}

	switch(cur->primitive_type){
	case PrimitiveTypeStrip:
	{
		GLubyte* ib;
		GLubyte vertex_index;
		int32_t primitive_count;
		GLsizei element_count;

		ib = s_index_buffer;
		vertex_index = 0;
		primitive_count = draw_count;
		while(primitive_count--){
			ib[0] = vertex_index + 0;
			ib[1] = vertex_index + 1;
			ib[2] = vertex_index + 2;
			ib[3] = vertex_index + 3;
			ib[4] = vertex_index + 3;
			ib[5] = vertex_index + 4;

			ib += 6;
			vertex_index += 4;
		}

		element_count = draw_count * 6 - 2;
		glDrawElements(GL_TRIANGLE_STRIP, element_count, GL_UNSIGNED_BYTE, s_index_buffer);
		vertex_count = 4 * draw_count;
	}
		break;

	case PrimitiveTypeTriangle:
	{
		GLubyte* ib;
		GLubyte vertex_index;
		int32_t primitive_count;
		GLsizei element_count;

		ib = s_index_buffer;
		vertex_index = 0;
		primitive_count = draw_count;
		while(primitive_count--){
			ib[0] = vertex_index + 4;
			ib[1] = vertex_index + 2;
			ib[2] = vertex_index + 0;

			ib[3] = vertex_index + 4;
			ib[4] = vertex_index + 0;
			ib[5] = vertex_index + 1;

			ib[6] = vertex_index + 4;
			ib[7] = vertex_index + 1;
			ib[8] = vertex_index + 3;

			ib[9] = vertex_index + 4;
			ib[10] = vertex_index + 3;
			ib[11] = vertex_index + 2;

			ib += 12;
			vertex_index += 5;
		}

		element_count = draw_count * 12;
		glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_BYTE, s_index_buffer);
		vertex_count = 12 * draw_count;
	}
		break;
	}

#if COCOS2D_VERSION >= 0x00030000
	CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1, vertex_count);
	CHECK_GL_ERROR_DEBUG();
#else
	CC_INCREMENT_GL_DRAWS(1);
	{
		GLenum e = glGetError();
		if(e){
			cocos2d::CCLog("OpenGL error 0x%04X in %s %s %d\n", e, __FILE__, __FUNCTION__, __LINE__);
		}
	}
#endif
}

void SSRenderer::flush_n()
{
	if(s_draw_count > 0){
		flush();
		s_draw_count = 0;
		s_vertex_buffer_cur = s_vertex_buffer;
	}
	s_render_state[1] = s_render_state[0];
}

#if COCOS2D_VERSION >= 0x00030000
void SSRenderer::begin(const cocos2d::Mat4& transform)
#else
void SSRenderer::begin(const kmMat4& transform)
#endif
{
	if(s_vertex_buffer == 0){
		s_vertex_buffer = (char*)malloc(sizeof(VertexUnion) * 5 * MAX_BATCH_COUNT);
		s_index_buffer = (GLubyte*)malloc(sizeof(GLubyte) * 12 * MAX_BATCH_COUNT);

		setup_shader_table();
	}


	s_transform = &transform;

	s_vertex_buffer_cur = s_vertex_buffer;
	memset(s_render_state, 0xff, sizeof(s_render_state));

	s_program = 0;
	s_blend_src_factor = GL_SRC_ALPHA;
	s_blend_dst_factor = GL_ONE_MINUS_SRC_ALPHA;
	s_blend_equation = GL_FUNC_ADD;

	// set dummy GL stete
#if COCOS2D_VERSION >= 0x00030000
	cocos2d::GL::enableVertexAttribs(0);
	cocos2d::GL::useProgram(0);
	cocos2d::GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// include glEnable(GL_BLEND)
	cocos2d::GL::bindTexture2D(0);
#else
	cocos2d::ccGLEnableVertexAttribs(0);
	cocos2d::ccGLUseProgram(0);
	cocos2d::ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// include glEnable(GL_BLEND)
	cocos2d::ccGLBindTexture2D(0);
#endif
}

void SSRenderer::end()
{
	if(s_draw_count > 0){
		flush();
		s_draw_count = 0;
	}

	// restore GL state
	if(s_program != 0){
		GLuint n;

		n = s_program->vertex_shader()->attrib_num();
		while(n--)
			glDisableVertexAttribArray(n);
		glUseProgram(0);
		if(s_blend_src_factor != GL_SRC_ALPHA || s_blend_dst_factor != GL_ONE_MINUS_SRC_ALPHA)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if(s_blend_equation != GL_FUNC_ADD)
			glBlendEquation(GL_FUNC_ADD);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

#if COCOS2D_VERSION >= 0x00030000
void SSRenderer::draw(const SSPartState* part_state, cocos2d::Texture2D* texture, uint32_t color)
#else
void SSRenderer::draw(const SSPartState* part_state, cocos2d::CCTexture2D* texture, uint32_t color)
#endif
{
	RenderState* next;
	uint32_t dirty;
	uint8_t color_blend;
	PrimitiveType primitive_type;

	next = &s_render_state[0];
	dirty = 0;

	{
		uint32_t opacity;

		opacity = part_state->opacity;
		opacity += (opacity >> 7) & 1;// 0xff -> 0x100
		opacity = (opacity * (color & 0xff)) >> 8;
		color = (color & 0xffffff00) | opacity;
		if(color != next->color){
			dirty |= DirtyColor;
			next->color = color;
		}
	}

	if(texture != next->texture){
		dirty |= DirtyTexture;
		next->texture = texture;
	}

	{
		uint8_t color_blend_type;

		color_blend = part_state->color_blend;
		if(color_blend != next->color_blend){
			dirty |= DirtyColorBlend;
			next->color_blend = part_state->color_blend;
		}
		if(color_blend != SSPartBlendNone){
			color_blend_type = part_state->color_blend_type;
			if(color_blend_type != next->color_blend_type){
				dirty |= DirtyColorBlendType;
				next->color_blend_type = color_blend_type;
			}
		}
		if(color_blend == SSPartBlendSingle){
			uint32_t color;
			float color_blend_rate;

			color = part_state->lt.color.rgba;
			color_blend_rate = part_state->lt.color_blend_rate;
			if(color != next->color_blend_color || color_blend_rate != next->color_blend_rate){
				dirty |= DirtyColorBlendColor;
				next->color_blend_color = color;
				next->color_blend_rate = color_blend_rate;
			}
		}
	}

	if(part_state->flags & SSPartFlagVertexTransform)
		primitive_type = PrimitiveTypeTriangle;
	else
		primitive_type = PrimitiveTypeStrip;
	if(primitive_type != next->primitive_type){
		dirty |= DirtyPrimitiveType;
		next->primitive_type = primitive_type;
	}

	{
		uint8_t blend;

		blend = part_state->blend;
		if(blend != next->blend){
			dirty |= DirtyBlend;
			next->blend = blend;
		}
	}

	if(dirty){
		next->dirty = dirty;
		flush_n();
	}


	if(color_blend == SSPartBlendPerVertex){
		VertexColor* v;

		v = (VertexColor*)s_vertex_buffer_cur;

		v[0].x = part_state->lt.x;
		v[0].y = part_state->lt.y;
		v[0].uv = part_state->lt.uv;
		v[0].color = part_state->lt.color.rgba;
		v[0].blend_rate = part_state->lt.color_blend_rate;

		v[1].x = part_state->lb.x;
		v[1].y = part_state->lb.y;
		v[1].uv = part_state->lb.uv;
		v[1].color = part_state->lb.color.rgba;
		v[1].blend_rate = part_state->lb.color_blend_rate;

		v[2].x = part_state->rt.x;
		v[2].y = part_state->rt.y;
		v[2].uv = part_state->rt.uv;
		v[2].color = part_state->rt.color.rgba;
		v[2].blend_rate = part_state->rt.color_blend_rate;

		v[3].x = part_state->rb.x;
		v[3].y = part_state->rb.y;
		v[3].uv = part_state->rb.uv;
		v[3].color = part_state->rb.color.rgba;
		v[3].blend_rate = part_state->rb.color_blend_rate;

		if(primitive_type == PrimitiveTypeTriangle){
			v[4].x = part_state->cc.x;
			v[4].y = part_state->cc.y;
			v[4].uv = part_state->cc.uv;
			v[4].color = part_state->cc.color.rgba;
			v[4].blend_rate = part_state->cc.color_blend_rate;

			s_vertex_buffer_cur = &v[5];
		}else{
			s_vertex_buffer_cur = &v[4];
		}

	}else{
		VertexNormal* v;

		v = (VertexNormal*)s_vertex_buffer_cur;

		v[0].x = part_state->lt.x;
		v[0].y = part_state->lt.y;
		v[0].uv = part_state->lt.uv;

		v[1].x = part_state->lb.x;
		v[1].y = part_state->lb.y;
		v[1].uv = part_state->lb.uv;

		v[2].x = part_state->rt.x;
		v[2].y = part_state->rt.y;
		v[2].uv = part_state->rt.uv;

		v[3].x = part_state->rb.x;
		v[3].y = part_state->rb.y;
		v[3].uv = part_state->rb.uv;

		if(primitive_type == PrimitiveTypeTriangle){
			v[4].x = part_state->cc.x;
			v[4].y = part_state->cc.y;
			v[4].uv = part_state->cc.uv;

			s_vertex_buffer_cur = &v[5];
		}else{
			s_vertex_buffer_cur = &v[4];
		}
	}


	s_draw_count++;
	if(s_draw_count >= MAX_BATCH_COUNT)
		flush_n();
}

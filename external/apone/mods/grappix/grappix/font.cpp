#include "GL_Header.h"

#include "texture-font.h"
#include "texture-atlas.h"

#include "font.h"
#include "shader.h"
#include "color.h"
#include "staticfont.h"
#include "render_target.h"

#include <coreutils/utils.h>
#include <coreutils/log.h>
#include <coreutils/file.h>

#define _USE_MATH_DEFINES
#include <cmath>

#include <sys/stat.h>

#include <vector>
using namespace std;
using namespace utils;

namespace grappix {

std::vector<std::weak_ptr<Font::FontRef>> Font::fontRefs;

uint8_t *make_distance_map(uint8_t *img, int width, int height);

Font::Font(bool stfont) : size(32) {

	program = get_program(FONT_PROGRAM_DF);

	ref = make_shared<FontRef>(0, 0, "", 0.0, 0);
	texture_atlas_t *atlas = new texture_atlas_t();
	ref->atlas = atlas;
	atlas->width = static_font.tex_width;
	atlas->height = static_font.tex_height;
	atlas->depth = 1;
	atlas->id = 0;
	atlas->data = static_font.tex_data;
	texture_atlas_upload(atlas);
	LOGD("Static font created");
}



// The glyph set baked into every font's atlas. Characters outside this set can
// never be drawn (make_text has no glyph for them). It must therefore cover
// every character that can reach a Font -- notably the accented Latin, curly
// quotes, dashes and symbols that appear in the song/format descriptions shown
// by the scroller. A font that lacks a glyph for one of these simply won't have
// it baked (see the .notdef skip in texture_font_load_glyphs); Font::covers()
// reports that, and the scroller transliterates the character to an ASCII
// fallback for that font only. NOTE: bump the ".NNN.dfield" cache version in the
// DISTANCE_MAP branch below whenever this string changes, or stale distance-map
// atlases baked from the old set will be reused.
const static wchar_t *fontLetters = L"@!ABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖabcdefghijklmnopqrstuvwxyzåäö0123456789 []/:<>,.-()'&?%#+\"$;=\\_µçéíøćŚΔ–—’“”′";
const static wchar_t *fontLettersUpper = L"@!ABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ0123456789 []/:<>,.-()'&?%#+\"$;=\\_µÇÉÍØĆŚΔ–—’“”′";


Font::Font(const string &ttfName, int size, int flags) : size(size) {

	//LOGD("TTF:%s", ttfName);
	// flags &= ~DISTANCE_MAP;
	program = flags & DISTANCE_MAP ? get_program(FONT_PROGRAM_DF) : get_program(FONT_PROGRAM);

	int tsize = flags & 0xffffc0;
	if(tsize == 0) tsize = 128;
	flags &= 0x3f;

	for(auto &fr : fontRefs) {
		auto r = fr.lock();
		if(r) {
			//LOGD("%s(%d) vs %s(%d)",r->ttfName, r->w, ttfName, tsize);
			if(r->ttfName == ttfName && r->w == tsize && r->h == tsize && r->flags == flags) {
				ref = r;
				LOGD("Reusing Font %s (%d)", ttfName, size);
				return;
			}
		}
	}

	ref = make_shared<FontRef>(tsize, tsize, ttfName, size, flags);
	fontRefs.push_back(ref);

	auto text = fontLetters;
	if(flags & UPPER_CASE)
		text = fontLettersUpper;

	texture_font_load_glyphs((texture_font_t*)ref->font, text);

	texture_atlas_t *atlas = (texture_atlas_t*)ref->atlas;
	if(flags & DISTANCE_MAP) {
		auto fn = path_filename(ttfName);
		struct stat st{};
		unsigned long long fmtime = 0, fsize = 0;
		if(::stat(ttfName.c_str(), &st) == 0) {
			fmtime = (unsigned long long)st.st_mtime;
			fsize = (unsigned long long)st.st_size;
		}
		File f { format("%s/%s.%d.%d.%llu.%llu_v5.dfield", File::getCacheDir().getName(), fn, size, tsize, fmtime, fsize) };
		if(f.exists()) {
			f.read(atlas->data, atlas->width*atlas->height);
		} else {
			uint8_t *data = make_distance_map(atlas->data, atlas->width, atlas->height);
			LOGD("%s: Distance map created", fn);
			
			// FIX: Overwrite the buffer contents directly. Do NOT swap the pointer
			// or mix allocator zones (malloc vs new[]) which corrupts the macOS heap.
			if (data) {
				memcpy(atlas->data, data, atlas->width * atlas->height);
				// Clean up the temporary buffer returned by the generator
				// (Using free here is fine for the temporary copy, or replace with delete[] if needed)
				free(data); 
			}
			
			f.write(atlas->data, atlas->width*atlas->height);
		}
		f.close();
	}
	texture_atlas_upload(atlas);
}

//static float scale = 1.0;
template <typename T> static void push_back(vector<T> &vec) {} // The end

template <typename T, typename U, typename...ARGS> static void push_back(vector<T> &vec, const U &u, const ARGS& ... args) {
	vec.push_back(u);
	push_back(vec, args...);
}

TextBuf Font::make_text2(const wstring &text) const {

	vector<GLfloat> verts;
	vector<GLushort> indexes;

	int i = 0;
	float x = 0;
	float y = 0;

	for(auto c : text) {

		texture_glyph2_t *glyph = 0;
		for(unsigned int j=0; j<static_font.glyphs_count; ++j) {
			if(static_font.glyphs[j].charcode == c) {
				glyph = &static_font.glyphs[j];
				break;
			}
		}
		if(!glyph) {
			x += 8.0;
			continue;
		}

		float x0  = x + glyph->offset_x;
		float x1  = x0 + glyph->width;

		float y1  = y + static_font.height;
		float y0  = y1 - glyph->offset_y;

		float s0 = glyph->s0;
		float t0 = glyph->t0;
		float s1 = glyph->s1;
		float t1 = glyph->t1;

		x += glyph->advance_x;

		push_back(verts, x0, y0, s0, t0);
		push_back(verts, x1, y0, s1, t0);
		push_back(verts, x0, y1, s0, t1);
		push_back(verts, x1, y1, s1, t1);

		push_back(indexes, i, i+1, i+2, i+1, i+3, i+2);

		i += 4;
		//break;
	}

	TextBuf tbuf;
	//vector<GLuint> vbuf(2);
	if(verts.size() >= 4) {

		tbuf.rec[0] = verts[0];
		tbuf.rec[1] = 0;//verts[1];
		tbuf.rec[2] = verts[verts.size()-4];
		tbuf.rec[3] = static_font.height;//verts[verts.size()-3];
	} else {
		tbuf.rec[0] = tbuf.rec[1] = tbuf.rec[2] = tbuf.rec[3] = 0;
	}
	tbuf.text = text;
	tbuf.size = i/4;
	glGenBuffers(2, &tbuf.vbuf[0]);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf.vbuf[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tbuf.vbuf[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * 2, &indexes[0], GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * 4, &verts[0], GL_STATIC_DRAW);

	return tbuf;
}

TextBuf Font::make_text(const wstring &text) const {

	// A default-constructed Font (no ref) or a font with no glyph face falls back
	// to the built-in static font rather than dereferencing a null ref.
	if(!ref || !ref->font)
		return make_text2(text);

	int lastChar = 0;
	int i = 0;
	float x = 0;
	texture_font_t *font = (texture_font_t*)ref->font;

	float y = font->ascender;

	int tl = text.length();

	vector<GLfloat> verts;
	vector<GLushort> indexes;

	verts.reserve(16*tl);
	indexes.reserve(4*tl);

	for(auto c : text) {

		// INTERCEPT: Scan freetype-gl's local glyph vector directly
		texture_glyph_t *glyph = nullptr;
		if (font->glyphs) {
			for (size_t g_idx = 0; g_idx < font->glyphs->size; ++g_idx) {
				texture_glyph_t *g = *(texture_glyph_t **)vector_get(font->glyphs, g_idx);
				if (g && g->charcode == c) {
					glyph = g;
					break;
				}
			}
		}
		
		// Fallback: Check if fallback space character exists in the baked vector
		if(!glyph && font->glyphs) {
			for (size_t g_idx = 0; g_idx < font->glyphs->size; ++g_idx) {
				texture_glyph_t *g = *(texture_glyph_t **)vector_get(font->glyphs, g_idx);
				if (g && g->charcode == L' ') {
					glyph = g;
					break;
				}
			}
		}

		if(!glyph)
			continue;

		if(lastChar)
			x += texture_glyph_get_kerning(glyph, lastChar);
		lastChar = c;

		float x0  = x + glyph->offset_x;
		float x1  = x0 + glyph->width;

		float y0  = y - glyph->offset_y;
		float y1  = y0 + glyph->height;

		float s0 = glyph->s0;
		float t0 = glyph->t0;
		float s1 = glyph->s1;
		float t1 = glyph->t1;

		push_back(verts, x0, y0, s0, t0);
		push_back(verts, x1, y0, s1, t0);
		push_back(verts, x0, y1, s0, t1);
		push_back(verts, x1, y1, s1, t1);

		push_back(indexes, i, i+1, i+2, i+1, i+3, i+2);

		i += 4;
		x += glyph->advance_x;
	}

	TextBuf tbuf;

	tbuf.text = text;
	tbuf.size = i/4;

	if(verts.size() >= 4) {

		tbuf.rec[0] = verts[0];
		tbuf.rec[1] = 0;
		tbuf.rec[2] = verts[verts.size()-4];
		tbuf.rec[3] = font->height;
	} else {
		tbuf.rec[0] = tbuf.rec[1] = tbuf.rec[2] = tbuf.rec[3] = 0;
	}
	glGenBuffers(2, &tbuf.vbuf[0]);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf.vbuf[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tbuf.vbuf[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * 2, &indexes[0], GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * 4, &verts[0], GL_STATIC_DRAW);

	return tbuf;
}

void Font::render_text(const RenderTarget &target, const TextBuf &text, float x, float y, uint32_t color, float scale) const {

	if (text.size == 0 || text.vbuf[0] == 0)
		return;

	scale = scale * 32.0 / (float)size;

	glBindFramebuffer(GL_FRAMEBUFFER, target.buffer());
	glViewport(0,0,target.width(), target.height());

	program.use();

	glBindBuffer(GL_ARRAY_BUFFER, text.vbuf[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text.vbuf[1]);

	mat4f matrix = make_scale(scale, scale);
	matrix = make_translate(x, y) * matrix;
	matrix = target.get_view_matrix() * matrix;
	program.setUniform("matrix", matrix.transpose());

	// Needed for DF shader
	program.setUniform("vScale", scale);

	program.setUniform("color", Color(color));

	program.vertexAttribPointer("vertex", 2, GL_FLOAT, GL_FALSE, 16, 0);
	program.vertexAttribPointer("uv", 2, GL_FLOAT, GL_FALSE, 16, 8);

	texture_atlas_t *atlas = (texture_atlas_t*)ref->atlas;
	glBindTexture( GL_TEXTURE_2D, atlas->id );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glDrawElements(GL_TRIANGLES, 6*text.size, GL_UNSIGNED_SHORT, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Font::render_text(const RenderTarget &target, const std::string &text, float x, float y, uint32_t col, float scale) const {
	if(text.empty())
		return;

	auto t = utf8_decode_wide(text);
	auto buf = cache.get(t);
	
	if(buf.text.empty() || buf.vbuf[0] == 0) {
		// Clean up old VRAM handle if text was empty or invalid but somehow tracked
		if (buf.vbuf[0] != 0) {
			glDeleteBuffers(2, &buf.vbuf[0]);
		}
		buf = make_text(t);
		cache.put(t, buf);
	}
	render_text(target, buf, x, y, col, scale);
}

void clean_cache() {
}


int Font::get_width(const string &text, float scale) const {
	return get_size(text, scale).x;
}

vec2i Font::get_size(const string &t, float scale) const {

	auto text = utf8_decode_wide(t);

	if(text.empty())
		return vec2i(0,0);
	auto buf = cache.get(text);
	if(buf.text.empty() || buf.vbuf[0] == 0) {
		if (buf.vbuf[0] != 0) {
			glDeleteBuffers(2, &buf.vbuf[0]);
		}
		buf = make_text(text);
		cache.put(text, buf);
	}
	scale = scale * 32.0 / (float)size;
	return vec2i(buf.rec[2] - buf.rec[0], buf.rec[3] - buf.rec[1]) * scale;
}


bool Font::covers(wchar_t c) const {
	// The static (built-in) font: check its fixed glyph table.
	if(!ref || !ref->font) {
		for(unsigned int j=0; j<static_font.glyphs_count; ++j)
			if(static_font.glyphs[j].charcode == c)
				return true;
		return false;
	}
	// A TTF/OTF font: a glyph is present in the baked vector only if the font
	// actually had it (texture_font_load_glyphs skips .notdef). So membership
	// here is an accurate "can this font draw c?" -- provided c is part of the
	// baked set (fontLetters); characters outside that set are never baked and
	// correctly report false.
	texture_font_t *font = (texture_font_t*)ref->font;
	if(!font->glyphs)
		return false;
	for(size_t g=0; g<font->glyphs->size; ++g) {
		texture_glyph_t *g0 = *(texture_glyph_t **)vector_get(font->glyphs, g);
		if(g0 && g0->charcode == c)
			return true;
	}
	return false;
}

Font::FontRef::FontRef(int w, int h, const std::string &ttfName, int fsize, int flags) : w(w), h(h), flags(flags), ttfName(ttfName), atlas(nullptr), font(nullptr) {
	texture_atlas_t *a = nullptr;
	if(w > 0 && h > 0)
		a = texture_atlas_new(w, h, 1);
	if(a && fsize > 0) {
		font = (texture_font_t*)texture_font_new(a, ttfName.c_str(), fsize);
	}
	atlas = a;
}
Font::FontRef::~FontRef() {
	if(font)
		texture_font_delete((texture_font_t*)font);
	if(atlas)
		texture_atlas_delete((texture_atlas_t*)atlas);
	font = nullptr;
	atlas = nullptr;
}


void TextBuf::destroy() {
	if (vbuf[0] != 0) {
		glDeleteBuffers(2, &vbuf[0]);
		vbuf[0] = 0;
		vbuf[1] = 0;
	}
}

}
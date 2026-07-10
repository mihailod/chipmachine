#ifndef SCOLLER_H
#define SCOLLER_H

#include "Effect.h"
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include <coreutils/file.h>
#include <grappix/grappix.h>
#include <coreutils/environment.h>

namespace demofx {

class Scroller : public Effect {
public:
	explicit Scroller(grappix::RenderTarget &target) : target(target), scr(grappix::screen.width()+10, 300) {
		program = grappix::get_program(grappix::TEXTURED_PROGRAM).clone();

		// Load the sine-scroll fragment shader INLINE and synchronously, exactly
		// like the font shader below. The previous implementation loaded it via
		// Resources::load() from a cache file (getCacheDir()/sine_shader.glsl) --
		// but that loader PREFERS an existing on-disk file over the inline default
		// (see resources.h TypedResource::load). A stale cache file left over from
		// an earlier build therefore silently overrides any change to sineShaderF,
		// which is exactly why edits to this effect appeared to do "nothing". Bind
		// the source directly so the compiled-in shader is always the one that runs.
		try {
			program.setFragmentSource(sineShaderF);
		} catch(grappix::shader_exception &e) {
			// Make failures LOUD: if this throws, `program` keeps the plain
			// textured shader (no gradient, no wobble) and the scroll looks
			// completely unchanged. Print the real GL log instead of swallowing it.
			LOGD("SINE SCROLL SHADER FAILED TO COMPILE: %s", e.what());
		}

		fprogram = grappix::get_program(grappix::FONT_PROGRAM_DF).clone();
		fprogram.setFragmentSource(fontShaderF);
		font.set_program(fprogram);
	}

	void resize(int w, int h) override {
		// Texture is (re)sized in render() to track the window/text scale; just
		// scale the height with the target here so the first frame isn't clipped.
		int texH = (int)(100 * (target.height() / 576.0f) * scrollsize);
		if(texH < 8) texH = 300;
		if(w > 8)
			scr = grappix::Texture(w+10, texH);
	}
	// --- Rotating font pool -------------------------------------------------
	// The scroller cycles through a set of fonts (loaded from a folder at
	// startup, see ChipMachine::loadScrollFonts). Every font is fully built up
	// front so a swap -- automatic (font_swap_interval) or manual (CTRL+N) -- is
	// instant, with no glyph-load or distance-map hitch mid-scroll.
	void clearFonts() {
		fonts.clear();
		fontNames.clear();
		fontIndex = 0;
		font_swap_timer = 0.0f;
	}

	// Build a font and add it to the pool. The first one added becomes active.
	void addFont(const std::string &path) {
		grappix::Font f(path, 120, 1024 | grappix::Font::DISTANCE_MAP);
		f.set_program(fprogram);
		fonts.push_back(f);
		fontNames.push_back(baseName(path));
		if(fonts.size() == 1) {
			font = fonts[0];
			fontIndex = 0;
		}
	}

	// Advance to the next font in the pool (wrapping). Returns the basename of
	// the now-active font (for the on-screen toast); resets the auto-swap timer
	// so a manual CTRL+N gives a full interval before the next automatic swap.
	std::string nextFont() {
		if(fonts.empty())
			return "";
		int oldIndex = fontIndex;
		int newIndex = (fontIndex + 1) % (int)fonts.size();
		// Reparametrize xpos so the glyph currently under screen-centre stays
		// under screen-centre in the new font -- otherwise the differing glyph
		// widths make the text visibly jump/skip/repeat at the swap moment.
		if(newIndex != oldIndex)
			anchorSwap(fonts[oldIndex], fonts[newIndex]);
		fontIndex = newIndex;
		font = fonts[newIndex];
		font_swap_timer = 0.0f;
		return fontNames[fontIndex];
	}

	std::string currentFontName() const {
		return fonts.empty() ? std::string() : fontNames[fontIndex];
	}
	size_t fontCount() const { return fonts.size(); }

	void set(const std::string &what, const std::string &val, float seconds = 0.0) override {
		if(what == "font") {
			font = grappix::Font(val, 120, 1024 | grappix::Font::DISTANCE_MAP);
			font.set_program(fprogram);
		} else if(what == "sine_amplitude") {
			sine_amplitude = std::stof(val);
		} else if(what == "sine_frequency") {
			sine_frequency = std::stof(val);
		} else if(what == "sine_speed") {
			sine_speed = std::stof(val);
		} else if(what == "sine_on") {
			// The lua value arrives via std::to_string(double) as "1.000000",
			// which is neither "true" nor "1" -- the old exact-string check turned
			// the whole effect OFF and made it look like the feature did nothing.
			// Parse numerically (atof never throws; returns 0.0 for "true", which
			// the explicit check below still handles).
			sine_on = (val == "true") || (atof(val.c_str()) != 0.0);
		} else if(what == "sine_interval") {
			sine_interval = std::stof(val);
		} else if(what == "sine_transition") {
			sine_transition = std::stof(val);
		} else if(what == "vbob_amplitude") {
			vbob_amplitude = std::stof(val);
		} else if(what == "vbob_speed") {
			vbob_speed = std::stof(val);
		} else if(what == "vbob_on") {
			vbob_on = (val == "true") || (atof(val.c_str()) != 0.0);
		} else if(what == "vbob_interval") {
			vbob_interval = std::stof(val);
		} else if(what == "vbob_transition") {
			vbob_transition = std::stof(val);
		} else {
			scrollText = val;
			LOGD("SCROLL: %s", scrollText);
			xpos = target.width() + 100;
		}
	}

	void render(uint32_t delta) override {
		if(alpha <= 0.01)
			return;

		// Calculate dynamic scale factor based on target resolution.
		// scrollsize is the FONT-SIZE multiplier (Settings.scroll[2] in lua); it
		// used to be dead. gscale keeps the visual size consistent across window
		// sizes; scrollsize is the user-tunable knob on top of that.
		float gscale = target.height() / 576.0f;
		float dynScale = gscale * scrollsize;

		// The render texture must grow with the text scale, otherwise large
		// windows clip the glyphs at the old fixed 300px height (the bottoms
		// of the letters disappear). Keep it 1:1 with on-screen pixels.
		int texW = target.width() + 10;
		// Texture height tracks the font size so larger scrollsize values don't
		// clip the glyph tops/bottoms (100 * 3 == the old fixed 300 at the default).
		int texH = (int)(100 * gscale * scrollsize);
		if(texH < 8) texH = 300;
		if((int)scr.width() != texW || (int)scr.height() != texH)
			scr = grappix::Texture(texW, texH);

		// Keep the reset boundary in sync with the scale actually rendered
		// (also covers window resizes after the text was set).
		scrollLen = font.get_width(scrollText, dynScale);
		if(xpos < -scrollLen)
			xpos = target.width() + 100;

		scr.clear(0x00000000);
		// Advance by a constant on-screen velocity (pixels per second) rather
		// than a fixed step per frame. scrollspeed is calibrated for 60 FPS, so
		// scale it by the real frame time. This keeps the scroll perfectly
		// smooth even when frame pacing is irregular (vsync/present jitter) --
		// a fixed per-frame step turned that jitter directly into stutter.
		// delta is clamped so a one-off long frame (e.g. after a window resize
		// or load hitch) can't teleport the text.
		float dt = (float)delta;
		if(dt > 50.0f) dt = 50.0f;
		// Scale the step by gscale -- the same factor the glyphs are scaled by --
		// so the scroll moves at a consistent VISUAL speed regardless of window
		// size. A fixed pixel step looks fast in a small window and slow in a big
		// one, because the text grows with the window but the movement didn't.
		xpos -= scrollspeed * gscale * (dt / (1000.0f / 60.0f));
		// Render text using dynamic scale factor; baseline centred in texture.
		scr.text(font, scrollText, xpos, texH / 2.0f, 0xffffffff, dynScale);

		time_counter += dt / 1000.0f;

		// Automatic font rotation: after font_swap_interval seconds of real time,
		// swap to the next font in the pool. Disabled when interval <= 0 or when
		// there is nothing to rotate to (0 or 1 fonts loaded).
		if(font_swap_interval > 0.0f && fonts.size() > 1) {
			font_swap_timer += dt / 1000.0f;
			if(font_swap_timer >= font_swap_interval)
				nextFont(); // also resets font_swap_timer
		}

		float cycle_time = 0.0f;
		if (sine_interval > 0.0f) {
			cycle_time = fmod(time_counter, 2.0f * sine_interval);
		}
		// Start the cycle in the SINE phase so the wobble is the first thing you
		// see (cycle_time < sine_interval == first half of the period). Otherwise
		// the first ~sine_interval seconds look identical to a plain flat scroll,
		// which reads as "the effect isn't working".
		float target_factor = (sine_on && (sine_interval <= 0.0f || cycle_time < sine_interval)) ? 1.0f : 0.0f;
		if (sine_transition > 0.0f) {
			if (current_amplitude_factor < target_factor) {
				current_amplitude_factor += (dt / 1000.0f) / sine_transition;
				if (current_amplitude_factor > target_factor) current_amplitude_factor = target_factor;
			} else if (current_amplitude_factor > target_factor) {
				current_amplitude_factor -= (dt / 1000.0f) / sine_transition;
				if (current_amplitude_factor < target_factor) current_amplitude_factor = target_factor;
			}
		} else {
			current_amplitude_factor = target_factor;
		}

		program.use();
		program.setUniform("uTime", time_counter * sine_speed);
		program.setUniform("uAmplitude", sine_amplitude * current_amplitude_factor);
		program.setUniform("uFrequency", sine_frequency);

		// Occasional vertical bob: lift the WHOLE scroller up and down now and
		// then, like the classic Amiga "bouncing" scrollers. This is separate from
		// the per-column sine wobble above (that displaces the texture lookup; this
		// translates the whole strip). Gated on its own interval so it happens
		// occasionally, with a smooth fade in/out via vbob_transition -- same shape
		// as the sine gating.
		float vbob_cycle = 0.0f;
		if (vbob_interval > 0.0f)
			vbob_cycle = fmod(time_counter, 2.0f * vbob_interval);
		float vbob_target = (vbob_on && (vbob_interval <= 0.0f || vbob_cycle < vbob_interval)) ? 1.0f : 0.0f;
		if (vbob_transition > 0.0f) {
			if (vbob_factor < vbob_target) {
				vbob_factor += (dt / 1000.0f) / vbob_transition;
				if (vbob_factor > vbob_target) vbob_factor = vbob_target;
			} else if (vbob_factor > vbob_target) {
				vbob_factor -= (dt / 1000.0f) / vbob_transition;
				if (vbob_factor < vbob_target) vbob_factor = vbob_target;
			}
		} else {
			vbob_factor = vbob_target;
		}
		// Amplitude is in pixels at gscale 1.0, scaled by gscale so the bob height
		// is visually consistent across window sizes. sin() starts at 0 so there
		// is no jump when a bob episode fades in.
		float voffset = vbob_amplitude * gscale * vbob_factor
		              * sin(time_counter * vbob_speed);

		static float uvs[] = { 0,0,1,0,0,1,1,1 };
		target.draw(scr, 0.0F, scrolly - texH / 2.0f + voffset, target.width(), texH, uvs, program);
	}

	float alpha = 1.0;

	// Pixels advanced per 60 FPS-frame (scaled by real frame time in render()).
	// NOTE: this default is overridden at startup by SCROLL_SPEED in
	// lua/screen.lua (via Settings.scroll) -- tune the speed THERE, not here.
	int scrollspeed = 8;
	int scrolly = 0;
	// Font-size multiplier for the scroll text (Settings.scroll[2]); the on-screen
	// glyph scale is gscale * scrollsize. Default 3.0 == the previous hardcoded size.
	float scrollsize = 3.0;

	// Tweakable parameters for sinusoid scroll
	float sine_amplitude = 0.15f;
	float sine_frequency = 8.0f;
	float sine_speed = 4.0f;
	bool sine_on = true;
	float sine_interval = 10.0f;
	float sine_transition = 1.0f;

	// Tweakable parameters for the occasional vertical bob (whole-strip up/down)
	float vbob_amplitude = 40.0f;  // how high it goes, in px at gscale 1.0
	float vbob_speed = 2.0f;       // how fast it oscillates up/down
	bool vbob_on = true;           // enable/disable the bob
	float vbob_interval = 12.0f;   // how often: bob for N s, rest N s (0 = always)
	float vbob_transition = 1.0f;  // fade in/out time between bob and rest

	float time_counter = 0.0f;
	float current_amplitude_factor = 0.0f;
	float vbob_factor = 0.0f;      // current eased bob on/off amount (internal)

	// Seconds between automatic font swaps (Settings.scroll[16] in lua). 0 = off.
	float font_swap_interval = 60.0f;

private:
	// Basename (strip directory) for on-screen font-name toasts.
	static std::string baseName(const std::string &path) {
		auto slash = path.find_last_of("/\\");
		return slash == std::string::npos ? path : path.substr(slash + 1);
	}

	// Keep the glyph under the horizontal centre of the screen fixed across a
	// font swap. The scroller draws the whole string starting at texture-x `xpos`
	// (which the final blit maps 1:1 onto screen width), so the text-space offset
	// currently under centre is `offOld = centre - xpos` measured in the OLD
	// font's pixels. We find which character boundary that offset falls on (plus
	// the fraction into that glyph), remeasure the same boundary in the NEW font,
	// and solve for the xpos that puts that identical point back under centre.
	// Without this, xpos is unchanged while the total width changes, so the
	// centre offset suddenly lands on a different character -- the visible jump.
	void anchorSwap(const grappix::Font &oldF, const grappix::Font &newF) {
		if(scrollText.empty() || scr.width() <= 0)
			return;

		const float gscale = target.height() / 576.0f;
		const float dynScale = gscale * scrollsize;
		const float centre = scr.width() / 2.0f;      // texture-space screen centre
		const float offOld = centre - xpos;           // text offset under centre (old px)

		const float lenOld = (float)oldF.get_width(scrollText, dynScale);
		// Centre sits on the blank gap before/after the text: nothing to anchor,
		// and leaving xpos alone keeps that gap continuous. (Avoids div-by-zero.)
		if(offOld <= 0.0f || offOld >= lenOld || lenOld <= 0.0f)
			return;

		// Byte offsets of every UTF-8 character boundary (0 .. length()).
		std::vector<int> bnd;
		bnd.push_back(0);
		for(int p = 0; p < (int)scrollText.size();) {
			unsigned char c = (unsigned char)scrollText[p];
			int adv = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
			p += adv;
			if(p > (int)scrollText.size()) p = (int)scrollText.size();
			bnd.push_back(p);
		}
		const int nchars = (int)bnd.size() - 1;

		// Width (old font) of the first k characters. Monotonic in k.
		auto prefixOld = [&](int k) -> float {
			if(k <= 0) return 0.0f;
			if(k >= nchars) return lenOld;
			return (float)oldF.get_width(scrollText.substr(0, bnd[k]), dynScale);
		};

		// Largest k with prefixOld(k) <= offOld  ->  the glyph under centre is k.
		int lo = 0, hi = nchars;
		while(lo < hi) {
			int mid = (lo + hi + 1) / 2;
			if(prefixOld(mid) <= offOld) lo = mid; else hi = mid - 1;
		}
		const int k = lo;                          // 0 <= k <= nchars-1

		const float wOldK  = prefixOld(k);
		const float wOldK1 = prefixOld(k + 1);
		const float frac = (wOldK1 > wOldK) ? (offOld - wOldK) / (wOldK1 - wOldK) : 0.0f;

		// Same character boundary, measured in the NEW font.
		auto prefixNew = [&](int j) -> float {
			if(j <= 0) return 0.0f;
			return (float)newF.get_width(scrollText.substr(0, bnd[j]), dynScale);
		};
		const float offNew = prefixNew(k) + frac * (prefixNew(k + 1) - prefixNew(k));

		xpos = centre - offNew;
	}

	grappix::RenderTarget& target;
	grappix::Font font;
	std::vector<grappix::Font> fonts;      // rotation pool (built at startup)
	std::vector<std::string> fontNames;    // basenames, parallel to `fonts`
	int fontIndex = 0;                     // active index into `fonts`
	float font_swap_timer = 0.0f;          // elapsed seconds since last swap
	grappix::Program program;
	grappix::Program fprogram;
	float xpos = -9999;
	grappix::Texture scr;
	std::string scrollText;
	int scrollLen{};


	const std::string sineShaderF = R"(
		#ifdef GL_ES
			precision highp float;
		#endif
		uniform sampler2D sTexture;
		uniform float uTime;
		uniform float uAmplitude;
		uniform float uFrequency;

		const vec4 color0 = vec4(1.0, 0.9, 0.2, 1.0); // Yellow/Orange
		const vec4 color1 = vec4(0.5, 0.2, 1.0, 1.0); // Purple/Blue

		varying vec2 UV;

		void main() {
			vec2 uv = UV;
			// Apply sinusoid vertical displacement
			uv.y += sin(uv.x * uFrequency + uTime) * uAmplitude;
			
			if (uv.y < 0.0 || uv.y > 1.0) {
				gl_FragColor = vec4(0.0);
			} else {
				float grad = smoothstep(0.3, 0.7, uv.y);
				vec4 rgb = mix(color0, color1, grad);
				vec4 color = texture2D(sTexture, uv);
				gl_FragColor = rgb * color;
			}
		}
	)";


	const std::string fontShaderF = R"(
		#ifdef GL_ES
			precision highp float;
		#endif
		uniform vec4 color;
		uniform float vScale;
		uniform sampler2D sTexture;
		varying vec2 UV;

		const float glyph_center = 0.50;

		void main() {
			float dist = texture2D(sTexture, UV).a;
			float smoothing = 0.03; 
			float alpha = smoothstep(glyph_center - smoothing, glyph_center + smoothing, dist);
			gl_FragColor = vec4(color.rgb, color.a * alpha);
		}
	)";


};

}

#endif // SCOLLER_H

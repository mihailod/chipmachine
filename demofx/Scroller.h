#ifndef SCOLLER_H
#define SCOLLER_H

#include "Effect.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <random>
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
	// startup, see ChipMachine::loadScrollFonts). Building a font is not cheap --
	// freetype rasterises the whole glyph set and (on the first ever run for a
	// font) a distance-map is computed -- so building the entire pool up front
	// added noticeable time to startup. Instead the pool is built LAZILY: only
	// the active font is built at startup (randomizeFont), and each other font is
	// built by ensureBuilt() the moment it is first swapped in. The automatic
	// swap fires only when the scroll text has scrolled fully off-screen (see the
	// xpos-reset in render()), so that one-off build cost lands on a frame where
	// the scroll strip is empty -- no visible stutter -- and it is at most one
	// build per scroll pass, never a burst.
	void clearFonts() {
		fontPaths.clear();
		fonts.clear();
		fontBuilt.clear();
		fontNames.clear();
		fontIndex = 0;
		font_swap_timer = 0.0f;
	}

	// Register a font path in the pool. Nothing is built here -- the heavy work
	// is deferred to ensureBuilt(), called when the font is first shown, so it
	// stays off the startup critical path.
	void addFont(const std::string &path) {
		fontPaths.push_back(path);
		fonts.emplace_back();               // placeholder; built on demand
		fontBuilt.push_back(0);
		fontNames.push_back(baseName(path));
	}

	// Build pool entry i if it isn't built yet (freetype glyph load + atlas
	// upload; distance map is cached on disk after the first run). Safe to call
	// from the render thread -- it only touches GL the same way startup did.
	void ensureBuilt(size_t i) {
		if(i >= fonts.size() || fontBuilt[i])
			return;
		grappix::Font f(fontPaths[i], 120, 1024 | grappix::Font::DISTANCE_MAP);
		f.set_program(fprogram);
		fonts[i] = f;
		fontBuilt[i] = 1;
	}


	// Advance to the next font in the pool (wrapping). Returns the basename of
	// the now-active font (for the on-screen toast); resets the auto-swap timer
	// so a manual CTRL+N gives a full interval before the next automatic swap.
	std::string nextFont() {
		if(fonts.empty())
			return "";
		int oldIndex = fontIndex;
		int newIndex = (fontIndex + 1) % (int)fonts.size();
		ensureBuilt(newIndex);
		// Reparametrize xpos so the glyph currently under screen-centre stays
		// under screen-centre in the new font -- otherwise the differing glyph
		// widths make the text visibly jump/skip/repeat at the swap moment.
		if(newIndex != oldIndex)
			anchorSwap(fonts[oldIndex], fonts[newIndex]);
		fontIndex = newIndex;
		font = fonts[newIndex];
		// The new font may transliterate a different subset of characters, so
		// rebuild the drawn string for it (anchorSwap above already accounted for
		// the per-font widths when repositioning xpos).
		rebuildDisplay();
		font_swap_timer = 0.0f;
		return fontNames[fontIndex];
	}

	std::string currentFontName() const {
		return fonts.empty() ? std::string() : fontNames[fontIndex];
	}
	size_t fontCount() const { return fonts.size(); }

	// Pick a random starting font from the pool. Called once after the pool is
	// loaded so each app launch begins on a different font (the rotation then
	// continues from there). No anchoring needed -- nothing is on screen yet.
	// Self-seeds from a high-res clock: the app never calls srand(), so plain
	// std::rand() would deterministically pick the SAME font every launch.
	void randomizeFont() {
		if(fonts.empty())
			return;
		if(fonts.size() >= 2) {
			std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now()
			                     .time_since_epoch().count());
			fontIndex = std::uniform_int_distribution<int>(0, (int)fonts.size() - 1)(rng);
		} else {
			fontIndex = 0;
		}
		// Build the chosen starting font now (the only one built synchronously at
		// startup); every other font is built on the swap that first shows it.
		ensureBuilt(fontIndex);
		font = fonts[fontIndex];
		rebuildDisplay();
	}

	void set(const std::string &what, const std::string &val, float seconds = 0.0) override {
		if(what == "font") {
			font = grappix::Font(val, 120, 1024 | grappix::Font::DISTANCE_MAP);
			font.set_program(fprogram);
			rebuildDisplay();
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
			// A completely new scroll text also cycles to the next font (same
			// intent as the wrap-around swap -- the font only changes at a
			// natural boundary, never mid-pass). Guard on the previous text
			// being non-empty so the very first assignment keeps font 0.
			if(fonts.size() > 1 && !rawScrollText.empty() && val != rawScrollText) {
				fontIndex = (fontIndex + 1) % (int)fonts.size();
				ensureBuilt(fontIndex);
				font = fonts[fontIndex];
			}
			rawScrollText = val;
			rebuildDisplay();
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
		if(xpos < -scrollLen) {
			xpos = target.width() + 100;
			// Font rotation is tied to the scroll CYCLE, not a wall-clock
			// timer: keep the same font for a whole pass and only advance
			// here, the instant the text has fully scrolled off and is about
			// to repeat. Swapping mid-scroll surprised users; this way the
			// font only ever changes on the "blank" between repeats. The text
			// is off-screen at this moment, so no glyph-anchoring is needed
			// (that's only for CTRL+N swaps in the middle of a pass).
			if(fonts.size() > 1 && !rawScrollText.empty()) {
				fontIndex = (fontIndex + 1) % (int)fonts.size();
				ensureBuilt(fontIndex);   // may not be built yet (lazy pool)
				font = fonts[fontIndex];
				rebuildDisplay();
			}
		}

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

		// (Automatic font rotation is handled at the scroll wrap-around above,
		// not on a timer -- see the xpos reset. CTRL+N still swaps instantly
		// via nextFont().)

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

	// Retained for config compatibility (Settings.scroll[16] in lua). Font
	// rotation is now driven by the scroll wrap-around, not this timer, so
	// this value no longer affects swap timing.
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
		// The string actually drawn differs per font (each transliterates the
		// characters it lacks), so measure the old font against the text it is
		// currently showing and the new font against the text it will show.
		const std::string oldText = scrollText;                       // active display
		const std::string newText = buildDisplay(newF, rawScrollText);
		if(oldText.empty() || scr.width() <= 0)
			return;

		const float gscale = target.height() / 576.0f;
		const float dynScale = gscale * scrollsize;
		const float centre = scr.width() / 2.0f;      // texture-space screen centre
		const float offOld = centre - xpos;           // text offset under centre (old px)

		const float lenOld = (float)oldF.get_width(oldText, dynScale);
		const float lenNew = (float)newF.get_width(newText, dynScale);
		// Centre sits on the blank gap before/after the text: nothing to anchor,
		// and leaving xpos alone keeps that gap continuous. (Avoids div-by-zero.)
		if(offOld <= 0.0f || offOld >= lenOld || lenOld <= 0.0f)
			return;

		// UTF-8 character boundaries (0 .. length()) for a string.
		auto boundaries = [](const std::string &s) {
			std::vector<int> bnd;
			bnd.push_back(0);
			for(int p = 0; p < (int)s.size();) {
				unsigned char c = (unsigned char)s[p];
				int adv = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
				p += adv;
				if(p > (int)s.size()) p = (int)s.size();
				bnd.push_back(p);
			}
			return bnd;
		};
		const std::vector<int> bndOld = boundaries(oldText);
		const std::vector<int> bndNew = boundaries(newText);
		const int nOld = (int)bndOld.size() - 1;
		const int nNew = (int)bndNew.size() - 1;

		// Per-character anchoring maps a character index between the two strings,
		// which is only well-defined when they contain the same characters (the
		// usual case: identical transliteration, or none). If the two fonts fold a
		// different set of characters the strings diverge, so fall back to keeping
		// the same fractional position -- close enough for a manual swap.
		if(nOld != nNew) {
			xpos = centre - (offOld / lenOld) * lenNew;
			return;
		}

		// Width (old font) of the first k characters. Monotonic in k.
		auto prefixOld = [&](int k) -> float {
			if(k <= 0) return 0.0f;
			if(k >= nOld) return lenOld;
			return (float)oldF.get_width(oldText.substr(0, bndOld[k]), dynScale);
		};

		// Largest k with prefixOld(k) <= offOld  ->  the glyph under centre is k.
		int lo = 0, hi = nOld;
		while(lo < hi) {
			int mid = (lo + hi + 1) / 2;
			if(prefixOld(mid) <= offOld) lo = mid; else hi = mid - 1;
		}
		const int k = lo;                          // 0 <= k <= nOld-1

		const float wOldK  = prefixOld(k);
		const float wOldK1 = prefixOld(k + 1);
		const float frac = (wOldK1 > wOldK) ? (offOld - wOldK) / (wOldK1 - wOldK) : 0.0f;

		// Same character boundary, measured in the NEW font's own string.
		auto prefixNew = [&](int j) -> float {
			if(j <= 0) return 0.0f;
			if(j >= nNew) return lenNew;
			return (float)newF.get_width(newText.substr(0, bndNew[j]), dynScale);
		};
		const float offNew = prefixNew(k) + frac * (prefixNew(k + 1) - prefixNew(k));

		xpos = centre - offNew;
	}

	grappix::RenderTarget& target;
	grappix::Font font;
	std::vector<std::string> fontPaths;    // .otf paths, parallel to `fonts`
	std::vector<grappix::Font> fonts;      // rotation pool, built lazily on demand
	std::vector<char> fontBuilt;           // 1 once fonts[i] has been built
	std::vector<std::string> fontNames;    // basenames, parallel to `fonts`
	int fontIndex = 0;                     // active index into `fonts`
	float font_swap_timer = 0.0f;          // elapsed seconds since last swap
	grappix::Program program;
	grappix::Program fprogram;
	float xpos = -9999;
	grappix::Texture scr;
	// rawScrollText is the text as handed to set("scrolltext",...) -- the source
	// of truth. scrollText is the version actually drawn: identical to raw for the
	// active font's supported characters, but with any character the active font
	// lacks a glyph for folded to an ASCII look-alike (see rebuildDisplay). It is
	// therefore recomputed every time the active font changes.
	std::string rawScrollText;
	std::string scrollText;
	int scrollLen{};

	// ASCII fallback for a character a font can't draw. Empty string => no known
	// fallback (the character is then left as-is and renders as a blank space via
	// the font's space-fallback). Accented Latin folds to its base letter; the
	// typographic quotes/dashes fold to their ASCII equivalents.
	static std::string transliterateChar(wchar_t c) {
		switch(c) {
			case 0x00B5: return "u";      // µ  MICRO SIGN
			case 0x00C5: return "A";      // Å
			case 0x00C4: return "A";      // Ä
			case 0x00D6: return "O";      // Ö
			case 0x00E5: return "a";      // å
			case 0x00E4: return "a";      // ä
			case 0x00F6: return "o";      // ö
			case 0x00E7: return "c";      // ç
			case 0x00C7: return "C";      // Ç
			case 0x00E9: return "e";      // é
			case 0x00C9: return "E";      // É
			case 0x00ED: return "i";      // í
			case 0x00CD: return "I";      // Í
			case 0x00F8: return "o";      // ø
			case 0x00D8: return "O";      // Ø
			case 0x0107: return "c";      // ć
			case 0x0106: return "C";      // Ć
			case 0x015B: return "s";      // ś
			case 0x015A: return "S";      // Ś
			case 0x0394: return "Delta";  // Δ  GREEK CAPITAL LETTER DELTA
			case 0x2013: return "-";      // –  EN DASH
			case 0x2014: return "-";      // —  EM DASH
			case 0x2018: return "'";      // ‘
			case 0x2019: return "'";      // ’
			case 0x201C: return "\"";     // “
			case 0x201D: return "\"";     // ”
			case 0x2032: return "'";      // ′  PRIME
			default: return std::string();
		}
	}

	// Produce the drawable string for `raw` under font `f`: every character the
	// font can render is kept verbatim (accents intact); every character it can't
	// is replaced by its ASCII fallback so it shows up instead of a blank gap.
	static std::string buildDisplay(const grappix::Font &f, const std::string &raw) {
		std::string out;
		out.reserve(raw.size());
		for(size_t p = 0; p < raw.size();) {
			unsigned char b = (unsigned char)raw[p];
			int adv = (b >= 0xF0) ? 4 : (b >= 0xE0) ? 3 : (b >= 0xC0) ? 2 : 1;
			if(p + adv > raw.size()) adv = (int)(raw.size() - p);
			if(adv == 1) {
				// Plain ASCII: always in the baked set and in every font.
				out += (char)b;
				p += 1;
				continue;
			}
			// Decode the UTF-8 codepoint.
			wchar_t cp = 0;
			if(adv == 2)
				cp = ((b & 0x1F) << 6) | (raw[p+1] & 0x3F);
			else if(adv == 3)
				cp = ((b & 0x0F) << 12) | ((raw[p+1] & 0x3F) << 6) | (raw[p+2] & 0x3F);
			else
				cp = ((b & 0x07) << 18) | ((raw[p+1] & 0x3F) << 12)
				   | ((raw[p+2] & 0x3F) << 6) | (raw[p+3] & 0x3F);
			if(f.covers(cp)) {
				out += raw.substr(p, adv);           // font has it: keep the accent
			} else {
				std::string sub = transliterateChar(cp);
				out += sub.empty() ? raw.substr(p, adv) : sub;
			}
			p += adv;
		}
		return out;
	}

	// Recompute scrollText from rawScrollText for the currently active font.
	void rebuildDisplay() {
		scrollText = buildDisplay(font, rawScrollText);
	}


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

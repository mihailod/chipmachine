#ifndef SCOLLER_H
#define SCOLLER_H

#include "Effect.h"

#include <coreutils/file.h>
#include <grappix/grappix.h>
#include <coreutils/environment.h>

namespace demofx {

class Scroller : public Effect {
public:
	explicit Scroller(grappix::RenderTarget &target) : target(target), scr(grappix::screen.width()+10, 300) {
		program = grappix::get_program(grappix::TEXTURED_PROGRAM).clone();

		grappix::Resources::getInstance().load<std::string>((Environment::getCacheDir() / "sine_shader.glsl").string(),
			[=](const std::shared_ptr<std::string>& source) {
				try {
					program.setFragmentSource(*source);
				} catch(grappix::shader_exception &e) {
					LOGD("ERROR");
				}
			}, sineShaderF);


		fprogram = grappix::get_program(grappix::FONT_PROGRAM_DF).clone();
		fprogram.setFragmentSource(fontShaderF);
		font.set_program(fprogram);
	}

	void resize(int w, int h) override {
		// Texture is (re)sized in render() to track the window/text scale; just
		// scale the height with the target here so the first frame isn't clipped.
		int texH = (int)(300 * (target.height() / 576.0f));
		if(texH < 8) texH = 300;
		if(w > 8)
			scr = grappix::Texture(w+10, texH);
	}
	void set(const std::string &what, const std::string &val, float seconds = 0.0) override {
		if(what == "font") {
			font = grappix::Font(val, 120, 1024 | grappix::Font::DISTANCE_MAP);
			font.set_program(fprogram);
		} else {
			scrollText = val;
			LOGD("SCROLL: %s", scrollText);
			xpos = target.width() + 100;
		}
	}

	void render(uint32_t delta) override {
		if(alpha <= 0.01)
			return;

		// Calculate dynamic scale factor based on target resolution
		float gscale = target.height() / 576.0f;
		float dynScale = gscale * 3.0f;

		// The render texture must grow with the text scale, otherwise large
		// windows clip the glyphs at the old fixed 300px height (the bottoms
		// of the letters disappear). Keep it 1:1 with on-screen pixels.
		int texW = target.width() + 10;
		int texH = (int)(300 * gscale);
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
		program.use();
		static float uvs[] = { 0,0,1,0,0,1,1,1 };
		target.draw(scr, 0.0F, scrolly - texH / 2.0f, target.width(), texH, uvs, program);
	}

	float alpha = 1.0;

	// Pixels advanced per 60 FPS-frame (scaled by real frame time in render()).
	// NOTE: this default is overridden at startup by SCROLL_SPEED in
	// lua/screen.lua (via Settings.scroll) -- tune the speed THERE, not here.
	int scrollspeed = 8;
	int scrolly = 0;
	float scrollsize = 4.0;

private:
	grappix::RenderTarget& target;
	grappix::Font font;
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

		const vec4 color0 = vec4(1.0, 0.9, 0.2, 1.0); // Yellow/Orange
		const vec4 color1 = vec4(0.5, 0.2, 1.0, 1.0); // Purple/Blue

		varying vec2 UV;

		void main() {
			// Center the gradient on the text (UV.y around 0.5)
			float grad = smoothstep(0.3, 0.7, UV.y);
			vec4 rgb = mix(color0, color1, grad);
			vec4 color = texture2D(sTexture, UV);
			gl_FragColor = rgb * color;
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

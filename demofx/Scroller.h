#ifndef SCOLLER_H
#define SCOLLER_H

#include "Effect.h"

#include <coreutils/file.h>
#include <grappix/grappix.h>
#include <coreutils/environment.h>

namespace demofx {

class Scroller : public Effect {
public:
	explicit Scroller(grappix::RenderTarget &target) : target(target), scr(grappix::screen.width()+10, 1080) {
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
		if(w > 8 && h > 8)
			scr = grappix::Texture(w+10, 1080);
	}
	void set(const std::string &what, const std::string &val, float seconds = 0.0) override {
		if(what == "font") {
			// Increased atlas size to 1024 to fit wide characters like 'W' and 'Y'
			font = grappix::Font(val, 120, 1024 | grappix::Font::DISTANCE_MAP);
			font.set_program(fprogram);
		} else {
			scrollText = val;
			LOGD("SCROLL: %s", scrollText);
			xpos = target.width() + 100;
			scrollLen = font.get_width(val, 3.0);
		}
	}

	void render(uint32_t delta) override {
		if(alpha <= 0.01)
			return;
		if(xpos < -scrollLen)
			xpos = target.width() + 100;

		scr.clear(0x00000000);
		// Baseline 180 and 3.0 scale for large, fully visible text
		scr.text(font, scrollText, xpos-=scrollspeed, 180, 0xffffffff, 3.0); 
		program.use();
		static float uvs[] = { 0,0,1,0,0,1,1,1 };
		target.draw(scr, 0.0F, scrolly, target.width(), target.height(), uvs, program);
	}

	float alpha = 1.0;

	int scrollspeed = 16;
	int scrolly = 0;
	float scrollsize = 4.0;

private:
	grappix::RenderTarget& target;
	grappix::Font font;
	grappix::Program program;
	grappix::Program fprogram;
	int xpos = -9999;
	grappix::Texture scr;
	std::string scrollText;
	int scrollLen{};


	const std::string sineShaderF = R"(
		#ifdef GL_ES
			precision highp float;
		#endif
		uniform sampler2D sTexture;

		const vec4 color0 = vec4(1.0, 0.9, 0.2, 1.0);
		const vec4 color1 = vec4(0.5, 0.2, 1.0, 1.0);

		varying vec2 UV;

		void main() {
			float grad = smoothstep(0.1, 0.3, UV.y);
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

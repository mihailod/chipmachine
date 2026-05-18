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
		if(w > 8 && h > 8)
			scr = grappix::Texture(w+10, 300);
	}
	void set(const std::string &what, const std::string &val, float seconds = 0.0) override {
		if(what == "font") {
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

		// Calculate dynamic scale factor based on target resolution
		float dynScale = (target.height() / 576.0f) * 3.0f;

		scr.clear(0x00000000);
		// Render text using dynamic scale factor
		scr.text(font, scrollText, xpos-=scrollspeed, 150, 0xffffffff, dynScale); 
		program.use();
		static float uvs[] = { 0,0,1,0,0,1,1,1 };
		target.draw(scr, 0.0F, scrolly - 150, target.width(), 300, uvs, program);
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

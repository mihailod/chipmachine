#ifdef GL_ES
	precision highp float;
#endif
	uniform vec4 color;
	uniform float vScale;
	uniform sampler2D sTexture;
	//uniform float smoothing;
	varying vec2 UV;

	vec3 glyph_color    = vec3(0.0,1.0,0.0);
	const float glyph_center   = 0.50;
	vec3 outline_color  = vec3(0.0,0.0,1.0);
	const float outline_center = 0.58;
	vec3 glow_color     = vec3(1.0, 1.0, 0.0);
	const float glow_center    = 1.0;

	void main() {
		float dist = texture2D(sTexture, UV).a;
		float smoothing = 0.25 / (vScale * 1.5);
		float alpha = smoothstep(glyph_center-smoothing, glyph_center+smoothing, dist);
		gl_FragColor = vec4(color.rgb, color.a * alpha);
	}

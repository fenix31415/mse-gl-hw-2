#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 tex;
layout(location=3) in vec3 tangent;
layout(location=4) in vec3 bitangent;

uniform mat4 mvp;
uniform mat4 model;
uniform float timeValue;
uniform float morphSpeed;

//out vec3 vert_col;
out vec3 vert_pos;
out vec2 vert_tex;
out vec3 vert_norm;
out mat3 TBN;

vec3 morph(vec3 pos) {
	vec3 newpos = pos;
	newpos.x += sin(morphSpeed * (timeValue / 500.0 + newpos.x * 5)) * 0.2;
	return newpos;
}

void main() {
	vec3 newpos = morph(pos);
	
	vec3 posPlusTangent = morph(pos + tangent * 0.01);
  vec3 posPlusBitangent = morph(pos + bitangent * 0.01);
  vec3 posPlusnormal = morph(pos + normal * 0.01);

	vec3 newtangent = normalize(posPlusTangent - newpos);
  vec3 newbitangent = normalize(posPlusBitangent - newpos);
	vec3 newnormal = normalize(posPlusnormal - newpos);

	vert_pos = vec3(model * vec4(newpos, 1.0));
	vert_tex = tex;
	vert_norm = normalize(newnormal);
	gl_Position = mvp * vec4(newpos, 1.0);
	
	vec3 T = normalize(vec3(model * vec4(newtangent, 0.0)));
  vec3 B = normalize(vec3(model * vec4(newbitangent, 0.0)));
  vec3 N = normalize(vec3(model * vec4(newnormal, 0.0)));
  TBN = mat3(T, B, N);
}

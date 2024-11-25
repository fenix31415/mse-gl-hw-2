#version 330 core

uniform sampler2D tex_2d;
uniform sampler2D normal_tex;
uniform vec3 cameraPos;
uniform float ambientStrength;
uniform float diffuseReflection;
uniform float Light1Param;
uniform float Light2Param;
uniform float shininess;
uniform float specularStrength;
uniform mat4 model;
uniform mat4 view;
uniform float timeValue;

in vec3 vert_pos;
in vec2 vert_tex;
in vec3 vert_norm;
in mat3 TBN;

out vec4 out_col;

float get_mult1(vec3 lightPos1) {
	vec3 dirFrag = normalize(vert_pos - lightPos1);
	vec3 dirLight = normalize(-lightPos1);
	float cosa = max(0.0, dot(dirFrag, dirLight));
	return smoothstep(Light1Param, 1, cosa);
}

float get_mult2(float dist) {
	return 1 - smoothstep(Light2Param, Light2Param * 2, dist);
}

void main() {
	vec4 texel = texture(tex_2d, vert_tex);
	vec3 normalMap = texture(normal_tex, vert_tex).rgb;
	vec3 normal = normalize(TBN * (normalMap * 2.0 - 1.0));
	vec3 camerapos_fixed = (vec4(0.0, 0.0, 0.0, 1.0) * inverse(transpose(view))).xyz;
	vec3 viewDir = normalize(camerapos_fixed - vert_pos);
	
	vec3 ambient = ambientStrength * vec3(0.1, 0.1, 0.1);
	
	vec3 lightColor1 = vec3(1.0, 0.0, 0.0);
	vec3 lightColor2 = vec3(0.7, 1.0, 0.7);
	vec3 lightPos1 = vec3(0.2, 0.2, 0.2);
	vec3 lightPos2 = vec3(0.2, 0.5, -0.2);

	vec3 lightDir1 = normalize(lightPos1 - vert_pos);
	vec3 lightDir2 = normalize(lightPos2 - vert_pos);

  float diff1 = max(0.0, dot(normal, lightDir1));
  float diff2 = max(0.0, dot(normal, lightDir2));
  vec3 diffuse1 = (diff1 * get_mult1(lightPos1) * diffuseReflection) * lightColor1;
	float dist2 = distance(lightPos2, vert_pos);
  vec3 diffuse2 = (diff2 * get_mult2(dist2) * diffuseReflection) * lightColor2;
	
  vec3 reflectDir1 = reflect(-lightDir1, normal);
  vec3 reflectDir2 = reflect(-lightDir2, normal);
  float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), shininess);
  float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), shininess);

  vec3 specular1 = specularStrength * spec1 * lightColor1;
  vec3 specular2 = specularStrength * spec2 * lightColor2;
	if (diff1 == 0.0)
		specular1 = vec3(0.0, 0.0, 0.0);
	if (diff2 == 0.0)
		specular2 = vec3(0.0, 0.0, 0.0);
	
	vec3 finalColor = (ambient + diffuse1 + specular1 + diffuse2 + specular2) * texel.rgb;
	out_col = vec4(finalColor, texel.a);

	//float greyscale_factor = dot(texel.rgb, vec3(0.21, 0.71, 0.07));
	//out_col = vec4(mix(vec3(greyscale_factor), vert_col.rgb, 0.7), 1.0f);
	//out_col = vec4(viewDir, 1.0f);
	//out_col = vec4(vert_pos, 1.0f);
	//out_col = vec4(cameraPos, 1.0f);
	//out_col = vec4(vec3(distance(lightPos, camerapos_fixed)), 1.0f);
	//out_col = vec4(vec3(distance(camerapos_fixed, vert_pos)), 1.0f);
	//out_col = vec4(lightDir, 1.0);
	//out_col = vec4(vec3(diff), 1.0);
}
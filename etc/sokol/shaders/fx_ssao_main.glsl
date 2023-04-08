
#define NUM_SAMPLES 8

float radius    = 0.6;
float bias      = 0.005;
float magnitude = 1.1;
float contrast  = 1.1;

// Get normals
float d = rgba_to_depth(texture(t_depth, uv));
vec3 position = world_pos_from_depth(uv, d, u_inv_mat_vp);

vec4 view_pos = vec4(position, 1.0) * u_inv_mat_p;
vec3 normal = normalize( cross( dFdx( view_pos.xyz ), dFdy( view_pos.xyz ) ) );

// int  noiseS = int(sqrt(NUM_NOISE));
// int  noiseX = int(gl_FragCoord.x - 0.5) % noiseS;
// int  noiseY = int(gl_FragCoord.y - 0.5) % noiseS;
vec3 random = rnd_vec3(uv);

vec3 tangent  = normalize(random - normal * dot(random, normal));
vec3 binormal = cross(normal, tangent);
mat3 tbn      = mat3(tangent, binormal, normal);

float occlusion = NUM_SAMPLES;

for (int i = 0; i < NUM_SAMPLES; ++i) {
  vec3 samplePosition = tbn * rnd_vec3(uv + i);
  samplePosition = view_pos.xyz + samplePosition * radius;

  vec4 offsetUV = vec4(samplePosition, 1.0);
  offsetUV      = u_mat_p * offsetUV;
  offsetUV.xyz /= offsetUV.w;
  offsetUV.xy   = offsetUV.xy * 0.5 + 0.5;

  // Config.prc
  // gl-coordinate-system  default
  // textures-auto-power-2 1
  // textures-power-2      down

  float offsetD = rgba_to_depth(texture(t_depth, offsetUV.xy));
  vec4 offsetPosition = vec4(world_pos_from_depth(uv, offsetD, u_inv_mat_vp), 1.0);
  offsetPosition *= u_mat_v;
  offsetPosition /= offsetPosition.w;

  float occluded = 0;
  if (samplePosition.y + bias <= offsetPosition.y) { occluded = 0; }
  else { occluded = 1; }

  float intensity = smoothstep ( 0, 1, radius / abs(position.y - offsetPosition.y) );
  occluded  *= intensity;
  occlusion -= occluded;
}

occlusion /= NUM_SAMPLES;
occlusion  = pow(occlusion, magnitude);
occlusion  = contrast * (occlusion - 0.5) + 0.5;

frag_color = vec4(vec3(occlusion), 1.0);

frag_color = vec4(normal, 1.0);
// frag_color = vec4(position, 1.0);
// frag_color = view_pos;

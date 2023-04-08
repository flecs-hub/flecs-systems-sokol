float ambientOcclusion = rgba_to_float(texture(t_occlusion, uv));
// frag_color = (1.0 - ambientOcclusion) * texture(t_scene, uv);
// frag_color = vec4(ambientOcclusion, ambientOcclusion, ambientOcclusion, 1.0);
// frag_color = texture(t_occlusion, uv);
frag_color = texture(t_scene, uv);

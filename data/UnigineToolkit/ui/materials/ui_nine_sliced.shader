// Copyright (C), UNIGINE. All rights reserved.

#define VERTEX_ATTRIBUTE_POST
#include <core/materials/shaders/render/common.h>

STRUCT(FRAGMENT_IN)
	INIT_POSITION
	INIT_DATA(float2, 0, DATA_UV0)
	INIT_DATA(float4, 1, DATA_VERTEX_COLOR)
END

#ifdef VERTEX

	MAIN_BEGIN(FRAGMENT_IN, VERTEX_IN)

		float4 row_0 = s_transform[0];
		float4 row_1 = s_transform[1];
		float4 row_2 = s_transform[2];

		float4 vertex = float4(
			dot(ATTRIBUTE_POSITION, row_0),
			dot(ATTRIBUTE_POSITION, row_1),
			dot(ATTRIBUTE_POSITION, row_2),
			1.0f);

		OUT_POSITION = mul4(s_projection, vertex);
		DATA_UV0 = ATTRIBUTE_UV.xy;
		DATA_VERTEX_COLOR = ATTRIBUTE_COLOR;

	MAIN_END

#elif FRAGMENT

	INIT_TEXTURE(0, TEX_COLOR)

	STRUCT_FRAG_BEGIN
		INIT_COLOR(float4)
	STRUCT_FRAG_END

	MAIN_FRAG_BEGIN(FRAGMENT_IN)

		float2 uv = DATA_UV0;

		float s0, s1, s2;
		s0 =            step(uv.x, var_stops.x);
		s1 = (1 - s0) * step(uv.x, var_stops.z);
		s2 = (1 - s0) * (1 - s1);
		uv.x = s0 *   uv.x                * var_scale.x
			   + s1 * ((uv.x - var_stops.x) * var_scale.z + var_offsets.x)
			   + s2 * ((uv.x - var_stops.z) * var_scale.x + var_offsets.z);

		s0 =            step(uv.y, var_stops.y);
		s1 = (1 - s0) * step(uv.y, var_stops.w);
		s2 = (1 - s0) * (1 - s1);
		uv.y = s0 *   uv.y                * var_scale.y
			   + s1 * ((uv.y - var_stops.y) * var_scale.w + var_offsets.y)
			   + s2 * ((uv.y - var_stops.w) * var_scale.y + var_offsets.w);

		OUT_COLOR = TEXTURE_BIAS_ZERO(tex_color, uv) * DATA_VERTEX_COLOR;

	MAIN_FRAG_END

#endif
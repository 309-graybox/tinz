/* Copyright (C) 2005-2026, UNIGINE. All rights reserved.
*
* This file is a part of the UNIGINE 2 SDK.
*
* Your use and / or redistribution of this software in source and / or
* binary form, with or without modification, is subject to: (i) your
* ongoing acceptance of and compliance with the terms and conditions of
* the UNIGINE License Agreement; and (ii) your inclusion of this notice
* in any version of this software that you use or redistribute.
* A copy of the UNIGINE License Agreement is available by contacting
* UNIGINE. at http://unigine.com/
*/

#ifndef __SHADING_H__
#define __SHADING_H__

#ifdef !NO_SHADING

/*
	GBuffer :
		roughness

	Data :
		normal_w
		reflection_w
*/

void loadEnvironmentAmbient(inout float3 ambient, Data data, TEXTURE_IN_CUBE(TEX_REFLECTION))
{
	ambient = TEXTURE_BIAS(TEX_REFLECTION, data.normal_w, s_environment_mipmaps).rgb;
	ambient *= s_environment_ambient_intensity;
}
void loadEnvironmentReflection(inout float3 reflection, GBuffer gbuffer, Data data, TEXTURE_IN_CUBE(TEX_REFLECTION))
{
	reflection = TEXTURE_BIAS(TEX_REFLECTION, data.sky_reflect, gbuffer.roughness * s_environment_mipmaps).rgb;
	reflection *= s_environment_reflection_intensity;
}

float3 getEnvironmentReflectVector(float3 normal, float3 reflection, float roughness)
{
	return lerp(reflection, normal, pow4(roughness));
}

void environmentShading(inout float3 ambient, inout float3 reflection, GBuffer gbuffer, Data data)
{
	branch if (s_roughness_offset != 0.0f)
	{
		ambient *= gbuffer.albedo;
		reflection = 0.0f;
		return;
	}

	float dotVN = 1.0f - saturate(data.dotNV);
	float dotVN_5 = pow5(dotVN);

	float f0_dielectric = gbuffer.f0 * (1.0f - 0.5f * pow8(gbuffer.roughness));
	float fresnel = dotVN_5 * lerpOne(0.05f, pow2(data.gloss)) * saturate(gbuffer.f0 * 25.0f + data.gloss);

	branch if (gbuffer.microfiber > 0.001f)
	{
		float fresnel_microfiber = pow(dotVN, 8.0f - 7.0f * sqrt(sqrt(gbuffer.roughness)));

		float3 reflection_microfiber = float3_one;
		reflection_microfiber *= 0.1f + gbuffer.roughness * fresnel_microfiber;
		reflection_microfiber *= gbuffer.f0 * 5.0f;
		reflection_microfiber *= gbuffer.albedo * gbuffer.metalness - gbuffer.metalness + 1.0f;

		float3 f0_metalness = lerp(toFloat3(f0_dielectric), gbuffer.albedo, gbuffer.metalness);
		float3 reflection_default = lerp(f0_metalness, float3_one, fresnel);

		reflection *= lerp(reflection_default, reflection_microfiber, gbuffer.microfiber);

		float ambient_microfiber = 0.7f + 0.3f * fresnel_microfiber + gbuffer.roughness * fresnel_microfiber;
		float ambient_default = data.dielectric * (1.0f - lerpOne(f0_dielectric, fresnel));

		ambient *= lerp(ambient_default, ambient_microfiber, gbuffer.microfiber);
		ambient *= gbuffer.albedo;
	} else
	{
		float3 f0_metalness = lerp(toFloat3(f0_dielectric), gbuffer.albedo, gbuffer.metalness);
		reflection *= lerp(f0_metalness, float3_one, fresnel);

		ambient *= data.dielectric * (1.0f - lerpOne(f0_dielectric, fresnel));
		ambient *= gbuffer.albedo;
	}
}

float getFresnelSpecular(float dotVH, float f0, float roughness)
{
	float fresnel = pow5(1.0f - saturate(dotVH));
	fresnel *= saturate(f0 * 25.0f + 1.0f - roughness);
	return fresnel;
}

float getFresnelDiffuse(float dotNV, float dotNL)
{
	float FdV = pow5(1.0f - saturate(dotNV));
	float FdL = pow5(1.0f - saturate(dotNL));
	return (1.0f - FdV) * (1.0f - FdL);
}

#define GGX_MIN_ROUGHNESS 0.02f
#define DENOM_EPSILON 0.00000001f

void getGGXRoughness(float roughness, float min_roughness, out float roughness_2, out float roughness_4)
{
	roughness_2 = pow2(roughness * (1.0f - min_roughness) + min_roughness);
	roughness_4 = pow2(roughness_2);
}

// Unified microfiber sheen calculation for direct lighting
// Returns sheen intensity based on angle to half-vector and roughness
float getMicrofiberSheen(float dotNH, float roughness)
{
	float exponent = 6.0f - 5.5f * pow(roughness, 0.35f);
	return pow(1.0f - dotNH, exponent);
}

float3 getMicrofiberSheenSpecular(float3 specular, float3 specular_color, float roughness, float dotNL, float microfiber_sheen, float microfiber)
{
	float sheen_brightness = lerp(80.0f, 4.0f, sqrt(sqrt(roughness)));
	float3 specular_sheen = microfiber_sheen * specular_color * sheen_brightness * saturate(dotNL);
	return lerp(specular, specular_sheen, microfiber * 0.65f);
}

float getRawGGX(float roughness_2, float roughness_4, float dotNV, float dotNL, float dotNH)
{
	float k = roughness_2 * 0.5f;

	float denom = pow2(dotNH) * (roughness_4 - 1.0f) + 1.0f + DENOM_EPSILON;

	float ggx = dotNL * roughness_4;
	ggx /= (dotNL * (1.0f - k) + k) * (dotNV * (1.0f - k) + k) * PI * pow2(denom);
	ggx *= 1.0f - pow10(1.0f - saturate(dotNL));

	return clamp(ggx, 0.0f, HALF_MAX);
}

float getGGXLow(float roughness, float dotNL, float dotNH)
{
	float roughness_2, roughness_4;
	getGGXRoughness(roughness, GGX_MIN_ROUGHNESS, roughness_2, roughness_4);

	float clamped_dotNL = saturate(dotNL);

	float denom = pow2(dotNH) * (roughness_4 - 1.0f) + 1.0f + DENOM_EPSILON;

	float ggx = clamped_dotNL * roughness_4 * (1.0f + roughness_2);
	ggx /= PI * pow2(denom);

	return clamp(ggx, 0.0f, HALF_MAX);
}

float getGGXMedium(float roughness, float dotNV, float dotNL, float dotNH)
{
	float roughness_2, roughness_4;
	getGGXRoughness(roughness, GGX_MIN_ROUGHNESS, roughness_2, roughness_4);

	float clamped_dotNL = saturate(dotNL);
	float k = roughness_2 * 0.5f;

	float denom = pow2(dotNH) * (roughness_4 - 1.0f) + 1.0f + DENOM_EPSILON;

	float ggx = clamped_dotNL * roughness_4;
	ggx /= (clamped_dotNL * (1.0f - k) + k) * (dotNV * (1.0f - k) + k) * PI * pow2(denom);

	return clamp(ggx, 0.0f, HALF_MAX);
}

float getGGXHigh(float roughness, float dotNV, float dotNL, float dotNH, float translucent_scale)
{
	float roughness_2, roughness_4;
	getGGXRoughness(roughness, GGX_MIN_ROUGHNESS, roughness_2, roughness_4);

	float clamped_dotNL = saturate(dotNL);
	float adjusted_dotNL = translucent_scale > 0.001f ? pow(clamped_dotNL, 1.0f + translucent_scale) : clamped_dotNL;

	return getRawGGX(roughness_2, roughness_4, dotNV, adjusted_dotNL, dotNH);
}

float getAreaLightGGX(float roughness, float dotNV, float dotNL, float dotLR, float size)
{
	const float area_light_min_roughness = 0.05f;

	float roughness_2, roughness_4;
	getGGXRoughness(roughness, area_light_min_roughness, roughness_2, roughness_4);

	float specular = getRawGGX(roughness_2, roughness_4, dotNV, dotNL, dotLR);
	specular /= 1.0f + saturate(size) / (PI * roughness_4);
	return specular;
}

float getAreaLightGGX(float roughness, float dotNV, float dotNL, float dotLR, float size, float translucent_scale)
{
	float adjusted_dotNL = translucent_scale > 0.001f ? pow(saturate(dotNL), 1.0f + translucent_scale) : saturate(dotNL);
	return getAreaLightGGX(roughness, dotNV, adjusted_dotNL, dotLR, size);
}

// diffuse lighting
float getBurley(float roughness, float dotNV, float dotLH, float dotNL)
{
	roughness *= roughness;

	float energy_factor = 1.0f - roughness * 0.337748344f;
	float f90 = 2.0f * roughness * (pow2(dotLH) + 0.25f) - 1.0f;

	float light_scatter = f90 * pow5(saturate(1.0f - dotNL)) + 1.0f;
	float view_scatter = f90 * pow5(saturate(1.0f - dotNV)) + 1.0f;

	return dotNL * energy_factor * light_scatter * view_scatter;
}

float3 getOurDiffuseBRDF(float3 albedo, float roughness, float dotNV, float dotNL, float dotVL, float dotNH, float microfiber_sheen, float microfiber)
{
	float t = 1.0f - dotNH;
	float retroreflection = t * sqrt(t) * 0.8f;
	float opposition_surge = pow8(saturate(-dotVL)) * 0.2f;

	float3 color_with_ms = toFloat3(dotNL) * lerp(albedo, float3_one, lerp(0.6f, 0.5f, microfiber));
	float3 color_without_ms = toFloat3(powMirror(dotNL, 1.5f)) / lerp(albedo + EPSILON, float3_one, 0.5f);

	float3 rough_color = lerp(color_with_ms, color_without_ms, retroreflection + opposition_surge);

	float glossy_color = dotNL * getFresnelDiffuse(dotNV, dotNL);

	float3 diffuse = max(lerp(toFloat3(glossy_color), rough_color, roughness), 0.0f);

	// Microfiber sheen replaces diffuse with sharper color_without_ms based look
	float3 microfiber_diffuse = max(lerp(color_with_ms, color_without_ms, microfiber_sheen), 0.0f);
	diffuse = lerp(diffuse, microfiber_diffuse, microfiber);

	return diffuse;
}

// translucent uses negative dotNL
float getTranslucent(float translucent_scale, float dotNL, float dotVL)
{
	float soft_dotNL = 0.5f * (dotNL + 1.0f);
	float dotVL_2 = pow20(saturate(dotVL));
	float back_side = saturate(-dotNL);
	float back_side_thickness = pow4(saturate(rerange(saturate(-dotNL), 0.0f, translucent_scale * 10.0f, 1.0f, 0.0f)));

	float translucent = (saturate(soft_dotNL) + saturate(back_side) + dotVL_2 * 15.0f * translucent_scale) * back_side_thickness;
	return (translucent - saturate(dotNL)) * translucent_scale;
}

float getTranslucentSmoke(float translucent_scale, float dotVL, float dotNL, float dotNV)
{
	float soft_dotNL = (dotNL + 1.0f) * 0.5f;
	float back_side = pow3(dotVL + 1.0f) * 0.35f;
	float rim_light = saturate(1.0f - dotNV);
	float translucent = soft_dotNL + rim_light + back_side;
	return max(lerp(dotNL, translucent, translucent_scale), 0.0f);
}

/*
GBuffer :
	roughness
	translucent
	f0
*/
float3 getSpecularBRDF(GBuffer gbuffer, float3 specular_color, float dotNV, float dotLH, float dotNL, float dotNH, float microfiber_sheen)
{
	float fresnel = getFresnelSpecular(dotLH, gbuffer.f0, gbuffer.roughness);

	#ifdef RENDER_SHADING_QUALITY_LOW
		float ggx = getGGXLow(gbuffer.roughness, dotNL, dotNH);
		return lerpOne(specular_color, fresnel) * ggx;
	#elif RENDER_SHADING_QUALITY_MEDIUM
		float adjusted_roughness = lerpOne(gbuffer.roughness, gbuffer.microfiber);
		float ggx = getGGXMedium(adjusted_roughness, dotNV, dotNL, dotNH);
		float3 specular = lerpOne(specular_color, fresnel) * ggx;
		return getMicrofiberSheenSpecular(specular, specular_color, gbuffer.roughness, dotNL, microfiber_sheen, gbuffer.microfiber);
	#else
		float3 specular_multiscattering = specular_color * gbuffer.roughness + 1.0f;

		float translucent_scale = gbuffer.translucent;
		float adjusted_roughness = lerpOne(gbuffer.roughness, gbuffer.microfiber);

		float ggx = getGGXHigh(adjusted_roughness, dotNV, dotNL, dotNH, translucent_scale);
		float3 specular = lerpOne(specular_color * specular_multiscattering, fresnel) * ggx;
		return getMicrofiberSheenSpecular(specular, specular_color, gbuffer.roughness, dotNL, microfiber_sheen, gbuffer.microfiber);
	#endif
}

float3 getAreaLightSpecularBRDF(GBuffer gbuffer, float3 specular_color, float dotNV, float dotLH, float dotNL, float dotNH, float size)
{
	float specular = getAreaLightGGX(gbuffer.roughness, dotNV, dotNL, dotNH, size, gbuffer.translucent);
	float3 color = lerpOne(specular_color, pow5(1.0f - saturate(dotLH)));
	return clamp(color * specular, 0.0f, 1000.0f);
}

float3 getDiffuseBRDF(GBuffer gbuffer, float dotNV, float dotLH, float dotNL, float dotVL, float dotNH, float microfiber_sheen)
{
	#ifdef RENDER_SHADING_QUALITY_LOW
		float3 diffuse_brdf = toFloat3(saturate(dotNL));
	#elif RENDER_SHADING_QUALITY_MEDIUM
		float glossy_lambert = dotNL * getFresnelDiffuse(dotNV, dotNL);
		float3 diffuse_brdf = toFloat3(saturate(lerp(glossy_lambert, dotNL, gbuffer.roughness)));
	#else
		float3 diffuse_brdf = getOurDiffuseBRDF(gbuffer.albedo, gbuffer.roughness, dotNV, dotNL, dotVL, dotNH, microfiber_sheen, gbuffer.microfiber);
	#endif

	branch if (gbuffer.translucent > 0.001f)
	{
		#ifdef SMOKE_TRANSLUCENT
			diffuse_brdf += getTranslucentSmoke(gbuffer.translucent, dotVL, dotNL, dotNV);
		#else
			diffuse_brdf += srgbInv(s_render_translucent_color.rgb) * getTranslucent(gbuffer.translucent, dotNL, dotVL);
		#endif
	}

	return diffuse_brdf;
}

void getBRDF(inout float3 diffuse, inout float3 specular, GBuffer gbuffer, Data data, float3 light_dir)
{
	float3 H = normalize(light_dir + data.frag_to_cam_dir);
	float dotLH = dotFixed(light_dir, H);
	float dotVL = dot(-data.frag_to_cam_dir, light_dir);
	float dotNV = saturate(data.dotNV);


	float dotNL_raw = dot(data.normal, light_dir);
	float threshold = 0.35;
	float edgeWidth = 0.02;
	float dotNL = smoothstep(threshold - edgeWidth, threshold + edgeWidth, dotNL_raw);

	float dotNH = dotFixed(data.normal, H);
	float3 specular_color = lerp(toFloat3(gbuffer.f0), gbuffer.albedo, gbuffer.metalness);

	// Compute microfiber sheen once, pass to both BRDF functions
	float microfiber_sheen = 0.0f;
	branch if (gbuffer.microfiber > 0.001f)
		microfiber_sheen = getMicrofiberSheen(dotNH, gbuffer.roughness) * gbuffer.microfiber;

	specular *= getSpecularBRDF(gbuffer, specular_color, dotNV, dotLH, dotNL, dotNH, microfiber_sheen);
	diffuse *= getDiffuseBRDF(gbuffer, dotNV, dotLH, dotNL, dotVL, dotNH, microfiber_sheen);
}

float getLightProbeAttenuation(float distance, float attenuation)
{
	return srgbInv(1.0f - pow(1.0f - saturate(1.0f - distance), attenuation));
}

float3 sphereLightToLight(float3 L, float3 direction, float radius)
{
	float3 center_to_ray = direction * dot(L, direction) - L;
	return normalize(L + center_to_ray * saturate(radius / length(center_to_ray)));
}
float3 capsuleLightToLight(float3 L, float3 direction, float radius, float3 axis, float size, float dotLA)
{
	float dotDA = dot(direction, axis);
	float dotDL = dot(direction, L);
	L += axis * clamp((dotDL * dotDA - dotLA) / (1.0f - pow2(dotDA)), -size, size);
	return sphereLightToLight(L, direction, radius);
}

#endif

#endif /* __SHADING_H__ */

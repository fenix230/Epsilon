float3		g_albedo_clr;
bool		g_albedo_map_enabled;
Texture2D	g_albedo_tex;

float2		g_metalness_clr;
Texture2D	g_metalness_tex;

float2		g_glossiness_clr;
Texture2D	g_glossiness_tex;

float4x4	g_model_mat;
float4x4	g_view_mat;
float4x4	g_proj_mat;
float4x4	g_inv_proj_mat;

Texture2D	g_buffer_tex;
Texture2D	g_buffer_1_tex;

float3		g_light_dir_es;
float4		g_light_attrib;
float4		g_light_color;

Texture2D	g_pp_tex;

#define MAX_SHININESS 8192.0f


SamplerState point_sampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};


SamplerState linear_sampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};


SamplerState aniso_sampler
{
	Filter = ANISOTROPIC;
	AddressU = Wrap;
	AddressV = Wrap;
};


DepthStencilState depth_enalbed
{
	DepthEnable = true;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
};


DepthStencilState lighting_dss
{
	DepthEnable = FALSE;
	DepthWriteMask = ZERO;

	FRONTFACESTENCILFAIL = true;
	FRONTFACESTENCILFUNC = NOT_EQUAL;
	FRONTFACESTENCILPASS = KEEP;

	BACKFACESTENCILFAIL = true;
	BACKFACESTENCILFUNC = NOT_EQUAL;
	BACKFACESTENCILPASS = KEEP;
};


RasterizerState back_solid_rs
{
	FillMode = Solid;
	CullMode = BACK;
};


RasterizerState front_solid_rs
{
	FillMode = Solid;
	CullMode = FRONT;
};


RasterizerState double_solid_rs
{
	FillMode = Solid;
	CullMode = NONE;
};


BlendState lighting_bs
{
	BLENDENABLE[0] = true;
	SRCBLEND = ONE;
	DESTBLEND = ONE;
	BLENDOP = ADD;
	SRCBLENDALPHA = ONE;
	DESTBLENDALPHA = ONE;
	BLENDOPALPHA = ADD;
};


float4 StoreGBufferRT0(float3 normal, float glossiness)
{
	float p = sqrt(-normal.z * 8 + 8);
	float2 enc = normal.xy / p + 0.5f;
	float2 enc255 = enc * 255;
	float2 residual = floor(frac(enc255) * 16);
	return float4(float3(floor(enc255), residual.x * 16 + residual.y) / 255, glossiness);
}


float4 StoreGBufferRT1(float3 albedo, float metalness)
{
	return float4(albedo, metalness);
}


void StoreGBufferMRT(float3 normal, float glossiness, float3 albedo, float metalness,
	out float4 mrt_0, out float4 mrt_1)
{
	mrt_0 = StoreGBufferRT0(normal, glossiness);
	mrt_1 = StoreGBufferRT1(albedo, metalness);
}


float3 GetNormal(float4 mrt0)
{
	float nz = floor(mrt0.z * 255) / 16;
	mrt0.xy += float2(floor(nz) / 16, frac(nz)) / 255;
	float2 fenc = mrt0.xy * 4 - 2;
	float f = dot(fenc, fenc);
	float g = sqrt(1 - f / 4);
	float3 normal;
	normal.xy = fenc * g;
	normal.z = f / 2 - 1;
	return normal;
}


float GetGlossiness(float4 mrt0)
{
	return mrt0.w;
}


float3 DiffuseColor(float3 albedo, float metalness)
{
	return albedo * (1 - metalness);
}


float3 SpecularColor(float3 albedo, float metalness)
{
	return lerp(0.04, albedo, metalness);
}


float3 GetDiffuse(float4 mrt1)
{
	return DiffuseColor(mrt1.xyz, mrt1.w);
}


float3 GetSpecular(float4 mrt1)
{
	return SpecularColor(mrt1.xyz, mrt1.w);
}


float Shininess2Glossiness(float shininess)
{
	return log2(shininess) / log2(MAX_SHININESS);
}


float Glossiness2Shininess(float glossiness)
{
	return pow(MAX_SHININESS, glossiness);
}


float3 FresnelTerm(float3 light_vec, float3 halfway_vec, float3 c_spec)
{
	float e_n = saturate(dot(light_vec, halfway_vec));
	return c_spec > 0 ? c_spec + (1 - c_spec) * exp2(-(5.55473f * e_n + 6.98316f) * e_n) : 0;
}


float SpecularNormalizeFactor(float shininess)
{
	return (shininess + 2) / 8;
}


float3 DistributionTerm(float3 halfway_vec, float3 normal, float shininess)
{
	return exp((shininess + 0.775f) * (max(dot(halfway_vec, normal), 0.0f) - 1));
}


float3 SpecularTerm(float3 c_spec, float3 light_vec, float3 halfway_vec, float3 normal, float shininess)
{
	return SpecularNormalizeFactor(shininess) * DistributionTerm(halfway_vec, normal, shininess)
		* FresnelTerm(light_vec, halfway_vec, c_spec);
}


struct GBUFFER_VSO
{
	float4 pos : SV_Position;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD0;
};


GBUFFER_VSO GBufferVS(
	float4 pos : POSITION,
	float3 norm : NORMAL,
	float2 tc : TEXCOORD0
)
{
	GBUFFER_VSO opt;

	opt.pos = pos;
	opt.pos = mul(opt.pos, g_model_mat);
	opt.pos = mul(opt.pos, g_view_mat);
	opt.pos = mul(opt.pos, g_proj_mat);

	opt.norm = norm;
	opt.norm = mul(opt.norm, (float3x3)g_model_mat);
	opt.norm = mul(opt.norm, (float3x3)g_view_mat);

	opt.tc = tc;

	return opt;
}


struct GBUFFER_PSO
{
	float4 mrt_0 : SV_Target0;
	float4 mrt_1 : SV_Target1;
};


GBUFFER_PSO GBufferPS(GBUFFER_VSO ipt)
{
	float3 normal = ipt.norm;

	float3 albedo = g_albedo_clr.rgb;
	if (g_albedo_map_enabled)
	{
		albedo *= g_albedo_tex.Sample(aniso_sampler, ipt.tc).rgb;
	}

	float metalness = g_metalness_clr.r;
	if (g_metalness_clr.y > 0.5f)
	{
		metalness *= g_metalness_tex.Sample(aniso_sampler, ipt.tc).r;
	}

	float glossiness = g_glossiness_clr.r;
	if (g_glossiness_clr.y > 0.5f)
	{
		glossiness *= g_glossiness_tex.Sample(aniso_sampler, ipt.tc).r;
	}

	GBUFFER_PSO opt;
	StoreGBufferMRT(normal, glossiness, albedo, metalness, opt.mrt_0, opt.mrt_1);

	return opt;
}


struct LIGHTING_VSO
{
	float4 pos : SV_Position;
	float2 tc : TEXCOORD0;
	float3 view_dir : VIEWDIR;
};


LIGHTING_VSO LightingVS(float4 pos : POSITION)
{
	LIGHTING_VSO opt;

	opt.pos = pos;

	opt.tc = pos.xy / 2;
	opt.tc.y *= -1;
	opt.tc += 0.5f;

	opt.view_dir = mul(pos, g_inv_proj_mat).xyz;

	return opt;
}


float4 LightingTestPS(LIGHTING_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;
	float3 view_dir = ipt.view_dir;

	float4 mrt_1 = g_buffer_1_tex.Sample(point_sampler, tc);

	float3 c_diff = GetDiffuse(mrt_1);

	float4 shading = float4(c_diff, 1);

	return shading;
}


float4 LightingAmbientPS(LIGHTING_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;
	float3 view_dir = ipt.view_dir;

	float4 mrt_0 = g_buffer_tex.Sample(point_sampler, tc);
	float4 mrt_1 = g_buffer_1_tex.Sample(point_sampler, tc);
	view_dir = normalize(view_dir);
	float3 normal = GetNormal(mrt_0);
	float glossiness = GetGlossiness(mrt_0);
	float shininess = Glossiness2Shininess(glossiness);
	float3 c_diff = GetDiffuse(mrt_1);
	float3 c_spec = GetSpecular(mrt_1);

	float n_dot_l = 0.5f + 0.5f * dot(g_light_dir_es.xyz, normal);
	float4 shading = float4(max(c_diff * g_light_attrib.x * n_dot_l, 0) * g_light_color.rgb, 1);

	return shading;
}


float4 LightingSunPS(LIGHTING_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;
	float3 view_dir = ipt.view_dir;

	float2 tc_ddx = ddx(tc);
	float2 tc_ddy = ddy(tc);

	float3 shading = 0;

	float4 mrt_0 = g_buffer_tex.Sample(point_sampler, tc);
	float3 normal = GetNormal(mrt_0);

	float3 dir = g_light_dir_es.xyz;
	float n_dot_l = dot(normal, dir);
	if (n_dot_l > 0)
	{
		float4 mrt_1 = g_buffer_1_tex.Sample(point_sampler, tc);

		view_dir = normalize(view_dir);

		float shininess = Glossiness2Shininess(GetGlossiness(mrt_0));
		float3 c_diff = GetDiffuse(mrt_1);
		float3 c_spec = GetSpecular(mrt_1);

		float3 halfway = normalize(dir - view_dir);
		float3 spec = SpecularTerm(c_spec, dir, halfway, normal, shininess);
		shading = max((c_diff * g_light_attrib.x + spec * g_light_attrib.y) * n_dot_l, 0) * g_light_color.rgb;
	}

	return float4(shading, 1);
}


float3 linear_to_srgb(float3 rgb)
{
	const float ALPHA = 0.055f;
	return rgb < 0.0031308f ? 12.92f * rgb : (1 + ALPHA) * pow(rgb, 1 / 2.4f) - ALPHA;
}


float2 TexCoordFromPos(float4 pos)
{
	float2 tex = pos.xy / 2;
	tex.y *= -1;
	tex += 0.5f;
	return tex;
}


struct PP_VSO
{
	float4 pos : SV_Position;
	float2 tc : TEXCOORD0;
};


PP_VSO PostProcessVS(float4 pos : POSITION)
{
	PP_VSO opt;

	opt.pos = pos;
	opt.tc = TexCoordFromPos(pos);

	return opt;
}


float4 SRGBCorrectionPS(PP_VSO ipt) : SV_Target
{
	float3 rgb = g_pp_tex.Sample(linear_sampler, ipt.tc).xyz;
	rgb = linear_to_srgb(max(rgb, 1e-6f));
	return float4(rgb, 1);
}


technique11 DeferredRendering
{
	pass GBuffer
	{
		SetVertexShader(CompileShader(vs_5_0, GBufferVS()));
		SetPixelShader(CompileShader(ps_5_0, GBufferPS()));
		SetRasterizerState(back_solid_rs);
		SetDepthStencilState(depth_enalbed, 0);
	}

	pass LightingAmbient
	{
		SetVertexShader(CompileShader(vs_5_0, LightingVS()));
		SetPixelShader(CompileShader(ps_5_0, LightingAmbientPS()));
		SetRasterizerState(front_solid_rs);
		SetDepthStencilState(lighting_dss, 128);
	}

	pass LightingSun
	{
		SetVertexShader(CompileShader(vs_5_0, LightingVS()));
		SetPixelShader(CompileShader(ps_5_0, LightingSunPS()));
		SetRasterizerState(front_solid_rs);
		SetDepthStencilState(lighting_dss, 128);
		SetBlendState(lighting_bs, float4(1,1,1,1), 0xffffffff);
	}

	pass SRGBCorrection
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, SRGBCorrectionPS()));
		SetRasterizerState(front_solid_rs);
		SetDepthStencilState(depth_enalbed, 0);
	}
}
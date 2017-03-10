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


SamplerState aniso_sampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};


DepthStencilState depth_enalbed
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
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


struct GBUFFER_VSO
{
	float4 pos : SV_POSITION;
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


technique11 DeferredRendering
{
	pass GBuffer
	{
		SetVertexShader(CompileShader(vs_5_0, GBufferVS()));
		SetPixelShader(CompileShader(ps_5_0, GBufferPS()));
		SetDepthStencilState(depth_enalbed, 0);
	}
}
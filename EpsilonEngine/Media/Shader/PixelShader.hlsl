cbuffer PS_CBUFFER : register(b0)
{
	float4 light_dir;
	float4 ka;
	float4 kd;
	float4 ks;
	int tex_enabled;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD0;
	float3 eye_dir : EYEDIRECTION;
};

Texture2D tex;
SamplerState sampler_stat;

float4 main(PS_INPUT input) : SV_TARGET
{
	float3 norm2 = normalize(input.norm);
	float3 light_dir2 = normalize(light_dir.xyz);
	float3 eye_dir2 = normalize(input.eye_dir);

	float illum_kd = clamp(dot(light_dir2, norm2), 0.0, 1.0);
	float illum_ks = clamp(dot(reflect(light_dir2, norm2), eye_dir2), 0.0, 1.0);
	float3 illum = ka.rgb + kd.rgb * illum_kd + ks.rgb * illum_ks;

	float4 tex_color = float4(1, 1, 1, 1);
	if (tex_enabled == 1)
	{
		tex_color = tex.Sample(sampler_stat, input.tc);
	}

	float4 out_color = float4(tex_color.x * illum.x, tex_color.y * illum.y, tex_color.z * illum.z, 1);
	out_color.x = clamp(out_color.x, 0.0f, 1.0f);
	out_color.y = clamp(out_color.y, 0.0f, 1.0f);
	out_color.z = clamp(out_color.z, 0.0f, 1.0f);

	return out_color;
}
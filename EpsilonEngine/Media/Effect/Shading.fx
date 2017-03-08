

matrix g_world_mat;
matrix g_view_mat;
matrix g_proj_mat;
float4 g_eye_pos;
float4 g_ka;
float4 g_kd;
float4 g_ks;
Texture2D g_tex;
bool g_tex_enabled;


DepthStencilState EnableDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
};


SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};


struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD0;
	float3 eye_dir : EYEDIRECTION;
};


VS_OUTPUT ShadingVS(
	float4 pos : POSITION,
	float3 norm : NORMAL,
	float2 tc : TEXCOORD
)
{
	VS_OUTPUT output;

	output.pos = pos;
	output.pos = mul(output.pos, g_world_mat);
	output.pos = mul(output.pos, g_view_mat);
	output.pos = mul(output.pos, g_proj_mat);

	float4 norm4 = float4(norm, 0);
	norm4 = mul(norm4, g_world_mat);
	norm4 = mul(norm4, g_view_mat);
	norm4 = mul(norm4, g_proj_mat);
	output.norm = norm4.xyz;

	output.tc = tc;

	output.eye_dir = g_eye_pos.xyz - pos.xyz;

	return output;
}


float4 ShadingPS(VS_OUTPUT input) : SV_TARGET
{
	float3 norm2 = normalize(input.norm);
	float3 light_dir2 = float3(-1.0, -1.0, -1.0);
	light_dir2 = normalize(light_dir2);
	float3 eye_dir2 = normalize(input.eye_dir);

	float illum_kd = clamp(dot(light_dir2, norm2), 0.0, 1.0);
	float illum_ks = clamp(dot(reflect(light_dir2, norm2), eye_dir2), 0.0, 1.0);
	float3 illum = g_ka.rgb + g_kd.rgb * illum_kd + g_ks.rgb * illum_ks;

	float4 tex_color = float4(1, 1, 1, 1);
	if (g_tex_enabled)
	{
		tex_color = g_tex.Sample(TextureSampler, input.tc);
	}

	float4 out_color = float4(tex_color.x * illum.x, tex_color.y * illum.y, tex_color.z * illum.z, 1);
	out_color.x = clamp(out_color.x, 0.0, 1.0);
	out_color.y = clamp(out_color.y, 0.0, 1.0);
	out_color.z = clamp(out_color.z, 0.0, 1.0);

	return out_color;
}

technique11 RenderSceneWithTexture1Light
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0_level_9_1, ShadingVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0_level_9_1, ShadingPS()));

		SetDepthStencilState(EnableDepth, 0);
	}
}
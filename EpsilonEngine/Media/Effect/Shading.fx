

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
	float3 light_dir : LIGHTDIRECTION;
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

	output.norm = norm;

	output.tc = tc;

	output.eye_dir = g_eye_pos.xyz - pos.xyz;

	float3 light_pos = float3(0, 10, 0);
	output.light_dir = light_pos - pos.xyz;

	return output;
}


float4 ShadingPS(VS_OUTPUT input) : SV_TARGET
{
	float3 norm2 = normalize(input.norm);
	float3 light_dir2 = normalize(input.light_dir);
	float3 eye_dir2 = normalize(input.eye_dir);

	float3 diff_color = float3(1, 1, 1);
	if (g_tex_enabled)
	{
		diff_color = g_tex.Sample(TextureSampler, input.tc).rgb;
	}

	float illum_diffuse = clamp(dot(light_dir2, norm2), 0.0, 1.0);

	float4 out_color = float4(diff_color * illum_diffuse, 1);
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
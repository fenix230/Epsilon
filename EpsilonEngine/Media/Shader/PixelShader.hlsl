cbuffer PS_CBUFFER
{
	int tex_enabled;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD0;
};


Texture2D tex;
SamplerState sampler_stat;

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 output_color = float4(1, 1, 1, 0);
	if (tex_enabled == 1)
	{
		output_color = tex.Sample(sampler_stat, input.tc);
	}

	return output_color;
}
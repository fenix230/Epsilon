struct Input
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD;
};

Texture2D tex;
SamplerState sampler_stat;

float4 main(Input input) : SV_TARGET
{
	float4 output_color = tex.Sample(sampler_stat, input.tc);

	return output_color;
}
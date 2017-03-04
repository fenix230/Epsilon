cbuffer VS_CBUFFER
{
	matrix world_mat;
	matrix view_mat;
	matrix proj_mat;
};

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD0;
};

VS_OUTPUT main(
	VS_INPUT input
)
{
	VS_OUTPUT output;
	
	output.pos = input.pos;
	output.pos = mul(output.pos, world_mat);
	output.pos = mul(output.pos, view_mat);
	output.pos = mul(output.pos, proj_mat);

	float4 norm4 = float4(input.norm, 0);
	norm4 = mul(norm4, world_mat);
	norm4 = mul(norm4, view_mat);
	norm4 = mul(norm4, proj_mat);
	output.norm = norm4.xyz;

	output.tc = input.tc;

	return output;
}
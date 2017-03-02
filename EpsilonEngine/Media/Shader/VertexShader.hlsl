cbuffer Constant
{
	matrix world_mat;
	matrix view_mat;
	matrix proj_mat;
};

struct Output
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD;
};

Output main(
	float4 pos : POSITION,
	float3 norm : NORMAL,
	float2 tc : TEXCOORD
)
{
	Output output;
	
	output.pos = pos;
	output.pos = mul(output.pos, world_mat);
	output.pos = mul(output.pos, view_mat);
	output.pos = mul(output.pos, proj_mat);

	float4 norm4 = float4(norm.x, norm.y, norm.z, 0.0);
	norm4 = mul(norm4, world_mat);
	norm4 = mul(norm4, view_mat);
	norm4 = mul(norm4, proj_mat);
	output.norm = norm4.xyz;

	output.tc = tc;

	return output;
}
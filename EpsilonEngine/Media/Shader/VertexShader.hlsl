cbuffer CB
{
	matrix world_mat;
	matrix view_mat;
	matrix proj_mat;
};

float4 main(
	float4 pos : POSITION, 
	float3 normal : NORMAL,
	float3 diffuse : DIFFUSE,
	float3 specular : SPECULAR
) : SV_POSITION
{
	float4 out_pos = pos;

	out_pos.w = 1.0f;
	out_pos = mul(out_pos, world_mat);
	out_pos = mul(out_pos, view_mat);
	out_pos = mul(out_pos, proj_mat);

	return out_pos;
}
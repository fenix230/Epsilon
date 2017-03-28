#pragma once
#include "D3D11Predeclare.h"
#include "RSPredeclare.h"
#include "Utils.h"
#include <vector>


namespace epsilon
{

	class Renderable
	{
	public:
		INTERFACE_SET_RE;

		virtual void Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass) = 0;
	};


	class StaticMesh : public Renderable
	{
	public:
		StaticMesh();
		virtual ~StaticMesh();

		virtual void Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass) override;

		void CreateVertexBuffer(size_t num_vert,
			const Vector3f* pos_data,
			const Vector3f* norm_data,
			const Vector2f* tc_data);
		void CreateIndexBuffer(size_t num_indice, const uint32_t* data);
		void CreateMaterial(std::string file_path, Vector3f ka, Vector3f kd, Vector3f ks);

		void Destory();

	private:
		ID3D11InputLayout* D3DInputLayout(ID3DX11EffectPass* pass);

	private:
		struct VS_INPUT
		{
			Vector3f pos;
			Vector3f norm;
			Vector2f tc;
		};

		ID3D11BufferPtr d3d_vertex_buffer_;
		ID3D11BufferPtr d3d_index_buffer_;

		unsigned int num_indice_;

		std::vector<std::pair<ID3DX11EffectPass*, ID3D11InputLayoutPtr>> d3d_input_layouts_;

		ID3D11ResourcePtr d3d_tex_;
		ID3D11ShaderResourceViewPtr d3d_srv_;

		Vector3f ka_, kd_, ks_;
	};


	class Quad : public Renderable
	{
	public:
		Quad();
		virtual ~Quad();

		virtual void Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass) override;

		void Destory();

	private:
		ID3D11InputLayout* D3DInputLayout(ID3DX11EffectPass* pass);

	private:
		ID3D11BufferPtr d3d_vertex_buffer_;

		std::vector<std::pair<ID3DX11EffectPass*, ID3D11InputLayoutPtr>> d3d_input_layouts_;
	};


}
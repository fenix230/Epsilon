#include "Renderable.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include "RenderEngine.h"
#include "d3dx11effect.h"
#include "DDSTextureLoader\DDSTextureLoader.h"


namespace epsilon
{

	void StaticMesh::CreateVertexBuffer(size_t num_vert,
		const Vector3f* pos_data,
		const Vector3f* norm_data,
		const Vector2f* tc_data)
	{
		std::vector<VS_INPUT> vs_inputs(num_vert);
		for (size_t i = 0; i != num_vert; i++)
		{
			vs_inputs[i].pos = pos_data[i];
			vs_inputs[i].norm = norm_data[i];
			vs_inputs[i].tc = tc_data[i];
		}

		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(VS_INPUT) * (UINT)num_vert;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = vs_inputs.data();
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_buffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_buffer));
		d3d_vertex_buffer_ = MakeCOMPtr(d3d_buffer);
	}

	void StaticMesh::CreateIndexBuffer(size_t num_indice, const uint32_t* data)
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(uint32_t) * (UINT)num_indice;
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = data;
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_index_buffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_index_buffer));
		d3d_index_buffer_ = MakeCOMPtr(d3d_index_buffer);

		num_indice_ = (UINT)num_indice;
	}

	void StaticMesh::CreateMaterial(std::string file_path, Vector3f ka, Vector3f kd, Vector3f ks)
	{
		if (!file_path.empty())
		{
			std::wstring wfile_path = ToWstring(file_path);

			ID3D11Resource* d3d_tex_res = nullptr;
			ID3D11ShaderResourceView* d3d_tex_srv = nullptr;
			if (SUCCEEDED(CreateDDSTextureFromFile(re_->D3DDevice(), wfile_path.c_str(), &d3d_tex_res, &d3d_tex_srv)))
			{
				d3d_tex_ = MakeCOMPtr(d3d_tex_res);
				d3d_srv_ = MakeCOMPtr(d3d_tex_srv);
			}
		}

		ka_ = ka;
		kd_ = kd;
		ks_ = ks;
	}


	ID3D11InputLayout* StaticMesh::D3DInputLayout(ID3DX11EffectPass* pass)
	{
		for (auto i = d3d_input_layouts_.begin(); i != d3d_input_layouts_.end(); i++)
		{
			if (i->first == pass)
			{
				return i->second.get();
			}
		}

		const D3D11_INPUT_ELEMENT_DESC d3d_elems_descs[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		D3DX11_PASS_DESC pass_desc;
		THROW_FAILED(pass->GetDesc(&pass_desc));

		ID3D11InputLayout* d3d_input_layout = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateInputLayout(d3d_elems_descs, (UINT)std::size(d3d_elems_descs),
			pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &d3d_input_layout));

		d3d_input_layouts_.emplace_back(pass, MakeCOMPtr(d3d_input_layout));

		return d3d_input_layout;
	}

	StaticMesh::StaticMesh()
	{
		num_indice_ = 0;
	}

	StaticMesh::~StaticMesh()
	{
		this->Destory();
	}

	void StaticMesh::Destory()
	{
		d3d_input_layouts_.clear();
		d3d_vertex_buffer_.reset();
		d3d_index_buffer_.reset();
		d3d_tex_.reset();
		d3d_srv_.reset();
	}

	void StaticMesh::Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass)
	{
		//Material
		if (d3d_srv_)
		{
			auto var_g_albedo_tex = effect->GetVariableByName("g_albedo_tex")->AsShaderResource();
			var_g_albedo_tex->SetResource(d3d_srv_.get());

			auto var_g_albedo_map_enabled = effect->GetVariableByName("g_albedo_map_enabled")->AsScalar();
			var_g_albedo_map_enabled->SetBool(true);
		}
		else
		{
			auto var_g_albedo_map_enabled = effect->GetVariableByName("g_albedo_map_enabled")->AsScalar();
			var_g_albedo_map_enabled->SetBool(false);
		}

		auto var_g_albedo_clr = effect->GetVariableByName("g_albedo_clr")->AsVector();
		Vector3f albedo_clr(0.58f, 0.58f, 0.58f);
		var_g_albedo_clr->SetFloatVector((float*)&albedo_clr);

		auto var_g_metalness_clr = effect->GetVariableByName("g_metalness_clr")->AsVector();
		Vector2f metalness_clr(0.02f, 0);
		var_g_metalness_clr->SetFloatVector((float*)&metalness_clr);

		auto var_g_glossiness_clr = effect->GetVariableByName("g_glossiness_clr")->AsVector();
		Vector2f glossiness_clr(0.04f, 0);
		var_g_glossiness_clr->SetFloatVector((float*)&glossiness_clr);

		//Vertex buffer and index buffer
		std::array<ID3D11Buffer*, 1> buffers = {
			d3d_vertex_buffer_.get()
		};

		std::array<UINT, 1> strides = {
			sizeof(VS_INPUT)
		};

		std::array<UINT, 1> offsets = {
			0
		};

		re_->D3DContext()->IASetVertexBuffers(0, 1, buffers.data(), strides.data(), offsets.data());

		re_->D3DContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		re_->D3DContext()->IASetIndexBuffer(d3d_index_buffer_.get(), DXGI_FORMAT_R32_UINT, 0);

		re_->D3DContext()->IASetInputLayout(this->D3DInputLayout(pass));

		pass->Apply(0, re_->D3DContext());

		re_->D3DContext()->DrawIndexed(num_indice_, 0, 0);
	}

	Quad::Quad()
	{

	}

	Quad::~Quad()
	{
		this->Destory();
	}

	void Quad::Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass)
	{
		if (!d3d_vertex_buffer_)
		{
			Vector3f vs_inputs[] =
			{
				Vector3f(-1, +1, 1),
				Vector3f(+1, +1, 1),
				Vector3f(-1, -1, 1),
				Vector3f(+1, -1, 1)
			};

			D3D11_BUFFER_DESC buffer_desc;
			buffer_desc.Usage = D3D11_USAGE_DEFAULT;
			buffer_desc.ByteWidth = sizeof(Vector3f) * (UINT)std::size(vs_inputs);
			buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			buffer_desc.CPUAccessFlags = 0;
			buffer_desc.MiscFlags = 0;
			buffer_desc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA buffer_data;
			buffer_data.pSysMem = vs_inputs;
			buffer_data.SysMemPitch = 0;
			buffer_data.SysMemSlicePitch = 0;

			ID3D11Buffer* d3d_buffer = nullptr;
			THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_buffer));
			d3d_vertex_buffer_ = MakeCOMPtr(d3d_buffer);
		}

		//Vertex buffer and index buffer
		std::array<ID3D11Buffer*, 1> buffers = {
			d3d_vertex_buffer_.get()
		};

		std::array<UINT, 1> strides = {
			sizeof(Vector3f)
		};

		std::array<UINT, 1> offsets = {
			0
		};

		re_->D3DContext()->IASetVertexBuffers(0, 1, buffers.data(), strides.data(), offsets.data());

		re_->D3DContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		re_->D3DContext()->IASetInputLayout(this->D3DInputLayout(pass));

		pass->Apply(0, re_->D3DContext());

		re_->D3DContext()->Draw(4, 0);
	}

	void Quad::Destory()
	{
		d3d_input_layouts_.clear();
	}

	ID3D11InputLayout* Quad::D3DInputLayout(ID3DX11EffectPass* pass)
	{
		for (auto i = d3d_input_layouts_.begin(); i != d3d_input_layouts_.end(); i++)
		{
			if (i->first == pass)
			{
				return i->second.get();
			}
		}

		const D3D11_INPUT_ELEMENT_DESC d3d_elems_descs[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		D3DX11_PASS_DESC pass_desc;
		THROW_FAILED(pass->GetDesc(&pass_desc));

		ID3D11InputLayout* d3d_input_layout = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateInputLayout(d3d_elems_descs, (UINT)std::size(d3d_elems_descs),
			pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &d3d_input_layout));

		d3d_input_layouts_.emplace_back(pass, MakeCOMPtr(d3d_input_layout));

		return d3d_input_layout;
	}


}
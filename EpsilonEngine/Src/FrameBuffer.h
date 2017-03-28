#pragma once
#include "Utils.h"
#include "D3D11Predeclare.h"
#include "RSPredeclare.h"
#include <vector>


namespace epsilon
{

	class FrameBuffer
	{
	public:
		FrameBuffer();
		virtual ~FrameBuffer();

		INTERFACE_SET_RE;

		void Create(uint32_t width, uint32_t height, ID3D11Texture2D* sc_buffer);

		void Create(uint32_t width, uint32_t height, size_t rtv_count);

		void Create(uint32_t width, uint32_t height, size_t rtv_count, ID3D11Texture2D* sc_buffer, size_t sc_index);

		void Destory();

		void Clear(Vector4f* c = nullptr);

		void Bind();

		ID3D11ShaderResourceView* RetriveShaderResourceView(size_t index);

	private:
		int /*DXGI_FORMAT*/ rtv_fmt_;
		int /*DXGI_FORMAT*/ dsv_fmt_;

		struct RTV
		{
			ID3D11Texture2DPtr d3d_rtv_tex_;
			ID3D11RenderTargetViewPtr d3d_rtv_;
			ID3D11ShaderResourceViewPtr d3d_srv_;
		};
		std::vector<RTV> rtvs_;

		ID3D11Texture2DPtr d3d_dsv_tex_;
		ID3D11DepthStencilViewPtr d3d_dsv_;
	};

}
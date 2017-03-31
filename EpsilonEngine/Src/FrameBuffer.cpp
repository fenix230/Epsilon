#include "FrameBuffer.h"
#include "RenderEngine.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>


namespace epsilon
{

	FrameBuffer::FrameBuffer()
	{
		rtv_fmt_ = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	FrameBuffer::FrameBuffer(int rtv_fmt)
	{
		rtv_fmt_ = rtv_fmt;
	}

	FrameBuffer::~FrameBuffer()
	{
		this->Destory();
	}

	void FrameBuffer::Create(uint32_t width, uint32_t height, ID3D11Texture2D* sc_buffer)
	{
		this->Create(width, height, 1, sc_buffer, 0);
	}

	void FrameBuffer::Create(uint32_t width, uint32_t height, size_t rtv_count)
	{
		this->Create(width, height, rtv_count, nullptr, -1);
	}

	void FrameBuffer::Create(uint32_t width, uint32_t height, size_t rtv_count, ID3D11Texture2D* sc_buffer, size_t sc_index)
	{
		//Render target view
		rtvs_.resize(rtv_count);

		for (size_t i = 0; i != rtv_count; i++)
		{
			RTV& rtv = rtvs_[i];
			if (i == sc_index)
			{
				rtv.d3d_rtv_ = MakeCOMPtr(re_->D3DCreateRenderTargetView(sc_buffer));
			}
			else
			{
				rtv.d3d_rtv_tex_ = MakeCOMPtr(re_->D3DCreateTexture2D(width, height, rtv_fmt_,
					D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE));

				D3D11_RENDER_TARGET_VIEW_DESC d3d_rtv_desc;
				d3d_rtv_desc.Format = (DXGI_FORMAT)rtv_fmt_;
				d3d_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				d3d_rtv_desc.Texture2D.MipSlice = 0;

				ID3D11RenderTargetView* d3d_rtv = nullptr;
				THROW_FAILED(re_->D3DDevice()->CreateRenderTargetView(rtv.d3d_rtv_tex_.get(), &d3d_rtv_desc, &d3d_rtv));

				rtv.d3d_rtv_ = MakeCOMPtr(d3d_rtv);
			}
		}

		//Depth stencil view
		d3d_dsv_tex_ = MakeCOMPtr(re_->D3DCreateTexture2D(width, height,
			DXGI_FORMAT_R24G8_TYPELESS, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE));

		d3d_dsv_ = MakeCOMPtr(re_->D3DCreateDepthStencilView(d3d_dsv_tex_.get(),
			DXGI_FORMAT_D24_UNORM_S8_UINT));
	}

	void FrameBuffer::Destory()
	{
		rtvs_.clear();
		d3d_dsv_tex_.reset();
		d3d_dsv_.reset();
	}

	void FrameBuffer::Clear(Vector4f* c)
	{
		float clean_color[4] = { 0, 0, 0, 0 };
		float* pc = (c != nullptr ? (float*)c : clean_color);

		for (size_t i = 0; i != rtvs_.size(); i++)
		{
			re_->D3DContext()->ClearRenderTargetView(rtvs_[i].d3d_rtv_.get(), pc);
		}

		re_->D3DContext()->ClearDepthStencilView(d3d_dsv_.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	void FrameBuffer::Bind()
	{
		//Bind render target
		std::vector<ID3D11RenderTargetView*> d3d_rtvs;
		for (size_t i = 0; i != rtvs_.size(); i++)
		{
			d3d_rtvs.push_back(rtvs_[i].d3d_rtv_.get());
		}

		re_->D3DContext()->OMSetRenderTargets((UINT)d3d_rtvs.size(), d3d_rtvs.data(), d3d_dsv_.get());
	}

	ID3D11ShaderResourceView* FrameBuffer::RetriveRTShaderResourceView(size_t index)
	{
		if (!rtvs_[index].d3d_srv_)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC d3d_srv_desc;
			d3d_srv_desc.Format = (DXGI_FORMAT)rtv_fmt_;
			d3d_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			d3d_srv_desc.Texture2D.MostDetailedMip = 0;
			d3d_srv_desc.Texture2D.MipLevels = 1;

			ID3D11ShaderResourceView* d3d_srv = nullptr;
			THROW_FAILED(re_->D3DDevice()->CreateShaderResourceView(rtvs_[index].d3d_rtv_tex_.get(), &d3d_srv_desc, &d3d_srv));
			rtvs_[index].d3d_srv_ = MakeCOMPtr(d3d_srv);
		}

		return rtvs_[index].d3d_srv_.get();
	}

	ID3D11ShaderResourceView* FrameBuffer::RetriveDSShaderResourceView()
	{
		if (!d3d_ds_srv_)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC d3d_srv_desc;
			d3d_srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			d3d_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			d3d_srv_desc.Texture2D.MostDetailedMip = 0;
			d3d_srv_desc.Texture2D.MipLevels = 1;

			ID3D11ShaderResourceView* d3d_srv = nullptr;
			THROW_FAILED(re_->D3DDevice()->CreateShaderResourceView(d3d_dsv_tex_.get(), &d3d_srv_desc, &d3d_srv));
			d3d_ds_srv_ = MakeCOMPtr(d3d_srv);
		}
		
		return d3d_ds_srv_.get();
	}

}
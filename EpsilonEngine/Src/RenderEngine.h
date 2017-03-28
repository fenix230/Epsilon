#pragma once
#include <stdint.h>
#include <string>
#include <Windows.h>
#include <memory>
#include <functional>
#include <vector>
#include <array>

#include "Utils.h"
#include "D3D11Predeclare.h"
#include "RSPredeclare.h"


namespace epsilon
{

	class RenderEngine
	{
	public:
		RenderEngine();
		~RenderEngine();

		void Create(HWND wnd, int width, int height);
		void Destory();

		void Resize(int width, int height);

		void LoadEffect(std::string file_path);

		void SetCamera(CameraPtr cam);

		void AddRenderable(RenderablePtr r);

		void Frame();

		template<class T>
		std::shared_ptr<T> MakeObject()
		{
			std::shared_ptr<T> obj = std::make_shared<T>();
			obj->SetRE(*this);
			return obj;
		}

		IDXGISwapChain1* DXGISwapChain();

		ID3D11Device* D3DDevice();

		ID3D11DeviceContext* D3DContext();

		HRESULT D3DCompile(std::string const & src_data, const char* entry_point, const char* target,
			std::vector<uint8_t>& code, std::string& error_msgs) const;

		ID3D11Texture2D* D3DCreateTexture2D(UINT width, UINT height, int /*DXGI_FORMAT*/ fmt, UINT bind_flags);

		ID3D11DepthStencilView* D3DCreateDepthStencilView(ID3D11Texture2D* tex, int /*DXGI_FORMAT*/ fmt);

		ID3D11RenderTargetView* D3DCreateRenderTargetView(ID3D11Texture2D* tex);

	private:
		HWND wnd_;
		uint32_t width_;
		uint32_t height_;

		HMODULE mod_d3d11_;
		HMODULE mod_dxgi_;
		HMODULE mod_d3dcompiler_;

		IDXGIFactory1Ptr gi_factory_1_;
		IDXGIFactory2Ptr gi_factory_2_;
		IDXGIAdapterPtr gi_adapter_;
		IDXGISwapChain1Ptr gi_swap_chain_1_;

		ID3D11DevicePtr d3d_device_;
		ID3D11DeviceContextPtr d3d_imm_ctx_;

		FrameBufferPtr gbuffer_pass_fb_;
		FrameBufferPtr lighting_pass_fb_;
		FrameBufferPtr srgb_pass_fb_;

		ID3DX11EffectPtr d3d_effect_;

		QuadPtr quad_;

		CameraPtr cam_;
		std::vector<RenderablePtr> rs_;
	};

}
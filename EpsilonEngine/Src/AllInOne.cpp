#include "AllInOne.h"
#include <VersionHelpers.h>
#include <ShellScalingAPI.h>
#include <windowsx.h>
#include <algorithm>
#include <assert.h>
#include <array>
#include <vector>
#include <fstream>
#include <d3dcompiler.h>
#include "DDSTextureLoader\DDSTextureLoader.h"


namespace epsilon
{

	std::wstring ToWstring(const std::string& str, const std::locale& loc = std::locale())
	{
		std::vector<wchar_t> buf(str.size());
		std::use_facet<std::ctype<wchar_t>>(loc).widen(str.data(),//ctype<char_type>  
			str.data() + str.size(),
			buf.data());//把char转换为T  
		return std::wstring(buf.data(), buf.size());
	}

	std::string ToString(const std::wstring& str, const std::locale& loc = std::locale())
	{
		std::vector<char> buf(str.size());
		std::use_facet<std::ctype<wchar_t>>(loc).narrow(str.data(),
			str.data() + str.size(),
			'?', buf.data());//把T转换为char  
		return std::string(buf.data(), buf.size());
	}


	inline std::string CombineFileLine(std::string const & file, int line)
	{
		char str[256];
		sprintf_s(str, "%s: %d", file.c_str(), line);
		return std::string(str);
	}

	template <typename T>
	inline std::shared_ptr<T> MakeCOMPtr(T* p)
	{
		return p ? std::shared_ptr<T>(p, std::mem_fn(&T::Release)) : std::shared_ptr<T>();
	}


#define DO_THROW(x)	{ throw std::system_error(std::make_error_code(x), CombineFileLine(__FILE__, __LINE__)); }
#define THROW_FAILED(x)	{ HRESULT _hr = x; if (static_cast<HRESULT>(_hr) < 0) { throw std::runtime_error(CombineFileLine(__FILE__, __LINE__)); } }


	Window::Window(const std::string& name, int width, int height)
		: active_(false), ready_(false), closed_(false), dpi_scale_(1), win_rotation_(WR_Identity), hide_(false)
	{
		re_ = nullptr;

		this->DetectsDPI();

		HINSTANCE hInst = ::GetModuleHandle(nullptr);

		// Register the window class
		name_ = ToWstring(name);
		WNDCLASSEXW wc;
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = sizeof(this);
		wc.hInstance = hInst;
		wc.hIcon = nullptr;
		wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = name_.c_str();
		wc.hIconSm = nullptr;
		::RegisterClassExW(&wc);

		uint32_t style = WS_OVERLAPPEDWINDOW;

		RECT rc = { 0, 0, width, height };
		::AdjustWindowRect(&rc, style, false);

		// Create our main window
		// Pass pointer to self
		wnd_ = ::CreateWindowW(name_.c_str(), name_.c_str(),
			style, 100, 50,
			rc.right - rc.left, rc.bottom - rc.top, 0, 0, hInst, nullptr);

		default_wnd_proc_ = ::DefWindowProc;
		external_wnd_ = false;

		::GetClientRect(wnd_, &rc);
		left_ = rc.left;
		top_ = rc.top;
		width_ = rc.right - rc.left;
		height_ = rc.bottom - rc.top;

#pragma warning(push)
#pragma warning(disable: 4244) // Pointer to LONG_PTR, possible loss of data
		::SetWindowLongPtrW(wnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
#pragma warning(pop)

		::ShowWindow(wnd_, hide_ ? SW_HIDE : SW_SHOWNORMAL);
		::UpdateWindow(wnd_);

		ready_ = true;
	}

	Window::~Window()
	{
		if (wnd_ != nullptr)
		{
#pragma warning(push)
#pragma warning(disable: 4244) // Pointer to LONG_PTR, possible loss of data
			::SetWindowLongPtrW(wnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
#pragma warning(pop)

			if (!external_wnd_)
			{
				::DestroyWindow(wnd_);
			}

			wnd_ = nullptr;
		}
	}

	HWND Window::HWnd() const
	{
		return wnd_;
	}

	int32_t Window::Left() const
	{
		return left_;
	}

	int32_t Window::Top() const
	{
		return top_;
	}

	uint32_t Window::Width() const
	{
		return width_;
	}

	uint32_t Window::Height() const
	{
		return height_;
	}

	bool Window::Active() const
	{
		return active_;
	}

	void Window::Active(bool active)
	{
		active_ = active;
	}

	bool Window::Ready() const
	{
		return ready_;
	}

	void Window::Ready(bool ready)
	{
		ready_ = ready;
	}

	bool Window::Closed() const
	{
		return closed_;
	}

	void Window::Closed(bool closed)
	{
		closed_ = closed;
	}

	float Window::DPIScale() const
	{
		return dpi_scale_;
	}

	epsilon::Window::WindowRotation Window::Rotation() const
	{
		return win_rotation_;
	}

	void Window::SetRE(RenderEngine& re)
	{
		re_ = &re;
	}

	LRESULT Window::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
#pragma warning(push)
#pragma warning(disable: 4312) // LONG_PTR to Window*, could cast to a greater size
		Window* win = reinterpret_cast<Window*>(::GetWindowLongPtrA(hWnd, GWLP_USERDATA));
#pragma warning(pop)

		if (win != nullptr)
		{
			return win->MsgProc(hWnd, uMsg, wParam, lParam);
		}
		else
		{
			return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}

	BOOL Window::EnumMonProc(HMONITOR mon, HDC dc_mon, RECT* rc_mon, LPARAM lparam)
	{
		(void)(dc_mon);
		(void)(rc_mon);

		HMODULE shcore = ::LoadLibraryEx(TEXT("SHCore.dll"), nullptr, 0);
		if (shcore)
		{
			typedef HRESULT(CALLBACK *GetDpiForMonitorFunc)(HMONITOR mon, MONITOR_DPI_TYPE dpi_type, UINT* dpi_x, UINT* dpi_y);
			GetDpiForMonitorFunc DynamicGetDpiForMonitor
				= reinterpret_cast<GetDpiForMonitorFunc>(::GetProcAddress(shcore, "GetDpiForMonitor"));
			if (DynamicGetDpiForMonitor)
			{
				UINT dpi_x, dpi_y;
				if (S_OK == DynamicGetDpiForMonitor(mon, MDT_DEFAULT, &dpi_x, &dpi_y))
				{
					Window* win = reinterpret_cast<Window*>(lparam);
					win->dpi_scale_ = (std::max)(win->dpi_scale_, static_cast<float>((std::max)(dpi_x, dpi_y)) / USER_DEFAULT_SCREEN_DPI);
				}
			}

			::FreeLibrary(shcore);
		}

		return TRUE;
	}

	LRESULT Window::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_ACTIVATE:
			active_ = (WA_INACTIVE != LOWORD(wParam));
			//this->OnActive()(*this, active_);
			break;

		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
			//this->OnPaint()(*this);
			break;

		case WM_ENTERSIZEMOVE:
			// Previent rendering while moving / sizing
			ready_ = false;
			//this->OnEnterSizeMove()(*this);
			break;

		case WM_EXITSIZEMOVE:
			//this->OnExitSizeMove()(*this);
			ready_ = true;
			break;

		case WM_SIZE:
		{
			RECT rc;
			::GetClientRect(wnd_, &rc);
			left_ = rc.left;
			top_ = rc.top;
			width_ = rc.right - rc.left;
			height_ = rc.bottom - rc.top;

			// Check to see if we are losing or gaining our window.  Set the
			// active flag to match
			if ((SIZE_MAXHIDE == wParam) || (SIZE_MINIMIZED == wParam))
			{
				active_ = false;
				//this->OnSize()(*this, false);
			}
			else
			{
				active_ = true;
				//this->OnSize()(*this, true);
				if (re_)
				{
					re_->Resize(width_, height_);
				}
			}
		}
		break;

		case WM_GETMINMAXINFO:
			// Prevent the window from going smaller than some minimu size
			reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.x = 100;
			reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.y = 100;
			break;

		case WM_SETCURSOR:
			//this->OnSetCursor()(*this);
			break;

		case WM_CHAR:
			//this->OnChar()(*this, static_cast<wchar_t>(wParam));
			break;

		case WM_INPUT:
			//this->OnRawInput()(*this, reinterpret_cast<HRAWINPUT>(lParam));
			break;

		case WM_DPICHANGED:
			dpi_scale_ = static_cast<float>(HIWORD(wParam)) / USER_DEFAULT_SCREEN_DPI;
			break;

		case WM_CLOSE:
			//this->OnClose()(*this);
			active_ = false;
			ready_ = false;
			closed_ = true;
			::PostQuitMessage(0);
			return 0;
		}

		return default_wnd_proc_(hWnd, uMsg, wParam, lParam);
	}

	void Window::DetectsDPI()
	{
		HMODULE shcore = ::LoadLibraryEx(TEXT("SHCore.dll"), nullptr, 0);
		if (shcore)
		{
			typedef HRESULT(WINAPI *SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS value);
			SetProcessDpiAwarenessFunc DynamicSetProcessDpiAwareness
				= reinterpret_cast<SetProcessDpiAwarenessFunc>(::GetProcAddress(shcore, "SetProcessDpiAwareness"));

			DynamicSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

			::FreeLibrary(shcore);
		}

		typedef NTSTATUS(WINAPI *RtlGetVersionFunc)(OSVERSIONINFOEXW* pVersionInformation);
		HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
		assert(ntdll != nullptr);
		RtlGetVersionFunc DynamicRtlGetVersion = reinterpret_cast<RtlGetVersionFunc>(::GetProcAddress(ntdll, "RtlGetVersion"));
		if (DynamicRtlGetVersion)
		{
			OSVERSIONINFOEXW os_ver_info;
			os_ver_info.dwOSVersionInfoSize = sizeof(os_ver_info);
			DynamicRtlGetVersion(&os_ver_info);

			if ((os_ver_info.dwMajorVersion > 6) || ((6 == os_ver_info.dwMajorVersion) && (os_ver_info.dwMinorVersion >= 3)))
			{
				HDC desktop_dc = ::GetDC(nullptr);
				::EnumDisplayMonitors(desktop_dc, nullptr, EnumMonProc, reinterpret_cast<LPARAM>(this));
				::ReleaseDC(nullptr, desktop_dc);
			}
		}
	}


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
			ID3D11ShaderResourceView* d3d_tex_sr_view = nullptr;
			THROW_FAILED(CreateDDSTextureFromFile(re_->D3DDevice(), wfile_path.c_str(), &d3d_tex_res, &d3d_tex_sr_view));
			d3d_tex_res_ = MakeCOMPtr(d3d_tex_res);
			d3d_tex_sr_view_ = MakeCOMPtr(d3d_tex_sr_view);
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
		d3d_tex_res_.reset();
		d3d_tex_sr_view_.reset();
	}


	void StaticMesh::Render(ID3DX11Effect* effect, ID3DX11EffectPass* pass)
	{
		//Shader resource view
		if (d3d_tex_sr_view_)
		{
			auto var_g_tex = effect->GetVariableByName("g_tex")->AsShaderResource();
			var_g_tex->SetResource(d3d_tex_sr_view_.get());

			auto var_g_tex_enabled = effect->GetVariableByName("g_tex_enabled")->AsScalar();
			var_g_tex_enabled->SetBool(true);
		}
		else
		{
			auto var_g_tex_enabled = effect->GetVariableByName("g_tex_enabled")->AsScalar();
			var_g_tex_enabled->SetBool(false);
		}

		//ka、kd、ks
		auto var_g_ka = effect->GetVariableByName("g_ka")->AsVector();
		auto var_g_kd = effect->GetVariableByName("g_kd")->AsVector();
		auto var_g_ks = effect->GetVariableByName("g_ks")->AsVector();
		Vector4f v4 = Vector4f(ka_.x, ka_.y, ka_.z, 0);
		var_g_ka->SetFloatVector((float*)&v4);
		v4 = Vector4f(kd_.x, kd_.y, kd_.z, 0);
		var_g_kd->SetFloatVector((float*)&v4);
		v4 = Vector4f(ks_.x, ks_.y, ks_.z, 0);
		var_g_ks->SetFloatVector((float*)&v4);

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

	void Camera::Bind(ID3DX11Effect* effect)
	{
		auto var_g_world_mat = effect->GetVariableByName("g_world_mat")->AsMatrix();
		auto var_g_view_mat = effect->GetVariableByName("g_view_mat")->AsMatrix();
		auto var_g_proj_mat = effect->GetVariableByName("g_proj_mat")->AsMatrix();
		auto var_g_eye_pos = effect->GetVariableByName("g_eye_pos")->AsVector();

		var_g_world_mat->SetMatrix((float*)&world_);
		var_g_view_mat->SetMatrix((float*)&view_);
		var_g_proj_mat->SetMatrix((float*)&proj_);
		Vector4f v4(eye_pos_.x, eye_pos_.y, eye_pos_.z, 0);
		var_g_eye_pos->SetFloatVector((float*)&v4);
	}

	void Camera::LookAt(Vector3f pos, Vector3f target, Vector3f up)
	{
		view_ = XMMatrixLookAtLH(pos.XMV(), target.XMV(), up.XMV());
		eye_pos_ = pos;
		look_at_ = target;
		up_ = up;
	}

	void Camera::Perspective(float ang, float aspect, float near_plane, float far_plane)
	{
		proj_ = XMMatrixPerspectiveFovLH(ang, aspect, near_plane, far_plane);
	}


	RenderEngine::RenderEngine()
	{
		wnd_ = nullptr;
		width_ = 0;
		height_ = 0;

		mod_dxgi_ = ::LoadLibraryEx(TEXT("dxgi.dll"), nullptr, 0);
		if (nullptr == mod_dxgi_)
		{
			::MessageBoxW(nullptr, L"Can't load dxgi.dll", L"Error", MB_OK);
		}
		mod_d3d11_ = ::LoadLibraryEx(TEXT("d3d11.dll"), nullptr, 0);
		if (nullptr == mod_d3d11_)
		{
			::MessageBoxW(nullptr, L"Can't load d3d11.dll", L"Error", MB_OK);
		}
		mod_d3dcompiler_ = ::LoadLibraryEx(TEXT("d3dcompiler_47.dll"), nullptr, 0);
		if (nullptr == mod_d3dcompiler_)
		{
			::MessageBoxW(nullptr, L"Can't load d3dcompiler_47.dll", L"Error", MB_OK);
		}

		if (mod_dxgi_ != nullptr)
		{
			DynamicCreateDXGIFactory1_ = reinterpret_cast<CreateDXGIFactory1Func>(::GetProcAddress(mod_dxgi_, "CreateDXGIFactory1"));
		}
		if (mod_d3d11_ != nullptr)
		{
			DynamicD3D11CreateDevice_ = reinterpret_cast<D3D11CreateDeviceFunc>(::GetProcAddress(mod_d3d11_, "D3D11CreateDevice"));
		}
		if (mod_d3dcompiler_ != nullptr)
		{
			DynamicD3DCompile_ = reinterpret_cast<pD3DCompile>(::GetProcAddress(mod_d3dcompiler_, "D3DCompile"));
		}
	}

	RenderEngine::~RenderEngine()
	{
		this->Destory();
	}

	void RenderEngine::Create(HWND wnd, int width, int height)
	{
		wnd_ = wnd;
		
		IDXGIFactory1* gi_factory;
		THROW_FAILED(DynamicCreateDXGIFactory1_(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&gi_factory)));
		gi_factory_1_ = MakeCOMPtr(gi_factory);

		IDXGIFactory2* gi_factory2;
		THROW_FAILED(gi_factory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&gi_factory2)));
		gi_factory_2_ = MakeCOMPtr(gi_factory2);

		IDXGIAdapter* dxgi_adapter = nullptr;
		THROW_FAILED(gi_factory_1_->EnumAdapters(0, &dxgi_adapter));
		gi_adapter_ = MakeCOMPtr(dxgi_adapter);

		IDXGIOutput* output = nullptr;
		THROW_FAILED(dxgi_adapter->EnumOutputs(0, &output));

		UINT num = 0;
		THROW_FAILED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num, 0));

		std::vector<DXGI_MODE_DESC> mode_descs(num);
		THROW_FAILED(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num, &mode_descs[0]));

		output->Release();
		output = nullptr;

		std::vector<D3D_FEATURE_LEVEL> d3d_feature_levels;
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_11_1);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_11_0);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_10_1);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_10_0);
		d3d_feature_levels.emplace_back(D3D_FEATURE_LEVEL_9_3);

		D3D_FEATURE_LEVEL d3d_out_feature_level = D3D_FEATURE_LEVEL_9_3;

		//Device and context
		ID3D11Device* d3d_device = nullptr;
		ID3D11DeviceContext* d3d_imm_ctx = nullptr;

		THROW_FAILED(DynamicD3D11CreateDevice_(dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
			d3d_feature_levels.data(), (UINT)d3d_feature_levels.size(), D3D11_SDK_VERSION, &d3d_device,
			&d3d_out_feature_level, &d3d_imm_ctx));
		d3d_device_ = MakeCOMPtr(d3d_device);
		d3d_imm_ctx_ = MakeCOMPtr(d3d_imm_ctx);

		//Rasterizer
		D3D11_RASTERIZER_DESC raster_desc;
		raster_desc.AntialiasedLineEnable = false;
		raster_desc.CullMode = D3D11_CULL_BACK;
		raster_desc.DepthBias = 0;
		raster_desc.DepthBiasClamp = 0.0f;
		raster_desc.DepthClipEnable = true;
		raster_desc.FillMode = D3D11_FILL_SOLID;
		raster_desc.FrontCounterClockwise = false;
		raster_desc.MultisampleEnable = false;
		raster_desc.ScissorEnable = false;
		raster_desc.SlopeScaledDepthBias = 0.0f;

		ID3D11RasterizerState* raster_state = nullptr;
		THROW_FAILED(d3d_device_->CreateRasterizerState(&raster_desc, &raster_state));
		d3d_raster_state_ = MakeCOMPtr(raster_state);

		d3d_imm_ctx_->RSSetState(raster_state);

		this->Resize(width, height);
	}

	void RenderEngine::Resize(int width, int height)
	{
		width_ = width;
		height_ = height;

		d3d_imm_ctx_->OMSetRenderTargets(0, 0, 0);
		d3d_imm_ctx_->OMSetDepthStencilState(0, 0);

		d3d_depth_stencil_view_.reset();
		d3d_depth_stencil_state_.reset();
		d3d_depth_stencil_buffer_.reset();
		d3d_render_target_view_.reset();

		//SwapChain
		IDXGISwapChain1* dxgi_sc = nullptr;
		if (gi_swap_chain_1_)
		{
			dxgi_sc = gi_swap_chain_1_.get();
			THROW_FAILED(dxgi_sc->ResizeBuffers(2, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, 
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC1 sc_desc1;
			ZeroMemory(&sc_desc1, sizeof(sc_desc1));
			sc_desc1.Width = width_;
			sc_desc1.Height = height_;
			sc_desc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sc_desc1.Stereo = false;
			sc_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sc_desc1.BufferCount = 2;
			sc_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			sc_desc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			sc_desc1.SampleDesc.Count = 1;
			sc_desc1.SampleDesc.Quality = 0;
			sc_desc1.Scaling = DXGI_SCALING_STRETCH;
			sc_desc1.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC sc_fs_desc;
			sc_fs_desc.RefreshRate.Numerator = 60;
			sc_fs_desc.RefreshRate.Denominator = 1;
			sc_fs_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			sc_fs_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			sc_fs_desc.Windowed = true;

			THROW_FAILED(gi_factory_2_->CreateSwapChainForHwnd(d3d_device_.get(), wnd_, &sc_desc1, &sc_fs_desc, nullptr, &dxgi_sc));
			gi_swap_chain_1_ = MakeCOMPtr(dxgi_sc);
		}

		//Render target view
		ID3D11Texture2D* back_buffer = nullptr;
		THROW_FAILED(dxgi_sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&back_buffer));

		ID3D11RenderTargetView* d3d_render_target_view = nullptr;
		THROW_FAILED(d3d_device_->CreateRenderTargetView(back_buffer, NULL, &d3d_render_target_view));
		d3d_render_target_view_ = MakeCOMPtr(d3d_render_target_view);

		back_buffer->Release();
		back_buffer = nullptr;

		//Depth stencil view
		D3D11_TEXTURE2D_DESC depth_buffer_desc;
		ZeroMemory(&depth_buffer_desc, sizeof(depth_buffer_desc));
		depth_buffer_desc.Width = width_;
		depth_buffer_desc.Height = height_;
		depth_buffer_desc.MipLevels = 1;
		depth_buffer_desc.ArraySize = 1;
		depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_buffer_desc.SampleDesc.Count = 1;
		depth_buffer_desc.SampleDesc.Quality = 0;
		depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depth_buffer_desc.CPUAccessFlags = 0;
		depth_buffer_desc.MiscFlags = 0;

		ID3D11Texture2D* depth_stencil_buffer;
		THROW_FAILED(d3d_device_->CreateTexture2D(&depth_buffer_desc, NULL, &depth_stencil_buffer));
		d3d_depth_stencil_buffer_ = MakeCOMPtr(depth_stencil_buffer);

		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		ZeroMemory(&depth_stencil_desc, sizeof(depth_stencil_desc));
		depth_stencil_desc.DepthEnable = true;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
		depth_stencil_desc.StencilEnable = true;
		depth_stencil_desc.StencilReadMask = 0xFF;
		depth_stencil_desc.StencilWriteMask = 0xFF;
		depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		ID3D11DepthStencilState* depth_stencil_state = nullptr;
		THROW_FAILED(d3d_device_->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state));
		d3d_depth_stencil_state_ = MakeCOMPtr(depth_stencil_state);

		d3d_imm_ctx_->OMSetDepthStencilState(depth_stencil_state, 1);

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
		ZeroMemory(&depth_stencil_view_desc, sizeof(depth_stencil_view_desc));
		depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;

		ID3D11DepthStencilView* depth_stencil_view = nullptr;
		THROW_FAILED(d3d_device_->CreateDepthStencilView(depth_stencil_buffer, &depth_stencil_view_desc, &depth_stencil_view));
		d3d_depth_stencil_view_ = MakeCOMPtr(depth_stencil_view);

		//Bind render target
		d3d_imm_ctx_->OMSetRenderTargets(1, &d3d_render_target_view, depth_stencil_view);

		//Viewport
		D3D11_VIEWPORT viewport;
		viewport.Width = (float)width_;
		viewport.Height = (float)height_;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		d3d_imm_ctx_->RSSetViewports(1, &viewport);
	}

	void RenderEngine::Destory()
	{
		rs_.clear();
		cam_.reset();

		if (gi_swap_chain_1_)
		{
			gi_swap_chain_1_->SetFullscreenState(false, nullptr);
		}

		d3d_effect_.reset();
		d3d_raster_state_.reset();
		d3d_depth_stencil_view_.reset();
		d3d_depth_stencil_state_.reset();
		d3d_depth_stencil_buffer_.reset();
		d3d_render_target_view_.reset();
		d3d_imm_ctx_.reset();
		d3d_device_.reset();

		gi_swap_chain_1_.reset();
		gi_adapter_.reset();
		gi_factory_1_.reset();
		gi_factory_2_.reset();

		::FreeLibrary(mod_d3dcompiler_);
		::FreeLibrary(mod_d3d11_);
		::FreeLibrary(mod_dxgi_);
	}

	void RenderEngine::LoadEffect(std::string file_path)
	{
		std::wstring wfile_path = ToWstring(file_path);

		ID3DX11Effect* d3d_effect = nullptr;
		ID3DBlob* d3d_error_blob = nullptr;
		THROW_FAILED(D3DX11CompileEffectFromFile(wfile_path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			D3DCOMPILE_ENABLE_STRICTNESS, 0, this->D3DDevice(), &d3d_effect, &d3d_error_blob));
		d3d_effect_ = MakeCOMPtr(d3d_effect);
	}

	void RenderEngine::SetCamera(CameraPtr cam)
	{
		cam_ = cam;
	}

	void RenderEngine::AddRenderable(RenderablePtr r)
	{
		rs_.push_back(r);
	}

	void RenderEngine::Frame()
	{
		float clean_color[4] = { 0.2f, 0.2f, 0.6f, 1 };
		d3d_imm_ctx_->ClearRenderTargetView(d3d_render_target_view_.get(), clean_color);
		d3d_imm_ctx_->ClearDepthStencilView(d3d_depth_stencil_view_.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		cam_->Bind(d3d_effect_.get());

		ID3DX11EffectTechnique* tech = d3d_effect_->GetTechniqueByIndex(0);
		D3DX11_TECHNIQUE_DESC tech_desc;
		THROW_FAILED(tech->GetDesc(&tech_desc));

		for (uint32_t p = 0; p < tech_desc.Passes; ++p)
		{
			ID3DX11EffectPass* pass = tech->GetPassByIndex(p);
			for (auto i = rs_.begin(); i != rs_.end(); i++)
			{
				RenderablePtr r = *i;
				r->Render(d3d_effect_.get(), pass);
			}
		}

		gi_swap_chain_1_->Present(0, 0);
	}

	ID3D11Device* RenderEngine::D3DDevice()
	{
		return d3d_device_.get();
	}

	ID3D11DeviceContext* RenderEngine::D3DContext()
	{
		return d3d_imm_ctx_.get();
	}

	HRESULT RenderEngine::D3DCompile(std::string const & src_data, const char* entry_point, const char* target,
		std::vector<uint8_t>& code, std::string& error_msgs) const
	{
		ID3DBlob* code_blob = nullptr;
		ID3DBlob* error_msgs_blob = nullptr;

		uint32_t const flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION;
		HRESULT hr = DynamicD3DCompile_(src_data.c_str(), static_cast<UINT>(src_data.size()),
			nullptr, nullptr, nullptr, entry_point,
			target, flags, 0, &code_blob, &error_msgs_blob);
		if (code_blob)
		{
			uint8_t const * p = static_cast<uint8_t const *>(code_blob->GetBufferPointer());
			code.assign(p, p + code_blob->GetBufferSize());
			code_blob->Release();
		}
		else
		{
			code.clear();
		}
		if (error_msgs_blob)
		{
			char const * p = static_cast<char const *>(error_msgs_blob->GetBufferPointer());
			error_msgs.assign(p, p + error_msgs_blob->GetBufferSize());
			error_msgs_blob->Release();
		}
		else
		{
			error_msgs.clear();
		}
		return hr;
	}

	Application::Application()
	{

	}

	Application::~Application()
	{
		this->Destory();
	}

	void Application::Create(const std::string& name, int width, int height)
	{
		main_wnd_ = std::make_unique<Window>(name, width, height);

		re_ = std::make_unique<RenderEngine>();
		re_->Create(main_wnd_->HWnd(), width, height);

		main_wnd_->SetRE(*re_);
	}

	void Application::Destory()
	{
		re_->Destory();
	}

	void Application::Run()
	{
		bool gotMsg;
		MSG  msg;

		::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

		while (WM_QUIT != msg.message)
		{
			// 如果窗口是激活的，用 PeekMessage()以便我们可以用空闲时间渲染场景
			// 不然, 用 GetMessage() 减少 CPU 占用率
			if (main_wnd_ && main_wnd_->Active())
			{
				gotMsg = (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0);
			}
			else
			{
				gotMsg = (::GetMessage(&msg, nullptr, 0, 0) != 0);
			}

			if (gotMsg)
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			else
			{
				if (re_)
				{
					re_->Frame();
				}
			}
		}
	}

}


#undef THR
#undef TIF
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


namespace epsilon
{

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
		this->DetectsDPI();

		HINSTANCE hInst = ::GetModuleHandle(nullptr);

		// Register the window class
		name_ = name;
		WNDCLASSEXA wc;
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
		::RegisterClassExA(&wc);

		uint32_t style = WS_OVERLAPPEDWINDOW;

		RECT rc = { 0, 0, width, height };
		::AdjustWindowRect(&rc, style, false);

		// Create our main window
		// Pass pointer to self
		wnd_ = ::CreateWindowA(name_.c_str(), name_.c_str(),
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
		::SetWindowLongPtrA(wnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
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
			::SetWindowLongPtrA(wnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
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


	void StaticMesh::CreatePositionBuffer(const std::vector<Vector3f>& positions)
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(Vector3f) * (UINT)positions.size();
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = positions.data();
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_position_buffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_position_buffer));
		d3d_position_buffer_ = MakeCOMPtr(d3d_position_buffer);

		num_position_ = (UINT)positions.size();
	}

	void StaticMesh::CreateNormalBuffer(const std::vector<Vector3f>& normals)
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(Vector3f) * (UINT)normals.size();
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = normals.data();
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_normal_buffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_normal_buffer));
		d3d_normal_buffer_ = MakeCOMPtr(d3d_normal_buffer);

		num_normal_ = (UINT)normals.size();
	}

	void StaticMesh::CreateDiffuseBuffer(const std::vector<Vector3f>& diffuses)
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(Vector3f) * (UINT)diffuses.size();
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = diffuses.data();
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_diffuse_buffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_diffuse_buffer));
		d3d_diffuse_buffer_ = MakeCOMPtr(d3d_diffuse_buffer);

		num_diffuse_ = (UINT)diffuses.size();
	}

	void StaticMesh::CreateSpecularBuffer(const std::vector<Vector3f>& speculars)
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(Vector3f) * (UINT)speculars.size();
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = speculars.data();
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_specular_buffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_specular_buffer));
		d3d_specular_buffer_ = MakeCOMPtr(d3d_specular_buffer);

		num_specular_ = (UINT)speculars.size();
	}

	void StaticMesh::CreateIndexBuffer(const std::vector<uint32_t>& indices)
	{
		D3D11_BUFFER_DESC buffer_desc;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.ByteWidth = sizeof(uint32_t) * (UINT)indices.size();
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = indices.data();
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;

		ID3D11Buffer* d3d_index_buffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&buffer_desc, &buffer_data, &d3d_index_buffer));
		d3d_index_buffer_ = MakeCOMPtr(d3d_index_buffer);

		num_indice_ = (UINT)indices.size();
	}

	void StaticMesh::PositionElemDesc(D3D11_INPUT_ELEMENT_DESC& desc)
	{
		desc.SemanticName = "POSITION";
		desc.SemanticIndex = 0;
		desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.InputSlot = 0;
		desc.AlignedByteOffset = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
	}

	void StaticMesh::NormalElemDesc(D3D11_INPUT_ELEMENT_DESC& desc)
	{
		desc.SemanticName = "NORMAL";
		desc.SemanticIndex = 0;
		desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.InputSlot = 0;
		desc.AlignedByteOffset = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
	}

	void StaticMesh::DiffuseElemDesc(D3D11_INPUT_ELEMENT_DESC& desc)
	{
		desc.SemanticName = "DIFFUSE";
		desc.SemanticIndex = 0;
		desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.InputSlot = 0;
		desc.AlignedByteOffset = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
	}

	void StaticMesh::SpecularElemDesc(D3D11_INPUT_ELEMENT_DESC& desc)
	{
		desc.SemanticName = "SPECULAR";
		desc.SemanticIndex = 0;
		desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.InputSlot = 0;
		desc.AlignedByteOffset = 0;
		desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		desc.InstanceDataStepRate = 0;
	}

	ID3D11InputLayout* StaticMesh::D3DInputLayout(ShaderObject* so)
	{
		for (auto i = d3d_input_layouts_.begin(); i != d3d_input_layouts_.end(); i++)
		{
			if (i->first == so)
			{
				return i->second.get();
			}
		}

		std::array<D3D11_INPUT_ELEMENT_DESC, 4> d3d_elems_descs;
		this->PositionElemDesc(d3d_elems_descs[0]);
		this->NormalElemDesc(d3d_elems_descs[1]);
		this->DiffuseElemDesc(d3d_elems_descs[2]);
		this->SpecularElemDesc(d3d_elems_descs[3]);

		const std::vector<uint8_t>& vs_code = so->VSCode();

		ID3D11InputLayout* d3d_input_layout = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateInputLayout(d3d_elems_descs.data(), (UINT)d3d_elems_descs.size(),
			&vs_code[0], vs_code.size(), &d3d_input_layout));

		d3d_input_layouts_.emplace_back(so, MakeCOMPtr(d3d_input_layout));

		return d3d_input_layout;
	}

	StaticMesh::StaticMesh()
	{
		num_position_ = 0;
		num_normal_ = 0;
		num_diffuse_ = 0;
		num_specular_ = 0;
		num_indice_ = 0;
	}

	StaticMesh::~StaticMesh()
	{
		this->Destory();
	}
	
	void StaticMesh::Destory()
	{
		d3d_position_buffer_.reset();
		d3d_normal_buffer_.reset();
		d3d_diffuse_buffer_.reset();
		d3d_specular_buffer_.reset();
		d3d_index_buffer_.reset();
		d3d_input_layouts_.clear();
	}

	void StaticMesh::Bind()
	{
		static const UINT num_buffers = 4;

		std::array<ID3D11Buffer*, num_buffers> buffers = {
			d3d_position_buffer_.get(),
			d3d_normal_buffer_.get(),
			d3d_diffuse_buffer_.get(),
			d3d_specular_buffer_.get()
		};

		std::array<UINT, num_buffers> strides = {
			num_position_,
			num_normal_,
			num_diffuse_,
			num_specular_
		};

		std::array<UINT, num_buffers> offsets = {
			0,
			0,
			0,
			0
		};

		re_->D3DContext()->IASetVertexBuffers(0, 4, buffers.data(), strides.data(), offsets.data());

		re_->D3DContext()->IASetIndexBuffer(d3d_index_buffer_.get(), DXGI_FORMAT_R32_UINT, 0);

		re_->D3DContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void StaticMesh::Render(ShaderObject* so)
	{
		re_->D3DContext()->IASetInputLayout(this->D3DInputLayout(so));

		so->Bind();

		re_->D3DContext()->DrawIndexed(num_indice_, 0, 0);
	}


	void Camera::LookAt(Vector3f pos, Vector3f target, Vector3f up)
	{
		view = XMMatrixLookAtLH(pos.XMVStore(), target.XMVStore(), up.XMVStore());
	}

	void Camera::Perspective(float ang, float aspect, float near_plane, float far_plane)
	{
		proj = XMMatrixPerspectiveFovLH(ang, aspect, near_plane, far_plane);
	}


	CBufferObject::CBufferObject()
	{
		re_ = nullptr;
	}

	CBufferObject::~CBufferObject()
	{
		this->Destory();
	}

	void CBufferObject::BindVS()
	{
		D3D11_MAPPED_SUBRESOURCE mapped_res;
		THROW_FAILED(re_->D3DContext()->Map(d3d_cbuffer_.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res));

		Matrix* cbuffer_ptr = (Matrix*)mapped_res.pData;
		cbuffer_ptr[0] = XMMatrixTranspose(camera_.world);
		cbuffer_ptr[1] = XMMatrixTranspose(camera_.view);
		cbuffer_ptr[2] = XMMatrixTranspose(camera_.proj);
		re_->D3DContext()->Unmap(d3d_cbuffer_.get(), 0);

		ID3D11Buffer* d3d_cbuffer = d3d_cbuffer_.get();
		re_->D3DContext()->VSSetConstantBuffers(0, 1, &d3d_cbuffer);
	}

	void CBufferObject::BindPS()
	{
		D3D11_MAPPED_SUBRESOURCE mapped_res;
		THROW_FAILED(re_->D3DContext()->Map(d3d_cbuffer_.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res));

		Matrix* cbuffer_ptr = (Matrix*)mapped_res.pData;
		cbuffer_ptr[0] = XMMatrixTranspose(camera_.world);
		cbuffer_ptr[1] = XMMatrixTranspose(camera_.view);
		cbuffer_ptr[2] = XMMatrixTranspose(camera_.proj);
		re_->D3DContext()->Unmap(d3d_cbuffer_.get(), 0);

		ID3D11Buffer* d3d_cbuffer = d3d_cbuffer_.get();
		re_->D3DContext()->PSSetConstantBuffers(0, 1, &d3d_cbuffer);
	}

	void CBufferObject::Create()
	{
		D3D11_BUFFER_DESC cbuffer_desc;
		cbuffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		cbuffer_desc.ByteWidth = sizeof(Matrix) * 3;
		cbuffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbuffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbuffer_desc.MiscFlags = 0;
		cbuffer_desc.StructureByteStride = 0;

		ID3D11Buffer* d3d_cbuffer = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateBuffer(&cbuffer_desc, NULL, &d3d_cbuffer));
		d3d_cbuffer_ = MakeCOMPtr(d3d_cbuffer);
	}


	void CBufferObject::Destory()
	{
		d3d_cbuffer_.reset();
		re_ = nullptr;
	}

	ShaderObject::ShaderObject()
	{
		re_ = nullptr;
	}

	ShaderObject::~ShaderObject()
	{
		this->Destory();
	}

	void ShaderObject::CreateVS(std::string file_path, std::string entry_point)
	{
		std::string hlsl_text;

		std::ifstream fs(file_path);
		char c = 0;
		while (fs.get(c))
		{
			hlsl_text.push_back(c);
		}
		fs.close();

		std::vector<uint8_t> shader_code;
		std::string err_msg;
		THROW_FAILED(re_->D3DCompile(hlsl_text, entry_point.c_str(), "vs_5_0", shader_code, err_msg));

		ID3D11VertexShader* d3d_vs = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreateVertexShader(shader_code.data(), shader_code.size(), nullptr, &d3d_vs));
		d3d_vs_ = MakeCOMPtr(d3d_vs);

		vs_code_ = shader_code;
	}

	void ShaderObject::CreatePS(std::string file_path, std::string entry_point)
	{
		std::string hlsl_text;

		std::ifstream fs(file_path);
		char c = 0;
		while (fs.get(c))
		{
			hlsl_text.push_back(c);
		}
		fs.close();

		std::vector<uint8_t> shader_code;
		std::string err_msg;
		THROW_FAILED(re_->D3DCompile(hlsl_text, entry_point.c_str(), "ps_5_0", shader_code, err_msg));

		ID3D11PixelShader* d3d_ps = nullptr;
		THROW_FAILED(re_->D3DDevice()->CreatePixelShader(shader_code.data(), shader_code.size(), nullptr, &d3d_ps));
		d3d_ps_ = MakeCOMPtr(d3d_ps);
	}

	void ShaderObject::Destory()
	{
		d3d_vs_.reset();
		d3d_ps_.reset();
		re_ = nullptr;
	}

	const std::vector<uint8_t>& ShaderObject::VSCode()
	{
		return vs_code_;
	}

	void ShaderObject::Bind()
	{
		re_->D3DContext()->VSSetShader(d3d_vs_.get(), nullptr, 0);
		re_->D3DContext()->PSSetShader(d3d_ps_.get(), nullptr, 0);
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
		width_ = width;
		height_ = height;

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

		//SwapChain
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

		IDXGISwapChain1* dxgi_sc = nullptr;
		THROW_FAILED(gi_factory_2_->CreateSwapChainForHwnd(d3d_device, wnd_, &sc_desc1, &sc_fs_desc, nullptr, &dxgi_sc));
		gi_swap_chain_1_ = MakeCOMPtr(dxgi_sc);

		//Render target view
		ID3D11Texture2D* back_buffer = nullptr;
		THROW_FAILED(dxgi_sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&back_buffer));

		ID3D11RenderTargetView* d3d_render_target_view = nullptr;
		THROW_FAILED(d3d_device->CreateRenderTargetView(back_buffer, NULL, &d3d_render_target_view));
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
		THROW_FAILED(d3d_device->CreateTexture2D(&depth_buffer_desc, NULL, &depth_stencil_buffer));
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
		THROW_FAILED(d3d_device->CreateDepthStencilState(&depth_stencil_desc, &depth_stencil_state));
		d3d_depth_stencil_state_ = MakeCOMPtr(depth_stencil_state);

		d3d_imm_ctx->OMSetDepthStencilState(depth_stencil_state, 1);

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
		ZeroMemory(&depth_stencil_view_desc, sizeof(depth_stencil_view_desc));
		depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;

		ID3D11DepthStencilView* depth_stencil_view = nullptr;
		THROW_FAILED(d3d_device->CreateDepthStencilView(depth_stencil_buffer, &depth_stencil_view_desc, &depth_stencil_view));
		d3d_depth_stencil_view_ = MakeCOMPtr(depth_stencil_view);

		//Bind render target
		d3d_imm_ctx->OMSetRenderTargets(1, &d3d_render_target_view, depth_stencil_view);

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
		THROW_FAILED(d3d_device->CreateRasterizerState(&raster_desc, &raster_state));
		d3d_raster_state_ = MakeCOMPtr(raster_state);

		d3d_imm_ctx->RSSetState(raster_state);

		//Viewport
		D3D11_VIEWPORT viewport;
		viewport.Width = (float)width_;
		viewport.Height = (float)height_;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		d3d_imm_ctx->RSSetViewports(1, &viewport);
	}

	void RenderEngine::Destory()
	{
		if (gi_swap_chain_1_)
		{
			gi_swap_chain_1_->SetFullscreenState(false, nullptr);
		}

		d3d_raster_state_.reset();
		d3d_depth_stencil_view_.reset();
		d3d_depth_stencil_state_.reset();
		d3d_depth_stencil_buffer_.reset();
		d3d_render_target_view_.reset();
		d3d_imm_ctx_.reset();
		d3d_device_.reset();
		gi_swap_chain_1_.reset();

		::FreeLibrary(mod_d3dcompiler_);
		::FreeLibrary(mod_d3d11_);
		::FreeLibrary(mod_dxgi_);
	}

	void RenderEngine::SetShaderObject(ShaderObjectPtr so)
	{
		so_ = so;
	}

	void RenderEngine::SetCBufferObject(CBufferObjectPtr cb)
	{
		cb_ = cb;
	}

	void RenderEngine::AddRenderable(RenderablePtr r)
	{
		rs_.push_back(r);
	}

	void RenderEngine::Frame()
	{
		float clean_color[4] = {0, 0, 0, 1};
		d3d_imm_ctx_->ClearRenderTargetView(d3d_render_target_view_.get(), clean_color);
		d3d_imm_ctx_->ClearDepthStencilView(d3d_depth_stencil_view_.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		cb_->BindVS();
		cb_->BindPS();

		for (auto i = rs_.begin(); i != rs_.end(); i++)
		{
			RenderablePtr r = *i;
			r->Bind();
			r->Render(so_.get());
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

	void Application::Create(const std::string& name, int width, int height)
	{
		main_wnd_ = std::make_unique<Window>(name, width, height);

		re_ = std::make_unique<RenderEngine>();
		re_->Create(main_wnd_->HWnd(), width, height);
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
			// ��������Ǽ���ģ��� PeekMessage()�Ա����ǿ����ÿ���ʱ����Ⱦ����
			// ��Ȼ, �� GetMessage() ���� CPU ռ����
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
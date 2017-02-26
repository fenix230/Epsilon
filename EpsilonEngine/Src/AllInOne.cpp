#include "AllInOne.h"
#include <VersionHelpers.h>
#include <ShellScalingAPI.h>
#include <windowsx.h>
#include <algorithm>
#include <assert.h>

namespace epsilon
{

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


	RenderEngine::RenderEngine()
	{
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

		if (mod_dxgi_ != nullptr)
		{
			DynamicCreateDXGIFactory1_ = reinterpret_cast<CreateDXGIFactory1Func>(::GetProcAddress(mod_dxgi_, "CreateDXGIFactory1"));
		}
		if (mod_d3d11_ != nullptr)
		{
			DynamicD3D11CreateDevice_ = reinterpret_cast<D3D11CreateDeviceFunc>(::GetProcAddress(mod_d3d11_, "D3D11CreateDevice"));
		}

		IDXGIFactory1* gi_factory;
		TIF(DynamicCreateDXGIFactory1_(IID_IDXGIFactory1, reinterpret_cast<void**>(&gi_factory)));
		gi_factory_1_ = MakeCOMPtr(gi_factory);
		dxgi_sub_ver_ = 1;

		IDXGIFactory2* gi_factory2;
		gi_factory->QueryInterface(IID_IDXGIFactory2, reinterpret_cast<void**>(&gi_factory2));
		if (gi_factory2 != nullptr)
		{
			gi_factory_2_ = MakeCOMPtr(gi_factory2);
			dxgi_sub_ver_ = 2;

			IDXGIFactory3* gi_factory3;
			gi_factory->QueryInterface(IID_IDXGIFactory3, reinterpret_cast<void**>(&gi_factory3));
			if (gi_factory3 != nullptr)
			{
				gi_factory_3_ = MakeCOMPtr(gi_factory3);
				dxgi_sub_ver_ = 3;

				IDXGIFactory4* gi_factory4;
				gi_factory->QueryInterface(IID_IDXGIFactory4, reinterpret_cast<void**>(&gi_factory4));
				if (gi_factory4 != nullptr)
				{
					gi_factory_4_ = MakeCOMPtr(gi_factory4);
					dxgi_sub_ver_ = 4;
				}
			}
		}

		UINT adapter_no = 0;
		IDXGIAdapter1* dxgi_adapter = nullptr;
		while (gi_factory_1_->EnumAdapters1(adapter_no, &dxgi_adapter) != DXGI_ERROR_NOT_FOUND)
		{
			if (dxgi_adapter != nullptr)
			{
				auto adapter = MakeUniquePtr<D3D11Adapter>(adapter_no, MakeCOMPtr(dxgi_adapter));
				adapter->Enumerate();
				adapters_.push_back(std::move(adapter));
			}

			++adapter_no;
		}

		if (adapters_.empty())
		{
			THR(errc::function_not_supported);
		}
	}

	void RenderEngine::Create(HWND wnd, int width, int height)
	{
	}

	void RenderEngine::Frame()
	{
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
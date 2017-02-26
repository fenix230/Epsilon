#pragma once
#include <stdint.h>
#include <string>
#include <Windows.h>
#include <memory>
#include <functional>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>

namespace epsilon
{
	using namespace DirectX;

#define DEFINE_VECTOR_OPERATORS(VECTOR)\
	inline VECTOR& operator+= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorAdd(v1.XMVStore(), v2.XMVStore());\
		return v1.XMVLoad(vm);\
	}\
	inline VECTOR& operator-= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorSubtract(v1.XMVStore(), v2.XMVStore());\
		return v1.XMVLoad(vm);\
	}\
	inline VECTOR& operator*= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorMultiply(v1.XMVStore(), v2.XMVStore());\
		return v1.XMVLoad(vm);\
	}\
	inline VECTOR& operator/= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorDivide(v1.XMVStore(), v2.XMVStore());\
		return v1.XMVLoad(vm);\
	}\
	inline VECTOR& operator*= (VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMVStore(), s);\
		return v.XMVLoad(vm);\
	}\
	inline VECTOR& operator/= (VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMVStore(), 1 / s);\
		return v.XMVLoad(vm);\
	}\
	inline VECTOR operator+ (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorAdd(v1.XMVStore(), v2.XMVStore());\
		return VECTOR().XMVLoad(vm);\
	}\
	inline VECTOR operator- (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorSubtract(v1.XMVStore(), v2.XMVStore());\
		return VECTOR().XMVLoad(vm);\
	}\
	inline VECTOR operator* (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorMultiply(v1.XMVStore(), v2.XMVStore());\
		return VECTOR().XMVLoad(vm);\
	}\
	inline VECTOR operator/ (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorDivide(v1.XMVStore(), v2.XMVStore());\
		return VECTOR().XMVLoad(vm);\
	}\
	inline VECTOR operator* (const VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMVStore(), s);\
		return VECTOR().XMVLoad(vm);\
	}\
	inline VECTOR operator* (float s, const VECTOR& v)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMVStore(), s);\
		return VECTOR().XMVLoad(vm);\
	}\
	inline VECTOR operator/ (const VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMVStore(), 1 / s);\
		return VECTOR().XMVLoad(vm);\
	}\
	inline VECTOR operator+ (const VECTOR& v)\
	{\
		return v;\
	}\
	inline VECTOR operator- (const VECTOR& v)\
	{\
		return v * -1.0f;\
	}

	struct Vector2f : public XMFLOAT2
	{
		Vector2f() : XMFLOAT2(0, 0) {}
		Vector2f(float xx, float yy) : XMFLOAT2(xx, yy) {}
		explicit Vector2f(const float* arr) : XMFLOAT2(arr) {}

		XMVECTOR XMVStore() const { return XMLoadFloat2(this); }
		Vector2f& XMVLoad(const XMVECTOR& v) { XMStoreFloat2(this, v); return *this; }
	};

	DEFINE_VECTOR_OPERATORS(Vector2f);


	struct Vector3f : public XMFLOAT3
	{
		Vector3f() : XMFLOAT3(0, 0, 0) {}
		Vector3f(float xx, float yy, float zz) : XMFLOAT3(xx, yy, zz) {}
		explicit Vector3f(const float* arr) : XMFLOAT3(arr) {}

		XMVECTOR XMVStore() const { return XMLoadFloat3(this); }
		Vector3f& XMVLoad(const XMVECTOR& v) { XMStoreFloat3(this, v); return *this; }
	};

	DEFINE_VECTOR_OPERATORS(Vector3f);


	struct Vector4f : public XMFLOAT4
	{
		Vector4f() : XMFLOAT4(0, 0, 0, 0) {}
		Vector4f(float xx, float yy, float zz, float ww) : XMFLOAT4(xx, yy, zz, ww) {}
		explicit Vector4f(const float* arr) : XMFLOAT4(arr) {}

		XMVECTOR XMVStore() const { return XMLoadFloat4(this); }
		Vector4f& XMVLoad(const XMVECTOR& v) { XMStoreFloat4(this, v); return *this; }
	};

	DEFINE_VECTOR_OPERATORS(Vector4f);

	struct Matrix : public XMMATRIX
	{
		Matrix() 
		{
			*(XMMATRIX*)this = XMMatrixIdentity();
		}
		Matrix(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33) : XMMATRIX(
				m00, m01, m02, m03, m10, m11, m12, m13,
				m20, m21, m22, m23, m30, m31, m32, m33) {}
		explicit Matrix(const float *arr) : XMMATRIX(arr) {}

		Matrix& operator= (const XMMATRIX& m) { *(XMMATRIX*)this = m; return *this; }
	};


	inline Vector4f Transform(const Vector4f& v, const Matrix& mat)
	{
		return Vector4f().XMVLoad(XMVector4Transform(v.XMVStore(), mat));
	}

	inline Vector3f TransformCoord(const Vector3f& v, const Matrix& mat)
	{
		return Vector3f().XMVLoad(XMVector3TransformCoord(v.XMVStore(), mat));
	}

	inline Vector3f TransformNormal(const Vector3f& v, const Matrix& mat)
	{
		return Vector3f().XMVLoad(XMVector3TransformNormal(v.XMVStore(), mat));
	}

	inline Vector2f TransformCoord(const Vector2f& v, const Matrix& mat)
	{
		return Vector2f().XMVLoad(XMVector2TransformCoord(v.XMVStore(), mat));
	}

	inline Vector2f TransformNormal(const Vector2f& v, const Matrix& mat)
	{
		return Vector2f().XMVLoad(XMVector2TransformNormal(v.XMVStore(), mat));
	}


	class Window
	{
	public:
		enum WindowRotation
		{
			WR_Unspecified,
			WR_Identity,
			WR_Rotate90,
			WR_Rotate180,
			WR_Rotate270
		};

	public:
		Window(const std::string& name, int width, int height);
		~Window();

		HWND HWnd() const;

		int32_t Left() const;
		int32_t Top() const;

		uint32_t Width() const;
		uint32_t Height() const;

		bool Active() const;
		void Active(bool active);
		bool Ready() const;
		void Ready(bool ready);
		bool Closed() const;
		void Closed(bool closed);

		float DPIScale() const;

		WindowRotation Rotation() const;

	private:
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static BOOL CALLBACK EnumMonProc(HMONITOR mon, HDC dc_mon, RECT* rc_mon, LPARAM lparam);

		LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void DetectsDPI();

	private:
		int32_t left_;
		int32_t top_;
		uint32_t width_;
		uint32_t height_;

		bool active_;
		bool ready_;
		bool closed_;

		float dpi_scale_;
		WindowRotation win_rotation_;

		bool hide_;
		bool external_wnd_;
		std::string name_;

		HWND wnd_;
		WNDPROC default_wnd_proc_;
	};

	
	typedef std::shared_ptr<IDXGIFactory1>				IDXGIFactory1Ptr;
	typedef std::shared_ptr<IDXGIFactory2>				IDXGIFactory2Ptr;
	typedef std::shared_ptr<IDXGIFactory3>				IDXGIFactory3Ptr;
	typedef std::shared_ptr<IDXGIFactory4>				IDXGIFactory4Ptr;
	typedef std::shared_ptr<IDXGIAdapter1>				IDXGIAdapter1Ptr;
	typedef std::shared_ptr<IDXGIAdapter2>				IDXGIAdapter2Ptr;
	typedef std::shared_ptr<IDXGISwapChain>				IDXGISwapChainPtr;
	typedef std::shared_ptr<IDXGISwapChain1>			IDXGISwapChain1Ptr;
	typedef std::shared_ptr<IDXGISwapChain2>			IDXGISwapChain2Ptr;
	typedef std::shared_ptr<IDXGISwapChain3>			IDXGISwapChain3Ptr;
	typedef std::shared_ptr<ID3D11Device>				ID3D11DevicePtr;
	typedef std::shared_ptr<ID3D11Device1>				ID3D11Device1Ptr;
	typedef std::shared_ptr<ID3D11Device2>				ID3D11Device2Ptr;
	typedef std::shared_ptr<ID3D11Device3>				ID3D11Device3Ptr;
	typedef std::shared_ptr<ID3D11DeviceContext>		ID3D11DeviceContextPtr;
	typedef std::shared_ptr<ID3D11DeviceContext1>		ID3D11DeviceContext1Ptr;
	typedef std::shared_ptr<ID3D11DeviceContext2>		ID3D11DeviceContext2Ptr;
	typedef std::shared_ptr<ID3D11DeviceContext3>		ID3D11DeviceContext3Ptr;
	typedef std::shared_ptr<ID3D11Resource>				ID3D11ResourcePtr;
	typedef std::shared_ptr<ID3D11Texture1D>			ID3D11Texture1DPtr;
	typedef std::shared_ptr<ID3D11Texture2D>			ID3D11Texture2DPtr;
	typedef std::shared_ptr<ID3D11Texture3D>			ID3D11Texture3DPtr;
	typedef std::shared_ptr<ID3D11Texture2D>			ID3D11TextureCubePtr;
	typedef std::shared_ptr<ID3D11Buffer>				ID3D11BufferPtr;
	typedef std::shared_ptr<ID3D11InputLayout>			ID3D11InputLayoutPtr;
	typedef std::shared_ptr<ID3D11Query>				ID3D11QueryPtr;
	typedef std::shared_ptr<ID3D11Predicate>			ID3D11PredicatePtr;
	typedef std::shared_ptr<ID3D11VertexShader>			ID3D11VertexShaderPtr;
	typedef std::shared_ptr<ID3D11PixelShader>			ID3D11PixelShaderPtr;
	typedef std::shared_ptr<ID3D11GeometryShader>		ID3D11GeometryShaderPtr;
	typedef std::shared_ptr<ID3D11ComputeShader>		ID3D11ComputeShaderPtr;
	typedef std::shared_ptr<ID3D11HullShader>			ID3D11HullShaderPtr;
	typedef std::shared_ptr<ID3D11DomainShader>			ID3D11DomainShaderPtr;
	typedef std::shared_ptr<ID3D11RenderTargetView>		ID3D11RenderTargetViewPtr;
	typedef std::shared_ptr<ID3D11DepthStencilView>		ID3D11DepthStencilViewPtr;
	typedef std::shared_ptr<ID3D11UnorderedAccessView>	ID3D11UnorderedAccessViewPtr;
	typedef std::shared_ptr<ID3D11RasterizerState>		ID3D11RasterizerStatePtr;
	typedef std::shared_ptr<ID3D11RasterizerState1>		ID3D11RasterizerState1Ptr;
	typedef std::shared_ptr<ID3D11DepthStencilState>	ID3D11DepthStencilStatePtr;
	typedef std::shared_ptr<ID3D11BlendState>			ID3D11BlendStatePtr;
	typedef std::shared_ptr<ID3D11BlendState1>			ID3D11BlendState1Ptr;
	typedef std::shared_ptr<ID3D11SamplerState>			ID3D11SamplerStatePtr;
	typedef std::shared_ptr<ID3D11ShaderResourceView>	ID3D11ShaderResourceViewPtr;


	inline std::string CombineFileLine(std::string const & file, int line)
	{
		char str[256];
		sprintf_s(str, "%s: %d", file.c_str(), line);
		return std::string(str);
	}
	
#define THR(x)	{ throw std::system_error(std::make_error_code(x), CombineFileLine(__FILE__, __LINE__)); }

// Throw if failed
#define TIF(x)	{ HRESULT _hr = x; if (static_cast<HRESULT>(_hr) < 0) { throw std::runtime_error(CombineFileLine(__FILE__, __LINE__)); } }

	template <typename T>
	inline std::shared_ptr<T> MakeCOMPtr(T* p)
	{
		return p ? std::shared_ptr<T>(p, std::mem_fn(&T::Release)) : std::shared_ptr<T>();
	}


	class RenderEngine
	{
	public:
		RenderEngine();

		void Create(HWND wnd, int width, int height);

		void Frame();

	private:
		HWND wnd_;
		uint32_t width_;
		uint32_t height_;

		typedef HRESULT(WINAPI *CreateDXGIFactory1Func)(REFIID riid, void** ppFactory);
		typedef HRESULT(WINAPI *D3D11CreateDeviceFunc)(IDXGIAdapter* pAdapter,
			D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags,
			D3D_FEATURE_LEVEL const * pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
			ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext);

		CreateDXGIFactory1Func DynamicCreateDXGIFactory1_;
		D3D11CreateDeviceFunc DynamicD3D11CreateDevice_;

		HMODULE mod_d3d11_;
		HMODULE mod_dxgi_;

		IDXGIFactory1Ptr gi_factory_1_;
		IDXGIFactory2Ptr gi_factory_2_;
		IDXGIFactory3Ptr gi_factory_3_;
		IDXGIFactory4Ptr gi_factory_4_;
		uint8_t dxgi_sub_ver_;
	};


	class Application
	{
	public:
		Application();

		void Create(const std::string& name, int width, int height);

		void Run();

	private:
		std::unique_ptr<RenderEngine> re_; 
		std::unique_ptr<Window> main_wnd_;
	};
}
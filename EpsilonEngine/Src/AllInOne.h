#pragma once
#include <stdint.h>
#include <string>
#include <Windows.h>
#include <memory>
#include <functional>
#include <DirectXMath.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <vector>
#include <d3dcompiler.h>
#include <array>
#include <d3dx11effect.h>


namespace epsilon
{
	using namespace DirectX;

#define DEFINE_VECTOR_OPERATORS(VECTOR)\
	inline VECTOR& operator+= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorAdd(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator-= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorSubtract(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator*= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorMultiply(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator/= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorDivide(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator*= (VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), s);\
		return v.XMV(vm);\
	}\
	inline VECTOR& operator/= (VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), 1 / s);\
		return v.XMV(vm);\
	}\
	inline VECTOR operator+ (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorAdd(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator- (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorSubtract(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator* (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorMultiply(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator/ (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorDivide(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator* (const VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), s);\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator* (float s, const VECTOR& v)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), s);\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator/ (const VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), 1 / s);\
		return VECTOR().XMV(vm);\
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

		XMVECTOR XMV() const { return XMLoadFloat2(this); }
		Vector2f& XMV(const XMVECTOR& v) { XMStoreFloat2(this, v); return *this; }
	};

	DEFINE_VECTOR_OPERATORS(Vector2f);


	struct Vector3f : public XMFLOAT3
	{
		Vector3f() : XMFLOAT3(0, 0, 0) {}
		Vector3f(float xx, float yy, float zz) : XMFLOAT3(xx, yy, zz) {}
		explicit Vector3f(const float* arr) : XMFLOAT3(arr) {}

		XMVECTOR XMV() const { return XMLoadFloat3(this); }
		Vector3f& XMV(const XMVECTOR& v) { XMStoreFloat3(this, v); return *this; }
	};

	DEFINE_VECTOR_OPERATORS(Vector3f);

	inline Vector3f CrossProduct3(const Vector3f& v1, const Vector3f& v2)
	{
		Vector3f rv;
		rv.XMV(XMVector3Cross(v1.XMV(), v2.XMV()));
		return rv;
	}


	struct Vector4f : public XMFLOAT4
	{
		Vector4f() : XMFLOAT4(0, 0, 0, 0) {}
		Vector4f(float xx, float yy, float zz, float ww) : XMFLOAT4(xx, yy, zz, ww) {}
		explicit Vector4f(const float* arr) : XMFLOAT4(arr) {}

		XMVECTOR XMV() const { return XMLoadFloat4(this); }
		Vector4f& XMV(const XMVECTOR& v) { XMStoreFloat4(this, v); return *this; }
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
		return Vector4f().XMV(XMVector4Transform(v.XMV(), mat));
	}

	inline Vector3f TransformCoord(const Vector3f& v, const Matrix& mat)
	{
		return Vector3f().XMV(XMVector3TransformCoord(v.XMV(), mat));
	}

	inline Vector3f TransformNormal(const Vector3f& v, const Matrix& mat)
	{
		return Vector3f().XMV(XMVector3TransformNormal(v.XMV(), mat));
	}

	inline Vector2f TransformCoord(const Vector2f& v, const Matrix& mat)
	{
		return Vector2f().XMV(XMVector2TransformCoord(v.XMV(), mat));
	}

	inline Vector2f TransformNormal(const Vector2f& v, const Matrix& mat)
	{
		return Vector2f().XMV(XMVector2TransformNormal(v.XMV(), mat));
	}


	typedef std::shared_ptr<IDXGIFactory1>				IDXGIFactory1Ptr;
	typedef std::shared_ptr<IDXGIFactory2>				IDXGIFactory2Ptr;
	typedef std::shared_ptr<IDXGIFactory3>				IDXGIFactory3Ptr;
	typedef std::shared_ptr<IDXGIAdapter>				IDXGIAdapterPtr;
	typedef std::shared_ptr<IDXGIAdapter1>				IDXGIAdapter1Ptr;
	typedef std::shared_ptr<IDXGIAdapter2>				IDXGIAdapter2Ptr;
	typedef std::shared_ptr<IDXGISwapChain>				IDXGISwapChainPtr;
	typedef std::shared_ptr<IDXGISwapChain1>			IDXGISwapChain1Ptr;
	typedef std::shared_ptr<IDXGISwapChain2>			IDXGISwapChain2Ptr;
	typedef std::shared_ptr<ID3D11Device>				ID3D11DevicePtr;
	typedef std::shared_ptr<ID3D11Device1>				ID3D11Device1Ptr;
	typedef std::shared_ptr<ID3D11Device2>				ID3D11Device2Ptr;
	typedef std::shared_ptr<ID3D11DeviceContext>		ID3D11DeviceContextPtr;
	typedef std::shared_ptr<ID3D11DeviceContext1>		ID3D11DeviceContext1Ptr;
	typedef std::shared_ptr<ID3D11DeviceContext2>		ID3D11DeviceContext2Ptr;
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
	typedef std::shared_ptr<ID3DX11Effect>				ID3DX11EffectPtr;

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
		std::wstring name_;

		HWND wnd_;
		WNDPROC default_wnd_proc_;
	};


#define DEFINE_SMART_POINTER(X)\
	class X;\
	typedef std::shared_ptr<X> X##Ptr;

#define INTERFACE_SET_RE\
	inline void SetRE(RenderEngine& re) { re_ = &re; }\
	RenderEngine* re_;

	class RenderEngine;
	DEFINE_SMART_POINTER(CBufferPerFrame);
	//DEFINE_SMART_POINTER(ShaderObject);
	DEFINE_SMART_POINTER(Renderable);
	DEFINE_SMART_POINTER(StaticMesh);
	DEFINE_SMART_POINTER(EffectObject);
	DEFINE_SMART_POINTER(Camera);


	struct REObject
	{
		REObject() : re_(nullptr) {}
		virtual ~REObject() {}

		inline void SetRE(RenderEngine& re) { re_ = &re; }

		RenderEngine* re_;
	};


	class Camera
	{
	public:
		void Bind(ID3DX11Effect* effect);

		void LookAt(Vector3f pos, Vector3f target, Vector3f up);

		void Perspective(float ang, float aspect, float near_plane, float far_plane);

		Matrix world_;
		Matrix view_;
		Matrix proj_;

		Vector3f eye_pos_;
		Vector3f look_at_;
		Vector3f up_;
	};


	class Renderable : public REObject
	{
	public:
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

		UINT num_indice_;

		std::vector<std::pair<ID3DX11EffectPass*, ID3D11InputLayoutPtr>> d3d_input_layouts_;

		ID3D11ResourcePtr d3d_tex_res_;
		ID3D11ShaderResourceViewPtr d3d_tex_sr_view_;

		Vector3f ka_, kd_, ks_;
	};


	class RenderEngine
	{
	public:
		RenderEngine();
		~RenderEngine();

		void Create(HWND wnd, int width, int height);
		void Destory();

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

		ID3D11Device* D3DDevice();

		ID3D11DeviceContext* D3DContext();

		HRESULT D3DCompile(std::string const & src_data, const char* entry_point, const char* target,
			std::vector<uint8_t>& code, std::string& error_msgs) const;

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
		pD3DCompile DynamicD3DCompile_;

		HMODULE mod_d3d11_;
		HMODULE mod_dxgi_;
		HMODULE mod_d3dcompiler_;

		IDXGIFactory1Ptr gi_factory_1_;
		IDXGIFactory2Ptr gi_factory_2_;
		IDXGIAdapterPtr gi_adapter_;
		IDXGISwapChain1Ptr gi_swap_chain_1_;

		ID3D11DevicePtr d3d_device_;
		ID3D11DeviceContextPtr d3d_imm_ctx_;

		ID3D11RenderTargetViewPtr d3d_render_target_view_;
		ID3D11Texture2DPtr d3d_depth_stencil_buffer_;
		ID3D11DepthStencilStatePtr d3d_depth_stencil_state_;
		ID3D11DepthStencilViewPtr d3d_depth_stencil_view_;
		ID3D11RasterizerStatePtr d3d_raster_state_;

		ID3DX11EffectPtr d3d_effect_;

		CameraPtr cam_;
		std::vector<RenderablePtr> rs_;
	};


	class Application
	{
	public:
		Application();
		~Application();

		void Create(const std::string& name, int width, int height);
		void Destory();

		RenderEngine& RE() { return *re_; }
		
		void Run();

	private:
		std::unique_ptr<RenderEngine> re_; 
		std::unique_ptr<Window> main_wnd_;
	};
}
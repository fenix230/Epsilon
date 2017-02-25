#pragma once
#include <stdint.h>
#include <string>
#include <Windows.h>
#include <memory>
#include <functional>
#include <DirectXMath.h>

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
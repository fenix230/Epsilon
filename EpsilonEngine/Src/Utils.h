#pragma once
#include <stdint.h>
#include <DirectXMath.h>
#include <string>
#include <xlocale>
#include <memory>


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

		inline Matrix Inverse() const
		{
			return Matrix().operator=(XMMatrixInverse(nullptr, *this));
		}
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

	inline Vector4f Normalize(const Vector4f& v)
	{
		return Vector4f().XMV(XMVector4Normalize(v.XMV()));
	}

	inline Vector3f Normalize(const Vector3f& v)
	{
		return Vector3f().XMV(XMVector3Normalize(v.XMV()));
	}

	inline Vector2f Normalize(const Vector2f& v)
	{
		return Vector2f().XMV(XMVector2Normalize(v.XMV()));
	}

	inline float Length(const Vector2f& v)
	{
		return ::sqrt(v.x * v.x + v.y * v.y);
	}

	inline float Length(const Vector3f& v)
	{
		return ::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	std::wstring ToWstring(const std::string& str, const std::locale& loc = std::locale());

	std::string ToString(const std::wstring& str, const std::locale& loc = std::locale());
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


#define DO_THROW_MSG(x)	{ throw std::exception(x); }
#define DO_THROW(x)	{ throw std::system_error(std::make_error_code(x), CombineFileLine(__FILE__, __LINE__)); }
#define THROW_FAILED(x)	{ HRESULT _hr = x; if (static_cast<HRESULT>(_hr) < 0) { throw std::runtime_error(CombineFileLine(__FILE__, __LINE__)); } }
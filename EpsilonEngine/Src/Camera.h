#pragma once
#include "Utils.h"
#include "D3D11Predeclare.h"


namespace epsilon
{

	class Camera
	{
	public:
		void Bind(ID3DX11Effect* effect);

		void LookAt(Vector3f pos, Vector3f target, Vector3f up);

		void Perspective(float ang, float aspect, float near_plane, float far_plane);

		Vector3f ForwardVec();

		Matrix world_;
		Matrix view_;
		Matrix proj_;

		Vector3f eye_pos_;
		Vector3f look_at_;
		Vector3f up_;

		float ang_;
		float aspect_;
		float near_plane_;
		float far_plane_;
	};

}
#include "Camera.h"
#include <d3dx11effect.h>


namespace epsilon
{

	void Camera::Bind(ID3DX11Effect* effect)
	{
		auto var_g_model_mat = effect->GetVariableByName("g_model_mat")->AsMatrix();
		auto var_g_view_mat = effect->GetVariableByName("g_view_mat")->AsMatrix();
		auto var_g_proj_mat = effect->GetVariableByName("g_proj_mat")->AsMatrix();
		auto var_g_inv_proj_mat = effect->GetVariableByName("g_inv_proj_mat")->AsMatrix();

		var_g_model_mat->SetMatrix((float*)&world_);
		var_g_view_mat->SetMatrix((float*)&view_);
		var_g_proj_mat->SetMatrix((float*)&proj_);

		Matrix inv_proj = proj_.Inverse();
		var_g_inv_proj_mat->SetMatrix((float*)&inv_proj);
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

		ang_ = ang;
		aspect_ = aspect;
		near_plane_ = near_plane;
		far_plane_ = far_plane;
	}

	Vector3f Camera::ForwardVec()
	{
		return Normalize(look_at_ - eye_pos_);
	}


}
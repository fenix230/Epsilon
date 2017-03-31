#include "Light.h"
#include "d3dx11effect.h"
#include "Camera.h"



namespace epsilon
{

	void AmbientLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		auto var_g_light_dir_es = d3d_effect->GetVariableByName("g_light_dir_es")->AsVector();
		auto var_g_light_color = d3d_effect->GetVariableByName("g_light_color")->AsVector();

		Vector3f light_dir(0, 1, 0);
		light_dir = TransformNormal(light_dir, cam->view_);

		var_g_light_dir_es->SetFloatVector((float*)&light_dir);
		var_g_light_color->SetFloatVector((float*)&color_);
	}


	void DirectionLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		auto var_g_light_dir_es = d3d_effect->GetVariableByName("g_light_dir_es")->AsVector();
		auto var_g_light_color = d3d_effect->GetVariableByName("g_light_color")->AsVector();

		Vector3f light_dir = TransformNormal(dir_, cam->view_);

		var_g_light_dir_es->SetFloatVector((float*)&light_dir);
		var_g_light_color->SetFloatVector((float*)&color_);
	}


	void SpotLight::Bind(ID3DX11Effect* d3d_effect, Camera* cam)
	{
		auto var_g_light_pos_es = d3d_effect->GetVariableByName("g_light_pos_es")->AsVector();
		auto var_g_light_dir_es = d3d_effect->GetVariableByName("g_light_dir_es")->AsVector();
		auto var_g_light_color = d3d_effect->GetVariableByName("g_light_color")->AsVector();
		auto var_g_light_falloff_range = d3d_effect->GetVariableByName("g_light_falloff_range")->AsVector();
		auto var_g_spot_light_cos_cone = d3d_effect->GetVariableByName("g_spot_light_cos_cone")->AsVector();

		Vector3f light_pos = TransformCoord(pos_, cam->view_);
		Vector3f light_dir = TransformNormal(dir_, cam->view_);
		Vector2f cos_cone;
		cos_cone.x = cos(inner_ang_);
		cos_cone.y = cos(outter_ang_);
		Vector4f falloff_range;
		falloff_range.x = falloff_.x;
		falloff_range.y = falloff_.y;
		falloff_range.z = falloff_.z;
		falloff_range.w = range_;

		var_g_light_pos_es->SetFloatVector((float*)&light_pos);
		var_g_light_dir_es->SetFloatVector((float*)&light_dir);
		var_g_light_color->SetFloatVector((float*)&color_);
		var_g_light_falloff_range->SetFloatVector((float*)&falloff_range);
		var_g_spot_light_cos_cone->SetFloatVector((float*)&cos_cone);
	}

}
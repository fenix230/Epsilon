#pragma once
#include "Utils.h"
#include "D3D11Predeclare.h"
#include "RSPredeclare.h"


namespace epsilon
{

	class AmbientLight
	{
	public:
		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

		Vector3f color_;
	};


	class DirectionLight
	{
	public:
		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

		Vector3f dir_;
		Vector3f color_;
	};


	class SpotLight
	{
	public:
		void Bind(ID3DX11Effect* d3d_effect, Camera* cam);

		Vector3f pos_;
		Vector3f dir_;
		Vector3f color_;
		Vector3f falloff_;
		float range_;
		float inner_ang_;
		float outter_ang_;
	};

}
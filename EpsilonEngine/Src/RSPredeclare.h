#pragma once
#include <memory>


namespace epsilon
{

	class RenderEngine;

	class Camera;
	typedef std::shared_ptr<Camera> CameraPtr;

	class Renderable;
	typedef std::shared_ptr<Renderable> RenderablePtr;

	class StaticMesh;
	typedef std::shared_ptr<StaticMesh> StaticMeshPtr;

	class Quad;
	typedef std::shared_ptr<Quad> QuadPtr;

	class FrameBuffer;
	typedef std::shared_ptr<FrameBuffer> FrameBufferPtr;

	class AmbientLight;
	typedef std::shared_ptr<AmbientLight> AmbientLightPtr;

	class DirectionLight;
	typedef std::shared_ptr<DirectionLight> DirectionLightPtr;

	class SpotLight;
	typedef std::shared_ptr<SpotLight> SpotLightPtr;

}


#define INTERFACE_SET_RE\
	inline void SetRE(RenderEngine& re) { re_ = &re; }\
	RenderEngine* re_;
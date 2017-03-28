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

}


#define INTERFACE_SET_RE\
	inline void SetRE(RenderEngine& re) { re_ = &re; }\
	RenderEngine* re_;
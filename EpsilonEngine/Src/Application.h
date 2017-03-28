#pragma once
#include "RenderEngine.h"
#include "Window.h"


namespace epsilon
{

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
#include "Application.h"


namespace epsilon
{

Application::Application()
{

}

Application::~Application()
{
	this->Destory();
}

void Application::Create(const std::string& name, int width, int height)
{
	main_wnd_ = std::make_unique<Window>(name, width, height);

	re_ = std::make_unique<RenderEngine>();
	re_->Create(main_wnd_->HWnd(), width, height);

	main_wnd_->SetRE(*re_);
}

void Application::Destory()
{
	re_->Destory();
}

void Application::Run()
{
	bool gotMsg;
	MSG  msg;

	::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

	while (WM_QUIT != msg.message)
	{
		// 如果窗口是激活的，用 PeekMessage()以便我们可以用空闲时间渲染场景
		// 不然, 用 GetMessage() 减少 CPU 占用率
		if (main_wnd_ && main_wnd_->Active())
		{
			gotMsg = (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0);
		}
		else
		{
			gotMsg = (::GetMessage(&msg, nullptr, 0, 0) != 0);
		}

		if (gotMsg)
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else
		{
			if (re_)
			{
				re_->Frame();
			}
		}
	}
}

}
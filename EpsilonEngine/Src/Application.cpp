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
		// ��������Ǽ���ģ��� PeekMessage()�Ա����ǿ����ÿ���ʱ����Ⱦ����
		// ��Ȼ, �� GetMessage() ���� CPU ռ����
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
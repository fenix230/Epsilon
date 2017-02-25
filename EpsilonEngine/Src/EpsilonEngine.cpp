// EpsilonEngine.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "AllInOne.h"


int main()
{
	epsilon::Matrix mat;
	mat = mat * mat * mat;

	epsilon::Application app;
	app.Create("Test", 1024, 768);
	app.Run();

    return 0;
}


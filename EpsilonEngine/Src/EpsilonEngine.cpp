// EpsilonEngine.cpp : �������̨Ӧ�ó������ڵ㡣
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


// EpsilonEngine.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "AllInOne.h"

using namespace epsilon;

void GenerateCube(StaticMeshPtr r)
{
	static Vector3f verts[8] = {
		Vector3f(-1, -1, -1),
		Vector3f(1, -1, -1),
		Vector3f(1, 1, -1),
		Vector3f(-1, 1, -1),
		Vector3f(-1, -1, 1),
		Vector3f(1, -1, 1),
		Vector3f(1, 1, 1),
		Vector3f(-1, 1, 1),
	};

	std::vector<Vector3f> positions;
	std::vector<Vector3f> normals;
	std::vector<Vector2f> texcoords;
	std::vector<uint16_t> indices;

	auto draw_plane = [&](int a, int b, int c, int d) {
		Vector3f p1 = verts[a], p2 = verts[b], p3 = verts[c], p4 = verts[d];
		Vector3f norm = CrossProduct3((p3 - p2), (p1 - p2));
		Vector2f tc1(0, 0), tc2(0, 1), tc3(1, 1), tc4(1, 0);

		uint16_t index = (uint16_t)positions.size();
		positions.push_back(p1);
		positions.push_back(p2);
		positions.push_back(p3);
		normals.insert(normals.end(), 3, norm);
		texcoords.push_back(tc1);
		texcoords.push_back(tc2);
		texcoords.push_back(tc3);
		indices.push_back(index);
		indices.push_back(index + 1);
		indices.push_back(index + 2);

		index = (uint16_t)positions.size();
		positions.push_back(p3);
		positions.push_back(p4);
		positions.push_back(p1);
		normals.insert(normals.end(), 3, norm);
		texcoords.push_back(tc3);
		texcoords.push_back(tc4);
		texcoords.push_back(tc1);
		indices.push_back(index);
		indices.push_back(index + 1);
		indices.push_back(index + 2);
	};

	draw_plane(0, 3, 2, 1);
	draw_plane(4, 5, 6, 7);
	draw_plane(0, 1, 5, 4);
	draw_plane(1, 2, 6, 5);
	draw_plane(2, 3, 7, 6);
	draw_plane(0, 4, 7, 3);

	r->CreatePositionBuffer(positions);
	r->CreateNormalBuffer(normals);
	r->CreateTexCoordBuffer(texcoords);
	r->CreateIndexBuffer(indices);
}


int main()
{
	try
	{
		int width = 1024;
		int height = 768;

		Application app;
		app.Create("Test", width, height);

		RenderEngine& re = app.RE();

		CBufferObjectPtr cb = re.MakeObject<CBufferObject>();
		cb->Create();
		Vector3f eye(0, -1.5, 2.0f), at(0, 0, 0), up(0, 0, 1);
		cb->camera_.LookAt(eye, at, up);
		cb->camera_.Perspective(XM_PI * 0.6f, (float)width / (float)height, 1, 500);
		re.SetCBufferObject(cb);

		ShaderObjectPtr so = re.MakeObject<ShaderObject>();
		so->CreateVS("../../../Media/Shader/VertexShader.hlsl", "main");
		so->CreatePS("../../../Media/Shader/PixelShader.hlsl", "main");
		re.SetShaderObject(so);

		StaticMeshPtr r = re.MakeObject<StaticMesh>();
		GenerateCube(r);
		r->CreateTexture("../../../Media/Texture/spnza_bricks_a_diff.dds");
		re.AddRenderable(r);

		app.Run();
	}
	catch (const std::exception& e)
	{
		e.what();
	}
	catch (const std::exception* pe)
	{
		pe->what();
	}
	
    return 0;
}


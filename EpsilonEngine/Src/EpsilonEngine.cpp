// EpsilonEngine.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "AllInOne.h"
#include <filesystem>
#include <assimp\config.h>
#include <assimp\cimport.h>
#include <assimp\postprocess.h>
#include <assimp\scene.h>


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
	std::vector<uint32_t> indices;

	auto draw_plane = [&](int a, int b, int c, int d) {
		Vector3f p1 = verts[a], p2 = verts[b], p3 = verts[c], p4 = verts[d];
		Vector3f norm = CrossProduct3((p3 - p2), (p1 - p2));

		float uv1 = 0.1f;
		float uv2 = 0.9f;
		Vector2f tc1(uv1, uv1), tc2(uv1, uv2), tc3(uv2, uv2), tc4(uv2, uv1);

		uint32_t index = (uint32_t)positions.size();
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

		index = (uint32_t)positions.size();
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

	r->CreateVertexBuffer(positions.size(), positions.data(), normals.data(), texcoords.data());
	r->CreateIndexBuffer(indices.size(), indices.data());
}


void LoadAssimpStaticMesh(RenderEngine& re, std::string file_path)
{
	aiPropertyStore* props = aiCreatePropertyStore();
	aiSetImportPropertyInteger(props, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
	aiSetImportPropertyFloat(props, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80);
	aiSetImportPropertyInteger(props, AI_CONFIG_PP_SBP_REMOVE, 0);
	aiSetImportPropertyInteger(props, AI_CONFIG_GLOB_MEASURE_TIME, 1);

	unsigned int ppsteps = aiProcess_JoinIdenticalVertices // join identical vertices/ optimize indexing
		| aiProcess_ValidateDataStructure // perform a full validation of the loader's output
		| aiProcess_RemoveRedundantMaterials // remove redundant materials
		| aiProcess_FindInstances; // search for instanced meshes and remove them by references to one master

	aiScene const * scene = aiImportFileExWithProperties(file_path.c_str(),
		ppsteps // configurable pp steps
		| aiProcess_GenSmoothNormals // generate smooth normal vectors if not existing
		| aiProcess_Triangulate // triangulate polygons with more than 3 edges
		| aiProcess_ConvertToLeftHanded // convert everything to D3D left handed space
		| aiProcess_FixInfacingNormals, // find normals facing inwards and inverts them
		nullptr, props);

	auto pp = _FSPFX path(file_path).parent_path();

	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
	{
		aiMesh const * mesh = scene->mMeshes[mi];

		std::string tex_path;
		Vector3f ka;
		Vector3f kd;
		Vector3f ks;
		std::vector<Vector3f> pos_data;
		std::vector<Vector3f> norm_data;
		std::vector<Vector2f> tc_data;
		std::vector<uint32_t> indice_data;
		size_t num_vert = mesh->mNumVertices;

		auto mtl = scene->mMaterials[mesh->mMaterialIndex];

		unsigned int count = aiGetMaterialTextureCount(mtl, aiTextureType_DIFFUSE);
		if (count > 0)
		{
			aiString str;
			aiGetMaterialTexture(mtl, aiTextureType_DIFFUSE, 0, &str, 0, 0, 0, 0, 0, 0);
			pp.append("/").append(str.C_Str());
			tex_path = pp.string();
		}

		if (AI_SUCCESS != aiGetMaterialColor(mtl, "Ka", 0, 0, (aiColor4D*)&ka))
		{
			ka = Vector3f(0.2f, 0.2f, 0.2f);
		}
		if (AI_SUCCESS != aiGetMaterialColor(mtl, "Kd", 0, 0, (aiColor4D*)&kd))
		{
			kd = Vector3f(0.5f, 0.5f, 0.5f);
		}
		if (AI_SUCCESS != aiGetMaterialColor(mtl, "Ks", 0, 0, (aiColor4D*)&ks))
		{
			ks = Vector3f(0.7f, 0.7f, 0.7f);
		}

		for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi)
		{
			if (3 == mesh->mFaces[fi].mNumIndices)
			{
				indice_data.push_back(mesh->mFaces[fi].mIndices[0]);
				indice_data.push_back(mesh->mFaces[fi].mIndices[1]);
				indice_data.push_back(mesh->mFaces[fi].mIndices[2]);
			}
		}

		pos_data.resize(num_vert);
		norm_data.resize(num_vert);
		tc_data.resize(num_vert);
		for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi)
		{
			pos_data[vi] = Vector3f(&mesh->mVertices[vi].x);

			if (mesh->mNormals)
			{
				norm_data[vi] = Vector3f(&mesh->mNormals[vi].x);
			}

			if (mesh->mTextureCoords && mesh->mTextureCoords[0])
			{
				tc_data[vi] = Vector2f(&mesh->mTextureCoords[0][vi].x);
			}
		}

		StaticMeshPtr r = re.MakeObject<StaticMesh>();
		r->CreateVertexBuffer(num_vert, pos_data.data(), norm_data.data(), tc_data.data());
		r->CreateIndexBuffer(indice_data.size(), indice_data.data());
		r->CreateMaterial(tex_path, ka, kd, ks);
		re.AddRenderable(r);
	}

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

		re.LoadEffect("../../../Media/Effect/Shading.fx");

		CameraPtr cam = std::make_shared<Camera>();
		Vector3f eye(0, 2, -3), at(0, 0, 0), up(0, 1, 0);
		cam->LookAt(eye, at, up);
		cam->Perspective(XM_PI * 0.6f, (float)width / (float)height, 1, 500);
		re.SetCamera(cam);

		LoadAssimpStaticMesh(re, "../../../Media/Model/Cup/cup.obj");

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


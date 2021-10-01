#ifndef SAVEALLMESHES_CPP
#define SAVEALLMESHES_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/triangleMesh.h"
#include "Util/encodeBinDataToBase64.h"
#include "Util/encodeEigenMatrixToBase64.h"

#include <string>
#include <mutex>
#include <fstream>

#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

#include "igl/writeOBJ.h"
#include "igl/writePLY.h"
#include "igl/writeSTL.h"
#include "igl/writeWRL.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Util/stb_image.h"
#include "Util/stb_image_write.h"

namespace
{
	void writeTextureToFile(const fs::path &texPath, const Doppelganger::triangleMesh::Texture &texture)
	{
		const int width = texture.texData.cols();
		const int height = texture.texData.rows();
		const int channels = STBI_rgb_alpha;
		stbi_flip_vertically_on_write(true);

		std::vector<unsigned char> image(width * height * channels);
		union
		{
			uint32_t uint_;
			unsigned char uchar_[4];
		} u;
		for (int h = 0; h < height; ++h)
		{
			for (int w = 0; w < width; ++w)
			{
				u.uint_ = texture.texData(h, w);
				for (int rgba = 0; rgba < channels; ++rgba)
				{
					image.at((h * width + w) * channels + rgba) = u.uchar_[rgba];
				}
			}
		}

		if (texture.fileFormat == "jpg" || texture.fileFormat == "jpeg")
		{
			// currently, quality == 90
			stbi_write_jpg(texPath.string().c_str(), width, height, channels, &(image[0]), 90);
		}
		else if (texture.fileFormat == "png")
		{
			stbi_write_png(texPath.string().c_str(), width, height, channels, &(image[0]), sizeof(unsigned char) * width * channels);
		}
		else if (texture.fileFormat == "bmp")
		{
			stbi_write_bmp(texPath.string().c_str(), width, height, channels, &(image[0]));
		}
		else if (texture.fileFormat == "tga")
		{
			stbi_write_tga(texPath.string().c_str(), width, height, channels, &(image[0]));
		}
		// else if (texture.fileFormat == "hdr")
		// {
		// 	stbi_write_hdr(fileName.c_str(), width, height, channels, &(image[0]));
		// }
		else
		{
			// error
		}
	}
}

extern "C" DLLEXPORT void metadata(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	response["author"] = "Kazutaka Nakashima";
	response["version"] = "1.0.0";
	broadcast = nlohmann::json::object();
}

extern "C" DLLEXPORT void pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	////
	// [IN]
	// parameters = {
	// 	"format": "obj"|"ply"|"stl"|"wrl",
	// 	"meshes": [
	// 	 "UUID-A",
	// 	 "UUID-B",
	//   ...
	//  ]
	// }

	// [OUT]
	// <when core.config.at("output").at("type") == "download">
	// response = {
	// 	"meshes" : {
	// 	 "<meshUUID-A>" : {
	//    "mesh": {
	//     fileName: fileName,
	//     base64Str: base64-encoded data,
	//     format: "obj"|"ply"|"stl"|"wrl"
	//    },
	//    "material" (optional): {
	//     fileName: fileName,
	//     base64Str: base64-encoded data,
	//     format: "mtl"
	//    },
	//    "texture" (optional): {
	//     fileName: fileName,
	//     base64Str: base64-encoded data,
	//     format: "jpg"|"jpeg"|"png"|"bmp"|"tga"
	//    }
	//   }
	//  }
	// }
	// <when core.config.at("output").at("type") == "local">
	// response = {
	// }
	// broadcast = {
	// }

	// create empty response/broadcast
	response = nlohmann::json::object();
	response["meshes"] = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	const bool saveToLocal = (room->core->config.at("output").at("type").get<std::string>() == "local");
	const std::string &meshFormat = parameters.at("format").get<std::string>();

	{
		std::lock_guard<std::mutex> lock(room->mutexMeshes);

		for (const auto &UUID : parameters.at("meshes"))
		{
			const std::string &meshUUID = UUID.get<std::string>();
			const std::shared_ptr<Doppelganger::triangleMesh> &mesh = room->meshes.at(meshUUID);

			////
			// write file(s)
			// path to directory
			const fs::path directoryPath = (saveToLocal ? room->outputDir : fs::temp_directory_path());
			// path to mesh
			fs::path meshFilePath;
			{
				std::string meshFileName(mesh->name);
				meshFileName += ".";
				meshFileName += meshFormat;
				meshFilePath = directoryPath;
				meshFilePath.append(meshFileName);
			}
			// path to mtl (this is only used when writing to OBJ and the mesh has texture(s))
			fs::path mtlFilePath;
			if (meshFormat == "obj" && mesh->textures.size() > 0 && mesh->TC.rows() > 0)
			{
				std::string mtlFileName(mesh->name);
				mtlFileName += ".mtl";
				mtlFilePath = directoryPath;
				mtlFilePath.append(mtlFileName);
			}
			// path to texture (this is only used when the mesh has texture(s))
			fs::path textureFilePath;
			if (mesh->textures.size() > 0 && mesh->TC.rows() > 0)
			{
				std::string textureFileName(mesh->textures.at(0).fileName);
				textureFileName += ".";
				textureFileName += mesh->textures.at(0).fileFormat;
				textureFilePath = directoryPath;
				textureFilePath.append(textureFileName);
			}

			{
				if (meshFormat == "obj")
				{
					// write OBJ
					igl::writeOBJ(meshFilePath.string(), mesh->V, mesh->F, mesh->VC, mesh->VN, mesh->F, mesh->TC, mesh->FTC);

					if (!mtlFilePath.empty())
					{
						{
							// we explicitly append header if the mesh has valid texture
							std::vector<unsigned char> byteMesh;
							std::ifstream ifs(meshFilePath, std::ios::in | std::ios::binary);
							ifs.seekg(0, ifs.end);
							unsigned int length = static_cast<int>(ifs.tellg());
							std::stringstream objTexHeader;
							objTexHeader << "mtllib ";
							objTexHeader << mesh->name;
							objTexHeader << ".mtl\n";
							objTexHeader << "usemtl material0\n";

							byteMesh.resize(length + objTexHeader.str().length());
#if defined(_WIN32) || defined(_WIN64)
							sprintf_s(reinterpret_cast<char *>(&byteMesh[0]), byteMesh.size(), "%s", objTexHeader.str().c_str());
#elif defined(__APPLE__) || defined(__linux__)
							sprintf(reinterpret_cast<char *>(&byteMesh[0]), "%s", objTexHeader.str().c_str());
#endif
							ifs.seekg(0, ifs.beg);
							ifs.read(reinterpret_cast<char *>(&byteMesh[0] + objTexHeader.str().length()), length);
							ifs.close();

							std::ofstream ofs(meshFilePath, std::ios::out | std::ios::binary);
							ofs.write(reinterpret_cast<char *>(&byteMesh[0]), byteMesh.size());
							ofs.close();
						}

						// .mtl file
						{
							std::string mtl("newmtl material0\nKa 1.000000 1.000000 1.000000\nKd 1.000000 1.000000 1.000000\nKs 0.000000 0.000000 0.000000\nTr 1.000000\nillum 1\nNs 0.000000\nmap_Kd ");
							mtl += textureFilePath.filename().string();
							mtl += "\n";
							std::ofstream ofsMtl(mtlFilePath, std::ios::out | std::ios::binary);
							ofsMtl.write(mtl.c_str(), mtl.size());
							ofsMtl.close();
						}

						// texture file
						writeTextureToFile(textureFilePath, mesh->textures.at(0));
					}
				}
				else if (meshFormat == "ply")
				{
					Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> outVC255 = (mesh->VC * 255.0).array().round().template cast<unsigned char>();

					const bool convertFTCToTC = (mesh->F.rows() == mesh->FTC.rows());
					if (convertFTCToTC)
					{
						Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> outTC;
						// convert FTC+TC to outTC
						outTC.resize(mesh->V.rows(), 2);
						for (int f = 0; f < mesh->FTC.rows(); ++f)
						{
							for (int fv = 0; fv < mesh->FTC.cols(); ++fv)
							{
								outTC.row(mesh->F(f, fv)) = mesh->TC.row(mesh->FTC(f, fv));
							}
						}

						// possibly, we need to support meshlab-style properties??
						igl::writePLY(meshFilePath.string(), mesh->V, mesh->F, mesh->VN, outTC, outVC255, ((mesh->V.rows() == outVC255.rows()) ? std::vector<std::string>({"red", "green", "blue"}) : std::vector<std::string>()));
					}
					else
					{
						igl::writePLY(meshFilePath.string(), mesh->V, mesh->F, mesh->VN, mesh->TC, outVC255, ((mesh->V.rows() == outVC255.rows()) ? std::vector<std::string>({"red", "green", "blue"}) : std::vector<std::string>()));
					}

					// texture file
					if (!textureFilePath.empty())
					{
						writeTextureToFile(textureFilePath, mesh->textures.at(0));
					}
				}
				else if (meshFormat == "stl")
				{
					igl::writeSTL(meshFilePath.string(), mesh->V, mesh->F, mesh->FN);
				}
				else if (meshFormat == "wrl")
				{
					if (mesh->VC.rows() == mesh->V.rows())
					{
						igl::writeWRL(meshFilePath.string(), mesh->V, mesh->F, mesh->VC);
					}
					else
					{
						igl::writeWRL(meshFilePath.string(), mesh->V, mesh->F);
					}
				}
			}

			if (saveToLocal)
			{
				// open a directory that containing filePath
				// open output directory.
				std::stringstream cmd;
#if defined(_WIN32) || defined(_WIN64)
				cmd << "start \"\" \"";
#elif defined(__APPLE__)
				cmd << "open \"";
#endif
				cmd << room->outputDir.string();
				cmd << "\"";
				system(cmd.str().c_str());
			}
			else
			{
				////
				// load temporary file(s) to memory
				std::vector<unsigned char> byteMesh, byteMtl, byteTex;
				const auto loadFileIntoBin = [](const fs::path &path, std::vector<unsigned char> &bytes)
				{
					if (fs::exists(path))
					{
						std::ifstream ifs(path, std::ios::in | std::ios::binary);
						ifs.seekg(0, ifs.end);
						const unsigned int length = static_cast<unsigned int>(ifs.tellg());
						bytes.resize(length);

						ifs.seekg(0, ifs.beg);
						ifs.read(reinterpret_cast<char *>(&bytes[0]), length);
						ifs.close();
						fs::remove(path);
					}
				};
				// mesh/mtl/texture
				loadFileIntoBin(meshFilePath, byteMesh);
				loadFileIntoBin(mtlFilePath, byteMtl);
				loadFileIntoBin(textureFilePath, byteTex);

				const auto generateJsonObj = [](const std::string &fileName, const std::vector<unsigned char> &bytes, const std::string &fileFormat)
				{
					nlohmann::json json = nlohmann::json::object();
					json["fileName"] = fileName;
					json["base64Str"] = Doppelganger::Util::encodeBinDataToBase64(bytes);
					json["format"] = fileFormat;
					return json;
				};
				nlohmann::json meshJson = nlohmann::json::object();
				{
					meshJson["mesh"] = generateJsonObj(mesh->name, byteMesh, meshFormat);
					if (byteMtl.size() > 0)
					{
						meshJson["material"] = generateJsonObj(mtlFilePath.filename().stem().string(), byteMtl, std::string("mtl"));
					}
					if (byteTex.size() > 0)
					{
						meshJson["texture"] = generateJsonObj(mesh->textures.at(0).fileName, byteTex, mesh->textures.at(0).fileFormat);
					}
				}

				response["meshes"][meshUUID] = meshJson;
			}
		}
	}
}

#endif

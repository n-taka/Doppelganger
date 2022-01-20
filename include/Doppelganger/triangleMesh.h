#ifndef TRIANGLEMESH_H
#define TRIANGLEMESH_H

#if defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define DECLSPEC
#elif defined(__linux__)
#define DECLSPEC
#endif

#include "Eigen/Core"

#include <boost/any.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace Doppelganger
{
	class DECLSPEC triangleMesh
	{
	public:
		// constructors/destructors
		triangleMesh(const std::string &UUID_)
			: UUID(UUID_)
		{
			visibility = true;
			RGBA.resize(1, 4);
			RGBA(0, 0) = static_cast<double>(241.0 / 255.0);
			RGBA(0, 1) = static_cast<double>(140.0 / 255.0);
			RGBA(0, 2) = static_cast<double>(69.0 / 255.0);
			RGBA(0, 3) = static_cast<double>(1.0);
		};
		triangleMesh() : triangleMesh(std::string("")){};
		~triangleMesh(){};

		// dump to json
		nlohmann::json dumpToJson(const bool &sendToClient) const;
		void restoreFromJson(const nlohmann::json &json);
		void projectMeshAttributes(const std::shared_ptr<triangleMesh> &source);

	public:
		std::string name;
		const std::string UUID;
		bool visibility;
		// geometry
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> V;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> F;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VN;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> FN;
		// face group
		Eigen::Matrix<int, Eigen::Dynamic, 1> FG;
		// color/texture
		// In this library, RGB value is always [0.0, 1.0].
		Eigen::Matrix<double, 1, Eigen::Dynamic> RGBA;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VC;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> FC;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> TC;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> FTC;
		std::vector<std::string> mtlFileName;
		typedef struct Texture_
		{
			std::string fileName;
			std::string fileFormat;
			Eigen::Matrix<uint32_t, Eigen::Dynamic, Eigen::Dynamic> texData;
		} Texture;
		std::vector<Texture> textures;
		// Halfedge data structure
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> TT;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> TTi;
		std::vector<std::vector<int>> VF;
		std::vector<std::vector<int>> VFi;

		std::unordered_map<std::string, boost::any> customData;
	};
}

#endif

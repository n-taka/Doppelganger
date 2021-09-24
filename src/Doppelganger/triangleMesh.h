#ifndef TRIANGLEMESH_H
#define TRIANGLEMESH_H

#include "Eigen/Core"
#include "igl/AABB.h"

#include <boost/any.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace Doppelganger
{
	class triangleMesh
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
		~triangleMesh(){};

		// dump to json
		nlohmann::json dumpToJson(const bool &sendToClient) const;
		void restoreFromJson(const nlohmann::json& json);

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
		typedef struct Texture_ {
			std::string fileName;
			Eigen::Matrix<uint32_t, Eigen::Dynamic, Eigen::Dynamic> texData;
		} Texture;
		std::vector<Texture> textures;
		// Halfedge data structure
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> TT;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> TTi;
		std::vector<std::vector<int>> VF;
		std::vector<std::vector<int>> VFi;
		// AABB
		igl::AABB<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>, 3> AABB;
		// PCA
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> principalComponent;
		// Curvature
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> PD1;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> PD2;
		Eigen::Matrix<double, Eigen::Dynamic, 1> PV1;
		Eigen::Matrix<double, Eigen::Dynamic, 1> PV2;
		Eigen::Matrix<double, Eigen::Dynamic, 1> K;

		std::unordered_map<std::string, boost::any> customData;
	};
}

#endif

#ifndef TRIANGLEMESH_H
#define TRIANGLEMESH_H

#include <Eigen/Core>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace Doppelganger
{
	class TriangleMesh
	{
	public:
		// constructor
		TriangleMesh();

		void projectMeshAttributes(const std::shared_ptr<TriangleMesh> &source);

	public:
		std::string name_;
		std::string UUID_;
		bool visibility_;
		// geometry
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> V_;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> F_;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VN_;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> FN_;
		// face group
		Eigen::Matrix<int, Eigen::Dynamic, 1> FG_;
		// color/texture
		// In this library, RGB value is always [0.0, 1.0].
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VC_;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> FC_;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> TC_;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> FTC_;
		std::vector<std::string> mtlFileName_;
		struct Texture
		{
			std::string fileName_;
			std::string fileFormat_;
			Eigen::Matrix<uint32_t, Eigen::Dynamic, Eigen::Dynamic> texData_;
		};
		std::vector<Texture> textures_;
		// Halfedge data structure
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> TT_;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> TTi_;
		std::vector<std::vector<int>> VF_;
		std::vector<std::vector<int>> VFi_;

		nlohmann::json extension_;
	};

	////
	// nlohmann::json conversion
	////
	void to_json(nlohmann::json &json, const TriangleMesh &mesh);
	void to_json(nlohmann::json &json, const TriangleMesh &mesh, const bool toClient);
	void from_json(const nlohmann::json &json, TriangleMesh &mesh);
}

////
// json format for Doppelganger::TrinagleMesh
////
// {
//   "name": name of this mesh,
//   "UUID": UUID of this mesh,
//   "visibility": visibility of this mesh,
//   "V": base64-encoded vertices (#V),
//   "F": base64-encoded facets (#F),
//   "VC": base64-encoded vertex colors (#V),
//   "FC": base64-encoded vertices (#F, only for edit history),
//   "TC": base64-encoded texture coordinates (#V),
//   "FTC": base64-encoded vertices (#F, only for edit history),
//   "textures": [
//     {
// 	     "name": original filename for this texture
// 	     "fileFormat": original file format for this texture
// 	     "width" = width of this texture
// 	     "height" = height of this texture
// 	     "texData" = base64-encoded texture data
//     },
//     ...
//   ],
//   "extension": json for extension (used by plugins)
// }

#endif

#ifndef TRIANGLEMESH_CPP
#define TRIANGLEMESH_CPP

#include "Doppelganger/TriangleMesh.h"

#include "igl/barycenter.h"
#include "igl/signed_distance.h"
#include "igl/barycentric_coordinates.h"
#include "igl/per_face_normals.h"
#include "igl/barycentric_interpolation.h"
#include "igl/per_vertex_normals.h"
#include "igl/triangle_triangle_adjacency.h"
#include "igl/vertex_triangle_adjacency.h"

#include "Doppelganger/Util/encodeEigenMatrixToBase64.h"
#include "Doppelganger/Util/decodeBase64ToEigenMatrix.h"

namespace Doppelganger
{
	// constructor
	TriangleMesh::TriangleMesh()
	{
		visibility_ = true;
		extension_ = nlohmann::json::object();
	}

	void to_json(nlohmann::json &json, const TriangleMesh &mesh)
	{
		to_json(json, mesh, false);
	}
	void to_json(nlohmann::json &json, const TriangleMesh &mesh, const bool toClient)
	{
		// * For the JSON that will be sent to clients, we duplicate triangles for face color or face texture
		//   because threejs only supports vertex attributes for rendering
		// * In addition, for better network performance, we use float for JSON will be sent to clients

		json = nlohmann::json::object();
		// meta info
		json["name"] = mesh.name_;
		json["UUID"] = mesh.UUID_;
		json["visibility"] = mesh.visibility_;

		// geometry
		const bool validFTC = (mesh.F_.rows() == mesh.FTC_.rows());
		const bool validFC = (mesh.F_.rows() == mesh.FC_.rows());
		const bool needDuplication = ((validFTC || validFC) && toClient);
		if (needDuplication)
		{
			assert((mesh.F_.rows() == mesh.FTC_.rows() || mesh.F_.rows() == mesh.FC_.rows()) && "#F and (#FTC | #FC) must be the same");

			// convert facet attributes to vertex attributes
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VDup;
			Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> FDup;
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VCDup;
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> TCDup;

			VDup.resize(mesh.F_.size(), 3);
			FDup.resizeLike(mesh.F_);
			if (validFTC)
			{
				TCDup.resize(mesh.F_.size(), 2);
			}
			if (validFC)
			{
				VCDup.resize(mesh.F_.size(), 3);
			}

			for (int f = 0; f < mesh.F_.rows(); ++f)
			{
				for (int fv = 0; fv < mesh.F_.cols(); ++fv)
				{
					const int vIdx = f * static_cast<int>(mesh.F_.cols()) + fv;
					VDup.row(vIdx) = mesh.V_.row(mesh.F_(f, fv));
					FDup(f, fv) = vIdx;

					if (validFTC)
					{
						TCDup.row(vIdx) = mesh.TC_.row(mesh.FTC_(f, fv));
					}
					if (validFC)
					{
						VCDup.row(vIdx) = mesh.FC_.row(f);
					}
				}
			}

			writeToJson(VDup, FDup, VCDup, mesh.FC_, TCDup, mesh.FTC_, toClient, json);
		}
		else
		{
			writeToJson(mesh.V_, mesh.F_, mesh.VC_, mesh.FC_, mesh.TC_, mesh.FTC_, toClient, json);
		}

		// textures
		json["textures"] = nlohmann::json::array();
		for (const auto &texture : mesh.textures_)
		{
			nlohmann::json texJson = nlohmann::json::object();
			texJson["name"] = texture.fileName_;
			texJson["fileFormat"] = texture.fileFormat_;
			texJson["width"] = texture.texData_.cols();
			texJson["height"] = texture.texData_.rows();
			texJson["texData"] = Doppelganger::Util::encodeEigenMatrixToBase64(texture.texData_);
			json.at("textures").push_back(texJson);
		}

		// extension
		json["extension"] = mesh.extension_;
	}

	void from_json(const nlohmann::json &json, TriangleMesh &mesh)
	{
		// meta info
		if (json.contains("name"))
		{
			mesh.name_ = json.at("name").get<std::string>();
		}
		if (json.contains("UUID"))
		{
			mesh.UUID_ = json.at("UUID").get<std::string>();
		}
		if (json.contains("visibility"))
		{
			mesh.visibility_ = json.at("visibility").get<bool>();
		}

		// geometry
		if (json.contains("V"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("V").get<std::string>(), 3, mesh.V_);
		}
		if (json.contains("F"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("F").get<std::string>(), 3, mesh.F_);
		}
		if (json.contains("VC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("VC").get<std::string>(), 3, mesh.VC_);
		}
		if (json.contains("TC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("TC").get<std::string>(), 2, mesh.TC_);
		}
		if (json.contains("FC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("FC").get<std::string>(), 3, mesh.FC_);
		}
		if (json.contains("FTC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("FTC").get<std::string>(), 3, mesh.FTC_);
		}

		// textures
		if (json.contains("textures"))
		{
			const nlohmann::json &texturesArray = json.at("textures");
			mesh.textures_.resize(texturesArray.size());
			for (int texIdx = 0; texIdx < texturesArray.size(); ++texIdx)
			{
				const nlohmann::json &textureJson = texturesArray.at(texIdx);
				Doppelganger::TriangleMesh::Texture &texture = mesh.textures_.at(texIdx);

				texture.fileName_ = textureJson.at("name").get<std::string>();
				texture.fileFormat_ = textureJson.at("fileFormat").get<std::string>();
				texture.texData_.resize(textureJson.at("height").get<int>(), textureJson.at("width").get<int>());
				Doppelganger::Util::decodeBase64ToEigenMatrix(textureJson.at("texData").get<std::string>(), texture.texData_.cols(), texture.texData_);
			}
		}

		// extension
		if(json.contains("extension"))
		{
			mesh.extension_ = json.at("extension");
		}
	}

	void TriangleMesh::projectMeshAttributes(const std::shared_ptr<TriangleMesh> &source)
	{
		// we assume that two meshes have (almost) identical shape
		// [vertex attributes]
		// - vertex color
		// - vertex normal (re-calculate)
		// - texture coordinates
		// [face attributes]
		// - face color
		// - face normal (re-calculate)
		// - face texture coordinates
		// - face group
		// [misc]
		// - halfedge data structure

		{
			// face/vertex correspondence
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> q;
			{
				q.resize(V_.rows() + F_.rows(), V_.cols());
				q.block(0, 0, V_.rows(), V_.cols()) = V_;
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> BC;
				igl::barycenter(V_, F_, BC);
				q.block(V_.rows(), 0, F_.rows(), V_.cols()) = BC;
			}

			Eigen::Matrix<int, Eigen::Dynamic, 1> vertCorrespondingFace;
			Eigen::Matrix<int, Eigen::Dynamic, 1> faceCorrespondingFace;
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> vertPointsOnMesh;
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> facePointsOnMesh;
			{
				Eigen::Matrix<int, Eigen::Dynamic, 1> correspondingFace;
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> pointsOnMesh;
				Eigen::Matrix<double, Eigen::Dynamic, 1> S;
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> N;
				igl::signed_distance(q, source->V_, source->F_, igl::SIGNED_DISTANCE_TYPE_FAST_WINDING_NUMBER, S, correspondingFace, pointsOnMesh, N);
				vertCorrespondingFace = correspondingFace.block(0, 0, V_.rows(), 1);
				faceCorrespondingFace = correspondingFace.block(V_.rows(), 0, F_.rows(), 1);
				vertPointsOnMesh = pointsOnMesh.block(0, 0, V_.rows(), V_.cols());
				facePointsOnMesh = pointsOnMesh.block(V_.rows(), 0, F_.rows(), V_.cols());
			}

			// barycentric coordinate (only for vertices)
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> L;
			{
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> A, B, C;
				A.resize(vertPointsOnMesh.rows(), 3);
				B.resize(vertPointsOnMesh.rows(), 3);
				C.resize(vertPointsOnMesh.rows(), 3);
				for (int vIdx = 0; vIdx < vertPointsOnMesh.rows(); ++vIdx)
				{
					const int &vertCorrespondingFaceIdx = vertCorrespondingFace(vIdx, 0);
					A.row(vIdx) = source->V_.row(source->F_(vertCorrespondingFaceIdx, 0));
					B.row(vIdx) = source->V_.row(source->F_(vertCorrespondingFaceIdx, 1));
					C.row(vIdx) = source->V_.row(source->F_(vertCorrespondingFaceIdx, 2));
				}
				igl::barycentric_coordinates(vertPointsOnMesh, A, B, C, L);
			}

			// face attributes
			{
				// face color (FC)
				if (source->FC_.rows() == source->F_.rows())
				{
					FC_.resize(F_.rows(), source->FC_.cols());
					for (int f = 0; f < F_.rows(); ++f)
					{
						FC_.row(f) = source->FC_.row(faceCorrespondingFace(f, 0));
					}
				}

				// face normal (FN, re-calculate)
				igl::per_face_normals(V_, F_, FN_);

				// face group (FG)
				if (source->FG_.rows() == source->F_.rows())
				{
					FG_.resize(F_.rows(), 1);
					for (int f = 0; f < F_.rows(); ++f)
					{
						FG_(f, 0) = source->FG_(faceCorrespondingFace(f, 0), 0);
					}
				}
			}

			// vertex attributes
			{
				// vertex color (VC)
				if (source->VC_.rows() == source->V_.rows())
				{
					igl::barycentric_interpolation(source->VC_, source->F_, L, vertCorrespondingFace, VC_);
				}

				// vertex normal (VN, re-calculate)
				igl::per_vertex_normals(V_, F_, FN_, VN_);
			}

			// texture
			{
				if (source->TC_.rows() == source->V_.rows())
				{
					igl::barycentric_interpolation(source->TC_, source->F_, L, vertCorrespondingFace, TC_);
				}
				// currently, we don't support FTC...
			}

			// misc
			{
				igl::triangle_triangle_adjacency(F_, TT_, TTi_);
				igl::vertex_triangle_adjacency(V_, F_, VF_, VFi_);
			}
		}
	}

}

namespace
{
	template <typename DerivedV, typename DerivedF, typename DerivedVC, typename DerivedFC, typename DerivedTC, typename DerivedFTC>
	void writeToJson(
		const Eigen::MatrixBase<DerivedV> &Vertices,
		const Eigen::MatrixBase<DerivedF> &Facets,
		const Eigen::MatrixBase<DerivedVC> &VertexColors,
		const Eigen::MatrixBase<DerivedFC> &FacetColors,
		const Eigen::MatrixBase<DerivedTC> &TexCoords,
		const Eigen::MatrixBase<DerivedFTC> &FacetTexCoords,
		const bool &toClient,
		nlohmann::json &json)
	{
		if (toClient)
		{
			Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> Vf;
			Eigen::Matrix<unsigned int, Eigen::Dynamic, Eigen::Dynamic> Fuint;
			Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> VCf;
			Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> TCf;
			Vf = Vertices.template cast<float>();
			Fuint = Facets.template cast<unsigned int>();
			VCf = VertexColors.template cast<float>();
			TCf = TexCoords.template cast<float>();

			// vertex
			json["V"] = Doppelganger::Util::encodeEigenMatrixToBase64(Vf);
			// face
			json["F"] = Doppelganger::Util::encodeEigenMatrixToBase64(Fuint);
			// color
			json["VC"] = Doppelganger::Util::encodeEigenMatrixToBase64(VCf);
			// uv
			json["TC"] = Doppelganger::Util::encodeEigenMatrixToBase64(TCf);
		}
		else
		{
			// vertex
			json["V"] = Doppelganger::Util::encodeEigenMatrixToBase64(Vertices);
			// face
			json["F"] = Doppelganger::Util::encodeEigenMatrixToBase64(Facets);
			// color
			json["VC"] = Doppelganger::Util::encodeEigenMatrixToBase64(VertexColors);
			// uv
			json["TC"] = Doppelganger::Util::encodeEigenMatrixToBase64(TexCoords);
			// for correctly store edit history
			// face color
			json["FC"] = Doppelganger::Util::encodeEigenMatrixToBase64(FacetColors);
			json["FTC"] = Doppelganger::Util::encodeEigenMatrixToBase64(FacetTexCoords);
		}
	};
}

#endif

#ifndef TRIANGLEMESH_CPP
#define TRIANGLEMESH_CPP

#include "Doppelganger/triangleMesh.h"
#include "Util/encodeEigenMatrixToBase64.h"
#include "Util/decodeBase64ToEigenMatrix.h"

#include "igl/barycenter.h"
#include "igl/signed_distance.h"
#include "igl/barycentric_coordinates.h"
#include "igl/per_face_normals.h"
#include "igl/barycentric_interpolation.h"
#include "igl/per_vertex_normals.h"
#include "igl/triangle_triangle_adjacency.h"
#include "igl/vertex_triangle_adjacency.h"

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

namespace Doppelganger
{
	nlohmann::json triangleMesh::dumpToJson(const bool &sendToClient) const
	{
		////
		// [IN]
		// sendToClient: boolean flag that this JSON will be sent to clients
		//   * For the JSON that will be sent to clients, we duplicate triangles for face color or face texture
		//     because threejs only supports vertex attributes for rendering
		//   * In addition, for better network performance, we use float for JSON will be sent to clients

		// [OUT]
		// <return valule> = {
		//  "UUID": UUID of this mesh,
		//  "name": name of this mesh,
		//  "visibility": visibility of this mesh,
		//  "V": base64-encoded vertices (#V),
		//  "F": base64-encoded facets (#F),
		//  "VC": base64-encoded vertex colors (#V),
		//  "TC": base64-encoded texture coordinates (#V),
		//  "FC": base64-encoded vertices (#F, only for edit history),
		//  "FTC": base64-encoded vertices (#F, only for edit history),
		//  "textures": [
		//   {
		// 	  "name": original filename for this texture
		// 	  "fileFormat": original file format for this texture
		// 	  "width" = width of this texture
		// 	  "height" = height of this texture
		// 	  "texData" = base64-encoded texture data
		//   }
		//  ]
		// }

		nlohmann::json response = nlohmann::json::object();
		response["UUID"] = UUID;
		response["name"] = name;
		response["visibility"] = visibility;

		const bool validFTC = (F.rows() == FTC.rows());
		const bool validFC = (F.rows() == FC.rows());
		const bool needDuplication = ((validFTC || validFC) && sendToClient);

		if (needDuplication)
		{
			assert((F.rows() == FTC.rows() || F.rows() == FC.rows()) && "#F and (#FTC | #FC) must be the same");

			// convert facet attributes to vertex attributes
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VDup;
			Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> FDup;
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VCDup;
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> TCDup;

			VDup.resize(F.rows() * F.cols(), 3);
			FDup.resize(F.rows(), F.cols());
			if (validFTC)
			{
				TCDup.resize(F.rows() * F.cols(), 2);
			}
			if (validFC)
			{
				VCDup.resize(F.rows() * F.cols(), 3);
			}

			for (int f = 0; f < F.rows(); ++f)
			{
				for (int fv = 0; fv < F.cols(); ++fv)
				{
					const int vIdx = f * static_cast<int>(F.cols()) + fv;
					VDup.row(vIdx) = V.row(F(f, fv));
					FDup(f, fv) = vIdx;

					if (validFTC)
					{
						TCDup.row(vIdx) = TC.row(FTC(f, fv));
					}
					if (validFC)
					{
						VCDup.row(vIdx) = FC.row(f);
					}
				}
			}

			writeToJson(VDup, FDup, VCDup, FC, TCDup, FTC, sendToClient, response);
		}
		else
		{
			writeToJson(V, F, VC, FC, TC, FTC, sendToClient, response);
		}

		// textures
		response["textures"] = nlohmann::json::array();
		for (const auto &texture : textures)
		{
			nlohmann::json texJson = nlohmann::json::object();
			texJson["name"] = texture.fileName;
			texJson["fileFormat"] = texture.fileFormat;
			texJson["width"] = texture.texData.cols();
			texJson["height"] = texture.texData.rows();
			texJson["texData"] = Doppelganger::Util::encodeEigenMatrixToBase64(texture.texData);
			response.at("textures").push_back(texJson);
		}
		return response;
	}

	void triangleMesh::restoreFromJson(const nlohmann::json &json)
	{
		// [IN]
		// json = {
		//  "UUID": UUID of this mesh,
		//  "name": name of this mesh,
		//  "visibility": visibility of this mesh,
		//  "V": base64-encoded vertices (#V),
		//  "F": base64-encoded facets (#F),
		//  "VC": base64-encoded vertex colors (#V),
		//  "TC": base64-encoded texture coordinates (#V),
		//  "FC": base64-encoded vertices (#F, only for edit history),
		//  "FTC": base64-encoded vertices (#F, only for edit history),
		//  "textures": [
		//   {
		// 	  "name": original filename for this texture
		// 	  "width" = width of this texture
		// 	  "height" = height of this texture
		// 	  "texData" = base64-encoded texture data
		//   }
		//  ]
		// }

		// [OUT]
		// nothing.

		// UUID
		if (UUID != json.at("UUID").get<std::string>())
		{
			// invalid.
			return;
		}
		// name
		if (json.contains("name"))
		{
			name = json.at("name").get<std::string>();
		}
		// visibility
		if (json.contains("visibility"))
		{
			visibility = json.at("visibility").get<bool>();
		}
		// V
		if (json.contains("V"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("V").get<std::string>(), 3, V);
		}
		// F
		if (json.contains("F"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("F").get<std::string>(), 3, F);
		}
		// VC
		if (json.contains("VC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("VC").get<std::string>(), 3, VC);
		}
		// TC
		if (json.contains("TC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("TC").get<std::string>(), 2, TC);
		}
		// FC
		if (json.contains("FC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("FC").get<std::string>(), 3, FC);
		}
		// FTC
		if (json.contains("FTC"))
		{
			Doppelganger::Util::decodeBase64ToEigenMatrix(json.at("FTC").get<std::string>(), 3, FTC);
		}
		// textures
		if (json.contains("textures"))
		{
			const nlohmann::json &texturesArray = json.at("textures");
			textures.resize(texturesArray.size());
			for (int texIdx = 0; texIdx < texturesArray.size(); ++texIdx)
			{
				const nlohmann::json &textureJson = texturesArray.at(texIdx);
				Texture &texture = textures.at(texIdx);

				texture.fileName = textureJson.at("name").get<std::string>();
				texture.fileFormat = textureJson.at("fileFormat").get<std::string>();
				texture.texData.resize(textureJson.at("height").get<int>(), textureJson.at("width").get<int>());
				Doppelganger::Util::decodeBase64ToEigenMatrix(textureJson.at("texData").get<std::string>(), texture.texData.cols(), texture.texData);
			}
		}
	}

	void triangleMesh::projectMeshAttributes(const std::shared_ptr<triangleMesh> &source)
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
		// - AABB

		{
			// face/vertex correspondence
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> q;
			{
				q.resize(V.rows() + F.rows(), V.cols());
				q.block(0, 0, V.rows(), V.cols()) = V;
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> BC;
				igl::barycenter(V, F, BC);
				q.block(V.rows(), 0, F.rows(), V.cols()) = BC;
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
				igl::signed_distance(q, source->V, source->F, igl::SIGNED_DISTANCE_TYPE_FAST_WINDING_NUMBER, S, correspondingFace, pointsOnMesh, N);
				vertCorrespondingFace = correspondingFace.block(0, 0, V.rows(), 1);
				faceCorrespondingFace = correspondingFace.block(V.rows(), 0, F.rows(), 1);
				vertPointsOnMesh = pointsOnMesh.block(0, 0, V.rows(), V.cols());
				facePointsOnMesh = pointsOnMesh.block(V.rows(), 0, F.rows(), V.cols());
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
					A.row(vIdx) = source->V.row(source->F(vertCorrespondingFaceIdx, 0));
					B.row(vIdx) = source->V.row(source->F(vertCorrespondingFaceIdx, 1));
					C.row(vIdx) = source->V.row(source->F(vertCorrespondingFaceIdx, 2));
				}
				igl::barycentric_coordinates(vertPointsOnMesh, A, B, C, L);
			}

			// face attributes
			{
				// face color (FC)
				if (source->FC.rows() == source->F.rows())
				{
					FC.resize(F.rows(), source->FC.cols());
					for (int f = 0; f < F.rows(); ++f)
					{
						FC.row(f) = source->FC.row(faceCorrespondingFace(f, 0));
					}
				}

				// face normal (FN, re-calculate)
				igl::per_face_normals(V, F, FN);

				// face group (FG)
				if (source->FG.rows() == source->F.rows())
				{
					FG.resize(F.rows(), 1);
					for (int f = 0; f < F.rows(); ++f)
					{
						FG(f, 0) = source->FG(faceCorrespondingFace(f, 0), 0);
					}
				}
			}

			// vertex attributes
			{
				// vertex color (VC)
				if (source->VC.rows() == source->V.rows())
				{
					igl::barycentric_interpolation(source->VC, source->F, L, vertCorrespondingFace, VC);
				}

				// vertex normal (VN, re-calculate)
				igl::per_vertex_normals(V, F, FN, VN);
			}

			// texture
			{
				// face texture coordinates (FTC)
				// this implementation is not perfect ...
				if (source->FTC.rows() == source->F.rows())
				{
					igl::barycentric_interpolation(source->TC, source->F, L, vertCorrespondingFace, TC);
					FTC = F;
				}
				else if (source->TC.rows() == source->V.rows())
				{
					igl::barycentric_interpolation(source->TC, source->F, L, vertCorrespondingFace, TC);
				}
			}

			// misc
			{
				igl::triangle_triangle_adjacency(F, TT, TTi);
				igl::vertex_triangle_adjacency(V, F, VF, VFi);
				AABB.init(V, F);
				principalComponent.resize(0, V.cols());
				PD1.resize(0, V.cols());
				PD2.resize(0, V.cols());
				PV1.resize(0, 1);
				PV2.resize(0, 1);
				K.resize(0, 1);
			}
		}
	}

}

#endif

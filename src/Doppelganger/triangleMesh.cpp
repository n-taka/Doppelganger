#ifndef TRIANGLEMESH_CPP
#define TRIANGLEMESH_CPP

#include "Doppelganger/triangleMesh.h"

#include <boost/beast/core/detail/base64.hpp>

namespace
{
	template <typename Derived>
	std::string encodeEigenMatrixToBase64(const Eigen::MatrixBase<Derived> &mat)
	{
		// enforce RowMajor
		const Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime, Derived::ColsAtCompileTime, Eigen::RowMajor> matRowMajor = mat;
		const size_t binDataBytes = matRowMajor.size() * sizeof(typename Derived::Scalar);
		const size_t len = boost::beast::detail::base64::encoded_size(binDataBytes);
		std::vector<unsigned char> destVec;
		destVec.resize(len);
		const size_t lenWritten = boost::beast::detail::base64::encode(&(destVec[0]), matRowMajor.data(), binDataBytes);
		return std::string(destVec.begin(), destVec.begin() + lenWritten);
	}

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
			json["V"] = encodeEigenMatrixToBase64(Vf);
			// face
			json["F"] = encodeEigenMatrixToBase64(Fuint);
			// color
			json["VC"] = encodeEigenMatrixToBase64(VCf);
			// uv
			json["TC"] = encodeEigenMatrixToBase64(TCf);
		}
		else
		{
			// vertex
			json["V"] = encodeEigenMatrixToBase64(Vertices);
			// face
			json["F"] = encodeEigenMatrixToBase64(Facets);
			// color
			json["VC"] = encodeEigenMatrixToBase64(VertexColors);
			// uv
			json["TC"] = encodeEigenMatrixToBase64(TexCoords);
			// for correctly store edit history
			// face color
			json["FC"] = encodeEigenMatrixToBase64(FacetColors);
			json["FTC"] = encodeEigenMatrixToBase64(FacetTexCoords);
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

			writeToJson(VDup, FDup, VCDup, FC, TC, FTC, sendToClient, response);
		}
		else
		{
			writeToJson(V, F, VC, FC, TC, FTC, sendToClient, response);
		}

		// textures
		if (textures.size() > 0)
		{
			response["textures"] = nlohmann::json::array();
			for (const auto &texture : textures)
			{
				nlohmann::json texJson = nlohmann::json::object();
				texJson["name"] = texture.fileName;
				texJson["width"] = texture.texData.cols();
				texJson["height"] = texture.texData.rows();
				texJson["texData"] = encodeEigenMatrixToBase64(texture.texData);
				response.at("textures").push_back(texJson);
			}
		}
		return response;
	}
}

#endif

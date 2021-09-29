#ifndef HOLLOWMESH_CPP
#define HOLLOWMESH_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/triangleMesh.h"

#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "igl/grid.h"
#include "igl/signed_distance.h"
#include "igl/copyleft/marching_cubes.h"

#include "igl/per_face_normals.h"
#include "igl/per_vertex_normals.h"
#include "igl/facet_components.h"
#include "igl/remove_unreferenced.h"
#include "igl/triangle_triangle_adjacency.h"
#include "igl/vertex_triangle_adjacency.h"

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
	// 	"meshes": [
	// 	 "UUID-A",
	// 	 "UUID-B",
	//   ...
	//  ],
	//  "oneVoxelSize": double for size of a voxel,
	//  "shellThickness": double for shell thickness,
	//  "largestVoidOnly": boolean flag
	// }

	// [OUT]
	// response = {
	// }
	// broadcast = {
	// 	"meshes" : {
	//    "<meshUUID>": JSON object that represents the loaded mesh
	//  }
	// }

	// create empty response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();
	nlohmann::json diff = nlohmann::json::object();
	nlohmann::json diffInv = nlohmann::json::object();

	broadcast["meshes"] = nlohmann::json::object();
	diff["meshes"] = nlohmann::json::object();
	diffInv["meshes"] = nlohmann::json::object();
	{
		std::lock_guard<std::mutex> lock(room->mutexMeshes);

		const double &oneVoxelSize = parameters.at("oneVoxelSize").get<double>();
		const double &shellThickness = parameters.at("shellThickness").get<double>();
		const bool &largestVoidOnly = parameters.at("largestVoidOnly").get<bool>();

		// remove mesh
		for (const auto &UUID : parameters.at("meshes"))
		{
			const std::string &meshUUID = UUID.get<std::string>();
			const std::shared_ptr<Doppelganger::triangleMesh> &mesh = room->meshes.at(meshUUID);
			// store current mesh
			diffInv.at("meshes")[meshUUID] = mesh->dumpToJson(false);
			diffInv.at("meshes").at(meshUUID)["remove"] = false;

			Eigen::Matrix<double, 1, Eigen::Dynamic> BBmin, BBmax, BB, BBcenter;
			{
				// calculate BB
				BBmin = mesh->V.colwise().minCoeff();
				BBmax = mesh->V.colwise().maxCoeff();
				BB = BBmax - BBmin;
				BBcenter = (BBmax + BBmin) * 0.5;
			}

			Eigen::Matrix<int, 1, Eigen::Dynamic> res;
			{
				// calculate grid size
				res = (BB / oneVoxelSize).array().ceil().matrix().template cast<int>();
				// margin 2 voxels for each side
				res(0, 0) += 4;
				res(0, 1) += 4;
				res(0, 2) += 4;
			}

			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> GV;
			{
				// prepare grid
				igl::grid(res, GV);
				GV.col(0) *= (res(0, 0) * oneVoxelSize);
				GV.col(1) *= (res(0, 1) * oneVoxelSize);
				GV.col(2) *= (res(0, 2) * oneVoxelSize);
				// translate grid
				Eigen::Matrix<double, 1, Eigen::Dynamic> trans;
				trans = BBcenter - ((GV.row(0) + GV.row(GV.rows() - 1)) * 0.5);
				GV.rowwise() += trans;
			}

			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> SV;
			Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> SF;
			{
				// marching cubes
				Eigen::Matrix<double, Eigen::Dynamic, 1> S;
				Eigen::Matrix<int, Eigen::Dynamic, 1> I;
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> C, N;
				igl::signed_distance(GV, mesh->V, mesh->F, igl::SIGNED_DISTANCE_TYPE_FAST_WINDING_NUMBER, S, I, C, N);
				S.array() += shellThickness;
				igl::copyleft::marching_cubes(S, GV, res(0), res(1), res(2), SV, SF);

				// flip internal void
				for (int f = 0; f < SF.rows(); ++f)
				{
					std::swap(SF(f, 1), SF(f, 2));
				}

				if (largestVoidOnly && SF.rows() > 0)
				{
					Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> SN;
					igl::per_face_normals(SV, SF, SN);

					Eigen::Matrix<int, Eigen::Dynamic, 1> C;
					igl::facet_components(SF, C);

					std::unordered_map<int, std::vector<int>> faceIdVec;
					std::unordered_map<int, double> faceIdVol;
					for (int f = 0; f < C.rows(); ++f)
					{
						faceIdVec[C(f, 0)].push_back(f);

						Eigen::Matrix<double, 1, Eigen::Dynamic> v01, v02;
						v01 = SV.row(SF(f, 1)) - SV.row(SF(f, 0));
						v02 = SV.row(SF(f, 2)) - SV.row(SF(f, 0));
						const double triarea = v01.template head<3>().cross(v02.template head<3>()).norm() * 0.5;
						const double height = -1.0 * SN.row(f).dot(SV.row(SF(f, 0)));
						const double volume = triarea * height;
						faceIdVol[C(f, 0)] += volume;
					}

					int maxVolId = -1;
					double maxVol = -1.0;
					for (const auto &id_vol : faceIdVol)
					{
						const int &id = id_vol.first;
						const double &vol = id_vol.second;
						if (vol >= maxVol)
						{
							maxVolId = id;
							maxVol = vol;
						}
					}
					if (maxVolId > -1)
					{
						const std::vector<int> &faces = faceIdVec.at(maxVolId);
						Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> SV_ = SV;
						Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> SF_;

						SF_.resize(faces.size(), SF.cols());
						for (int ff = 0; ff < faces.size(); ++ff)
						{
							SF_.row(ff) = SF.row(faces.at(ff));
						}
						Eigen::Matrix<int, Eigen::Dynamic, 1> I;
						igl::remove_unreferenced(SV_, SF_, SV, SF, I);
					}
				}
			}

			// merge to original mesh
			{
				// vertex
				const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VPrev = std::move(mesh->V);
				{
					mesh->V.resize(VPrev.rows() + SV.rows(), 3);
					mesh->V << VPrev, SV;
				}
				// faces
				const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> FPrev = std::move(mesh->F);
				{
					mesh->F.resize(FPrev.rows() + SF.rows(), 3);
					mesh->F << FPrev, (SF.array() + VPrev.rows());
				}

				// face normal (re-calculate)
				igl::per_face_normals(mesh->V, mesh->F, mesh->FN);
				// vertex normal (re-calculate)
				igl::per_vertex_normals(mesh->V, mesh->F, mesh->FN, mesh->VN);

				// vertex color
				if (VPrev.rows() == mesh->VC.rows())
				{
					const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> VCPrev = std::move(mesh->VC);

					mesh->VC.resize(mesh->V.rows(), VCPrev.cols());
					mesh->VC.setOnes();
					mesh->VC.block(0, 0, VCPrev.rows(), VCPrev.cols()) = VCPrev;
				}
				// face color
				if (FPrev.rows() == mesh->FC.rows())
				{
					const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> FCPrev = std::move(mesh->FC);

					mesh->FC.resize(mesh->F.rows(), FCPrev.cols());
					mesh->FC.setOnes();
					mesh->FC.block(0, 0, FCPrev.rows(), FCPrev.cols()) = FCPrev;
				}

				// face group
				if (FPrev.rows() == mesh->FG.rows())
				{
					const Eigen::Matrix<int, Eigen::Dynamic, 1> FGPrev = std::move(mesh->FG);

					mesh->FG.resize(mesh->F.rows(), FGPrev.cols());
					const int freshGroupIdx = FGPrev.maxCoeff() + 1;
					mesh->FG.setOnes();
					mesh->FG *= freshGroupIdx;
					mesh->FG.block(0, 0, FGPrev.rows(), FGPrev.cols()) = FGPrev;
				}

				// texture coordinates
				{
					if (FPrev.rows() == mesh->FTC.rows())
					{
						// face texture coordinates (FTC)
						// this implementation is not perfect ...
						//   FTC could contain different uv-coordinate on the same vertex
						const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> TCPrev = std::move(mesh->TC);
						const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> FTCPrev = std::move(mesh->FTC);

						mesh->TC.resize(mesh->V.rows(), 2);
						// we assign (0 ,0) for vertices on the internal void.
						mesh->TC.setZero();
						for (int f = 0; f < FPrev.rows(); ++f)
						{
							for (int fv = 0; fv < FPrev.cols(); ++fv)
							{
								mesh->TC.row(FPrev(f, fv)) = TCPrev.row(FTCPrev(f, fv));
							}
						}
						mesh->FTC = mesh->F;
					}
					else if (VPrev.rows() == mesh->TC.rows())
					{
						// (vertex) texture coordinates
						const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> TCPrev = std::move(mesh->TC);

						mesh->TC.resize(mesh->V.rows(), TCPrev.cols());
						// we assign (0 ,0) for vertices on the internal void.
						mesh->TC.setZero();
						mesh->TC.block(0, 0, TCPrev.rows(), TCPrev.cols()) = TCPrev;
					}
				}

				// misc
				{
					igl::triangle_triangle_adjacency(mesh->F, mesh->TT, mesh->TTi);
					igl::vertex_triangle_adjacency(mesh->V, mesh->F, mesh->VF, mesh->VFi);
					mesh->AABB.init(mesh->V, mesh->F);
					mesh->principalComponent.resize(0, mesh->V.cols());
					mesh->PD1.resize(0, mesh->V.cols());
					mesh->PD2.resize(0, mesh->V.cols());
					mesh->PV1.resize(0, 1);
					mesh->PV2.resize(0, 1);
					mesh->K.resize(0, 1);
				}
			}

			broadcast["meshes"][meshUUID] = mesh->dumpToJson(true);
			broadcast["meshes"][meshUUID]["remove"] = false;
			diff["meshes"][meshUUID] = mesh->dumpToJson(false);
			diff["meshes"][meshUUID]["remove"] = false;
		}
	}
	room->storeHistory(diff, diffInv);
}

#endif

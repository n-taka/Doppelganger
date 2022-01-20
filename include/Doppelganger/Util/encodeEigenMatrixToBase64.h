#ifndef ENCODEEIGENMATRIXTOBASE64_H
#define ENCODEEIGENMATRIXTOBASE64_H

#include <string>
#include "Eigen/Core"
#include <boost/beast/core/detail/base64.hpp>

namespace Doppelganger
{
	namespace Util
	{
		template <typename Derived>
		std::string encodeEigenMatrixToBase64(const Eigen::MatrixBase<Derived> &mat)
		{
			if (mat.size() > 0)
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
			else
			{
				return std::string("");
			}
		}
	};
} // namespace

#endif

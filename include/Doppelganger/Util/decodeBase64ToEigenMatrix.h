#ifndef DECODEBASE64TOEIGENMATRIX_H
#define DECODEBASE64TOEIGENMATRIX_H

#include <string>
#include "Eigen/Core"
#include <boost/beast/core/detail/base64.hpp>

namespace Doppelganger
{
	namespace Util
	{
		template <typename Derived>
		void decodeBase64ToEigenMatrix(const std::string &base64, const int &cols, Eigen::MatrixBase<Derived> &mat)
		{
			if (base64.length() > 0)
			{
				std::vector<unsigned char> binData;
				const size_t len = boost::beast::detail::base64::decoded_size(base64.size());
				binData.resize(len);
				const std::pair<size_t, size_t> lenWrittenRead = boost::beast::detail::base64::decode(&(binData[0]), base64.data(), base64.size());

				Eigen::Matrix<typename Derived::Scalar, Derived::RowsAtCompileTime, Derived::ColsAtCompileTime, Eigen::RowMajor> matRowMajor;
				matRowMajor.resize(lenWrittenRead.first / (sizeof(typename Derived::Scalar) * cols), cols);
				memmove(matRowMajor.data(), &(binData[0]), binData.size());
				mat = matRowMajor;
			}
		}
	};
} // namespace

#endif

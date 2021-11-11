#ifndef UNZIP_CPP
#define UNZIP_CPP

#include "Doppelganger/Util/unzip.h"
#include <string>
#include <fstream>

#include "minizip/unzip.h"
#if defined(__APPLE__)
#include <sys/stat.h>
#endif

namespace Doppelganger
{
	namespace Util
	{
		void unzip(const fs::path &zipPath, const fs::path &destPath)
		{
			// http://hp.vector.co.jp/authors/VA016379/cpplib/zip.cpp
			unzFile unzipHandle = unzOpen(zipPath.string().c_str());
			if (unzipHandle)
			{
				do
				{
					char fileName[512];
					unz_file_info fileInfo;
					if (unzGetCurrentFileInfo(unzipHandle, &fileInfo, fileName, sizeof fileName, NULL, 0, NULL, 0) != UNZ_OK)
					{
						break;
					}

					fs::path filePath(fileName);
					filePath.make_preferred();
					fs::path targetPath(destPath);
					for (const auto &p : filePath)
					{
						targetPath.append(p.string());
					}

					if (unzOpenCurrentFile(unzipHandle) != UNZ_OK)
					{
						break;
					}
					// create direcotry
					fs::create_directories(targetPath.parent_path());

					std::ofstream ofs(targetPath.string(), std::ios::binary);

					char buffer[8192];
					unsigned long sizeRead;
					while ((sizeRead = unzReadCurrentFile(unzipHandle, buffer, sizeof buffer)) > 0)
					{
						ofs.write(buffer, sizeRead);
					}
					ofs.close();

#if defined(__APPLE__)
					// restore original permission
					chmod(targetPath.string().c_str(), ((fileInfo.external_fa >> 16) & 0x01FF));
#endif

					unzCloseCurrentFile(unzipHandle);

				} while (unzGoToNextFile(unzipHandle) != UNZ_END_OF_LIST_OF_FILE);

				unzClose(unzipHandle);
			}
		}
	}
} // namespace

#endif

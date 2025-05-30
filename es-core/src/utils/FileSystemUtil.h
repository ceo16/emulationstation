#pragma once
#ifndef ES_CORE_UTILS_FILE_SYSTEM_UTIL_H
#define ES_CORE_UTILS_FILE_SYSTEM_UTIL_H

#include <list>
#include <string>
#include <vector>
#include "utils/TimeUtil.h"

namespace Utils
{
	namespace FileSystem
	{
		typedef std::list<std::string> stringList;
		
		stringList  getDirContent      (const std::string& _path, const bool _recursive = false, const bool includeHidden = false);
		std::vector<std::string>  getPathList        (const std::string& _path);
		std::string getCWDPath         ();
		std::string getPreferredPath   (const std::string& _path);
		std::string getGenericPath     (const std::string& _path);
		std::string getEscapedPath     (const std::string& _path);
		std::string getCanonicalPath   (const std::string& _path);
		std::string getAbsolutePath    (const std::string& _path, const std::string& _base = getCWDPath());
		std::string getParent          (const std::string& _path);
		std::string getFileName        (const std::string& _path);
		std::string getStem            (const std::string& _path);
		std::string getExtension       (const std::string& _path, bool withPoint = true);
		std::string resolveRelativePath(const std::string& _path, const std::string& _relativeTo, const bool _allowHome);
		std::string createRelativePath (const std::string& _path, const std::string& _relativeTo, const bool _allowHome);
		std::string removeCommonPath   (const std::string& _path, const std::string& _common, bool& _contains);
		std::string resolveSymlink     (const std::string& _path);
		bool        removeFile         (const std::string& _path);
		bool        createDirectory    (const std::string& _path);
		bool        removeDirectory    (const std::string& _path);
		bool        exists             (const std::string& _path);
		bool        isAbsolute         (const std::string& _path);
		bool        isRegularFile      (const std::string& _path);
		bool        isDirectory        (const std::string& _path);
		bool        isSymlink          (const std::string& _path);
		bool        isHidden           (const std::string& _path);

		bool		isImage		       (const std::string& _path);
		bool		isVideo            (const std::string& _path);
		bool		isAudio            (const std::string& _path);
		bool		isSVG			   (const std::string& _path);

		struct FileInfo
		{
		public:
			std::string path;
			bool hidden;
			bool directory;
#if WIN32
			time_t lastWriteTime;
#endif
		};

		typedef std::list<FileInfo> fileList;

		fileList	getDirectoryFiles(const std::string& _path);
		std::string combine(const std::string& _path, const std::string& filename);
		unsigned long long	getFileSize(const std::string& _path);

		Utils::Time::DateTime getFileCreationDate(const std::string& _path);
		Utils::Time::DateTime getFileModificationDate(const std::string& _path);

		std::string	readAllText(const std::string& fileName);
		stringList	readAllLines(const std::string& fileName);
		void		writeAllText(const std::string& fileName, const std::string& text);
		bool		copyFile(const std::string src, const std::string dst);
		void		deleteDirectoryFiles(const std::string path, bool deleteDirectory = false);
		bool		renameFile(const std::string src, const std::string dst, bool overWrite = true);

		std::string megaBytesToString(unsigned long size);
		std::string kiloBytesToString(unsigned long size);

		std::string getTempPath();
		std::string getPdfTempPath();
        std::string createValidFileName(const std::string& path); // CORRETTO
#ifdef WIN32
		void		splitCommand(std::string cmd, std::string* executable, std::string* parameters);
#endif
		void		preloadFileSystemCache(const std::string& path, bool trySaveStates = true);

		std::string getFileCrc32(const std::string& filename);
		std::string getFileMd5(const std::string& filename);

		std::string changeExtension(const std::string& _path, const std::string& extension);

		class FileSystemCacheActivator
		{
		public:
			FileSystemCacheActivator();
			~FileSystemCacheActivator();

		private:
			static int mReferenceCount;
		};
         std::string getEsConfigPath();
	} // FileSystem::

} // Utils::

#endif // ES_CORE_UTILS_FILE_SYSTEM_UTIL_H

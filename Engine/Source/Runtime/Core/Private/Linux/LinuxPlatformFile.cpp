// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include <sys/file.h>	// flock()
#include <sys/stat.h>   // mkdirp()

DEFINE_LOG_CATEGORY_STATIC(LogLinuxPlatformFile, Log, All);

// make an FTimeSpan object that represents the "epoch" for time_t (from a stat struct)
const FDateTime UnixEpoch(1970, 1, 1);

/** 
 * Linux file handle implementation
**/
class CORE_API FFileHandleLinux : public IFileHandle
{
	enum {READWRITE_SIZE = 1024 * 1024};
	int32 FileHandle;

	FORCEINLINE bool IsValid()
	{
		return FileHandle != -1;
	}

public:
	FFileHandleLinux(int32 InFileHandle = -1)
		: FileHandle(InFileHandle)
	{
	}

	virtual ~FFileHandleLinux()
	{
		close(FileHandle);
		FileHandle = -1;
	}

	virtual int64 Tell() override
	{
		check(IsValid());
		return lseek(FileHandle, 0, SEEK_CUR);
	}

	virtual bool Seek(int64 NewPosition) override
	{
		check(IsValid());
		check(NewPosition >= 0);
		return lseek(FileHandle, NewPosition, SEEK_SET) != -1;
	}

	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) override
	{
		check(IsValid());
		check(NewPositionRelativeToEnd <= 0);
		return lseek(FileHandle, NewPositionRelativeToEnd, SEEK_END) != -1;
	}

	virtual bool Read(uint8* Destination, int64 BytesToRead) override
	{
		check(IsValid());
		while (BytesToRead)
		{
			check(BytesToRead >= 0);
			int64 ThisSize = FMath::Min<int64>(READWRITE_SIZE, BytesToRead);
			check(Destination);
			if (read(FileHandle, Destination, ThisSize) != ThisSize)
			{
				return false;
			}
			Destination += ThisSize;
			BytesToRead -= ThisSize;
		}
		return true;
	}

	virtual bool Write(const uint8* Source, int64 BytesToWrite) override
	{
		check(IsValid());
		while (BytesToWrite)
		{
			check(BytesToWrite >= 0);
			int64 ThisSize = FMath::Min<int64>(READWRITE_SIZE, BytesToWrite);
			check(Source);
			if (write(FileHandle, Source, ThisSize) != ThisSize)
			{
				return false;
			}
			Source += ThisSize;
			BytesToWrite -= ThisSize;
		}
		return true;
	}
};

/**
 * Linux File I/O implementation
**/
FString FLinuxPlatformFile::NormalizeFilename(const TCHAR* Filename)
{
	FString Result(Filename);
	FPaths::NormalizeFilename(Result);
	return FPaths::ConvertRelativePathToFull(Result);
}

FString FLinuxPlatformFile::NormalizeDirectory(const TCHAR* Directory)
{
	FString Result(Directory);
	FPaths::NormalizeDirectoryName(Result);
	return FPaths::ConvertRelativePathToFull(Result);
}

bool FLinuxPlatformFile::FileExists(const TCHAR* Filename)
{
	struct stat FileInfo;
	if (stat(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), &FileInfo) != -1)
	{
		return S_ISREG(FileInfo.st_mode);
	}
	return false;
}

int64 FLinuxPlatformFile::FileSize(const TCHAR* Filename)
{
	struct stat FileInfo;
	FileInfo.st_size = -1;
	if (stat(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), &FileInfo) != -1)
	{
		// make sure to return -1 for directories
		if (S_ISDIR(FileInfo.st_mode))
		{
			FileInfo.st_size = -1;
		}
	}
	return FileInfo.st_size;
}

bool FLinuxPlatformFile::DeleteFile(const TCHAR* Filename)
{
	return unlink(TCHAR_TO_UTF8(*NormalizeFilename(Filename))) == 0;
}

bool FLinuxPlatformFile::IsReadOnly(const TCHAR* Filename)
{
	if (access(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), F_OK) == -1)
	{
		return false; // file doesn't exist
	}
	if (access(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), W_OK) == -1)
	{
		return errno == EACCES;
	}
	return false;
}

bool FLinuxPlatformFile::MoveFile(const TCHAR* To, const TCHAR* From)
{
	return rename(TCHAR_TO_UTF8(*NormalizeFilename(From)), TCHAR_TO_UTF8(*NormalizeFilename(To))) != -1;
}

bool FLinuxPlatformFile::SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue)
{
	struct stat FileInfo;
	if (stat(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), &FileInfo) != -1)
	{
		if (bNewReadOnlyValue)
		{
			FileInfo.st_mode &= ~S_IWUSR;
		}
		else
		{
			FileInfo.st_mode |= S_IWUSR;
		}
		return chmod(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), FileInfo.st_mode);
	}
	return false;
}

FDateTime FLinuxPlatformFile::GetTimeStamp(const TCHAR* Filename)
{
	// get file times
	struct stat FileInfo;
	if(stat(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), &FileInfo) == -1)
	{
		if (errno == EOVERFLOW)
		{
			// hacky workaround for files mounted on Samba (see https://bugzilla.samba.org/show_bug.cgi?id=7707)
			return FDateTime::Now();
		}
		else
		{
			return FDateTime::MinValue();
		}
	}

	// convert _stat time to FDateTime
	FTimespan TimeSinceEpoch(0, 0, FileInfo.st_mtime);
	return UnixEpoch + TimeSinceEpoch;
}

void FLinuxPlatformFile::SetTimeStamp(const TCHAR* Filename, const FDateTime DateTime)
{
	// get file times
	struct stat FileInfo;
	if(stat(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), &FileInfo) == -1)
	{
		return;
	}

	// change the modification time only
	struct utimbuf Times;
	Times.actime = FileInfo.st_atime;
	Times.modtime = (DateTime - UnixEpoch).GetTotalSeconds();
	utime(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), &Times);
}

FDateTime FLinuxPlatformFile::GetAccessTimeStamp(const TCHAR* Filename)
{
	// get file times
	struct stat FileInfo;
	if(stat(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), &FileInfo) == -1)
	{
		return FDateTime::MinValue();
	}

	// convert _stat time to FDateTime
	FTimespan TimeSinceEpoch(0, 0, FileInfo.st_atime);
	return UnixEpoch + TimeSinceEpoch;
}

FString FLinuxPlatformFile::GetFilenameOnDisk(const TCHAR* Filename)
{
	return Filename;
}

IFileHandle* FLinuxPlatformFile::OpenRead(const TCHAR* Filename)
{
	int32 Handle = open(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), O_RDONLY);
	if (Handle != -1)
	{
		return new FFileHandleLinux(Handle);
	}
	// log non-standard errors only
	if (ENOENT != errno)
	{
		UE_LOG(LogLinuxPlatformFile, Warning, TEXT( "open('%s', ORDONLY) failed: errno=%d (%s)" ), *NormalizeFilename(Filename), errno, ANSI_TO_TCHAR(strerror(errno)));
	}
	return NULL;
}

IFileHandle* FLinuxPlatformFile::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
	int Flags = O_CREAT | O_CLOEXEC;	// prevent children from inheriting this
	if (bAppend)
	{
		Flags |= O_APPEND;
	}

	if (bAllowRead)
	{
		Flags |= O_RDWR;
	}
	else
	{
		Flags |= O_WRONLY;
	}

	// create directories if needed.
	if (!CreateDirectoriesFromPath(Filename))
	{
		return NULL;
	}

	// Caveat: cannot specify O_TRUNC in flags, as this will corrupt the file which may be "locked" by other process. We will ftruncate() it once we "lock" it
	int32 Handle = open(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), Flags, S_IRUSR | S_IWUSR);
	if (Handle != -1)
	{
		// mimic Windows "exclusive write" behavior (we don't use FILE_SHARE_WRITE) by locking the file.
		// note that the (non-mandatory) "lock" will be removed by itself when the last file descriptor is close()d
		if (flock(Handle, LOCK_EX | LOCK_NB) == -1)
		{
			// if locked, consider operation a failure
			if (EAGAIN == errno || EWOULDBLOCK == errno)
			{
				close(Handle);
				return nullptr;
			}
			// all the other locking errors are ignored.
		}

		// truncate the file now that we locked it
		if (!bAppend)
		{
			if (ftruncate(Handle, 0) != 0)
			{
				int ErrNo = errno;
				UE_LOG(LogLinuxPlatformFile, Warning, TEXT( "ftruncate() failed for '%s': errno=%d (%s)" ),
															Filename, ErrNo, ANSI_TO_TCHAR(strerror(ErrNo)));
				close(Handle);
				return nullptr;
			}
		}

		FFileHandleLinux* FileHandleLinux = new FFileHandleLinux(Handle);
		if (bAppend)
		{
			FileHandleLinux->SeekFromEnd(0);
		}
		return FileHandleLinux;
	}

	int ErrNo = errno;
	UE_LOG(LogLinuxPlatformFile, Warning, TEXT( "open('%s', Flags=0x%08X) failed: errno=%d (%s)" ), *NormalizeFilename(Filename), Flags, ErrNo, ANSI_TO_TCHAR(strerror(ErrNo)));
	return nullptr;
}

bool FLinuxPlatformFile::DirectoryExists(const TCHAR* Directory)
{
	struct stat FileInfo;
	if (stat(TCHAR_TO_UTF8(*NormalizeFilename(Directory)), &FileInfo) != -1)
	{
		return S_ISDIR(FileInfo.st_mode);
	}
	return false;
}

bool FLinuxPlatformFile::CreateDirectory(const TCHAR* Directory)
{
	return mkdir(TCHAR_TO_UTF8(*NormalizeFilename(Directory)), 0755) == 0;
}

bool FLinuxPlatformFile::DeleteDirectory(const TCHAR* Directory)
{
	return rmdir(TCHAR_TO_UTF8(*NormalizeFilename(Directory))) == 0;
}

bool FLinuxPlatformFile::IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor)
{
	bool Result = false;

	FString NormalizedDirectory = NormalizeFilename(Directory);
	DIR* Handle = opendir(TCHAR_TO_UTF8(*NormalizedDirectory));
	if (Handle)
	{
		Result = true;
		struct dirent *Entry;
		while ((Entry = readdir(Handle)) != NULL)
		{
			if (FCString::Strcmp(UTF8_TO_TCHAR(Entry->d_name), TEXT(".")) && FCString::Strcmp(UTF8_TO_TCHAR(Entry->d_name), TEXT("..")))
			{
				bool bIsDirectory = false;
				FString UnicodeEntryName = UTF8_TO_TCHAR(Entry->d_name);
				if (Entry->d_type != DT_UNKNOWN)
				{
					bIsDirectory = Entry->d_type == DT_DIR;
				}
				else
				{
					// filesystem does not support d_type, fallback to stat
					struct stat FileInfo;
					FString AbsoluteUnicodeName = NormalizedDirectory / UnicodeEntryName;	
					if (stat(TCHAR_TO_UTF8(*AbsoluteUnicodeName), &FileInfo) != -1)
					{
						bIsDirectory = ((FileInfo.st_mode & S_IFMT) == S_IFDIR);
					}
					else
					{
						int ErrNo = errno;
						UE_LOG(LogLinuxPlatformFile, Warning, TEXT( "Cannot determine whether '%s' is a directory - d_type not supported and stat() failed with errno=%d (%s)"), *AbsoluteUnicodeName, ErrNo, UTF8_TO_TCHAR(strerror(ErrNo)));
					}
				}
				Result = Visitor.Visit(*(FString(Directory) / UnicodeEntryName), bIsDirectory);
			}
		}
		closedir(Handle);
	}
	return Result;
}

bool FLinuxPlatformFile::CreateDirectoriesFromPath(const TCHAR* Path)
{
	// if the file already exists, then directories exist.
	struct stat FileInfo;
	if (stat(TCHAR_TO_UTF8(*NormalizeFilename(Path)), &FileInfo) != -1)
	{
		return true;
	}

	// convert path to native char string.
	int32 Len = strlen(TCHAR_TO_UTF8(*NormalizeFilename(Path)));
	char *DirPath = reinterpret_cast<char *>(FMemory::Malloc((Len+2) * sizeof(char)));
	char *SubPath = reinterpret_cast<char *>(FMemory::Malloc((Len+2) * sizeof(char)));
	strcpy(DirPath, TCHAR_TO_UTF8(*NormalizeFilename(Path)));

	for (int32 i=0; i<Len; ++i)
	{
		struct stat FileInfo;

		SubPath[i] = DirPath[i];

		if (SubPath[i] == '/')
		{
			SubPath[i+1] = 0;

			// directory exists?
			if (stat(SubPath, &FileInfo) == -1)
			{
				// nope. create it.
				if (mkdir(SubPath, 0755) == -1)
				{
					int ErrNo = errno;
					UE_LOG(LogLinuxPlatformFile, Warning, TEXT( "create dir('%s') failed: errno=%d (%s)" ),
																DirPath, ErrNo, ANSI_TO_TCHAR(strerror(ErrNo)));
					FMemory::Free(DirPath);
					FMemory::Free(SubPath);
					return false;
				}
			}
		}
	}

	FMemory::Free(DirPath);
	FMemory::Free(SubPath);
	return true;
}

IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
	static FLinuxPlatformFile Singleton;
	return Singleton;
}

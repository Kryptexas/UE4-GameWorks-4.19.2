// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsPlatformFile.cpp: Windows implementations of File functions
=============================================================================*/

#include "CorePrivate.h"
#include <sys/utime.h>

// make an FTimeSpan object that represents the "epoch" for time_t (from a _stat struct)
const FDateTime WindowsEpoch(1970, 1, 1);

#include "AllowWindowsPlatformTypes.h"
	namespace FileConstants
	{
		uint32 WIN_INVALID_SET_FILE_POINTER = INVALID_SET_FILE_POINTER;
	}
#include "HideWindowsPlatformTypes.h"

/** 
 * Windows file handle implementation
**/
class CORE_API FFileHandleWindows : public IFileHandle
{
	enum {READWRITE_SIZE = 1024 * 1024};
	HANDLE FileHandle;

	FORCEINLINE int64 FileSeek(int64 Distance, uint32 MoveMethod)
	{
		LARGE_INTEGER li;
		li.QuadPart = Distance;
		li.LowPart = SetFilePointer(FileHandle, li.LowPart, &li.HighPart, MoveMethod);
		if (li.LowPart == FileConstants::WIN_INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		{
			li.QuadPart = -1;
		}
		return li.QuadPart;
	}
	FORCEINLINE bool IsValid()
	{
		return FileHandle != NULL && FileHandle != INVALID_HANDLE_VALUE;
	}

public:
	FFileHandleWindows(HANDLE InFileHandle = NULL)
		: FileHandle(InFileHandle)
	{
	}
	virtual ~FFileHandleWindows()
	{
		CloseHandle(FileHandle);
		FileHandle = NULL;
	}
	virtual int64 Tell() OVERRIDE
	{
		check(IsValid());
		return FileSeek(0, FILE_CURRENT);
	}
	virtual bool Seek(int64 NewPosition) OVERRIDE
	{
		check(IsValid());
		check(NewPosition >= 0);
		return FileSeek(NewPosition, FILE_BEGIN) != -1;
	}
	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) OVERRIDE
	{
		check(IsValid());
		check(NewPositionRelativeToEnd <= 0);
		return FileSeek(NewPositionRelativeToEnd, FILE_END) != -1;
	}
	virtual bool Read(uint8* Destination, int64 BytesToRead) OVERRIDE
	{
		check(IsValid());
		while (BytesToRead)
		{
			check(BytesToRead >= 0);
			int64 ThisSize = FMath::Min<int64>(READWRITE_SIZE, BytesToRead);
			check(Destination);
			uint32 Result=0;
			if (!ReadFile(FileHandle, Destination, uint32(ThisSize), (::DWORD *)&Result, NULL) || Result != uint32(ThisSize))
			{
				return false;
			}
			Destination += ThisSize;
			BytesToRead -= ThisSize;
		}
		return true;
	}
	virtual bool Write(const uint8* Source, int64 BytesToWrite) OVERRIDE
	{
		check(IsValid());
		while (BytesToWrite)
		{
			check(BytesToWrite >= 0);
			int64 ThisSize = FMath::Min<int64>(READWRITE_SIZE, BytesToWrite);
			check(Source);
			uint32 Result=0;
			if (!WriteFile(FileHandle, Source, uint32(ThisSize), (::DWORD *)&Result, NULL) || Result != uint32(ThisSize))
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
 * Windows File I/O implementation
**/
class CORE_API FWindowsPlatformFile : public IPhysicalPlatformFile
{
protected:
	virtual FString NormalizeFilename(const TCHAR* Filename)
	{
		FString Result(Filename);
		FPaths::NormalizeFilename(Result);
		if (Result.StartsWith(TEXT("//")))
		{
			Result = FString(TEXT("\\\\")) + Result.RightChop(2);
		}
		return FPaths::ConvertRelativePathToFull(Result);
	}
	virtual FString NormalizeDirectory(const TCHAR* Directory)
	{
		FString Result(Directory);
		FPaths::NormalizeDirectoryName(Result);
		if (Result.StartsWith(TEXT("//")))
		{
			Result = FString(TEXT("\\\\")) + Result.RightChop(2);
		}
		return FPaths::ConvertRelativePathToFull(Result);
	}
public:
	virtual bool FileExists(const TCHAR* Filename) OVERRIDE
	{
		uint32 Result = GetFileAttributesW(*NormalizeFilename(Filename));
		if (Result != 0xFFFFFFFF && !(Result & FILE_ATTRIBUTE_DIRECTORY))
		{
			return true;
		}
		return false;
	}
	virtual int64 FileSize(const TCHAR* Filename) OVERRIDE
	{
		WIN32_FILE_ATTRIBUTE_DATA Info;
		if (!!GetFileAttributesExW(*NormalizeFilename(Filename), GetFileExInfoStandard, &Info))
		{
			if ((Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				LARGE_INTEGER li;
				li.HighPart = Info.nFileSizeHigh;
				li.LowPart = Info.nFileSizeLow;
				return li.QuadPart;
			}
		}
		return -1;
	}
	virtual bool DeleteFile(const TCHAR* Filename) OVERRIDE
	{
		return !!DeleteFileW(*NormalizeFilename(Filename));
	}
	virtual bool IsReadOnly(const TCHAR* Filename) OVERRIDE
	{
		uint32 Result = GetFileAttributes(*NormalizeFilename(Filename));
		if (Result != 0xFFFFFFFF)
		{
			return !!(Result & FILE_ATTRIBUTE_READONLY);
		}
		return false;
	}
	virtual bool MoveFile(const TCHAR* To, const TCHAR* From) OVERRIDE
	{
		return !!MoveFileW(*NormalizeFilename(From), *NormalizeFilename(To));
	}
	virtual bool SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) OVERRIDE
	{
		return !!SetFileAttributesW(*NormalizeFilename(Filename), bNewReadOnlyValue ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_NORMAL);
	}

	virtual FDateTime GetTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		// get file times
		struct _stati64 FileInfo;
		if(_wstati64(*NormalizeFilename(Filename), &FileInfo))
		{
			return FDateTime::MinValue();
		}

		// convert _stat time to FDateTime
		FTimespan TimeSinceEpoch(0, 0, FileInfo.st_mtime);
		return WindowsEpoch + TimeSinceEpoch;
	}

	virtual void SetTimeStamp(const TCHAR* Filename, FDateTime DateTime) OVERRIDE
	{
		// get file times
		struct _stati64 FileInfo;
		if(_wstati64(*NormalizeFilename(Filename), &FileInfo))
		{
			return;
		}

		// change the modification time only
		struct _utimbuf Times;
		Times.actime = FileInfo.st_atime;
		Times.modtime = (DateTime - WindowsEpoch).GetTotalSeconds();
		_wutime(Filename, &Times);
	}

	virtual FDateTime GetAccessTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		// get file times
		struct _stati64 FileInfo;
		if(_wstati64(*NormalizeFilename(Filename), &FileInfo))
		{
			return FDateTime::MinValue();
		}

		// convert _stat time to FDateTime
		FTimespan TimeSinceEpoch(0, 0, FileInfo.st_atime);
		return WindowsEpoch + TimeSinceEpoch;
	}

	virtual IFileHandle* OpenRead(const TCHAR* Filename) OVERRIDE
	{
		uint32  Access    = GENERIC_READ;
		uint32  WinFlags  = FILE_SHARE_READ;
		uint32  Create    = OPEN_EXISTING;
		HANDLE Handle    = CreateFileW(*NormalizeFilename(Filename), Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL);
		if(Handle != INVALID_HANDLE_VALUE)
		{
			return new FFileHandleWindows(Handle);
		}
		return NULL;
	}
	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false) OVERRIDE
	{
		uint32  Access    = GENERIC_WRITE;
		uint32  WinFlags  = bAllowRead ? FILE_SHARE_READ : 0;
		uint32  Create    = bAppend ? OPEN_ALWAYS : CREATE_ALWAYS;
		HANDLE Handle    = CreateFileW(*NormalizeFilename(Filename), Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL);
		if(Handle != INVALID_HANDLE_VALUE)
		{
			return new FFileHandleWindows(Handle);
		}
		return NULL;
	}

	virtual bool DirectoryExists(const TCHAR* Directory) OVERRIDE
	{
		// Empty Directory is the current directory so assume it always exists.
		bool bExists = !FCString::Strlen(Directory);
		if (!bExists) 
		{
			uint32 Result = GetFileAttributesW(*NormalizeDirectory(Directory));
			bExists = (Result != 0xFFFFFFFF && (Result & FILE_ATTRIBUTE_DIRECTORY));
		}
		return bExists;
	}
	virtual bool CreateDirectory(const TCHAR* Directory) OVERRIDE
	{
		return CreateDirectoryW(*NormalizeDirectory(Directory), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
	}
	virtual bool DeleteDirectory(const TCHAR* Directory) OVERRIDE
	{
		RemoveDirectoryW(*NormalizeDirectory(Directory));
		return !DirectoryExists(Directory);
	}
	bool IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor)
	{
		bool Result = false;
		WIN32_FIND_DATAW Data;
		HANDLE Handle = FindFirstFileW(*(NormalizeDirectory(Directory) / TEXT("*.*")), &Data);
		if (Handle != INVALID_HANDLE_VALUE)
		{
			Result = true;
			do
			{
				if (FCString::Strcmp(Data.cFileName, TEXT(".")) && FCString::Strcmp(Data.cFileName, TEXT("..")))
				{
					Result = Visitor.Visit(*(FString(Directory) / Data.cFileName), !!(Data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY));
				}
			} while (Result && FindNextFileW(Handle, &Data));
			FindClose(Handle);
		}
		return Result;
	}
};

IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
	static FWindowsPlatformFile Singleton;
	return Singleton;
}

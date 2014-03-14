// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"

#include "IOSPlatformFile.h"

//#if PLATFORM_IOS
//#include "IPlatformFileSandboxWrapper.h"
//#endif

// make an FTimeSpan object that represents the "epoch" for time_t (from a stat struct)
const FDateTime MacEpoch(1970, 1, 1);

/** 
 * iOS file handle implementation
**/
FFileHandleIOS::FFileHandleIOS(int32 InFileHandle)
	: FileHandle(InFileHandle)
{
}

FFileHandleIOS::~FFileHandleIOS()
{
	close(FileHandle);
	FileHandle = -1;
}

int64 FFileHandleIOS::Tell()
{
	check(IsValid());
	return lseek(FileHandle, 0, SEEK_CUR);
}

bool FFileHandleIOS::Seek(int64 NewPosition)
{
	check(IsValid());
	check(NewPosition >= 0);
	return lseek(FileHandle, NewPosition, SEEK_SET) != -1;
}

bool FFileHandleIOS::SeekFromEnd(int64 NewPositionRelativeToEnd)
{
	check(IsValid());
	check(NewPositionRelativeToEnd <= 0);
	return lseek(FileHandle, NewPositionRelativeToEnd, SEEK_END) != -1;
}

bool FFileHandleIOS::Read(uint8* Destination, int64 BytesToRead)
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

bool FFileHandleIOS::Write(const uint8* Source, int64 BytesToWrite)
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


/**
 * iOS File I/O implementation
**/
bool Initialize(IPlatformFile* Inner, const TCHAR* CommandLineParam)
{
	return true;
}

FString FIOSPlatformFile::NormalizeFilename(const TCHAR* Filename)
{
	FString Result(Filename);
	Result.ReplaceInline(TEXT("\\"), TEXT("/"));
	return Result;
}

FString FIOSPlatformFile::NormalizeDirectory(const TCHAR* Directory)
{
	FString Result(Directory);
	Result.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (Result.EndsWith(TEXT("/")))
	{
		Result.LeftChop(1);
	}
	return Result;
}

bool FIOSPlatformFile::FileExists(const TCHAR* Filename)
{
	struct stat FileInfo;
	FString NormalizedFilename = NormalizeFilename(Filename);
	// check the read path
	if (stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, false)), &FileInfo) == -1)
	{
		// if not in read path, check the write path
		if (stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, true)), &FileInfo) == -1)
		{
			return false;
		}
	}

	return S_ISREG(FileInfo.st_mode);
}

int64 FIOSPlatformFile::FileSize(const TCHAR* Filename)
{
	struct stat FileInfo;
	FileInfo.st_size = -1;
	FString NormalizedFilename = NormalizeFilename(Filename);
	// check the read path
	if(stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, false)), &FileInfo) == -1)
	{
		// if not in read path, check the write path
		stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, true)), &FileInfo);
	}

	// make sure to return -1 for directories
	if (S_ISDIR(FileInfo.st_mode))
	{
		FileInfo.st_size = -1;
	}
	return FileInfo.st_size;
}

bool FIOSPlatformFile::DeleteFile(const TCHAR* Filename)
{
	// only delete from write path
	FString IOSFilename = ConvertToIOSPath(NormalizeFilename(Filename), true);
	return unlink(TCHAR_TO_UTF8(*IOSFilename)) == 0;
}

bool FIOSPlatformFile::IsReadOnly(const TCHAR* Filename)
{
	FString NormalizedFilename = NormalizeFilename(Filename);
	FString Filepath = ConvertToIOSPath(NormalizedFilename, false);
	// check read path
	if (access(TCHAR_TO_UTF8(*Filepath), F_OK) == -1)
	{
		// if not in read path, check write path
		Filepath = ConvertToIOSPath(NormalizedFilename, true);
		if (access(TCHAR_TO_UTF8(*Filepath), F_OK) == -1)
		{
			return false; // file doesn't exist
		}
	}

	if (access(TCHAR_TO_UTF8(*Filepath), W_OK) == -1)
	{
		return errno == EACCES;
	}
	return false;
}

bool FIOSPlatformFile::MoveFile(const TCHAR* To, const TCHAR* From)
{
	// move to the write path
	FString ToIOSFilename = ConvertToIOSPath(NormalizeFilename(To), true);
	FString FromIOSFilename = ConvertToIOSPath(NormalizeFilename(From), false);
	return rename(TCHAR_TO_UTF8(*FromIOSFilename), TCHAR_TO_UTF8(*ToIOSFilename)) != -1;
}

bool FIOSPlatformFile::SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue)
{
	struct stat FileInfo;
	FString IOSFilename = ConvertToIOSPath(NormalizeFilename(Filename), false);
	if (stat(TCHAR_TO_UTF8(*IOSFilename), &FileInfo) != -1)
	{
		if (bNewReadOnlyValue)
		{
			FileInfo.st_mode &= ~S_IWUSR;
		}
		else
		{
			FileInfo.st_mode |= S_IWUSR;
		}
		return chmod(TCHAR_TO_UTF8(*IOSFilename), FileInfo.st_mode);
	}
	return false;
}


FDateTime FIOSPlatformFile::GetTimeStamp(const TCHAR* Filename)
{
	// get file times
	struct stat FileInfo;
	FString NormalizedFilename = NormalizeFilename(Filename);
	// check the read path
	if(stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, false)), &FileInfo) == -1)
	{
		// if not in the read path, check the write path
		if(stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, true)), &FileInfo) == -1)
		{
			return FDateTime::MinValue();
		}
	}

	// convert _stat time to FDateTime
	FTimespan TimeSinceEpoch(0, 0, FileInfo.st_mtime);
	return MacEpoch + TimeSinceEpoch;

}

void FIOSPlatformFile::SetTimeStamp(const TCHAR* Filename, const FDateTime DateTime)
{
	// get file times
	struct stat FileInfo;
	FString IOSFilename = ConvertToIOSPath(NormalizeFilename(Filename), true);
	if(stat(TCHAR_TO_UTF8(*IOSFilename), &FileInfo) == -1)
	{
		return;
	}

	// change the modification time only
	struct utimbuf Times;
	Times.actime = FileInfo.st_atime;
	Times.modtime = (DateTime - MacEpoch).GetTotalSeconds();
	utime(TCHAR_TO_UTF8(*IOSFilename), &Times);
}

FDateTime FIOSPlatformFile::GetAccessTimeStamp(const TCHAR* Filename)
{
	// get file times
	struct stat FileInfo;
	FString NormalizedFilename = NormalizeFilename(Filename);
	// check the read path
	if(stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, false)), &FileInfo) == -1)
	{
		// if not in the read path, check the write path
		if(stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, true)), &FileInfo) == -1)
		{
			return FDateTime::MinValue();
		}
	}

	// convert _stat time to FDateTime
	FTimespan TimeSinceEpoch(0, 0, FileInfo.st_atime);
	return MacEpoch + TimeSinceEpoch;
}

IFileHandle* FIOSPlatformFile::OpenRead(const TCHAR* Filename)
{
	FString NormalizedFilename = NormalizeFilename(Filename);
	// check the read path
	int32 Handle = open(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, false)), O_RDONLY);
	if(Handle == -1)
	{
		// if not in the read path, check the write path
		Handle = open(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedFilename, true)), O_RDONLY);
	}

	if (Handle != -1)
	{
		return new FFileHandleIOS(Handle);
	}
	return NULL;
}

IFileHandle* FIOSPlatformFile::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
	int Flags = O_CREAT;
	if (bAppend)
	{
		Flags |= O_APPEND;
	}
	else
	{
		Flags |= O_TRUNC;
	}
	if (bAllowRead)
	{
		Flags |= O_RDWR;
	}
	else
	{
		Flags |= O_WRONLY;
	}
	FString IOSFilename = ConvertToIOSPath(NormalizeFilename(Filename), true);
	int32 Handle = open(TCHAR_TO_UTF8(*IOSFilename), Flags, S_IRUSR | S_IWUSR);
	if (Handle != -1)
	{
		FFileHandleIOS* FileHandleIOS = new FFileHandleIOS(Handle);
		if (bAppend)
		{
			FileHandleIOS->SeekFromEnd(0);
		}
		return FileHandleIOS;
	}
	return NULL;
}

bool FIOSPlatformFile::DirectoryExists(const TCHAR* Directory)
{
	struct stat FileInfo;
	FString NormalizedDirectory = NormalizeFilename(Directory);
	if (stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedDirectory, false)), &FileInfo)== -1)
	{
		if (stat(TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedDirectory, true)), &FileInfo)== -1)
		{
			return false;
		}
	}
	return S_ISDIR(FileInfo.st_mode);
}

bool FIOSPlatformFile::CreateDirectory(const TCHAR* Directory)
{
	FString IOSDirectory = ConvertToIOSPath(NormalizeFilename(Directory), true);
	CFStringRef CFDirectory = FPlatformString::TCHARToCFString(*IOSDirectory);
	bool Result = [[NSFileManager defaultManager] createDirectoryAtPath:(NSString*)CFDirectory withIntermediateDirectories:true attributes:nil error:nil];
	CFRelease(CFDirectory);
	return Result;
}

bool FIOSPlatformFile::DeleteDirectory(const TCHAR* Directory)
{
	FString IOSDirectory = ConvertToIOSPath(NormalizeFilename(Directory), true);
	return rmdir(TCHAR_TO_UTF8(*IOSDirectory));
}

bool FIOSPlatformFile::IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor)
{
	bool Result = false;
	const ANSICHAR* FrameworksPath;
	if (Directory[0] == 0)
	{
		if ([[[[NSBundle mainBundle] bundlePath] pathExtension] isEqual: @"app"])
		{
			FrameworksPath = [[[NSBundle mainBundle] privateFrameworksPath] fileSystemRepresentation];
		}
		else
		{
			FrameworksPath = [[[NSBundle mainBundle] bundlePath] fileSystemRepresentation];
		}
	}

	FString NormalizedDirectory = NormalizeFilename(Directory);
	// If Directory is an empty string, assume that we want to iterate Binaries/Mac (current dir), but because we're an app bundle, iterate bundle's Contents/Frameworks instead
	DIR* Handle = opendir(Directory[0] ? TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedDirectory, false)) : FrameworksPath);
	if(!Handle)
	{
		// look in the write file path if it's not in the read file path
		Handle = opendir(Directory[0] ? TCHAR_TO_UTF8(*ConvertToIOSPath(NormalizedDirectory, true)) : FrameworksPath);
	}
	if (Handle)
	{
		Result = true;
		struct dirent *Entry;
		while ((Entry = readdir(Handle)) != NULL)
		{
			if (FCString::Strcmp(UTF8_TO_TCHAR(Entry->d_name), TEXT(".")) && FCString::Strcmp(UTF8_TO_TCHAR(Entry->d_name), TEXT("..")))
			{
				Result = Visitor.Visit(*(FString(Directory) / UTF8_TO_TCHAR(Entry->d_name)), Entry->d_type == DT_DIR);
			}
		}
		closedir(Handle);
	}
	return Result;
}

FString FIOSPlatformFile::ConvertToIOSPath(const FString& Filename, bool bForWrite)
{
	FString Result = Filename;
	Result.ReplaceInline(TEXT("../"), TEXT(""));
	Result.ReplaceInline(TEXT(".."), TEXT(""));
	Result.ReplaceInline(FPlatformProcess::BaseDir(), TEXT(""));

	if(bForWrite)
	{
		static FString WritePathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
		return WritePathBase + Result;
	}
	else
	{
		// if filehostip exists in the command line, cook on the fly read path should be used
		FString Value;
		// Cache this value as the command line doesn't change...
		static bool bHasHostIP = FParse::Value(FCommandLine::Get(), TEXT("filehostip"), Value) || FParse::Value(FCommandLine::Get(), TEXT("streaminghostip"), Value);
		if (bHasHostIP)
		{
			static FString ReadPathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
			return ReadPathBase + Result;
		}
		else
		{
			static FString ReadPathBase = FString([[NSBundle mainBundle] bundlePath]) + TEXT("/cookeddata/");
			return ReadPathBase + Result.ToLower();
		}
	}

	return Result;
}


/**
 * iOS platform file declaration
**/
IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
	static FIOSPlatformFile IOSPlatformSingleton;
	//static FSandboxPlatformFile CookOnTheFlySandboxSingleton(false);
	//static FSandboxPlatformFile CookByTheBookSandboxSingleton(false);
	//static bool bInitialized = false;
	//if(!bInitialized)
	//{
	//	NSString *DocumentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
	//	CookOnTheFlySandboxSingleton.Initialize(&IOSPlatformSingleton, ANSI_TO_TCHAR([DocumentsPath cStringUsingEncoding:NSASCIIStringEncoding]));

	//	DocumentsPath = [[NSBundle mainBundle] bundlePath];
	//	DocumentsPath = [DocumentsPath stringByAppendingPathComponent:@"/CookedContent/"];
	//	CookByTheBookSandboxSingleton.Initialize(&CookOnTheFlySandboxSingleton, ANSI_TO_TCHAR([DocumentsPath cStringUsingEncoding:NSASCIIStringEncoding]));

	//	bInitialized = true;
	//}

	//return CookByTheBookSandboxSingleton;

	return IOSPlatformSingleton;
}

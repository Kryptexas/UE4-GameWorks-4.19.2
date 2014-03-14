// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidFile.cpp: Android platform implementations of File functions
=============================================================================*/

#include "CorePrivate.h"

#include <dirent.h>
#include <jni.h>

// make an FTimeSpan object that represents the "epoch" for time_t (from a stat struct)
const FDateTime AndroidEpoch(1970, 1, 1);

// AndroidProcess uses this for executable name
FString GPackageName;
static int32 GPackageVersion = 0;
static int32 GPackagePatchVersion = 0;

//This function is declared in the Java-defined class, GameActivity.java: "public native void nativeSetObbInfo(String PackageName, int Version, int PatchVersion);"
extern "C" void Java_com_epicgames_ue4_GameActivity_nativeSetObbInfo(JNIEnv* jenv, jobject thiz, jstring PackageName, jint Version, jint PatchVersion)
{
	const char* JavaChars = jenv->GetStringUTFChars(PackageName, 0);
	GPackageName = UTF8_TO_TCHAR(JavaChars);
	GPackageVersion = Version;
	GPackagePatchVersion = PatchVersion;

	//Release the string
	jenv->ReleaseStringUTFChars(PackageName, JavaChars);
}

/** 
 * Android file handle implementation
**/
class CORE_API FFileHandleAndroid : public IFileHandle
{
	enum {READWRITE_SIZE = 1024 * 1024};
	int32 FileHandle;

	FORCEINLINE bool IsValid()
	{
		return FileHandle != -1;
	}

public:
	FFileHandleAndroid(int32 InFileHandle = -1)
		: FileHandle(InFileHandle)
	{
	}

	virtual ~FFileHandleAndroid()
	{
		close(FileHandle);
		FileHandle = -1;
	}

	virtual int64 Tell() OVERRIDE
	{
		check(IsValid());
		return lseek64(FileHandle, 0, SEEK_CUR);
	}

	virtual bool Seek(int64 NewPosition) OVERRIDE
	{
		check(IsValid());
		check(NewPosition >= 0);
		return lseek64(FileHandle, NewPosition, SEEK_SET) != -1;
	}

	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) OVERRIDE
	{
		check(IsValid());
		check(NewPositionRelativeToEnd <= 0);
		return lseek64(FileHandle, NewPositionRelativeToEnd, SEEK_END) != -1;
	}

	virtual bool Read(uint8* Destination, int64 BytesToRead) OVERRIDE
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

	virtual bool Write(const uint8* Source, int64 BytesToWrite) OVERRIDE
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
 * Android File I/O implementation
**/
class CORE_API FAndroidPlatformFile : public IPhysicalPlatformFile
{
protected:
	virtual FString NormalizeFilename(const TCHAR* Filename)
	{
		FString Result(Filename);
		Result.ReplaceInline(TEXT("\\"), TEXT("/"));
		return Result;
	}

	virtual FString NormalizeDirectory(const TCHAR* Directory)
	{
		FString Result(Directory);
		Result.ReplaceInline(TEXT("\\"), TEXT("/"));
		if (Result.EndsWith(TEXT("/")))
		{
			Result.LeftChop(1);
		}
		return Result;
	}

public:

	FAndroidPlatformFile()
		: bLookedForObbs(false)
	{
	}

	virtual bool FileExists(const TCHAR* Filename) OVERRIDE
	{
		struct stat FileInfo;
		FString NormalizedFilename = NormalizeFilename(Filename);
		// check the read path
		if (stat(TCHAR_TO_UTF8(*ConvertToAndroidPath(NormalizedFilename, false)), &FileInfo) == -1)
		{
			// if not in read path, check the write path
			if (stat(TCHAR_TO_UTF8(*ConvertToAndroidPath(NormalizedFilename, true)), &FileInfo) == -1)
			{
				return false;
			}
		}

		return S_ISREG(FileInfo.st_mode);
	}

	virtual int64 FileSize(const TCHAR* Filename) OVERRIDE
	{
		struct stat FileInfo;
		FileInfo.st_size = -1;
		FString NormalizedFilename = NormalizeFilename(Filename);
		// check the read path
		if(stat(TCHAR_TO_UTF8(*ConvertToAndroidPath(NormalizedFilename, false)), &FileInfo) == -1)
		{
			// if not in read path, check the write path
			stat(TCHAR_TO_UTF8(*ConvertToAndroidPath(NormalizedFilename, true)), &FileInfo);
		}

		// make sure to return -1 for directories
		if (S_ISDIR(FileInfo.st_mode))
		{
			FileInfo.st_size = -1;
		}
		return FileInfo.st_size;
	}

	virtual bool DeleteFile(const TCHAR* Filename) OVERRIDE
	{
		// only delete from write path
		FString AndroidFilename = ConvertToAndroidPath(NormalizeFilename(Filename), true);
		return unlink(TCHAR_TO_UTF8(*AndroidFilename)) == 0;
	}

	virtual bool IsReadOnly(const TCHAR* Filename) OVERRIDE
	{
		FString NormalizedFilename = NormalizeFilename(Filename);
		FString Filepath = ConvertToAndroidPath(NormalizedFilename, false);
		// check read path
		if (access(TCHAR_TO_UTF8(*Filepath), F_OK) == -1)
		{
			// if not in read path, check write path
			Filepath = ConvertToAndroidPath(NormalizedFilename, true);
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

	virtual bool MoveFile(const TCHAR* To, const TCHAR* From) OVERRIDE
	{
		// move to the write path
		FString ToAndroidFilename = ConvertToAndroidPath(NormalizeFilename(To), true);
		FString FromAndroidFilename = ConvertToAndroidPath(NormalizeFilename(From), false);
		return rename(TCHAR_TO_UTF8(*FromAndroidFilename), TCHAR_TO_UTF8(*ToAndroidFilename)) != -1;
	}

	virtual bool SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) OVERRIDE
	{
		struct stat FileInfo;
		FString AndroidFilename = ConvertToAndroidPath(NormalizeFilename(Filename), false);
		if (stat(TCHAR_TO_UTF8(*AndroidFilename), &FileInfo) != -1)
		{
			if (bNewReadOnlyValue)
			{
				FileInfo.st_mode &= ~S_IWUSR;
			}
			else
			{
				FileInfo.st_mode |= S_IWUSR;
			}
			return chmod(TCHAR_TO_UTF8(*AndroidFilename), FileInfo.st_mode);
		}
		return false;
	}

	virtual FDateTime GetTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		// get file times
		struct stat FileInfo;
		FString NormalizedFilename = NormalizeFilename(Filename);
		// check the read path
		FString AndroidFilename = ConvertToAndroidPath(NormalizedFilename, false);
		if(stat(TCHAR_TO_UTF8(*AndroidFilename), &FileInfo) == -1)
		{
			// if not in the read path, check the write path
			AndroidFilename = ConvertToAndroidPath(NormalizedFilename, true);
			if(stat(TCHAR_TO_UTF8(*AndroidFilename), &FileInfo) == -1)
			{
				return FDateTime::MinValue();
			}
		}

		// convert _stat time to FDateTime
		FTimespan TimeSinceEpoch(0, 0, FileInfo.st_mtime);
		return AndroidEpoch + TimeSinceEpoch;
	}

	virtual void SetTimeStamp(const TCHAR* Filename, const FDateTime DateTime) OVERRIDE
	{
		// get file times
		struct stat FileInfo;
		FString AndroidFilename = ConvertToAndroidPath(NormalizeFilename(Filename), true);
		if(stat(TCHAR_TO_UTF8(*AndroidFilename), &FileInfo) == -1)
		{
			return;
		}

		// change the modification time only
		struct utimbuf Times;
		Times.actime = FileInfo.st_atime;
		Times.modtime = (DateTime - AndroidEpoch).GetTotalSeconds();
		utime(TCHAR_TO_UTF8(*AndroidFilename), &Times);
	}

	virtual FDateTime GetAccessTimeStamp(const TCHAR* Filename) OVERRIDE
	{
		// get file times
		struct stat FileInfo;
		FString NormalizedFilename = NormalizeFilename(Filename);
		// check the read path
		if(stat(TCHAR_TO_UTF8(*ConvertToAndroidPath(NormalizedFilename, false)), &FileInfo) == -1)
		{
			// if not in the read path, check the write path
			if(stat(TCHAR_TO_UTF8(*ConvertToAndroidPath(NormalizedFilename, true)), &FileInfo) == -1)
			{
				return FDateTime::MinValue();
			}
		}

		// convert _stat time to FDateTime
		FTimespan TimeSinceEpoch(0, 0, FileInfo.st_atime);
		return AndroidEpoch + TimeSinceEpoch;
	}

	virtual IFileHandle* OpenRead(const TCHAR* Filename) OVERRIDE
	{
		FString NormalizedFilename = NormalizeFilename(Filename);
		// check the read path
		FString AndroidFilename = ConvertToAndroidPath(NormalizedFilename, false);
		int32 Handle = open(TCHAR_TO_UTF8(*AndroidFilename), O_RDONLY);
		if(Handle == -1)
		{
			// if not in the read path, check the write path
			FString AndroidFilename = ConvertToAndroidPath(NormalizedFilename, true);
			Handle = open(TCHAR_TO_UTF8(*AndroidFilename), O_RDONLY);
		}

		if (Handle != -1)
		{
			return new FFileHandleAndroid(Handle);
		}
		return NULL;
	}

	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead) OVERRIDE
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
		FString AndroidFilename = ConvertToAndroidPath(NormalizeFilename(Filename), true);
		int32 Handle = open(TCHAR_TO_UTF8(*AndroidFilename), Flags, S_IRUSR | S_IWUSR);
		if (Handle != -1)
		{
			FFileHandleAndroid* FileHandleAndroid = new FFileHandleAndroid(Handle);
			if (bAppend)
			{
				FileHandleAndroid->SeekFromEnd(0);
			}
			return FileHandleAndroid;
		}
		return NULL;
	}

	virtual bool DirectoryExists(const TCHAR* Directory) OVERRIDE
	{
		struct stat FileInfo;
		FString NormalizedDirectory = NormalizeFilename(Directory);
		FString AndroidDirectory = ConvertToAndroidPath(NormalizedDirectory, false);
		if (stat(TCHAR_TO_UTF8(*AndroidDirectory), &FileInfo)== -1)
		{
			FString AndroidDirectory = ConvertToAndroidPath(NormalizeFilename(Directory), true);
			if (stat(TCHAR_TO_UTF8(*AndroidDirectory), &FileInfo)== -1)
			{
				return false;
			}
		}
		return S_ISDIR(FileInfo.st_mode);
	}

	virtual bool CreateDirectory(const TCHAR* Directory) OVERRIDE
	{
		FString AndroidDirectory = ConvertToAndroidPath(NormalizeFilename(Directory), true);
		return mkdir(TCHAR_TO_UTF8(*AndroidDirectory), 0766) || (errno == EEXIST);
	}

	virtual bool DeleteDirectory(const TCHAR* Directory) OVERRIDE
	{
		FString AndroidDirectory = ConvertToAndroidPath(NormalizeFilename(Directory), true);
		return rmdir(TCHAR_TO_UTF8(*AndroidDirectory));
	}

	bool IterateDirectory(const TCHAR* Directory, FDirectoryVisitor& Visitor) OVERRIDE
	{
		// we haven't hijacked a directory yet, and this one ends in Paks, which means the engine is looking for .pak files
		if (!bLookedForObbs && FString(Directory).EndsWith("/Paks/"))
		{
			// never look again after this time
			bLookedForObbs = true;

			// look in the downloader path first, then manual location as a fallback
			for (int32 PassIndex = 0; PassIndex < 2 && ObbHijackedPakDirectory == TEXT(""); PassIndex++)
			{
				FString ObbPath = ((PassIndex == 0) ? FString(TEXT("/mnt/sdcard/Android/obb/")) : FString(TEXT("/mnt/sdcard/obb/"))) + GPackageName;

				DIR* Handle = opendir(TCHAR_TO_UTF8(*ObbPath));
				if (Handle)
				{
					bool Result = true;
					struct dirent *Entry;
					while ((Entry = readdir(Handle)) != NULL && Result == true)
					{
						if (FCStringAnsi::Strstr(Entry->d_name, ".obb") != NULL)
						{
							// convert to a ".pak" filename
							FString ObbFilename = ObbPath / FString(UTF8_TO_TCHAR(Entry->d_name));
							FString PakFilename = FString(Directory) / FString(UTF8_TO_TCHAR(Entry->d_name)) + TEXT(".pak");
							ObbRemaps.Add(PakFilename, ObbFilename);

							FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Obb found at '%s', pretending to be '%s'\n"), *ObbFilename, *PakFilename);

							// remember which directory we are hijacking only if we found a .obb file
							ObbHijackedPakDirectory = Directory;
						}
					}
					closedir(Handle);
				}
				else
				{
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Obb directory not found\n"));
				}
			}
		}

		// we've already hijacked a directory for reporting .obbs, and this is that directory again, so just return the already known "pak" files
		if (bLookedForObbs && ObbHijackedPakDirectory == Directory)
		{
			bool Result = true;
			for (auto It = ObbRemaps.CreateIterator(); It && Result == true; ++It)
			{
				Result = Visitor.Visit(*It.Key(), false);
			}
			return true;
		}

		FString AndroidDirectory = ConvertToAndroidPath(NormalizeFilename(Directory), false);
		DIR* Handle = opendir(TCHAR_TO_UTF8(*AndroidDirectory));
		if (Handle)
		{
			bool Result = true;
			struct dirent *Entry;
			while ((Entry = readdir(Handle)) != NULL && Result == true)
			{
				if (FCString::Strcmp(UTF8_TO_TCHAR(Entry->d_name), TEXT(".")) && FCString::Strcmp(UTF8_TO_TCHAR(Entry->d_name), TEXT("..")))
				{
					Result = Visitor.Visit(*(FString(Directory) / UTF8_TO_TCHAR(Entry->d_name)), Entry->d_type == DT_DIR);
				}
			}
			closedir(Handle);
			return true;
		}

		return false;
	}

	FString ConvertToAndroidPath(const FString& Filename, bool bForWrite)
	{
		// if we were pretending an .obb was a pak somewhere, return that
		FString* RemappedObb = ObbRemaps.Find(Filename);
		if (RemappedObb)
		{
			return *RemappedObb;
		}

		FString Result = Filename;
		Result.ReplaceInline(TEXT("../"), TEXT(""));
		Result.ReplaceInline(TEXT(".."), TEXT(""));
		Result.ReplaceInline(FPlatformProcess::BaseDir(), TEXT(""));

		static FString BasePath = FString("/mnt/sdcard/") + GGameName + FString("/");
		Result =  BasePath + Result;
		return Result;
	}

	FString ReadDirectory;
	FString WriteDirectory;
	
	/** 
	 * Because the .obb file is simply a .pak file, we redirect the engine looking for .pak files to actually find
	 * .obb files instead. These variables track that remapping, as it happens only the first time a query into a Paks
	 * directory is made
	*/
	FString ObbHijackedPakDirectory;
	TMap<FString, FString> ObbRemaps;
	bool bLookedForObbs;
};

/**
	* Android platform file declaration
**/
IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
	static FAndroidPlatformFile AndroidPlatformSingleton;
	return AndroidPlatformSingleton;
}

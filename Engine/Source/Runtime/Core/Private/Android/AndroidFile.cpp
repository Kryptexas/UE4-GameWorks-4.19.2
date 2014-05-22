// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidFile.cpp: Android platform implementations of File functions
=============================================================================*/

#include "CorePrivate.h"

#include <dirent.h>
#include <jni.h>
#include <Android/asset_manager.h>
#include <Android/asset_manager_jni.h>

// make an FTimeSpan object that represents the "epoch" for time_t (from a stat struct)
const FDateTime AndroidEpoch(1970, 1, 1);

// AndroidProcess uses this for executable name
FString GPackageName;
static int32 GPackageVersion = 0;
static int32 GPackagePatchVersion = 0;

// External File Path base - setup during load
FString GFilePathBase;

// Is the OBB in an APK file or not
bool GOBBinAPK;

extern AAssetManager * AndroidThunkCpp_GetAssetManager();

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
* Android file handle implementation for files in Pak/OBB data
**/
class CORE_API FFileHandlePakAndroid : public IFileHandle
{
	enum { READWRITE_SIZE = 1024 * 1024 };
	AAsset * File;
	int32 FileHandle;
	off_t Start;
	off_t Length;

	FORCEINLINE bool IsValid()
	{
		return FileHandle != -1;
	}

public:
	FFileHandlePakAndroid(AAsset * file)
		: File(file), FileHandle(0), Start(0), Length(0)
	{
		FileHandle = AAsset_openFileDescriptor(file, &Start, &Length);
	}

	virtual ~FFileHandlePakAndroid()
	{
		close(FileHandle);
		FileHandle = -1;
		AAsset_close(File);
	}

	virtual int64 Tell() OVERRIDE
	{
		check(IsValid());
		int64 pos = lseek(FileHandle, 0, SEEK_CUR);
		return pos - Start; // We are treating 'tell' as a virtual location from file Start
	}

	virtual bool Seek(int64 NewPosition) OVERRIDE
	{
		check(IsValid());
		// we need to offset all positions by the Start offset
		NewPosition += Start;
		check(NewPosition >= 0);
		
		return lseek(FileHandle, NewPosition, SEEK_SET) != -1;
	}

    virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) OVERRIDE
	{
		check(IsValid());
		check(NewPositionRelativeToEnd <= 0);
		// We need to convert this to a virtual offset inside the file we are interested in
		int64 position = Start + (Length - NewPositionRelativeToEnd);
		return lseek(FileHandle, position, SEEK_SET) != -1;
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

	virtual int64 Size()
	{
		return Length;
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
		: AssetMgr(nullptr)
		, bLookedForObbs(false)
	{
		AssetMgr = AndroidThunkCpp_GetAssetManager();
	}

	virtual bool FileExists(const TCHAR* Filename) OVERRIDE
	{
		struct stat FileInfo;
		FString NormalizedFilename = NormalizeFilename(Filename);
		// check the read path
		FString convertedPath = ConvertToAndroidPath(NormalizedFilename, false);
		if (stat(TCHAR_TO_UTF8(*convertedPath), &FileInfo) == -1)
		{
			// if not in read path, check the write path
			convertedPath = ConvertToAndroidPath(NormalizedFilename, true);
			if (stat(TCHAR_TO_UTF8(*convertedPath), &FileInfo) == -1)
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
		FString convertedPath = ConvertToAndroidPath(NormalizedFilename, false);
		// See if we need to route via the asset path
		if (convertedPath.Contains(TEXT(".pak")) && GOBBinAPK)
		{
			AAsset * file = AAssetManager_open(AssetMgr, TCHAR_TO_UTF8(*convertedPath), AASSET_MODE_RANDOM); // seems reasonable?
			FileInfo.st_size = AAsset_getLength(file);
			AAsset_close(file);
		}
		else
		{
			if (stat(TCHAR_TO_UTF8(*convertedPath), &FileInfo) == -1)
			{
				// if not in read path, check the write path
				convertedPath = ConvertToAndroidPath(NormalizedFilename, true);
				stat(TCHAR_TO_UTF8(*convertedPath), &FileInfo);
			}

			// make sure to return -1 for directories
			if (S_ISDIR(FileInfo.st_mode))
			{
				FileInfo.st_size = -1;
			}
		}
		return FileInfo.st_size;
	}

	virtual bool DeleteFile(const TCHAR* Filename) OVERRIDE
	{
		// only delete from write path
		FString AndroidFilename = ConvertToAndroidPath(NormalizeFilename(Filename), true);
		// lets not try and delete things if we are pulling files from the APK
		if (AndroidFilename.Contains(TEXT(".pak")) && GOBBinAPK)
			return false;

		return unlink(TCHAR_TO_UTF8(*AndroidFilename)) == 0;
	}

	virtual bool IsReadOnly(const TCHAR* Filename) OVERRIDE
	{
		FString NormalizedFilename = NormalizeFilename(Filename);
		FString Filepath = ConvertToAndroidPath(NormalizedFilename, false);
		
		// If we are pulling the pak files from the APK they are ALWAYS read only
		if (Filepath.Contains(TEXT(".pak")) && GOBBinAPK)
		{
			return true;
		}
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
		// Can't move pak files in the APK
		if (ToAndroidFilename.Contains(TEXT(".pak")) && GOBBinAPK)
			return false;

		FString FromAndroidFilename = ConvertToAndroidPath(NormalizeFilename(From), false);
		return rename(TCHAR_TO_UTF8(*FromAndroidFilename), TCHAR_TO_UTF8(*ToAndroidFilename)) != -1;
	}

	virtual bool SetReadOnly(const TCHAR* Filename, bool bNewReadOnlyValue) OVERRIDE
	{
		struct stat FileInfo;
		FString AndroidFilename = ConvertToAndroidPath(NormalizeFilename(Filename), false);
		
		// We can't change the state of pak in APK files - so indicate failure
		if (AndroidFilename.Contains(TEXT(".pak")) && GOBBinAPK)
			return false;

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

		// No TimeStamp for pak files in APK, so just return a default timespan for now
		if (AndroidFilename.Contains(TEXT(".pak")) && GOBBinAPK)
			return AndroidEpoch;

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
		
		// Can't set time stamp on pak files in APK
		if (AndroidFilename.Contains(TEXT(".pak")) && GOBBinAPK)
			return ;
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
		FString convertedPath = ConvertToAndroidPath(NormalizedFilename, false);
		// No TimeStamp for pak files in APK, so just return a default timespan for now
		if (convertedPath.Contains(TEXT(".pak")) && GOBBinAPK)
			return AndroidEpoch;

		if(stat(TCHAR_TO_UTF8(*convertedPath), &FileInfo) == -1)
		{
			// if not in the read path, check the write path
			convertedPath = ConvertToAndroidPath(NormalizedFilename, true);
			if (stat(TCHAR_TO_UTF8(*convertedPath), &FileInfo) == -1)
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
		// No TimeStamp for pak files in APK, so just return a default timespan for now
		if (AndroidFilename.Contains(TEXT(".pak")) && GOBBinAPK)
		{
			AAsset * file = AAssetManager_open(AssetMgr, TCHAR_TO_UTF8(*AndroidFilename), AASSET_MODE_RANDOM); // seems reasonable?
			return new FFileHandlePakAndroid(file);
		}
		else
		{
			int32 Handle = open(TCHAR_TO_UTF8(*AndroidFilename), O_RDONLY);
			if (Handle == -1)
			{
				// if not in the read path, check the write path
				AndroidFilename = ConvertToAndroidPath(NormalizedFilename, true);
				Handle = open(TCHAR_TO_UTF8(*AndroidFilename), O_RDONLY);
			}

			if (Handle != -1)
			{
				return new FFileHandleAndroid(Handle);
			}
		}
		return NULL;
	}

	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead) OVERRIDE
	{
		FString AndroidFilename = ConvertToAndroidPath(NormalizeFilename(Filename), true);
		if (AndroidFilename.Contains(TEXT(".pak")) && GOBBinAPK)
		{
			return nullptr; // Can't open pak files in the APK for writing
		}

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
			AndroidDirectory = ConvertToAndroidPath(NormalizeFilename(Directory), true);
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
			// If we aren't looking in the APK for the OBB then go looking in the expansion directory...
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Obb %s.\n"), GOBBinAPK ? TEXT("in APK") : TEXT("in external dir."));
			if (!GOBBinAPK)
			{
				FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Looking for Obb in external dir.\n"));
				// look in the downloader path first, then manual location as a fallback
				for (int32 PassIndex = 0; PassIndex < 2 && ObbHijackedPakDirectory == TEXT(""); PassIndex++)
				{
					FString ObbPath = GFilePathBase + ((PassIndex == 0) ? FString(TEXT("/Android/obb/")) : FString(TEXT("/obb/"))) + GPackageName;

					DIR* Handle = opendir(TCHAR_TO_UTF8(*ObbPath));
					if (Handle)
					{
						struct dirent *Entry;
						while ((Entry = readdir(Handle)) != NULL)
						{
							if (FCStringAnsi::Strstr(Entry->d_name, ".obb") != NULL)
							{
								// convert to a ".pak" filename
								FString ObbFileName = ObbPath / FString(UTF8_TO_TCHAR(Entry->d_name));
								FString PakFileName = FString(Directory) / FString(UTF8_TO_TCHAR(Entry->d_name)) + TEXT(".pak");
								ObbRemaps.Add(PakFileName, ObbFileName);

								FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Obb found at '%s', pretending to be '%s'\n"), *ObbFileName, *PakFileName);

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
			else // otherwise we need to open the assets directory to find the target file + deal with renaming
			{
				FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Looking for Obb in APK file.\n"));
				AAssetDir * topDir = AAssetManager_openDir(AssetMgr, ""); // grab the top level dir
				if (topDir)
				{
					const char * fileName = NULL;
					while ((fileName = AAssetDir_getNextFileName(topDir)) != NULL)
					{
						const char * pakPosition = NULL;
						if ((pakPosition = strstr(fileName, ".pak")) != 0)
						{
							FString ObbFileName = FString(UTF8_TO_TCHAR(fileName)); // The real asset file name
							FString PakFileName = FString(Directory) / FString(UTF8_TO_TCHAR(fileName));
							PakFileName.RemoveFromEnd(TEXT(".png")); // strip the '.png' from the end
							ObbRemaps.Add(PakFileName, ObbFileName);

							FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Obb found at '%s', pretending to be '%s'\n"), *ObbFileName, *PakFileName);

							// remember which directory we are hijacking only if we found a .pak file
							ObbHijackedPakDirectory = Directory;
						}
					}
					AAssetDir_close(topDir);
				}

				if (ObbHijackedPakDirectory == TEXT(""))
				{
					FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Failed to find PAK in APK file.\n"));
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

		static FString BasePath = GFilePathBase + FString("/") + GGameName + FString("/");
		Result =  BasePath + Result;
		return Result;
	}

	FString ReadDirectory;
	FString WriteDirectory;
	
	AAssetManager* AssetMgr;

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

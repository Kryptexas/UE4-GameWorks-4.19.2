#include "UnrealAudioPrivate.h"
#include "UnrealAudioSoundFile.h"
#include "UnrealAudioSoundFileInternal.h"
#include "UnrealAudioModule.h"
#include "ModuleManager.h"

#if ENABLE_UNREAL_AUDIO

namespace UAudio
{
	// Virtual Sound File Function Pointers
	typedef SoundFileCount(*VirtualSoundFileGetLengthFuncPtr)(void* UserData);
	typedef SoundFileCount(*VirtualSoundFileSeekFuncPtr)(SoundFileCount Offset, int32 Mode, void* UserData);
	typedef SoundFileCount(*VirtualSoundFileReadFuncPtr)(void* DataPtr, SoundFileCount ByteCount, void* UserData);
	typedef SoundFileCount(*VirtualSoundFileWriteFuncPtr)(const void* DataPtr, SoundFileCount ByteCount, void* UserData);
	typedef SoundFileCount(*VirtualSoundFileTellFuncPtr)(void* UserData);

	// Struct describing function pointers to call for virtual file IO
	struct FVirtualSoundFileCallbackInfo
	{
		VirtualSoundFileGetLengthFuncPtr VirtualSoundFileGetLength;
		VirtualSoundFileSeekFuncPtr VirtualSoundFileSeek;
		VirtualSoundFileReadFuncPtr VirtualSoundFileRead;
		VirtualSoundFileWriteFuncPtr VirtualSoundFileWrite;
		VirtualSoundFileTellFuncPtr VirtualSoundFileTell;
	};

	// SoundFile Constants
	static const int32 SET_ENCODING_QUALITY = 0x1300;
	static const int32 SET_CHANNEL_MAP_INFO = 0x1101;
	static const int32 GET_CHANNEL_MAP_INFO = 0x1100;

	// Exported SoundFile Functions
	typedef SoundFileHandle*(*SoundFileOpenFuncPtr)(const char* Path, int32 Mode, FSoundFileDescription* Description);
	typedef SoundFileHandle*(*SoundFileOpenVirtualFuncPtr)(FVirtualSoundFileCallbackInfo* VirtualFileDescription, int32 Mode, FSoundFileDescription* Description, void* UserData);
	typedef int32(*SoundFileCloseFuncPtr)(SoundFileHandle* FileHandle);
	typedef int32(*SoundFileErrorFuncPtr)(SoundFileHandle* FileHandle);
	typedef const char*(*SoundFileStrErrorFuncPtr)(SoundFileHandle* FileHandle);
	typedef const char*(*SoundFileErrorNumberFuncPtr)(int32 ErrorNumber);
	typedef int32(*SoundFileCommandFuncPtr)(SoundFileHandle* FileHandle, int32 Command, void* Data, int32 DataSize);
	typedef int32(*SoundFileFormatCheckFuncPtr)(const FSoundFileDescription* Description);
	typedef SoundFileCount(*SoundFileSeekFuncPtr)(SoundFileHandle* FileHandle, SoundFileCount NumFrames, int32 SeekMode);
	typedef const char*(*SoundFileGetVersionFuncPtr)(void);
	typedef SoundFileCount(*SoundFileReadFramesFloatFuncPtr)(SoundFileHandle* FileHandle, float* Buffer, SoundFileCount NumFrames);
	typedef SoundFileCount(*SoundFileReadFramesDoubleFuncPtr)(SoundFileHandle* FileHandle, double* Buffer, SoundFileCount NumFrames);
	typedef SoundFileCount(*SoundFileWriteFramesFloatFuncPtr)(SoundFileHandle* FileHandle, const float* Buffer, SoundFileCount NumFrames);
	typedef SoundFileCount(*SoundFileWriteFramesDoubleFuncPtr)(SoundFileHandle* FileHandle, const double* Buffer, SoundFileCount NumFrames);
	typedef SoundFileCount(*SoundFileReadSamplesFloatFuncPtr)(SoundFileHandle* FileHandle, float* Buffer, SoundFileCount NumSamples);
	typedef SoundFileCount(*SoundFileReadSamplesDoubleFuncPtr)(SoundFileHandle* FileHandle, double* Buffer, SoundFileCount NumSamples);
	typedef SoundFileCount(*SoundFileWriteSamplesFloatFuncPtr)(SoundFileHandle* FileHandle, const float* Buffer, SoundFileCount NumSamples);
	typedef SoundFileCount(*SoundFileWriteSamplesDoubleFuncPtr)(SoundFileHandle* FileHandle, const double* Buffer, SoundFileCount NumSamples);

	SoundFileOpenFuncPtr SoundFileOpen = nullptr;
	SoundFileOpenVirtualFuncPtr SoundFileOpenVirtual = nullptr;
	SoundFileCloseFuncPtr SoundFileClose = nullptr;
	SoundFileErrorFuncPtr SoundFileError = nullptr;
	SoundFileStrErrorFuncPtr SoundFileStrError = nullptr;
	SoundFileErrorNumberFuncPtr SoundFileErrorNumber = nullptr;
	SoundFileCommandFuncPtr SoundFileCommand = nullptr;
	SoundFileFormatCheckFuncPtr SoundFileFormatCheck = nullptr;
	SoundFileSeekFuncPtr SoundFileSeek = nullptr;
	SoundFileGetVersionFuncPtr SoundFileGetVersion = nullptr;
	SoundFileReadFramesFloatFuncPtr SoundFileReadFramesFloat = nullptr;
	SoundFileReadFramesDoubleFuncPtr SoundFileReadFramesDouble = nullptr;
	SoundFileWriteFramesFloatFuncPtr SoundFileWriteFramesFloat = nullptr;
	SoundFileWriteFramesDoubleFuncPtr SoundFileWriteFramesDouble = nullptr;
	SoundFileReadSamplesFloatFuncPtr SoundFileReadSamplesFloat = nullptr;
	SoundFileReadSamplesDoubleFuncPtr SoundFileReadSamplesDouble = nullptr;
	SoundFileWriteSamplesFloatFuncPtr SoundFileWriteSamplesFloat = nullptr;
	SoundFileWriteSamplesDoubleFuncPtr SoundFileWriteSamplesDouble = nullptr;

	/** 
	Function implementations of virtual function callbacks
	*/

	static SoundFileCount OnSoundFileGetLengthBytes(void* UserData)
	{
		SoundFileCount Length = 0;
		((ISoundFileVirtual*)UserData)->GetLengthBytes(Length);
		return Length;
	}

	static SoundFileCount OnSoundFileSeekBytes(SoundFileCount Offset, int32 Mode, void* UserData)
	{
		SoundFileCount OutOffset = 0;
		((ISoundFileVirtual*)UserData)->SeekBytes(Offset, (ESoundFileSeekMode::Type)Mode, OutOffset);
		return OutOffset;
	}

	static SoundFileCount OnSoundFileReadBytes(void* DataPtr, SoundFileCount ByteCount, void* UserData)
	{
		SoundFileCount OutBytesRead = 0;
		((ISoundFileVirtual*)UserData)->ReadBytes(DataPtr, ByteCount, OutBytesRead);
		return OutBytesRead;
	}

	static SoundFileCount OnSoundFileWriteBytes(const void* DataPtr, SoundFileCount ByteCount, void* UserData)
	{
		SoundFileCount OutBytesWritten = 0;
		((ISoundFileVirtual*)UserData)->WriteBytes(DataPtr, ByteCount, OutBytesWritten);
		return OutBytesWritten;
	}

	static SoundFileCount OnSoundFileTell(void* UserData)
	{
		return ((ISoundFileVirtual*)UserData)->GetOffsetBytes();
	}

	/**
	* Gets the default channel mapping for the given channel number
	*/
	static void GetDefaultMappingsForChannelNumber(int32 NumChannels, TArray<ESoundFileChannelMap::Type>& ChannelMap)
	{
		check(ChannelMap.Num() == NumChannels);

		switch (NumChannels)
		{
			case 1:	// MONO
				ChannelMap[0] = ESoundFileChannelMap::MONO;
				break;

			case 2:	// STEREO
				ChannelMap[0] = ESoundFileChannelMap::LEFT;
				ChannelMap[1] = ESoundFileChannelMap::RIGHT;
				break;

			case 3:	// 2.1
				ChannelMap[0] = ESoundFileChannelMap::LEFT;
				ChannelMap[1] = ESoundFileChannelMap::RIGHT;
				ChannelMap[2] = ESoundFileChannelMap::LFE;
				break;

			case 4: // Quadraphonic
				ChannelMap[0] = ESoundFileChannelMap::LEFT;
				ChannelMap[1] = ESoundFileChannelMap::RIGHT;
				ChannelMap[2] = ESoundFileChannelMap::BACK_LEFT;
				ChannelMap[3] = ESoundFileChannelMap::BACK_RIGHT;
				break;

			case 5: // 5.0
				ChannelMap[0] = ESoundFileChannelMap::LEFT;
				ChannelMap[1] = ESoundFileChannelMap::RIGHT;
				ChannelMap[2] = ESoundFileChannelMap::CENTER;
				ChannelMap[3] = ESoundFileChannelMap::SIDE_LEFT;
				ChannelMap[4] = ESoundFileChannelMap::SIDE_RIGHT;
				break;

			case 6: // 5.1
				ChannelMap[0] = ESoundFileChannelMap::LEFT;
				ChannelMap[1] = ESoundFileChannelMap::RIGHT;
				ChannelMap[2] = ESoundFileChannelMap::CENTER;
				ChannelMap[3] = ESoundFileChannelMap::LFE;
				ChannelMap[4] = ESoundFileChannelMap::SIDE_LEFT;
				ChannelMap[5] = ESoundFileChannelMap::SIDE_RIGHT;
				break;

			case 7: // 6.1
				ChannelMap[0] = ESoundFileChannelMap::LEFT;
				ChannelMap[1] = ESoundFileChannelMap::RIGHT;
				ChannelMap[2] = ESoundFileChannelMap::CENTER;
				ChannelMap[3] = ESoundFileChannelMap::LFE;
				ChannelMap[4] = ESoundFileChannelMap::SIDE_LEFT;
				ChannelMap[5] = ESoundFileChannelMap::SIDE_RIGHT;
				ChannelMap[6] = ESoundFileChannelMap::BACK_CENTER;
				break;

			case 8: // 7.1
				ChannelMap[0] = ESoundFileChannelMap::LEFT;
				ChannelMap[1] = ESoundFileChannelMap::RIGHT;
				ChannelMap[2] = ESoundFileChannelMap::CENTER;
				ChannelMap[3] = ESoundFileChannelMap::LFE;
				ChannelMap[4] = ESoundFileChannelMap::BACK_LEFT;
				ChannelMap[5] = ESoundFileChannelMap::BACK_RIGHT;
				ChannelMap[6] = ESoundFileChannelMap::SIDE_LEFT;
				ChannelMap[7] = ESoundFileChannelMap::SIDE_RIGHT;
				break;

			default:
				break;
		}
	}

	static int32 SoundFileId = 0;

	FSoundFile::FSoundFile()
		: CurrentIndexBytes(0)
		, FileHandle(nullptr)
		, State(ESoundFileState::UNINITIALIZED)
		, CurrentError(ESoundFileError::NONE)
		, Id(SoundFileId++)
	{
	}

	FSoundFile::~FSoundFile()
	{
		check(FileHandle == nullptr);
	}

	ESoundFileError::Type FSoundFile::GetSoundFileData(TSharedPtr<ISoundFileData>& OutSoundFileData)
	{
		if (State == ESoundFileState::IMPORTING)
		{
			return SetError(ESoundFileError::ALREADY_INTIALIZED);
		}

		OutSoundFileData = SoundFileData;
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::Initialize(TSharedPtr<ISoundFileData> InSoundFileData)
	{
		if (State != ESoundFileState::UNINITIALIZED)
		{
			return SetError(ESoundFileError::ALREADY_INTIALIZED);
		}

		check(FileHandle == nullptr);

		// Setting sound file data initializes this sound file
		SoundFileData = InSoundFileData;

		// Open up a virtual file handle with this data
		FVirtualSoundFileCallbackInfo VirtualSoundFileInfo;
		VirtualSoundFileInfo.VirtualSoundFileGetLength = OnSoundFileGetLengthBytes;
		VirtualSoundFileInfo.VirtualSoundFileSeek = OnSoundFileSeekBytes;
		VirtualSoundFileInfo.VirtualSoundFileRead = OnSoundFileReadBytes;
		VirtualSoundFileInfo.VirtualSoundFileWrite = OnSoundFileWriteBytes;
		VirtualSoundFileInfo.VirtualSoundFileTell = OnSoundFileTell;

		FSoundFileDescription Description = SoundFileData->GetDescription();
		if (!SoundFileFormatCheck(&Description))
		{
			return SetError(ESoundFileError::INVALID_INPUT_FORMAT);
		}

		FileHandle = SoundFileOpenVirtual(&VirtualSoundFileInfo, ESoundFileOpenMode::READING, &Description, (void*)this);
		if (!FileHandle)
		{
			FString StrErr = SoundFileStrError(nullptr);
			UE_LOG(LogUnrealAudio, Error, TEXT("Failed to intitialize sound file: %s"), *StrErr);
			return SetError(ESoundFileError::FAILED_TO_OPEN);
		}

		State = ESoundFileState::INITIALIZED;
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::GetName(FString& Name)
	{
		if (SoundFileData.IsValid())
		{
			const FString& FilePath = SoundFileData->GetFilePath();
			Name = FPaths::GetBaseFilename(FilePath);
			return ESoundFileError::NONE;
		}
		return ESoundFileError::INVALID_DATA;
	}

	ESoundFileState::Type FSoundFile::GetState() const
	{
		return State;
	}

	ESoundFileError::Type FSoundFile::GetError() const
	{
		return CurrentError;
	}

	ESoundFileError::Type FSoundFile::SetError(ESoundFileError::Type Error)
	{
		if (Error != ESoundFileError::NONE)
		{
			State = ESoundFileState::HAS_ERROR;
		}
		return (CurrentError = Error);
	}

	ESoundFileError::Type FSoundFile::OpenFileForReading(const FString& FilePath)
	{
		if (FileHandle != nullptr)
		{
			UE_LOG(LogUnrealAudio, Error, TEXT("Sound file already opened"));
			return SetError(ESoundFileError::ALREADY_OPENED);
		}

		// Make sure we don't already have data
		if (SoundFileData.IsValid())
		{
			return SetError(ESoundFileError::ALREADY_HAS_DATA);
		}

		// Make a data ptr
		SoundFileData = TSharedPtr<FSoundFileData>(new FSoundFileData(ESoundFileOpenMode::READING));
		SoundFileData->SetFilePath(FilePath);

		// Check to see if the file exists
		if (!FPaths::FileExists(FilePath))
		{
			UE_LOG(LogUnrealAudio, Error, TEXT("Sound file %s doesn't exist."), *FilePath);
			return SetError(ESoundFileError::FILE_DOESNT_EXIST);
		}

		// Open up the file
		FSoundFileDescription Description;
		FileHandle = SoundFileOpen(TCHAR_TO_ANSI(*FilePath), ESoundFileOpenMode::READING, &Description);
		if (!FileHandle)
		{
			FString StrError = FString(SoundFileStrError(nullptr));
			UE_LOG(LogUnrealAudio, Error, TEXT("Failed to open sound file %s: %s"), *FilePath, *StrError);
			return SetError(ESoundFileError::FAILED_TO_OPEN);
		}
		SoundFileData->SetDescription(Description);

		// Try to get a channel mapping
		int32 NumChannels = Description.NumChannels;
		TArray<ESoundFileChannelMap::Type> ChannelMap;
		ChannelMap.Init(ESoundFileChannelMap::INVALID, NumChannels);

		int32 Result = SoundFileCommand(FileHandle, GET_CHANNEL_MAP_INFO, (int32*)ChannelMap.GetData(), sizeof(int32)*NumChannels);

		// If we failed to get the file's channel map definition, then we set the default based on the number of channels
		if (Result == 0)
		{
			GetDefaultMappingsForChannelNumber(NumChannels, ChannelMap);
		}
		else
		{
			// Check to see if the channel map we did get back is filled with INVALID channels
			bool bIsInvalid = false;
			for (ESoundFileChannelMap::Type ChannelType : ChannelMap)
			{
				if (ChannelType == ESoundFileChannelMap::INVALID)
				{
					bIsInvalid = true;
					break;
				}
			}
			// If invalid, then we need to get the default channel mapping
			if (bIsInvalid)
			{
				GetDefaultMappingsForChannelNumber(NumChannels, ChannelMap);
			}
		}
		SoundFileData->SetChannelMap(ChannelMap);

		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::OpenEmptyFileForImport(const FSoundFileDescription& InDescription, const TArray<ESoundFileChannelMap::Type>& InChannelMap)
	{
		// First check the input format to make sure it's valid
		if (!SoundFileFormatCheck(&InDescription))
		{
			UE_LOG(LogUnrealAudio, 
				   Error, 
				   TEXT("Sound file input format (%s - %s) is invalid."), 
				   ESoundFileFormat::ToStringMajor(InDescription.FormatFlags), 
				   ESoundFileFormat::ToStringMinor(InDescription.FormatFlags));

			return SetError(ESoundFileError::INVALID_INPUT_FORMAT);
		}
		
		// Make sure we don't already have data
		if (SoundFileData.IsValid())
		{
			return SetError(ESoundFileError::ALREADY_HAS_DATA);
		}
		
		// Make sure we have the right number of channels and our channel map size
		if (InChannelMap.Num() != InDescription.NumChannels)
		{
			UE_LOG(LogUnrealAudio, Error, TEXT("Channel map didn't match the input NumChannels"));
			return (CurrentError = ESoundFileError::INVALID_CHANNEL_MAP);
		}
		
		// Make a data ptr
		SoundFileData = TSharedPtr<FSoundFileData>(new FSoundFileData(ESoundFileOpenMode::WRITING));

		FVirtualSoundFileCallbackInfo VirtualSoundFileInfo;
		VirtualSoundFileInfo.VirtualSoundFileGetLength = OnSoundFileGetLengthBytes;
		VirtualSoundFileInfo.VirtualSoundFileSeek = OnSoundFileSeekBytes;
		VirtualSoundFileInfo.VirtualSoundFileRead = OnSoundFileReadBytes;
		VirtualSoundFileInfo.VirtualSoundFileWrite = OnSoundFileWriteBytes;
		VirtualSoundFileInfo.VirtualSoundFileTell = OnSoundFileTell;

		FSoundFileDescription Description = InDescription;
		FileHandle = SoundFileOpenVirtual(&VirtualSoundFileInfo, ESoundFileOpenMode::WRITING, &Description, (void*)this);
		if (!FileHandle)
		{
			FString StrErr = FString(SoundFileStrError(nullptr));
			UE_LOG(LogUnrealAudio, Error, TEXT("Failed to open empty sound file: %s"), *StrErr);
			return SetError(ESoundFileError::FAILED_TO_OPEN);
		}

		// For some reason, virtual sound files set the NumFrames to be the max int64 value, so we're going to set it back to 0
		// since at this point, the number of frames is 0!
		SoundFileData->SetDescription(Description);

		// Set the channel map on the virtual file
		SoundFileData->SetChannelMap(InChannelMap);
		int32 Result = SoundFileCommand(FileHandle, SET_CHANNEL_MAP_INFO, (int32*)InChannelMap.GetData(), sizeof(int32)*Description.NumChannels);
		if (Result)
		{
			FString StrErr = SoundFileStrError(nullptr);
			UE_LOG(LogUnrealAudio, Error, TEXT("Failed to set the channel map on empty file for writing: %s"), *StrErr);
			return SetError(ESoundFileError::INVALID_CHANNEL_MAP);
		}

		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::OpenEmptyFileForExport(const FString& FilePath, const FSoundFileDescription& InDescription, const TArray<ESoundFileChannelMap::Type>& InChannelMap)
	{
		// First check the input format to make sure it's valid
		if (!SoundFileFormatCheck(&InDescription))
		{
			UE_LOG(LogUnrealAudio,
				   Error,
				   TEXT("Sound file input format (%s - %s) is invalid."),
				   ESoundFileFormat::ToStringMajor(InDescription.FormatFlags),
				   ESoundFileFormat::ToStringMinor(InDescription.FormatFlags));

			return SetError(ESoundFileError::INVALID_INPUT_FORMAT);
		}

		// Make sure we have the right number of channels and our channel map size
		if (InChannelMap.Num() != InDescription.NumChannels)
		{
			UE_LOG(LogUnrealAudio, Error, TEXT("Channel map didn't match the input NumChannels"));
			return SetError(ESoundFileError::INVALID_CHANNEL_MAP);
		}

		// Open up the file
		FSoundFileDescription Description = InDescription;
		FileHandle = SoundFileOpen(TCHAR_TO_ANSI(*FilePath), ESoundFileOpenMode::WRITING, &Description);
		if (!FileHandle)
		{
			FString StrErr = SoundFileStrError(nullptr);
			UE_LOG(LogUnrealAudio, Error, TEXT("Failed to open sound file %s: %s"), *FilePath, *StrErr);
			return SetError(ESoundFileError::FAILED_TO_OPEN);
		}

		return ESoundFileError::NONE;
	}


	ESoundFileError::Type FSoundFile::ReleaseSoundFileHandle()
	{
		if (FileHandle)
		{
			SoundFileClose(FileHandle);
			FileHandle = nullptr;
		}
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::SetImportFileInfo(const FSoundFileDescription& FileDescription, const FString& FullPath)
	{
		if (!SoundFileData.IsValid())
		{
			return SetError(ESoundFileError::INVALID_DATA);
		}

		SoundFileData->SetOriginalDescription(FileDescription);
		SoundFileData->SetFilePath(FullPath);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::SetEncodingQuality(double EncodingQuality)
	{
		if (FileHandle == nullptr || State != ESoundFileState::UNINITIALIZED)
		{
			return SetError(ESoundFileError::INVALID_SOUND_FILE);
		}

		if (!SoundFileData.IsValid())
		{
			return SetError(ESoundFileError::INVALID_DATA);
		}

		// This only does anything if the format is OGG
		if ((SoundFileData->GetDescription().FormatFlags & ESoundFileFormat::MAJOR_FORMAT_MASK) == ESoundFileFormat::OGG)
		{

			int32 Result = SoundFileCommand(FileHandle, SET_ENCODING_QUALITY, &EncodingQuality, sizeof(double));
			if (Result != 1)
			{
				FString StrErr = SoundFileStrError(FileHandle);
				UE_LOG(LogUnrealAudio, Error, TEXT("Failed to set encoding quality: %s"), *StrErr);
				return CurrentError = ESoundFileError::BAD_ENCODING_QUALITY;
			}
		}
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::BeginImport()
	{
		State = ESoundFileState::IMPORTING;
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::EndImport()
	{
		// Set the current NumFrames to the original NumFrames
		((FSoundFileData*)SoundFileData.Get())->SetNumFrames(SoundFileData->GetOriginalDescription().NumFrames);
		State = ESoundFileState::IMPORTED;
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::SeekFrames(SoundFileCount Offset, ESoundFileSeekMode::Type SeekMode, SoundFileCount& OutOffset)
	{
		SoundFileCount Pos = SoundFileSeek(FileHandle, Offset, (int32)SeekMode);
		if (Pos == -1)
		{
			FString StrErr = SoundFileStrError(FileHandle);
			UE_LOG(LogUnrealAudio, Error, TEXT("Failed to seek file: %s"), *StrErr);
			return SetError(ESoundFileError::FAILED_TO_SEEK);
		}
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::ReadFrames(float* DataPtr, SoundFileCount NumFrames, SoundFileCount& NumFramesRead)
	{
		NumFramesRead = SoundFileReadFramesFloat(FileHandle, DataPtr, NumFrames);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::ReadFrames(double* DataPtr, SoundFileCount NumFrames, SoundFileCount& NumFramesRead)
	{
		NumFramesRead = SoundFileReadFramesDouble(FileHandle, DataPtr, NumFrames);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::ReadSamples(float* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesRead)
	{
		OutNumSamplesRead = SoundFileReadSamplesFloat(FileHandle, DataPtr, NumSamples);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::ReadSamples(double* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesRead)
	{
		OutNumSamplesRead = SoundFileReadSamplesDouble(FileHandle, DataPtr, NumSamples);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::WriteFrames(const float* DataPtr, SoundFileCount NumFrames, SoundFileCount& OutNumFramesWritten)
	{
		OutNumFramesWritten = SoundFileWriteFramesFloat(FileHandle, DataPtr, NumFrames);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::WriteFrames(const double* DataPtr, SoundFileCount NumFrames, SoundFileCount& OutNumFramesWritten)
	{
		OutNumFramesWritten = SoundFileWriteFramesDouble(FileHandle, DataPtr, NumFrames);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::WriteSamples(const float* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSampleWritten)
	{
		OutNumSampleWritten = SoundFileWriteSamplesFloat(FileHandle, DataPtr, NumSamples);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::WriteSamples(const double* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSampleWritten)
	{
		OutNumSampleWritten = SoundFileWriteSamplesDouble(FileHandle, DataPtr, NumSamples);
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::GetLengthBytes(SoundFileCount& OutLength) const
	{
		if (!SoundFileData.IsValid())
		{
			return ESoundFileError::INVALID_DATA;
		}

		OutLength = (SoundFileCount)SoundFileData->GetDataSize();
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::SeekBytes(SoundFileCount Offset, ESoundFileSeekMode::Type SeekMode, SoundFileCount& OutOffset)
	{
		if (!SoundFileData.IsValid())
		{
			return ESoundFileError::INVALID_DATA;
		}

		// If there's no data, then seek does nothing
		SoundFileCount MaxBytes = SoundFileData->GetDataSize();
		if (MaxBytes == 0)
		{
			OutOffset = 0;
			CurrentIndexBytes = 0;
			return ESoundFileError::NONE;
		}

		switch (SeekMode)
		{
			case ESoundFileSeekMode::FROM_START:
			CurrentIndexBytes = Offset;
			break;

			case ESoundFileSeekMode::FROM_CURRENT:
			CurrentIndexBytes += Offset;
			break;

			case ESoundFileSeekMode::FROM_END:
			CurrentIndexBytes = MaxBytes + Offset;
			break;

			default:
			checkf(false, TEXT("Uknown seek mode!"));
			break;
		}

		// Wrap the byte index to fall between 0 and MaxBytes
		while (CurrentIndexBytes < 0)
		{
			CurrentIndexBytes += MaxBytes;
		}

		while (CurrentIndexBytes >= MaxBytes)
		{
			CurrentIndexBytes -= MaxBytes;
		}

		OutOffset = CurrentIndexBytes;
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::ReadBytes(void* DataPtr, SoundFileCount NumBytes, SoundFileCount& NumBytesRead)
	{
		if (!SoundFileData.IsValid())
		{
			return ESoundFileError::INVALID_DATA;
		}

		SoundFileCount EndByte = CurrentIndexBytes + NumBytes;
		SoundFileCount MaxBytes = (SoundFileCount)SoundFileData->GetDataSize();
		if (EndByte > MaxBytes)
		{
			NumBytes = MaxBytes - CurrentIndexBytes;
		}

		if (NumBytes > 0)
		{
			const uint8* DataArrayData = SoundFileData->GetData();
			FMemory::Memcpy(DataPtr, (const void*)&DataArrayData[CurrentIndexBytes], NumBytes);
 			CurrentIndexBytes += NumBytes;
		}
		NumBytesRead = NumBytes;
		return ESoundFileError::NONE;
	}

	ESoundFileError::Type FSoundFile::WriteBytes(const void* DataPtr, SoundFileCount NumBytes, SoundFileCount& NumBytesWritten)
	{
		const uint8* InDataBytes = (const uint8*)DataPtr;
		FSoundFileData* Data = (FSoundFileData*)SoundFileData.Get();
		SoundFileCount MaxBytes = Data->GetDataArray().Num();

		// Append the data to the buffer
		if (CurrentIndexBytes == MaxBytes)
		{
			Data->GetDataArray().Append(InDataBytes, NumBytes);
			CurrentIndexBytes += NumBytes;
		}
		else if ((CurrentIndexBytes + NumBytes) < MaxBytes)
		{
			// Write over the existing data in the buffer
			for (SoundFileCount i = 0; i < NumBytes; ++i)
			{
				Data->GetDataArray()[CurrentIndexBytes++] = InDataBytes[i];
			}
		}
		else
		{
			// Overwrite some data until end of current buffer size
			SoundFileCount RemainingBytes = MaxBytes - CurrentIndexBytes;
			SoundFileCount InputBufferIndex = 0;
			for (InputBufferIndex = 0; InputBufferIndex < RemainingBytes; ++InputBufferIndex)
			{
				Data->GetDataArray()[CurrentIndexBytes++] = InDataBytes[InputBufferIndex];
			}

			// Now append the remainder of the input buffer to the internal data buffer
			const uint8* RemainingData = &InDataBytes[InputBufferIndex];
			RemainingBytes = NumBytes - RemainingBytes;
			Data->GetDataArray().Append(RemainingData, RemainingBytes);
			CurrentIndexBytes += RemainingBytes;
			check(CurrentIndexBytes == Data->GetDataArray().Num());
		}
		NumBytesWritten = NumBytes;
		return ESoundFileError::NONE;
	}

	SoundFileCount FSoundFile::GetOffsetBytes() const
	{
		return CurrentIndexBytes;
	}

	FSoundFileData::FSoundFileData(ESoundFileOpenMode::Type InMode)
		: DataBuffer(nullptr)
		, DataBufferSize(0)
		, Mode(InMode)
	{
		Description = { 0 };
		OriginalDescription = { 0 };
	}

	FSoundFileData::~FSoundFileData()
	{
	}

	const FString& FSoundFileData::GetFilePath() const
	{
		return FilePath;
	}

	void FSoundFileData::SetFilePath(const FString& InPath)
	{
		FilePath = InPath;
	}

	const FSoundFileDescription& FSoundFileData::GetDescription() const
	{
		return Description;
	}

	void FSoundFileData::SetDescription(const FSoundFileDescription& InDescription)
	{
		Description = InDescription;
	}

	const FSoundFileDescription& FSoundFileData::GetOriginalDescription() const
	{
		return OriginalDescription;
	}

	void FSoundFileData::SetOriginalDescription(const FSoundFileDescription& InDescription)
	{
		OriginalDescription = InDescription;
	}

	const TArray<ESoundFileChannelMap::Type>& FSoundFileData::GetChannelMap() const
	{
		return ChannelMap;
	}

	void FSoundFileData::SetChannelMap(const TArray<ESoundFileChannelMap::Type>& InChannelMap)
	{
		ChannelMap = InChannelMap;
	}

	void FSoundFileData::SetNumFrames(int64 InNumFrames)
	{
		Description.NumFrames = InNumFrames;
	}

	const uint8* FSoundFileData::GetData() const
	{
		if (Mode == ESoundFileOpenMode::WRITING)
		{
			return DataArray.GetData();
		}
		else
		{
			return DataBuffer;
		}
	}

	int64 FSoundFileData::GetDataSize() const
	{
		if (Mode == ESoundFileOpenMode::WRITING)
		{
			return (int64)DataArray.Num();
		}
		else
		{
			return DataBufferSize;
		}
	}

	TArray<uint8>& FSoundFileData::GetDataArray()
	{
		// If we're accessing the array, make sure this sound file data object is in write-mode
		check(Mode == ESoundFileOpenMode::WRITING);
		return DataArray;
	}

	void FSoundFileData::SetData(uint8* InDataPtr, int64 InDataSize)
	{
		// Make sure this sound file data object wasn't opened for writing
		check(Mode == ESoundFileOpenMode::READING);
		DataBuffer = InDataPtr;
		DataBufferSize = InDataSize;
	}

	// Exported Functions

	TSharedPtr<ISoundFileData> CreateSoundFileData()
	{
		// Only internal sound file code creates write-able sound file data
		return TSharedPtr<ISoundFileData>(new FSoundFileData(ESoundFileOpenMode::READING));
	}

	TSharedPtr<ISoundFile> CreateSoundFile()
	{
		return TSharedPtr<ISoundFile>(new FSoundFile());
	}

	// UnrealAudio Module Functions

	static void* GetSoundFileDllHandle()
	{
		void* DllHandle = nullptr;
#if PLATFORM_WINDOWS
		FString Path = FPaths::EngineDir() / FString(TEXT("Binaries/ThirdParty/libsndfile/Win64/"));
		FPlatformProcess::PushDllDirectory(*Path);
		DllHandle = FPlatformProcess::GetDllHandle(*(Path + "libsndfile-1.dll"));
		FPlatformProcess::PopDllDirectory(*Path);
#elif PLATFORM_MAC
//		FString Path = FPaths::EngineDir() / FString(TEXT("Binaries/ThirdParty/libsndfile/Mac/"));
//		FPlatformProcess::PushDllDirectory(*Path);
		DllHandle = FPlatformProcess::GetDllHandle(TEXT("libsndfile.1.dylib"));
//		FPlatformProcess::PopDllDirectory(*Path);
#endif
		return DllHandle;
	}

	bool FUnrealAudioModule::LoadSoundFileLib()
	{
		SoundFileDllHandle = GetSoundFileDllHandle();
		if (!SoundFileDllHandle)
		{
			UE_LOG(LogUnrealAudio, Error, TEXT("Failed to load Sound File dll"));
			return false;
		}

		bool bSuccess = true;

		void* LambdaDLLHandle = SoundFileDllHandle;

		// Helper function to load DLL exports and report warnings
		auto GetSoundFileDllExport = [&LambdaDLLHandle](const TCHAR* FuncName, bool& bInSuccess) -> void*
		{
			if (bInSuccess)
			{
				void* FuncPtr = FPlatformProcess::GetDllExport(LambdaDLLHandle, FuncName);
				if (FuncPtr == nullptr)
				{
					bInSuccess = false;
					UE_LOG(LogUnrealAudio, Warning, TEXT("Failed to locate the expected DLL import function '%s' in the SoundFile DLL."), *FuncName);
					FPlatformProcess::FreeDllHandle(LambdaDLLHandle);
					LambdaDLLHandle = nullptr;
				}
				return FuncPtr;
			}
			else
			{
				return nullptr;
			}
		};

		SoundFileOpen = (SoundFileOpenFuncPtr)GetSoundFileDllExport(TEXT("sf_open"), bSuccess);
		SoundFileOpenVirtual = (SoundFileOpenVirtualFuncPtr)GetSoundFileDllExport(TEXT("sf_open_virtual"), bSuccess);
		SoundFileClose = (SoundFileCloseFuncPtr)GetSoundFileDllExport(TEXT("sf_close"), bSuccess);
		SoundFileError = (SoundFileErrorFuncPtr)GetSoundFileDllExport(TEXT("sf_error"), bSuccess);
		SoundFileStrError = (SoundFileStrErrorFuncPtr)GetSoundFileDllExport(TEXT("sf_strerror"), bSuccess);
		SoundFileErrorNumber = (SoundFileErrorNumberFuncPtr)GetSoundFileDllExport(TEXT("sf_error_number"), bSuccess);
		SoundFileCommand = (SoundFileCommandFuncPtr)GetSoundFileDllExport(TEXT("sf_command"), bSuccess);
		SoundFileFormatCheck = (SoundFileFormatCheckFuncPtr)GetSoundFileDllExport(TEXT("sf_format_check"), bSuccess);
		SoundFileSeek = (SoundFileSeekFuncPtr)GetSoundFileDllExport(TEXT("sf_seek"), bSuccess);
		SoundFileGetVersion = (SoundFileGetVersionFuncPtr)GetSoundFileDllExport(TEXT("sf_version_string"), bSuccess);
		SoundFileReadFramesFloat = (SoundFileReadFramesFloatFuncPtr)GetSoundFileDllExport(TEXT("sf_readf_float"), bSuccess);
		SoundFileReadFramesDouble = (SoundFileReadFramesDoubleFuncPtr)GetSoundFileDllExport(TEXT("sf_readf_double"), bSuccess);
		SoundFileWriteFramesFloat = (SoundFileWriteFramesFloatFuncPtr)GetSoundFileDllExport(TEXT("sf_writef_float"), bSuccess);
		SoundFileWriteFramesDouble = (SoundFileWriteFramesDoubleFuncPtr)GetSoundFileDllExport(TEXT("sf_writef_double"), bSuccess);
		SoundFileReadSamplesFloat = (SoundFileReadSamplesFloatFuncPtr)GetSoundFileDllExport(TEXT("sf_read_float"), bSuccess);
		SoundFileReadSamplesDouble = (SoundFileReadSamplesDoubleFuncPtr)GetSoundFileDllExport(TEXT("sf_read_double"), bSuccess);
		SoundFileWriteSamplesFloat = (SoundFileWriteSamplesFloatFuncPtr)GetSoundFileDllExport(TEXT("sf_write_float"), bSuccess);
		SoundFileWriteSamplesDouble = (SoundFileWriteSamplesDoubleFuncPtr)GetSoundFileDllExport(TEXT("sf_write_double"), bSuccess);

		// make sure we're successful
		check(bSuccess);
		return bSuccess;
	}

	bool FUnrealAudioModule::ShutdownSoundFileLib()
	{
		if (SoundFileDllHandle)
		{
			FPlatformProcess::FreeDllHandle(SoundFileDllHandle);
			SoundFileDllHandle = nullptr;
			SoundFileOpen = nullptr;
			SoundFileOpenVirtual = nullptr;
			SoundFileClose = nullptr;
			SoundFileError = nullptr;
			SoundFileStrError = nullptr;
			SoundFileErrorNumber = nullptr;
			SoundFileCommand = nullptr;
			SoundFileFormatCheck = nullptr;
			SoundFileSeek = nullptr;
			SoundFileGetVersion = nullptr;
			SoundFileReadFramesFloat = nullptr;
			SoundFileReadFramesDouble = nullptr;
			SoundFileWriteFramesFloat = nullptr;
			SoundFileWriteFramesDouble = nullptr;
			SoundFileReadSamplesFloat = nullptr;
			SoundFileReadSamplesDouble = nullptr;
			SoundFileWriteSamplesFloat = nullptr;
			SoundFileWriteSamplesDouble = nullptr;
		}
		return true;
	}

}

#endif // #if ENABLE_UNREAL_AUDIO
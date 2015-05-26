// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "UnrealAudioModule.h"
#include "UnrealAudioTypes.h"

namespace UAudio
{
	/*
	* ESoundFileError
	**/
	namespace ESoundFileError
	{
		enum Type
		{
			NONE,
			INVALID_SOUND_FILE,
			BAD_ENCODING_QUALITY,
			ALREADY_OPENED,
			ALREADY_HAS_DATA,
			INVALID_DATA,
			FILE_DOESNT_EXIST,
			INVALID_INPUT_FORMAT,
			INVALID_CHANNEL_MAP,
			FAILED_TO_OPEN,
			FAILED_TO_SEEK,
			ALREADY_INTIALIZED,
			LOADING,
			UNKNOWN
		};

		/** @return the stringified version of the enum passed in */
		inline const TCHAR* ToString(ESoundFileError::Type SoundFileError)
		{
			switch (SoundFileError)
			{
				case NONE:					return TEXT("NONE");
				case INVALID_SOUND_FILE:	return TEXT("INVALID_SOUND_FILE");
				case BAD_ENCODING_QUALITY:	return TEXT("BAD_ENCODING_QUALITY");
				case ALREADY_OPENED:		return TEXT("ALREADY_OPENED");
				case ALREADY_HAS_DATA:		return TEXT("ALREADY_HAS_DATA");
				case INVALID_DATA:			return TEXT("INVALID_DATA");
				case FILE_DOESNT_EXIST:		return TEXT("FILE_DOESNT_EXIST");
				case INVALID_INPUT_FORMAT:	return TEXT("INVALID_INPUT_FORMAT");
				case INVALID_CHANNEL_MAP:	return TEXT("INVALID_CHANNEL_MAP");
				case FAILED_TO_OPEN:		return TEXT("FAILED_TO_OPEN");
				case FAILED_TO_SEEK:		return TEXT("FAILED_TO_SEEK");
				case ALREADY_INTIALIZED:	return TEXT("ALREADY_INTIALIZED");
				case LOADING:				return TEXT("LOADING");

				default:
				case UNKNOWN:				return TEXT("UNKNOWN");
			}
		}
	}

	/*
	* ESoundFileFormatFlags
	* Specifies the major format type of the sound source.
	* File formats are fully specified by a major/minor format.
	*
	* For example, a Ogg-Vorbis encoding would use:
	* uint32 FormatFlags = ESoundFormatFlags::OGG | ESoundFormatFlags::VORBIS;
	**/
	namespace ESoundFileFormat
	{
		enum Flags
		{
			// Major Formats
			WAV				= 0x010000,		// Microsoft WAV format
			AIFF			= 0x020000,		// Apple AIFF format
			FLAC			= 0x170000,		// FLAC lossless
			OGG				= 0x200000,		// Xiph OGG

			// Uncompressed Minor Formats
			PCM_SIGNED_8	= 0x0001,		// Signed 8 bit PCM
			PCM_SIGNED_16	= 0x0002,		// Signed 16 bit PCM
			PCM_SIGNED_24	= 0x0003,		// Signed 24 bit PCM
			PCM_SIGNED_32	= 0x0004,		// Signed 32 bit PCM
			PCM_UNSIGNED_8	= 0x0005,		// Unsigned 8 bit PCM
			PCM_FLOAT		= 0x0006,		// 32 bit float
			PCM_DOUBLE		= 0x0007,		// 64 bit float

			// Compressed Minor Formats
			MU_LAW			= 0x0010,		// Mu-law encoding
			A_LAW			= 0x0011,		// A-law encoding
			IMA_ADPCM		= 0x0012,		// IMA ADPCM encoding
			MS_ADPCM		= 0x0013,		// Microsoft ADPCM encoding
			GSM_610			= 0x0020,		// GSM 6.10 encoding
			G721_32			= 0x0030,		// 32 kbps G721 ADPCM encoding
			G723_24			= 0x0031,		// 23 kbps G723 ADPCM encoding
			G723_40			= 0x0032,		// 40 kbps G723 ADPCM encoding
			DWVW_12			= 0x0040,		// 12 bit delta-width variable word encoding
			DMVW_16			= 0x0041,		// 16 bit delta-width variable word encoding
			DMVW_24			= 0x0042,		// 24 bit delta-width variable word encoding
			DMVW_N			= 0x0043,		// N bit delta-width variable word encoding
			VORBIS			= 0x0060,		// Xiph vorbis encoding

			// Endian opts
			ENDIAN_FILE		= 0x00000000,	// default file endian
			ENDIAN_LITTLE	= 0x10000000,	// little-endian
			ENDIAN_BIG		= 0x20000000,	// big-endian
			ENDIAN_CPU		= 0x30000000,	// cpu-endian

			// Masks
			MINOR_FORMAT_MASK = 0x0000FFFF,
			MAJOR_FORMAT_MASK = 0x0FFF0000,
			ENDIAN_MASK		  = 0x30000000,
		};

		inline const TCHAR* ToStringMajor(int32 FormatFlags)
		{
			switch (FormatFlags & ESoundFileFormat::MAJOR_FORMAT_MASK)
			{
				case ESoundFileFormat::WAV:		return TEXT("WAV");
				case ESoundFileFormat::AIFF:	return TEXT("AIFF");
				case ESoundFileFormat::FLAC:	return TEXT("FLAC");
				case ESoundFileFormat::OGG:		return TEXT("OGG");
				default:						return TEXT("INVALID");
			}
		}

		inline const TCHAR* ToStringMinor(int32 FormatFlags)
		{
			switch (FormatFlags & ESoundFileFormat::MINOR_FORMAT_MASK)
			{
				case ESoundFileFormat::PCM_SIGNED_8:	return TEXT("PCM_SIGNED_8");
				case ESoundFileFormat::PCM_SIGNED_16:	return TEXT("PCM_SIGNED_16");
				case ESoundFileFormat::PCM_SIGNED_24:	return TEXT("PCM_SIGNED_24");
				case ESoundFileFormat::PCM_SIGNED_32:	return TEXT("PCM_SIGNED_32");
				case ESoundFileFormat::PCM_UNSIGNED_8:	return TEXT("PCM_UNSIGNED_8");
				case ESoundFileFormat::PCM_FLOAT:		return TEXT("PCM_FLOAT");
				case ESoundFileFormat::PCM_DOUBLE:		return TEXT("PCM_DOUBLE");
				case ESoundFileFormat::MU_LAW:			return TEXT("MU_LAW");
				case ESoundFileFormat::A_LAW:			return TEXT("A_LAW");
				case ESoundFileFormat::IMA_ADPCM:		return TEXT("IMA_ADPCM");
				case ESoundFileFormat::MS_ADPCM:		return TEXT("MS_ADPCM");
				case ESoundFileFormat::GSM_610:			return TEXT("GSM_610");
				case ESoundFileFormat::G721_32:			return TEXT("G721_32");
				case ESoundFileFormat::G723_24:			return TEXT("G723_24");
				case ESoundFileFormat::G723_40:			return TEXT("G723_40");
				case ESoundFileFormat::DWVW_12:			return TEXT("DWVW_12");
				case ESoundFileFormat::DMVW_16:			return TEXT("DMVW_16");
				case ESoundFileFormat::DMVW_24:			return TEXT("DMVW_24");
				case ESoundFileFormat::DMVW_N:			return TEXT("DMVW_N");
				case ESoundFileFormat::VORBIS:			return TEXT("VORBIS");
				default:								return TEXT("INVALID");
			}
		}
	};

	/*
	* ESoundFileChannelMap
	* These are separated from the device channel mappings purposefully since
	* the enumeration may not exactly be the same as the output speaker mapping.
	**/
	namespace ESoundFileChannelMap
	{
		enum Type
		{
			INVALID = 0,
			MONO,
			LEFT,
			RIGHT,
			CENTER,
			FRONT_LEFT,
			FRONT_RIGHT,
			FRONT_CENTER,
			BACK_CENTER,
			BACK_LEFT,
			BACK_RIGHT,
			LFE,
			LEFT_CENTER,
			RIGHT_CENTER,
			SIDE_LEFT,
			SIDE_RIGHT,
			TOP_CENTER,
			TOP_FRONT_LEFT,
			TOP_FRONT_RIGHT,
			TOP_FRONT_CENTER,
			TOP_BACK_LEFT,
			TOP_BACK_RIGHT,
			TOP_BACK_CENTER,
		};

		/** @return the stringified version of the enum passed in */
		inline const TCHAR* ToString(ESoundFileChannelMap::Type ChannelMap)
		{
			switch (ChannelMap)
			{
				case INVALID:			return TEXT("INVALID");
				case MONO:				return TEXT("MONO");
				case LEFT:				return TEXT("LEFT");
				case RIGHT:				return TEXT("RIGHT");
				case CENTER:			return TEXT("CENTER");
				case FRONT_LEFT:		return TEXT("FRONT_LEFT");
				case FRONT_RIGHT:		return TEXT("FRONT_RIGHT");
				case FRONT_CENTER:		return TEXT("FRONT_CENTER");
				case BACK_CENTER:		return TEXT("BACK_CENTER");
				case BACK_LEFT:			return TEXT("BACK_LEFT");
				case BACK_RIGHT:		return TEXT("BACK_RIGHT");
				case LFE:				return TEXT("LFE");
				case LEFT_CENTER:		return TEXT("LEFT_CENTER");
				case RIGHT_CENTER:		return TEXT("RIGHT_CENTER");
				case SIDE_LEFT:			return TEXT("SIDE_LEFT");
				case SIDE_RIGHT:		return TEXT("SIDE_RIGHT");
				case TOP_CENTER:		return TEXT("TOP_CENTER");
				case TOP_FRONT_LEFT:	return TEXT("TOP_FRONT_LEFT");
				case TOP_FRONT_RIGHT:	return TEXT("TOP_FRONT_RIGHT");
				case TOP_FRONT_CENTER:	return TEXT("TOP_FRONT_CENTER");
				case TOP_BACK_LEFT:		return TEXT("TOP_BACK_LEFT");
				case TOP_BACK_RIGHT:	return TEXT("TOP_BACK_RIGHT");
				case TOP_BACK_CENTER:	return TEXT("TOP_BACK_CENTER");
				default:				return TEXT("UNKNOWN");
			}
		}
	}

	/*
	* FSoundFileDescription
	* Struct specifying a sound file description.
	**/
	struct FSoundFileDescription
	{
		// The number of frames (interleaved samples) in the sound file.
		int64 NumFrames;

		// The sample rate of the sound file.
		int32 SampleRate;

		// The number of channels of the sound file.
		int32 NumChannels;

		// The format flags of the sound file.
		int32 FormatFlags;

		// The number of sections of the sound file.
		int32 NumSections;

		// Whether or not the sound file is seekable.
		int32 bIsSeekable;
	};

	/**
	* FSoundFileImportSettings
	* Struct defining sound file import settings.
	*/
	struct UNREALAUDIO_API FSoundFileImportSettings
	{
		// Full path to the sound file on disk.
		FString SoundFilePath;

		// Desired import format.
		int32 Format;

		// Desired import sample rate.
		uint32 SampleRate;

		// For compression-type target formats that used an encoding quality (0.0 = low, 1.0 = high).
		double EncodingQuality;

		// Whether or not to peak-normalize the audio file during import.
		bool bPerformPeakNormalization;
	};

	/*
	* ESoundFileState
	**/
	namespace ESoundFileState
	{
		enum Type
		{
			// Sound file hasn't begun loading and was just created.
			UNINITIALIZED,

			// Sound file has been initialized with data and is ready for playback
			INITIALIZED,

			// Sound file is importing.
			IMPORTING,

			// Sound file has finished importing.
			IMPORTED,

			// Sound file has an error. Called GetError() on ISoundFile to get error.
			HAS_ERROR,
		};
	}

	/**
	* FSoundFileData
	* Data structure containing data for a sound file. Can be stored within a UAsset or loaded with a sound file.
	*/
	class UNREALAUDIO_API ISoundFileData
	{
	public:
		virtual ~ISoundFileData() {}

		/** Sets and gets the sound file path */
		virtual const FString& GetFilePath() const = 0;
		virtual void SetFilePath(const FString& InPath) = 0;
		
		/** Sets and gets the file description */
		virtual const FSoundFileDescription& GetDescription() const = 0;
		virtual void SetDescription(const FSoundFileDescription& InDescription) = 0;

		/** Sets and gets the original file description */
		virtual const FSoundFileDescription& GetOriginalDescription() const = 0;
		virtual void SetOriginalDescription(const FSoundFileDescription& InDescription) = 0;

		/** Sets and gets the channel map */
		virtual const TArray<ESoundFileChannelMap::Type>& GetChannelMap() const = 0;
		virtual void SetChannelMap(const TArray<ESoundFileChannelMap::Type>& InChannelMap) = 0;

		/** Gets a const pointer to the internal buffer of data. */
		virtual const uint8* GetData() const = 0;

		/** Gets the size of the data */
		virtual int64 GetDataSize() const = 0;

		/** Sets the data buffer pointer and the size of the data buffer */
		virtual void SetData(uint8* InDataPtr, int64 InDataSize) = 0;
	};

	/**
	* ISoundFile
	* Interface for sound file reading/writing.
	*/
	class UNREALAUDIO_API ISoundFile
	{
	public:
		/** Virtual Destructor */
		virtual ~ISoundFile() {}

		/** Initializes the sound file with the given sound file data. */
		virtual ESoundFileError::Type Initialize(TSharedPtr<ISoundFileData> InSoundFileData) = 0;

		/** Returns the name of the sound file */
		virtual ESoundFileError::Type GetName(FString& Name) = 0;

		/** Gets the sound file data associated with this ISoundFile. */
		virtual ESoundFileError::Type GetSoundFileData(TSharedPtr<ISoundFileData>& OutSoundFileData) = 0;

		/** @return Returns the state of the sound file. */
		virtual  ESoundFileState::Type GetState() const = 0;

		/** @return Returns the last sound file error. */
		virtual ESoundFileError::Type GetError() const = 0;
	};

	/**
	* Creates an empty sound file data object
	* Creates an empty sound file (with no sound file data)
	*/
	UNREALAUDIO_API TSharedPtr<ISoundFileData> CreateSoundFileData();

	/**
	* CreateSoundFile
	* Creates an empty sound file (with no sound file data)
	*/
	UNREALAUDIO_API TSharedPtr<ISoundFile> CreateSoundFile();

	/**
	* ImportSound
	* Imports a sound given the import settings.
	* The function immediately returns a ISoundFile reference, but that sound file will
	* be in a loading state. If the sound file succeeds in importing, the sound file state
	* will become Loaded, otherwise it will have an error that can be retrieved.
	*/
	UNREALAUDIO_API TSharedPtr<ISoundFile> ImportSound(const FSoundFileImportSettings& ImportSettings);

	/**
	* ExportSound
	* Exports a sound to the given path.
	* The function immediately returns a ISoundFile reference, but that sound file will
	* be in a loading state. If the sound file succeeds in importing, the sound file state
	* will become Loaded, otherwise it will have an error that can be retrieved.
	*/
	UNREALAUDIO_API void ExportSound(TSharedPtr<ISoundFileData> SoundFileData, const FString& ExportPath);

}

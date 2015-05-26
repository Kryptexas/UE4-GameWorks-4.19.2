// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "UnrealAudioModule.h"
#include "UnrealAudioTypes.h"
#include "UnrealAudioSoundFile.h"

#if ENABLE_UNREAL_AUDIO

namespace UAudio
{
	typedef int64 SoundFileCount;
	typedef struct SoundFileHandleOpaque SoundFileHandle;

	/*
	* ESoundFileSeekMode
	* Specifies the how to seek a sound file
	**/
	namespace ESoundFileSeekMode
	{
		enum Type
		{
			// Seek the file from the start of the file.
			FROM_START = 0,

			// Seek the file from the current read/write position.
			FROM_CURRENT = 1,

			// Seek the file from the end position.
			FROM_END = 2,
		};
	}

	/*
	* ESoundFileOpenMode
	* Specifies the how to open a sound file1
	**/
	namespace ESoundFileOpenMode
	{
		enum Type
		{
			// The sound file is open for reading.
			READING = 0x10,

			// The sound file is open for writing.
			WRITING = 0x20,

			UNKNOWN = 0,
		};
	}


	/**
	ISoundFileVirtual
	Interface for specifying the virtual sound file i/o operations.
	*/
	class ISoundFileVirtual : public ISoundFile
	{
	public:
		virtual ~ISoundFileVirtual() {}

		/**
		* Gets the length of the file in bytes.
		* @param OutLength The returned length of the file.
		*/
		virtual ESoundFileError::Type GetLengthBytes(SoundFileCount& OutLength) const = 0;

		/**
		* Seeks the read/write position offset in bytes.
		* @param Offset The offset in bytes of the file
		* @param SeekMode The type of seek to use when performing the seek.
		* @param OutOffset Optionally retrieve the actual offset in bytes after the seek operation.
		*/
		virtual ESoundFileError::Type SeekBytes(SoundFileCount Offset, ESoundFileSeekMode::Type SeekMode, SoundFileCount& OutOffset) = 0;

		/**
		* Reads the number of bytes from the given buffer ptr.
		* @param DataPtr Pointer to a buffer for copying into.
		* @param NumBytes The number of bytes to read.
		* @param NumBytesRead Returns the number of bytes actually read.
		*/
		virtual ESoundFileError::Type ReadBytes(void* DataPtr, SoundFileCount NumBytes, SoundFileCount& NumBytesRead) = 0;

		/**
		* Writes the number of bytes into the given buffer ptr.
		* @param DataPtr Pointer to an input buffer
		* @param NumBytes The number of bytes to write.
		* @param NumBytesWritten The number of bytes actually written.
		*/
		virtual ESoundFileError::Type WriteBytes(const void* DataPtr, SoundFileCount NumBytes, SoundFileCount& NumBytesWritten) = 0;

		/** @return Gets the current file offset in bytes. */
		virtual SoundFileCount GetOffsetBytes() const = 0;
	};

	/**
	FSoundFile
	Internal SoundFile API
	*/
	class ISoundFileInternal : public ISoundFileVirtual
	{
	public:
		virtual ~ISoundFileInternal() {}

		/**
		* Sets the error on the sound file. Internal audio systems can cause audio files to get errors. For example, if a failure occurs
		* during import due to an error on the input sound file, the output (imported) sound file needs to also get the error.
		*/
		virtual ESoundFileError::Type SetError(ESoundFileError::Type Error) = 0;

		/**
		* Opens a sound file from the given path.
		* @param FilePath Path to the sound file.
		* @param Mode The mode to open the sound file in.
		*/
		virtual ESoundFileError::Type OpenFileForReading(const FString& FilePath) = 0;

		/**
		* Opens an empty virtual sound file of the given description for writing to.
		* @param FileDescription Description of the type of file this is to be.
		* @param InChannelMap The input channel map to use for the file.
		* @param Mode The mode to open the sound file in.
		*/
		virtual ESoundFileError::Type OpenEmptyFileForImport(const FSoundFileDescription& FileDescription, const TArray<ESoundFileChannelMap::Type>& InChannelMap) = 0;

		/**
		* Opens an empty sound file on disk of the given description for writing to.
		* @param FileDescription Description of the type of file this is to be.
		* @param InChannelMap The input channel map to use for the file.
		* @param Mode The mode to open the sound file in.
		*/
		virtual ESoundFileError::Type OpenEmptyFileForExport(const FString& FilePath, const FSoundFileDescription& FileDescription, const TArray<ESoundFileChannelMap::Type>& InChannelMap) = 0;

		/** Releases the internal sound file handle if its open */
		virtual ESoundFileError::Type ReleaseSoundFileHandle() = 0;

		/**
		* Sets the description of the original file used to import this file.
		* @param FileDescription Description of the original file used to import this file.
		*/
		virtual ESoundFileError::Type SetImportFileInfo(const FSoundFileDescription& FileDescription, const FString& FullPath) = 0;

		/**
		* Sets the encoding quality of a sound file.
		* @param EncodingQuality The encoding quality of a sound file (between 0.0 (low) and 1.0 (high))
		* @note This can only be set during sound file import before importing begins, but after the file has been opened.
		*		It can also only be set on OGG-Vorbis files.
		*/
		virtual ESoundFileError::Type SetEncodingQuality(double EncodingQuality) = 0;

		/**
		* Sets the state of the sound file to begin loading
		*/
		virtual ESoundFileError::Type BeginImport() = 0;

		/**
		* Sets the state of the sound file to end loading
		*/
		virtual ESoundFileError::Type EndImport() = 0;

		/**
		* Seeks the read/write position offset in frames.
		* @param Offset The offset in frames of the file
		* @param SeekMode The type of seek to use when performing the seek.
		* @param OutOffset Optionally retrieve the actual offset in bytes after the seek operation.
		*/
		virtual ESoundFileError::Type SeekFrames(SoundFileCount Offset, ESoundFileSeekMode::Type SeekMode, SoundFileCount& OutOffset) = 0;

		/**
		* Reads out the number of frames of data as interleaved floats/doubles.
		* @param DataPtr Pointer to a buffer to copy data into.
		* @param NumFrames The number of frames to read.
		* @param NumFramesRead Returns the number of frames actually read.
		*/
		virtual ESoundFileError::Type ReadFrames(float* DataPtr, SoundFileCount NumFrames, SoundFileCount& NumFramesRead) = 0;
		virtual ESoundFileError::Type ReadFrames(double* DataPtr, SoundFileCount NumFrames, SoundFileCount& NumFramesRead) = 0;

		/**
		* Reads out the number of samples of data as floats/doubles.
		* @param DataPtr Pointer to a buffer for copy data into.
		* @param NumSamples The number of samples to read.
		* @param OutNumSamplesRead Returns the number of samples actually read.
		*/
		virtual ESoundFileError::Type ReadSamples(float* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesRead) = 0;
		virtual ESoundFileError::Type ReadSamples(double* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesRead) = 0;

		/**
		* Writes the number of frames of data as interleaved floats/doubles.
		* @param DataPtr Pointer to a buffer for copying data from.
		* @param NumFrames The number of frames to write.
		* @param OutNumFramesWritten Returns the number of frames actually written.
		*/
		virtual ESoundFileError::Type WriteFrames(const float* Data, SoundFileCount NumFrames, SoundFileCount& OutNumFramesWritten) = 0;
		virtual ESoundFileError::Type WriteFrames(const double* Data, SoundFileCount NumFrames, SoundFileCount& OutNumFramesWritten) = 0;

		/**
		* Writes the number of samples of data as floats/doubles.
		* @param DataPtr Pointer to a buffer for copying data from.
		* @param NumSamples The number of samples to write.
		* @param OutNumSamplesWritten Returns the number of samples actually written.
		*/
		virtual ESoundFileError::Type WriteSamples(const float* Data, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesWritten) = 0;
		virtual ESoundFileError::Type WriteSamples(const double* Data, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesWritten) = 0;
	};


	/**
	FSoundFile
	Concrete implementation of ISoundFile which uses libsndfile for file reading/writing.
	*/
	class FSoundFile : public ISoundFileInternal
	{
	public:
		FSoundFile();
		~FSoundFile();

		// ISoundFile implementation
		ESoundFileError::Type Initialize(TSharedPtr<ISoundFileData> InSoundFileData) override;
		ESoundFileError::Type GetName(FString& Name) override;
		ESoundFileError::Type GetSoundFileData(TSharedPtr<ISoundFileData>& OutSoundFileData) override;
		ESoundFileState::Type GetState() const override;
		ESoundFileError::Type GetError() const override;

		// ISoundFileInternal implementation
		ESoundFileError::Type SetError(ESoundFileError::Type Error) override;
		ESoundFileError::Type OpenFileForReading(const FString& FilePath) override;
		ESoundFileError::Type OpenEmptyFileForImport(const FSoundFileDescription& FileDescription, const TArray<ESoundFileChannelMap::Type>& InChannelMap) override;
		ESoundFileError::Type OpenEmptyFileForExport(const FString& FilePath, const FSoundFileDescription& FileDescription, const TArray<ESoundFileChannelMap::Type>& InChannelMap) override;
		ESoundFileError::Type ReleaseSoundFileHandle() override;

		ESoundFileError::Type SetImportFileInfo(const FSoundFileDescription& FileDescription, const FString& FullPath) override;
		ESoundFileError::Type SetEncodingQuality(double EncodingQuality) override;
		ESoundFileError::Type BeginImport() override;
		ESoundFileError::Type EndImport() override;
		ESoundFileError::Type SeekFrames(SoundFileCount Offset, ESoundFileSeekMode::Type SeekMode, SoundFileCount& OutOffset) override;
		ESoundFileError::Type ReadFrames(float* DataPtr, SoundFileCount NumFrames, SoundFileCount& OutNumFramesRead) override;
		ESoundFileError::Type ReadFrames(double* DataPtr, SoundFileCount NumFrames, SoundFileCount& OutNumFramesRead) override;
		ESoundFileError::Type ReadSamples(float* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesRead) override;
		ESoundFileError::Type ReadSamples(double* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSamplesRead) override;
		ESoundFileError::Type WriteFrames(const float* Data, SoundFileCount NumFrames, SoundFileCount& OutNumFramesWritten) override;
		ESoundFileError::Type WriteFrames(const double* Data, SoundFileCount NumFrames, SoundFileCount& OutNumFramesWritten) override;
		ESoundFileError::Type WriteSamples(const float* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSampleWritten) override;
		ESoundFileError::Type WriteSamples(const double* DataPtr, SoundFileCount NumSamples, SoundFileCount& OutNumSampleWritten) override;
		
		// ISoundFileVirtual implementation
		ESoundFileError::Type GetLengthBytes(SoundFileCount& OutLength) const override;
		ESoundFileError::Type SeekBytes(SoundFileCount Offset, ESoundFileSeekMode::Type SeekMode, SoundFileCount& OutOffset) override;
		ESoundFileError::Type ReadBytes(void* DataPtr, SoundFileCount NumBytes, SoundFileCount& OutNumBytesRead) override;
		ESoundFileError::Type WriteBytes(const void* DataPtr, SoundFileCount NumBytes, SoundFileCount& OutNumBytesWritten) override;
		SoundFileCount GetOffsetBytes() const override;

	private:

		TSharedPtr<ISoundFileData> SoundFileData;

		/** The current read/write index in the raw data */
		SoundFileCount CurrentIndexBytes;

		/** Handle to the sound file decoder. */
		SoundFileHandle* FileHandle;

		/** The current state of the sound file. */
		volatile ESoundFileState::Type State;

		/** The last error of the sound file. */
		volatile ESoundFileError::Type CurrentError;

		int32 Id;
	};

	/**
	FSoundFileData
	*/
	class FSoundFileData : public ISoundFileData
	{
	public:
		FSoundFileData(ESoundFileOpenMode::Type Mode);
		~FSoundFileData();

		// ISoundFileData interface
		const FString& GetFilePath() const override;
		void SetFilePath(const FString& InPath) override;
		const FSoundFileDescription& GetDescription() const override;
		void SetDescription(const FSoundFileDescription& InDescription) override;
		const FSoundFileDescription& GetOriginalDescription() const override;
		void SetOriginalDescription(const FSoundFileDescription& InDescription) override;
		const TArray<ESoundFileChannelMap::Type>& GetChannelMap() const override;
		void SetChannelMap(const TArray<ESoundFileChannelMap::Type>& InChannelMap) override;
		const uint8* GetData() const override;
		int64 GetDataSize() const override;
		void SetData(uint8* InDataPtr, int64 InDataSize) override;

		TArray<uint8>& GetDataArray();
		void SetNumFrames(int64 InNumFrames);

	private:
		// The path of the file when it was imported
		FString FilePath;

		// The current description of the file (i.e. compressed)
		FSoundFileDescription Description;

		// The description of the file before it was imported
		FSoundFileDescription OriginalDescription;

		// The channel map of the file
		TArray<ESoundFileChannelMap::Type> ChannelMap;

		// An array of data, used to build the file data during import
		TArray<uint8> DataArray;

		// A ptr to a buffer of data, used to point this sound file data to an arbitrary location in memory
		// This sound file data object does not own this data buffer
		uint8* DataBuffer;

		// The size of the data buffer in bytes
		int64 DataBufferSize;

		// The mode this sound file data is in (read or write)
		ESoundFileOpenMode::Type Mode;
	};
}



#endif // #if ENABLE_UNREAL_AUDIO


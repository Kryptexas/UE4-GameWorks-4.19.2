// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "ModuleInterface.h"
#include "UnrealAudioModule.h"
#include "UnrealAudioDeviceFormat.h"

#if ENABLE_UNREAL_AUDIO

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealAudioDevice, Log, All);

#define UA_DEVICE_PLATFORM_ERROR(INFO)	(OnDeviceError(EDeviceError::PLATFORM, INFO, FString(__FILE__), __LINE__))
#define AU_DEVICE_PARAM_ERROR(INFO)		(OnDeviceError(EDeviceError::INVALID_PARAMETER, INFO, FString(__FILE__), __LINE__))
#define AU_DEVICE_WARNING(INFO)			(OnDeviceError(EDeviceError::WARNING, INFO, FString(__FILE__), __LINE__))


namespace UAudio
{
	/**
	* EDeviceError
	* The types of errors that can occur in IUnrealAudioDevice implementations.
	*/
	namespace EDeviceError
	{
		enum Type
		{
			WARNING,
			UNKNOWN,
			INVALID_PARAMETER,
			PLATFORM,
			SYSTEM,
			THREAD,
		};

		/** @return the stringified version of the enum passed in */
		static inline const TCHAR* ToString(EDeviceError::Type DeviceError)
		{
			switch (DeviceError)
			{
				case WARNING:			return TEXT("WARNING");
				case UNKNOWN:			return TEXT("UNKNOWN");
				case INVALID_PARAMETER:	return TEXT("INVALID_PARAMETER");
				case PLATFORM:			return TEXT("PLATFORM");
				case SYSTEM:			return TEXT("SYSTEM");
				case THREAD:			return TEXT("THREAD");
				default:				check(false);
			}
			return TEXT("");
		}
	}

	/**
	* EDeviceApi
	* Implemented platform-Specific audio device APIs.
	*/
	namespace EDeviceApi
	{
		enum Type
		{
			WASAPI,				// Windows
			XAUDIO2,			// Windows/XBox
			NGS2,				// PS4
			ALSA,				// Linux
			CORE_AUDIO,			// Mac/IOS
			OPENAL,				// Linux
			HTML5,				// Web
			DUMMY,				// DUMMY (no device api)
		};

		/** @return the stringified version of the enum passed in */
		static inline const TCHAR* ToString(EDeviceApi::Type DeviceApi)
		{
			switch (DeviceApi)
			{
				case WASAPI:		return TEXT("WASAPI");
				case XAUDIO2:		return TEXT("XAUDIO2");
				case NGS2:			return TEXT("NGS2");
				case ALSA:			return TEXT("ALSA");
				case CORE_AUDIO:	return TEXT("CORE_AUDIO");
				case OPENAL:		return TEXT("OPENAL");
				case HTML5:			return TEXT("HTML5");
				case DUMMY:			return TEXT("DUMMY");
				default:			check(false);
			}
			return TEXT("");
		}
	};

	/**
	* EStreamStatus
	* An enumeration to specify the state of the audio stream.
	*/
	namespace EStreamState
	{
		enum Type
		{
			CLOSED,				/** The stream is closed */
			STOPPED,			/** The stream was running and is now stopped */
			RUNNING,			/** The stream is running (currently streaming callbacks) */
			STOPPING,			/** The stream is currently stopping*/
		};
	}

	/**
	* EDeviceDataFlow
	* An enumeration used to specify the flow of audio on the device
	*/
	namespace EStreamType
	{
		enum Type
		{
			INPUT,			/** The audio device streams audio into the application (e.g. microphone) */
			OUTPUT,			/** The audio device only streams audio out of the application */
			COUNT,			/** The count of stream types. */
			UNKNOWN,		/** Unknown stream type. */
		};
	}

	/**
	* EStreamStatus
	* An enumeration to specify the flow status of the audio stream.
	*/
	namespace EStreamFlowStatus
	{
		enum Flag
		{
			INPUT_OVERFLOW		= (1 << 0),		/** The input stream is has too much data */
			OUTPUT_UNDERFLOW	= (1 << 1)		/** The output stream has too little data */
		};
	}
	typedef uint8 StreamStatus;

	/**
	* ESpeaker
	* An enumeration to specify speaker types
	*/
	struct ESpeaker
	{
		enum Type
		{
			FRONT_LEFT,
			FRONT_RIGHT,
			FRONT_CENTER,
			LOW_FREQUENCY,
			BACK_LEFT,
			BACK_RIGHT,
			FRONT_LEFT_OF_CENTER,
			FRONT_RIGHT_OF_CENTER,
			BACK_CENTER,
			SIDE_LEFT,
			SIDE_RIGHT,
			TOP_CENTER,
			TOP_FRONT_LEFT,
			TOP_FRONT_CENTER,
			TOP_FRONT_RIGHT,
			TOP_BACK_LEFT,
			TOP_BACK_CENTER,
			TOP_BACK_RIGHT,
			SPEAKER_TYPE_COUNT
		};

		/** @return the stringified version of the enum passed in */
		static inline const TCHAR* ToString(ESpeaker::Type Speaker)
		{
			switch (Speaker)
			{
				case FRONT_LEFT:				return TEXT("FRONT_LEFT");
				case FRONT_RIGHT:				return TEXT("FRONT_RIGHT");
				case FRONT_CENTER:				return TEXT("FRONT_CENTER");
				case LOW_FREQUENCY:				return TEXT("LOW_FREQUENCY");
				case BACK_LEFT:					return TEXT("BACK_LEFT");
				case BACK_RIGHT:				return TEXT("BACK_RIGHT");
				case FRONT_LEFT_OF_CENTER:		return TEXT("FRONT_LEFT_OF_CENTER");
				case FRONT_RIGHT_OF_CENTER:		return TEXT("FRONT_RIGHT_OF_CENTER");
				case BACK_CENTER:				return TEXT("BACK_CENTER");
				case SIDE_LEFT:					return TEXT("SIDE_LEFT");
				case SIDE_RIGHT:				return TEXT("SIDE_RIGHT");
				case TOP_CENTER:				return TEXT("TOP_CENTER");
				case TOP_FRONT_LEFT:			return TEXT("TOP_FRONT_LEFT");
				case TOP_FRONT_CENTER:			return TEXT("TOP_FRONT_CENTER");
				case TOP_FRONT_RIGHT:			return TEXT("TOP_FRONT_RIGHT");
				case TOP_BACK_LEFT:				return TEXT("TOP_BACK_LEFT");
				case TOP_BACK_CENTER:			return TEXT("TOP_BACK_CENTER");
				case TOP_BACK_RIGHT:			return TEXT("TOP_BACK_RIGHT");
				default:						return TEXT("UKNOWN");
			}
			return TEXT("");
		}
	};


	/**
	* FCallbackInfo
	* A struct for callback info. Using this rather than params in callback function
	* because updating new members here will be less painful.
	*/
	struct FCallbackInfo
	{
		/** A pointer to a buffer of float data which will be written to output device.*/
		float* OutBuffer;

		/** The number of buffer output frames */
		uint32 NumFrames;

		/** A pointer to a buffer of float data which is passed in from device input. */
		float* InBuffer;

		/** Array of outputs speakers */
		TArray<ESpeaker::Type> OutputSpeakers;

		/** The number of channels in output. */
		int32 NumChannels;

		/** The number of input channels in input stream. */
		int32 NumInputChannels;

		/** The current status flags of the input and output buffers */
		StreamStatus StatusFlags;

		/** The current frame-accurate lifetime of the audio stream. */
		double StreamTime;

		/** The output device framerate */
		int32 FrameRate;

		/** A pointer to user data */
		void* UserData;
	};

	/**
	* StreamCallback
	*
	* Callback which calls from platform audio device code into platform-independent mixing code.
	* @param CallbackInfo A struct of callback info needed to process audio.
	*/
	typedef bool(*StreamCallback)(FCallbackInfo& CallbackInfo);

	/**
	* FDeviceInfo
	* Struct used to hold information about audio devices, queried by user.
	*/
	struct FDeviceInfo
	{
		/** Friendly name of device. */
		FString FriendlyName;

		/** Number of channels (e.g. 2 for stereo) natively supported by the device. */
		uint32 NumChannels;

		/** The frame rate of the device. */
		uint32 FrameRate;

		/** The data format of the device (e.g. float). */
		EStreamFormat::Type StreamFormat;

		/** What type of device it is (input, output, or both). */
		EStreamType::Type StreamType;

		/** What speakers this device supports (if output device) */
		TArray<ESpeaker::Type> Speakers;

		/** Whether or not it is the OS default device for the type. */
		bool bIsSystemDefault;

		FDeviceInfo()
			: FriendlyName(TEXT("Uknown"))
			, NumChannels(0)
			, FrameRate(0)
			, StreamFormat(EStreamFormat::UNKNOWN)
			, StreamType(EStreamType::UNKNOWN)
			, bIsSystemDefault(false)
		{}
	};

	/**
	* FCreateStreamParams
	* Struct used to define stream creation.
	*/
	struct FCreateStreamParams
	{
		/** The index of the device to use for audio input. Initialized to INDEX_NONE, which means no device input. */
		uint32 InputDeviceIndex;

		/** The index of the device to use for audio output. Must be defined. */
		uint32 OutputDeviceIndex;

		/** The size of the callback block (in frames) that the user would like. (e.g. 512) */
		uint32 CallbackBlockSize;

		/** The function pointer of a user callback function to generate audio samples to output device. */
		StreamCallback CallbackFunction;

		/** Pointer to store user data. Passed in callback function. */
		void* UserData;

		/** Constructor */
		FCreateStreamParams()
			: InputDeviceIndex(INDEX_NONE)
			, OutputDeviceIndex(INDEX_NONE)
			, CallbackBlockSize(512)
			, UserData(nullptr)
		{}
	};

	/**
	* IDeviceErrorListener
	* An optional class that defines an error listener interface. Used for handling or logging any audio device errors which may have occurred.
	*/
	class UNREALAUDIO_API IDeviceErrorListener
	{
	public:
		virtual void OnDeviceError(const EDeviceError::Type Error, const FString& ErrorDetails, const FString& FileName, int32 LineNumber) const = 0;
	};

	/**
	* IUnrealAudioDeviceModule
	* Main audio device module, needs to be implemented for every specific platform API.
	*/
	class UNREALAUDIO_API IUnrealAudioDeviceModule : public IModuleInterface,
													 public IDeviceErrorListener
	{
	public:
		/** Constructor */
		IUnrealAudioDeviceModule();

		/** Virtual destructor */
		virtual ~IUnrealAudioDeviceModule();

		/** 
		* Initializes the audio device module. 
		* @return true if succeeded.
		*/
		virtual bool Initialize() = 0;

		/** 
		* Shuts down the audio device module. 
		* @return true if succeeded.
		*/
		virtual bool Shutdown() = 0;

		/** 
		* Returns the API enumeration for the internal implementation of the device module. 
		* @param OutType Enumeration of the output tyep.
		* @return true if succeeded.
		*/
		virtual bool GetDevicePlatformApi(EDeviceApi::Type & OutType) const = 0;

		/** 
		* Returns the number of connected output devices to the system. 
		* @param OutNumDevices The number of output devices
		* @return true if succeeded.
		*/
		virtual bool GetNumOutputDevices(uint32& OutNumDevices) const = 0;

		/** 
		* Returns information about the output device at that index. 
		* @param DeviceIndex The index of the output device. 
		* @param OutInfo Information struct about the device.
		* @return true if succeeded.
		* @note DeviceIndex should be less than total number of devices given by GetNumOutputDevices.
		*/
		virtual bool GetOutputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const = 0;

		/** 
		* Returns the number of input devices. 
		* @param OutNumDevices The number of input devices.
		* @return true if succeeded.
		*/
		virtual bool GetNumInputDevices(uint32& OutNumDevices) const = 0;

		/** 
		* Returns information about the input device at the given index. 
		* @param DeviceIndex The index of the input device.
		* @param OutInfo Information struct about the device.
		* @return true if succeeded.
		*/
		virtual bool GetInputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const = 0;

		/**
		* Returns the default output device index.
		* @param OutDefaultIndex The default device index.
		* @return true if succeeded.
		* @note The default device is specified by the OS default device output.
		*/
		virtual bool GetDefaultOutputDeviceIndex(uint32& OutDefaultIndex) const = 0;

		/**
		* Returns the default input device index.
		* @param OutDefaultIndex The default device index.
		* @return true if succeeded.
		* @note The default device is specified by the OS default device input.
		*/
		virtual bool GetDefaultInputDeviceIndex(uint32& OutDefaultIndex) const = 0;

		/**
		* Starts the device audio stream.
		* @return true if succeeded.
		* @note The device stream needs to have been created before starting.
		*/
		virtual bool StartStream() = 0;

		/**
		* Stops the device audio stream.
		* @return true if succeeded.
		* @note The device stream needs to have been created and started before stopping.
		*/
		virtual bool StopStream() = 0;

		/**
		* Frees resources of the device audio stream.
		* @return true if succeeded.
		* @note The device stream needs to have been created before shutting down.
		*/
		virtual bool ShutdownStream() = 0;

		/**
		* Returns the latency of the input and output devices.
		* @param OutputDeviceLatency The latency of the output device as reported by platform API
		* @param InputDeviceLatency The latency of the input device as reported by platform API
		* @return true if succeeded.
		* @note The device stream needs to have been started for this return anything. A return of 0 means
		* the device hasn't started or that device hasn't been initialized.
		*/
		virtual bool GetLatency(uint32& OutputDeviceLatency, uint32& InputDeviceLatency) const = 0;

		/**
		* Returns the frame rate of the audio devices.
		* @param OutFrameRate The frame rate of the output audio device.
		* @return true if succeeded.
		* @note FrameRate is also known as SampleRate. A "Frame" is the minimal time delta of 
		* audio and is composed of interleaved samples. e.g. 1 stereo frame is 2 samples: left and right.
		*/
		virtual bool GetFrameRate(uint32& OutFrameRate) const = 0;

		/**
		* Creates an audio stream given the input parameter struct.
		* @param Params The parameters needed to create an audio stream.
		* @return true if succeeded.
		*/
		bool CreateStream(const FCreateStreamParams& Params);

		/**
		* Function called when an error occurs in the device code.
		* @param Error The error type that occurred.
		* @param ErrorDetails A string of specific details about the error.
		* @param FileName The file that the error occurred in.
		* @param LineNumber The line number that the error occurred in.
		*/
		void OnDeviceError(const EDeviceError::Type Error, const FString& ErrorDetails, const FString& FileName, int32 LineNumber) const override;

	protected:

		/**
		* FBufferFormatConvertInfo
		* Struct used to convert data formats (e.g. float, int32, etc) to and from the device to user callback stream.
		* The user audio callback will always be in floats, but audio devices may have native formats that are different, so 
		* we need to support converting data formats.
		*/
		struct FBufferFormatConvertInfo
		{
			/** The base number of channels for the format conversion. */
			uint32 NumChannels;

			/** The number of channels we're converting from. */
			uint32 FromChannels;

			/** The number of channels we're to. */
			uint32 ToChannels;

			/** The data format we're converting from. */
			EStreamFormat::Type FromFormat;

			/** The data format we're converting to. */
			EStreamFormat::Type	ToFormat;
		};

		/**
		* FStreamDeviceInfo
		* Struct used to represent information about a particular device (input or output device)
		*/
		struct FStreamDeviceInfo
		{
			/** The index this device is in (from list of devices of this type). */
			uint32 DeviceIndex;

			/** The speaker types this device uses. */
			TArray<ESpeaker::Type> Speakers;

			/** The number of channels this device actually supports. */
			uint32 NumChannels;

			/** The reported latency of this device. */
			uint64 Latency;

			/** The native framerate of this device. */
			uint32 FrameRate;

			/** The native data format of this device. */
			EStreamFormat::Type DeviceDataFormat;

			/** Conversion information to convert audio streams to/from this device. */
			FBufferFormatConvertInfo BufferFormatConvertInfo;

			/** A buffer used to store data to/from this device. */
			TArray<uint8> UserBuffer;

			/** True if we need to perform a format conversion. */
			bool bPerformFormatConversion;

			/** True if we need to perform a byte swap for this device. */
			bool bPerformByteSwap;

			/** Constructor */
			FStreamDeviceInfo()
			{
				Initialize();
			}

			/** Function which initializes the struct variables to default values. Maye be called multiple times. */
			void Initialize()
			{
				DeviceIndex = 0;
				NumChannels = 0;
				Latency = 0;
				FrameRate = 0;
				DeviceDataFormat = EStreamFormat::UNKNOWN;
				bPerformFormatConversion = false;
				bPerformByteSwap = false;

				BufferFormatConvertInfo.NumChannels = 0;
				BufferFormatConvertInfo.FromChannels = 0;
				BufferFormatConvertInfo.ToChannels = 0;
				BufferFormatConvertInfo.FromFormat = EStreamFormat::UNKNOWN;
				BufferFormatConvertInfo.ToFormat = EStreamFormat::UNKNOWN;
			}
		};

		/**
		* FStreamInfo
		* Struct used to represent general information about the audio stream.
		*/
		class FStreamInfo
		{
		public:
			/** The overall framerate of the stream. This may be different from device frame rate. */
			uint32 FrameRate;

			/** The type of stream this is (input or output) */
			EStreamType::Type StreamType;

			/** The current state of the stream. */
			EStreamState::Type State;

			/** Information on the user callback */
			FCallbackInfo CallbackInfo;

			/** The sample-accurate running time of the stream in seconds (i.e. not necessarily real-world time but stream time). */
			double StreamTime;

			/** The amount of time that passes per update block. */
			double StreamDelta;

			/** Running audio thread */
			FRunnableThread* Thread;

			/** User callback function */
			StreamCallback CallbackFunction;

			/** User data */
			void* UserData;

			/** The size of the callback frame count*/
			uint32 BlockSize;

			/** A byte array used to store data to and from audio devices. */
			TArray<uint8> DeviceBuffer;

			/** An array of device-specific information for input and output devices. */
			FStreamDeviceInfo DeviceInfo[EStreamType::COUNT];

			/** Constructor */
			FStreamInfo()
			{
				// Just call intialize since it will get called when starting up a new stream.
				Initialize();
			}

			/** Gets called every time a stream is opened, is reset in cae the device data exists from a previous open. */
			void Initialize()
			{
				FrameRate = 0;
				StreamType = EStreamType::COUNT;
				State = EStreamState::CLOSED;
				StreamTime = 0.0;
				StreamDelta = 0.0;

				CallbackFunction = nullptr;
				Thread = nullptr;
				UserData = nullptr;
				BlockSize = 0;

				for (uint32 Id = 0; Id < EStreamType::COUNT; ++Id)
				{
					DeviceInfo[Id].Initialize();
				}
			}

			/** Helper function to get specific convert information based on stream type */
			FBufferFormatConvertInfo& GetConvertInfo(EStreamType::Type StreamType)
			{
				check(StreamType < EStreamType::COUNT);
				return DeviceInfo[StreamType].BufferFormatConvertInfo;
			}

			/** Helper function to get device info based on stream type */
			FStreamDeviceInfo& GetDeviceInfo(EStreamType::Type StreamType)
			{
				check(StreamType < EStreamType::COUNT);
				return DeviceInfo[StreamType];
			}

			/** Helper function to get device info based on stream type const overload */
			const FStreamDeviceInfo& GetDeviceInfo(EStreamType::Type StreamType) const
			{
				check(StreamType < EStreamType::COUNT);
				return DeviceInfo[StreamType];
			}
		};

	protected: // Protected Methods

		/** Opens audio devices given the input params. Implemented per platform */
		virtual bool OpenDevices(const FCreateStreamParams& Params) = 0;

		/** Called before opening up new streams. */
		void Reset();

		/** Helper function to get number of bytes for a given stream format. */
		uint32 GetNumBytesForFormat(EStreamFormat::Type Format) const;

		/** Sets up any convert information for given stream type (figures out to/from convert format and channel formats) */
		void SetupBufferFormatConvertInfo(EStreamType::Type StreamType);

		/** Performs actual buffer format and channel conversion */
		bool ConvertBufferFormat(TArray<uint8>& OutputBuffer, TArray<uint8>& InputBuffer, EStreamType::Type StreamType);

		/** Updates the sample-accurate stream time value. */
		void UpdateStreamTimeTick();

		/** Templated function to convert to float from any other format type. */
		template <typename FromType, typename ToType>
		void ConvertToFloatType(TArray<uint8>& ToBuffer, TArray<uint8>& FromBuffer, uint32 NumFrames, FBufferFormatConvertInfo& ConvertInfo);

		/** Templated function to any format type to float. */
		template <typename FloatType>
		void ConvertAllToFloatType(TArray<uint8>& ToBuffer, TArray<uint8>& FromBuffer, uint32 NumFrames, FBufferFormatConvertInfo& ConvertInfo);

		/** Templated function to integer type to any other integer type. */
		template <typename FromType, typename ToType>
		void ConvertIntegerToIntegerType(TArray<uint8>& ToBuffer, TArray<uint8>& FromBuffer, uint32 NumFrames, FBufferFormatConvertInfo& ConvertInfo);

		/** Templated function to convert a float type to integer type. */
		template <typename FromType, typename ToType>
		void ConvertFloatToIntegerType(TArray<uint8>& ToBuffer, TArray<uint8>& FromBuffer, uint32 NumFrames, FBufferFormatConvertInfo& ConvertInfo);

		/** Templated function to any type to integer type. */
		template <typename FloatType>
		void ConvertAllToIntegerType(TArray<uint8>& ToBuffer, TArray<uint8>& FromBuffer, uint32 NumFrames, FBufferFormatConvertInfo& ConvertInfo);

	protected: // Protected Data

		/** An array of possible frame rates that can exist with any audio device. */
		static const uint32 PossibleFrameRates[];
		static const uint32 MaxPossibleFrameRates;

		/** An object which listens to device errors. */
		IDeviceErrorListener* DeviceErrorListener;

		/** Device stream info struct */
		FStreamInfo StreamInfo;
	};

	// Exported Functions
	UNREALAUDIO_API IUnrealAudioDeviceModule* CreateDummyDeviceModule();
}

#endif // #if ENABLE_UNREAL_AUDIO



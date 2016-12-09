using System;
using System.Runtime.InteropServices;

namespace Tools.CrashReporter.CrashReportProcess
{
	static class NativeMethods
	{
		public static unsafe int UncompressMemoryZlib(byte[] OutUncompressedBuffer, byte[] InCompressedBuffer)
		{
			fixed (byte* UncompressedBufferPtr = OutUncompressedBuffer, CompressedBufferPtr = InCompressedBuffer)
			{
				return UE4UncompressMemoryZLIB((IntPtr)UncompressedBufferPtr, OutUncompressedBuffer.Length, (IntPtr)CompressedBufferPtr, InCompressedBuffer.Length);
			}
		}

		public static unsafe int CompressMemoryZlib(byte[] OutCompressedBuffer, byte[] InUncompressedBuffer)
		{
			fixed (byte* CompressedBufferPtr = OutCompressedBuffer, UncompressedBufferPtr = InUncompressedBuffer)
			{
				return UE4CompressMemoryZLIB((IntPtr)CompressedBufferPtr, OutCompressedBuffer.Length, (IntPtr)UncompressedBufferPtr, InUncompressedBuffer.Length);
			}
		}

		public static unsafe int CompressFileGZIP(string InFilepath, byte[] InUncompressedBuffer)
		{
			fixed (byte* UncompressedBufferPtr = InUncompressedBuffer)
			{
				//fixed (char* FilepathPtr = InFilepath)
				{
					return UE4CompressFileGZIP(InFilepath, (IntPtr)UncompressedBufferPtr, InUncompressedBuffer.Length);
				}
			}
		}

		[DllImport("CompressionHelper.dll", CallingConvention = CallingConvention.Cdecl)]
		private static extern Int32 UE4UncompressMemoryZLIB( /*void**/ IntPtr UncompressedBuffer, Int32 UncompressedSize, /*const void**/ IntPtr CompressedBuffer, Int32 CompressedSize);

		[DllImport("CompressionHelper.dll", CallingConvention = CallingConvention.Cdecl)]
		private static extern Int32 UE4CompressMemoryZLIB( /*void**/ IntPtr CompressedBuffer, Int32 CompressedSize, /*const void**/ IntPtr UncompressedBuffer, Int32 UncompressedSize);

		[DllImport("CompressionHelper.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
		private static extern Int32 UE4CompressFileGZIP( /*const char**/ string Path, /*const void**/ IntPtr UncompressedBuffer, Int32 UncompressedSize);

		public static string GetZlibError(Int32 ErrorCode)
		{
			if (ErrorCode == 0) return "Z_OK";
			if (ErrorCode == 1) return "Z_STREAM_END";
			if (ErrorCode == 2) return "Z_NEED_DICT";
			if (ErrorCode == -1) return "Z_ERRNO";
			if (ErrorCode == -2) return "Z_STREAM_ERROR";
			if (ErrorCode == -3) return "Z_DATA_ERROR";
			if (ErrorCode == -4) return "Z_MEM_ERROR";
			if (ErrorCode == -5) return "Z_BUF_ERROR";
			if (ErrorCode == -6) return "Z_VERSION_ERROR";
			return "UnknownZLibError";
		}
	}
}

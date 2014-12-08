/**
 * Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using MobileDeviceInterface;

namespace Manzana
{
    /// <summary>
    /// Structures with unknown innards.  They should only ever be used as the target of a TypedPtr, never inspected.
    /// </summary>
    public class CFString { }
    public class CFURL {   }
    public class CFDictionary {  }
    public class CFNumber {  }
    public class CFBoolean { }

    internal class RequiredWinAPI
    {
        [DllImport("kernel32", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern IntPtr LoadLibrary(string lpFileName);

        [DllImport("kernel32", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern IntPtr GetModuleHandle(string lpModuleName);
    }

    /// <summary>
    /// Declarations for various methods in Apple's CoreFoundation.dll, needed for interop with the MobileDevice DLL
    /// </summary>
    internal partial class MobileDevice
    {
        public static IntPtr kCFAllocatorDefault;
        public static IntPtr kCFTypeDictionaryKeyCallBacks;
        public static IntPtr kCFTypeDictionaryValueCallBacks;

        public static TypedPtr<CFBoolean> kCFBooleanTrue;
        public static TypedPtr<CFBoolean> kCFBooleanFalse;

        static void InitializeCoreFoundation()
        {
            IntPtr CoreFoundation = RequiredWinAPI.GetModuleHandle("CoreFoundation.dll");

            kCFAllocatorDefault = RequiredWinAPI.GetProcAddress(CoreFoundation, "kCFAllocatorDefault");
            kCFTypeDictionaryKeyCallBacks = RequiredWinAPI.GetProcAddress(CoreFoundation, "kCFTypeDictionaryKeyCallBacks");
            kCFTypeDictionaryValueCallBacks = RequiredWinAPI.GetProcAddress(CoreFoundation, "kCFTypeDictionaryValueCallBacks");

            kCFBooleanTrue = RequiredWinAPI.GetProcAddress(CoreFoundation, "kCFBooleanTrue");
            kCFBooleanFalse = RequiredWinAPI.GetProcAddress(CoreFoundation, "kCFBooleanFalse");
        }

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr __CFStringMakeConstantString(byte[] s);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr/*CFURL*/ CFURLCreateWithFileSystemPath(IntPtr Allocator, IntPtr/*CFString*/ FilePath, CFURLPathStyle PathStyle, int isDirectory);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr/*CFString*/ CFURLGetString(IntPtr anURL);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint CFGetTypeID(IntPtr FromInstance);
        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint CFStringGetTypeID();
        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint CFNumberGetTypeID();
        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint CFBooleanGetTypeID();
        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint CFDictionaryGetTypeID();

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CFDictionaryAddValue(IntPtr/*CFDictionary*/ Dict, /*const*/ IntPtr Key, /*const*/ IntPtr Value);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CFDictionarySetValue(IntPtr/*CFDictionary*/ Dict, /*const*/ IntPtr Key, /*const*/ IntPtr Value);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void CFDictionaryGetKeysAndValues(IntPtr/*CFDictionary*/ Dict, /*const*/ IntPtr[] Keys, /*const*/ IntPtr[] Values);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr/*CFDictionary*/ CFDictionaryCreateMutable(
            /* CFAllocatorRef */ IntPtr allocator,
            /* CFIndex */ int capacity,
            /* const CFDictionaryKeyCallBacks* */ IntPtr keyCallBacks,
            /* const CFDictionaryValueCallBacks* */ IntPtr valueCallBacks
        );

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int CFDictionaryGetCount(IntPtr/*CFDictionary*/ Dict);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private extern static Boolean CFStringGetCString(IntPtr/*CFString*/ theString, byte[] buffer, int bufferSize, CFStringBuiltInEncodings encoding);

        //[DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        //public extern static IntPtr CFNumberCreate(IntPtr allocator, CFNumberType theType, IntPtr valuePtr);

        //@TODO: make this more general
        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private extern static IntPtr CFNumberCreate(IntPtr allocator, CFNumberType theType, ref Int32 Value);

        [DllImport("CoreFoundation.dll", EntryPoint = "CFNumberGetValue", CallingConvention = CallingConvention.Cdecl)]
        private extern static Boolean CFNumberGetValue(IntPtr/*CFNumber*/ Number, CFNumberType TheType, out float Value);

        [DllImport("CoreFoundation.dll", EntryPoint = "CFNumberGetValue", CallingConvention = CallingConvention.Cdecl)]
        private extern static Boolean CFNumberGetValue(IntPtr/*CFNumber*/ Number, CFNumberType TheType, out double Value);

        [DllImport("CoreFoundation.dll", EntryPoint = "CFNumberGetValue", CallingConvention = CallingConvention.Cdecl)]
        private extern static Boolean CFNumberGetValue(IntPtr/*CFNumber*/ Number, CFNumberType TheType, out int Value);

        [DllImport("CoreFoundation.dll", EntryPoint = "CFNumberGetValue", CallingConvention = CallingConvention.Cdecl)]
        private extern static Boolean CFNumberGetValue(IntPtr/*CFNumber*/ Number, CFNumberType TheType, out uint Value);

        [DllImport("CoreFoundation.dll", EntryPoint = "CFNumberGetValue", CallingConvention = CallingConvention.Cdecl)]
        private extern static Boolean CFNumberGetValue(IntPtr/*CFNumber*/ Number, CFNumberType TheType, out Int64 Value);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        private extern static CFNumberType CFNumberGetType(IntPtr/*CFNumber*/ Number);

        [DllImport("CoreFoundation.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CFPreferencesSetAppValue(IntPtr Key, IntPtr Value, IntPtr ApplicationID);






        public enum CFStringBuiltInEncodings
        {
            kCFStringEncodingMacRoman = 0x00000000,
            kCFStringEncodingWindowsLatin1 = 0x00000500,
            kCFStringEncodingISOLatin1 = 0x00000201,
            kCFStringEncodingNextStepLatin = 0x00000B01,
            kCFStringEncodingASCII = 0x00000600,
            kCFStringEncodingUnicode = 0x00000100,
            kCFStringEncodingUTF8 = 0x08000100,
            kCFStringEncodingNonLossyASCII = 0x00000BFF
        };

        public enum CFURLPathStyle
        {
            kCFURLPOSIXPathStyle = 0,
            kCFURLHFSPathStyle = 1,
            kCFURLWindowsPathStyle = 2
        };

        public enum CFNumberType
        {
            kCFNumberSInt8Type = 1,
            kCFNumberSInt16Type = 2,
            kCFNumberSInt32Type = 3,
            kCFNumberSInt64Type = 4,
            kCFNumberFloat32Type = 5,
            kCFNumberFloat64Type = 6,
            kCFNumberCharType = 7,
            kCFNumberShortType = 8,
            kCFNumberIntType = 9,
            kCFNumberLongType = 10,
            kCFNumberLongLongType = 11,
            kCFNumberFloatType = 12,
            kCFNumberDoubleType = 13,
            kCFNumberCFIndexType = 14,
            kCFNumberNSIntegerType = 15,
            kCFNumberCGFloatType = 16,
            kCFNumberMaxType = 16
        };






        private static byte[] StringToCString(string value)
        {
            byte[] bytes = new byte[value.Length + 1];
            Encoding.ASCII.GetBytes(value, 0, value.Length, bytes, 0);
            return bytes;
        }

        public static TypedPtr<CFString> CFStringMakeConstantString(string s)
        {
            return __CFStringMakeConstantString(StringToCString(s));
        }

        public static TypedPtr<CFURL> CFURLCreateWithFileSystemPath(IntPtr Allocator, TypedPtr<CFString> FilePath, CFURLPathStyle PathStyle, int isDirectory)
        {
            return (TypedPtr<CFURL>)(CFURLCreateWithFileSystemPath(Allocator, FilePath.Handle, PathStyle, isDirectory));
        }

        public static string CFStringGetCString(TypedPtr<CFString> InString)
        {
            byte[] bytes = new byte[2048];
            CFStringGetCString(InString.Handle, bytes, 2048, CFStringBuiltInEncodings.kCFStringEncodingUTF8);

            int ValidLength = 0;
            foreach (byte b in bytes)
            {
                if (b == 0)
                {
                    break;
                }
                else
                {
                    ValidLength++;
                }
            }

            return Encoding.UTF8.GetString(bytes, 0, ValidLength);
        }

        public static string GetStringForUrl(TypedPtr<CFURL> Url)
        {
            IntPtr cfString = CFURLGetString((IntPtr)Url);
            return CFStringGetCString((IntPtr)cfString);
        }

        public static TypedPtr<CFURL> CreateFileUrlHelper(string InPath, bool bIsDirectory)
        {
            return CFURLCreateWithFileSystemPath(MobileDevice.kCFAllocatorDefault, CFStringMakeConstantString(InPath), CFURLPathStyle.kCFURLWindowsPathStyle, bIsDirectory ? 1 : 0);
        }

        public static TypedPtr<CFNumber> CFNumberCreate(Int32 Value)
        {
            return CFNumberCreate(kCFAllocatorDefault, CFNumberType.kCFNumberSInt32Type, ref Value);
        }

        public static TypedPtr<CFDictionary> CreateCFDictionaryHelper()
        {
            return CFDictionaryCreateMutable(kCFAllocatorDefault, 0, kCFTypeDictionaryKeyCallBacks, kCFTypeDictionaryValueCallBacks);
        }

        /// <summary>
        /// Converts a CF type to a managed type (e.g., CFString to String)
        /// </summary>
        /// <param name="Source"></param>
        /// <returns></returns>
        public static object ConvertArbitraryCFType(IntPtr Source)
        {
            if (Source == IntPtr.Zero)
            {
                return null;
            }

            // Convert based on the type of the source object
            uint TypeID = CFGetTypeID(Source);

            if (TypeID == CFNumberGetTypeID())
            {
                return ConvertCFNumber(Source);
            }
            else if (TypeID == CFStringGetTypeID())
            {
                return CFStringGetCString(Source);
            }
            else if (TypeID == CFDictionaryGetTypeID())
            {
                return ConvertCFDictionaryToDictionary(Source);
            }
            else if (TypeID == CFBooleanGetTypeID())
            {
                return (Source == kCFBooleanTrue.Handle);
            }

            return null;
        }

        /// <summary>
        /// Converts an arbitrary CFNumber to a double
        /// </summary>
        public static double ConvertCFNumber(TypedPtr<CFNumber> Number)
        {
            CFNumberType TypeID = CFNumberGetType((IntPtr)Number);

            switch (TypeID)
            {
                case CFNumberType.kCFNumberFloat32Type:
                case CFNumberType.kCFNumberFloatType:
                    {
                        float Result;
                        CFNumberGetValue((IntPtr)Number, CFNumberType.kCFNumberFloat32Type, out Result);
                        return Result;
                    }

                case CFNumberType.kCFNumberFloat64Type:
                case CFNumberType.kCFNumberDoubleType:
                case CFNumberType.kCFNumberCGFloatType:
                    {
                        double Result;
                        CFNumberGetValue((IntPtr)Number, CFNumberType.kCFNumberFloat64Type, out Result);
                        return Result;
                    }

                case CFNumberType.kCFNumberSInt8Type:
                case CFNumberType.kCFNumberSInt16Type:
                case CFNumberType.kCFNumberSInt32Type:
                case CFNumberType.kCFNumberCharType:
                case CFNumberType.kCFNumberShortType:
                case CFNumberType.kCFNumberIntType:
                case CFNumberType.kCFNumberLongType:
                case CFNumberType.kCFNumberCFIndexType:
                    {
                        int Result;
                        CFNumberGetValue((IntPtr)Number, CFNumberType.kCFNumberIntType, out Result);
                        return Result;
                    }

                case CFNumberType.kCFNumberSInt64Type:
                case CFNumberType.kCFNumberLongLongType:
                case CFNumberType.kCFNumberNSIntegerType:
                    {
                        Int64 Result;
                        CFNumberGetValue((IntPtr)Number, CFNumberType.kCFNumberSInt64Type, out Result);
                        return Result;
                    }

                default:
                    return 0.0;
            }
        }

        /// <summary>
        /// Converts a CFDictionary into a Dictionary (keys and values are object)
        /// </summary>
        public static Dictionary<object, object> ConvertCFDictionaryToDictionary(TypedPtr<CFDictionary> Dict)
        {
            // Get the raw key-value pairs
            int NumPairs = CFDictionaryGetCount(Dict);

            IntPtr [] Keys = new IntPtr[NumPairs];
            IntPtr [] Values = new IntPtr[NumPairs];

            CFDictionaryGetKeysAndValues((IntPtr)Dict, Keys, Values);

            // Convert each key and value to a managed type
            Dictionary<object, object> Result = new Dictionary<object, object>();
            for (int i = 0; i < NumPairs; ++i)
            {
                try
                {
                    Result.Add(ConvertArbitraryCFType(Keys[i]), ConvertArbitraryCFType(Values[i]));
                }
                catch (Exception)
                {
                    Console.WriteLine("Unable to properly convert dictionary");
                }
            }

            return Result;
        }

        public static Dictionary<string, object> ConvertCFDictionaryToDictionaryStringy(TypedPtr<CFDictionary> Dict)
        {
            Dictionary<string, object> Result = new Dictionary<string,object>();
            Dictionary<object, object> Temp = ConvertCFDictionaryToDictionary(Dict);

            foreach (KeyValuePair<object, object> KVP in Temp)
            {
                string Key = (string)KVP.Key;
                Result.Add(Key, KVP.Value);
            }

            return Result;
        }

        public static int CFDictionaryGetCount(TypedPtr<CFDictionary> Dict)
        {
            return CFDictionaryGetCount((IntPtr)Dict);
        }

        public static void CFDictionaryAddHelper(TypedPtr<CFDictionary> InDict, string Key, string Value)
        {
            CFDictionarySetValue((IntPtr)InDict, (IntPtr)CFStringMakeConstantString(Key), (IntPtr)CFStringMakeConstantString(Value));
        }
    }
}
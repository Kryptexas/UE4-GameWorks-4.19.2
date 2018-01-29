// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Tools.DotNETCommon;

namespace UnrealBuildTool
{
	/// <summary>
	/// Helper functions for dealing with encryption and pak signing
	/// </summary>
	public static class EncryptionAndSigning
	{
		/// <summary>
		/// Wrapper class for a single RSA key
		/// </summary>
		public class SigningKey
		{
			/// <summary>
			/// Exponent
			/// </summary>
			public byte[] Exponent;

			/// <summary>
			/// Modulus
			/// </summary>
			public byte[] Modulus;
		}

		/// <summary>
		/// Wrapper class for an RSA public/private key pair
		/// </summary>
		public class SigningKeyPair
		{
			/// <summary>
			/// Public key
			/// </summary>
			public SigningKey PublicKey = new SigningKey();

			/// <summary>
			/// Private key
			/// </summary>
			public SigningKey PrivateKey = new SigningKey();
		}

		/// <summary>
		/// Wrapper class for a 128 bit AES encryption key
		/// </summary>
		public class EncryptionKey
		{
			/// <summary>
			/// 128 bit AES key
			/// </summary>
			public byte[] Key;
		}

		/// <summary>
		/// Wrapper class for all crypto settings
		/// </summary>
		public class CryptoSettings
		{
			/// <summary>
			/// AES encyption key
			/// </summary>
			public EncryptionKey EncryptionKey = null;

			/// <summary>
			/// RSA public/private key
			/// </summary>
			public SigningKeyPair SigningKey = null;

			/// <summary>
			/// Enable pak signature checking
			/// </summary>
			public bool bEnablePakSigning = false;

			/// <summary>
			/// Encrypt the index of the pak file. Stops the pak file being easily accessible by unrealpak
			/// </summary>
			public bool bEnablePakIndexEncryption = false;

			/// <summary>
			/// Encrypt all ini files in the pak. Good for game data obsfucation
			/// </summary>
			public bool bEnablePakIniEncryption = false;

			/// <summary>
			/// Encrypt the uasset files in the pak file. After cooking, uasset files only contain package metadata / nametables / export and import tables. Provides good string data obsfucation without
			/// the downsides of full package encryption, with the security drawbacks of still having some data stored unencrypted 
			/// </summary>
			public bool bEnablePakUAssetEncryption = false;

			/// <summary>
			/// Encrypt all assets data files (including exp and ubulk) in the pak file. Likely to be slow, and to cause high data entropy (bad for delta patching)
			/// </summary>
			public bool bEnablePakFullAssetEncryption = false;

			/// <summary>
			/// Some platforms have their own data crypto systems, so allow the config settings to totally disable our own crypto
			/// </summary>
			public bool bDataCryptoRequired = false;

			/// <summary>
			/// 
			/// </summary>
			public bool IsAnyEncryptionEnabled()
			{
				return bEnablePakFullAssetEncryption || bEnablePakUAssetEncryption || bEnablePakIndexEncryption || bEnablePakIniEncryption;
			}
		}

		/// <summary>
		/// Parse crypto settings from INI file
		/// </summary>
		public static CryptoSettings ParseCryptoSettings(DirectoryReference InProjectDirectory, UnrealTargetPlatform InTargetPlatform)
		{
			CryptoSettings Settings = new CryptoSettings();

			ConfigHierarchy Ini = ConfigCache.ReadHierarchy(ConfigHierarchyType.Engine, InProjectDirectory, InTargetPlatform);
			Ini.GetBool("PlatformCrypto", "PlatformRequiresDataCrypto", out Settings.bDataCryptoRequired);

			// For now, we'll just not parse any keys if data crypto is disabled for this platform. In the future, we might want to use
			// these keys for non-data purposes (other general purpose encryption maybe?)
			if (!Settings.bDataCryptoRequired)
			{
				return Settings;
			}

			Ini = ConfigCache.ReadHierarchy(ConfigHierarchyType.Crypto, InProjectDirectory, InTargetPlatform);
			string SectionName = "/Script/CryptoKeys.CryptoKeysSettings";
			ConfigHierarchySection CryptoSection = Ini.FindSection(SectionName);

			// If we have new format crypto keys, use them
			if (CryptoSection != null && CryptoSection.KeyNames.Count() > 0)
			{
				Ini.GetBool(SectionName, "bEnablePakSigning", out Settings.bEnablePakSigning);
				Ini.GetBool(SectionName, "bEncryptPakIniFiles", out Settings.bEnablePakIniEncryption);
				Ini.GetBool(SectionName, "bEncryptPakIndex", out Settings.bEnablePakIndexEncryption);
				Ini.GetBool(SectionName, "bEncryptUAssetFiles", out Settings.bEnablePakUAssetEncryption);
				Ini.GetBool(SectionName, "bEncryptAllAssetFiles", out Settings.bEnablePakFullAssetEncryption);

				// Parse encryption key
				string EncryptionKeyString;
				Ini.GetString(SectionName, "EncryptionKey", out EncryptionKeyString);
				if (!string.IsNullOrEmpty(EncryptionKeyString))
				{
					Settings.EncryptionKey = new EncryptionKey();
					Settings.EncryptionKey.Key = System.Convert.FromBase64String(EncryptionKeyString);

					if (Settings.EncryptionKey.Key.Length != 32)
					{
						throw new Exception("The encryption key specified in the crypto config file must be 32 bytes long!");
					}
				}

				// Parse signing key
				string PrivateExponent, PublicExponent, Modulus;
				Ini.GetString(SectionName, "SigningPrivateExponent", out PrivateExponent);
				Ini.GetString(SectionName, "SigningModulus", out Modulus);
				Ini.GetString(SectionName, "SigningPublicExponent", out PublicExponent);

				if (!String.IsNullOrEmpty(PrivateExponent) && !String.IsNullOrEmpty(PublicExponent) && !String.IsNullOrEmpty(Modulus))
				{
					Settings.SigningKey = new SigningKeyPair();
					Settings.SigningKey.PublicKey.Exponent = System.Convert.FromBase64String(PublicExponent);
					Settings.SigningKey.PublicKey.Modulus = System.Convert.FromBase64String(Modulus);
					Settings.SigningKey.PrivateKey.Exponent = System.Convert.FromBase64String(PrivateExponent);
					Settings.SigningKey.PrivateKey.Modulus = Settings.SigningKey.PublicKey.Modulus;
				}
			}
			else
			{
				// We don't have new format crypto keys in a crypto.ini file, so try and find the old format settings
				Ini = ConfigCache.ReadHierarchy(ConfigHierarchyType.Encryption, InProjectDirectory, InTargetPlatform);
				Ini.GetBool("Core.Encryption", "SignPak", out Settings.bEnablePakSigning);

				string[] SigningKeyStrings = new string[3];
				Ini.GetString("Core.Encryption", "rsa.privateexp", out SigningKeyStrings[0]);
				Ini.GetString("Core.Encryption", "rsa.modulus", out SigningKeyStrings[1]);
				Ini.GetString("Core.Encryption", "rsa.publicexp", out SigningKeyStrings[2]);

				if (String.IsNullOrEmpty(SigningKeyStrings[0]) || String.IsNullOrEmpty(SigningKeyStrings[1]) || String.IsNullOrEmpty(SigningKeyStrings[2]))
				{
					SigningKeyStrings = null;
				}
				else
				{
					Settings.SigningKey = new SigningKeyPair();
					Settings.SigningKey.PrivateKey.Exponent = ParseHexStringToByteArray(SigningKeyStrings[0]);
					Settings.SigningKey.PrivateKey.Modulus = ParseHexStringToByteArray(SigningKeyStrings[1]);
					Settings.SigningKey.PublicKey.Exponent = ParseHexStringToByteArray(SigningKeyStrings[2]);
					Settings.SigningKey.PublicKey.Modulus = Settings.SigningKey.PrivateKey.Modulus;
				}

				Ini.GetBool("Core.Encryption", "EncryptPak", out Settings.bEnablePakIndexEncryption);
				Settings.bEnablePakFullAssetEncryption = false;
				Settings.bEnablePakUAssetEncryption = false;
				Settings.bEnablePakIniEncryption = Settings.bEnablePakIndexEncryption;

				string EncryptionKeyString;
				Ini.GetString("Core.Encryption", "aes.key", out EncryptionKeyString);
				Settings.EncryptionKey = new EncryptionKey();
				Settings.EncryptionKey.Key = ParseAnsiStringToByteArray(EncryptionKeyString);
			}

			return Settings;
		}

		/// <summary>
		/// Take a hex string and parse into an array of bytes
		/// </summary>
		private static byte[] ParseHexStringToByteArray(string InString)
		{
			if (InString.StartsWith("0x"))
			{
				InString = InString.Substring(2);
			}

			List<byte> Bytes = new List<byte>();
			while (InString.Length > 0)
			{
				int CharsToParse = Math.Min(2, InString.Length);
				string Value = InString.Substring(InString.Length - CharsToParse);
				InString = InString.Substring(0, InString.Length - CharsToParse);
				Bytes.Add(byte.Parse(Value, System.Globalization.NumberStyles.AllowHexSpecifier));
			}
			return Bytes.ToArray();
		}

		private static byte[] ParseAnsiStringToByteArray(string InString)
		{
			List<byte> Bytes = new List<byte>();
			foreach (char C in InString)
			{
				Bytes.Add((byte)C);
			}
			return Bytes.ToArray();
		}
	}
}

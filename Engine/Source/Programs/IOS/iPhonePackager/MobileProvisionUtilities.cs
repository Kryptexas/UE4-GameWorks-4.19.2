/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Security.Cryptography.X509Certificates;
using System.Security.Cryptography;
using System.IO;
using System.Diagnostics;
using System.Xml;

namespace iPhonePackager
{
	/// <summary>
	/// Represents the salient parts of a mobile provision, wrt. using it for code signing
	/// </summary>
	public class MobileProvision
	{
		public object Tag;

		public string ApplicationIdentifierPrefix = null;
		public List<X509Certificate2> DeveloperCertificates = new List<X509Certificate2>();
		public List<string> ProvisionedDeviceIDs;
		public string ProvisionName;
		public Utilities.PListHelper Data;

		/// <summary>
		/// Extracts the dict values for the Entitlements key and creates a new full .plist file
		/// from them (with outer plist and dict keys as well as doctype, etc...)
		/// </summary>
		public string GetEntitlementsString(string CFBundleIdentifier)
		{
			Utilities.PListHelper XCentPList = null;
			Data.ProcessValueForKey("Entitlements", "dict", delegate(XmlNode ValueNode)
			{
				XCentPList = Utilities.PListHelper.CloneDictionaryRootedAt(ValueNode);
			});

			// Modify the application-identifier to be fully qualified if needed
			string CurrentApplicationIdentifier;
			XCentPList.GetString("application-identifier", out CurrentApplicationIdentifier);

			if (CurrentApplicationIdentifier.Contains("*"))
			{
				// Replace the application identifier
				string NewApplicationIdentifier = String.Format("{0}.{1}", ApplicationIdentifierPrefix, CFBundleIdentifier);
				XCentPList.SetString("application-identifier", NewApplicationIdentifier);


				// Replace the keychain access groups
				// Note: This isn't robust, it ignores the existing value in the wildcard and uses the same value for
				// each entry.  If there is a legitimate need for more than one entry in the access group list, then
				// don't use a wildcard!
				List<string> KeyGroups = XCentPList.GetArray("keychain-access-groups", "string");

				for (int i = 0; i < KeyGroups.Count; ++i)
				{
					string Entry = KeyGroups[i];
					if (Entry.Contains("*"))
					{
						Entry = NewApplicationIdentifier;
					}
					KeyGroups[i] = Entry;
				}

				XCentPList.SetValueForKey("keychain-access-groups", KeyGroups);
			}

			return XCentPList.SaveToString();
		}

		/// <summary>
		/// Constructs a MobileProvision from an xml blob extracted from the real ASN.1 file
		/// </summary>
		public MobileProvision(string EmbeddedPListText)
		{
			Data = new Utilities.PListHelper(EmbeddedPListText);

			// Now extract things

			// Key: ApplicationIdentifierPrefix, Array<String>
			List<string> PrefixList = Data.GetArray("ApplicationIdentifierPrefix", "string");
			if (PrefixList.Count > 1)
			{
				Program.Warning("Found more than one entry for ApplicationIdentifierPrefix in the .mobileprovision, using the first one found");
			}

			if (PrefixList.Count > 0)
			{
				ApplicationIdentifierPrefix = PrefixList[0];
			}

			// Key: DeveloperCertificates, Array<Data> (uuencoded)
			string CertificatePassword = "";
			List<string> CertificateList = Data.GetArray("DeveloperCertificates", "data");
			foreach (string EncodedCert in CertificateList)
			{
				byte[] RawCert = Convert.FromBase64String(EncodedCert);
				DeveloperCertificates.Add(new X509Certificate2(RawCert, CertificatePassword));
			}

			// Key: Name, String
			if (!Data.GetString("Name", out ProvisionName))
			{
				ProvisionName = "(unknown)";
			}

			// Key: ProvisionedDevices, Array<String>
			ProvisionedDeviceIDs = Data.GetArray("ProvisionedDevices", "string");
		}

		/// <summary>
		/// Does this provision contain the specified UDID?
		/// </summary>
		public bool ContainsUDID(string UDID)
		{
			bool bFound = false;
			foreach (string TestUDID in ProvisionedDeviceIDs)
			{
				if (TestUDID.Equals(UDID, StringComparison.InvariantCultureIgnoreCase))
				{
					bFound = true;
					break;
				}
			}

			return bFound;
		}
	}

	/// <summary>
	/// This class understands how to get the embedded plist in a .mobileprovision file.  It doesn't
	/// understand the full format and is not capable of writing a new one out or anything similar.
	/// </summary>
	public class MobileProvisionParser
	{
		public static MobileProvision ParseFile(byte[] RawData)
		{
			//@TODO: This file is just an ASN.1 stream, should find or make a raw ASN1 parser and use
			// that instead of this (theoretically fragile) code (particularly the length extraction)

			// Scan it for the start of the embedded blob of xml
			byte[] SearchPattern = Encoding.UTF8.GetBytes("<?xml");
			for (int TextStart = 2; TextStart < RawData.Length - SearchPattern.Length; ++TextStart)
			{
				// See if this point is a match
				bool bMatch = true;
				for (int PatternIndex = 0; bMatch && (PatternIndex < SearchPattern.Length); ++PatternIndex)
				{
					bMatch = bMatch && (RawData[TextStart + PatternIndex] == SearchPattern[PatternIndex]);
				}

				if (bMatch)
				{
					// Back up two bytes and read a two byte little endian plist size
					int TextLength = (RawData[TextStart - 2] << 8) | (RawData[TextStart - 1]);

					// Convert the data to a string
					string PlistText = Encoding.UTF8.GetString(RawData, TextStart, TextLength);

					//@TODO: Distribution provisions seem to be a byte too long, and it may be a general problem with interpreting the length
					// For now, we just cut back to the end of the last tag.
					int CutPoint = PlistText.LastIndexOf('>');
					PlistText = PlistText.Substring(0, CutPoint + 1);

					// Return the constructed 'mobile provision'
					return new MobileProvision(PlistText);
				}
			}

			// Unable to find the start of the plist data
			Program.Error("Failed to find embedded plist in .mobileprovision file");
			return null;
		}

		public static MobileProvision ParseFile(Stream InputStream)
		{
			// Read in the entire file
			int NumBytes = (int)InputStream.Length;
			byte[] RawData = new byte[NumBytes];
			InputStream.Read(RawData, 0, NumBytes);

			return ParseFile(RawData);
		}


		public static MobileProvision ParseFile(string Filename)
		{
			FileStream InputStream = File.OpenRead(Filename);
			MobileProvision Result = ParseFile(InputStream);
			InputStream.Close();

			return Result;
		}

		/// <summary>
		/// Opens embedded.mobileprovision from within an IPA
		/// </summary>
		public static MobileProvision ParseIPA(string Filename)
		{
			FileOperations.ReadOnlyZipFileSystem FileSystem = new FileOperations.ReadOnlyZipFileSystem(Filename);
			MobileProvision Result = ParseFile(FileSystem.ReadAllBytes("embedded.mobileprovision"));
			FileSystem.Close();

			return Result;
		}
	}
}
/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Security.Cryptography.X509Certificates;
using System.Security.Cryptography;
using System.IO;
using MachObjectHandling;
using System.Diagnostics;
using System.Xml;

namespace iPhonePackager
{
	public class CodeSignatureBuilder
	{
		/// <summary>
		/// The file system used to read/write different parts of the application bundle
		/// </summary>
		public FileOperations.FileSystemAdapter FileSystem;

		/// <summary>
		/// The contents of Info.plist, mutable after PrepareForSigning is called, until PerformSigning is called, where it is flattened back to bytes
		/// </summary>
		public Utilities.PListHelper Info = null;

		/// <summary>
		/// The certificate used for code signing
		/// </summary>
		public X509Certificate2 SigningCert;

		/// <summary>
		/// The mobile provision used to sign the application
		/// </summary>
		protected MobileProvision Provision;

		/// <summary>
		/// Returns null if successful, or an error string if it failed
		/// </summary>
		public static string InstallCertificate(string CertificateFilename, string PrivateKeyFilename)
		{
			try
			{
				// Load the certificate
				string CertificatePassword = "";
				X509Certificate2 Cert = new X509Certificate2(CertificateFilename, CertificatePassword, X509KeyStorageFlags.MachineKeySet | X509KeyStorageFlags.PersistKeySet);
				if (!Cert.HasPrivateKey)
				{
					return "Error: Certificate does not include a private key and cannot be used to code sign";
				}

				// Add the certificate to the store
				X509Store Store = new X509Store();
				Store.Open(OpenFlags.ReadWrite);
				Store.Add(Cert);
				Store.Close();
			}
			catch (Exception ex)
			{
				string ErrorMsg = String.Format("Failed to load or install certificate with error '{0}'", ex.Message);
				Program.Error(ErrorMsg);
				return ErrorMsg;
			}

			return null;
		}

		/// <summary>
		/// Makes sure the required files for code signing exist and can be found
		/// </summary>
		public static void FindRequiredFiles(out MobileProvision Provision, out X509Certificate2 Cert, out bool bHasOverridesFile)
		{
			// Check for a mobile provision
			Provision = null;
			try
			{
				string ExpectedProvisionFile = FileOperations.FindPrefixedFile(Config.BuildDirectory, Program.GameName + ".mobileprovision");
				Provision = MobileProvisionParser.ParseFile(ExpectedProvisionFile);
			}
			catch (Exception)
			{
			}

			// Check for a suitable signature to match the mobile provision
			Cert = null;
			if (Provision != null)
			{
				Cert = CodeSignatureBuilder.FindCertificate(Provision);
			}

			bHasOverridesFile = File.Exists(Config.GetPlistOverrideFilename());
		}

		public static bool DoRequiredFilesExist()
		{
			MobileProvision Provision;
			X509Certificate2 Cert;
			bool bOverridesExists;
			CodeSignatureBuilder.FindRequiredFiles(out Provision, out Cert, out bOverridesExists);

			return bOverridesExists && (Provision != null) && (Cert != null);
		}

		protected virtual byte[] GetMobileProvision()
		{
			// Find the mobile provision file to use
			string MobileProvisionFilename = Path.Combine(Config.RepackageStagingDirectory, "embedded.mobileprovision");
				
			if (!File.Exists(MobileProvisionFilename))
			{
				MobileProvisionFilename = FileOperations.FindPrefixedFile(Config.BuildDirectory, Program.GameName + ".mobileprovision");
			}

			if (!File.Exists(MobileProvisionFilename))
			{
				MobileProvisionFilename = FileOperations.FindPrefixedFile(Config.EngineBuildDirectory, "UE4Game.mobileprovision");
				if (!File.Exists(MobileProvisionFilename))
				{
					MobileProvisionFilename = FileOperations.FindAnyFileWithExtension(Config.BuildDirectory, ".mobileprovision");
				}
			}

			byte[] Result = null;
			try
			{
				Result = File.ReadAllBytes(MobileProvisionFilename);
			}
			catch (System.Exception ex)
			{
				Program.Error("Could not find {0}.mobileprovision in {1} (error: '{2}').  Please follow the setup instructions to get a mobile provision from the Apple Developer site.",
					Program.GameName, Path.GetFullPath(Config.BuildDirectory), ex.Message);
			}

			return Result;
		}

		public void LoadMobileProvision()
		{
			byte[] MobileProvisionFile = GetMobileProvision();

			if (MobileProvisionFile != null)
			{
				Provision = MobileProvisionParser.ParseFile(MobileProvisionFile);

				Program.Log("Using mobile provision '{0}' to code sign", Provision.ProvisionName);

				// Re-embed the mobile provision
				FileSystem.WriteAllBytes("embedded.mobileprovision", MobileProvisionFile);
				Program.Log(" ... Writing updated embedded.mobileprovision");
			}
		}

		/// <summary>
		/// Tries to find a matching certificate on this machine from the the serial number of one of the
		/// certificates in the mobile provision (the one in the mobileprovision is missing the public/private key pair)
		/// </summary>
		public static X509Certificate2 FindCertificate(MobileProvision ProvisionToWorkFrom)
		{
			// Open the personal certificate store on this machine
			X509Store Store = new X509Store();
			Store.Open(OpenFlags.ReadOnly);

			// Try finding a matching certificate from the serial number (the one in the mobileprovision is missing the public/private key pair)
			X509Certificate2 Result = null;
			foreach (X509Certificate2 SourceCert in ProvisionToWorkFrom.DeveloperCertificates)
			{
				X509Certificate2Collection FoundCerts = Store.Certificates.Find(X509FindType.FindBySerialNumber, SourceCert.SerialNumber, false);

				if (FoundCerts.Count > 0)
				{
					Result = FoundCerts[0];
					break;
				}
			}

			Store.Close();

			return Result;
		}

		/// <summary>
		/// Installs a list of certificates to the local store
		/// </summary>
		/// <param name="CertFilenames"></param>
		public static void InstallCertificates(List<string> CertFilenames)
		{
			// Open the personal certificate store on this machine
			X509Store Store = new X509Store();
			Store.Open(OpenFlags.ReadWrite);

			// Install the trust chain certs
			string CertificatePassword = "";
			foreach (string AdditionalCertFilename in CertFilenames)
			{
				if (File.Exists(AdditionalCertFilename))
				{
					X509Certificate2 AdditionalCert = new X509Certificate2(AdditionalCertFilename, CertificatePassword, X509KeyStorageFlags.MachineKeySet);
					Store.Add(AdditionalCert);
				}
				else
				{
					string ErrorMessage = String.Format("... Failed to find an additional certificate '{0}' that is required for code signing", AdditionalCertFilename);
					Program.Error(ErrorMessage);
					throw new FileNotFoundException(ErrorMessage);
				}
			}

			// Close (save) the certificate store
			Store.Close();
		}

		/// <summary>
		/// Loads the signing certificate
		/// </summary>
		protected virtual X509Certificate2 LoadSigningCertificate()
		{
			return FindCertificate(Provision);
		}

		// Creates an omitted resource entry for the resource rules dictionary
		protected Dictionary<string, object> CreateOmittedResource(int Weight)
		{
			Dictionary<string, object> Result = new Dictionary<string, object>();
			Result.Add("omit", true);
			Result.Add("weight", (double)Weight);
			return Result;
		}

		protected virtual byte[] CreateCodeResourcesDirectory(string CFBundleExecutable)
		{
			// Verify that there is a rules plist
			string CFBundleResourceSpecification;
			if (!Info.GetString("CFBundleResourceSpecification", out CFBundleResourceSpecification))
			{
				throw new InvalidDataException("Info.plist must contain the key CFBundleResourceSpecification");
			}

			// Create a rules dict that includes (by wildcard) everything but Info.plist and the rules file
			Dictionary<string, object> Rules = new Dictionary<string, object>();
			Rules.Add(".*", true);
			Rules.Add("Info.plist", CreateOmittedResource(10));
			Rules.Add(CFBundleResourceSpecification, CreateOmittedResource(100));

			// Write the rules file out 
			{
				Utilities.PListHelper RulesPList = new Utilities.PListHelper();
				RulesPList.AddKeyValuePair("rules", Rules);
				string PListString = RulesPList.SaveToString();
				FileSystem.WriteAllBytes(CFBundleResourceSpecification, Encoding.UTF8.GetBytes(PListString));
			}

			// Create the full list of files to exclude (some files get excluded by 'magic' even though they aren't listed as special by rules)
			Dictionary<string, object> TrueExclusionList = new Dictionary<string, object>();
			TrueExclusionList.Add("Info.plist", null);
			TrueExclusionList.Add(CFBundleResourceSpecification, null);
			TrueExclusionList.Add(CFBundleExecutable, null);
			TrueExclusionList.Add("CodeResources", null);
			TrueExclusionList.Add("_CodeSignature/CodeResources", null);

			// Hash each file
			IEnumerable<string> FileList = FileSystem.GetAllPayloadFiles();
			SHA1CryptoServiceProvider HashProvider = new SHA1CryptoServiceProvider();

			Utilities.PListHelper HashedFileEntries = new Utilities.PListHelper();
			foreach (string Filename in FileList)
			{
				if (!TrueExclusionList.ContainsKey(Filename))
				{
					byte[] FileData = FileSystem.ReadAllBytes(Filename);
					byte[] HashData = HashProvider.ComputeHash(FileData);

					HashedFileEntries.AddKeyValuePair(Filename, HashData);
				}
			}

			// Create the CodeResources file that will contain the hashes
			Utilities.PListHelper CodeResources = new Utilities.PListHelper();
			CodeResources.AddKeyValuePair("files", HashedFileEntries);
			CodeResources.AddKeyValuePair("rules", Rules);

			// Write the CodeResources file out
			string CodeResourcesAsString = CodeResources.SaveToString();
			byte[] CodeResourcesAsBytes = Encoding.UTF8.GetBytes(CodeResourcesAsString);
			FileSystem.WriteAllBytes("_CodeSignature/CodeResources", CodeResourcesAsBytes);

			return CodeResourcesAsBytes;
		}

		protected virtual Utilities.PListHelper LoadInfoPList()
		{
			byte[] RawInfoPList = FileSystem.ReadAllBytes("Info.plist");
			return new Utilities.PListHelper(Encoding.UTF8.GetString(RawInfoPList));
		}

		/// <summary>
		/// Prepares this signer to sign an application
		///   Modifies the following files:
		///	 embedded.mobileprovision
		/// </summary>
		public void PrepareForSigning()
		{
			// Load Info.plist, which guides nearly everything else
			Info = LoadInfoPList();

			// Load the mobile provision, which provides entitlements and a partial cert which can be used to find an installed certificate
			LoadMobileProvision();
			if (Provision == null)
			{
				return;
			}

			// Install the Apple trust chain certs (required to do a CMS signature with full chain embedded)
			List<string> TrustChainCertFilenames = new List<string>();
			TrustChainCertFilenames.Add("AppleWorldwideDeveloperRelationsCA.pem");
			TrustChainCertFilenames.Add("AppleRootCA.pem");
			InstallCertificates(TrustChainCertFilenames);

			// Find and load the signing cert
			SigningCert = LoadSigningCertificate();
			if (SigningCert == null)
			{
				// Failed to find a cert already installed or to install, cannot proceed any futher
				Program.Error("... Failed to find a certificate that matches the mobile provision to be used for code signing");
			}
			else
			{
				Program.Log("... Found matching certificate '{0}'", SigningCert.FriendlyName);
			}
		}

		/// <summary>
		/// Creates an entitlements blob string from the entitlements structure in the mobile provision, merging in an on disk file if it is present.
		/// </summary>
		private string BuildEntitlementString(string CFBundleIdentifier)
		{
			// Load the base entitlements string from the mobile provision
			string ProvisionEntitlements = Provision.GetEntitlementsString(CFBundleIdentifier);

			// See if there is an override entitlements file on disk
			string UserOverridesEntitlementsFilename = FileOperations.FindPrefixedFile(Config.BuildDirectory, Program.GameName + ".entitlements");
			if (File.Exists(UserOverridesEntitlementsFilename))
			{
				// Merge in the entitlements from the on disk file as overrides
				Program.Log("Merging override entitlements from {0} into provision specified entitlements", Path.GetFileName(UserOverridesEntitlementsFilename));

				Utilities.PListHelper Merger = new Utilities.PListHelper(ProvisionEntitlements);
				string Overrides = File.ReadAllText(UserOverridesEntitlementsFilename, Encoding.UTF8);
				Merger.MergePlistIn(Overrides);

				return Merger.SaveToString();
			}
			else
			{
				// The ones from the mobile provision need no overrides
				return ProvisionEntitlements;
			}
		}


		/// <summary>
		/// Does the actual work of signing the application
		///   Modifies the following files:
		///	 Info.plist
		///	 [Executable] (file name derived from CFBundleExecutable in the Info.plist, e.g., UDKGame)
		///	 _CodeSignature/CodeResources
		///	 [ResourceRules] (file name derived from CFBundleResourceSpecification, e.g., CustomResourceRules.plist)
		/// </summary>
		public void PerformSigning()
		{
			DateTime SigningTime = DateTime.Now;

			// Get the name of the executable file
			string CFBundleExecutable;
			if (!Info.GetString("CFBundleExecutable", out CFBundleExecutable))
			{
				throw new InvalidDataException("Info.plist must contain the key CFBundleExecutable");
			}

			// Get the name of the bundle
			string CFBundleIdentifier;
			if (!Info.GetString("CFBundleIdentifier", out CFBundleIdentifier))
			{
				throw new InvalidDataException("Info.plist must contain the key CFBundleIdentifier");
			}

			// Verify there is a resource rules file and make a dummy one if needed.
			// If it's missing, CreateCodeResourceDirectory can't proceed (the Info.plist is already written to disk at that point)
			if (!Info.HasKey("CFBundleResourceSpecification"))
			{
				// Couldn't find the key, create a dummy one
				string CFBundleResourceSpecification = "CustomResourceRules.plist";
				Info.SetString("CFBundleResourceSpecification", CFBundleResourceSpecification);

				Program.Warning("Info.plist was missing the key CFBundleResourceSpecification, creating a new resource rules file '{0}'.", CFBundleResourceSpecification);
			}

			// Save the Info.plist out
			byte[] RawInfoPList = Encoding.UTF8.GetBytes(Info.SaveToString());
			Info.SetReadOnly(true);
			FileSystem.WriteAllBytes("Info.plist", RawInfoPList);
			Program.Log(" ... Writing updated Info.plist");

			// Create the code resources file and load it
			byte[] ResourceDirBytes = CreateCodeResourcesDirectory(CFBundleExecutable);

			// Open the executable
			Program.Log("Opening source executable...");
			byte[] SourceExeData = FileSystem.ReadAllBytes(CFBundleExecutable);

			FatBinaryFile FatBinary = new FatBinaryFile();
			FatBinary.LoadFromBytes(SourceExeData);

			foreach (MachObjectFile Exe in FatBinary.MachObjectFiles)
			{
				// Pad the memory stream with extra room to handle any possible growth in the code signing data
				int OverSize = 1024 * 1024;
				MemoryStream OutputExeStream = new MemoryStream(SourceExeData.Length + OverSize);
				
				// Copy the executable into the stream
				OutputExeStream.Seek(0, SeekOrigin.Begin);
				OutputExeStream.Write(SourceExeData, 0, SourceExeData.Length);
				OutputExeStream.Seek(0, SeekOrigin.Begin);

				//@TODO: Verify it's an executable (not an object file, etc...)

				// Find out if there was an existing code sign blob and find the linkedit segment command
				MachLoadCommandCodeSignature CodeSigningBlobLC = null;
				MachLoadCommandSegment LinkEditSegmentLC = null;
				foreach (MachLoadCommand Command in Exe.Commands)
				{
					if (CodeSigningBlobLC == null)
					{
						CodeSigningBlobLC = Command as MachLoadCommandCodeSignature;
					}

					if (LinkEditSegmentLC == null)
					{
						LinkEditSegmentLC = Command as MachLoadCommandSegment;
						if (LinkEditSegmentLC.SegmentName != "__LINKEDIT")
						{
							LinkEditSegmentLC = null;
						}
					}
				}

				if (LinkEditSegmentLC == null)
				{
					throw new InvalidDataException("Did not find a Mach segment load command for the __LINKEDIT segment");
				}

				// If the existing code signing blob command is missing, make sure there is enough space to add it
				// Insert the code signing blob if it isn't present
				//@TODO: Insert the code signing blob if it isn't present
				if (CodeSigningBlobLC == null)
				{
					throw new InvalidDataException("Did not find a Code Signing LC.  Injecting one into a fresh executable is not currently supported.");
				}

				// Verify that the code signing blob is at the end of the linkedit segment (and thus can be expanded if needed)
				if ((CodeSigningBlobLC.BlobFileOffset + CodeSigningBlobLC.BlobFileSize) != (LinkEditSegmentLC.FileOffset + LinkEditSegmentLC.FileSize))
				{
					throw new InvalidDataException("Code Signing LC was present but not at the end of the __LINKEDIT segment, unable to replace it");
				}

				int SignedFileLength = (int)CodeSigningBlobLC.BlobFileOffset;

				// Create the code directory blob
				CodeDirectoryBlob FinalCodeDirectoryBlob = CodeDirectoryBlob.Create(CFBundleIdentifier, SignedFileLength);

				// Create the entitlements blob
				string EntitlementsText = BuildEntitlementString(CFBundleIdentifier);
				EntitlementsBlob FinalEntitlementsBlob = EntitlementsBlob.Create(EntitlementsText);

				// Create or preserve the requirements blob
				RequirementsBlob FinalRequirementsBlob = null;
				if ((CodeSigningBlobLC != null) && Config.bMaintainExistingRequirementsWhenCodeSigning)
				{
					RequirementsBlob OldRequirements = CodeSigningBlobLC.Payload.GetBlobByMagic(AbstractBlob.CSMAGIC_REQUIREMENTS_TABLE) as RequirementsBlob;
					FinalRequirementsBlob = OldRequirements;
				}

				if (FinalRequirementsBlob == null)
				{
					FinalRequirementsBlob = RequirementsBlob.CreateEmpty();
				}

				// Create the code signature blob (which actually signs the code directory)
				CodeDirectorySignatureBlob CodeSignatureBlob = CodeDirectorySignatureBlob.Create();

				// Create the code signature superblob (which contains all of the other signature-related blobs)
				CodeSigningTableBlob CodeSignPayload = CodeSigningTableBlob.Create();
				CodeSignPayload.Add(0x00000, FinalCodeDirectoryBlob);
				CodeSignPayload.Add(0x00002, FinalRequirementsBlob);
				CodeSignPayload.Add(0x00005, FinalEntitlementsBlob);
				CodeSignPayload.Add(0x10000, CodeSignatureBlob);


				// The ordering of the following steps (and doing the signature twice below) must be preserved.
				// The reason is there are some chicken-and-egg issues here:
				//   The code directory stores a hash of the header, but
				//   The header stores the size of the __LINKEDIT section, which is where the signature blobs go, but
				//   The CMS signature blob signs the code directory
				//
				// So, we need to know the size of a signature blob in order to write a header that is itself hashed
				// and signed by the signature blob

				// Do an initial signature just to get the size
				Program.Log("... Initial signature step");
				CodeSignatureBlob.SignCodeDirectory(SigningCert, SigningTime, FinalCodeDirectoryBlob);

				// Compute the size of everything, and push it into the EXE header
				byte[] DummyPayload = CodeSignPayload.GetBlobBytes();

				// Adjust the header and load command to have the correct size for the code sign blob
				WritingContext OutputExeContext = new WritingContext(new BinaryWriter(OutputExeStream));

				long StartPosition = (CodeSigningBlobLC.BlobFileOffset - (long)LinkEditSegmentLC.FileOffset);
				long BlobLength = DummyPayload.Length;

				long NonCodeSigSize = (long)LinkEditSegmentLC.FileSize - CodeSigningBlobLC.BlobFileSize;
				long BlobStartPosition = NonCodeSigSize + (long)LinkEditSegmentLC.FileOffset;

				LinkEditSegmentLC.PatchFileLength(OutputExeContext, (uint)(NonCodeSigSize + BlobLength));
				CodeSigningBlobLC.PatchPositionAndSize(OutputExeContext, (uint)BlobStartPosition, (uint)BlobLength);

				// Now that the executable loader command has been inserted and the appropriate section modified, compute all the hashes
				Program.Log("... Computing hashes");
				OutputExeContext.Flush();

				// Fill out the special hashes
				FinalCodeDirectoryBlob.GenerateSpecialSlotHash(CodeDirectoryBlob.cdInfoSlot, RawInfoPList);
				FinalCodeDirectoryBlob.GenerateSpecialSlotHash(CodeDirectoryBlob.cdRequirementsSlot, FinalRequirementsBlob.GetBlobBytes());
				FinalCodeDirectoryBlob.GenerateSpecialSlotHash(CodeDirectoryBlob.cdResourceDirSlot, ResourceDirBytes);
				FinalCodeDirectoryBlob.GenerateSpecialSlotHash(CodeDirectoryBlob.cdApplicationSlot);
				FinalCodeDirectoryBlob.GenerateSpecialSlotHash(CodeDirectoryBlob.cdEntitlementSlot, FinalEntitlementsBlob.GetBlobBytes());

				// Fill out the regular hashes
				FinalCodeDirectoryBlob.ComputeImageHashes(OutputExeStream.ToArray());

				// And compute the final signature
				Program.Log("... Final signature step");
				CodeSignatureBlob.SignCodeDirectory(SigningCert, SigningTime, FinalCodeDirectoryBlob);

				// Generate the signing blob and place it in the output (verifying it didn't change in size)
				byte[] FinalPayload = CodeSignPayload.GetBlobBytes();

				if (DummyPayload.Length != FinalPayload.Length)
				{
					throw new InvalidDataException("CMS signature blob changed size between practice run and final run, unable to create useful code signing data");
				}

				OutputExeContext.PushPositionAndJump(BlobStartPosition);
				OutputExeContext.Write(FinalPayload);
				OutputExeContext.PopPosition();

				// Truncate the data so the __LINKEDIT section extends right to the end
				OutputExeContext.CompleteWritingAndClose();

				ulong DesiredExecutableLength = LinkEditSegmentLC.FileSize + LinkEditSegmentLC.FileOffset;
				byte[] FinalExeData = OutputExeStream.ToArray();
				if ((ulong)FinalExeData.Length < DesiredExecutableLength)
				{
					throw new InvalidDataException("Data written is smaller than expected, unable to finish signing process");
				}
				Array.Resize(ref FinalExeData, (int)DesiredExecutableLength); //@todo: Extend the file system interface so we don't have to copy 20 MB just to truncate a few hundred bytes

				// Save the patched and signed executable
				Program.Log("Saving signed executable...");
				FileSystem.WriteAllBytes(CFBundleExecutable, FinalExeData);
			}
		}
	}
}

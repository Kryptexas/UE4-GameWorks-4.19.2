using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace UnrealBuildTool
{
	public enum PluginDescriptorVersion
	{
		Invalid = 0,
		Initial = 1,
		NameHash = 2,
		ProjectPluginUnification = 3,
		// !!!!!!!!!! IMPORTANT: Remember to also update LatestPluginDescriptorFileVersion in Plugins.cs (and Plugin system documentation) when this changes!!!!!!!!!!!
		// -----<new versions can be added before this line>-------------------------------------------------
		// - this needs to be the last line (see note below)
		LatestPlusOne,
		Latest = LatestPlusOne - 1
	};

	public class PluginDescriptor
	{
		// Descriptor version number
		public int FileVersion;

		// Version number for the plugin.  The version number must increase with every version of the plugin, so that the system 
		// can determine whether one version of a plugin is newer than another, or to enforce other requirements.  This version
		// number is not displayed in front-facing UI.  Use the VersionName for that.
		public int Version;

		// Name of the version for this plugin.  This is the front-facing part of the version number.  It doesn't need to match
		// the version number numerically, but should be updated when the version number is increased accordingly.
		public string VersionName;
		 
		// Friendly name of the plugin
		public string FriendlyName;

		// Description of the plugin
		public string Description;

		// The name of the category this plugin
		public string Category;

		// The company or individual who created this plugin.  This is an optional field that may be displayed in the user interface.
		public string CreatedBy;

		// Hyperlink URL string for the company or individual who created this plugin.  This is optional.
		public string CreatedByURL;

		// Documentation URL string.
		public string DocsURL;

		// List of all modules associated with this plugin
		public ModuleDescriptor[] Modules;

		// Whether this plugin should be enabled by default for all projects
		public bool bEnabledByDefault;

		// Can this plugin contain content?
		public bool bCanContainContent;

		// Marks the plugin as beta in the UI
		public bool bIsBetaVersion;

		/// <summary>
		/// Private constructor. This object should not be created directly; read it from disk using FromFile() instead.
		/// </summary>
		private PluginDescriptor()
		{
			FileVersion = (int)PluginDescriptorVersion.Latest;
		}

		/// <summary>
		/// Creates a plugin descriptor from a file on disk
		/// </summary>
		/// <param name="FileName">The filename to read</param>
		/// <returns>New plugin descriptor</returns>
		public static PluginDescriptor FromFile(string FileName)
		{
			JsonObject RawObject = JsonObject.FromFile(FileName);

			PluginDescriptor Descriptor = new PluginDescriptor();

			// Read the version
			if(!RawObject.TryGetNumericField("FileVersion", out Descriptor.FileVersion))
			{
				if(!RawObject.TryGetNumericField("PluginFileVersion", out Descriptor.FileVersion))
				{
					throw new BuildException("Plugin descriptor file '{0}' does not contain a valid FileVersion entry", FileName);
				}
			}

			// Check it's not newer than the latest version we can parse
			if(Descriptor.FileVersion > (int)PluginDescriptorVersion.Latest)
			{
				throw new BuildException( "Plugin descriptor file '{0}' appears to be in a newer version ({1}) of the file format that we can load (max version: {2}).", FileName, Descriptor.FileVersion, (int)PluginDescriptorVersion.Latest);
			}

			// Read the other fields
			RawObject.TryGetNumericField("Version", out Descriptor.Version);
			RawObject.TryGetStringField("VersionName", out Descriptor.VersionName);
			RawObject.TryGetStringField("FriendlyName", out Descriptor.FriendlyName);
			RawObject.TryGetStringField("Description", out Descriptor.Description);

			if (!RawObject.TryGetStringField("Category", out Descriptor.Category))
			{
				// Category used to be called CategoryPath in .uplugin files
				RawObject.TryGetStringField("CategoryPath", out Descriptor.Category);
			}
        
			// Due to a difference in command line parsing between Windows and Mac, we shipped a few Mac samples containing
			// a category name with escaped quotes. Remove them here to make sure we can list them in the right category.
			if (Descriptor.Category != null && Descriptor.Category.Length >= 2 && Descriptor.Category.StartsWith("\"") && Descriptor.Category.EndsWith("\""))
			{
				Descriptor.Category = Descriptor.Category.Substring(1, Descriptor.Category.Length - 2);
			}

			RawObject.TryGetStringField("CreatedBy", out Descriptor.CreatedBy);
			RawObject.TryGetStringField("CreatedByURL", out Descriptor.CreatedByURL);
			RawObject.TryGetStringField("DocsURL", out Descriptor.DocsURL);

			JsonObject[] ModulesArray;
			if(RawObject.TryGetObjectArrayField("Modules", out ModulesArray))
			{
				Descriptor.Modules = Array.ConvertAll(ModulesArray, x => ModuleDescriptor.FromJsonObject(x));
			}

			RawObject.TryGetBoolField("EnabledByDefault", out Descriptor.bEnabledByDefault);
			RawObject.TryGetBoolField("CanContainContent", out Descriptor.bCanContainContent);
			RawObject.TryGetBoolField("IsBetaVersion", out Descriptor.bIsBetaVersion);

			return Descriptor;
		}
	};
}

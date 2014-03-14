// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.IO;
using System.Xml;

namespace AutomationTool
{

    public partial class CommandUtils
    {
        public class TempStorageFileInfo
        {
            public string Name;
            public DateTime Timestamp;
            public long Size;

            public TempStorageFileInfo()
            {
            }

            public TempStorageFileInfo(string Filename, string RelativeFile)
            {
                FileInfo Info = new FileInfo(Filename);
                Name = RelativeFile;
                Timestamp = Info.LastWriteTimeUtc;
                Size = Info.Length;
            }

            public bool Compare(TempStorageFileInfo Other)
            {
                bool bOk = true;
                if (!Name.Equals(Other.Name, StringComparison.InvariantCultureIgnoreCase))
                {
                    Log(System.Diagnostics.TraceEventType.Error, "File name mismatch {0} {1}", Name, Other.Name);
                    bOk = false;
                }
                else
                {
                    // this is a bit of a hack, but UAT itself creates these, so we need to allow them to be 
                    bool bOkToBeDifferent = Name.Contains(@"Engine\Binaries\DotNET\");

                    if (!((Timestamp - Other.Timestamp).TotalSeconds < 1 && (Timestamp - Other.Timestamp).TotalSeconds > -1))
                    {
                        CommandUtils.Log(bOkToBeDifferent ? System.Diagnostics.TraceEventType.Information : System.Diagnostics.TraceEventType.Error, "File date mismatch {0} {1} {2} {3}", Name, Timestamp.ToString(), Other.Name, Other.Timestamp.ToString());
                        bOk = bOkToBeDifferent;
                    }
                    if (!(Size == Other.Size))
                    {
                        CommandUtils.Log(bOkToBeDifferent ? System.Diagnostics.TraceEventType.Information : System.Diagnostics.TraceEventType.Error, "File size mismatch {0} {1} {2} {3}", Name, Size, Other.Name, Other.Size);
                        bOk = bOkToBeDifferent;
                    }
                }
                return bOk;
            }

            public override string ToString()
            {
                return String.IsNullOrEmpty(Name) ? "" : Name;
            }
        }
        public class TempStorageManifest
        {
            private static readonly string RootElementName = "tempstorage";
            private static readonly string DirectoryElementName = "directory";
            private static readonly string FileElementName = "file";
            private static readonly string NameAttributeName = "name";
            private static readonly string TimestampAttributeName = "timestamp";
            private static readonly string SizeAttributeName = "size";

            private Dictionary<string, List<TempStorageFileInfo>> Directories = new Dictionary<string, List<TempStorageFileInfo>>();

            public void Create(List<string> InFiles, string BaseFolder)
            {
                BaseFolder = CombinePaths(BaseFolder, "/");
                if (!BaseFolder.EndsWith("/")&& !BaseFolder.EndsWith("\\"))
                {
                    throw new AutomationException("base folder {0} should end with a separator", BaseFolder);
                }

                foreach (string InFilename in InFiles)
                {
                    var Filename = CombinePaths(InFilename);
                    if (!FileExists_NoExceptions(true, Filename))
                    {
                        throw new AutomationException("Could not add {0} to manifest because it does not exist", Filename);
                    }
                    if (!Filename.StartsWith(BaseFolder, StringComparison.InvariantCultureIgnoreCase))
                    {
                        throw new AutomationException("Could not add {0} to manifest because it does not start with the base folder {1}", Filename, BaseFolder);
                    }
                    var RelativeFile = Filename.Substring(BaseFolder.Length);
                    List<TempStorageFileInfo> ManifestDirectory;
                    string DirectoryName = CombinePaths(Path.GetDirectoryName(Filename), "/");
                    if (!DirectoryName.StartsWith(BaseFolder, StringComparison.InvariantCultureIgnoreCase))
                    {
                        throw new AutomationException("Could not add directory {0} to manifest because it does not start with the base folder {1}", DirectoryName, BaseFolder);
                    }
                    var RelativeDir = DirectoryName.Substring(BaseFolder.Length);
                    if (Directories.TryGetValue(RelativeDir, out ManifestDirectory) == false)
                    {
                        ManifestDirectory = new List<TempStorageFileInfo>();
                        Directories.Add(RelativeDir, ManifestDirectory);
                    }
                    ManifestDirectory.Add(new TempStorageFileInfo(Filename, RelativeFile));
                }

                Log("Created manifest:");
                Stats();

            }

            public void Stats()
            {

                Log("Directories={0}", Directories.Count);
                Log("Files={0}", GetFileCount());
                Log("Size={0}", GetTotalSize());
            }
            
            public bool Compare(TempStorageManifest Other)
            {
                if (Directories.Count != Other.Directories.Count)
                {
                    Log(System.Diagnostics.TraceEventType.Error, "Directory count mismatch {0} {1}", Directories.Count, Other.Directories.Count);
                    foreach (KeyValuePair<string, List<TempStorageFileInfo>> Directory in Directories)
                    {
                        List<TempStorageFileInfo> OtherDirectory;
                        if (Other.Directories.TryGetValue(Directory.Key, out OtherDirectory) == false)
                        {
                            Log(System.Diagnostics.TraceEventType.Error, "Missing Directory {0}", Directory.Key);
                            return false;
                        }
                    }
                    foreach (KeyValuePair<string, List<TempStorageFileInfo>> Directory in Other.Directories)
                    {
                        List<TempStorageFileInfo> OtherDirectory;
                        if (Directories.TryGetValue(Directory.Key, out OtherDirectory) == false)
                        {
                            Log(System.Diagnostics.TraceEventType.Error, "Missing Other Directory {0}", Directory.Key);
                            return false;
                        }
                    }
                    return false;
                }

                foreach (KeyValuePair<string, List<TempStorageFileInfo>> Directory in Directories)
                {
                    List<TempStorageFileInfo> OtherDirectory;
                    if (Other.Directories.TryGetValue(Directory.Key, out OtherDirectory) == false)
                    {
                        Log(System.Diagnostics.TraceEventType.Error, "Missing Directory {0}", Directory.Key); 
                        return false;
                    }
                    if (OtherDirectory.Count != Directory.Value.Count)
                    {
                        Log(System.Diagnostics.TraceEventType.Error, "File count mismatch {0} {1} {2}", Directory.Key, OtherDirectory.Count, Directory.Value.Count);
                        for (int FileIndex = 0; FileIndex < Directory.Value.Count; ++FileIndex)
                        {
                            Log("Manifest1: {0}", Directory.Value[FileIndex].Name);
                        }
                        for (int FileIndex = 0; FileIndex < OtherDirectory.Count; ++FileIndex)
                        {
                            Log("Manifest2: {0}", OtherDirectory[FileIndex].Name);
                        }
                        return false;
                    }
                    bool bResult = true;
                    for (int FileIndex = 0; FileIndex < Directory.Value.Count; ++FileIndex)
                    {
                        TempStorageFileInfo File = Directory.Value[FileIndex];
                        TempStorageFileInfo OtherFile = OtherDirectory[FileIndex];
                        if (File.Compare(OtherFile) == false)
                        {
                            bResult = false;
                        }
                    }
                    return bResult;
                }

                return true;
            }

            public TempStorageFileInfo FindFile(string LocalPath, string BaseFolder)
            {
                BaseFolder = CombinePaths(BaseFolder, "/");
                if (!BaseFolder.EndsWith("/")&& !BaseFolder.EndsWith("\\"))
                {
                    throw new AutomationException("base folder {0} should end with a separator", BaseFolder);
                }
                var Filename = CombinePaths(LocalPath);
                if (!Filename.StartsWith(BaseFolder, StringComparison.InvariantCultureIgnoreCase))
                {
                    throw new AutomationException("Could not add {0} to manifest because it does not start with the base folder {1}", Filename, BaseFolder);
                }
                var RelativeFile = Filename.Substring(BaseFolder.Length);
                string DirectoryName = CombinePaths(Path.GetDirectoryName(Filename), "/");
                if (!DirectoryName.StartsWith(BaseFolder, StringComparison.InvariantCultureIgnoreCase))
                {
                    throw new AutomationException("Could not add directory {0} to manifest because it does not start with the base folder {1}", DirectoryName, BaseFolder);
                }
                var RelativeDir = DirectoryName.Substring(BaseFolder.Length);               

                List<TempStorageFileInfo> Directory;
                if (Directories.TryGetValue(RelativeDir, out Directory))
                {
                    foreach (var SyncedFile in Directory)
                    {
                        if (SyncedFile.Name.Equals(RelativeFile, StringComparison.InvariantCultureIgnoreCase))
                        {
                            return SyncedFile;
                        }
                    }
                }
                return null;
            }

            public int GetFileCount()
            {
                int FileCount = 0;
                foreach (var Directory in Directories)
                {
                    FileCount += Directory.Value.Count;
                }
                return FileCount;
            }


            public void Load(string Filename)
            {
                XmlDocument File = new XmlDocument();
                try
                {
                    File.Load(Filename);
                }
                catch (Exception Ex)
                {
                    throw new AutomationException("Unable to load manifest file {0}: {1}", Filename, Ex.Message);
                }

                XmlNode RootNode = File.FirstChild;
                if (RootNode != null && RootNode.Name == RootElementName)
                {
                    for (XmlNode ChildNode = RootNode.FirstChild; ChildNode != null; ChildNode = ChildNode.NextSibling)
                    {
                        if (ChildNode.Name == DirectoryElementName)
                        {
                            LoadDirectory(ChildNode);
                        }
                        else
                        {
                            throw new AutomationException("Unexpected node {0} while reading manifest files.", ChildNode.Name);
                        }
                    }
                }
                else
                {
                    throw new AutomationException("Bad root node ({0}).", RootNode != null ? RootNode.Name : "null");
                }

                Log("Loaded manifest {0}:", Filename);

                Stats();
                if (GetFileCount() <= 0 || GetTotalSize() <= 0)
                {
                    throw new AutomationException("Attempt to load empty manifest.");
                }
            }

            bool TryGetAttribute(XmlNode Node, string AttributeName, out string Value)
            {
                bool Result = false;
                Value = "";
                if (Node.Attributes != null && Node.Attributes.Count > 0)
                {
                    for (int Index = 0; Result == false && Index < Node.Attributes.Count; ++Index)
                    {
                        if (Node.Attributes[Index].Name == AttributeName)
                        {
                            Value = Node.Attributes[Index].Value;
                            Result = true;
                        }
                    }
                }
                return Result;
            }

            void LoadDirectory(XmlNode DirectoryNode)
            {
                string DirectoryName;
                if (TryGetAttribute(DirectoryNode, NameAttributeName, out DirectoryName) == false)
                {
                    throw new AutomationException("Unable to get directory name attribute.");
                }

                List<TempStorageFileInfo> Files = new List<TempStorageFileInfo>();
                for (XmlNode ChildNode = DirectoryNode.FirstChild; ChildNode != null; ChildNode = ChildNode.NextSibling)
                {
                    if (ChildNode.Name == FileElementName)
                    {
                        TempStorageFileInfo Info = LoadFile(ChildNode);
                        if (Info != null)
                        {
                            Files.Add(Info);
                        }
                        else
                        {
                            throw new AutomationException("Error reading file entry in the manifest file ({0})", ChildNode.InnerXml);
                        }
                    }
                    else
                    {
                        throw new AutomationException("Unexpected node {0} while reading file nodes.", ChildNode.Name);
                    }
                }
                Directories.Add(DirectoryName, Files);
            }

            TempStorageFileInfo LoadFile(XmlNode FileNode)
            {
                TempStorageFileInfo Info = new TempStorageFileInfo();

                long Timestamp;
                string TimestampAttribute;
                if (TryGetAttribute(FileNode, TimestampAttributeName, out TimestampAttribute) == false)
                {
                   throw new AutomationException("Unable to get timestamp attribute.");
                }
                if (Int64.TryParse(TimestampAttribute, out Timestamp) == false)
                {
                    throw new AutomationException("Unable to parse timestamp attribute ({0}).", TimestampAttribute);
                }
                Info.Timestamp = new DateTime(Timestamp);

                string SizeAttribute;
                if (TryGetAttribute(FileNode, SizeAttributeName, out SizeAttribute) == false)
                {
                    throw new AutomationException("Unable to get size attribute.");
                }
                if (Int64.TryParse(SizeAttribute, out Info.Size) == false)
                {
                    throw new AutomationException("Unable to parse size attribute ({0}).", SizeAttribute);
                }

                Info.Name = FileNode.InnerText;

                return Info;
            }

            public void Save(string Filename)
            {
                if (GetFileCount() <= 0 || GetTotalSize() <= 0)
                {
                    throw new AutomationException("Attempt to save empty manifest.");
                }
                XmlDocument File = new XmlDocument();

                XmlElement RootElement = File.CreateElement(RootElementName);
                File.AppendChild(RootElement);

                SaveDirectories(File, RootElement);

                File.Save(Filename);
            }

            void SaveDirectories(XmlDocument File, XmlElement Root)
            {
                foreach (KeyValuePair<string, List<TempStorageFileInfo>> Directory in Directories)
                {
                    XmlElement DirectoryElement = File.CreateElement(DirectoryElementName);
                    Root.AppendChild(DirectoryElement);
                    DirectoryElement.SetAttribute(NameAttributeName, Directory.Key);
                    SaveFiles(File, DirectoryElement, Directory.Value);
                }
            }

            void SaveFiles(XmlDocument File, XmlElement Directory, List<TempStorageFileInfo> Files)
            {
                foreach (TempStorageFileInfo Info in Files)
                {
                    XmlElement FileElement = File.CreateElement(FileElementName);
                    FileElement.SetAttribute(TimestampAttributeName, Info.Timestamp.Ticks.ToString());
                    FileElement.SetAttribute(SizeAttributeName, Info.Size.ToString());
                    FileElement.InnerText = Info.Name;
                    Directory.AppendChild(FileElement);
                }
            }

            public Dictionary<string, List<TempStorageFileInfo>> DirectoryList
            {
                get { return Directories; }
            }

            public long GetTotalSize()
            {

                long TotalSize = 0;

                foreach (var Dir in Directories)
                {
                    foreach (var FileInfo in Dir.Value)
                    {
                        TotalSize += FileInfo.Size;
                    }
                }

                return TotalSize;
            }
            public List<string> GetFiles(string NewBaseDir)
            {
                var Result = new List<string>();
                foreach (var Dir in Directories)
                {
                    foreach (var ThisFileInfo in Dir.Value)
                    {
                        var NewFile = CombinePaths(NewBaseDir, ThisFileInfo.Name);
                        if (!FileExists_NoExceptions(false, NewFile))
                        {
                            throw new AutomationException("Rebased manifest file does not exist {0}", NewFile);
                        }
                        FileInfo Info = new FileInfo(NewFile);

                        Result.Add(Info.FullName);
                    }
                }
                if (Result.Count < 1)
                {
                    throw new AutomationException("No files in attempt to GetFiles().");
                }
                return Result;
            }
        }

        public static string LocalTempStorageManifestDirectory(CommandEnvironment Env)
        {
            return CombinePaths(Env.LocalRoot, "Engine", "Saved", "GUBP");
        }

        static HashSet<string> TestedForClean = new HashSet<string>();
        static void CleanSharedTempStorage(string Directory)
        {
            if (!IsBuildMachine || TestedForClean.Contains(Directory))
            {
                return;
            }
            TestedForClean.Add(Directory);
            const int MaximumDaysToKeepTempStorage = 4;
            DirectoryInfo DirInfo = new DirectoryInfo(Directory);
            var TopLevelDirs = DirInfo.GetDirectories();
            foreach (var TopLevelDir in TopLevelDirs)
            {
                if (DirectoryExists_NoExceptions(TopLevelDir.FullName))
                {
                    bool bOld = false;
                    foreach (var ThisFile in CommandUtils.FindFiles_NoExceptions(true, "*.TempManifest", false, TopLevelDir.FullName))
                    {
                        FileInfo Info = new FileInfo(ThisFile);

                        if ((DateTime.UtcNow - Info.LastWriteTimeUtc).TotalDays > MaximumDaysToKeepTempStorage)
                        {
                            bOld = true;
                        }
                    }
                    if (bOld)
                    {
                        Log("Deleteing temp storage directory {0}, because it is more than {1} days old.", TopLevelDir.FullName, MaximumDaysToKeepTempStorage);
                        DeleteDirectory_NoExceptions(true, TopLevelDir.FullName);
                    }
                }
            }
        }
        
        public static string RootSharedTempStorageDirectory()
        {
            return CombinePaths("P:", "Builds");
        }
        public static string SharedTempStorageDirectory()
        {
            return "GUBP";
        }
        public static string UE4TempStorageDirectory()
        {
            string SharedSubdir = SharedTempStorageDirectory();
            string Root = RootSharedTempStorageDirectory();
            return CombinePaths(Root, "UE4", SharedSubdir);
        }
        public static bool HaveSharedTempStorage()
        {
            if (!DirectoryExists_NoExceptions(UE4TempStorageDirectory()))
            {
                return false;
            }
            return true;
        }

        public static string ResolveSharedTempStorageDirectory(string GameFolder, bool bClean = true)
        {
            string SharedSubdir = SharedTempStorageDirectory();
            string Root = RootSharedTempStorageDirectory();
            string Result = CombinePaths(Root, GameFolder, SharedSubdir);
            if (!DirectoryExists_NoExceptions(Result))
            {
                string GameStr = "Game";
                bool HadGame = false;
                if (GameFolder.EndsWith(GameStr, StringComparison.InvariantCultureIgnoreCase))
                {
                    string ShortFolder = GameFolder.Substring(0, GameFolder.Length - GameStr.Length);
                    Result = CombinePaths(Root, ShortFolder, SharedSubdir);
                    HadGame = true;
                }
                if (!HadGame || !DirectoryExists_NoExceptions(Result))
                {
                    Result = CombinePaths(Root, "UE4", SharedSubdir);
                    if (!DirectoryExists_NoExceptions(Result))
                    {
                        throw new AutomationException("Could not find an appropriate shared temp folder {0}", Result);
                    }
                }
            }
            if (bClean)
            {
                CleanSharedTempStorage(Result);
            }
            return Result;
        }
        public static string SharedTempStorageDirectory(string StorageBlock, string GameFolder = "", bool bClean = true)
        {
            return CombinePaths(ResolveSharedTempStorageDirectory(GameFolder, bClean), StorageBlock);
        }

        public static TempStorageManifest SaveTempStorageManifest(string RootDir, string FinalFilename, List<string> Files)
        {
            var Saver = new TempStorageManifest();
            Saver.Create(Files, RootDir);
            if (Saver.GetFileCount() != Files.Count)
            {
                throw new AutomationException("Saver manifest differs has wrong number of files {0} != {1}", Saver.GetFileCount(), Files.Count);
            }
            var TempFilename = FinalFilename + ".temp";
            if (FileExists_NoExceptions(TempFilename))
            {
                throw new AutomationException("Temp manifest file already exists {0}", TempFilename);
            }
            CreateDirectory(Path.GetDirectoryName(FinalFilename));
            Saver.Save(TempFilename);

            var Tester = new TempStorageManifest();
            Tester.Load(TempFilename);

            if (!Saver.Compare(Tester))
            {
                throw new AutomationException("Temp manifest differs {0}", TempFilename);
            }

            RenameFile(TempFilename, FinalFilename);
            if (FileExists_NoExceptions(TempFilename))
            {
                throw new AutomationException("Temp manifest didn't go away {0}", TempFilename);
            }
            var FinalTester = new TempStorageManifest();
            FinalTester.Load(FinalFilename);

            if (!Saver.Compare(FinalTester))
            {
                throw new AutomationException("Final manifest differs {0}", TempFilename);
            }
            Log("Saved {0} with {1} files and total size {2}", FinalFilename, Saver.GetFileCount(), Saver.GetTotalSize());
            return Saver;
        }
        public static string LocalTempStorageManifestFilename(CommandEnvironment Env, string StorageBlockName)
        {
            return CombinePaths(LocalTempStorageManifestDirectory(Env), StorageBlockName + ".TempManifest");
        }
        public static TempStorageManifest SaveLocalTempStorageManifest(CommandEnvironment Env, string BaseFolder, string StorageBlockName, List<string> Files)
        {
            return SaveTempStorageManifest(BaseFolder, LocalTempStorageManifestFilename(Env, StorageBlockName), Files);
        }
        public static string SharedTempStorageManifestFilename(CommandEnvironment Env, string StorageBlockName, string GameFolder, bool bClean = true)
        {
            return CombinePaths(SharedTempStorageDirectory(StorageBlockName, GameFolder, bClean), StorageBlockName + ".TempManifest");
        }
        public static TempStorageManifest SaveSharedTempStorageManifest(CommandEnvironment Env, string StorageBlockName, string GameFolder, List<string> Files)
        {
            return SaveTempStorageManifest(SharedTempStorageDirectory(StorageBlockName, GameFolder), SharedTempStorageManifestFilename(Env, StorageBlockName, GameFolder), Files);
        }
        public static void DeleteLocalTempStorageManifests(CommandEnvironment Env)
        {
            DeleteDirectory(true, LocalTempStorageManifestDirectory(Env));
        }
        public static void DeleteSharedTempStorageManifests(CommandEnvironment Env, string StorageBlockName, string GameFolder = "")
        {
            DeleteDirectory(true, SharedTempStorageDirectory(StorageBlockName, GameFolder));
        }
        public static bool LocalTempStorageExists(CommandEnvironment Env, string StorageBlockName)
        {
            var LocalManifest = LocalTempStorageManifestFilename(Env, StorageBlockName);
            if (FileExists_NoExceptions(LocalManifest))
            {
                return true;
            }
            return false;
        }
        public static bool SharedTempStorageExists(CommandEnvironment Env, string StorageBlockName, string GameFolder = "")
        {
            var SharedManifest = SharedTempStorageManifestFilename(Env, StorageBlockName, GameFolder, false);
            if (FileExists_NoExceptions(SharedManifest))
            {
                return true;
            }
            return false;
        }
        public static bool TempStorageExists(CommandEnvironment Env, string StorageBlockName, string GameFolder = "", bool bLocalOnly = false)
        {
            return LocalTempStorageExists(Env, StorageBlockName) || (!bLocalOnly && SharedTempStorageExists(Env, StorageBlockName, GameFolder));
        }
        public static void StoreToTempStorage(CommandEnvironment Env, string StorageBlockName, List<string> Files, bool bLocalOnly = false, string GameFolder = "", string BaseFolder = "")
        {
            if (String.IsNullOrEmpty(BaseFolder))
            {
                BaseFolder = Env.LocalRoot;
            }

            BaseFolder = CombinePaths(BaseFolder, "/"); 
            if (!BaseFolder.EndsWith("/") && !BaseFolder.EndsWith("\\"))
            {
                throw new AutomationException("base folder {0} should end with a separator", BaseFolder);
            }

            var Local = SaveLocalTempStorageManifest(Env, BaseFolder, StorageBlockName, Files); 
            if (!bLocalOnly)
            {
                var BlockPath = SharedTempStorageDirectory(StorageBlockName, GameFolder);
                Log("Storing to {0}", BlockPath);
                if (DirectoryExists_NoExceptions(BlockPath))
                {
                    throw new AutomationException("Storage Block Already Exists! {0}", BlockPath);
                }
                CreateDirectory(BlockPath);
                if (!DirectoryExists_NoExceptions(BlockPath))
                {
                    throw new AutomationException("Storage Block Could Not Be Created! {0}", BlockPath);
                }
                var DestFiles = new List<string>();
                foreach (string InFilename in Files)
                {
                    var Filename = CombinePaths(InFilename);
                    if (!FileExists_NoExceptions(false, Filename))
                    {
                        throw new AutomationException("Could not add {0} to manifest because it does not exist", Filename);
                    }
                    if (!Filename.StartsWith(BaseFolder, StringComparison.InvariantCultureIgnoreCase))
                    {
                        throw new AutomationException("Could not add {0} to manifest because it does not start with the base folder {1}", Filename, BaseFolder);
                    }
                    var RelativeFile = Filename.Substring(BaseFolder.Length);
                    var DestFile = CombinePaths(BlockPath, RelativeFile);
                    if (FileExists_NoExceptions(true, DestFile))
                    {
                        throw new AutomationException("Dest file {0} already exists.", DestFile);
                    }
                    CopyFile(Filename, DestFile, true);
                    if (!FileExists_NoExceptions(true, DestFile))
                    {
                        throw new AutomationException("Could not copy {0} to {1}", Filename, DestFile);
                    }
                    DestFiles.Add(DestFile);
                }
                var Shared = SaveSharedTempStorageManifest(Env, StorageBlockName, GameFolder, DestFiles);
                if (!Local.Compare(Shared))
                {
                    // we will rename this so it can't be used, but leave it around for inspection
                    RenameFile_NoExceptions(SharedTempStorageManifestFilename(Env, StorageBlockName, GameFolder), SharedTempStorageManifestFilename(Env, StorageBlockName, GameFolder) + ".broken");
                    throw new AutomationException("Shared and Local manifest mismatch.");
                }

            }

        }

        public static List<string> RetrieveFromTempStorage(CommandEnvironment Env, string StorageBlockName, string GameFolder = "", string BaseFolder = "")
        {
            if (String.IsNullOrEmpty(BaseFolder))
            {
                BaseFolder = Env.LocalRoot;
            }

            BaseFolder = CombinePaths(BaseFolder, "/");
            if (!BaseFolder.EndsWith("/") && !BaseFolder.EndsWith("\\"))
            {
                throw new AutomationException("base folder {0} should end with a separator", BaseFolder);
            }

            var Files = new List<string>();
            var LocalManifest = LocalTempStorageManifestFilename(Env, StorageBlockName);
            if (FileExists_NoExceptions(LocalManifest))
            {
                Log("Found local manifest {0}", LocalManifest);
                var Local = new TempStorageManifest();
                Local.Load(LocalManifest);
                Files = Local.GetFiles(BaseFolder);
                var LocalTest = new TempStorageManifest();
                LocalTest.Create(Files, BaseFolder);
                if (!Local.Compare(LocalTest))
                {
                    throw new AutomationException("Local files in manifest {0} were tampered with.", LocalManifest);
                }
                return Files;
            }

            var BlockPath = CombinePaths(SharedTempStorageDirectory(StorageBlockName, GameFolder, false), "/");
            if (!BlockPath.EndsWith("/") && !BlockPath.EndsWith("\\"))
            {
                throw new AutomationException("base folder {0} should end with a separator", BlockPath);
            }
            Log("Attempting to retrieve from {0}", BlockPath);
            if (!DirectoryExists_NoExceptions(BlockPath))
            {
                throw new AutomationException("Storage Block Does Not Exists! {0}", BlockPath);
            }
            var SharedManifest = SharedTempStorageManifestFilename(Env, StorageBlockName, GameFolder);
            if (!FileExists_NoExceptions(SharedManifest))
            {
                throw new AutomationException("Storage Block Manifest Does Not Exists! {0}", SharedManifest);
            }
            var Shared = new TempStorageManifest();
            Shared.Load(SharedManifest);

            var SharedFiles = Shared.GetFiles(BlockPath);

            var DestFiles = new List<string>();
            foreach (string InFilename in SharedFiles)
            {
                var Filename = CombinePaths(InFilename);
                if (!FileExists_NoExceptions(true, Filename))
                {
                    throw new AutomationException("Could not add {0} to manifest because it does not exist", Filename);
                }
                if (!Filename.StartsWith(BlockPath, StringComparison.InvariantCultureIgnoreCase))
                {
                    throw new AutomationException("Could not add {0} to manifest because it does not start with the base folder {1}", Filename, BlockPath);
                }
                var RelativeFile = Filename.Substring(BlockPath.Length);
                var DestFile = CombinePaths(BaseFolder, RelativeFile);
                if (FileExists_NoExceptions(true, DestFile))
                {
                    Log("Dest file {0} already exists, deleting and overwriting", DestFile);
                    DeleteFile(DestFile);
                }
                CopyFile(Filename, DestFile, true);
                if (!FileExists_NoExceptions(true, DestFile))
                {
                    throw new AutomationException("Could not copy {0} to {1}", Filename, DestFile);
                }
                FileInfo Info = new FileInfo(DestFile);
                DestFiles.Add(Info.FullName);
            }
            var NewLocal = SaveLocalTempStorageManifest(Env, BaseFolder, StorageBlockName, DestFiles);
            if (!NewLocal.Compare(Shared))
            {
                // we will rename this so it can't be used, but leave it around for inspection
                RenameFile_NoExceptions(LocalManifest, LocalManifest + ".broken");
                throw new AutomationException("Shared and Local manifest mismatch.");
            }
            return DestFiles;
        }
        public static List<string> FindTempStorageManifests(CommandEnvironment Env, string StorageBlockName, bool LocalOnly = false, bool SharedOnly = false, string GameFolder = "")
        {
            var Files = new List<string>();

            var LocalFiles = LocalTempStorageManifestFilename(Env, StorageBlockName);
            Log("Looking for local directories that match {0}", LocalFiles);
            var LocalParent = Path.GetDirectoryName(LocalFiles);
            Log("  Looking in parent dir {0}", LocalParent);
            var WildCard = Path.GetFileName(LocalFiles);
            Log("  WildCard {0}", WildCard);

            int IndexOfStar = WildCard.IndexOf("*");
            if (IndexOfStar < 0 || WildCard.LastIndexOf("*") != IndexOfStar)
            {
                throw new AutomationException("Wildcard {0} either has no star or it has more than one.", WildCard);
            }

            string PreStarWildcard = WildCard.Substring(0, IndexOfStar);
            Log("  PreStarWildcard {0}", PreStarWildcard);
            string PostStarWildcard = Path.GetFileNameWithoutExtension(WildCard.Substring(IndexOfStar + 1));
            Log("  PostStarWildcard {0}", PostStarWildcard);

            if (!SharedOnly && DirectoryExists_NoExceptions(LocalParent))
            {
                foreach (var ThisFile in CommandUtils.FindFiles_NoExceptions(WildCard, true, LocalParent))
                {
                    Log("  Found local file {0}", ThisFile);
                    int IndexOfWildcard = ThisFile.IndexOf(PreStarWildcard);
                    if (IndexOfWildcard < 0)
                    {
                        throw new AutomationException("File {0} didn't contain {1}.", ThisFile, PreStarWildcard);
                    }
                    int LastIndexOfWildcardTail = ThisFile.LastIndexOf(PostStarWildcard);
                    if (LastIndexOfWildcardTail < 0 || LastIndexOfWildcardTail < IndexOfWildcard + PreStarWildcard.Length)
                    {
                        throw new AutomationException("File {0} didn't contain {1} or it was before the prefix", ThisFile, PostStarWildcard);
                    }
                    string StarReplacement = ThisFile.Substring(IndexOfWildcard + PreStarWildcard.Length, LastIndexOfWildcardTail - IndexOfWildcard - PreStarWildcard.Length);
                    if (StarReplacement.Length < 1)
                    {
                        throw new AutomationException("Dir {0} didn't have any string to fit the star in the wildcard {1}", ThisFile, WildCard);
                    }
                    Log("  Star Replacement {0}", StarReplacement);
                    if (!Files.Contains(StarReplacement))
                    {
                        Files.Add(StarReplacement);
                    }
                }
            }

            if (!LocalOnly)
            {
                var SharedFiles = SharedTempStorageManifestFilename(Env, StorageBlockName, GameFolder);
                Log("Looking for shared directories that match {0}", SharedFiles);
                var SharedParent = Path.GetDirectoryName(Path.GetDirectoryName(SharedFiles));

                if (DirectoryExists_NoExceptions(SharedParent))
                {
                    Log("  Looking in shared parent dir {0} with wildcard {1}", SharedParent, Path.GetFileNameWithoutExtension(SharedFiles));
                    string[] Dirs = null;

                    try
                    {
                        Dirs = Directory.GetDirectories(SharedParent, Path.GetFileNameWithoutExtension(SharedFiles), SearchOption.TopDirectoryOnly);
                    }
                    catch (Exception Ex)
                    {
                        Log("Unable to Find Directories in {0} with wildcard {1}", SharedParent, Path.GetFileNameWithoutExtension(SharedFiles));
                        Log(" Exception was {0}", LogUtils.FormatException(Ex));
                    }
                    if (Dirs != null)
                    {
                        foreach (var ThisSubDir in Dirs)
                        {
                            Log("  Found dir {0}", ThisSubDir);
                            int IndexOfWildcard = ThisSubDir.IndexOf(PreStarWildcard);
                            if (IndexOfWildcard < 0)
                            {
                                throw new AutomationException("Dir {0} didn't contain {1}.", ThisSubDir, PreStarWildcard);
                            }
                            int LastIndexOfWildcardTail = ThisSubDir.LastIndexOf(PostStarWildcard);
                            if (LastIndexOfWildcardTail < 0 || LastIndexOfWildcardTail < IndexOfWildcard + PreStarWildcard.Length)
                            {
                                throw new AutomationException("Dir {0} didn't contain {1} or it was before the prefix", ThisSubDir, PostStarWildcard);
                            }
                            string StarReplacement = ThisSubDir.Substring(IndexOfWildcard + PreStarWildcard.Length, LastIndexOfWildcardTail - IndexOfWildcard - PreStarWildcard.Length);
                            if (StarReplacement.Length < 1)
                            {
                                throw new AutomationException("Dir {0} didn't have any string to fit the star in the wildcard {1}", ThisSubDir, WildCard);
                            }
                            Log("  Star Replacement {0}", StarReplacement);
                            if (!Files.Contains(StarReplacement))
                            {
                                Files.Add(StarReplacement);
                            }
                        }
                    }
                }
            }

            var OutFiles = new List<string>();
            foreach (var StarReplacement in Files)
            {
                var NewBlock = StorageBlockName.Replace("*", StarReplacement);

                if (TempStorageExists(Env, NewBlock, GameFolder, LocalOnly))
                {
                    OutFiles.Add(StarReplacement);
                }
            }
            return OutFiles;
        }
    }
}

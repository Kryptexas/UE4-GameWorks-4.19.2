Welcome to the UE4 source code!
===============================

This is the source code page for the **Unreal Engine on GitHub**.  With the UE4 source code, you can modify the
engine and tools in any way imaginable and share your changes with others!

You can build the editor for Windows and Mac and compile games for both Android and iOS.  Source code for 
all tools is included as well, such as Unreal Lightmass and Unreal Frontend.

Before continuing, check out this [short tutorial video](http://youtu.be/usjlNHPn-jo) about getting started with the engine code.

Have fun!!  We can't wait to see what you create!!



Source releases
---------------

The latest version is [4.0.1](https://github.com/EpicGames/UnrealEngine/releases/tag/4.0.1-release).  Other 
releases can be found [right here](https://github.com/EpicGames/UnrealEngine/releases).

You can **download the source** in .zip format, but in order to *contribute to the community* you should
**setup a Git client** on your computer.  [This page](http://help.github.com/articles/set-up-git) will have 
the info you need.

Remember, you'll need to _download dependencies_ in order to actually build and run the engine.  You'll find those on 
the [releases page](https://github.com/EpicGames/UnrealEngine/releases) along with the source!

We're also publishing bleeding edge changes from our engine team!  This is the newest code we have, pushed into 
the [master branch](https://github.com/EpicGames/UnrealEngine/tree/master) on GitHub.  We'll continue to update
the 'master' code with fresh snapshots every so often, we can't guarantee the code is stable or even compiles.



Getting up and running
----------------------

Here is the fun part!  This is a quick start guide to getting up and running with the source.  The steps below will take you through cloning your own private fork, then compiling and 
running the editor yourself on Windows or Mac.  Oh, and you might want to watch our [short tutorial video](http://youtu.be/usjlNHPn-jo)
first.  Okay, here we go!

1. **Download the source** and unzip it to a folder, or [create a fork](https://github.com/EpicGames/UnrealEngine/tree/4.0.1-release)
   and **clone the repository**.  If you clone, don't forget to switch to the correct branch for this release!  (The 'master' branch 
   has unstable code, so you will want to make sure to choose a release branch.)

1. You should now have an _UnrealEngine_ folder on your computer.  All of the source and dependencies will go into this folder.  The folder name might 
   have a branch suffix (such as _UnrealEngine-4.0_), but that's totally fine.

1. Download the **required dependencies** files for the [latest release](https://github.com/EpicGames/UnrealEngine/releases/tag/4.0.1-release): 
   [Required_1of2.zip](https://github.com/EpicGames/UnrealEngine/releases/download/4.0.1-release/Required_1of2.zip), 
   [Required_2of2.zip](https://github.com/EpicGames/UnrealEngine/releases/download/4.0.1-release/Required_2of2.zip).

1. Unzip the dependencies into the _UnrealEngine_ folder alongside the source.  Be careful to make sure the folders are merged together 
   correctly.  On Mac, we recommend **Option + dragging** the unzipped files into the _UnrealEngine_ folder, then selecting **Keep Newer** if prompted.

1. Okay, platform stuff comes next.  Depending on whether you are on Windows or Mac, follow one of the sections below:


### Windows

1. Be sure to have [Visual Studio 2013](http://www.microsoft.com/en-us/download/details.aspx?id=40787) installed.  You can use any 
   desktop version of Visual Studio 2013, including the free version:  [Visual Studio 2013 Express for Windows Desktop](http://www.microsoft.com/en-us/download/details.aspx?id=40787)

1. Make sure you have [June 2010 DirectX runtime](http://www.microsoft.com/en-us/download/details.aspx?id=8109) installed.  You don't need the SDK, just the runtime.

1. You'll need project files in order to compile.  In the _UnrealEngine_ folder, double-click on **GenerateProjectFiles.bat**.  It should take less than a minute to complete.

1. Load the project into Visual Studio by double-clicking on the **UE4.sln** file.

1. It's time to **compile the editor**!  In Visual Studio, make sure your solution configuration is set to **Development Editor**, and your solution 
   platform is set to **Win64**.  Right click on the **UE4** target and select **Build**.  It will take between 15 and 40 minutes to finish compiling,
   depending on your system specs.

1. After compiling finishes, you can **load the editor** from Visual Studio by setting your startup project to **UE4** and pressing **F5** to debug.

1. One last thing.  You'll want to setup your Windows shell so that you can interact with .uproject files.  Find the file named **RegisterShellCommands.bat** in 
   the _UnrealEngine/Engine/Build/BatchFiles/_ folder.  Right click on the file and select **run as Administrator**.  Now, you'll be able to double-click .uproject files to load the project, or right click on
   .uprojects to quickly update Visual Studio files.


### Mac

1. Be sure to have [Xcode 5.1](https://itunes.apple.com/us/app/xcode/id497799835) installed.

1. You'll need project files in order to compile.  In the _UnrealEngine_ folder, double-click on **GenerateProjectFiles.command**.  It should take less than a minute to complete.  You can close the Terminal window afterwards.

1. Load the project into Xcode by double-clicking on the **UE4.xcodeproj** file.

1. It's time to **compile the editor**!  In Xcode, build the **UE4Editor - Mac** target for **My Mac** by selecting it in the target drop down
   in the top left, then using **Product** -> **Build For** -> **Running**.  It will take between 15 and 40 minutes, depending on your system specs.

1. After compiling finishes, you can **load the editor** from Xcode using the **Product** -> **Run** menu command!



More info
---------

Visual Studio 2013 and Xcode 5.1 are the supported and recommended compilers to use.

The first time you start the editor from a fresh source build, you may experience long load times.  This only happens on the first 
run as the engine optimizes content for the platform and _fills the derived data cache_.

You should probably always work with a versioned **release** branch.  The master branch contains unstable and possibly untested code,
but it should be a great reference for new developments, or for spot merging bug fixes.  Use it at your own risk.  

To build for platforms besides Windows and Mac, please see the Unreal Engine [documentation](http://docs.unrealengine.com).  Android 
development currently works best from a PC.  Conversely, iOS programming requires a Mac.  Some platforms may require
you to download _optional dependencies_ or install _platform development SDKs_ as well.

Visual Studio 2012 is also supported, but you'll need to make a code change and download the _optional dependencies_
on the [releases page](https://github.com/EpicGames/UnrealEngine/releases/tag/4.0.1-release).  See the Unreal Engine 
[documentation](http://docs.unrealengine.com) for more details about using older versions of Visual Studio.

Your private forks of the Unreal Engine code are associated with your GitHub account permissions.  Just remember
that if you unsubscribe or switch GitHub user names, you'll need to re-fork and upload your changes from a local copy. 



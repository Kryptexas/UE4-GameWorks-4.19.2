Welcome to the UE4 source code!
=================================

This is the source code page for the **Unreal Engine on GitHub**.  With the UE4 source code, you can modify the
engine and tools in any way imaginable and share your changes with others!

You can compile for either Windows or Mac, as well as build runtime games for Android and iOS.  Source for 
all tools is included as well, such as Unreal Lightmass and Unreal Frontend.

Have fun!!  We can't wait to see what you create!!



Source releases
---------------

The latest version is [4.0.1](https://github.com/EpicGames/UnrealEngine/releases/tag/4.0.1-release).  Other 
releases can be found [right here](https://github.com/EpicGames/UnrealEngine/releases).

You can **download the source** in .zip format, but in order to *contribute to the community* you should
**setup a Git client** on your computer.  [This page](http://help.github.com/articles/set-up-git) will have 
the info you need.

Remember, you'll need to download dependencies in order to actually build and run the engine.  You'll find those on 
the [releases page](https://github.com/EpicGames/UnrealEngine/releases) along with the source!

We're also experimenting with publishing bleeding edge changes from our development teams!  This is the newest stuff
we have, pushed into the [master branch](https://github.com/EpicGames/UnrealEngine/tree/master) on GitHub.  No guarantee 
is made that the code in 'master' is stable or compiles.



Getting up and running
----------------------

1. **Create a folder** on for the code and dependencies.

1. **Download the source** and unzip it into your folder, or [create a fork](https://github.com/EpicGames/UnrealEngine/tree/4.0.1-release)
   and clone it in.  If you clone, don't forget to switch to the correct branch for this release!  (The 'master' branch has unstable code, 
   so you will want to make sure to choose a release branch.)

1. Download the **required dependencies** files for the [latest release](https://github.com/EpicGames/UnrealEngine/releases/tag/4.0.1-release): 
   [First](https://github.com/EpicGames/UnrealEngine/releases/download/4.0.1-release/Required_1of2.zip), 
   [Second](https://github.com/EpicGames/UnrealEngine/releases/download/4.0.1-release/Required_2of2.zip).

1. Unzip the dependencies to the **same folder** that you saved the source.  Be careful to make sure the folders are merged together 
   correctly.  On Mac, we recommend **Alt + dragging** the unzipped files into your source folder, then selecting 'Keep Newer' if prompted.

1. Okay, platform-specific stuff comes next.  See the sections below:


### Windows

1. Be sure to have [Visual Studio 2013](http://www.microsoft.com/en-us/download/details.aspx?id=40787) installed.  You can use any 
   desktop version of Visual Studio 2013, including the free version:  [Visual Studio 2013 Express for Windows Desktop](http://www.microsoft.com/en-us/download/details.aspx?id=40787)

1. Make sure you have [June 2010 DirectX runtime](http://www.microsoft.com/en-us/download/details.aspx?id=8109) installed.  You don't need the SDK, just the runtime.

1. You'll need project files in order to compile.  Double-click **GenerateProjectFiles.bat**.  It will only take a few moments to complete.

1. Load the project into Visual Studio by double-clicking on the **UE4.sln** file.

1. It's time to **compile the editor**!  In Visual Studio, make sure your solution configuration is set to **Development Editor**, and your solution platform is set to **Win64**.  Right click on the **UE4** target and select **Build**.  It will take between 15 and 40 minutes to finish compiling, depending on your system specs.

1. After compiling finishes, you can **load the editor** from the IDE by setting your startup project to **UE4** and pressing **F5** to debug.

1. Setup your Windows shell so that you can right click on .uproject files.  Find the file named **RegisterShellCommands.bat** in the _UnrealEngine/Engine/Build/BatchFiles/_ folder, and run it!


### Mac

1. Be sure to have [Xcode 5.1](https://itunes.apple.com/us/app/xcode/id497799835) installed.

1. You'll need project files in order to compile.  Double-click **GenerateProjectFiles.command**.  It will only take a few moments to complete.

1. Load the project into Xcode by double-clicking on the **UE4.xcodeproj** file.

1. It's time to **compile the editor**!  In Xcode, build the **UE4Editor - Mac** target for **My Mac** by selecting it in the target drop down in the top left, then using **Product** -> **Build For** -> **Running**.  It will take between 15 and 40 minutes, depending on your system specs.

1. After compiling finishes, you can **load the editor** by from the IDE using the **Product** -> **Run** menu command!



Notes
-----

The first time you start the editor from a fresh source build, you may experience longer load times and out of date shaders.  This only happens on the first run as the engine optimizes content for the platform.

Visual Studio 2013 and Xcode 5.1 are the supported and recommended compilers to use.

Visual Studio 2012 is also supported, but you'll need to make a code change and download the **optional** dependencies 
on the [releases page](https://github.com/EpicGames/UnrealEngine/releases/tag/4.0.1-release).  See the Unreal Engine 
documentation for more details about using older versions of Visual Studio.

You should probably always work with a versioned **release** branch.  The master branch contains unstable and possibly untested code,
but it should be a great reference for new developments, or for spot merging bug fixes.  Use it fat your own risk.  
Full engine releases tend to lag behind latest development branch due to QA testing, but we wanted to make sure that 
you have access to the very latest stuff we're doing.

To build for platforms besides Windows and Mac, please see the Unreal Engine documentation.  Some platforms may require
you to download the **optional** dependencies as well.


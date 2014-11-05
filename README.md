Welcome to the UE 4.6 source code!
==================================

This is the source code page for the **Unreal Engine on GitHub**.  With the UE4 source code, you can modify the
engine and tools in any way imaginable and share your changes with others!

You can build the editor for Windows and Mac, and compile games for Android, iOS, Playstation 4, Xbox
One, HTML5 and Linux.  Source code for all tools is included as well, such as Unreal Lightmass and Unreal Frontend.

Before continuing, check out this [wiki page](https://wiki.unrealengine.com/GitHub_Setup) about getting started
with the engine code.  We also have a [programming guide](https://docs.unrealengine.com/latest/INT/Programming/index.html) and
full [API documentation](https://docs.unrealengine.com/latest/INT/API/index.html).

We also have a forum section where you can discuss [engine source and GitHub](https://forums.unrealengine.com/forumdisplay.php?1-Development-Discussion).
Have fun - we can't wait to see what you create!

Source releases
---------------

This branch contains source code for the **4.6 preview release**.

We're also publishing bleeding edge changes from our engine team in the [master branch](https://github.com/EpicGames/UnrealEngine/tree/master) on GitHub.  Here you can 
see [live commits](https://github.com/EpicGames/UnrealEngine/commits/master) from Epic programmers along with integrated code submissions from the community. This branch is 
unstable and may not even compile, though we periodically tag [preview releases](https://github.com/EpicGames/UnrealEngine/releases/tag/latest-preview) which
receive basic testing and have matching dependencies attached.

We recommend you work with a versioned release such as this one.  The master branch contains unstable and possibly untested code,
but it should be a great reference for new developments, or for spot merging bug fixes.  Use it at your own risk.  




Getting up and running
----------------------

Here is the fun part! The steps below will take you through cloning your own private fork, then compiling and running the editor yourself. You might want to
watch our [short tutorial video](http://youtu.be/usjlNHPn-jo) too.

### Windows

1. Install **[GitHub for Windows](https://windows.github.com/)** then **fork our repository** and **clone it to your computer**. 

   To use Git from the command line, see the [Setting up Git](https://help.github.com/articles/set-up-git/) and [Fork a Repo](https://help.github.com/articles/fork-a-repo/) articles. 
   Or if you'd rather not use Git at all, you can [download the source as a zip file](https://github.com/EpicGames/UnrealEngine/archive/4.6.zip).

1. Open your source folder in Explorer and run **GitSetup.bat**. This will download binary content for the engine, as well as installing any missing prerequisites and 
   registering file types.

1. Install **Visual Studio 2013**. Any desktop edition of Visual Studio 2013 will do, including the free version: [Visual Studio 2013 Express for Windows Desktop](http://www.microsoft.com/en-us/download/details.aspx?id=40787).

1. Run **GenerateProjectFiles.bat** to create project files for engine. It should take less than a minute to complete.  On Windows 8, a warning from SmartScreen may appear.  Click "More info", then "Run anyway" to continue.

1. Load the project into Visual Studio by double-clicking on the **UE4.sln** file.

1. It's time to **compile the editor**!  In Visual Studio, make sure your solution configuration is set to **Development Editor**, and your solution 
   platform is set to **Win64**.  Right click on the **UE4** target and select **Build**.  It will take between 15 and 40 minutes to finish compiling,
   depending on your system specs.

1. After compiling finishes, you can load the editor from Visual Studio by setting your startup project to **UE4** and pressing **F5** to debug.




### Mac

1. Install **[GitHub for Mac](https://mac.github.com/)** then **fork our repository** and **clone it to your computer**. 
   To use Git from the Terminal, see the [Setting up Git](https://help.github.com/articles/set-up-git/) and [Fork a Repo](https://help.github.com/articles/fork-a-repo/) articles. 
   Or if you'd rather not use Git at all, you can [download the source as a zip file](https://github.com/EpicGames/UnrealEngine/archive/4.6.zip).
   
1. Open your source folder in Finder and run **GitSetup.command** to download binary content for the engine.

1. Be sure to have [Xcode 5.1](https://itunes.apple.com/us/app/xcode/id497799835) installed.

1. You'll need project files in order to compile.  In the _UnrealEngine_ folder, double-click on **GenerateProjectFiles.command**.  It should take less than a minute to complete.  You can close the Terminal window afterwards.  If you downloaded the source in .zip format, you may see a warning about an unidentified developer.  This is because because the .zip files on GitHub are not digitally signed.  To work around this, right-click on GenerateProjectFiles.command, select Open, then click the Open button if you are sure you want to open it.

1. Load the project into Xcode by double-clicking on the **UE4.xcodeproj** file.

1. It's time to **compile the editor**!  In Xcode, build the **UE4Editor - Mac** target for **My Mac** by selecting it in the target drop down
   in the top left, then using **Product** -> **Build For** -> **Running**.  It will take between 15 and 40 minutes, depending on your system specs.

1. After compiling finishes, you can **load the editor** from Xcode using the **Product** -> **Run** menu command!




### Linux

1. [Set up Git](https://help.github.com/articles/set-up-git/) and [fork our repository](https://help.github.com/articles/fork-a-repo/).
   If you'd prefer not to use Git, you can [download the source as a zip file](https://github.com/EpicGames/UnrealEngine/archive/4.6.zip).

1. Open your source folder and run **GitSetup.sh** to download binary content for the engine.

1. Both cross-compiling and native builds are supported. 

1. Cross-compiling is handy when you are a Windows (Mac support planned too) developer who wants to package your game for Linux with minimal hassle, and it requires a [cross-compiler toolchain](http://cdn.unrealengine.com/qfe/v4_clang-3.5.0_ld-2.24_glibc-2.12.2.zip) to be installed (see the [Linux cross-compiling page on the wiki](https://wiki.unrealengine.com/Compiling_For_Linux)). Note that you will also need [optional dependencies](https://github.com/EpicGames/UnrealEngine/releases/download/latest-preview/Optional.zip) to be unzipped into your _UnrealEngine_ folder.

1. Native compilation is discussed in [a separate README](https://github.com/EpicGames/UnrealEngine/blob/4.5/Engine/Build/BatchFiles/Linux/README.md) and [community wiki page](https://wiki.unrealengine.com/Building_On_Linux). Downloading the dependencies has now been automated, so you will only need to clone the repo and run **GenerateProjectFiles.sh** (provided that you have OAUTH_TOKEN set to your personal access token, see the above README for details).




### Additional target platforms

**Android** support will be downloaded by the GitSetup script if you have the Android NDK installed. See the [Android getting started guide](https://docs.unrealengine.com/latest/INT/Platforms/Android/GettingStarted/).

**iOS** programming requires a Mac. Instructions are in the [iOS getting started guide](https://docs.unrealengine.com/latest/INT/Platforms/iOS/GettingStarted/index.html).

**HTML5** support files will be downloaded by the GitSetup script if you have Emscripten installed. Please see the [HTML5 getting started guide](https://docs.unrealengine.com/latest/INT/Platforms/HTML5/GettingStarted/index.html).

**Playstation 4** or **XboxOne** development require additional files that can only be provided after your registered developer status is confirmed by Sony or Microsoft. See [the announcement blog post](https://www.unrealengine.com/blog/playstation-4-and-xbox-one-now-supported) for more information.



Additional Notes
----------------

Visual Studio 2013 and Xcode 5.1 are strongly recommended for development.

The first time you start the editor from a fresh source build, you may experience long load times.  This only happens on the first 
run as the engine optimizes content for the platform and fills the _derived data cache_.

Your private forks of the Unreal Engine code are associated with your GitHub account permissions.  Just remember
that if you unsubscribe or switch GitHub user names, you'll need to re-fork and upload your changes from a local copy. 
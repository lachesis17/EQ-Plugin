# EQ-Plugin
 JUCE C++ Equalizer Plugin (VST3 and Standalone.exe)
Download / fork JUCE; put it in a pretty folder.

Set up CMake for VS2019 (MingW64 is too slow) and open up the JUCE folder in VSCode.

If using github version (without Projucer.exe), make projucer from the JUCE dir with:

```cmake . -B build -DJUCE_BUILD_EXAMPLES=ON -DJUCE_BUILD_EXTRAS=ON
cmake --build build --target Projucer```

Once Projucer is up, set the global paths for JUCE and JUCE/modules.

Then make a new project (e.g. Plug-in/Basic) and create it in a new dir, generating the default boilerplates:

```/Builds/
/JuceLibraryCode/
/Source/
```

Take across these handy templates from this repo (.vscode, CMakeLists.txt).

CMakeLists.txt needs to have a few things:

# Getting JUCE
```include(FetchContent)
set(FETCH_CONTENT_QUIET OFF)
FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git 
        GIT_TAG origin/master
        GIT_SHALLOW ON
        #[[FIND_PACKAGE_ARGS 7.0.3 GLOBAL]] <-- Uncomment this funny little character if you don't wanna redownload juce every project but you gotta do some shennanigans which the juce cmake api docs talk you thru 
)
FetchContent_MakeAvailable(JUCE)```

Nice snippet to #include <JUCE> using the github repo. This can also apparantly be done with a local clone using:
```CPMAddPackage("gh:juce-framework/JUCE") #local fork```

Change the obvious names and add in the boilerplate scripts from /Source/ with:

```target_sources(EQ_Plugin
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.h
        resources.rc
        )
```

# Building
Now JUCE, Projucer and the blank template are up, open up the new blank plugin dir in VSCode. Let CMake make the magic. Then to build, either use the VSCode left toolbar shortcut or these run commands:
```cmake --build . --target AudioPluginExample_VST3
cmake --build . --target AudioPluginExample_Standalone```
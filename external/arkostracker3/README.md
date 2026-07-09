# Welcome to Arkos Tracker 3!

This is the ultimate Tracker for Amstrad CPC, MSX, Spectrum, Atari ST/XE/XL, Vectrex, and Sharp MZ700! It works on Windows, MacOSX and Linux.

The code of the main software is 100% **C++17**. Players to be used on hardware are written in their respective assembler (Z80, 68000, etc.). 

Arkos Tracker relies on the **JUCE** [cross-platform library](https://github.com/juce-framework/JUCE).

## Credits

### Main software

- **Main code** by Julien Névo a.k.a. Targhan/Arkos

### Player credits

- **Z80 players** by Targhan/Arkos
    - except FAP player by Gozeur/Contrast and Hicks/Vanity.
- **68000 AKY** player by [ggn](https://github.com/ggnkua/Arkos-Tracker-2-ST)
- **Apple2/Oric 6502 AKY** player by Arnaud Cocquière
- **Atari XE/XL 6502 AKY** player by Krzysztof Dudek

### Third-party library credits

- **Z80 assembler** [Rasm](http://www.cpcwiki.eu/forum/programming/rasm-z80-assembler-in-beta/) by Roudoudou/Flower Corp, used to produce Z80 binary files
- **FAP player and cruncher** [FAP](https://github.com/grim1z/FastAyPlayer) by Hicks/Vanity and Gozeur/Contrast.
- **Z80 emulator** [SUZUKI PLAN](https://github.com/suzukiplan/z80) by Yoji Suzuki, used for some test units
- **Serial port communication** library - [serial](https://github.com/wjwwood/serial) - by William Woodall and John Harrison
- **LZH depack code** by Haruhiko Okumura (1991) and Kerwin F. Medina (1996). [C++ wrapper](http://leonard.oxg.free.fr/) by Arnaud Carré (Leonard), used to depack YM files
- **Speaker emulation** based on [RBJ Biquad HighPass](https://github.com/vinniefalco/DSPFilters) by Vincent Falco
- **CPM** ([Cmake Dependency Management](https://github.com/cpm-cmake/CPM.cmake)) by Lars Melchior

### Contributions

- **Mr. Earwig**: the docker scripts to build the project using docker.

### Special thanks

- Thanks to Zik for his algorithm about noise generation, and discussions over players and music software
- Thanks to Grim/Arkos for the AY volume measurements
- Thanks to Benjamin Gérard for the YM volume measurements
- Thanks to TotO for his PlayCity and great CPC hardware

## Compiling

Compiling is done via **CMake** (minimum version is 3.17).

Development is done and has been tested with the following IDEs:

* CLion (Linux)
* Visual Studio 2019 (Windows)
* XCode (MacOSX)

CLion is the main development software, it is strongly advised to use it, as it is the best IDE around!

### Dependencies on Linux

See the JUCE page for [its dependencies](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md).

As a summing up, the following dependencies must be installed in order to compile:

```
sudo apt-get -y install build-essential libfreetype6-dev libx11-dev libxinerama-dev libxrandr-dev libxcursor-dev mesa-common-dev libasound2-dev freeglut3-dev libxcomposite-dev
```

For CMake to be able to download JUCE:

```
sudo apt-get install pkg-config
```

### Compiling using an IDE

The process should be the same regardless of the IDE you use: simply open the CMakeLists.txt at the sourceProfileRoot of the project.
The project contains several targets:

* **MainSoftware**: this is Arkos Tracker itself.
* **ThirdParty**: the third parties, separate from the source as they use a more lax warning setting.
* **BinaryResourcesMain**: the binary resources for the main software.
* **TestUnits**: the test units. Make sure they always run and pass!
* **TestUnitResources**: the binary resources used in the test units.

JUCE is automatically downloaded by CMake.

### Compiling via command-line

These commands are especially useful for a CI, or if you want to compile the project without using an IDE.

#### Linux

```
cd <sourceProfileRoot of the project>
# Generate the project (only needed to be done once). "Debug" can be replaced with "Release".
cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" .
# Build the project. Replace "MainSoftware" with any other target listed above.
# Cleaning the target is optional.
cmake --build . --target clean
cmake --build . --target MainSoftware
```

#### Windows

```
cd <sourceProfileRoot of the project>
# Generate the project (only needed to be done once). You might want to change the VS version.
cmake -G "Visual Studio 16 2019" .
# Build the project. Replace "MainSoftware" with any other target listed above.
# Cleaning the target is optional. "Debug" can be replaced with "Release".  
cmake --build . --target clean
cmake --build . --target MainSoftware --config Debug 
```

#### MacOsX

```
cd <sourceProfileRoot of the project>
# Generate the project (only needed to be done once).
cmake -G Xcode .
# Build the project. Replace "MainSoftware" with any other target listed above.
# Cleaning the target is optional. "Debug" can be replaced with "Release".  
cmake --build . --target clean
cmake --build . --target MainSoftware --config Debug 
```



### Formatting

You can use import the `clionCodeStyle.xml` into CLion to get _most_ of the formatting rules _(File > Settings > Editor > Code style > C/C++)_. However, not all of them are explicit in this file.
Simply check how it is done in the existing source files and apply the rules to your code. Please avoid full-file auto-formatting, as it is too coarse.

### Static analysis

I am *very* meticulous on the warnings shown both by the compiler and the IDE (using several of them is instrumental in getting most of them).

* **No warning must be shown in the compiler**. The main project has the highest warning setting, and a warning is considered an error on Release.
* The IDE must show no warning in the classes, if possible. Unused methods are tolerated. Some warnings can be discarded on purpose, either via a comment to the Linter, or by adding a rule in the IDE settings. Some warnings are sadly present but are false positive.

You can import the `clionInspections.xml` file into CLion to get the same warning level I use _(File > Settings > Editor > Inspections)_.
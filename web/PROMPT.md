# Chip Machine Project

`chipmachine` is the C/C++ project that implements music player that can play music that was developed for the old home computers first, and then arcade machines, and then later using programs called trackers starting with Amiga home computers. It is plugin based and includes a number of plugins to cover many of today's music formats in use.

The compiled code binary can be used from the command line to play a particular music file or it can start the GUI with a music database collection of the 700K music files that can be downloaded and played.

The music player is designed to compile into a binary that executes in Apple Silicone in MacOS devices.

## The New Feature

The new feature would be to build the same music player with a new target for execution: a web browser.

## The main objective

The main objective is to figure the necessary steps to start from the existing code implementation and to end up with the code that can compile and execute in the web browser. Preferable if the existing code could be used to compile to two different executable binaries.

The first result should be a detailed plan on how to get to the end goal: same capable music player as the command line version that can execute as browser API. 

## Considerations

* Focus on a way to utilize modern browser technologies like
  * WASM and Web Assembly - a way to compile the C/C++ code into a binary that can be executed in the browser
  * WebAudio - a browser implementation of the standard API that allows you to play the audio in the browser
* Focus on the current code implementation feature that allows the one single executable, `cm` that has compiled and linked all the plugins that allows to play hundreds of different music file formats from the command line
  * check the feasibility of compiling and linking the same code into a WASM executable that can be invoked in the browser environment using JS
  * alternatively check the feasibility of including some of the existing solutions for playing music files in the browser:
    * chiptune2.js - the JS implementation based on OpenMPT plugin implementation that can play the following music formats: .mod, .it, .s3m, .xm
    * WebSID - JS implementation that can play the Commodore music developed for the Commodore 64 SID music chip
    * and similar available implementations
 
# References

* `https://github.com/deskjet/chiptune2.js/blob/master/chiptune2.js`
* `https://deskjet.github.io/chiptune2.js/`
* `https://github.com/wothke/websid`
* `https://bitbucket.org/wothke/websid/src/master/`


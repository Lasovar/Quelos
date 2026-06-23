# Quelos
A cross platform game engine focused on Editor functionality and developer workflow.
The main idea is to have a convenient/intuitive editor that doesn't compromise the runtime's performance, nor the developer's time.
the Engine is being developed to be as portable as possible and to ship on all platforms. The Editor is developed for Windows, Mac and Linux and tested almost weekly on them

<img width="2560" height="1439" alt="image" src="https://github.com/user-attachments/assets/b4ed0d43-4045-4bb8-b788-f6e8e5cba0e1" />

## Building
As of right now the developer environment is centered around CLion which would be my suggestion to try it out... but it's a CMake project so you just build with cmake to your target IDE

## Idea behind it
The editor is built to be as fast as possible and as snapping as possible... a lot of engines go with "This is just an editor it doesn't need to be fast" but for me the Editor is where I spend most of my time working so I don't want to stay looking at my screen for 5 minutes waiting for my code to compile or waiting for the assets to reimport.

And the developer isn't just a programmer, that involves the Level Designers and artists that will be working on the Editor... so tools to help developers and engineers communicate/work together with the least friction is also a priority. That would be through visual scripting that compiles down to native code to not sacrifice performance, and to be version control aware so scene changes are saved in indivisual files per actor to avoid merge conflicts, and version control file locking inside of the editor to avoid multiple people editing the same file.

Plus Most importantly a plugin system for the runtime and the editor to have the least features when you don't need them, while still having the ability to add more. This is most important for Renderer plugins since console renderer APIs are closed source and cannot be part of the engine's core

### Current goals:
- [ ] Proper asset management system/workflow:
  - [x] Automatic reference counted asset management
  - [ ] Add new assets through the plugin system
  - [x] Asset cooking workflow
  - [ ] Automatic asset reimporting/recooking when raw asset are updated
- [ ] Feature full renderer API
  - [x] fully abstract renderer backend
  - [ ] multiple render passes
  - [x] renderer api specific shader compilation
  - [ ] A builtint stylized renderer (cell shading, outlines, skybox)
- [ ] Integrate daslang as the engine's core scripting language
- [ ] Create a visual scripting graph that can be used to compile down to text files (script.das files and shader assets)
- [ ] Automatic plugin injection in the editor and runtime

### Future goals (first thing after current goals):
- [ ] Asset packer that takes the editor assets+cooked assets and packs them for the standalone player
- [ ] Create a player runtime project that runs the game as standalone
- [ ] Create an editor files generater (EntryPoint.cpp + CMakeLists.txt) to automatically compile all plugins as part of the standalone player statically
- [ ] Ship a game

This are my current personal requirements for a first release of the engine, which when I started thought I might never reach that point but rn I confident of my ability to at least release something with this featureset!

## License

This project is licensed under the MIT License — see the LICENSE file for details.

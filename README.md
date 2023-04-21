Repo intended for various, small Gothic 3 abomination mods of my own creation. All of them came into existence thanks to Georgeto's Gothic 3 SDK.

# Building
- Get Visual Studio Express 2013 for Windows Desktop (toolset v120)
- Gothic 3 SDK is added via git submodules so remember to update them if not cloned with `--recurse-submodules --remote-submodules` flags
```
git submodule init 
git submodule update
```

# How to Use
Move the .dll into `<Gothic 3 Install Dir>\scripts` and hope it doesn't conflict with any other mods/scripts you have installed. 

# Scripts
## Script_NoRespawn
- Deletes any NPCs after they respawned (or were moved to the dead NPC "graveyard") so you can enjoy destroying all living creatures of Myrtana without wondering about all respawned strugglers hiding in the distant map corners :)
- Alleviates the XP-FOMO of XP-maximizing maniacs. Game will display a notification if Hero has just lost any experience points due to NPC killing each other. Now you know immediately if you **must** reload the game.

Noote: I have briefly tested it with latest Community Patch v1.75.14, CP Update Pack v1.04.11 and Parallel Universe Patch v1.0.4.
### todos
- Allow configuring whether notification should be played/extract as separate script?
- ...

# Dependencies
- Gothic 3 SDK by Georgeto https://github.com/georgeto/gothic3sdk
- AsmJit https://github.com/asmjit/asmjit
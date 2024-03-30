# Scripts
## Script_NoRespawn
Disables respawning of monsters (or to be more precise - deletes monsters the exact moment they are resurrected by game's "Respawn" script).

## Script_DeathAlerts
Flashes a text notification if player is robbed of XP i.e. when wolf kills a scavenger.

# How to Use
Move the .dll into `<Gothic 3 Install Dir>\scripts` and hope it doesn't conflict with any other mods/scripts you have installed. 
### Note
I briefly tested this only with Community Patch (v1.75.14), CP Update Pack v1.04.11 and Parallel Universe Patch (v1.0.4). 
No other mods were installed. Yes, I'm one of a few who actually like their Gothic vanilla.

# Building
- Get "Visual Studio Express 2013 for Windows Desktop" (toolset v120)
- Gothic 3 SDK dependency is added via git submodules, remember to clone this repo with `--recurse-submodules --remote-submodules` flags or run the below:
```
git submodule init 
git submodule update
```
- If having a problem with VS Express fussing about "trial ended" go see: https://stackoverflow.com/a/30014257
# Make a new project as a copy of existing script
Using git bash on Windows
```
read -p "Provide a new project name: " PROJECTNAME
export PROJECTNAME
cp -r Script_DeathAlerts "Script_$PROJECTNAME" && \
grep -rl DeathAlerts "./Script_$PROJECTNAME/" | xargs sed -i "s/DeathAlerts/$PROJECTNAME/g" && \
grep -rl DEATHALERTS "./Script_$PROJECTNAME/" | xargs sed -i "s/DEATHALERTS/${PROJECTNAME^^}/g" && \
find "./Script_$PROJECTNAME/" -name "*DeathAlerts*" -exec bash -c 'mv "$1" "${1/DeathAlerts/$PROJECTNAME}"' _ {} \;
unset PROJECTNAME
```
# Dependencies
- Gothic 3 SDK by Georgeto https://github.com/georgeto/gothic3sdk
- AsmJit https://github.com/asmjit/asmjit
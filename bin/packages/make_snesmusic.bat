@echo off

echo == Building lostvikings_data.patch.snes.lvpa ==
..\lvpak.exe c lostvikings_data.patch.snes.lvpa -H0 -f lostvikings_data.patch.snes.listfile 
echo == Checking for correctness ==
..\lvpak.exe t lostvikings_data.patch.snes.lvpa

echo == Finished ==
pause > nul

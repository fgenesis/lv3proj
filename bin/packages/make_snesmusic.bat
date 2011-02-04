@echo off

echo == Building lostvikings_data.patch.snes.lvpa ==
..\lvpak.exe -c0 -f lostvikings_data.patch.snes.listfile a lostvikings_data.patch.snes.lvpa
echo == Checking for correctness ==
..\lvpak.exe t lostvikings_data.patch.snes.lvpa

echo == Finished ==
pause > nul

@echo off

echo == Building lostvikings_data.lvpa ==
..\lvpak.exe -c9 -f lostvikings_data.listfile a lostvikings_data.lvpa
echo == Checking for correctness ==
..\lvpak.exe t lostvikings_data.lvpa

echo == Building lostvikings_test.lvpa ==
..\lvpak.exe -c9 -f lostvikings_test.listfile a lostvikings_test.lvpa
echo == Checking for correctness ==
..\lvpak.exe t lostvikings_test.lvpa

echo == Finished ==
pause > nul

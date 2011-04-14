@echo off

echo == Building lostvikings_data.lvpa ==
..\lvpak.exe c lostvikings_data.lvpa -Hlzo9 -f lostvikings_data.listfile 
echo == Checking for correctness ==
..\lvpak.exe t lostvikings_data.lvpa

echo == Building lostvikings_test.lvpa ==
..\lvpak.exe c lostvikings_test.lvpa -Hlzo9 -f lostvikings_test.listfile 
echo == Checking for correctness ==
..\lvpak.exe t lostvikings_test.lvpa

echo == Finished ==
pause > nul

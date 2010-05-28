@ECHO OFF
echo.
echo Warning!
echo.
echo You are about to (re-)create the basepak.lvpa file from source files.
echo Be sure to have all files listed in each of the basepak.listfile on your disk.
echo.
echo If you want to bail out, close this window now.
echo Press any key to continue and create the file.
echo.
pause > nul

echo == Building basepak.lvpa ==
lvpak.exe -f basepak.listfile a basepak.lvpa
echo == Checking for correctness ==
lvpak.exe t basepak.lvpa
echo == Finished ==
pause > nul

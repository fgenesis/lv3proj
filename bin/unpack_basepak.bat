@ECHO OFF
echo.
echo Warning!
echo.
echo You are about to unpack the basepak.lvpa file to your disk.
echo There is nothing to worry about if this is the first time you are doing that.
echo.
echo If not:
echo ALREADY EXISTING FILES WILL BE OVERWRITTEN WITHOUT PROMPT!
echo If you have changed any of the default files previously extracted,
echo be sure to have them saved under a different name, and made a backup!
echo.
echo To bail out, close this window now.
echo Press any key to continue and extract all files.
pause > nul

echo == Extracting files ==
lvpak.exe x basepak.lvpa

echo == Finished ==
pause > nul

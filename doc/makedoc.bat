@echo off
echo --- Note that this script needs the falcon executables in PATH!
echo == Building Docs ==
falcon "faldoc/faldoc.fal" faldoc.fd
echo == Done. ==

pause

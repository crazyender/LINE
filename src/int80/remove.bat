@echo off
echo Unloading int80.sys...
echo.
instdrv\instdrv.exe LinuxSyscallRedirector remove 

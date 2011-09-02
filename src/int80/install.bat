@echo off
echo Loading int80.sys...
echo.
instdrv\instdrv.exe LinuxSyscallRedirector i386\int80.sys


# Requirements

To build the program you need the following:
- Make for Windows
- Mingw-w64 C++ Windows compiler

# Building

In the Project directory run `make` and let it compile

# Usage

This Program edits the Windows service TrustedInstaller configs to run custom provided commands
with the Trusted Installer group permission, this can be used to easily bypass the ownership
error when deleting System files/folders.

Also note that once the program returns, the original config of the TrustedInstaller service is restored.

# Syntax

execAsTI.exe only requires 1 argument, being the command to execute. 

The command argument needs to be strictly wrapped in double quotes, for example: `execAsTI.exe "powershell -c Set-Content C:\Windows\Temp\tmp1.txt (whoami /groups)"`.
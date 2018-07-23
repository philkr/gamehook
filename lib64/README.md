output folder for 64-bit libraries and plugins. All dlls and plugins will be copied here.
you can symlink them from the main game directory, which avoids additional copies.
Open cmd.exe and navigate to the game dir.
 $ mklink dxgi.dll \PATH\TO\GAMEHOOK\lib64\gamehook.dll
 $ mklink server.hk \PATH\TO\GAMEHOOK\lib64\server.hk
 $ mklink python.hk \PATH\TO\GAMEHOOK\lib64\python.hk
 ...

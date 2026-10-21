@echo off
echo Creating desktop shortcut for Ninja Gaiden Black (Direct3D 12)...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ws = New-Object -ComObject WScript.Shell; $desktop = [Environment]::GetFolderPath('Desktop'); $s = $ws.CreateShortcut([IO.Path]::Combine($desktop, 'Ninja Gaiden Black (DX12).lnk')); $s.TargetPath = [IO.Path]::Combine('%~dp0', 'NinjaGaidenBlack.exe'); $s.Arguments = '--dx12'; $s.WorkingDirectory = '%~dp0'; $s.IconLocation = [IO.Path]::Combine('%~dp0', 'NinjaGaidenBlack.exe') + ',0'; $s.Description = 'Ninja Gaiden Black (Direct3D 12)'; $s.Save(); Write-Host 'Direct3D 12 desktop shortcut created successfully on your Desktop!'"
pause

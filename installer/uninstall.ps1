# Removes the MtmdEdit bundle and its "Installed apps" entry. The Visual C++
# runtime is left in place: other programs (AutoCAD included) depend on it.
$bundleDir = Join-Path $env:APPDATA 'Autodesk\ApplicationPlugins\MtmdEdit.bundle'
Remove-Item 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\MtmdEdit' -Recurse -Force -ErrorAction SilentlyContinue
# This script lives inside the bundle; PowerShell has already read it, so the
# folder can be deleted while it runs.
Remove-Item $bundleDir -Recurse -Force -ErrorAction SilentlyContinue

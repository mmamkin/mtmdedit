# MtmdEdit setup for AutoCAD 2027.
# Installs the prerequisites the plugin needs at run time and the autoloader
# bundle for the current user. AutoCAD 2027 itself cannot be installed from
# here; it is only detected so the user gets a clear message.
# -Quiet: no dialogs (used by "MtmdEdit-Setup.exe /Q" for unattended deployment);
# a missing AutoCAD 2027 is then not treated as an error.
param([switch]$Quiet)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms

$here       = $PSScriptRoot
$appName    = 'MtmdEdit'
$appVersion = '4.0.0'
$bundleDir  = Join-Path $env:APPDATA 'Autodesk\ApplicationPlugins\MtmdEdit.bundle'
$uninstKey  = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\MtmdEdit'

function Show-Message([string]$text, [string]$icon = 'Information') {
    if ($Quiet) { Write-Host $text; return }
    [void][System.Windows.Forms.MessageBox]::Show($text, "$appName $appVersion setup", 'OK', $icon)
}

try {
    # 1. AutoCAD 2027 (release R26.0). The .arx loads in no other release.
    $acad = Get-ChildItem 'HKLM:\SOFTWARE\Autodesk\AutoCAD\R26.0' -ErrorAction SilentlyContinue
    if (-not $acad -and -not $Quiet) {
        $answer = [System.Windows.Forms.MessageBox]::Show(
            "AutoCAD 2027 was not found on this computer.`n`nMtmdEdit works only in AutoCAD 2027. Install it anyway (for example before installing AutoCAD)?",
            "$appName $appVersion setup", 'YesNo', 'Warning')
        if ($answer -ne 'Yes') { exit 1 }
    }

    # 2. Microsoft Visual C++ 2015-2026 runtime (x64) incl. MFC. The plugin is
    #    built with a newer toolset than the runtime AutoCAD installs, and the
    #    runtime must be at least as new as the toolset. Skip it when a new
    #    enough runtime is already present to avoid an unnecessary UAC prompt.
    $redist = Join-Path $here 'vc_redist.x64.exe'
    $required = [version](Get-Content (Join-Path $here 'vc_redist.version') -Raw).Trim()
    $rt = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64' -ErrorAction SilentlyContinue
    $installed = if ($rt -and $rt.Installed -eq 1) { [version]"$($rt.Major).$($rt.Minor).$($rt.Bld)" } else { [version]'0.0' }
    if ($installed -lt $required) {
        $p = Start-Process -FilePath $redist -ArgumentList '/install', $(if ($Quiet) { '/quiet' } else { '/passive' }), '/norestart' -Wait -PassThru
        # 1638 = a newer version is already installed, 3010 = reboot required
        if ($p.ExitCode -notin 0, 1638, 3010) {
            throw "Visual C++ runtime installation failed (exit code $($p.ExitCode))."
        }
    }

    # 3. Autoloader bundle. %APPDATA%\Autodesk\ApplicationPlugins is a trusted
    #    location, so AutoCAD loads it at start without a SECURELOAD prompt.
    if (Test-Path $bundleDir) { Remove-Item $bundleDir -Recurse -Force }
    $contents = Join-Path $bundleDir 'Contents'
    New-Item -ItemType Directory -Path $contents -Force | Out-Null
    Copy-Item (Join-Path $here 'PackageContents.xml') $bundleDir
    Copy-Item (Join-Path $here 'MtmdEdit2027.arx') $contents
    Copy-Item (Join-Path $here 'uninstall.ps1') $bundleDir

    # 4. Entry in "Installed apps" so it can be removed like any other program.
    $uninstall = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$bundleDir\uninstall.ps1`""
    New-Item -Path $uninstKey -Force | Out-Null
    Set-ItemProperty $uninstKey -Name DisplayName -Value "$appName for AutoCAD 2027"
    Set-ItemProperty $uninstKey -Name DisplayVersion -Value $appVersion
    Set-ItemProperty $uninstKey -Name Publisher -Value 'Mtmlab'
    Set-ItemProperty $uninstKey -Name InstallLocation -Value $bundleDir
    Set-ItemProperty $uninstKey -Name UninstallString -Value $uninstall
    Set-ItemProperty $uninstKey -Name NoModify -Value 1 -Type DWord
    Set-ItemProperty $uninstKey -Name NoRepair -Value 1 -Type DWord

    Show-Message "MtmdEdit was installed.`n`nRestart AutoCAD 2027 and use the TE command."
    exit 0
}
catch {
    Show-Message "Installation failed:`n`n$($_.Exception.Message)" 'Error'
    exit 1
}

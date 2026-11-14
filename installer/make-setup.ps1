# Packs the built .arx, the autoloader manifest, the setup scripts and the
# Visual C++ redistributable into a single self-extracting MtmdEdit-Setup.exe.
# The stub is compiled with the csc.exe that ships with Windows, so building
# the installer needs no extra tools.
param(
    [Parameter(Mandatory)] [string]$ArxPath,
    [Parameter(Mandatory)] [string]$OutDir
)
$ErrorActionPreference = 'Stop'

$ArxPath = (Resolve-Path $ArxPath).Path
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$OutDir = (Resolve-Path $OutDir).Path

# The redistributable matching the toolset the .arx was built with comes from
# the Visual Studio installation, so nothing has to be downloaded.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
if (-not $vsPath) { throw 'Visual Studio with the C++ MFC component was not found.' }
$redist = Get-ChildItem (Join-Path $vsPath 'VC\Redist\MSVC') -Recurse -Filter 'vc_redist.x64.exe' |
    Sort-Object { [version]$_.VersionInfo.ProductVersion } -Descending | Select-Object -First 1
if (-not $redist) { throw 'vc_redist.x64.exe not found in the Visual Studio installation.' }
$redistVersion = [version]$redist.VersionInfo.ProductVersion

$stage = Join-Path $env:TEMP "MtmdEditSetup-$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    # setup.ps1 rebuilds the bundle layout from these flat payload files.
    $files = @('setup.ps1', 'uninstall.ps1', 'PackageContents.xml') |
        ForEach-Object { Copy-Item (Join-Path $PSScriptRoot $_) $stage -PassThru }
    $files += Copy-Item $ArxPath $stage -PassThru
    $files += Copy-Item $redist.FullName $stage -PassThru
    $verFile = Join-Path $stage 'vc_redist.version'
    # Major.Minor.Build, the same parts the runtime registers in the registry
    Set-Content $verFile "$($redistVersion.Major).$($redistVersion.Minor).$($redistVersion.Build)" -Encoding ascii
    $files += Get-Item $verFile

    $target = Join-Path $OutDir 'MtmdEdit-Setup.exe'
    $csc = Join-Path $env:SystemRoot 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
    $cscArgs = @('/nologo', '/target:winexe', '/platform:anycpu', '/optimize+', "/out:$target")
    $cscArgs += $files | ForEach-Object { "/resource:$($_.FullName),payload/$($_.Name)" }
    $cscArgs += (Join-Path $PSScriptRoot 'SetupStub.cs')
    & $csc @cscArgs
    if ($LASTEXITCODE -ne 0) { throw "csc.exe failed (exit code $LASTEXITCODE)." }
    Write-Host "Built $target (VC++ runtime $redistVersion included)"
}
finally {
    Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
}

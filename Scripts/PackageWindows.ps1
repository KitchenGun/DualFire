[CmdletBinding()]
param(
    [switch]$PreflightOnly,
    [string]$EngineRoot = 'D:\UE_5.8'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-CheckedCommand {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList
    )

    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath"
    }
}

function Get-RequiredSingleFile {
    param(
        [string]$Root,
        [string]$Filter,
        [string]$Description
    )

    $matches = @(Get-ChildItem -LiteralPath $Root -Recurse -File -Filter $Filter)
    if ($matches.Count -lt 1) {
        throw "Required artifact is missing ($Description): $Filter"
    }
    return $matches[0]
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $projectRoot 'DualFire.uproject'
$gameConfigPath = Join-Path $projectRoot 'Config\DefaultGame.ini'
$uatPath = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$prereqSourcePath = Join-Path $EngineRoot 'Engine\Extras\Redist\en-us\vc_redist.x64.exe'
$releasesRoot = Join-Path $projectRoot 'Saved\Releases'

if (-not (Test-Path -LiteralPath $projectPath)) {
    throw "Project file not found: $projectPath"
}
$versionMatch = Select-String -LiteralPath $gameConfigPath -Pattern '^ProjectVersion=(\d+\.\d+\.\d+)$' | Select-Object -First 1
if ($null -eq $versionMatch) {
    throw 'Config/DefaultGame.ini must contain ProjectVersion=<major.minor.patch>.'
}
$version = $versionMatch.Matches[0].Groups[1].Value
$versionTag = "v$version"

$dirtyFiles = @(git -C $projectRoot status --porcelain)
if ($dirtyFiles.Count -ne 0) {
    throw 'Packaging requires a clean working tree. Commit or stash changes before running this script.'
}

$branch = (git -C $projectRoot branch --show-current).Trim()
if ($branch -ne 'main') {
    throw "Packaging must run from main; current branch is '$branch'."
}

Invoke-CheckedCommand -FilePath 'git' -ArgumentList @('-C', $projectRoot, 'fetch', 'origin', 'main', '--tags')
$head = (git -C $projectRoot rev-parse HEAD).Trim()
$originMain = (git -C $projectRoot rev-parse origin/main).Trim()
if ($head -ne $originMain) {
    throw 'Local main is not synchronized with origin/main.'
}

$tagRef = "refs/tags/$versionTag"
$tagObjectOutput = @(git -C $projectRoot cat-file -t $tagRef 2>$null)
if ($LASTEXITCODE -ne 0) {
    throw "Required release tag is missing: $versionTag"
}
$tagObjectType = $tagObjectOutput[0].Trim()
if ($tagObjectType -ne 'tag') {
    throw "Release tag $versionTag must be annotated."
}

$localTagCommitOutput = @(git -C $projectRoot rev-parse --verify --quiet "$versionTag^{commit}")
if ($LASTEXITCODE -ne 0) {
    throw "Required release tag commit cannot be resolved: $versionTag"
}
$localTagCommit = $localTagCommitOutput[0].Trim()

$remoteTagLines = @(git -C $projectRoot ls-remote --tags origin $tagRef "$tagRef^{}")
if ($LASTEXITCODE -ne 0) {
    throw "Unable to verify remote release tag: $versionTag"
}
$remotePeeledRef = "$tagRef^{}"
$remotePeeledLines = @($remoteTagLines | Where-Object {
    $parts = $_ -split '\s+'
    $parts.Count -eq 2 -and $parts[1] -eq $remotePeeledRef
})
if ($remotePeeledLines.Count -ne 1) {
    throw "Remote release tag $versionTag must resolve to exactly one peeled commit."
}
$remoteTagCommit = (($remotePeeledLines[0] -split '\s+')[0]).Trim()
if ($remoteTagCommit -notmatch '^[0-9a-fA-F]{40}$' -or $remoteTagCommit -ne $localTagCommit) {
    throw "Remote release tag $versionTag does not match the local annotated tag commit."
}

& git -C $projectRoot merge-base --is-ancestor $versionTag HEAD
if ($LASTEXITCODE -ne 0) {
    throw "Release tag $versionTag is not an ancestor of HEAD."
}

$shortSha = (git -C $projectRoot rev-parse --short=8 HEAD).Trim()
$utcStamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$buildId = "$version-dev.$utcStamp+$shortSha"
Write-Host "Preflight passed: version=$version tag=$versionTag buildId=$buildId"

if ($PreflightOnly) {
    exit 0
}

if (-not (Test-Path -LiteralPath $uatPath)) {
    throw "UE 5.8 RunUAT was not found: $uatPath"
}
if (-not (Test-Path -LiteralPath $prereqSourcePath -PathType Leaf)) {
    throw "UE 5.8 prerequisite installer was not found: $prereqSourcePath"
}

$versionReleaseRoot = Join-Path $releasesRoot "v$version"
$stagingRoot = Join-Path $releasesRoot (Join-Path '.staging' $buildId)
$zipPath = Join-Path $versionReleaseRoot "DualFire-v$buildId-Win64.zip"
$tarPath = Join-Path $env:SystemRoot 'System32\tar.exe'

if (-not (Test-Path -LiteralPath $tarPath)) {
    throw "Windows tar.exe was not found: $tarPath"
}

New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null
New-Item -ItemType Directory -Path $versionReleaseRoot -Force | Out-Null

try {
    Invoke-CheckedCommand -FilePath $uatPath -ArgumentList @(
        'BuildCookRun',
        "-project=$projectPath",
        '-noP4',
        '-platform=Win64',
        '-clientconfig=Development',
        '-build',
        '-clean',
        '-cook',
        '-AdditionalCookerOptions=-noxgeshadercompile',
        '-map=/Game/Level/LV_Start+/Game/Level/LV_Test+/Game/Level/LV_Result',
        '-stage',
        '-pak',
        '-iostore',
        '-prereqs',
        '-archive',
        "-archivedirectory=$stagingRoot",
        '-utf8output'
    )

    $gameExecutable = Get-RequiredSingleFile -Root $stagingRoot -Filter 'DualFire.exe' -Description 'Win64 executable'
    $packageRoot = $gameExecutable.Directory.FullName
    $null = Get-RequiredSingleFile -Root $packageRoot -Filter '*.pak' -Description 'pak file'
    $null = Get-RequiredSingleFile -Root $packageRoot -Filter '*.utoc' -Description 'IoStore container index'
    $null = Get-RequiredSingleFile -Root $packageRoot -Filter '*.ucas' -Description 'IoStore container data'
    $prereqDestinationPath = Join-Path $packageRoot 'vc_redist.x64.exe'
    Copy-Item -LiteralPath $prereqSourcePath -Destination $prereqDestinationPath -Force
    if (-not (Test-Path -LiteralPath $prereqDestinationPath -PathType Leaf) -or (Get-Item -LiteralPath $prereqDestinationPath).Length -le 0) {
        throw "Packaged prerequisite installer is missing: $prereqDestinationPath"
    }

    $cookedRoot = Join-Path $projectRoot 'Saved\Cooked'
    foreach ($mapName in @('LV_Start', 'LV_Test', 'LV_Result')) {
        $cookedMap = @(Get-ChildItem -LiteralPath $cookedRoot -Recurse -File -Filter "$mapName.umap" | Where-Object {
            $_.FullName -match '[\\/]DualFire[\\/]Content[\\/]Level[\\/]'
        })
        if ($cookedMap.Count -lt 1) {
            throw "Cook output is missing required map: $mapName"
        }
    }

    $buildInfoPath = Join-Path $packageRoot 'BUILD_INFO.txt'
    @(
        "ProductVersion=$version",
        'Channel=dev',
        "BuildId=$buildId",
        "Commit=$head",
        "ReleaseTag=$versionTag",
        'UnrealEngine=5.8',
        'Configuration=Development',
        "BuiltUtc=$utcStamp"
    ) | Set-Content -LiteralPath $buildInfoPath -Encoding ASCII

    if (Test-Path -LiteralPath $zipPath) {
        Remove-Item -LiteralPath $zipPath -Force
    }
    Push-Location -LiteralPath $packageRoot
    try {
        Invoke-CheckedCommand -FilePath $tarPath -ArgumentList @('-a', '-c', '-f', $zipPath, '.')
    }
    finally {
        Pop-Location
    }
    if (-not (Test-Path -LiteralPath $zipPath) -or (Get-Item -LiteralPath $zipPath).Length -le 0) {
        throw "ZIP creation failed: $zipPath"
    }

    Write-Host "Package created: $zipPath"
}
catch {
    if (Test-Path -LiteralPath $zipPath) {
        Remove-Item -LiteralPath $zipPath -Force
    }
    throw
}

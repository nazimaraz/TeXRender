param(
    [Parameter(Mandatory = $true)]
    [string] $InstallPrefix,

    [string] $Dumpbin = ""
)

$ErrorActionPreference = "Stop"

function Find-Dumpbin {
    param([string] $ExplicitPath)

    if ($ExplicitPath -ne "") {
        if (-not (Test-Path -LiteralPath $ExplicitPath)) {
            throw "dumpbin path does not exist: $ExplicitPath"
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    $command = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "dumpbin.exe was not found on PATH and vswhere.exe is unavailable"
    }

    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or $installPath -eq "") {
        throw "Visual Studio with MSVC tools was not found"
    }

    $toolsRoot = Join-Path $installPath "VC\Tools\MSVC"
    $candidate = Get-ChildItem -LiteralPath $toolsRoot -Directory |
        Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName "bin\Hostx64\x64\dumpbin.exe" } |
        Where-Object { Test-Path -LiteralPath $_ } |
        Select-Object -First 1

    if ($null -eq $candidate) {
        throw "dumpbin.exe was not found under $toolsRoot"
    }

    return $candidate
}

function Require-Pattern {
    param(
        [string[]] $Lines,
        [string] $Pattern,
        [string] $Description
    )

    if (-not ($Lines | Select-String -SimpleMatch $Pattern -Quiet)) {
        Write-Error "expected exported symbol for $Description matching '$Pattern'"
    }
}

function Reject-Pattern {
    param(
        [string[]] $Lines,
        [string] $Pattern,
        [string] $Description
    )

    $matches = $Lines | Select-String -SimpleMatch $Pattern
    if ($matches) {
        Write-Error "unexpected private/internal export for $Description matching '$Pattern':`n$($matches -join "`n")"
    }
}

$resolvedPrefix = Resolve-Path -LiteralPath $InstallPrefix
$dll = Get-ChildItem -LiteralPath $resolvedPrefix.Path -Recurse -Filter "texrender.dll" |
    Select-Object -First 1
if ($null -eq $dll) {
    throw "expected texrender.dll under $($resolvedPrefix.Path)"
}

$dumpbinPath = Find-Dumpbin -ExplicitPath $Dumpbin
$exports = & $dumpbinPath /nologo /exports $dll.FullName
if ($LASTEXITCODE -ne 0) {
    throw "dumpbin /exports failed for $($dll.FullName)"
}

Require-Pattern -Lines $exports -Pattern "?render@LatexDocument@TeXRender@@" -Description "LatexDocument::render"
Require-Pattern -Lines $exports -Pattern "?draw@RenderHandle@TeXRender@@" -Description "RenderHandle::draw"
Require-Pattern -Lines $exports -Pattern "?draw_glyph@HeadlessSurface@TeXRender@@" -Description "HeadlessSurface::draw_glyph"
Require-Pattern -Lines $exports -Pattern "?svg@SvgSurface@TeXRender@@" -Description "SvgSurface::svg"
Require-Pattern -Lines $exports -Pattern "?cache_limits@TeXRender@@" -Description "cache_limits"
Require-Pattern -Lines $exports -Pattern "?shutdown@TeXRender@@" -Description "shutdown"

Reject-Pattern -Lines $exports -Pattern "@detail@TeXRender@@" -Description "TeXRender::detail"
Reject-Pattern -Lines $exports -Pattern "@tex@@" -Description "upstream tex namespace"
Reject-Pattern -Lines $exports -Pattern "?blend@HeadlessSurface@TeXRender@@" -Description "HeadlessSurface::blend"
Reject-Pattern -Lines $exports -Pattern "?paint_attributes@SvgSurface@TeXRender@@" -Description "SvgSurface::paint_attributes"
Reject-Pattern -Lines $exports -Pattern "microtex" -Description "MicroTeX internals"

$symbolCount = ($exports | Select-String "TeXRender").Count
Write-Host ""
Write-Host "MSVC exported-symbol audit passed"
Write-Host "library: $($dll.FullName)"
Write-Host "TeXRender export lines: $symbolCount"

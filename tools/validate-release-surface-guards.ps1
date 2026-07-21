[CmdletBinding()]
param(
    [string]$RepositoryRoot = '',
    [string]$ContractPath = '',
    [string]$OutputPath = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
    $RepositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
}
if ([string]::IsNullOrWhiteSpace($ContractPath)) {
    $ContractPath = Join-Path `
        $PSScriptRoot '../docs/data/release_surface_contract.json'
}

$modulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseSurface.psm1'
Import-Module $modulePath -Force

$analysis = Invoke-PartisanReleaseSurfaceAnalysis `
    -RepositoryRoot $RepositoryRoot `
    -ContractPath $ContractPath
$json = $analysis | ConvertTo-Json -Depth 8
if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
    $fullOutput = [IO.Path]::GetFullPath($OutputPath)
    $parent = Split-Path -Parent $fullOutput
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        [void](New-Item -ItemType Directory -Path $parent)
    }
    [IO.File]::WriteAllText(
        $fullOutput,
        $json + [Environment]::NewLine,
        [Text.UTF8Encoding]::new($false))
}
if (-not $analysis.passed) {
    $analysis.violations | ForEach-Object {
        Write-Host ('Release-surface violation: ' + $_)
    }
    throw (
        'Release-surface guard validation failed with ' +
        "$($analysis.violations.Count) violation(s).")
}
Write-Host (
    'Release-surface guards PASS: ' +
    "$($analysis.carrierFileCount) carriers, " +
    "$($analysis.mixedDiagnosticFileCount) mixed files, " +
    "$($analysis.forbiddenTypeCount) forbidden types, " +
    "$($analysis.forbiddenCommandActionIdCount) forbidden commands, " +
    "$($analysis.forbiddenMemberSurfaceCount) forbidden member surfaces, " +
    "$($analysis.forbiddenLiteralSurfaceCount) forbidden literal surfaces, " +
    "$($analysis.productionPositiveControlTypeCount) production type controls, " +
    "$($analysis.productionPositiveControlCommandActionIdCount) " +
    'production command controls, ' +
    "$($analysis.productionObservabilityMemberSurfaceCount) " +
    'production observability member controls.')
$analysis

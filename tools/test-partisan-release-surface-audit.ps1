[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'Partisan.ReleaseSurface.psm1') -Force

function Write-Fixture {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$Text
    )

    $path = Join-Path $Root $RelativePath
    $parent = Split-Path -Parent $path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        [void](New-Item -ItemType Directory -Path $parent)
    }
    [IO.File]::WriteAllText(
        $path,
        $Text.Replace([string][char]10, [Environment]::NewLine),
        [Text.UTF8Encoding]::new($false))
}

function Assert-Disposition {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$ContractPath,
        [Parameter(Mandatory = $true)][bool]$Expected
    )

    $result = Invoke-PartisanReleaseSurfaceAnalysis `
        -RepositoryRoot $Root `
        -ContractPath $ContractPath
    if ([bool]$result.passed -ne $Expected) {
        throw (
            "Self-test $Label expected $Expected and got " +
            "$($result.passed): " +
            (@($result.violations) -join ' | '))
    }
}

function Remove-FixtureRoot {
    param([Parameter(Mandatory = $true)][string]$Path)

    $base = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $full = [IO.Path]::GetFullPath($Path).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $prefix = $base + [IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith(
            $prefix,
            [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($full) -cnotmatch
            '^PartisanReleaseSurfaceSelfTest_[0-9a-f]{32}$') {
        throw 'Release-surface self-test cleanup escaped its owned root.'
    }
    foreach ($item in @(Get-ChildItem -LiteralPath $full -Recurse -Force)) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Release-surface self-test cleanup found a reparse point.'
        }
    }
    Remove-Item -LiteralPath $full -Recurse -Force
    if (Test-Path -LiteralPath $full) {
        throw 'Release-surface self-test cleanup did not converge.'
    }
}

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'PartisanReleaseSurfaceSelfTest_' + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $tempRoot)
try {
    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Proof.c' `
        -Text @'
#ifdef ENABLE_DIAG
class HST_Proof
{
	void Run()
	{
		string command = "dev_action";
	}
}
#endif
'@
    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Mixed.c' `
        -Text @'
class HST_Mixed
{
#ifdef ENABLE_DIAG
	void RunProof(HST_Proof proof)
	{
		string hidden = "fixture_only";
	}
#endif
}
'@
    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Production.c' `
        -Text @'
class HST_Production
{
	void Run()
	{
		string command = "inspect_status";
	}
}
'@
    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/Services/HST_CommandUIService.c' `
        -Text @'
class HST_CommandUIService
{
	protected bool IsKnownVisibleCommand(string commandId)
	{
#ifdef ENABLE_DIAG
		if (commandId == "dev_action") return true;
#endif
		return false;
	}

	protected bool IsVisibleCommandDispatchHandled(string commandId)
	{
#ifdef ENABLE_DIAG
		if (commandId == "dev_action") return true;
#endif
		return false;
	}
}
'@
    Write-Fixture -Root $tempRoot `
        -RelativePath 'docs/data/parity.json' `
        -Text @'
{
  "actionRules": [
    {
      "pattern": "^dev_action$",
      "rowId": "development-diagnostics",
      "surface": "development-admin"
    }
  ],
  "commandActionIds": ["dev_action", "inspect_status"]
}
'@
    Write-Fixture -Root $tempRoot `
        -RelativePath 'docs/data/contract.json' `
        -Text @'
{
  "schemaVersion": 1,
  "evidenceKind": "partisan_release_surface_contract_v1",
  "diagnosticSymbol": "ENABLE_DIAG",
  "paritySource": "docs/data/parity.json",
  "carrierFileNamePattern": "(Proof|CampaignDebug|Autotest|SmokeTest|Fixture|Harness)",
  "forbiddenTypeNamePattern": "(Proof|CampaignDebug|Autotest|SmokeTest|Fixture|Harness)",
  "expectedCarrierFileCount": 1,
  "expectedForbiddenTypeCount": 1,
  "expectedForbiddenCommandActionIdCount": 1,
  "expectedForbiddenMemberSurfaceCount": 1,
  "expectedForbiddenLiteralSurfaceCount": 1,
  "expectedProductionObservabilityMemberSurfaceCount": 1,
  "carrierFiles": ["Scripts/Game/HST/HST_Proof.c"],
  "mixedDiagnosticFiles": ["Scripts/Game/HST/HST_Mixed.c", "Scripts/Game/HST/Services/HST_CommandUIService.c"],
  "commandSurfaceFiles": ["Scripts/Game/HST/HST_Production.c", "Scripts/Game/HST/HST_Proof.c", "Scripts/Game/HST/Services/HST_CommandUIService.c"],
  "forbiddenTypeNames": ["HST_Proof"],
  "forbiddenCommandActionIds": ["dev_action"],
  "productionPositiveControlTypeNames": ["HST_Production"],
  "productionPositiveControlCommandActionIds": ["inspect_status"],
  "forbiddenMemberSurfaces": ["Scripts/Game/HST/HST_Mixed.c::RunProof"],
  "forbiddenLiteralSurfaces": ["Scripts/Game/HST/HST_Mixed.c::fixture_only"],
  "productionObservabilityMemberSurfaces": ["Scripts/Game/HST/HST_Production.c::Run"],
  "productionObservabilityPolicy": ["Production controls remain available."]
}
'@
    $contractPath = Join-Path $tempRoot 'docs/data/contract.json'
    $carrierPath = Join-Path $tempRoot 'Scripts/Game/HST/HST_Proof.c'
    $mixedPath = Join-Path $tempRoot 'Scripts/Game/HST/HST_Mixed.c'
    $productionPath =
        Join-Path $tempRoot 'Scripts/Game/HST/HST_Production.c'
    $commandUiPath = Join-Path $tempRoot `
        'Scripts/Game/HST/Services/HST_CommandUIService.c'
    $parityPath = Join-Path $tempRoot 'docs/data/parity.json'
    $baselineCarrier = [IO.File]::ReadAllText($carrierPath)
    $baselineMixed = [IO.File]::ReadAllText($mixedPath)
    $baselineProduction = [IO.File]::ReadAllText($productionPath)
    $baselineCommandUi = [IO.File]::ReadAllText($commandUiPath)
    $baselineParity = [IO.File]::ReadAllText($parityPath)

    Assert-Disposition -Label 'baseline' -Root $tempRoot `
        -ContractPath $contractPath -Expected $true

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/Services/HST_CommandUIService.c' `
        -Text @'
class HST_CommandUIService
{
	protected bool IsKnownVisibleCommand(string commandId)
	{
#ifdef ENABLE_DIAG
		if (commandId == "dev_action") return true;
#endif
		return false;
	}

	protected bool IsVisibleCommandDispatchHandled(string commandId)
	{
		return false;
	}
}
'@
    Assert-Disposition -Label 'missing diagnostic command route' `
        -Root $tempRoot -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $commandUiPath,
        $baselineCommandUi,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Mixed.c' `
        -Text @'
class HST_Mixed
{
	HST_Proof m_Leak;
#ifdef ENABLE_DIAG
	void RunProof(HST_Proof proof)
	{
	}
#endif
}
'@
    Assert-Disposition -Label 'retail type leak' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $mixedPath,
        $baselineMixed,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Mixed.c' `
        -Text @'
class HST_Mixed
{
	void RunProof()
	{
	}
#ifdef ENABLE_DIAG
	void DiagnosticPlaceholder()
	{
		string hidden = "fixture_only";
	}
#endif
}
'@
    Assert-Disposition -Label 'retail member leak' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $mixedPath,
        $baselineMixed,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Production.c' `
        -Text @'
class HST_Production
{
	void Run()
	{
		string keep = "inspect_status";
		string leak = "dev_action";
	}
}
'@
    Assert-Disposition -Label 'retail command leak' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $productionPath,
        $baselineProduction,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Mixed.c' `
        -Text @'
class HST_Mixed
{
	string m_RetailLeak = "fixture_only";
#ifdef ENABLE_DIAG
	void RunProof(HST_Proof proof)
	{
	}
#endif
}
'@
    Assert-Disposition -Label 'retail literal leak' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $mixedPath,
        $baselineMixed,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Proof.c' `
        -Text @'
#define ENABLE_DIAG
#ifdef ENABLE_DIAG
class HST_Proof
{
	string command = "dev_action";
}
#endif
'@
    Assert-Disposition -Label 'local symbol mutation' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Proof.c' `
        -Text @'
#if defined(ENABLE_DIAG)
class HST_Proof
{
	string command = "dev_action";
}
#endif
'@
    Assert-Disposition -Label 'partial condition' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Proof.c' `
        -Text @'
#ifdef ENABLE_DIAG
class HST_Proof
{
	string command = "dev_action";
}
#else
class HST_RetailLeak
{
	HST_Proof m_Leak;
}
#endif
'@
    Assert-Disposition -Label 'retail else leak' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $carrierPath,
        $baselineCarrier,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_ExtraProof.c' `
        -Text @'
#ifdef ENABLE_DIAG
class HST_ExtraProof
{
}
#endif
'@
    Assert-Disposition -Label 'unrecorded carrier' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    Remove-Item -LiteralPath (
        Join-Path $tempRoot 'Scripts/Game/HST/HST_ExtraProof.c') -Force

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Unrecorded.c' `
        -Text @'
class HST_UnrecordedProof
{
}
'@
    Assert-Disposition -Label 'unrecorded type' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    Remove-Item -LiteralPath (
        Join-Path $tempRoot 'Scripts/Game/HST/HST_Unrecorded.c') -Force

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Mixed.c' `
        -Text @'
class HST_Mixed
{
#ifdef ENABLE_DIAG
	class HST_NeutralHelper
	{
	}
	void RunProof(HST_Proof proof)
	{
		string hidden = "fixture_only";
	}
#endif
}
'@
    Assert-Disposition -Label 'neutral diagnostic type drift' `
        -Root $tempRoot -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $mixedPath,
        $baselineMixed,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Production.c' `
        -Text @'
class HST_Production
{
}
'@
    Assert-Disposition -Label 'missing positive control' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $productionPath,
        $baselineProduction,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'Scripts/Game/HST/HST_Production.c' `
        -Text @'
class HST_Production
{
	void Observe()
	{
		string command = "inspect_status";
	}
}
'@
    Assert-Disposition -Label 'missing production member control' `
        -Root $tempRoot -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $productionPath,
        $baselineProduction,
        [Text.UTF8Encoding]::new($false))

    Write-Fixture -Root $tempRoot `
        -RelativePath 'docs/data/parity.json' `
        -Text @'
{
  "actionRules": [
    {
      "pattern": "^changed_dev_action$",
      "rowId": "development-diagnostics",
      "surface": "development-admin"
    }
  ],
  "commandActionIds": ["changed_dev_action", "inspect_status"]
}
'@
    Assert-Disposition -Label 'parity drift' -Root $tempRoot `
        -ContractPath $contractPath -Expected $false
    [IO.File]::WriteAllText(
        $parityPath,
        $baselineParity,
        [Text.UTF8Encoding]::new($false))

    Write-Host 'Release-surface source-audit self-tests PASS: 15/15'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-FixtureRoot -Path $tempRoot
    }
}

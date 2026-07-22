[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$runnerPath = Join-Path $PSScriptRoot 'run-guarded-focused-autotest.ps1'
$tokens = $null
$parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    $runnerPath,
    [ref]$tokens,
    [ref]$parseErrors)
if ($parseErrors.Count -ne 0) {
    throw 'The focused runner must parse before its mount helper is tested.'
}
$definitions = @($ast.FindAll({
    param($node)
    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
        $node.Name -ceq 'Get-CandidateMountAttestation'
}, $true))
if ($definitions.Count -ne 1) {
    throw 'The focused runner must define exactly one mount-attestation helper.'
}
$definition = [ScriptBlock]::Create($definitions[0].Extent.Text)
. $definition

$checks = 0
function Assert-MountCondition {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
    $script:checks++
}

$tempRoot = Join-Path `
    ([IO.Path]::GetTempPath()) `
    ('PartisanFocusedMountSelfTest-' + [Guid]::NewGuid().ToString('N'))
$guardRoot = Join-Path $tempRoot 'guard'
$projectPath = Join-Path $guardRoot 'candidate-addons\Partisan\addon.gproj'
$consolePath = Join-Path $guardRoot 'logs\console.log'
$candidateGuid = '698532771130111D'
$foreignCoreGuid = '5614BBCCBB55ED1C'
$foreignDataGuid = '58D0FB3206B6F859'

function Write-MountConsole {
    param([Parameter(Mandatory = $true)][string[]]$Lines)

    $parent = Split-Path -Parent $consolePath
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
    [IO.File]::WriteAllText(
        $consolePath,
        (@($Lines) -join "`n") + "`n",
        (New-Object Text.UTF8Encoding($false, $true)))
}

function Invoke-MountCase {
    Get-CandidateMountAttestation `
        -GuardRoot $guardRoot `
        -PackedProjectPath $projectPath `
        -AddonGuid $candidateGuid
}

try {
    New-Item -ItemType Directory -Path (Split-Path -Parent $projectPath) `
        -Force | Out-Null
    [IO.File]::WriteAllText($projectPath, '')
    $projectLogPath = [IO.Path]::GetFullPath($projectPath).Replace('\', '/')

    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid' (packed)",
        "00:00:01.100 ENGINE : gproj: 'runtime/core/core.gproj' guid: '$foreignCoreGuid' (packed)",
        "00:00:01.200 ENGINE : gproj: 'runtime/data/data.gproj' guid: '$foreignDataGuid' (packed)",
        "00:00:02.000 ENGINE : gproj: 'runtime/core/core.gproj' guid: '$foreignCoreGuid'",
        "00:00:02.100 ENGINE : gproj: 'runtime/data/data.gproj' guid: '$foreignDataGuid'",
        "00:00:02.200 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid'"
    )
    $valid = Invoke-MountCase
    Assert-MountCondition $valid.Valid 'Candidate plus foreign mounts must pass.'
    Assert-MountCondition ($valid.RecordCount -eq 2) `
        'Only exact candidate-GUID rows may enter the mount census.'
    Assert-MountCondition ($valid.ExactPathCount -eq 2) `
        'Both candidate rows must use the exact staged path.'
    Assert-MountCondition ($valid.PackedCount -eq 1) `
        'Exactly one fixture candidate row must attest packed mode.'
    Assert-MountCondition $valid.GuidExact `
        'The canonical candidate GUID must be exact.'

    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid' (packed)"
    )
    $packedOnly = Invoke-MountCase
    Assert-MountCondition (-not $packedOnly.Valid -and
        $packedOnly.RecordCount -eq 1 -and $packedOnly.PackedCount -eq 1) `
        'A packed availability row without a loaded-addon row must fail.'

    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid' (packed)",
        "00:00:02.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid' (packed)"
    )
    $duplicatePacked = Invoke-MountCase
    Assert-MountCondition (-not $duplicatePacked.Valid -and
        $duplicatePacked.RecordCount -eq 2 -and
        $duplicatePacked.PackedCount -eq 2 -and
        -not $duplicatePacked.Packed) `
        'Two packed candidate rows must fail the exact mount lifecycle.'

    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid' (packed)",
        "00:00:02.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid'",
        "00:00:03.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid'"
    )
    $extraCandidate = Invoke-MountCase
    Assert-MountCondition (-not $extraCandidate.Valid -and
        $extraCandidate.RecordCount -eq 3 -and
        $extraCandidate.ExactPathCount -eq 3) `
        'An extra candidate mount row must fail the exact mount lifecycle.'

    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: 'runtime/core/core.gproj' guid: '$foreignCoreGuid' (packed)",
        "00:00:02.000 ENGINE : gproj: 'runtime/core/core.gproj' guid: '$foreignCoreGuid'"
    )
    $foreignOnly = Invoke-MountCase
    Assert-MountCondition (-not $foreignOnly.Valid -and
        $foreignOnly.RecordCount -eq 0) `
        'Foreign mounts alone must not attest the candidate.'

    $wrongPath = $projectLogPath.Replace(
        '/candidate-addons/Partisan/addon.gproj',
        '/other-addons/Partisan/addon.gproj')
    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$wrongPath' guid: '$candidateGuid' (packed)",
        "00:00:02.000 ENGINE : gproj: '$wrongPath' guid: '$candidateGuid'"
    )
    $wrongPathResult = Invoke-MountCase
    Assert-MountCondition (-not $wrongPathResult.Valid -and
        $wrongPathResult.ExactPathCount -eq 0) `
        'The candidate GUID on a different path must fail.'

    $lowerGuid = $candidateGuid.ToLowerInvariant()
    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$projectLogPath' guid: '$lowerGuid' (packed)",
        "00:00:02.000 ENGINE : gproj: '$projectLogPath' guid: '$lowerGuid'"
    )
    $caseDrift = Invoke-MountCase
    Assert-MountCondition (-not $caseDrift.Valid -and
        $caseDrift.RecordCount -eq 0 -and -not $caseDrift.GuidExact) `
        'A case-drifted candidate GUID must not be exact.'

    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid' (loose)",
        "00:00:02.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid'"
    )
    $invalidMode = Invoke-MountCase
    Assert-MountCondition (-not $invalidMode.Valid -and
        $invalidMode.InvalidModeCount -eq 1 -and
        $invalidMode.PackedCount -eq 0) `
        'An invalid candidate mode must fail.'

    Write-MountConsole @(
        "00:00:01.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid'",
        "00:00:02.000 ENGINE : gproj: '$projectLogPath' guid: '$candidateGuid'"
    )
    $unpacked = Invoke-MountCase
    Assert-MountCondition (-not $unpacked.Valid -and
        $unpacked.PackedCount -eq 0 -and -not $unpacked.Packed) `
        'Candidate rows without a packed record must fail.'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        $resolvedTemp = [IO.Path]::GetFullPath($tempRoot)
        $systemTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd(
            [IO.Path]::DirectorySeparatorChar,
            [IO.Path]::AltDirectorySeparatorChar) +
            [IO.Path]::DirectorySeparatorChar
        if (-not $resolvedTemp.StartsWith(
                $systemTemp,
                [StringComparison]::OrdinalIgnoreCase) -or
            (Split-Path -Leaf $resolvedTemp) -cnotmatch
                '^PartisanFocusedMountSelfTest-[0-9a-f]{32}$') {
            throw 'Focused mount self-test cleanup root is not nonce-owned.'
        }
        Remove-Item -LiteralPath $resolvedTemp -Recurse -Force
    }
}

if (Test-Path -LiteralPath $tempRoot) {
    throw 'Focused mount self-test cleanup did not converge.'
}

Write-Output ('FOCUSED_MOUNT_ATTESTATION_SELFTEST ' +
    ([pscustomobject][ordered]@{
        status = 'PASS'
        checks = $checks
        candidateRows = 2
        foreignRowsIgnored = 4
        exactGuidCase = $true
        exactStagedPath = $true
        packedModeRequired = $true
        cleanup = 'exact-zero'
    } | ConvertTo-Json -Compress))

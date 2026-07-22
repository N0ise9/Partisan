[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-Condition {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Condition) {
        throw "Self-test assertion failed: $Label."
    }
}

function Assert-Rejected {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][scriptblock]$Action,
        [string]$ExpectedIdentifier
    )

    $rejected = $false
    try {
        & $Action
    }
    catch {
        if (-not [string]::IsNullOrWhiteSpace($ExpectedIdentifier) -and
            -not $_.Exception.Message.Contains(
                '[' + $ExpectedIdentifier + ']')) {
            throw ('Self-test rejection identifier mismatch for ' + $Label +
                '; expected=' + $ExpectedIdentifier + ';actual=' +
                $_.Exception.Message)
        }
        $rejected = $true
    }
    if (-not $rejected) {
        throw "Self-test expected rejection: $Label."
    }
}

function Get-TextSha256 {
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Get-FreeUdpPort {
    $socket = New-Object Net.Sockets.Socket(
        [Net.Sockets.AddressFamily]::InterNetwork,
        [Net.Sockets.SocketType]::Dgram,
        [Net.Sockets.ProtocolType]::Udp)
    try {
        $socket.ExclusiveAddressUse = $true
        $socket.Bind((New-Object Net.IPEndPoint(
            [Net.IPAddress]::Loopback,
            0)))
        return [int]$socket.LocalEndPoint.Port
    }
    finally {
        $socket.Dispose()
    }
}

function New-SelfTestCandidate {
    param(
        [Parameter(Mandatory = $true)][string]$CandidateSource,
        [Parameter(Mandatory = $true)][string]$RuntimeAddon,
        [Parameter(Mandatory = $true)][object[]]$PackageRows,
        [Parameter(Mandatory = $true)][string]$PackageSha256,
        [string]$CandidateId = 'self-test-candidate'
    )

    $rows = @($PackageRows | ForEach-Object {
        [pscustomobject][ordered]@{
            indexPath = [string]$_.indexPath
            length = [long]$_.length
            sha256 = [string]$_.sha256
        }
    })
    return [pscustomobject][ordered]@{
        CandidateId = $CandidateId
        PackageSha256 = $PackageSha256
        PackageFiles = [object[]]$rows
        PackedAddonPath = $CandidateSource
        RuntimeAddonRootPath = $RuntimeAddon
    }
}

function New-SelfTestRuntimeContext {
    param(
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [Parameter(Mandatory = $true)][string]$WatchedRoot,
        [Parameter(Mandatory = $true)][string]$SpillRoot,
        [Parameter(Mandatory = $true)][string]$Purpose,
        [string[]]$Faults = @()
    )

    return New-PartisanGuardedRuntimeContext `
        -GuardBase $GuardBase `
        -Purpose $Purpose `
        -WatchedRoots @($WatchedRoot) `
        -SpillRoots @($SpillRoot) `
        -LoopbackPorts @((Get-FreeUdpPort)) `
        -NonEngineSelfTestFaults $Faults
}

$modulePath = Join-Path $PSScriptRoot 'Partisan.GuardedRuntime.psm1'
Import-Module -Name $modulePath -Force -ErrorAction Stop

$selfTestStopwatch = [Diagnostics.Stopwatch]::StartNew()
$checks = New-Object Collections.Generic.List[string]
$tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$tempRoot = [IO.Path]::GetFullPath((Join-Path `
    $tempBase `
    ('PartisanGuardedRuntimeSelfTest_' + [Guid]::NewGuid().ToString('N'))))
$junctionPath = $null
$occupiedSocket = $null
$context = $null
$poisonContext = $null
$testFailure = $null
$cleanupFailure = $null

if (-not (Test-PartisanContainedPath `
        -Root $tempBase `
        -Candidate $tempRoot)) {
    throw 'The self-test temporary root escaped the operating-system temp root.'
}
New-Item -ItemType Directory -Path $tempRoot -ErrorAction Stop | Out-Null

try {
    $containmentRoot = Join-Path $tempRoot 'containment'
    $contained = Join-Path $containmentRoot 'inside\leaf'
    $prefixSibling = $containmentRoot + '-escape\leaf'
    New-Item -ItemType Directory -Path $contained -Force | Out-Null
    Assert-Condition `
        -Condition (Test-PartisanContainedPath `
            -Root $containmentRoot `
            -Candidate $contained) `
        -Label 'contained path accepted'
    Assert-Condition `
        -Condition (-not (Test-PartisanContainedPath `
            -Root $containmentRoot `
            -Candidate $prefixSibling)) `
        -Label 'prefix sibling rejected'
    Assert-Condition `
        -Condition (-not (Test-PartisanContainedPath `
            -Root $containmentRoot `
            -Candidate (Join-Path $containmentRoot '..\escape'))) `
        -Label 'parent traversal rejected'
    [void]$checks.Add('path-containment')

    $reparseTarget = Join-Path $tempRoot 'reparse-target'
    $junctionPath = Join-Path $containmentRoot 'junction'
    New-Item -ItemType Directory -Path $reparseTarget | Out-Null
    try {
        New-Item `
            -ItemType Junction `
            -Path $junctionPath `
            -Target $reparseTarget `
            -ErrorAction Stop | Out-Null
        Assert-Rejected -Label 'reparse ancestry' -Action {
            Assert-PartisanNoReparseAncestry `
                -Path (Join-Path $junctionPath 'child')
        }
        Assert-Rejected -Label 'reparse descendant' -Action {
            Assert-PartisanNoReparseTree -Root $containmentRoot
        }
        Remove-Item -LiteralPath $junctionPath -Force -ErrorAction Stop
        $junctionPath = $null
        [void]$checks.Add('reparse-rejection')
    }
    finally {
        if ($junctionPath -and (Test-Path -LiteralPath $junctionPath)) {
            Remove-Item -LiteralPath $junctionPath -Force -ErrorAction Stop
            $junctionPath = $null
        }
    }

    $fixtureExecutable = Join-Path $tempRoot 'program folder\runtime.exe'
    $argumentVector = @(
        '',
        'plain',
        'two words',
        'trail\',
        'embedded"quote',
        'slashes\\before"quote',
        "tab`tvalue")
    $commandLine = ConvertTo-PartisanNativeCommandLine `
        -Executable $fixtureExecutable `
        -Arguments $argumentVector
    Assert-Condition `
        -Condition (Test-PartisanExactNativeArgumentVector `
            -CommandLine $commandLine `
            -ExpectedExecutable $fixtureExecutable `
            -ExpectedArguments $argumentVector) `
        -Label 'native argument vector round-trip'
    $changedArguments = [string[]]$argumentVector.Clone()
    $changedArguments[2] = 'changed words'
    Assert-Condition `
        -Condition (-not (Test-PartisanExactNativeArgumentVector `
            -CommandLine $commandLine `
            -ExpectedExecutable $fixtureExecutable `
            -ExpectedArguments $changedArguments)) `
        -Label 'native argument mismatch rejected'
    [void]$checks.Add('native-argument-encoding')

    $identityTime = [DateTime]::UtcNow
    $expectedIdentity = [pscustomobject][ordered]@{
        ProcessId = 4242
        StartUtc = $identityTime
        ExecutablePath = $fixtureExecutable
        Arguments = @('one', 'two words')
        CommandLine = $commandLine
    }
    $matchingIdentity = [pscustomobject][ordered]@{
        ProcessId = 4242
        StartUtc = $identityTime
        ExecutablePath = $fixtureExecutable
        Arguments = @('one', 'two words')
    }
    Assert-Condition `
        -Condition (Test-PartisanProcessIdentity `
            -Expected $expectedIdentity `
            -Actual $matchingIdentity) `
        -Label 'matching process identity accepted'
    foreach ($mismatch in @(
            [pscustomobject]@{
                ProcessId = 4243
                StartUtc = $identityTime
                ExecutablePath = $fixtureExecutable
                Arguments = @('one', 'two words')
            },
            [pscustomobject]@{
                ProcessId = 4242
                StartUtc = $identityTime.AddTicks(1)
                ExecutablePath = $fixtureExecutable
                Arguments = @('one', 'two words')
            },
            [pscustomobject]@{
                ProcessId = 4242
                StartUtc = $identityTime
                ExecutablePath = ($fixtureExecutable + '.other')
                Arguments = @('one', 'two words')
            },
            [pscustomobject]@{
                ProcessId = 4242
                StartUtc = $identityTime
                ExecutablePath = $fixtureExecutable
                Arguments = @('one', 'changed')
            })) {
        Assert-Condition `
            -Condition (-not (Test-PartisanProcessIdentity `
                -Expected $expectedIdentity `
                -Actual $mismatch)) `
            -Label 'process identity mismatch rejected'
    }
    [void]$checks.Add('process-identity-ledger')

    $runtimeModule = Get-Module `
        -Name 'Partisan.GuardedRuntime' `
        -ErrorAction Stop
    $identityPolicy = & $runtimeModule {
        param($IdentityValue)

        $newProbe = {
            param(
                [bool]$HasExited,
                [bool]$RefreshFails
            )

            $probe = [pscustomobject][ordered]@{
                HasExited = $HasExited
                RefreshFails = $RefreshFails
                RefreshCount = 0
                InspectionCount = 0
            }
            $probe | Add-Member `
                -MemberType ScriptMethod `
                -Name Refresh `
                -Value {
                    [void]($this.RefreshCount++)
                    if ($this.RefreshFails) {
                        throw 'Injected process-state inspection failure.'
                    }
                }
            return $probe
        }

        $raceProbe = & $newProbe $false $false
        $raceInspector = {
            param([int]$TargetProcessId)
            [void]($raceProbe.InspectionCount++)
            $raceProbe.HasExited = $true
            throw 'Injected identity inspection failure after normal exit.'
        }.GetNewClosure()

        $liveFailureProbe = & $newProbe $false $false
        $liveFailureInspector = {
            param([int]$TargetProcessId)
            [void]($liveFailureProbe.InspectionCount++)
            throw 'Injected live identity inspection failure.'
        }.GetNewClosure()

        $mismatchProbe = & $newProbe $false $false
        $mismatchActual = $IdentityValue.PSObject.Copy()
        $mismatchActual.Arguments = [string[]]@('changed')
        $mismatchInspector = {
            param([int]$TargetProcessId)
            [void]($mismatchProbe.InspectionCount++)
            return $mismatchActual
        }.GetNewClosure()

        $stateFailureProbe = & $newProbe $false $true
        $stateFailureInspector = {
            param([int]$TargetProcessId)
            [void]($stateFailureProbe.InspectionCount++)
            return $IdentityValue
        }.GetNewClosure()

        $race = Get-PartisanProcessIdentityStatusCore `
            $IdentityValue $raceProbe $raceInspector
        $liveFailure = Get-PartisanProcessIdentityStatusCore `
            $IdentityValue $liveFailureProbe $liveFailureInspector
        $mismatch = Get-PartisanProcessIdentityStatusCore `
            $IdentityValue $mismatchProbe $mismatchInspector
        $stateFailure = Get-PartisanProcessIdentityStatusCore `
            $IdentityValue $stateFailureProbe $stateFailureInspector

        return [pscustomobject][ordered]@{
            Race = $race
            LiveFailure = $liveFailure
            Mismatch = $mismatch
            StateFailure = $stateFailure
            RaceProbe = $raceProbe
            LiveFailureProbe = $liveFailureProbe
            MismatchProbe = $mismatchProbe
            StateFailureProbe = $stateFailureProbe
        }
    } $expectedIdentity
    Assert-Condition `
        -Condition ($identityPolicy.Race.Status -ceq 'dead' -and
            $identityPolicy.Race.Reason -ceq
                'process-exited-during-identity-inspection' -and
            $null -eq $identityPolicy.Race.Actual -and
            [int]$identityPolicy.RaceProbe.RefreshCount -eq 2 -and
            [int]$identityPolicy.RaceProbe.InspectionCount -eq 1) `
        -Label 'normal exit during identity inspection resolves dead'
    Assert-Condition `
        -Condition ($identityPolicy.LiveFailure.Status -ceq 'unknown' -and
            $identityPolicy.LiveFailure.Reason -ceq
                'identity-inspection-failed' -and
            [int]$identityPolicy.LiveFailureProbe.RefreshCount -eq 2 -and
            [int]$identityPolicy.LiveFailureProbe.InspectionCount -eq 1) `
        -Label 'live identity inspection failure remains unknown'
    Assert-Condition `
        -Condition ($identityPolicy.Mismatch.Status -ceq 'unknown' -and
            $identityPolicy.Mismatch.Reason -ceq 'identity-mismatch' -and
            [int]$identityPolicy.MismatchProbe.RefreshCount -eq 1 -and
            [int]$identityPolicy.MismatchProbe.InspectionCount -eq 1) `
        -Label 'live identity mismatch remains unknown'
    Assert-Condition `
        -Condition ($identityPolicy.StateFailure.Status -ceq 'unknown' -and
            $identityPolicy.StateFailure.Reason -ceq
                'process-state-inspection-failed' -and
            [int]$identityPolicy.StateFailureProbe.RefreshCount -eq 1 -and
            [int]$identityPolicy.StateFailureProbe.InspectionCount -eq 0) `
        -Label 'process state inspection failure remains unknown'
    [void]$checks.Add('process-identity-normal-exit-race-policy')

    $jsonRoot = Join-Path $tempRoot 'json'
    New-Item -ItemType Directory -Path $jsonRoot | Out-Null
    $jsonOne = Join-Path $jsonRoot 'one.json'
    $jsonTwo = Join-Path $jsonRoot 'two.json'
    $jsonValue = [pscustomobject][ordered]@{
        zeta = 9
        alpha = [ordered]@{
            second = 'value'
            first = 1
        }
        rows = @(
            [pscustomobject][ordered]@{ b = 2; a = 1 },
            [pscustomobject][ordered]@{ d = 4; c = 3 })
    }
    [void](Write-PartisanPortableJson -Path $jsonOne -Value $jsonValue)
    [void](Write-PartisanPortableJson -Path $jsonTwo -Value $jsonValue)
    $bytesOne = [IO.File]::ReadAllBytes($jsonOne)
    $bytesTwo = [IO.File]::ReadAllBytes($jsonTwo)
    Assert-Condition `
        -Condition ([Convert]::ToBase64String($bytesOne) -ceq
            [Convert]::ToBase64String($bytesTwo)) `
        -Label 'canonical JSON bytes stable'
    Assert-Condition `
        -Condition (-not ($bytesOne.Length -ge 3 -and
            $bytesOne[0] -eq 0xEF -and
            $bytesOne[1] -eq 0xBB -and
            $bytesOne[2] -eq 0xBF)) `
        -Label 'JSON has no BOM'
    $jsonText = [IO.File]::ReadAllText($jsonOne)
    Assert-Condition `
        -Condition ($jsonText.EndsWith("`n") -and -not $jsonText.Contains("`r")) `
        -Label 'JSON uses portable newlines'
    $signatureOne = Get-PartisanFileSignature -Path $jsonOne
    $signatureTwo = Get-PartisanFileSignature -Path $jsonTwo
    Assert-Condition `
        -Condition ($signatureOne.length -eq $signatureTwo.length -and
            $signatureOne.sha256 -ceq $signatureTwo.sha256) `
        -Label 'portable file signature stable'
    $jsonRead = Read-PartisanPortableJson -Path $jsonOne
    Assert-Condition `
        -Condition ([int]$jsonRead.alpha.first -eq 1) `
        -Label 'portable JSON readback'
    [void]$checks.Add('json-hash-stability')

    $guardBase = Join-Path $tempRoot 'guard-base'
    New-Item -ItemType Directory -Path $guardBase | Out-Null
    $guard = New-PartisanGuardDirectory `
        -GuardBase $guardBase `
        -Purpose 'self_test_guard'
    [IO.File]::WriteAllText((Join-Path $guard.Directory 'owned.txt'), 'owned')
    $wrongOwner = $guard.PSObject.Copy()
    $wrongOwner.OwnerPid = $PID + 1
    Assert-Rejected -Label 'copied guard owner' -Action {
        Remove-PartisanGuardDirectory -Ownership $wrongOwner
    }
    Assert-Condition `
        -Condition (Test-Path -LiteralPath $guard.Directory -PathType Container) `
        -Label 'wrong owner did not remove guard'
    $sentinelBytes = [IO.File]::ReadAllBytes($guard.Sentinel)
    $sameFields = Read-PartisanPortableJson -Path $guard.Sentinel
    $differentBytes = $sameFields | ConvertTo-Json -Compress -Depth 8
    [IO.File]::WriteAllText(
        $guard.Sentinel,
        $differentBytes,
        (New-Object Text.UTF8Encoding($false)))
    Assert-Rejected -Label 'same-fields different-bytes sentinel' -Action {
        Remove-PartisanGuardDirectory -Ownership $guard
    }
    Assert-Condition `
        -Condition (Test-Path -LiteralPath $guard.Directory -PathType Container) `
        -Label 'byte-tampered sentinel did not remove guard'
    [IO.File]::WriteAllBytes($guard.Sentinel, $sentinelBytes)
    Assert-Rejected `
        -Label 'unknown guard child rejects cleanup without recursive erase' `
        -ExpectedIdentifier 'PGR_GUARD_INVENTORY_MISMATCH' `
        -Action { Remove-PartisanGuardDirectory -Ownership $guard }
    Assert-Condition `
        -Condition ([IO.File]::ReadAllText((Join-Path `
                $guard.Directory `
                'owned.txt')) -ceq 'owned') `
        -Label 'unknown guard child preserved byte-for-byte'
    Remove-Item -LiteralPath (Join-Path $guard.Directory 'owned.txt') -Force
    [void](Remove-PartisanGuardDirectory -Ownership $guard)
    Assert-Condition `
        -Condition (-not (Test-Path -LiteralPath $guard.Directory)) `
        -Label 'exact owned guard removed'
    [void]$checks.Add('guard-ownership-cleanup')

    $snapshotRoot = Join-Path $tempRoot 'snapshot'
    New-Item -ItemType Directory -Path $snapshotRoot | Out-Null
    $stableFile = Join-Path $snapshotRoot 'stable.txt'
    $deleteFile = Join-Path $snapshotRoot 'delete.txt'
    [IO.File]::WriteAllText($stableFile, 'before')
    [IO.File]::WriteAllText($deleteFile, 'delete')
    $snapshot = New-PartisanRootSnapshot -Root $snapshotRoot -Kind watched
    [IO.File]::WriteAllText($stableFile, 'after')
    Remove-Item -LiteralPath $deleteFile -Force
    [IO.File]::WriteAllText((Join-Path $snapshotRoot 'new.txt'), 'new')
    $delta = Compare-PartisanRootSnapshot -Snapshot $snapshot
    Assert-Condition `
        -Condition ($delta.NewEntries -eq 1 -and
            $delta.ModifiedEntries -eq 1 -and
            $delta.DeletedEntries -eq 1 -and
            -not $delta.Clean) `
        -Label 'snapshot new modified deleted delta'

    $watchedRoot = Join-Path $tempRoot 'watched-boundary'
    $spillRoot = Join-Path $tempRoot 'spill-boundary'
    New-Item -ItemType Directory -Path $watchedRoot | Out-Null
    New-Item -ItemType Directory -Path $spillRoot | Out-Null
    $boundarySet = New-PartisanBoundarySnapshotSet `
        -WatchedRoots @($watchedRoot) `
        -SpillRoots @($spillRoot)
    [IO.File]::WriteAllText((Join-Path $spillRoot 'spill.txt'), 'spill')
    $boundaryDelta = Compare-PartisanBoundarySnapshotSet -Snapshots $boundarySet
    Assert-Condition `
        -Condition ($boundaryDelta.NewEntries -eq 1 -and
            @($boundaryDelta.Deltas | Where-Object {
                $_.Kind -ceq 'spill' -and $_.NewEntries -eq 1
            }).Count -eq 1) `
        -Label 'spill boundary delta detected'
    [void]$checks.Add('snapshot-spill-deltas')

    $occupiedSocket = New-Object Net.Sockets.Socket(
        [Net.Sockets.AddressFamily]::InterNetwork,
        [Net.Sockets.SocketType]::Dgram,
        [Net.Sockets.ProtocolType]::Udp)
    $occupiedSocket.ExclusiveAddressUse = $true
    $occupiedSocket.Bind((New-Object Net.IPEndPoint(
        [Net.IPAddress]::Loopback,
        0)))
    $occupiedPort = [int]$occupiedSocket.LocalEndPoint.Port
    Assert-Condition `
        -Condition (-not (Test-PartisanLoopbackPortAvailable `
            -Port $occupiedPort `
            -Protocol Udp)) `
        -Label 'occupied loopback port rejected'
    Assert-Rejected -Label 'occupied loopback port assertion' -Action {
        Assert-PartisanLoopbackPortAvailable -Port $occupiedPort -Protocol Udp
    }
    $occupiedSocket.Dispose()
    $occupiedSocket = $null
    [void](Wait-PartisanLoopbackPortReleased `
        -Port $occupiedPort `
        -Protocol Udp `
        -TimeoutSeconds 5)
    [void]$checks.Add('loopback-port-gate')

    $provenanceFile = Join-Path $tempRoot 'runtime-fixture.exe'
    [IO.File]::WriteAllText($provenanceFile, 'fixture-one')
    $provenance = Get-PartisanExecutableProvenance -Path $provenanceFile
    [void](Assert-PartisanExecutableProvenance `
        -Expected $provenance `
        -Path $provenanceFile)
    [IO.File]::WriteAllText($provenanceFile, 'fixture-two')
    Assert-Rejected -Label 'executable provenance tamper' -Action {
        Assert-PartisanExecutableProvenance `
            -Expected $provenance `
            -Path $provenanceFile
    }
    [void]$checks.Add('executable-provenance')

    Assert-Rejected -Label 'null teardown context' -Action {
        Invoke-PartisanGuardedTeardown -Context $null
    }
    Assert-Rejected -Label 'shallow teardown context' -Action {
        Invoke-PartisanGuardedTeardown -Context ([pscustomobject]@{
            Magic = 'not-a-runtime-context'
        })
    }
    [void]$checks.Add('negative-teardown-inputs')

    if ($false) {
    $engineNames = @(
        'ArmaReforger', 'ArmaReforger_BE', 'ArmaReforgerSteam',
        'ArmaReforgerSteamDiag', 'ArmaReforgerServer',
        'ArmaReforgerServerDiag', 'ArmaReforgerWorkbench',
        'ArmaReforgerWorkbenchDiag', 'ArmaReforgerWorkbenchSteamDiag',
        'CrashReporter', 'CrashReportClient')
    $liveEngines = @(Get-Process -ErrorAction Stop | Where-Object {
        $engineNames -contains $_.ProcessName
    })
    Assert-Condition `
        -Condition ($liveEngines.Count -eq 0) `
        -Label 'engine-free self-test preflight'

    $contextGuardBase = Join-Path $tempRoot 'context-guard'
    $contextWatched = Join-Path $tempRoot 'context-watched'
    $contextSpill = Join-Path $tempRoot 'context-spill'
    $candidateSource = Join-Path $tempRoot 'candidate-source'
    $runtimeAddon = Join-Path $tempRoot 'runtime-addons'
    foreach ($directory in @(
            $contextGuardBase,
            $contextWatched,
            $contextSpill,
            $candidateSource,
            $runtimeAddon)) {
        New-Item -ItemType Directory -Path $directory | Out-Null
    }
    $candidateContents = [ordered]@{
        'addon.gproj' = 'project-fixture'
        'data.pak' = 'package-fixture'
    }
    $packageRows = New-Object Collections.Generic.List[object]
    foreach ($name in $candidateContents.Keys) {
        $path = Join-Path $candidateSource $name
        [IO.File]::WriteAllText($path, [string]$candidateContents[$name])
        $signature = Get-PartisanFileSignature -Path $path
        [void]$packageRows.Add([pscustomobject][ordered]@{
            indexPath = 'Partisan/' + $name
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $indexLines = @($packageRows.ToArray() | Sort-Object indexPath | ForEach-Object {
        "{0}`t{1}`t{2}" -f $_.sha256, $_.length, $_.indexPath
    })
    $canonicalIndexText = ($indexLines -join "`n") + "`n"
    $canonicalIndexBytes =
        (New-Object Text.UTF8Encoding($false)).GetBytes($canonicalIndexText)
    Assert-Condition `
        -Condition ($canonicalIndexBytes -contains 0x09 -and
            -not $canonicalIndexText.Contains('`t')) `
        -Label 'candidate canonical index uses real tab bytes'
    $candidate = [pscustomobject][ordered]@{
        CandidateId = 'self-test-candidate'
        PackageSha256 = Get-TextSha256 -Text $canonicalIndexText
        PackageFiles = $packageRows.ToArray()
        PackedAddonPath = $candidateSource
        RuntimeAddonRootPath = $runtimeAddon
    }

    $commaGuard = Join-Path $tempRoot 'guard,comma'
    New-Item -ItemType Directory -Path $commaGuard | Out-Null
    Assert-Rejected -Label 'comma guard base' -Action {
        New-PartisanGuardedRuntimeContext `
            -GuardBase $commaGuard `
            -Purpose 'self_test_comma' `
            -WatchedRoots @($contextWatched) `
            -SpillRoots @($contextSpill) `
            -LoopbackPorts @((Get-FreeUdpPort))
    }

    $context = New-PartisanGuardedRuntimeContext `
        -GuardBase $contextGuardBase `
        -Purpose 'self_test_context' `
        -WatchedRoots @($contextWatched) `
        -SpillRoots @($contextSpill) `
        -LoopbackPorts @((Get-FreeUdpPort))
    $commaRuntime = Join-Path $tempRoot 'runtime,comma'
    New-Item -ItemType Directory -Path $commaRuntime | Out-Null
    $commaCandidate = $candidate.PSObject.Copy()
    $commaCandidate.RuntimeAddonRootPath = $commaRuntime
    Assert-Rejected -Label 'comma candidate stage path' -Action {
        New-PartisanCandidateStage `
            -Context $context `
            -Candidate $commaCandidate
    }
    $stage = New-PartisanCandidateStage -Context $context -Candidate $candidate
    [void](Assert-PartisanCandidateStage -Context $context -Stage $stage)
    $guardDirectory = $context.Guard.Directory

    $wrongNonceStage = $stage.PSObject.Copy()
    $wrongNonceStage.GuardNonce = [Guid]::NewGuid().ToString('N')
    Assert-Rejected -Label 'wrong stage guard nonce' -Action {
        Assert-PartisanCandidateStage -Context $context -Stage $wrongNonceStage
    }
    $externalStage = $stage.PSObject.Copy()
    Assert-Rejected -Label 'byte-exact external stage object clone' -Action {
        Assert-PartisanCandidateStage -Context $context -Stage $externalStage
    }
    Assert-Condition `
        -Condition (Test-Path -LiteralPath $guardDirectory -PathType Container) `
        -Label 'stage clone rejection retained real guard'

    $frozenBinding = [string]$context.CandidateBindingSha256
    $candidate.CandidateId = 'mutated-after-stage'
    $candidate.RuntimeAddonRootPath = Join-Path $tempRoot 'mutated-path'
    [void](Assert-PartisanCandidateStage -Context $context -Stage $stage)
    Assert-Condition `
        -Condition ([string]$context.CandidateBindingSha256 -ceq $frozenBinding) `
        -Label 'original candidate mutation cannot alter frozen binding'
    [void]$checks.Add('candidate-context-deep-binding')

    $shellExecutable = (Get-Process -Id $PID -ErrorAction Stop).Path
    $shellProvenance = Get-PartisanExecutableProvenance -Path $shellExecutable
    $wrongShellProvenance = $shellProvenance.PSObject.Copy()
    $wrongShellProvenance.sha256 = '0' * 64
    Assert-Rejected -Label 'launch with unsealed executable provenance' -Action {
        Start-PartisanGuardedServer `
            -Context $context `
            -Executable $shellExecutable `
            -ExecutableProvenance $wrongShellProvenance `
            -Arguments @('-NoLogo', '-Command', 'exit 0') `
            -WorkingDirectory $tempRoot `
            -NonEngineSelfTestOnly
    }
    Assert-Rejected -Label 'non-engine launch without explicit self-test bypass' -Action {
        Start-PartisanGuardedServer `
            -Context $context `
            -Executable $shellExecutable `
            -ExecutableProvenance $shellProvenance `
            -Arguments @('-NoLogo', '-Command', 'exit 0') `
            -WorkingDirectory $tempRoot
    }
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments @('-NoLogo', '-NoProfile', '-NonInteractive', '-Command',
            'Start-Sleep -Seconds 30') `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly

    $liveStatus = Get-PartisanProcessIdentityStatus `
        -Identity $serverLaunch.RootIdentity
    $wrongLiveIdentity = $serverLaunch.RootIdentity.PSObject.Copy()
    $wrongLiveIdentity.ExecutablePath =
        [string]$wrongLiveIdentity.ExecutablePath + '.wrong'
    $unknownStatus = Get-PartisanProcessIdentityStatus `
        -Identity $wrongLiveIdentity
    Assert-Condition `
        -Condition ([string]$liveStatus.Status -ceq 'alive' -and
            [string]$unknownStatus.Status -ceq 'unknown') `
        -Label 'live identity mismatch is unknown rather than dead'

    $argvScript = Join-Path $tempRoot 'echo-argv.ps1'
    $argvOutput = Join-Path $tempRoot 'echo-argv.json'
    [IO.File]::WriteAllText($argvScript, @'
param(
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Values)
$nativeArguments = [Environment]::GetCommandLineArgs()
$outputIndex = [Array]::IndexOf($nativeArguments, $OutputPath)
if ($outputIndex -lt 0) {
    throw 'The argv output marker was not present in the native argument vector.'
}
$Values = @($nativeArguments[($outputIndex + 1)..($nativeArguments.Length - 1)])
[IO.File]::WriteAllText(
    $OutputPath,
    (ConvertTo-Json -InputObject ([string[]]$Values) -Compress),
    (New-Object Text.UTF8Encoding($false)))
Start-Sleep -Seconds 2
'@)
    $expectedArgv = @('', 'two words', ' trail and tail ', 'trail\', 'quote"inside')
    $argvClient = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments (@('-NoLogo', '-NoProfile', '-NonInteractive', '-File',
            $argvScript, $argvOutput) + $expectedArgv) `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    [void](Wait-PartisanGuardedProcess `
        -Context $context `
        -Launch $argvClient `
        -TimeoutSeconds 10 `
        -RequireZeroExit)
    $actualArgv = [string[]](ConvertFrom-Json `
        -InputObject ([IO.File]::ReadAllText($argvOutput)))
    $argvDifference = @(Compare-Object `
            -ReferenceObject $expectedArgv `
            -DifferenceObject $actualArgv `
            -CaseSensitive `
            -SyncWindow 0)
    if ($argvDifference.Count -ne 0) {
        throw ('Real child argv mismatch; expected=' +
            ($expectedArgv | ConvertTo-Json -Compress) + ';actual=' +
            ($actualArgv | ConvertTo-Json -Compress))
    }
    Assert-Condition `
        -Condition ($argvDifference.Count -eq 0) `
        -Label 'real child argv preserves exact edge cases'

    $longClient = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments @('-NoLogo', '-NoProfile', '-NonInteractive', '-Command',
            'Start-Sleep -Seconds 30') `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    Assert-Rejected -Label 'server stop before live client' -Action {
        Stop-PartisanGuardedProcess -Context $context -Launch $serverLaunch
    }
    [void](Assert-PartisanRuntimeOwnershipAudit -Context $context)
    [void](Stop-PartisanGuardedProcess -Context $context -Launch $longClient)
    [void](Stop-PartisanGuardedProcess -Context $context -Launch $serverLaunch)
    $teardown = Invoke-PartisanGuardedTeardown -Context $context
    $context = $null
    Assert-Condition `
        -Condition ($teardown.ClientsStoppedBeforeServer -and
            $teardown.JobClosed -and
            $teardown.GuardRemoved -and
            $teardown.StageRemoved -and
            $teardown.IPv4LoopbackPortsBindAvailable -and
            $teardown.PortSemantics -ceq 'availability-only-no-pid-attribution' -and
            $teardown.BoundariesClean) `
        -Label 'clean guarded context exact teardown'
    [void]$checks.Add('non-engine-process-lifecycle-and-real-argv')

    $freshCandidate = [pscustomobject][ordered]@{
        CandidateId = 'self-test-candidate'
        PackageSha256 = Get-TextSha256 -Text $canonicalIndexText
        PackageFiles = $packageRows.ToArray()
        PackedAddonPath = $candidateSource
        RuntimeAddonRootPath = $runtimeAddon
    }
    $poisonContext = New-PartisanGuardedRuntimeContext `
        -GuardBase $contextGuardBase `
        -Purpose 'self_test_poison' `
        -WatchedRoots @($contextWatched) `
        -SpillRoots @($contextSpill) `
        -LoopbackPorts @((Get-FreeUdpPort))
    $poisonStage = New-PartisanCandidateStage `
        -Context $poisonContext `
        -Candidate $freshCandidate
    $stagedData = Join-Path $poisonStage.PackedAddonPath 'data.pak'
    $boundaryTamper = Join-Path $contextWatched 'boundary-tamper.txt'
    [IO.File]::WriteAllText($stagedData, 'tampered')
    [IO.File]::WriteAllText($boundaryTamper, 'tampered')
    Assert-Rejected -Label 'tampered stage and boundary teardown' -Action {
        Invoke-PartisanGuardedTeardown -Context $poisonContext
    }
    Assert-Condition `
        -Condition ($poisonContext.CertificationInvalidated -and
            (Test-Path -LiteralPath $poisonContext.Guard.Directory) -and
            (Test-Path -LiteralPath $poisonContext.PermanentFailureLedgerPath)) `
        -Label 'teardown failure latched durable permanent no-go'
    Copy-Item `
        -LiteralPath (Join-Path $candidateSource 'data.pak') `
        -Destination $stagedData `
        -Force
    Remove-Item -LiteralPath $boundaryTamper -Force
    [void](Assert-PartisanCandidateStage `
        -Context $poisonContext `
        -Stage $poisonStage)
    Assert-Rejected -Label 'historical no-go survives restored retry' -Action {
        Invoke-PartisanGuardedTeardown -Context $poisonContext
    }
    $failureLedger = Read-PartisanPortableJson `
        -Path $poisonContext.PermanentFailureLedgerPath
    Assert-Condition `
        -Condition ([string]$failureLedger.certification -ceq 'permanent-no-go' -and
            @($failureLedger.failures).Count -ge 2 -and
            (Test-Path -LiteralPath $poisonContext.Guard.Directory)) `
        -Label 'restored retry retained historical evidence and guard'
    [void](Remove-PartisanGuardDirectory -Ownership $poisonContext.Guard)
    $poisonContext = $null
    [void]$checks.Add('durable-no-go-across-retry')

    $context = New-PartisanGuardedRuntimeContext `
        -GuardBase $contextGuardBase `
        -Purpose 'self_test_unknown' `
        -WatchedRoots @($contextWatched) `
        -SpillRoots @($contextSpill) `
        -LoopbackPorts @((Get-FreeUdpPort))
    [void](New-PartisanCandidateStage -Context $context -Candidate $freshCandidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments @('-NoLogo', '-NoProfile', '-NonInteractive', '-Command',
            'Start-Sleep -Seconds 30') `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $serverLedger = @($context.Ledger | Where-Object {
        [string]$_.RootRole -ceq 'server'
    })[0]
    $originalLedgerExecutable = [string]$serverLedger.Identity.ExecutablePath
    $serverLedger.Identity.ExecutablePath = $originalLedgerExecutable + '.wrong'
    Assert-Rejected -Label 'live ledger inspection mismatch' -Action {
        Assert-PartisanRuntimeOwnershipAudit -Context $context
    }
    $serverLedger.Identity.ExecutablePath = $originalLedgerExecutable
    Assert-Condition `
        -Condition ($context.CertificationInvalidated -and
            $context.State -ceq 'teardown-failed') `
        -Label 'unknown live ledger identity poisoned context'
    Assert-Rejected -Label 'poisoned context rejects launch retry' -Action {
        Start-PartisanGuardedClient `
            -Context $context `
            -Executable $shellExecutable `
            -ExecutableProvenance $shellProvenance `
            -Arguments @('-NoLogo', '-Command', 'exit 0') `
            -WorkingDirectory $tempRoot `
            -NonEngineSelfTestOnly
    }
    Assert-Rejected -Label 'poisoned identity teardown retains evidence' -Action {
        Invoke-PartisanGuardedTeardown -Context $context
    }
    Assert-Condition `
        -Condition ((Get-PartisanProcessIdentityStatus `
            -Identity $serverLaunch.RootIdentity).Status -ceq 'dead') `
        -Label 'poisoned identity teardown still stopped exact server'
    [void](Remove-PartisanGuardDirectory -Ownership $context.Guard)
    $context = $null
    [void]$checks.Add('tri-state-ledger-unknown')

    $context = New-PartisanGuardedRuntimeContext `
        -GuardBase $contextGuardBase `
        -Purpose 'self_test_post_resume' `
        -WatchedRoots @($contextWatched) `
        -SpillRoots @($contextSpill) `
        -LoopbackPorts @((Get-FreeUdpPort))
    [void](New-PartisanCandidateStage -Context $context -Candidate $freshCandidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments @('-NoLogo', '-NoProfile', '-NonInteractive', '-Command',
            'Start-Sleep -Seconds 30') `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $sleepScript = Join-Path $tempRoot 'descendant-sleep.ps1'
    $spawnScript = Join-Path $tempRoot 'spawn-descendant.ps1'
    $descendantMarker = Join-Path $tempRoot 'descendant.pid'
    [IO.File]::WriteAllText($sleepScript, 'Start-Sleep -Seconds 30')
    [IO.File]::WriteAllText($spawnScript, @'
param([string]$Shell, [string]$SleepScript, [string]$Marker)
$child = Start-Process -FilePath $Shell -ArgumentList @(
    '-NoLogo', '-NoProfile', '-NonInteractive', '-File', $SleepScript) -PassThru
[IO.File]::WriteAllText($Marker, [string]$child.Id)
Start-Sleep -Seconds 30
'@)
    Assert-Rejected -Label 'post-resume failure injection' -Action {
        Start-PartisanGuardedClient `
            -Context $context `
            -Executable $shellExecutable `
            -ExecutableProvenance $shellProvenance `
            -Arguments @('-NoLogo', '-NoProfile', '-NonInteractive', '-File',
                $spawnScript, $shellExecutable, $sleepScript, $descendantMarker) `
            -WorkingDirectory $tempRoot `
            -NonEngineSelfTestOnly `
            -NonEngineSelfTestForcePostResumeFailure
    }
    Assert-Condition `
        -Condition ($context.CertificationInvalidated -and
            (Test-Path -LiteralPath $descendantMarker) -and
            @($context.Ledger | Where-Object {
                [string]$_.RootRole -ceq 'client-1' -and
                (Get-PartisanProcessIdentityStatus `
                    -Identity $_.Identity).Status -cne 'dead'
            }).Count -eq 0) `
        -Label 'post-resume failure poisoned and killed exact descendant role'
    Assert-Rejected -Label 'post-resume no-go teardown' -Action {
        Invoke-PartisanGuardedTeardown -Context $context
    }
    [void](Remove-PartisanGuardDirectory -Ownership $context.Guard)
    $context = $null
    [void]$checks.Add('post-resume-poison-and-descendant-cleanup')

    $abruptRootScript = Join-Path $tempRoot 'abrupt-job-root.ps1'
    $abruptOwnerScript = Join-Path $tempRoot 'abrupt-job-owner.ps1'
    $abruptPidMarker = Join-Path $tempRoot 'abrupt-job-pids.txt'
    $abruptReadyMarker = Join-Path $tempRoot 'abrupt-job-ready.txt'
    $abruptExitMarker = Join-Path $tempRoot 'abrupt-job-exit.txt'
    [IO.File]::WriteAllText($abruptRootScript, @'
param([string]$Shell, [string]$SleepScript, [string]$PidMarker)
$child = Start-Process -FilePath $Shell -ArgumentList @(
    '-NoLogo', '-NoProfile', '-NonInteractive', '-File', $SleepScript) -PassThru
[IO.File]::WriteAllText($PidMarker, ([string]$PID + ',' + [string]$child.Id))
Start-Sleep -Seconds 30
'@)
    [IO.File]::WriteAllText($abruptOwnerScript, @'
Import-Module -Name $env:PARTISAN_SELFTEST_MODULE -Force -ErrorAction Stop
$job = New-Object Partisan.GuardedRuntime.NativeJob
$arguments = @(
    '-NoLogo', '-NoProfile', '-NonInteractive', '-File',
    $env:PARTISAN_SELFTEST_ROOT_SCRIPT,
    $env:PARTISAN_SELFTEST_SHELL,
    $env:PARTISAN_SELFTEST_SLEEP_SCRIPT,
    $env:PARTISAN_SELFTEST_PID_MARKER)
$commandLine = ConvertTo-PartisanNativeCommandLine `
    -Executable $env:PARTISAN_SELFTEST_SHELL `
    -Arguments $arguments
$suspended = New-Object Partisan.GuardedRuntime.SuspendedProcess(
    $env:PARTISAN_SELFTEST_SHELL,
    $commandLine,
    $env:PARTISAN_SELFTEST_WORKING)
$job.Add($suspended.Child)
$suspended.Resume()
$deadline = [DateTime]::UtcNow.AddSeconds(10)
while (-not (Test-Path -LiteralPath $env:PARTISAN_SELFTEST_PID_MARKER)) {
    if ([DateTime]::UtcNow -ge $deadline) {
        throw 'The abrupt-owner child marker did not appear.'
    }
    Start-Sleep -Milliseconds 50
}
[IO.File]::WriteAllText($env:PARTISAN_SELFTEST_READY_MARKER, 'ready')
while (-not (Test-Path -LiteralPath $env:PARTISAN_SELFTEST_EXIT_MARKER)) {
    if ([DateTime]::UtcNow -ge $deadline) {
        throw 'The abrupt-owner exit signal did not appear.'
    }
    Start-Sleep -Milliseconds 50
}
[Environment]::Exit(0)
'@)
    $env:PARTISAN_SELFTEST_MODULE = $modulePath
    $env:PARTISAN_SELFTEST_ROOT_SCRIPT = $abruptRootScript
    $env:PARTISAN_SELFTEST_SHELL = $shellExecutable
    $env:PARTISAN_SELFTEST_SLEEP_SCRIPT = $sleepScript
    $env:PARTISAN_SELFTEST_PID_MARKER = $abruptPidMarker
    $env:PARTISAN_SELFTEST_READY_MARKER = $abruptReadyMarker
    $env:PARTISAN_SELFTEST_EXIT_MARKER = $abruptExitMarker
    $env:PARTISAN_SELFTEST_WORKING = $tempRoot
    $abruptOwner = Start-Process `
        -FilePath $shellExecutable `
        -ArgumentList @(
            '-NoLogo', '-NoProfile', '-NonInteractive', '-File',
            $abruptOwnerScript) `
        -PassThru
    $abruptDeadline = [DateTime]::UtcNow.AddSeconds(10)
    while (-not (Test-Path -LiteralPath $abruptReadyMarker)) {
        if ($abruptOwner.HasExited) {
            throw 'The abrupt job owner exited before its children were observable.'
        }
        if ([DateTime]::UtcNow -ge $abruptDeadline) {
            throw 'The abrupt job owner did not reach its ready barrier.'
        }
        Start-Sleep -Milliseconds 50
        $abruptOwner.Refresh()
    }
    $abruptPids = @([IO.File]::ReadAllText($abruptPidMarker).Split(',') |
        ForEach-Object { [int]$_ })
    Assert-Condition `
        -Condition ($abruptPids.Count -eq 2) `
        -Label 'abrupt job root and descendant marker'
    $abruptIdentities = @($abruptPids | ForEach-Object {
        Get-PartisanProcessIdentity -ProcessId $_
    })
    [IO.File]::WriteAllText($abruptExitMarker, 'exit')
    Assert-Condition `
        -Condition ($abruptOwner.WaitForExit(10000) -and
            $abruptOwner.ExitCode -eq 0) `
        -Label 'abrupt job owner exit'
    $abruptDeadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        $abruptStatuses = @($abruptIdentities | ForEach-Object {
            Get-PartisanProcessIdentityStatus -Identity $_
        })
        if (@($abruptStatuses | Where-Object {
                [string]$_.Status -cne 'dead'
            }).Count -eq 0) {
            break
        }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $abruptDeadline)
    Assert-Condition `
        -Condition (@($abruptStatuses | Where-Object {
            [string]$_.Status -cne 'dead'
        }).Count -eq 0) `
        -Label 'kill-on-close stopped live root and descendant after abrupt owner exit'
    foreach ($name in @(
            'PARTISAN_SELFTEST_MODULE',
            'PARTISAN_SELFTEST_ROOT_SCRIPT',
            'PARTISAN_SELFTEST_SHELL',
            'PARTISAN_SELFTEST_SLEEP_SCRIPT',
            'PARTISAN_SELFTEST_PID_MARKER',
            'PARTISAN_SELFTEST_READY_MARKER',
            'PARTISAN_SELFTEST_EXIT_MARKER',
            'PARTISAN_SELFTEST_WORKING')) {
        Remove-Item -LiteralPath ('Env:' + $name) -ErrorAction Stop
    }
    [void]$checks.Add('abrupt-owner-kill-on-close-descendants')
    }

    $engineNames = @(
        'ArmaReforger', 'ArmaReforger_BE', 'ArmaReforgerSteam',
        'ArmaReforgerSteamDiag', 'ArmaReforgerServer',
        'ArmaReforgerServerDiag', 'ArmaReforgerWorkbench',
        'ArmaReforgerWorkbenchDiag', 'ArmaReforgerWorkbenchSteamDiag',
        'CrashReporter', 'CrashReportClient')
    Assert-Condition `
        -Condition (@(Get-Process -ErrorAction Stop | Where-Object {
            $engineNames -contains $_.ProcessName
        }).Count -eq 0) `
        -Label 'authoritative runtime engine-free preflight'

    $contextGuardBase = Join-Path $tempRoot 'context-guard'
    $contextWatched = Join-Path $tempRoot 'context-watched'
    $contextSpill = Join-Path $tempRoot 'context-spill'
    $candidateSource = Join-Path $tempRoot 'candidate-source'
    $runtimeAddon = Join-Path $tempRoot 'runtime-addons'
    foreach ($directory in @(
            $contextGuardBase,
            $contextWatched,
            $contextSpill,
            $candidateSource,
            $runtimeAddon)) {
        New-Item -ItemType Directory -Path $directory | Out-Null
    }
    Assert-Rejected `
        -Label 'context creation cleanup failure injection' `
        -ExpectedIdentifier 'PGR_SELFTEST_CREATE_FAILURE' `
        -Action { New-SelfTestRuntimeContext `
            -GuardBase $contextGuardBase `
            -WatchedRoot $contextWatched `
            -SpillRoot $contextSpill `
            -Purpose 'self_test_create_cleanup_failure' `
            -Faults @('create-cleanup-failure') }
    $creationFaultJournals = @(Get-ChildItem `
        -LiteralPath $contextGuardBase `
        -File `
        -Filter '.PartisanGuardedRuntime_*.journal-fault.json' `
        -ErrorAction Stop)
    Assert-Condition `
        -Condition ($creationFaultJournals.Count -eq 1) `
        -Label 'failed context creation emitted one external fault journal'
    $creationFault = Read-PartisanPortableJson `
        -Path $creationFaultJournals[0].FullName
    $creationFaultGuard = Join-Path `
        $contextGuardBase `
        ('PartisanGuardedRuntime_' + [string]$creationFault.nonce)
    Assert-Condition `
        -Condition ([string]$creationFault.failure -ceq
                'PGR_CREATE_CLEANUP_FAILED' -and
            [string]$creationFault.mode -ceq 'permanent-no-go' -and
            (Test-Path -LiteralPath $creationFaultGuard -PathType Container) -and
            -not (Test-Path -LiteralPath (Join-Path `
                $creationFaultGuard `
                '.partisan-guarded-runtime-owner.json'))) `
        -Label 'failed context cleanup retained partial guard and durable no-go'
    Assert-Rejected `
        -Label 'failed creation guard remains registry protected' `
        -ExpectedIdentifier 'PGR_REGISTERED_GUARD_PROTECTED' `
        -Action { Remove-PartisanGuardDirectory `
            -Ownership ([pscustomobject]@{ Directory = $creationFaultGuard }) }
    [void]$checks.Add('authoritative-create-cleanup-failure-journal')
    $candidateContents = [ordered]@{
        'addon.gproj' = 'project-fixture'
        'data.pak' = 'package-fixture'
    }
    $packageRows = New-Object Collections.Generic.List[object]
    foreach ($name in $candidateContents.Keys) {
        $path = Join-Path $candidateSource $name
        [IO.File]::WriteAllText($path, [string]$candidateContents[$name])
        $signature = Get-PartisanFileSignature -Path $path
        [void]$packageRows.Add([pscustomobject][ordered]@{
            indexPath = 'Partisan/' + $name
            length = [long]$signature.length
            sha256 = [string]$signature.sha256
        })
    }
    $indexLines = @($packageRows.ToArray() | Sort-Object indexPath |
        ForEach-Object {
            "{0}`t{1}`t{2}" -f $_.sha256, $_.length, $_.indexPath
        })
    $canonicalIndexText = ($indexLines -join "`n") + "`n"
    $packageSha = Get-TextSha256 -Text $canonicalIndexText
    $shellExecutable = (Get-Process -Id $PID -ErrorAction Stop).Path
    $shellProvenance = Get-PartisanExecutableProvenance -Path $shellExecutable
    $longArguments = @(
        '-NoLogo', '-NoProfile', '-NonInteractive', '-Command',
        'Start-Sleep -Seconds 30')

    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_frozen_candidate'
    $stage = New-PartisanCandidateStage -Context $context -Candidate $candidate
    $runtimeModule = Get-Module -Name 'Partisan.GuardedRuntime' -ErrorAction Stop
    $candidateArguments = [string[]]@(
        '-addonsDir', $stage.AddonSearchPath,
        '-gproj', $stage.PackedProjectPath,
        '-hstReleaseCandidateId', $candidate.CandidateId,
        '-hstReleasePackageSha256', $candidate.PackageSha256)
    $candidateConsumptionEvidence = & $runtimeModule {
        param($ContextValue, [string[]]$ArgumentValue)
        $privateRecord = Find-PartisanV2Record -Context $ContextValue
        Get-PartisanV2CandidateConsumptionEvidence `
            -Record $privateRecord `
            -Arguments $ArgumentValue `
            -IsEngine $true
    } $context $candidateArguments
    Assert-Condition `
        -Condition ($candidateConsumptionEvidence.mode -ceq
                'exact-stage-and-candidate-argv-enforced' -and
            @($candidateConsumptionEvidence.exactArgumentPairs).Count -eq 4) `
        -Label 'engine candidate consumption binds exact stage and candidate argv'
    Assert-Rejected `
        -Label 'engine candidate consumption rejects missing argv pair' `
        -ExpectedIdentifier 'PGR_ENGINE_CANDIDATE_ARGV' `
        -Action {
            & $runtimeModule {
                param($ContextValue, [string[]]$ArgumentValue)
                $privateRecord = Find-PartisanV2Record -Context $ContextValue
                Get-PartisanV2CandidateConsumptionEvidence `
                    -Record $privateRecord `
                    -Arguments $ArgumentValue `
                    -IsEngine $true
            } $context ([string[]]@(
                '-addonsDir', $stage.AddonSearchPath,
                '-gproj', $stage.PackedProjectPath,
                '-hstReleaseCandidateId', $candidate.CandidateId))
        }
    foreach ($argvCase in @(
            [pscustomobject]@{
                Label = 'wrong-case addons option'
                Arguments = [string[]]@(
                    '-ADDONSDIR', $stage.AddonSearchPath,
                    '-gproj', $stage.PackedProjectPath,
                    '-hstReleaseCandidateId', $candidate.CandidateId,
                    '-hstReleasePackageSha256', $candidate.PackageSha256)
                Identifier = 'PGR_ENGINE_CANDIDATE_ARGV_FORM'
            },
            [pscustomobject]@{
                Label = 'equals-form project option'
                Arguments = [string[]]@(
                    '-addonsDir', $stage.AddonSearchPath,
                    ('-GPROJ=' + $stage.PackedProjectPath),
                    '-hstReleaseCandidateId', $candidate.CandidateId,
                    '-hstReleasePackageSha256', $candidate.PackageSha256)
                Identifier = 'PGR_ENGINE_CANDIDATE_ARGV_FORM'
            },
            [pscustomobject]@{
                Label = 'duplicate exact addons option'
                Arguments = [string[]]@($candidateArguments + @(
                    '-addonsDir', $stage.AddonSearchPath))
                Identifier = 'PGR_ENGINE_CANDIDATE_ARGV'
            },
            [pscustomobject]@{
                Label = 'conflicting equals-form addons option'
                Arguments = [string[]]@($candidateArguments + @(
                    ('-addonsDir=' + $stage.AddonSearchPath)))
                Identifier = 'PGR_ENGINE_CANDIDATE_ARGV_FORM'
            })) {
        Assert-Rejected `
            -Label ([string]$argvCase.Label) `
            -ExpectedIdentifier ([string]$argvCase.Identifier) `
            -Action {
                & $runtimeModule {
                    param($ContextValue, [string[]]$ArgumentValue)
                    $privateRecord = Find-PartisanV2Record -Context $ContextValue
                    Get-PartisanV2CandidateConsumptionEvidence `
                        -Record $privateRecord `
                        -Arguments $ArgumentValue `
                        -IsEngine $true
                } $context ([string[]]$argvCase.Arguments)
            }
    }
    $candidate.CandidateId = 'mutated-input'
    $candidate.PackageFiles[0].sha256 = '0' * 64
    $candidate.PackageFiles = @()
    $candidate.RuntimeAddonRootPath = Join-Path $tempRoot 'mutated-runtime'
    [void](Assert-PartisanCandidateStage -Context $context -Stage $stage)
    $clean = Invoke-PartisanGuardedTeardown -Context $context
    $receiptStatus = Test-PartisanGuardedRuntimeReceipt `
        -Path $clean.CleanReceiptPath `
        -ExpectedSignature $clean.CleanReceiptSignature
    Assert-Condition `
        -Condition ($clean.State -ceq 'closed' -and
            $clean.GuardRemovalWasFinalAction -and
            (Test-Path -LiteralPath $clean.CleanReceiptPath -PathType Leaf) -and
            $receiptStatus.Status -ceq 'complete' -and
            $receiptStatus.GuardDirectoryAbsent -and
            $receiptStatus.CompletionAttestationExact) `
        -Label 'nested candidate mutation cannot alter private sealed binding'
    Assert-Rejected `
        -Label 'receipt verifier requires trusted expected signature' `
        -ExpectedIdentifier 'PGR_EXPECTED_RECEIPT_SIGNATURE_REQUIRED' `
        -Action { Test-PartisanGuardedRuntimeReceipt `
            -Path $clean.CleanReceiptPath `
            -ExpectedSignature $null }
    $context = $null
    [void]$checks.Add('authoritative-deep-sealed-candidate')
    [void]$checks.Add('authoritative-engine-candidate-argv-correlation')
    [void]$checks.Add('authoritative-receipt-trusted-signature-attestation')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_reload_registry'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    $stage = New-PartisanCandidateStage -Context $context -Candidate $candidate
    Import-Module -Name $modulePath -Force -ErrorAction Stop
    $runtimeModule = Get-Module -Name 'Partisan.GuardedRuntime' -ErrorAction Stop
    Assert-Rejected `
        -Label 'module reload preserves failed-creation guard protection' `
        -ExpectedIdentifier 'PGR_REGISTERED_GUARD_PROTECTED' `
        -Action { Remove-PartisanGuardDirectory `
            -Ownership ([pscustomobject]@{ Directory = $creationFaultGuard }) }
    [void](Assert-PartisanCandidateStage -Context $context -Stage $stage)
    Assert-Rejected `
        -Label 'module reload preserves direct guard protection' `
        -ExpectedIdentifier 'PGR_REGISTERED_GUARD_PROTECTED' `
        -Action { Remove-PartisanGuardDirectory -Ownership $context.Guard }
    $reloadClean = Invoke-PartisanGuardedTeardown -Context $context
    $reloadReceipt = Test-PartisanGuardedRuntimeReceipt `
        -Path $reloadClean.CleanReceiptPath `
        -ExpectedSignature $reloadClean.CleanReceiptSignature
    Assert-Condition `
        -Condition ($reloadReceipt.Complete -and
            $reloadReceipt.CompletionAttestationExact) `
        -Label 'module reload retained durable private ownership and recovery'
    $context = $null
    [void]$checks.Add('authoritative-module-reload-registry-protection')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_launch_reference'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments @(
            '-NoLogo', '-NoProfile', '-NonInteractive', '-Command',
            'Start-Sleep -Milliseconds 500') `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    Assert-Condition `
        -Condition ([object]::ReferenceEquals($serverLaunch, $context.Server)) `
        -Label 'server launch projection exact reference at start'
    [void](Wait-PartisanGuardedProcess `
        -Context $context `
        -Launch $serverLaunch `
        -TimeoutSeconds 10 `
        -PollMilliseconds 50 `
        -RequireZeroExit)
    Assert-Condition `
        -Condition ([object]::ReferenceEquals($serverLaunch, $context.Server)) `
        -Label 'wait preserves exact launch projection reference'
    [void](Stop-PartisanGuardedProcess `
        -Context $context `
        -Launch $serverLaunch)
    Assert-Condition `
        -Condition ([object]::ReferenceEquals($serverLaunch, $context.Server)) `
        -Label 'stop preserves exact launch projection reference'
    [void](Invoke-PartisanGuardedTeardown -Context $context)
    $context = $null
    [void]$checks.Add('authoritative-launch-reference-persistence')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_launch_clone'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $launchClone = $serverLaunch.PSObject.Copy()
    Assert-Rejected `
        -Label 'equal-value launch clone rejected' `
        -ExpectedIdentifier 'PGR_LAUNCH_REFERENCE_MISMATCH' `
        -Action { Stop-PartisanGuardedProcess `
            -Context $context `
            -Launch $launchClone }
    Assert-Rejected `
        -Label 'launch-clone no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ($context.CertificationInvalidated -and
            $context.JobClosed -and
            (Get-PartisanProcessIdentityStatus `
                -Identity $serverLaunch.RootIdentity).Status -ceq 'dead') `
        -Label 'launch clone latched no-go and retained private ownership'
    $context = $null
    [void]$checks.Add('authoritative-launch-clone-rejection')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_stage_required'
    Assert-Rejected `
        -Label 'teardown requires an explicit candidate stage' `
        -ExpectedIdentifier 'PGR_STAGE_MISSING' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Rejected `
        -Label 'missing-stage no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ($context.CertificationInvalidated -and
            $context.JobClosed -and
            (Test-Path -LiteralPath $context.Guard.Directory)) `
        -Label 'missing stage latched and retained its registered guard'
    $context = $null
    [void]$checks.Add('authoritative-stage-mandatory')

    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_context_clone_all_apis'
    $stage = New-PartisanCandidateStage -Context $context -Candidate $candidate
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $clientOne = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $clientTwo = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $contextClone = $context.PSObject.Copy()
    $cloneCalls = @(
        { Assert-PartisanRuntimeOwnershipAudit -Context $contextClone },
        { New-PartisanCandidateStage -Context $contextClone -Candidate $candidate },
        { Assert-PartisanCandidateStage -Context $contextClone -Stage $stage },
        { Start-PartisanGuardedServer `
                -Context $contextClone `
                -Executable $shellExecutable `
                -ExecutableProvenance $shellProvenance `
                -Arguments $longArguments `
                -WorkingDirectory $tempRoot `
                -NonEngineSelfTestOnly },
        { Start-PartisanGuardedClient `
                -Context $contextClone `
                -Executable $shellExecutable `
                -ExecutableProvenance $shellProvenance `
                -Arguments $longArguments `
                -WorkingDirectory $tempRoot `
                -NonEngineSelfTestOnly },
        { Wait-PartisanGuardedProcess `
                -Context $contextClone `
                -Launch $clientOne `
                -TimeoutSeconds 1 },
        { Stop-PartisanGuardedProcess `
                -Context $contextClone `
                -Launch $clientOne },
        { Invoke-PartisanGuardedTeardown -Context $contextClone })
    foreach ($call in $cloneCalls) {
        Assert-Rejected `
            -Label 'context clone rejected by every context API' `
            -ExpectedIdentifier 'PGR_CONTEXT_REFERENCE_MISMATCH' `
            -Action $call
    }
    $contextClone.State = 'active'
    $contextClone.CertificationInvalidated = $false
    $contextClone.FailureJournalPath = $null
    Assert-Rejected `
        -Label 'pre-poison clone cannot erase private no-go' `
        -ExpectedIdentifier 'PGR_CONTEXT_REFERENCE_MISMATCH' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $contextClone }
    Assert-Condition `
        -Condition ($context.State -ceq 'permanent-no-go' -and
            $context.CertificationInvalidated -and
            (Test-Path -LiteralPath $context.FailureJournalPath)) `
        -Label 'context clone poisoning persisted on authoritative context'
    Assert-Rejected `
        -Label 'authoritative no-go teardown retains evidence' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ($context.JobClosed -and
            (Get-PartisanProcessIdentityStatus `
                -Identity $serverLaunch.RootIdentity).Status -ceq 'dead' -and
            (Get-PartisanProcessIdentityStatus `
                -Identity $clientOne.RootIdentity).Status -ceq 'dead' -and
            (Get-PartisanProcessIdentityStatus `
                -Identity $clientTwo.RootIdentity).Status -ceq 'dead' -and
            (Test-Path -LiteralPath $context.Guard.Directory)) `
        -Label 'no-go cleanup stopped processes but retained guard'
    Assert-Rejected `
        -Label 'registered no-go guard remover blocked' `
        -ExpectedIdentifier 'PGR_REGISTERED_GUARD_PROTECTED' `
        -Action { Remove-PartisanGuardDirectory -Ownership $context.Guard }
    $context = $null
    [void]$checks.Add('authoritative-context-reference-all-apis')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_stage_clones'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    $stage = New-PartisanCandidateStage -Context $context -Candidate $candidate
    $stageClone = $stage.PSObject.Copy()
    Assert-Rejected `
        -Label 'exact context plus stage clone' `
        -ExpectedIdentifier 'PGR_STAGE_REFERENCE_MISMATCH' `
        -Action { Assert-PartisanCandidateStage `
            -Context $context `
            -Stage $stageClone }
    $contextStageClone = $context.PSObject.Copy()
    $contextStageClone.Stage = $stageClone
    Assert-Rejected `
        -Label 'context and stage clone pair' `
        -ExpectedIdentifier 'PGR_CONTEXT_REFERENCE_MISMATCH' `
        -Action { Assert-PartisanCandidateStage `
            -Context $contextStageClone `
            -Stage $stageClone }
    Assert-Rejected `
        -Label 'stage clone no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-context-stage-clones')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_projection_mutations'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    $stage = New-PartisanCandidateStage -Context $context -Candidate $candidate
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $clientOne = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $mutations = @(
        { $context.CertificationInvalidated = $true },
        { $context.Guard = $context.Guard.PSObject.Copy() },
        { $context.Job = $context.Job.PSObject.Copy() },
        { $context.Ledger = @() },
        { $context.Stage = $context.Stage.PSObject.Copy() },
        { $context.Server = $context.Server.PSObject.Copy() },
        { $context.Clients = @() },
        { $context.BoundarySnapshots = @() },
        { $context.PortAvailabilityChecks = @() },
        { $context.FailureJournalSignature = [pscustomobject]@{
                length = 1; sha256 = ('0' * 64) } },
        { $context.Server.Process = [pscustomobject]@{ ProcessId = 1 } },
        { $context.Server.RootIdentity = [pscustomobject]@{ ProcessId = 1 } },
        { $context.Server.Arguments = [string[]]$context.Server.Arguments.Clone() },
        { $context.Server.CandidateConsumptionEvidence =
                $context.Server.CandidateConsumptionEvidence.PSObject.Copy() },
        { $context.Server.ExecutableProvenance = [pscustomobject]@{
                sha256 = ('0' * 64) } })
    foreach ($mutation in $mutations) {
        & $mutation
        Assert-Rejected `
            -Label 'public field or nested projection mutation' `
            -ExpectedIdentifier 'PGR_PUBLIC_PROJECTION_MUTATED' `
            -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    }
    Assert-Rejected `
        -Label 'projection mutation no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-public-projection-mutations')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_ordered_teardown'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $clientOne = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $clientTwo = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $ordered = Invoke-PartisanGuardedTeardown -Context $context
    $orderedRoles = [string[]]$ordered.TeardownStopOrder
    $clientTwoIndex = [array]::IndexOf($orderedRoles, 'client-2')
    $clientOneIndex = [array]::IndexOf($orderedRoles, 'client-1')
    $serverIndex = [array]::IndexOf($orderedRoles, 'server')
    $serverDescendantIndex = [array]::IndexOf(
        $orderedRoles,
        'server-descendant')
    Assert-Condition `
        -Condition ($clientTwoIndex -ge 0 -and
            $clientTwoIndex -lt $clientOneIndex -and
            $clientOneIndex -lt $serverIndex -and
            ($serverDescendantIndex -lt 0 -or
                $serverDescendantIndex -lt $serverIndex) -and
            $ordered.JobClosed -and
            $ordered.State -ceq 'closed') `
        -Label ('actual server and two clients stopped in role-safe order; order=' +
            ($orderedRoles -join ','))
    $context = $null
    [void]$checks.Add('authoritative-two-client-ordered-teardown')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_client_barrier' `
        -Faults @('client-unknown-barrier')
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $clientOne = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    $clientTwo = Start-PartisanGuardedClient `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    Assert-Rejected `
        -Label 'client unknown barrier first teardown' `
        -ExpectedIdentifier 'PGR_SELFTEST_CLIENT_UNKNOWN_BARRIER' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition (-not $context.JobClosed -and
            (Get-PartisanProcessIdentityStatus `
                -Identity $serverLaunch.RootIdentity).Status -ceq 'alive' -and
            (Get-PartisanProcessIdentityStatus `
                -Identity $clientOne.RootIdentity).Status -ceq 'alive' -and
            (Test-Path -LiteralPath $context.Guard.Directory)) `
        -Label 'client barrier retained server clients job and guard'
    Assert-Rejected `
        -Label 'client barrier no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-client-barrier-retention')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_handle_dispose' `
        -Faults @('handle-dispose-failure')
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    Assert-Rejected `
        -Label 'injected handle dispose failure' `
        -ExpectedIdentifier 'PGR_SELFTEST_HANDLE_DISPOSE' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ($context.State -ceq 'permanent-no-go' -and
            -not $context.JobClosed -and
            (Test-Path -LiteralPath $context.Guard.Directory)) `
        -Label 'handle dispose failure retained job and guard'
    Assert-Rejected `
        -Label 'handle dispose no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-handle-dispose-failure')

    $sleepScript = Join-Path $tempRoot 'descendant-sleep.ps1'
    $spawnScript = Join-Path $tempRoot 'spawn-descendant.ps1'
    $descendantMarker = Join-Path $tempRoot 'descendant.pid'
    [IO.File]::WriteAllText($sleepScript, 'Start-Sleep -Seconds 30')
    [IO.File]::WriteAllText($spawnScript, @'
param([string]$Shell, [string]$SleepScript, [string]$Marker)
$child = Start-Process -FilePath $Shell -ArgumentList @(
    '-NoLogo', '-NoProfile', '-NonInteractive', '-File', $SleepScript) -PassThru
[IO.File]::WriteAllText($Marker, [string]$child.Id)
Start-Sleep -Seconds 30
'@)
    $serverDescendantMarker = Join-Path $tempRoot 'server-descendant.pid'
    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_actual_server_descendant'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments @('-NoLogo', '-NoProfile', '-NonInteractive', '-File',
            $spawnScript, $shellExecutable, $sleepScript, $serverDescendantMarker) `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    for ($attempt = 1; $attempt -le 100 -and
        -not (Test-Path -LiteralPath $serverDescendantMarker); $attempt++) {
        Start-Sleep -Milliseconds 50
    }
    Assert-Condition `
        -Condition (Test-Path `
            -LiteralPath $serverDescendantMarker `
            -PathType Leaf) `
        -Label 'actual server descendant started before teardown'
    $serverDescendantPid = [int][IO.File]::ReadAllText($serverDescendantMarker)
    $serverDescendantClean = Invoke-PartisanGuardedTeardown -Context $context
    $serverDescendantReceipt = Read-PartisanPortableJson `
        -Path $serverDescendantClean.CleanReceiptPath
    $serverDescendantEntries = @(
        $serverDescendantReceipt.processLedger | Where-Object {
            [string]$_.role -ceq 'server-descendant'
        })
    $serverDescendantOrder = [string[]]$serverDescendantClean.TeardownStopOrder
    $serverDescendantOrderIndex = [array]::IndexOf(
        $serverDescendantOrder,
        'server-descendant')
    $serverRootOrderIndex = [array]::IndexOf(
        $serverDescendantOrder,
        'server')
    Assert-Condition `
        -Condition (@($serverDescendantEntries | Where-Object {
                [int]$_.identity.ProcessId -eq $serverDescendantPid -and
                [int]$_.identity.ParentProcessId -eq
                    [int]$serverLaunch.RootIdentity.ProcessId
            }).Count -eq 1 -and
            $serverDescendantOrderIndex -ge 0 -and
            $serverDescendantOrderIndex -lt $serverRootOrderIndex) `
        -Label ('actual server descendant classified and stopped before root; order=' +
            ($serverDescendantOrder -join ','))
    $context = $null
    [void]$checks.Add('authoritative-server-descendant-before-root')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_stop_race'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    Assert-Rejected `
        -Label 'post-resume descendant race injected' `
        -ExpectedIdentifier 'PGR_SELFTEST_POST_RESUME_INJECTED' `
        -Action { Start-PartisanGuardedClient `
            -Context $context `
            -Executable $shellExecutable `
            -ExecutableProvenance $shellProvenance `
            -Arguments @('-NoLogo', '-NoProfile', '-NonInteractive', '-File',
                $spawnScript, $shellExecutable, $sleepScript, $descendantMarker) `
            -WorkingDirectory $tempRoot `
            -NonEngineSelfTestOnly `
            -NonEngineSelfTestForcePostResumeFailure }
    Assert-Condition `
        -Condition ((Test-Path -LiteralPath $descendantMarker) -and
            @($context.Ledger | Where-Object {
                [string]$_.RootRole -ceq 'client-1' -and
                (Get-PartisanProcessIdentityStatus `
                    -Identity $_.Identity).Status -cne 'dead'
            }).Count -eq 0) `
        -Label 'stop-race descendant role reached exact dead fixed point'
    Assert-Rejected `
        -Label 'stop-race no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-stop-race-fixed-point')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_job_owned_member' `
        -Faults @('job-owned-member')
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    [void](Start-PartisanGuardedServer `
        -Context $context `
        -Executable $shellExecutable `
        -ExecutableProvenance $shellProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly)
    $jobOwned = Invoke-PartisanGuardedTeardown -Context $context
    $jobOwnedReceipt = Read-PartisanPortableJson `
        -Path $jobOwned.CleanReceiptPath
    $jobOwnedLedgerRoles = @($jobOwnedReceipt.processLedger | ForEach-Object {
        [string]$_.role
    })
    $jobOwnedLedgerEntries = @($jobOwnedReceipt.processLedger | Where-Object {
        [string]$_.role -ceq 'job-owned'
    })
    $jobOwnedProcessIds = @($jobOwnedLedgerEntries | ForEach-Object {
        [int]$_.identity.ProcessId
    })
    $jobOwnedDescendants = @($jobOwnedLedgerEntries | Where-Object {
        $jobOwnedProcessIds -contains [int]$_.identity.ParentProcessId
    })
    $jobOwnedOrder = [string[]]$jobOwned.TeardownStopOrder
    $jobOwnedIndex = [array]::IndexOf($jobOwnedOrder, 'job-owned')
    $jobServerDescendantIndex = [array]::IndexOf(
        $jobOwnedOrder,
        'server-descendant')
    $jobServerIndex = [array]::IndexOf($jobOwnedOrder, 'server')
    Assert-Condition `
        -Condition ($jobOwnedIndex -ge 0 -and
            $jobOwnedIndex -lt $jobServerIndex -and
            ($jobServerDescendantIndex -lt 0 -or
                $jobServerDescendantIndex -lt $jobServerIndex) -and
            @($jobOwnedLedgerRoles | Where-Object {
                $_ -ceq 'job-owned'
            }).Count -ge 2 -and
            $jobOwnedDescendants.Count -ge 1) `
        -Label ('unclassified job member stopped before exact server and ' +
            'recorded; order=' + ($jobOwned.TeardownStopOrder -join ',') +
            ';ledger=' + ($jobOwnedLedgerRoles -join ','))
    $context = $null
    [void]$checks.Add('authoritative-job-owned-before-server')

    $copiedExecutable = Join-Path $tempRoot 'guarded-provenance-copy.exe'
    Copy-Item `
        -LiteralPath $shellExecutable `
        -Destination $copiedExecutable `
        -ErrorAction Stop
    $copiedProvenance = Get-PartisanExecutableProvenance `
        -Path $copiedExecutable
    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_executable_replacement'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $serverLaunch = Start-PartisanGuardedServer `
        -Context $context `
        -Executable $copiedExecutable `
        -ExecutableProvenance $copiedProvenance `
        -Arguments $longArguments `
        -WorkingDirectory $tempRoot `
        -NonEngineSelfTestOnly
    [void](Stop-PartisanGuardedProcess `
        -Context $context `
        -Launch $serverLaunch)
    [IO.File]::WriteAllBytes(
        $copiedExecutable,
        [byte[]]([IO.File]::ReadAllBytes($copiedExecutable) + [byte]0))
    Assert-Rejected `
        -Label 'private executable replacement audit' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Rejected `
        -Label 'executable replacement no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ($context.CertificationInvalidated -and $context.JobClosed) `
        -Label 'executable replacement latched durable no-go'
    $context = $null
    [void]$checks.Add('authoritative-executable-replacement')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_legacy_ledger'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $legacyLedgerPath = Join-Path `
        $context.Guard.Directory `
        '.partisan-permanent-no-go.json'
    [IO.File]::WriteAllText($legacyLedgerPath, '{}')
    Assert-Rejected `
        -Label 'unexpected legacy in-guard ledger' `
        -ExpectedIdentifier 'PGR_LEGACY_LEDGER_UNEXPECTED' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Rejected `
        -Label 'legacy-ledger no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ($context.JobClosed -and
            (Test-Path -LiteralPath $legacyLedgerPath -PathType Leaf)) `
        -Label 'legacy ledger preserved as permanent no-go evidence'
    $context = $null

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_preexisting_receipt'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    [IO.File]::WriteAllText($context.CleanReceiptPath, '{}')
    Assert-Rejected `
        -Label 'preexisting clean receipt discovered at API entry' `
        -ExpectedIdentifier 'PGR_UNEXPECTED_EXTERNAL_EVIDENCE' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Rejected `
        -Label 'preexisting-receipt no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ([IO.File]::ReadAllText($context.CleanReceiptPath) -ceq '{}') `
        -Label 'untrusted preexisting clean receipt preserved unchanged'
    $context = $null

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_unexpected_journal'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $primaryCollisionBytes = '{"preexisting":"primary"}'
    $faultCollisionBytes = '{"preexisting":"fault"}'
    [IO.File]::WriteAllText(
        $context.FailureJournalExpectedPath,
        $primaryCollisionBytes)
    [IO.File]::WriteAllText(
        $context.FailureJournalFaultExpectedPath,
        $faultCollisionBytes)
    Assert-Rejected `
        -Label 'unexpected external journal discovered at API entry' `
        -ExpectedIdentifier 'PGR_UNEXPECTED_EXTERNAL_EVIDENCE' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Condition `
        -Condition (($context.FailureJournalPath -cne
                $context.FailureJournalExpectedPath) -and
            ($context.FailureJournalPath -cne
                $context.FailureJournalFaultExpectedPath) -and
            [IO.File]::ReadAllText($context.FailureJournalExpectedPath) -ceq
                $primaryCollisionBytes -and
            [IO.File]::ReadAllText(
                $context.FailureJournalFaultExpectedPath) -ceq
                $faultCollisionBytes -and
            (Split-Path -Leaf $context.FailureJournalPath) -like
                '.PartisanGuardedRuntime_*.journal-overflow-*.json') `
        -Label 'primary and fault journal collisions preserved via fresh overflow evidence'
    Assert-Rejected `
        -Label 'unexpected-journal no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-legacy-ledger-preexisting-receipt')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_journal_recovery'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    $stage = New-PartisanCandidateStage -Context $context -Candidate $candidate
    Assert-Rejected `
        -Label 'journal recovery setup stage clone' `
        -ExpectedIdentifier 'PGR_STAGE_REFERENCE_MISMATCH' `
        -Action { Assert-PartisanCandidateStage `
            -Context $context `
            -Stage $stage.PSObject.Copy() }
    $missingJournalPath = $context.FailureJournalPath
    Remove-Item -LiteralPath $missingJournalPath -Force -ErrorAction Stop
    Assert-Rejected `
        -Label 'missing expected external journal' `
        -ExpectedIdentifier 'PGR_EXPECTED_JOURNAL_MISSING' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Condition `
        -Condition ((Test-Path `
                -LiteralPath $context.FailureJournalPath `
                -PathType Leaf) -and
            $context.FailureJournalExpectedPath -ceq $missingJournalPath) `
        -Label 'missing external journal recreated durable no-go evidence'
    Assert-Rejected `
        -Label 'journal-recovery no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-external-journal-recovery')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_unknown_guard_child'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $unknownGuardChild = Join-Path $context.Guard.Directory 'untrusted-child.bin'
    $unknownGuardBytes = [byte[]](0, 1, 2, 3, 255)
    [IO.File]::WriteAllBytes($unknownGuardChild, $unknownGuardBytes)
    Assert-Rejected `
        -Label 'unknown sealed-guard child rejects ownership audit' `
        -ExpectedIdentifier 'PGR_GUARD_INVENTORY_MISMATCH' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Rejected `
        -Label 'unknown child guard remains protected from direct remover' `
        -ExpectedIdentifier 'PGR_REGISTERED_GUARD_PROTECTED' `
        -Action { Remove-PartisanGuardDirectory -Ownership $context.Guard }
    Assert-Rejected `
        -Label 'unknown child no-go teardown preserves entire guard' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    Assert-Condition `
        -Condition ((Test-Path `
                -LiteralPath $context.Guard.Directory `
                -PathType Container) -and
            [Convert]::ToBase64String(
                [IO.File]::ReadAllBytes($unknownGuardChild)) -ceq
                [Convert]::ToBase64String($unknownGuardBytes)) `
        -Label 'unknown guard inventory child preserved exactly without recursion'
    $context = $null
    [void]$checks.Add('authoritative-exact-guard-inventory-preservation')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_bounded_failure_journal'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    $runtimeModule = Get-Module -Name 'Partisan.GuardedRuntime' -ErrorAction Stop
    & $runtimeModule {
        param($ContextValue)
        $privateRecord = Find-PartisanV2Record -Context $ContextValue
        for ($repeat = 1; $repeat -le 8; $repeat++) {
            Set-PartisanV2PermanentNoGo `
                -Record $privateRecord `
                -Identifier 'PGR_REPEATED_SELFTEST_FAILURE' `
                -Message ('same-message-' + ('x' * 900)) `
                -SkipJournal
        }
        for ($index = 1; $index -le 96; $index++) {
            Set-PartisanV2PermanentNoGo `
                -Record $privateRecord `
                -Identifier ('PGR_DISTINCT_SELFTEST_' + $index) `
                -Message ('distinct-' + $index + '-' + ('y' * 900)) `
                -SkipJournal
        }
        Write-PartisanV2Journal -Record $privateRecord -Mode 'permanent-no-go'
        Update-PartisanV2PublicProjection -Record $privateRecord
    } $context
    $boundedJournalFile = Get-Item `
        -LiteralPath $context.FailureJournalPath `
        -Force `
        -ErrorAction Stop
    $boundedJournal = Read-PartisanPortableJson `
        -Path $context.FailureJournalPath
    $repeatedFailure = @($boundedJournal.failures | Where-Object {
        [string]$_.identifier -ceq 'PGR_REPEATED_SELFTEST_FAILURE'
    })
    Assert-Condition `
        -Condition (@($boundedJournal.failures).Count -le 64 -and
            [long]$boundedJournal.omittedFailureCount -gt 0 -and
            $repeatedFailure.Count -eq 1 -and
            [long]$repeatedFailure[0].repeatCount -eq 8 -and
            ([string]$repeatedFailure[0].message).Length -le 512 -and
            $boundedJournalFile.Length -lt 131072) `
        -Label 'permanent failure journal is bounded deduplicated and readable'
    Assert-Rejected `
        -Label 'bounded-journal permanent no-go cleanup' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-bounded-deduplicated-failure-journal')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_crash_after_receipt' `
        -Faults @('crash-after-receipt')
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    Assert-Rejected `
        -Label 'crash after receipt leaves pending completion transaction' `
        -ExpectedIdentifier 'PGR_SELFTEST_CRASH_AFTER_RECEIPT' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $pendingAttestation = Read-PartisanPortableJson `
        -Path $context.CompletionAttestationPath
    $preRemovalReceipt = Test-PartisanGuardedRuntimeReceipt `
        -Path $context.CleanReceiptPath `
        -ExpectedSignature $context.CleanReceiptSignature
    Assert-Condition `
        -Condition (-not $preRemovalReceipt.Complete -and
            -not $preRemovalReceipt.GuardDirectoryAbsent -and
            -not $preRemovalReceipt.CompletionAttestationExact -and
            [string]$pendingAttestation.status -ceq 'pending-guard-removal' -and
            [string]$pendingAttestation.completionTokenSha256 -cmatch
                '^[0-9a-f]{64}$' -and
            @($pendingAttestation.PSObject.Properties.Name) -cnotcontains
                'completionToken') `
        -Label 'prewritten receipt and pending state do not disclose completion secret'
    & $runtimeModule {
        param($ContextValue)
        $privateRecord = Find-PartisanV2Record -Context $ContextValue
        Write-PartisanV2Journal -Record $privateRecord -Mode 'permanent-no-go'
        Update-PartisanV2PublicProjection -Record $privateRecord
    } $context
    $deletedFaultPath = [string]$context.FailureJournalPath
    Assert-Condition `
        -Condition ($deletedFaultPath -cne $context.FailureJournalExpectedPath) `
        -Label 'crash recovery emitted separate fault evidence before deletion attack'
    Remove-Item -LiteralPath $deletedFaultPath -Force -ErrorAction Stop
    Assert-PartisanNoReparseTree -Root $context.Guard.Directory
    Remove-Item `
        -LiteralPath $context.Guard.Directory `
        -Recurse `
        -Force `
        -ErrorAction Stop
    $launderedReceipt = Test-PartisanGuardedRuntimeReceipt `
        -Path $context.CleanReceiptPath `
        -ExpectedSignature $context.CleanReceiptSignature
    Assert-Condition `
        -Condition (-not $launderedReceipt.Complete -and
            $launderedReceipt.GuardDirectoryAbsent -and
            -not $launderedReceipt.CompletionAttestationExact -and
            -not (Test-Path -LiteralPath $deletedFaultPath)) `
        -Label 'fault deletion and external guard deletion cannot launder prewritten receipt'
    $context = $null
    [void]$checks.Add('authoritative-post-removal-attestation-anti-laundering')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_missing_guard_final' `
        -Faults @('missing-guard-final')
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    Assert-Rejected `
        -Label 'final guard removal missing sentinel fault' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $incompleteReceipt = Test-PartisanGuardedRuntimeReceipt `
        -Path $context.CleanReceiptPath `
        -ExpectedSignature $context.CleanReceiptSignature
    Assert-Condition `
        -Condition ($context.CertificationInvalidated -and
            $context.JobClosed -and
            $incompleteReceipt.Status -ceq 'incomplete-permanent-no-go' -and
            $incompleteReceipt.GuardDirectoryAbsent -and
            -not $incompleteReceipt.CompletionAttestationExact -and
            (Test-Path -LiteralPath $context.FailureJournalPath -PathType Leaf)) `
        -Label 'failed final guard removal remains pending without completion attestation'
    Assert-Rejected `
        -Label 'failed-final-removal permanent no-go cleanup retry' `
        -ExpectedIdentifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
        -Action { Invoke-PartisanGuardedTeardown -Context $context }
    $context = $null
    [void]$checks.Add('authoritative-final-removal-journal-recovery')

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_partial_guard'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    Remove-Item -LiteralPath $context.Guard.Sentinel -Force
    Assert-Rejected `
        -Label 'partial guard entry validation latches externally' `
        -ExpectedIdentifier 'PGR_GUARD_IDENTITY_MISMATCH' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Condition `
        -Condition ($context.State -ceq 'permanent-no-go' -and
            (Test-Path -LiteralPath $context.FailureJournalPath)) `
        -Label 'partial guard failure journal external to guard'
    $context = $null

    $context = New-SelfTestRuntimeContext `
        -GuardBase $contextGuardBase `
        -WatchedRoot $contextWatched `
        -SpillRoot $contextSpill `
        -Purpose 'self_test_missing_guard'
    $candidate = New-SelfTestCandidate `
        -CandidateSource $candidateSource `
        -RuntimeAddon $runtimeAddon `
        -PackageRows $packageRows.ToArray() `
        -PackageSha256 $packageSha
    [void](New-PartisanCandidateStage -Context $context -Candidate $candidate)
    Assert-PartisanNoReparseTree -Root $context.Guard.Directory
    Remove-Item -LiteralPath $context.Guard.Directory -Recurse -Force
    Assert-Rejected `
        -Label 'missing guard entry validation latches externally' `
        -ExpectedIdentifier 'PGR_GUARD_IDENTITY_MISMATCH' `
        -Action { Assert-PartisanRuntimeOwnershipAudit -Context $context }
    Assert-Condition `
        -Condition ($context.State -ceq 'permanent-no-go' -and
            (Test-Path -LiteralPath $context.FailureJournalPath)) `
        -Label 'missing guard durable external no-go evidence'
    $context = $null
    [void]$checks.Add('authoritative-missing-partial-guard')
}
catch {
    $testFailure = $_
}
finally {
    if ($occupiedSocket) {
        $occupiedSocket.Dispose()
    }
    if ($context) {
        try {
            [void](Invoke-PartisanGuardedTeardown -Context $context)
        }
        catch {
            if ($context.Job -and -not $context.JobClosed) {
                try {
                    $context.Job.Dispose()
                    $context.JobClosed = $true
                    $context.Job = $null
                }
                catch { }
            }
            if (Test-Path -LiteralPath $context.Guard.Directory) {
                try {
                    [void](Remove-PartisanGuardDirectory `
                        -Ownership $context.Guard)
                }
                catch { }
            }
        }
    }
    if ($poisonContext) {
        if ($poisonContext.Job -and -not $poisonContext.JobClosed) {
            try { $poisonContext.Job.Dispose() } catch { }
        }
        if (Test-Path -LiteralPath $poisonContext.Guard.Directory) {
            try {
                [void](Remove-PartisanGuardDirectory `
                    -Ownership $poisonContext.Guard)
            }
            catch { }
        }
    }
    if ($junctionPath -and (Test-Path -LiteralPath $junctionPath)) {
        Remove-Item -LiteralPath $junctionPath -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path -LiteralPath $tempRoot -PathType Container) {
        if (-not (Test-PartisanContainedPath `
                -Root $tempBase `
                -Candidate $tempRoot)) {
            throw 'Self-test cleanup target escaped the operating-system temp root.'
        }
        for ($attempt = 1; $attempt -le 20; $attempt++) {
            try {
                Assert-PartisanNoReparseTree -Root $tempRoot
                Remove-Item `
                    -LiteralPath $tempRoot `
                    -Recurse `
                    -Force `
                    -ErrorAction Stop
                break
            }
            catch {
                if ($attempt -eq 20) {
                    $cleanupFailure = $_
                    break
                }
                Start-Sleep -Milliseconds 100
            }
        }
    }
}

if ($testFailure) {
    throw $testFailure
}
if ($cleanupFailure) {
    throw $cleanupFailure
}

$selfTestStopwatch.Stop()
[pscustomobject][ordered]@{
    Status = 'PASS'
    ProcessLifecycleMode = 'explicit-non-engine-self-test-only'
    EngineLaunchCoverage = 'not-exercised'
    PortPidAttributionCoverage = 'not-implemented-wrapper-gap'
    CheckCount = [int]$checks.Count
    RuntimeSeconds = [Math]::Round($selfTestStopwatch.Elapsed.TotalSeconds, 3)
    Checks = $checks.ToArray()
} | ConvertTo-Json -Compress -Depth 4

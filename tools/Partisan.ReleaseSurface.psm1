Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-PartisanReleaseSurfaceContract {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $resolved = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        throw 'The release-surface contract does not exist.'
    }
    $contract = Get-Content -LiteralPath $resolved -Raw | ConvertFrom-Json
    $names = @($contract.PSObject.Properties |
        ForEach-Object { [string]$_.Name })
    $expectedNames = @(
            'schemaVersion',
            'evidenceKind',
            'diagnosticSymbol',
            'paritySource',
            'carrierFileNamePattern',
            'forbiddenTypeNamePattern',
            'expectedCarrierFileCount',
            'expectedForbiddenTypeCount',
            'expectedForbiddenCommandActionIdCount',
            'expectedForbiddenMemberSurfaceCount',
            'expectedForbiddenLiteralSurfaceCount',
            'expectedProductionObservabilityMemberSurfaceCount',
            'carrierFiles',
            'mixedDiagnosticFiles',
            'commandSurfaceFiles',
            'forbiddenTypeNames',
            'forbiddenCommandActionIds',
            'productionPositiveControlTypeNames',
            'productionPositiveControlCommandActionIds',
            'forbiddenMemberSurfaces',
            'forbiddenLiteralSurfaces',
            'productionObservabilityMemberSurfaces',
            'productionObservabilityPolicy')
    if (@(Compare-Object `
            -ReferenceObject @($expectedNames | Sort-Object) `
            -DifferenceObject @($names | Sort-Object) `
            -CaseSensitive).Count -ne 0) {
        throw 'The release-surface contract property set is invalid.'
    }
    if ([int]$contract.schemaVersion -ne 1 -or
        [string]$contract.evidenceKind -cne
            'partisan_release_surface_contract_v1' -or
        [string]$contract.diagnosticSymbol -cne 'ENABLE_DIAG' -or
        [string]$contract.carrierFileNamePattern -cne
            '(Proof|CampaignDebug|Autotest|SmokeTest|Fixture|Harness)' -or
        [string]$contract.forbiddenTypeNamePattern -cne
            '(Proof|CampaignDebug|Autotest|SmokeTest|Fixture|Harness)') {
        throw 'The release-surface contract identity is unsupported.'
    }
    foreach ($arrayName in @(
            'carrierFiles',
            'mixedDiagnosticFiles',
            'commandSurfaceFiles',
            'forbiddenTypeNames',
            'forbiddenCommandActionIds',
            'productionPositiveControlTypeNames',
            'productionPositiveControlCommandActionIds',
            'forbiddenMemberSurfaces',
            'forbiddenLiteralSurfaces',
            'productionObservabilityMemberSurfaces',
            'productionObservabilityPolicy')) {
        $values = @($contract.$arrayName)
        if ($null -eq $contract.$arrayName -or
            $contract.$arrayName -is [string] -or
            $values.Count -eq 0 -or
            @($values | Sort-Object -Unique).Count -ne $values.Count) {
            throw "The release-surface contract array $arrayName is invalid."
        }
    }
    if (@($contract.carrierFiles).Count -ne
            [int]$contract.expectedCarrierFileCount -or
        @($contract.forbiddenTypeNames).Count -ne
            [int]$contract.expectedForbiddenTypeCount -or
        @($contract.forbiddenCommandActionIds).Count -ne
            [int]$contract.expectedForbiddenCommandActionIdCount -or
        @($contract.forbiddenMemberSurfaces).Count -ne
            [int]$contract.expectedForbiddenMemberSurfaceCount -or
        @($contract.forbiddenLiteralSurfaces).Count -ne
            [int]$contract.expectedForbiddenLiteralSurfaceCount -or
        @($contract.productionObservabilityMemberSurfaces).Count -ne
            [int]$contract.expectedProductionObservabilityMemberSurfaceCount) {
        throw 'The release-surface contract counts are inconsistent.'
    }
    foreach ($pathValue in @(
            @($contract.carrierFiles) +
            @($contract.mixedDiagnosticFiles) +
            @($contract.commandSurfaceFiles) +
            @([string]$contract.paritySource))) {
        $portable = [string]$pathValue
        if ([string]::IsNullOrWhiteSpace($portable) -or
            $portable.Contains('\') -or
            $portable.Contains(':') -or
            $portable.StartsWith('/', [StringComparison]::Ordinal) -or
            $portable.Split('/') -contains '..') {
            throw 'The release-surface contract contains a nonportable path.'
        }
    }
    foreach ($surfaceName in @(
            'forbiddenMemberSurfaces',
            'forbiddenLiteralSurfaces',
            'productionObservabilityMemberSurfaces')) {
        foreach ($rawSurface in @($contract.$surfaceName)) {
            $surface = [string]$rawSurface
            $separator = $surface.IndexOf('::')
            if ($separator -le 0 -or $separator -ge $surface.Length - 2) {
                throw "The release-surface contract $surfaceName entry is invalid."
            }
            $pathValue = $surface.Substring(0, $separator)
            $tokenValue = $surface.Substring($separator + 2)
            if ($pathValue.Contains('\') -or
                $pathValue.Contains(':') -or
                $pathValue.StartsWith('/', [StringComparison]::Ordinal) -or
                $pathValue.Split('/') -contains '..' -or
                [string]::IsNullOrWhiteSpace($tokenValue) -or
                $tokenValue.Contains([string][char]10) -or
                $tokenValue.Contains([string][char]13)) {
                throw "The release-surface contract $surfaceName entry is unsafe."
            }
            if ($surfaceName -cne 'forbiddenLiteralSurfaces' -and
                $tokenValue -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
                throw "The release-surface contract $surfaceName token is invalid."
            }
        }
    }
    return $contract
}

function Compare-PartisanReleaseSurfaceSet {
    param(
        [Parameter(Mandatory = $true)][object[]]$Expected,
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)][object[]]$Actual,
        [Parameter(Mandatory = $true)][string]$Label,
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.List[string]]$Violations
    )

    $difference = @(Compare-Object `
        -ReferenceObject @($Expected | ForEach-Object { [string]$_ } |
            Sort-Object -Unique) `
        -DifferenceObject @($Actual | ForEach-Object { [string]$_ } |
            Sort-Object -Unique) `
        -CaseSensitive)
    if ($difference.Count -eq 0) {
        return
    }
    $preview = @($difference | Select-Object -First 12 | ForEach-Object {
        '{0}:{1}' -f $_.SideIndicator, $_.InputObject
    }) -join ', '
    $Violations.Add("$Label differs from the contract: $preview")
}

function Get-PartisanReleaseSurfaceCommandRouteIds {
    param(
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$MemberName
    )

    $escaped = [regex]::Escape($MemberName)
    $start = -1
    for ($index = 0; $index -lt $Lines.Count; $index++) {
        if ([string]$Lines[$index] -cmatch (
                '^\s*(?:protected\s+)?bool\s+' + $escaped + '\s*\(')) {
            if ($start -ge 0) {
                throw "Release-surface route member $MemberName is duplicated."
            }
            $start = $index
        }
    }
    if ($start -lt 0) {
        return @()
    }

    $depth = 0
    $opened = $false
    $methodLines = [System.Collections.Generic.List[string]]::new()
    for ($index = $start; $index -lt $Lines.Count; $index++) {
        $line = [string]$Lines[$index]
        $methodLines.Add($line)
        $opens = [regex]::Matches($line, '\{').Count
        $closes = [regex]::Matches($line, '\}').Count
        if ($opens -gt 0) {
            $opened = $true
        }
        $depth += $opens - $closes
        if ($opened -and $depth -eq 0) {
            break
        }
    }
    if (-not $opened -or $depth -ne 0) {
        throw "Release-surface route member $MemberName is structurally invalid."
    }

    return @($methodLines | ForEach-Object {
        foreach ($match in [regex]::Matches(
                [string]$_,
                'commandId\s*==\s*"(?<id>[a-z][a-z0-9_]*)"')) {
            [string]$match.Groups['id'].Value
        }
    } | Sort-Object -Unique)
}

function Get-PartisanReleaseSurfaceLineStates {
    param(
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$Path,
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.List[string]]$Violations
    )

    $states = [System.Collections.Generic.List[object]]::new()
    $stack = [System.Collections.Generic.List[object]]::new()
    $retail = $true
    $diagnostic = $true
    for ($index = 0; $index -lt $Lines.Count; $index++) {
        $text = [string]$Lines[$index]
        $number = $index + 1
        $trimmed = $text.Trim()
        $directive = $trimmed.StartsWith(
            '#',
            [StringComparison]::Ordinal)
        if ($directive) {
            if ($trimmed -cmatch '^#\s*(define|undef)\s+ENABLE_DIAG\b') {
                $Violations.Add(
                    "$($Path):$number locally mutates ENABLE_DIAG.")
            }
            if ($trimmed.Contains('ENABLE_DIAG') -and
                $trimmed -cnotmatch '^#ifdef ENABLE_DIAG$') {
                $Violations.Add(
                    "$($Path):$number uses an unsupported ENABLE_DIAG directive.")
            }
            if ($trimmed -cmatch '^#ifdef\s+([A-Za-z_][A-Za-z0-9_]*)$') {
                $isDiagnostic = $Matches[1] -ceq 'ENABLE_DIAG'
                $frame = [pscustomobject]@{
                    retail = [bool]$retail
                    diagnostic = [bool]$diagnostic
                    isDiagnostic = $isDiagnostic
                    elseSeen = $false
                }
                $stack.Add($frame)
                if ($isDiagnostic) {
                    $retail = $false
                    $diagnostic = [bool]$frame.diagnostic
                }
            }
            elseif ($trimmed -cmatch '^#if(n?def)?\b') {
                $stack.Add([pscustomobject]@{
                    retail = [bool]$retail
                    diagnostic = [bool]$diagnostic
                    isDiagnostic = $false
                    elseSeen = $false
                })
            }
            elseif ($trimmed -cmatch '^#elif\b') {
                if ($stack.Count -eq 0) {
                    $Violations.Add(
                        "$($Path):$number has an unmatched #elif.")
                }
                elseif ($stack[$stack.Count - 1].isDiagnostic) {
                    $Violations.Add(
                        "$($Path):$number branches a diagnostic guard with #elif.")
                }
            }
            elseif ($trimmed -ceq '#else') {
                if ($stack.Count -eq 0) {
                    $Violations.Add(
                        "$($Path):$number has an unmatched #else.")
                }
                else {
                    $frame = $stack[$stack.Count - 1]
                    if ($frame.elseSeen) {
                        $Violations.Add(
                            "$($Path):$number repeats #else.")
                    }
                    $frame.elseSeen = $true
                    if ($frame.isDiagnostic) {
                        $retail = [bool]$frame.retail
                        $diagnostic = $false
                    }
                }
            }
            elseif ($trimmed -ceq '#endif') {
                if ($stack.Count -eq 0) {
                    $Violations.Add(
                        "$($Path):$number has an unmatched #endif.")
                }
                else {
                    $frame = $stack[$stack.Count - 1]
                    $stack.RemoveAt($stack.Count - 1)
                    $retail = [bool]$frame.retail
                    $diagnostic = [bool]$frame.diagnostic
                }
            }
            elseif ($trimmed -cmatch '^#') {
                $known = $trimmed -cmatch
                    '^#(?:include|define|undef|pragma|error|warning)\b'
                if (-not $known) {
                    $Violations.Add(
                        "$($Path):$number has an unknown preprocessor directive.")
                }
            }
        }
        $states.Add([pscustomobject]@{
            number = $number
            text = $text
            directive = $directive
            retail = [bool]$retail
            diagnostic = [bool]$diagnostic
        })
    }
    if ($stack.Count -ne 0) {
        $Violations.Add("$Path has an unterminated preprocessor conditional.")
    }
    return $states.ToArray()
}

function Invoke-PartisanReleaseSurfaceAnalysis {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$ContractPath
    )

    $root = [IO.Path]::GetFullPath($RepositoryRoot).TrimEnd('\', '/')
    $contract = Get-PartisanReleaseSurfaceContract -Path $ContractPath
    $violations = [System.Collections.Generic.List[string]]::new()

    $parityPath = [IO.Path]::GetFullPath(
        (Join-Path $root ([string]$contract.paritySource)))
    $rootPrefix = $root + [IO.Path]::DirectorySeparatorChar
    if (-not $parityPath.StartsWith(
            $rootPrefix,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $parityPath -PathType Leaf)) {
        throw 'The release-surface parity source is unavailable.'
    }
    $parity = Get-Content -LiteralPath $parityPath -Raw |
        ConvertFrom-Json
    $rules = @($parity.actionRules | Where-Object {
        [string]$_.rowId -ceq 'development-diagnostics' -or
        ([string]$_.surface).StartsWith(
            'development-',
            [StringComparison]::Ordinal)
    })
    $parityCommands = @($parity.commandActionIds | Where-Object {
        $id = [string]$_
        @($rules | Where-Object {
            $id -match [string]$_.pattern
        }).Count -gt 0
    } | ForEach-Object { [string]$_ } | Sort-Object -Unique)
    Compare-PartisanReleaseSurfaceSet `
        -Expected @($contract.forbiddenCommandActionIds) `
        -Actual $parityCommands `
        -Label 'Parity-derived development command IDs' `
        -Violations $violations

    $sourceRoot = Join-Path $root 'Scripts/Game/HST'
    $files = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -File |
        Where-Object { $_.Extension -ceq '.c' } |
        Sort-Object FullName)
    $carrierPattern = [regex][string]$contract.carrierFileNamePattern
    $typePattern = [regex][string]$contract.forbiddenTypeNamePattern

    $forbiddenTypes =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    foreach ($name in @($contract.forbiddenTypeNames)) {
        [void]$forbiddenTypes.Add([string]$name)
    }
    $forbiddenCommands =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    foreach ($name in @($contract.forbiddenCommandActionIds)) {
        [void]$forbiddenCommands.Add([string]$name)
    }
    $positiveTypes = @($contract.productionPositiveControlTypeNames)
    $positiveCommands =
        @($contract.productionPositiveControlCommandActionIds)
    $commandSurfaceFiles =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    foreach ($path in @($contract.commandSurfaceFiles)) {
        [void]$commandSurfaceFiles.Add([string]$path)
    }

    $carriers = [System.Collections.Generic.List[string]]::new()
    $mixed = [System.Collections.Generic.List[string]]::new()
    $discoveredTypes = [System.Collections.Generic.List[string]]::new()
    $seenTypes =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $seenCommands =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $retailPositiveTypes =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $diagPositiveTypes =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $retailPositiveCommands =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $diagPositiveCommands =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $seenForbiddenMembers =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $seenForbiddenLiterals =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $retailPositiveMembers =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
    $diagPositiveMembers =
        New-Object 'System.Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)

    foreach ($file in $files) {
        $full = [IO.Path]::GetFullPath($file.FullName)
        if (-not $full.StartsWith(
                $rootPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A release-surface source escaped the repository.'
        }
        $path = $full.Substring($rootPrefix.Length).Replace('\', '/')
        $surfacePrefix = $path + '::'
        $fileForbiddenMembers = @($contract.forbiddenMemberSurfaces |
            Where-Object {
                ([string]$_).StartsWith(
                    $surfacePrefix,
                    [StringComparison]::Ordinal)
            })
        $fileForbiddenLiterals = @($contract.forbiddenLiteralSurfaces |
            Where-Object {
                ([string]$_).StartsWith(
                    $surfacePrefix,
                    [StringComparison]::Ordinal)
            })
        $filePositiveMembers = @(
            $contract.productionObservabilityMemberSurfaces |
            Where-Object {
                ([string]$_).StartsWith(
                    $surfacePrefix,
                    [StringComparison]::Ordinal)
            })
        $carrier = $carrierPattern.IsMatch($file.Name)
        if ($carrier) {
            $carriers.Add($path)
        }
        $lines = @([IO.File]::ReadAllLines($full))
        $states = @(Get-PartisanReleaseSurfaceLineStates `
            -Lines $lines `
            -Path $path `
            -Violations $violations)
        $guardCount = @($states | Where-Object {
            $_.directive -and
            ([string]$_.text).Trim() -ceq '#ifdef ENABLE_DIAG'
        }).Count
        if (-not $carrier -and $guardCount -gt 0) {
            $mixed.Add($path)
        }
        if ($carrier) {
            $nonEmpty = @($lines | ForEach-Object { $_.Trim() } |
                Where-Object { $_.Length -gt 0 })
            if ($nonEmpty.Count -lt 2 -or
                $nonEmpty[0] -cne '#ifdef ENABLE_DIAG' -or
                $nonEmpty[$nonEmpty.Count - 1] -cne '#endif') {
                $violations.Add(
                    "$path is not enclosed by one whole-file diagnostic guard.")
            }
        }

        foreach ($state in $states) {
            if ($state.directive) {
                continue
            }
            $line = [string]$state.text
            if ($line -match
                '^\s*(?:(?:modded|sealed)\s+)*(?:class|enum)\s+(HST_[A-Za-z0-9_]+)') {
                $declared = $Matches[1]
                if (($state.diagnostic -and -not $state.retail) -or
                    $carrier -or
                    $typePattern.IsMatch($declared)) {
                    $discoveredTypes.Add($declared)
                }
                if ($positiveTypes -ccontains $declared) {
                    if ($state.retail) {
                        [void]$retailPositiveTypes.Add($declared)
                    }
                    if ($state.diagnostic) {
                        [void]$diagPositiveTypes.Add($declared)
                    }
                }
            }
            foreach ($match in [regex]::Matches(
                    $line,
                    '\bHST_[A-Za-z0-9_]+\b')) {
                $token = [string]$match.Value
                if (-not $forbiddenTypes.Contains($token)) {
                    continue
                }
                if ($state.retail) {
                    $violations.Add(
                        "$($path):$($state.number) exposes forbidden type $token.")
                }
                if ($state.diagnostic) {
                    [void]$seenTypes.Add($token)
                }
            }
            foreach ($surface in $fileForbiddenMembers) {
                $token = ([string]$surface).Substring(
                    $surfacePrefix.Length)
                $pattern = '(?<![A-Za-z0-9_])' +
                    [regex]::Escape($token) +
                    '(?![A-Za-z0-9_])'
                if ($line -cnotmatch $pattern) {
                    continue
                }
                if ($state.retail) {
                    $violations.Add(
                        "$($path):$($state.number) exposes forbidden member $token.")
                }
                if ($state.diagnostic) {
                    [void]$seenForbiddenMembers.Add([string]$surface)
                }
            }
            foreach ($surface in $fileForbiddenLiterals) {
                $literal = ([string]$surface).Substring(
                    $surfacePrefix.Length)
                if (-not $line.Contains('"' + $literal + '"')) {
                    continue
                }
                if ($state.retail) {
                    $violations.Add(
                        "$($path):$($state.number) exposes forbidden literal $literal.")
                }
                if ($state.diagnostic) {
                    [void]$seenForbiddenLiterals.Add([string]$surface)
                }
            }
            foreach ($surface in $filePositiveMembers) {
                $token = ([string]$surface).Substring(
                    $surfacePrefix.Length)
                $pattern = '(?<![A-Za-z0-9_])' +
                    [regex]::Escape($token) +
                    '(?![A-Za-z0-9_])'
                if ($line -cnotmatch $pattern) {
                    continue
                }
                if ($state.retail) {
                    [void]$retailPositiveMembers.Add([string]$surface)
                }
                if ($state.diagnostic) {
                    [void]$diagPositiveMembers.Add([string]$surface)
                }
            }
            if (-not $commandSurfaceFiles.Contains($path)) {
                continue
            }
            foreach ($match in [regex]::Matches(
                    $line,
                    '"(?<id>[a-z][a-z0-9_]*)"')) {
                $command = [string]$match.Groups['id'].Value
                if ($forbiddenCommands.Contains($command)) {
                    if ($state.retail) {
                        $violations.Add(
                            "$($path):$($state.number) exposes forbidden command $command.")
                    }
                    if ($state.diagnostic) {
                        [void]$seenCommands.Add($command)
                    }
                }
                if ($positiveCommands -ccontains $command) {
                    if ($state.retail) {
                        [void]$retailPositiveCommands.Add($command)
                    }
                    if ($state.diagnostic) {
                        [void]$diagPositiveCommands.Add($command)
                    }
                }
            }
        }
    }

    $routingPath =
        'Scripts/Game/HST/Services/HST_CommandUIService.c'
    if ($commandSurfaceFiles.Contains($routingPath)) {
        $routingFullPath = Join-Path $root $routingPath.Replace('/', '\')
        $routingLines = @([IO.File]::ReadAllLines($routingFullPath))
        foreach ($routeMember in @(
                'IsKnownVisibleCommand',
                'IsVisibleCommandDispatchHandled')) {
            $routeIds = @(Get-PartisanReleaseSurfaceCommandRouteIds `
                -Lines $routingLines `
                -MemberName $routeMember | Where-Object {
                    $forbiddenCommands.Contains([string]$_)
                })
            Compare-PartisanReleaseSurfaceSet `
                -Expected @($contract.forbiddenCommandActionIds) `
                -Actual $routeIds `
                -Label "Diagnostic command routing in $routeMember" `
                -Violations $violations
        }
    }

    Compare-PartisanReleaseSurfaceSet `
        -Expected @($contract.carrierFiles) `
        -Actual $carriers.ToArray() `
        -Label 'Diagnostic carrier files' `
        -Violations $violations
    Compare-PartisanReleaseSurfaceSet `
        -Expected @($contract.mixedDiagnosticFiles) `
        -Actual $mixed.ToArray() `
        -Label 'Mixed diagnostic files' `
        -Violations $violations
    Compare-PartisanReleaseSurfaceSet `
        -Expected @($contract.forbiddenTypeNames) `
        -Actual @($discoveredTypes | Sort-Object -Unique) `
        -Label 'Forbidden type names' `
        -Violations $violations

    foreach ($name in @($contract.forbiddenTypeNames)) {
        if (-not $seenTypes.Contains([string]$name)) {
            $violations.Add(
                "Forbidden type $name is absent from the diagnostic source.")
        }
    }
    foreach ($name in @($contract.forbiddenCommandActionIds)) {
        if (-not $seenCommands.Contains([string]$name)) {
            $violations.Add(
                "Forbidden command $name is absent from the diagnostic source.")
        }
    }
    foreach ($surface in @($contract.forbiddenMemberSurfaces)) {
        if (-not $seenForbiddenMembers.Contains([string]$surface)) {
            $violations.Add(
                "Forbidden member surface $surface is absent from diagnostic source.")
        }
    }
    foreach ($surface in @($contract.forbiddenLiteralSurfaces)) {
        if (-not $seenForbiddenLiterals.Contains([string]$surface)) {
            $violations.Add(
                "Forbidden literal surface $surface is absent from diagnostic source.")
        }
    }
    foreach ($name in $positiveTypes) {
        if (-not $retailPositiveTypes.Contains([string]$name) -or
            -not $diagPositiveTypes.Contains([string]$name)) {
            $violations.Add(
                "Production type $name is not active in both modes.")
        }
    }
    foreach ($name in $positiveCommands) {
        if (-not $retailPositiveCommands.Contains([string]$name) -or
            -not $diagPositiveCommands.Contains([string]$name)) {
            $violations.Add(
                "Production command $name is not active in both modes.")
        }
    }
    foreach ($surface in @(
            $contract.productionObservabilityMemberSurfaces)) {
        if (-not $retailPositiveMembers.Contains([string]$surface) -or
            -not $diagPositiveMembers.Contains([string]$surface)) {
            $violations.Add(
                "Production member surface $surface is not active in both modes.")
        }
    }

    return [pscustomobject][ordered]@{
        schemaVersion = 1
        evidenceKind = 'partisan_release_surface_source_analysis_v1'
        passed = $violations.Count -eq 0
        carrierFileCount = $carriers.Count
        mixedDiagnosticFileCount = $mixed.Count
        forbiddenTypeCount =
            @($discoveredTypes | Sort-Object -Unique).Count
        forbiddenCommandActionIdCount = $seenCommands.Count
        forbiddenMemberSurfaceCount = $seenForbiddenMembers.Count
        forbiddenLiteralSurfaceCount = $seenForbiddenLiterals.Count
        productionPositiveControlTypeCount =
            $retailPositiveTypes.Count
        productionPositiveControlCommandActionIdCount =
            $retailPositiveCommands.Count
        productionObservabilityMemberSurfaceCount =
            $retailPositiveMembers.Count
        sourceFileCount = $files.Count
        violations = $violations.ToArray()
    }
}

function Assert-PartisanReleaseSurfaceGuards {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$ContractPath
    )

    $analysis = Invoke-PartisanReleaseSurfaceAnalysis `
        -RepositoryRoot $RepositoryRoot `
        -ContractPath $ContractPath
    if (-not $analysis.passed) {
        $preview = @($analysis.violations | Select-Object -First 40) -join
            [Environment]::NewLine
        throw (
            "Release-surface source validation failed with " +
            "$($analysis.violations.Count) violation(s):" +
            [Environment]::NewLine +
            $preview)
    }
    return $analysis
}

Export-ModuleMember -Function @(
    'Get-PartisanReleaseSurfaceContract',
    'Invoke-PartisanReleaseSurfaceAnalysis',
    'Assert-PartisanReleaseSurfaceGuards'
)

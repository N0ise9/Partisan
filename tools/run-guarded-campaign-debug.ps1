[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeAddonRoot,

    [Parameter(Mandatory = $true)]
    [string]$CandidateManifest,

    [Parameter(Mandatory = $true)]
    [string]$CandidateBundleRoot,

    [Parameter(Mandatory = $true)]
    [string]$SettingsSource,

    [string]$EvidenceOutputRoot = '',

    [ValidateSet("full_certification", "force_authority")]
    [string]$Profile = "full_certification",

    [string]$WorldResource = "Worlds/HST_Dev/HST_Dev.ent",
    [string[]]$WatchedRoots = @(),
    [string[]]$SpillRoots = @(),
    [ValidateRange(60, 3600)]
    [int]$TimeoutSeconds = 1080,
    [ValidateRange(10, 600)]
    [int]$ArmedTimeoutSeconds = 90,
    [ValidateRange(10, 900)]
    [int]$StartedTimeoutSeconds = 180,
    [ValidateRange(1, 30)]
    [int]$PollSeconds = 5,
    [switch]$PreflightOnly,
    [switch]$ArtifactValidatorSelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ValidateSet is intentionally case-insensitive, so normalize once before any
# case-sensitive profile routing or retained evidence serialization.
$Profile = $Profile.ToLowerInvariant()

if (-not ("PartisanGuardedJob" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public sealed class PartisanGuardedJob : IDisposable
{
    private IntPtr _handle;

    [StructLayout(LayoutKind.Sequential)]
    private struct IO_COUNTERS
    {
        public UInt64 ReadOperationCount;
        public UInt64 WriteOperationCount;
        public UInt64 OtherOperationCount;
        public UInt64 ReadTransferCount;
        public UInt64 WriteTransferCount;
        public UInt64 OtherTransferCount;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct JOBOBJECT_BASIC_LIMIT_INFORMATION
    {
        public Int64 PerProcessUserTimeLimit;
        public Int64 PerJobUserTimeLimit;
        public UInt32 LimitFlags;
        public UIntPtr MinimumWorkingSetSize;
        public UIntPtr MaximumWorkingSetSize;
        public UInt32 ActiveProcessLimit;
        public UIntPtr Affinity;
        public UInt32 PriorityClass;
        public UInt32 SchedulingClass;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION
    {
        public JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
        public IO_COUNTERS IoInfo;
        public UIntPtr ProcessMemoryLimit;
        public UIntPtr JobMemoryLimit;
        public UIntPtr PeakProcessMemoryUsed;
        public UIntPtr PeakJobMemoryUsed;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern IntPtr CreateJobObject(IntPtr attributes, string name);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetInformationJobObject(
        IntPtr job,
        int informationClass,
        IntPtr information,
        UInt32 informationLength);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool QueryInformationJobObject(
        IntPtr job,
        int informationClass,
        IntPtr information,
        UInt32 informationLength,
        out UInt32 returnLength);

    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr handle);

    public PartisanGuardedJob()
    {
        _handle = CreateJobObject(IntPtr.Zero, null);
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException("Unable to create guarded process job.");

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
            new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
        info.BasicLimitInformation.LimitFlags = 0x00002000;
        int size = Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
        IntPtr pointer = Marshal.AllocHGlobal(size);
        try
        {
            Marshal.StructureToPtr(info, pointer, false);
            if (!SetInformationJobObject(_handle, 9, pointer, (UInt32)size))
                throw new InvalidOperationException("Unable to configure guarded process job.");
        }
        finally
        {
            Marshal.FreeHGlobal(pointer);
        }
    }

    public void Add(Process process)
    {
        if (process == null)
            throw new ArgumentNullException("process");
        if (!AssignProcessToJobObject(_handle, process.Handle))
            throw new InvalidOperationException("Unable to assign process to guarded job.");
    }

    public Int32[] GetProcessIds()
    {
        if (_handle == IntPtr.Zero)
            return new Int32[0];

        int capacity = 16;
        while (capacity <= 4096)
        {
            int size = 8 + (capacity * IntPtr.Size);
            IntPtr pointer = Marshal.AllocHGlobal(size);
            try
            {
                for (int offset = 0; offset < size; offset += 4)
                    Marshal.WriteInt32(pointer, offset, 0);

                UInt32 returned;
                bool ok = QueryInformationJobObject(
                    _handle,
                    3,
                    pointer,
                    (UInt32)size,
                    out returned);
                int assigned = Marshal.ReadInt32(pointer, 0);
                int listed = Marshal.ReadInt32(pointer, 4);
                if (!ok && assigned <= capacity)
                    throw new InvalidOperationException("Unable to query guarded process job.");
                if (assigned > capacity || listed > capacity)
                {
                    capacity *= 2;
                    continue;
                }

                List<Int32> result = new List<Int32>();
                for (int index = 0; index < listed; index++)
                {
                    long value = Marshal.ReadIntPtr(pointer, 8 + (index * IntPtr.Size)).ToInt64();
                    if (value > 0 && value <= Int32.MaxValue)
                        result.Add((Int32)value);
                }
                return result.ToArray();
            }
            finally
            {
                Marshal.FreeHGlobal(pointer);
            }
        }

        throw new InvalidOperationException("Guarded process job membership exceeded the safety limit.");
    }

    public void Dispose()
    {
        if (_handle == IntPtr.Zero)
            return;
        CloseHandle(_handle);
        _handle = IntPtr.Zero;
    }
}

public sealed class PartisanGuardedSuspendedProcess : IDisposable
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct STARTUPINFO
    {
        public UInt32 cb;
        public string lpReserved;
        public string lpDesktop;
        public string lpTitle;
        public UInt32 dwX;
        public UInt32 dwY;
        public UInt32 dwXSize;
        public UInt32 dwYSize;
        public UInt32 dwXCountChars;
        public UInt32 dwYCountChars;
        public UInt32 dwFillAttribute;
        public UInt32 dwFlags;
        public UInt16 wShowWindow;
        public UInt16 cbReserved2;
        public IntPtr lpReserved2;
        public IntPtr hStdInput;
        public IntPtr hStdOutput;
        public IntPtr hStdError;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct PROCESS_INFORMATION
    {
        public IntPtr hProcess;
        public IntPtr hThread;
        public UInt32 dwProcessId;
        public UInt32 dwThreadId;
    }

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool CreateProcessW(
        string applicationName,
        StringBuilder commandLine,
        IntPtr processAttributes,
        IntPtr threadAttributes,
        bool inheritHandles,
        UInt32 creationFlags,
        IntPtr environment,
        string currentDirectory,
        ref STARTUPINFO startupInfo,
        out PROCESS_INFORMATION processInformation);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern UInt32 ResumeThread(IntPtr thread);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool TerminateProcess(IntPtr process, UInt32 exitCode);

    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr handle);

    private IntPtr _threadHandle;
    private bool _resumed;
    public Process Child { get; private set; }

    public PartisanGuardedSuspendedProcess(
        string applicationName,
        string commandLine,
        string currentDirectory)
    {
        STARTUPINFO startup = new STARTUPINFO();
        startup.cb = (UInt32)Marshal.SizeOf(typeof(STARTUPINFO));
        PROCESS_INFORMATION information;
        const UInt32 CREATE_SUSPENDED = 0x00000004;
        if (!CreateProcessW(
            applicationName,
            new StringBuilder(commandLine),
            IntPtr.Zero,
            IntPtr.Zero,
            false,
            CREATE_SUSPENDED,
            IntPtr.Zero,
            currentDirectory,
            ref startup,
            out information))
        {
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to create suspended diagnostic runtime process.");
        }

        try
        {
            Child = Process.GetProcessById((Int32)information.dwProcessId);
            _threadHandle = information.hThread;
            information.hThread = IntPtr.Zero;
        }
        catch
        {
            TerminateProcess(information.hProcess, 1);
            throw;
        }
        finally
        {
            if (information.hThread != IntPtr.Zero)
                CloseHandle(information.hThread);
            if (information.hProcess != IntPtr.Zero)
                CloseHandle(information.hProcess);
        }
    }

    public void Resume()
    {
        if (_threadHandle == IntPtr.Zero)
            throw new InvalidOperationException("Suspended runtime thread is unavailable.");
        if (ResumeThread(_threadHandle) == UInt32.MaxValue)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to resume diagnostic runtime process.");
        _resumed = true;
        CloseHandle(_threadHandle);
        _threadHandle = IntPtr.Zero;
    }

    public void Dispose()
    {
        if (!_resumed && Child != null)
        {
            try
            {
                if (!Child.HasExited)
                {
                    Child.Kill();
                    Child.WaitForExit(2000);
                }
            }
            catch { }
        }
        if (_threadHandle != IntPtr.Zero)
        {
            CloseHandle(_threadHandle);
            _threadHandle = IntPtr.Zero;
        }
    }
}

public static class PartisanNativeCommandLine
{
    [DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern IntPtr CommandLineToArgvW(string commandLine, out int argumentCount);

    [DllImport("kernel32.dll")]
    private static extern IntPtr LocalFree(IntPtr memory);

    public static string[] Split(string commandLine)
    {
        if (commandLine == null)
            throw new ArgumentNullException("commandLine");

        int count;
        IntPtr pointer = CommandLineToArgvW(commandLine, out count);
        if (pointer == IntPtr.Zero)
            throw new InvalidOperationException("Unable to parse native command line.");
        try
        {
            string[] result = new string[count];
            for (int index = 0; index < count; index++)
            {
                IntPtr value = Marshal.ReadIntPtr(pointer, index * IntPtr.Size);
                result[index] = Marshal.PtrToStringUni(value);
            }
            return result;
        }
        finally
        {
            LocalFree(pointer);
        }
    }
}
"@
}

function Resolve-ExistingPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [ValidateSet("Leaf", "Container")]
        [string]$Kind
    )

    if (-not (Test-Path -LiteralPath $Path -PathType $Kind)) {
        throw "Required $Kind path is unavailable."
    }

    return [IO.Path]::GetFullPath((Get-Item -LiteralPath $Path -Force).FullName)
}

function ConvertTo-NativeArgument {
    param([AllowEmptyString()][string]$Value)

    if ($Value.Length -eq 0) {
        return '""'
    }
    if ($Value -notmatch '[\s"]') {
        return $Value
    }

    $builder = New-Object Text.StringBuilder
    [void]$builder.Append('"')
    $slashes = 0
    foreach ($character in $Value.ToCharArray()) {
        if ($character -eq '\') {
            $slashes++
            continue
        }

        if ($character -eq '"') {
            [void]$builder.Append(('\' * (($slashes * 2) + 1)))
            [void]$builder.Append('"')
            $slashes = 0
            continue
        }

        if ($slashes -gt 0) {
            [void]$builder.Append(('\' * $slashes))
            $slashes = 0
        }
        [void]$builder.Append($character)
    }

    if ($slashes -gt 0) {
        [void]$builder.Append(('\' * ($slashes * 2)))
    }
    [void]$builder.Append('"')
    return $builder.ToString()
}

function ConvertTo-SafeEvidenceLine {
    param(
        [AllowNull()][string]$Line,
        [string]$GuardRoot = "",
        [string]$ProjectDirectory = "",
        [string[]]$ResolvedAddonRoots = @()
    )

    if ([string]::IsNullOrWhiteSpace($Line)) {
        return $null
    }
    $safe = $Line.Trim()
    $replacements = New-Object Collections.Generic.List[object]
    if (-not [string]::IsNullOrWhiteSpace($GuardRoot)) {
        $replacements.Add([pscustomobject]@{ Value = $GuardRoot; Label = "<guard>" })
    }
    if (-not [string]::IsNullOrWhiteSpace($ProjectDirectory)) {
        $replacements.Add([pscustomobject]@{ Value = $ProjectDirectory; Label = "<project>" })
    }
    foreach ($root in $ResolvedAddonRoots) {
        if (-not [string]::IsNullOrWhiteSpace($root)) {
            $replacements.Add([pscustomobject]@{ Value = $root; Label = "<addon>" })
        }
    }
    foreach ($replacement in @($replacements.ToArray() | Sort-Object { $_.Value.Length } -Descending)) {
        $safe = $safe.Replace($replacement.Value, $replacement.Label)
        $safe = $safe.Replace($replacement.Value.Replace('\', '/'), $replacement.Label)
    }
    $safe = [regex]::Replace($safe, '(?i)(["''])\\\\[^"'']+\1', '$1<path>$1')
    $safe = [regex]::Replace($safe, '(?i)\\\\[^\s;,)]+', '<path>')
    $safe = [regex]::Replace($safe, '(?i)(["''])//[^"'']+\1', '$1<path>$1')
    $safe = [regex]::Replace($safe, '(?i)(?<!:)//[^\s;,)]+', '<path>')
    $safe = [regex]::Replace($safe, '(?i)(["''])[A-Z]:[\\/][^"'']+\1', '$1<path>$1')
    $safe = [regex]::Replace($safe, '(?i)\b[A-Z]:[\\/][^\s;,)]+', '<path>')
    $safe = [regex]::Replace(
        $safe,
        '(?i)(["''])(?:\\(?!\\)|/(?!/))[^"'']+\1',
        '$1<path>$1')
    $safe = [regex]::Replace(
        $safe,
        '(?i)(?<![A-Z0-9:/\\])(?:\\(?!\\)|/(?!/))[^\s;,)]+',
        '<path>')
    $safe = [regex]::Replace($safe, '(?i)\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b', '<email>')
    $safe = [regex]::Replace($safe, '\b[0-9]{15,20}\b', '<id>')
    if ($safe.Length -gt 600) {
        $safe = $safe.Substring(0, 600)
    }
    return $safe
}

function ConvertTo-SafeArtifactScalar {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    if ($null -eq $Value -or $Value -is [bool] -or
        $Value -is [byte] -or $Value -is [int16] -or
        $Value -is [int32] -or $Value -is [int64] -or
        $Value -is [uint16] -or $Value -is [uint32] -or
        $Value -is [uint64] -or $Value -is [single] -or
        $Value -is [double] -or $Value -is [decimal]) {
        return $Value
    }
    return ConvertTo-SafeEvidenceLine `
        -Line ([string]$Value) `
        -GuardRoot $GuardRoot `
        -ProjectDirectory $ProjectDirectory `
        -ResolvedAddonRoots $ResolvedAddonRoots
}

function ConvertTo-SafeArtifactPropertyBag {
    param(
        [AllowNull()]$Bag,
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    $safeBag = [ordered]@{}
    if ($null -eq $Bag) {
        return [pscustomobject]$safeBag
    }
    foreach ($property in $Bag.PSObject.Properties) {
        $safeBag[$property.Name] = ConvertTo-SafeArtifactScalar `
            -Value $property.Value `
            -GuardRoot $GuardRoot `
            -ProjectDirectory $ProjectDirectory `
            -ResolvedAddonRoots $ResolvedAddonRoots
    }
    return [pscustomobject]$safeBag
}

function Get-SafeArtifactValidationSummary {
    param(
        [Parameter(Mandatory = $true)]$Validation,
        [Parameter(Mandatory = $true)][string]$ExpectedBuildSha,
        [Parameter(Mandatory = $true)][string]$ExpectedBuildUtc,
        [Parameter(Mandatory = $true)][string]$ExpectedBuildLabel,
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    $safeProblems = @($Validation.Problems | ForEach-Object {
        ConvertTo-SafeArtifactScalar `
            -Value $_ `
            -GuardRoot $GuardRoot `
            -ProjectDirectory $ProjectDirectory `
            -ResolvedAddonRoots $ResolvedAddonRoots
    })
    $safePhase17 = @($Validation.Phase17 | ForEach-Object {
        [pscustomobject]@{
            Id = ConvertTo-SafeArtifactScalar -Value $_.Id -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            Pass = [bool]$_.Pass
        }
    })
    $safePhase24 = @($Validation.Phase24 | ForEach-Object {
        [pscustomobject]@{
            Id = ConvertTo-SafeArtifactScalar -Value $_.Id -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            Pass = [bool]$_.Pass
            Accepted = [bool]$_.Accepted
            Status = ConvertTo-SafeArtifactScalar -Value $_.Status -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            Actual = ConvertTo-SafeArtifactScalar -Value $_.Actual -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        }
    })
    $safeStagedCleanup = @($Validation.StagedCleanup | ForEach-Object {
        [pscustomobject]@{
            Id = ConvertTo-SafeArtifactScalar -Value $_.Id -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            Pass = [bool]$_.Pass
            CaseStatus = ConvertTo-SafeArtifactScalar -Value $_.CaseStatus -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            ActiveGroupsStatus = ConvertTo-SafeArtifactScalar -Value $_.ActiveGroupsStatus -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            RuntimeFactionsStatus = ConvertTo-SafeArtifactScalar -Value $_.RuntimeFactionsStatus -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            RuntimeGroupPopulationSettledStatus = ConvertTo-SafeArtifactScalar -Value $_.RuntimeGroupPopulationSettledStatus -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            ExpectedZeroMemberGraceApplied = ConvertTo-SafeArtifactScalar -Value $_.ExpectedZeroMemberGraceApplied -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            OrphanActiveGroups = ConvertTo-SafeArtifactScalar -Value $_.OrphanActiveGroups -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            RuntimeFactionMismatches = ConvertTo-SafeArtifactScalar -Value $_.RuntimeFactionMismatches -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            ZeroMemberGraceCandidates = ConvertTo-SafeArtifactScalar -Value $_.ZeroMemberGraceCandidates -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            PendingPopulationGroups = ConvertTo-SafeArtifactScalar -Value $_.PendingPopulationGroups -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        }
    })
    $safeFocusedAssertions = @($Validation.FocusedAssertions | ForEach-Object {
        [pscustomobject]@{
            Id = ConvertTo-SafeArtifactScalar -Value $_.Id -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            Pass = [bool]$_.Pass
            Status = ConvertTo-SafeArtifactScalar -Value $_.Status -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
            Actual = ConvertTo-SafeArtifactScalar -Value $_.Actual -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        }
    })
    return [pscustomobject]@{
        Valid = [bool]$Validation.Valid
        Problems = $safeProblems
        RunId = ConvertTo-SafeArtifactScalar -Value $Validation.RunId -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        Profile = ConvertTo-SafeArtifactScalar -Value $Validation.Profile -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        ProofScope = ConvertTo-SafeArtifactScalar -Value $Validation.ProofScope -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        FullCertification = [bool]$Validation.FullCertification
        BuildProvenanceMatches = [string]$Validation.BuildSha -eq $ExpectedBuildSha -and
            [string]$Validation.BuildUtc -eq $ExpectedBuildUtc -and
            [string]$Validation.BuildLabel -eq $ExpectedBuildLabel
        StartedAtSecond = [int]$Validation.StartedAtSecond
        EndedAtSecond = [int]$Validation.EndedAtSecond
        CaseCount = [int]$Validation.CaseCount
        Pass = [int]$Validation.Pass
        Warn = [int]$Validation.Warn
        Fail = [int]$Validation.Fail
        Blocked = [int]$Validation.Blocked
        Skipped = [int]$Validation.Skipped
        CertificationRequired = [int]$Validation.CertificationRequired
        CertificationProven = [int]$Validation.CertificationProven
        CertificationFail = [int]$Validation.CertificationFail
        CertificationBlocked = [int]$Validation.CertificationBlocked
        CertificationWarn = [int]$Validation.CertificationWarn
        CertificationPassed = [bool]$Validation.CertificationPassed
        CorrectedCanaryContract = [bool]$Validation.CorrectedCanaryContract
        Trigger = ConvertTo-SafeArtifactScalar -Value $Validation.Trigger -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        ArtifactCount = [int]$Validation.ArtifactCount
        StateDiffRows = [int]$Validation.StateDiffRows
        NonzeroStateDiffRows = [int]$Validation.NonzeroStateDiffRows
        StateDiffManifestExact = [bool](
            $Validation.PSObject.Properties['StateDiffManifestExact'] -and
            $Validation.StateDiffManifestExact)
        Phase17 = $safePhase17
        Phase17Metrics = ConvertTo-SafeArtifactPropertyBag -Bag $Validation.Phase17Metrics -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        Phase24 = $safePhase24
        Phase24Metrics = ConvertTo-SafeArtifactPropertyBag -Bag $Validation.Phase24Metrics -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        StagedCleanup = $safeStagedCleanup
        FocusedCaseId = ConvertTo-SafeArtifactScalar -Value $Validation.FocusedCaseId -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        FocusedCaseStatus = ConvertTo-SafeArtifactScalar -Value $Validation.FocusedCaseStatus -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        FocusedAssertions = $safeFocusedAssertions
        CorrectedCanaryAssertionManifestExact = [bool](
            $Validation.PSObject.Properties['CorrectedCanaryAssertionManifestExact'] -and
            $Validation.CorrectedCanaryAssertionManifestExact)
        CorrectedCanaryCaseSetExact = [bool](
            $Validation.PSObject.Properties['CorrectedCanaryCaseSetExact'] -and
            $Validation.CorrectedCanaryCaseSetExact)
        CorrectedCanaryWarningContractExact = [bool](
            $Validation.PSObject.Properties['CorrectedCanaryWarningContractExact'] -and
            $Validation.CorrectedCanaryWarningContractExact)
        CorrectedCanaryBlockedContractExact = [bool](
            $Validation.PSObject.Properties['CorrectedCanaryBlockedContractExact'] -and
            $Validation.CorrectedCanaryBlockedContractExact)
        CorrectedCanaryOrphanContractExact = [bool](
            $Validation.PSObject.Properties['CorrectedCanaryOrphanContractExact'] -and
            $Validation.CorrectedCanaryOrphanContractExact)
        IntentionalMissionConvoyAdmissionDiagnosticsProven =
            [bool]$Validation.IntentionalMissionConvoyAdmissionDiagnosticsProven
        IntentionalMissionConvoySettlementDiagnosticProven =
            [bool]$Validation.IntentionalMissionConvoySettlementDiagnosticProven
        IntentionalMissionConvoyCorruptionDiagnosticsProven =
            [bool]$Validation.IntentionalMissionConvoyCorruptionDiagnosticsProven
        IntentionalMissionConvoyWatchdogDiagnosticProven =
            [bool]$Validation.IntentionalMissionConvoyWatchdogDiagnosticProven
        FinalOrphanCleanupPass = [bool]$Validation.FinalOrphanCleanupPass
        FinalOrphanActiveGroups = ConvertTo-SafeArtifactScalar -Value $Validation.FinalOrphanActiveGroups -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
    }
}

function Read-SharedFileText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [long]$MaximumBytes = 0
    )

    $stream = $null
    $reader = $null
    try {
        $stream = [IO.File]::Open(
            $Path,
            [IO.FileMode]::Open,
            [IO.FileAccess]::Read,
            ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
        if ($MaximumBytes -gt 0 -and $stream.Length -gt $MaximumBytes) {
            [void]$stream.Seek(-$MaximumBytes, [IO.SeekOrigin]::End)
        }
        $reader = New-Object IO.StreamReader($stream)
        return $reader.ReadToEnd()
    }
    finally {
        if ($reader) {
            $reader.Dispose()
        }
        elseif ($stream) {
            $stream.Dispose()
        }
    }
}

function Get-GuardLogTail {
    param([Parameter(Mandatory = $true)][string]$GuardRoot)

    $logRoot = Join-Path $GuardRoot "logs"
    if (-not (Test-Path -LiteralPath $logRoot -PathType Container)) {
        return ""
    }

    $parts = New-Object Collections.Generic.List[string]
    $logs = @(Get-ChildItem -LiteralPath $logRoot -Recurse -File -Force -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -in @(".log", ".rpt") })
    foreach ($log in $logs) {
        try {
            $parts.Add((Read-SharedFileText -Path $log.FullName -MaximumBytes 4194304))
        }
        catch {
            continue
        }
    }
    return $parts -join [Environment]::NewLine
}

function Get-GuardLaunchFailureEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    $tail = Get-GuardLogTail -GuardRoot $GuardRoot
    if ([string]::IsNullOrWhiteSpace($tail)) {
        return "no guarded runtime log was produced"
    }
    $lines = @($tail -split "`r?`n" | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_)
    })
    $diagnosticLines = @($lines | Where-Object {
        $_ -match '(?i)(?:SCRIPT\s+\(E\)|\(E\):|\bERROR\b|\bFATAL\b|\bCRASH\b|failed|failure|cannot|unable|missing|invalid|gproj)'
    } | Select-Object -Last 8)
    if ($diagnosticLines.Count -eq 0) {
        $diagnosticLines = @($lines | Select-Object -Last 8)
    }
    $safeLines = New-Object Collections.Generic.List[string]
    foreach ($line in $diagnosticLines) {
        $safeLine = ConvertTo-SafeEvidenceLine `
            -Line ([string]$line) `
            -GuardRoot $GuardRoot `
            -ProjectDirectory $ProjectDirectory `
            -ResolvedAddonRoots $ResolvedAddonRoots
        if (-not [string]::IsNullOrWhiteSpace($safeLine)) {
            $safeLines.Add($safeLine)
        }
    }
    if ($safeLines.Count -eq 0) {
        return "guarded runtime log contained no readable diagnostics"
    }
    return $safeLines.ToArray() -join " || "
}

function Get-SafeSnapshotEntries {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$ExcludedRoots = @()
    )

    $rootFull = [IO.Path]::GetFullPath($Root)
    $result = New-Object 'Collections.Generic.Dictionary[string,object]' ([StringComparer]::OrdinalIgnoreCase)
    $pending = New-Object Collections.Generic.Queue[string]
    $pending.Enqueue($rootFull)
    while ($pending.Count -gt 0) {
        $directory = $pending.Dequeue()
        foreach ($item in @(Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop)) {
            $itemFull = [IO.Path]::GetFullPath($item.FullName)
            if (-not (Test-ContainedPath -Root $rootFull -Candidate $itemFull)) {
                throw "A snapshot entry escaped its explicit monitoring root."
            }
            $excluded = $false
            foreach ($excludedRoot in $ExcludedRoots) {
                if (Test-ContainedPath -Root $excludedRoot -Candidate $itemFull -AllowEqual) {
                    $excluded = $true
                    break
                }
            }
            if ($excluded) {
                continue
            }
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "A monitoring tree must not contain reparse points."
            }
            $length = [long]0
            $lastWriteTicks = [long]0
            if (-not $item.PSIsContainer) {
                $length = [long]$item.Length
                $lastWriteTicks = $item.LastWriteTimeUtc.Ticks
            }
            $result[$itemFull] = [pscustomobject]@{
                IsDirectory = [bool]$item.PSIsContainer
                Length = $length
                LastWriteTicks = $lastWriteTicks
            }
            if ($item.PSIsContainer) {
                $pending.Enqueue($itemFull)
            }
        }
    }
    return $result
}

function New-RootSnapshot {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$ExcludedRoots = @()
    )

    $fullRoot = Resolve-ExistingPath -Path $Root -Kind Container
    Assert-NoReparsePathAncestry -Path $fullRoot
    $resolvedExclusions = New-Object Collections.Generic.List[string]
    foreach ($excludedRoot in $ExcludedRoots) {
        if ([string]::IsNullOrWhiteSpace([string]$excludedRoot)) {
            continue
        }
        $resolvedExclusion = [IO.Path]::GetFullPath($excludedRoot)
        if ($resolvedExclusion.Equals($fullRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "A monitoring root must not be fully excluded."
        }
        if (-not $resolvedExclusions.Contains($resolvedExclusion)) {
            $resolvedExclusions.Add($resolvedExclusion)
        }
    }
    return [pscustomobject]@{
        Root = $fullRoot
        ExcludedRoots = $resolvedExclusions.ToArray()
        Entries = Get-SafeSnapshotEntries `
            -Root $fullRoot `
            -ExcludedRoots $resolvedExclusions.ToArray()
    }
}

function Get-RootSnapshotDelta {
    param([Parameter(Mandatory = $true)]$Snapshot)

    if (-not (Test-Path -LiteralPath $Snapshot.Root -PathType Container)) {
        return [pscustomobject]@{
            NewEntries = 0
            ModifiedFiles = 0
            DeletedEntries = 0
            MissingRoot = 1
        }
    }
    Assert-NoReparsePathAncestry -Path $Snapshot.Root
    $rootItem = Get-Item -LiteralPath $Snapshot.Root -Force -ErrorAction Stop
    if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "A monitoring root became a reparse point."
    }
    $current = Get-SafeSnapshotEntries `
        -Root $Snapshot.Root `
        -ExcludedRoots $Snapshot.ExcludedRoots
    $newEntries = 0
    $modifiedFiles = 0
    $deletedEntries = 0
    foreach ($path in $current.Keys) {
        if (-not $Snapshot.Entries.ContainsKey($path)) {
            $newEntries++
            continue
        }
        $before = $Snapshot.Entries[$path]
        $after = $current[$path]
        if ($before.IsDirectory -ne $after.IsDirectory -or
            (-not $after.IsDirectory -and
                ($before.Length -ne $after.Length -or
                    $before.LastWriteTicks -ne $after.LastWriteTicks))) {
            $modifiedFiles++
        }
    }
    foreach ($path in $Snapshot.Entries.Keys) {
        if (-not $current.ContainsKey($path)) {
            $deletedEntries++
        }
    }
    return [pscustomobject]@{
        NewEntries = $newEntries
        ModifiedFiles = $modifiedFiles
        DeletedEntries = $deletedEntries
        MissingRoot = 0
    }
}

function Test-ContainedPath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Candidate,
        [switch]$AllowEqual
    )

    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $candidateFull = [IO.Path]::GetFullPath($Candidate).TrimEnd('\', '/')
    if ($AllowEqual -and $candidateFull.Equals($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    return $candidateFull.StartsWith(
        $rootFull + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)
}

function Assert-NoReparsePathAncestry {
    param([Parameter(Mandatory = $true)][string]$Path)

    $cursor = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    while (-not (Test-Path -LiteralPath $cursor)) {
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            throw "A safe existing ancestor could not be resolved for the guarded path."
        }
        $cursor = $parent
    }
    while (-not [string]::IsNullOrWhiteSpace($cursor)) {
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "A guarded path ancestor must not be a reparse point."
        }
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $cursor = $parent
    }
}

function Read-GuardOwnership {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$GuardBase
    )

    try {
        $resolved = [IO.Path]::GetFullPath($Directory)
        if (-not (Test-ContainedPath -Root $GuardBase -Candidate $resolved)) {
            return $null
        }
        $leaf = Split-Path -Leaf $resolved
        if ($leaf -notmatch '^PartisanCampaignDebugGuard_([0-9a-f]{32})$') {
            return $null
        }
        if (-not (Test-Path -LiteralPath $resolved -PathType Container)) {
            return $null
        }
        $directoryItem = Get-Item -LiteralPath $resolved -Force
        if (($directoryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }

        $sentinel = Join-Path $resolved ".partisan-campaign-debug-owner"
        if (-not (Test-Path -LiteralPath $sentinel -PathType Leaf)) {
            return $null
        }
        $sentinelItem = Get-Item -LiteralPath $sentinel -Force
        if (($sentinelItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }
        $ownership = Read-SharedFileText -Path $sentinel | ConvertFrom-Json
        $nonce = [string]$matches[1]
        if ([int]$ownership.version -ne 1 -or
            [string]$ownership.nonce -cne $nonce -or
            [string]$ownership.guardLeaf -cne $leaf -or
            [int]$ownership.ownerPid -le 0 -or
            [string]::IsNullOrWhiteSpace([string]$ownership.ownerStartUtc) -or
            [string]::IsNullOrWhiteSpace([string]$ownership.createdUtc)) {
            return $null
        }

        $ownerStartUtc = [DateTime]::Parse(
            [string]$ownership.ownerStartUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        $createdUtc = [DateTime]::Parse(
            [string]$ownership.createdUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        return [pscustomobject]@{
            Directory = $resolved
            Sentinel = $sentinel
            Nonce = $nonce
            GuardLeaf = $leaf
            OwnerPid = [int]$ownership.ownerPid
            OwnerStartUtc = $ownerStartUtc
            CreatedUtc = $createdUtc
        }
    }
    catch {
        return $null
    }
}

function Test-ProcessIdentityAlive {
    param(
        [Parameter(Mandatory = $true)][int]$ProcessId,
        [Parameter(Mandatory = $true)][datetime]$StartUtc
    )

    try {
        $candidate = Get-Process -Id $ProcessId -ErrorAction Stop
        return $candidate.StartTime.ToUniversalTime().Ticks -eq $StartUtc.ToUniversalTime().Ticks
    }
    catch {
        return $false
    }
}

function Remove-ExactOwnedGuard {
    param(
        [Parameter(Mandatory = $true)]$Ownership,
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [int]$Attempts = 4
    )

    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        if (-not (Test-Path -LiteralPath $Ownership.Directory)) {
            return $true
        }
        $current = Read-GuardOwnership -Directory $Ownership.Directory -GuardBase $GuardBase
        if (-not $current -or
            $current.Nonce -cne $Ownership.Nonce -or
            $current.OwnerPid -ne $Ownership.OwnerPid -or
            $current.OwnerStartUtc.Ticks -ne $Ownership.OwnerStartUtc.Ticks) {
            return $false
        }
        $reparseDescendant = Get-ChildItem -LiteralPath $current.Directory -Recurse -Force -ErrorAction Stop |
            Where-Object { ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 } |
            Select-Object -First 1
        if ($reparseDescendant) {
            return $false
        }
        try {
            Remove-Item -LiteralPath $current.Directory -Recurse -Force -ErrorAction Stop
        }
        catch {
            if ($attempt -lt $Attempts) {
                Start-Sleep -Milliseconds 250
            }
        }
    }
    return -not (Test-Path -LiteralPath $Ownership.Directory)
}

function Remove-StaleOwnedGuards {
    param(
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [timespan]$MinimumAge = ([timespan]::FromHours(6))
    )

    if (-not (Test-Path -LiteralPath $GuardBase -PathType Container)) {
        return 0
    }
    $baseItem = Get-Item -LiteralPath $GuardBase -Force
    if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Guard base must not be a reparse point."
    }

    $removed = 0
    foreach ($candidate in @(Get-ChildItem -LiteralPath $GuardBase -Directory -Force -ErrorAction Stop)) {
        $ownership = Read-GuardOwnership -Directory $candidate.FullName -GuardBase $GuardBase
        if (-not $ownership) {
            continue
        }
        if (([DateTime]::UtcNow - $ownership.CreatedUtc) -lt $MinimumAge) {
            continue
        }
        if (Test-ProcessIdentityAlive -ProcessId $ownership.OwnerPid -StartUtc $ownership.OwnerStartUtc) {
            continue
        }
        if (Remove-ExactOwnedGuard -Ownership $ownership -GuardBase $GuardBase) {
            $removed++
        }
    }
    return $removed
}

function Get-EngineProcessRows {
    $processNames = @(
        "ArmaReforger",
        "ArmaReforger_BE",
        "ArmaReforgerSteam",
        "ArmaReforgerSteamDiag",
        "ArmaReforgerServer",
        "ArmaReforgerServerDiag",
        "ArmaReforgerWorkbench",
        "ArmaReforgerWorkbenchDiag",
        "ArmaReforgerWorkbenchSteamDiag",
        "CrashReporter"
    )

    return @(Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $processNames -contains $_.ProcessName })
}

function Update-OwnedProcesses {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Owned,
        [Parameter(Mandatory = $true)][int]$RootProcessId,
        [Parameter(Mandatory = $true)][datetime]$RootStartUtc,
        $Job
    )

    $rows = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $jobProcessIds = @()
    if ($Job) {
        try {
            $jobProcessIds = @($Job.GetProcessIds())
        }
        catch {
            $jobProcessIds = @()
        }
    }

    foreach ($jobProcessId in $jobProcessIds) {
        $candidateId = [int]$jobProcessId
        if ($Owned.ContainsKey($candidateId)) {
            continue
        }
        try {
            $candidate = Get-Process -Id $candidateId -ErrorAction Stop
            $candidateStart = $candidate.StartTime.ToUniversalTime()
            if ($candidateStart -ge $RootStartUtc) {
                $Owned[$candidateId] = $candidateStart
            }
        }
        catch {
            continue
        }
    }

    $changed = $true
    while ($changed) {
        $changed = $false
        foreach ($row in $rows) {
            $candidateId = [int]$row.ProcessId
            $parentId = [int]$row.ParentProcessId
            if ($Owned.ContainsKey($candidateId) -or -not $Owned.ContainsKey($parentId)) {
                continue
            }
            if (-not (Test-ProcessIdentityAlive -ProcessId $parentId -StartUtc $Owned[$parentId])) {
                continue
            }
            try {
                $candidate = Get-Process -Id $candidateId -ErrorAction Stop
                $candidateStart = $candidate.StartTime.ToUniversalTime()
                if ($candidateStart -ge $RootStartUtc) {
                    $Owned[$candidateId] = $candidateStart
                    $changed = $true
                }
            }
            catch {
                continue
            }
        }
    }

    if (-not $Owned.ContainsKey($RootProcessId)) {
        try {
            $rootProcess = Get-Process -Id $RootProcessId -ErrorAction Stop
            $actualRootStart = $rootProcess.StartTime.ToUniversalTime()
            if ($actualRootStart.Ticks -eq $RootStartUtc.Ticks) {
                $Owned[$RootProcessId] = $actualRootStart
                $changed = $true
            }
        }
        catch {
            return
        }
    }
}

function Stop-OwnedProcesses {
    param([Parameter(Mandatory = $true)][hashtable]$Owned)

    $rows = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $depths = @{}
    foreach ($processId in @($Owned.Keys)) {
        $depth = 0
        $cursor = [int]$processId
        while ($depth -lt 32) {
            $row = $rows | Where-Object { [int]$_.ProcessId -eq $cursor } | Select-Object -First 1
            if (-not $row -or -not $Owned.ContainsKey([int]$row.ParentProcessId)) {
                break
            }
            $cursor = [int]$row.ParentProcessId
            $depth++
        }
        $depths[[int]$processId] = $depth
    }

    foreach ($processId in @($Owned.Keys | Sort-Object { $depths[[int]$_] } -Descending)) {
        try {
            $process = Get-Process -Id ([int]$processId) -ErrorAction Stop
            $expectedStart = $Owned[[int]$processId]
            if ($null -ne $expectedStart) {
                $actualStart = $process.StartTime.ToUniversalTime()
                if ($actualStart.Ticks -ne $expectedStart.ToUniversalTime().Ticks) {
                    continue
                }
            }
            $process.Kill()
        }
        catch {
            continue
        }
    }
}

function Test-ExactNativeArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$CommandLine,
        [Parameter(Mandatory = $true)][string]$ExpectedExecutable,
        [Parameter(Mandatory = $true)][string[]]$ExpectedArguments
    )

    $tokens = @([PartisanNativeCommandLine]::Split($CommandLine))
    if ($tokens.Count -ne ($ExpectedArguments.Count + 1)) {
        return $false
    }
    try {
        $actualExecutable = [IO.Path]::GetFullPath($tokens[0])
        $expectedExecutablePath = [IO.Path]::GetFullPath($ExpectedExecutable)
    }
    catch {
        return $false
    }
    if (-not $actualExecutable.Equals($expectedExecutablePath, [StringComparison]::OrdinalIgnoreCase)) {
        return $false
    }
    for ($index = 0; $index -lt $ExpectedArguments.Count; $index++) {
        if (-not ([string]$tokens[$index + 1]).Equals(
            [string]$ExpectedArguments[$index],
            [StringComparison]::Ordinal)) {
            return $false
        }
    }
    return $true
}

function Invoke-IsolatedCleanupPhase {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Action,
        [Parameter(Mandatory = $true)]$Errors
    )

    try {
        & $Action
    }
    catch {
        [void]$Errors.Add($Name)
    }
}

function Get-FileSignature {
    param([Parameter(Mandatory = $true)][string[]]$Paths)

    $parts = New-Object Collections.Generic.List[string]
    foreach ($path in $Paths) {
        $file = Get-Item -LiteralPath $path -Force
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        $parts.Add("$($file.Length):$($file.LastWriteTimeUtc.Ticks):$hash")
    }
    return $parts -join "|"
}

function Find-Case {
    param(
        [Parameter(Mandatory = $true)]$Run,
        [Parameter(Mandatory = $true)][string]$CaseId
    )

    return @($Run.m_aCases | Where-Object { $_.m_sCaseId -eq $CaseId })
}

function Find-Assertion {
    param(
        [Parameter(Mandatory = $true)]$Case,
        [Parameter(Mandatory = $true)][string]$AssertionId
    )

    return @($Case.m_aAssertions | Where-Object { $_.m_sAssertionId -eq $AssertionId })
}

function Get-MetricValue {
    param(
        [Parameter(Mandatory = $true)]$Metrics,
        [Parameter(Mandatory = $true)][string]$MetricId
    )

    $matches = @($Metrics | Where-Object { $_.m_sMetricId -eq $MetricId })
    if ($matches.Count -ne 1) {
        return $null
    }
    return [string]$matches[0].m_sValue
}

function Test-RunMetric {
    param(
        [Parameter(Mandatory = $true)]$Metrics,
        [Parameter(Mandatory = $true)][string]$MetricId,
        [Parameter(Mandatory = $true)][string]$Value,
        [Parameter(Mandatory = $true)][string]$Unit
    )

    $matches = @($Metrics | Where-Object { $_.m_sMetricId -eq $MetricId })
    return $matches.Count -eq 1 -and
        [string]$matches[0].m_sValue -eq $Value -and
        [string]$matches[0].m_sUnit -eq $Unit -and
        [string]$matches[0].m_sFeature -eq "campaign_debug" -and
        [string]$matches[0].m_sStage -eq "run"
}

function Test-ExactPassingAssertion {
    param(
        [Parameter(Mandatory = $true)]$Case,
        [Parameter(Mandatory = $true)][string]$AssertionId
    )

    $matches = @(Find-Assertion -Case $Case -AssertionId $AssertionId)
    return $matches.Count -eq 1 -and $matches[0].m_sStatus -eq "PASS"
}

function Get-ExactAssertionStatus {
    param(
        [Parameter(Mandatory = $true)]$Case,
        [Parameter(Mandatory = $true)][string]$AssertionId
    )

    $matches = @(Find-Assertion -Case $Case -AssertionId $AssertionId)
    if ($matches.Count -ne 1) {
        return $null
    }
    return [string]$matches[0].m_sStatus
}

function Get-ExactAssertionActual {
    param(
        [Parameter(Mandatory = $true)]$Case,
        [Parameter(Mandatory = $true)][string]$AssertionId
    )

    $matches = @(Find-Assertion -Case $Case -AssertionId $AssertionId)
    if ($matches.Count -ne 1) {
        return $null
    }
    return [string]$matches[0].m_sActual
}

function Get-FocusedForceAuthorityAssertionIds {
    return @(
        "combat_presence.aggregate",
        "combat_presence.empty_vehicle",
        "combat_presence.authoritative_samples",
        "combat_presence.rejected_rows",
        "combat_presence.heat_lifecycle",
        "combat_presence.schema62_migration",
        "combat_presence.schema63_restore",
        "combat_presence.malformed_fail_cold",
        "combat_presence.deterministic_diagnostics",
        "ownership_transition.aggregate",
        "ownership_transition.military",
        "ownership_transition.political",
        "ownership_transition.recapture",
        "ownership_transition.replay",
        "ownership_transition.restore",
        "ownership_transition.restore_queue_order",
        "ownership_transition.persistence_deadline",
        "ownership_transition.projection_revision",
        "ownership_transition.location_identity",
        "ownership_transition.linked_support",
        "ownership_transition.causes",
        "ownership_transition.security_fail_closed",
        "ownership_transition.migration_retention",
        "town_influence.aggregate",
        "town_influence.scaling",
        "town_influence.hysteresis",
        "town_influence.idempotency",
        "town_influence.projection",
        "town_influence.population",
        "town_influence.rejection",
        "town_influence.ownership_authority",
        "town_influence.external_completion",
        "town_influence.pre64_invader",
        "town_influence.migration",
        "town_influence.current_restore"
    )
}

function Get-CorrectedCanaryCaseManifest {
    param([Parameter(Mandatory = $true)][string]$RunId)

    return @(
        [pscustomobject]@{ Id = 'preflight.state_isolation'; Status = 'PASS'; Category = 'preflight'; Feature = 'campaign_debug'; Stage = 'state_isolation' },
        [pscustomobject]@{ Id = 'post_case_cleanup.preflight_state_isolation'; Status = 'PASS'; Category = 'cleanup'; Feature = 'campaign_debug'; Stage = 'post_case' },
        [pscustomobject]@{ Id = 'cleanup.prefixed_state.start_preflight.hst_debug_'; Status = 'PASS'; Category = 'cleanup'; Feature = 'campaign_debug'; Stage = 'prefix_cleanup' },
        [pscustomobject]@{ Id = 'early_mechanics.force_authority'; Status = 'PASS'; Category = 'early_mechanics'; Feature = 'force_authority'; Stage = 'focused_force_authority' },
        [pscustomobject]@{ Id = 'post_case_cleanup.early_mechanics_force_authority'; Status = 'PASS'; Category = 'cleanup'; Feature = 'force_authority'; Stage = 'post_case' },
        [pscustomobject]@{ Id = 'cleanup.enemy_orders.run_completion'; Status = 'PASS'; Category = 'cleanup'; Feature = 'enemy_commander'; Stage = 'typed_settlement' },
        [pscustomobject]@{ Id = "cleanup.prefixed_state.run_completion.hst_debug_$RunId"; Status = 'PASS'; Category = 'cleanup'; Feature = 'campaign_debug'; Stage = 'prefix_cleanup' },
        [pscustomobject]@{ Id = 'cleanup.prefixed_state.run_completion_persistence_smoke_cleanup.hst_smoke'; Status = 'PASS'; Category = 'cleanup'; Feature = 'campaign_debug'; Stage = 'prefix_cleanup' },
        [pscustomobject]@{ Id = 'cleanup.player_marker_completion'; Status = 'WARN'; Category = 'cleanup'; Feature = 'player_markers'; Stage = 'final' },
        [pscustomobject]@{ Id = 'cleanup.run_leak_snapshot'; Status = 'PASS'; Category = 'cleanup'; Feature = 'campaign_debug'; Stage = 'final' },
        [pscustomobject]@{ Id = 'cleanup.state_isolation_restore'; Status = 'BLOCKED'; Category = 'cleanup'; Feature = 'campaign_debug'; Stage = 'state_restore' }
    )
}

function Get-CorrectedCanaryAssertionManifest {
    param([Parameter(Mandatory = $true)][string]$RunId)

    $rows = New-Object Collections.Generic.List[object]
    $append = {
        param(
            [string]$CaseId,
            [string[]]$AssertionIds,
            [string[]]$NoncertifyingIds = @(),
            [string[]]$WarningIds = @(),
            [string[]]$BlockedIds = @()
        )
        foreach ($assertionId in $AssertionIds) {
            [void]$rows.Add([pscustomobject]@{
                CaseId = $CaseId
                Id = $assertionId
                Status = if ($BlockedIds -ccontains $assertionId) {
                    'BLOCKED'
                }
                elseif ($WarningIds -ccontains $assertionId) {
                    'WARN'
                }
                else {
                    'PASS'
                }
                CountsTowardCertification = $NoncertifyingIds -cnotcontains $assertionId
            })
        }
    }

    & $append 'preflight.state_isolation' @(
        'isolation.development_world',
        'isolation.distinct_state',
        'isolation.persistence_suspended')
    $postCleanupIds = @(
        'post_cleanup.active_missions',
        'post_cleanup.mission_assets',
        'post_cleanup.active_groups',
        'post_cleanup.runtime_factions',
        'post_cleanup.runtime_group_primary_spawn',
        'post_cleanup.runtime_group_population_settled',
        'post_cleanup.markers',
        'post_cleanup.backing_markers')
    & $append 'post_case_cleanup.preflight_state_isolation' $postCleanupIds
    $prefixCleanupIds = @(
        'cleanup.prefix.before_count',
        'cleanup.prefix.radio_fixture_world',
        'cleanup.prefix.after_count',
        'cleanup.prefix.tagged_world_entities',
        'cleanup.prefix.marker_rebuild')
    & $append 'cleanup.prefixed_state.start_preflight.hst_debug_' $prefixCleanupIds
    $focusedIds = @(Get-FocusedForceAuthorityAssertionIds)
    & $append 'early_mechanics.force_authority' $focusedIds @(
        'town_influence.external_completion')
    & $append 'post_case_cleanup.early_mechanics_force_authority' $postCleanupIds
    & $append 'cleanup.enemy_orders.run_completion' @(
        'cleanup.enemy_orders.typed_settlement',
        'cleanup.enemy_orders.open',
        'cleanup.enemy_orders.runtime')
    & $append "cleanup.prefixed_state.run_completion.hst_debug_$RunId" $prefixCleanupIds
    & $append `
        'cleanup.prefixed_state.run_completion_persistence_smoke_cleanup.hst_smoke' `
        $prefixCleanupIds
    & $append 'cleanup.player_marker_completion' @(
        'cleanup.player_marker.config',
        'cleanup.player_marker.live') @('cleanup.player_marker.live') @(
        'cleanup.player_marker.live')
    & $append 'cleanup.run_leak_snapshot' @(
        'cleanup.current_mission_id',
        'cleanup.debug_prefixed_records',
        'cleanup.smoke_prefixed_records',
        'cleanup.active_mission_delta',
        'cleanup.pending_player_support',
        'cleanup.open_enemy_orders',
        'cleanup.orphan_active_groups',
        'cleanup.runtime_factions',
        'cleanup.runtime_group_primary_spawn',
        'cleanup.runtime_group_population_settled',
        'cleanup.marker_orphans',
        'cleanup.backing_markers') @('cleanup.smoke_prefixed_records')
    & $append 'cleanup.state_isolation_restore' @(
        'isolation.radio_fixture_world',
        'isolation.snapshot',
        'isolation.state_restore',
        'isolation.persistence_restore',
        'isolation.world_scope') @('isolation.world_scope') @() @(
        'isolation.world_scope')

    return $rows.ToArray()
}

function Get-CampaignDebugStateDiffLabels {
    return @(
        'elapsed',
        'money',
        'HR',
        'training',
        'war level',
        'active missions',
        'objectives',
        'runtime vehicles',
        'mission assets',
        'active groups',
        'support requests',
        'enemy orders',
        'markers',
        'garage vehicles',
        'arsenal items',
        'civilian zones',
        'strategic events',
        'undercover records')
}

function Get-CampaignDebugStateDiffValidation {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$RunId
    )

    $expectedLabels = @(Get-CampaignDebugStateDiffLabels)
    $normalizedText = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    if ($normalizedText.EndsWith("`n", [StringComparison]::Ordinal)) {
        # WriteAllLines emits one terminal newline. Remove exactly one so any
        # additional blank line remains an unexpected grammar row.
        $normalizedText = $normalizedText.Substring(0, $normalizedText.Length - 1)
    }
    $lines = @($normalizedText -split "`n")
    $candidateRows = if ($lines.Count -gt 2) {
        @($lines[2..($lines.Count - 1)])
    }
    else {
        @()
    }

    $headerExact = $lines.Count -ge 2 -and
        $lines[0] -ceq 'Partisan campaign debug state diff' -and
        $lines[1] -ceq "run $RunId"
    $labelsExact = $candidateRows.Count -eq $expectedLabels.Count
    $grammarExact = $labelsExact
    $arithmeticExact = $labelsExact
    $zeroDeltaExact = $labelsExact
    $nonzero = 0
    for ($index = 0; $index -lt $candidateRows.Count; $index++) {
        $row = [string]$candidateRows[$index]
        $censusMatch = [regex]::Match(
            $row,
            ' \| delta (?<delta>-?\d+)$',
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        if ($censusMatch.Success) {
            $censusDelta = [int64]0
            if ([int64]::TryParse(
                    $censusMatch.Groups['delta'].Value,
                    [Globalization.NumberStyles]::AllowLeadingSign,
                    [Globalization.CultureInfo]::InvariantCulture,
                    [ref]$censusDelta) -and $censusDelta -ne 0) {
                $nonzero++
            }
        }

        if ($index -ge $expectedLabels.Count) {
            $labelsExact = $false
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        $prefix = $expectedLabels[$index] + ' '
        if (-not $row.StartsWith($prefix, [StringComparison]::Ordinal)) {
            $labelsExact = $false
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        $valueText = $row.Substring($prefix.Length)
        $valueMatch = [regex]::Match(
            $valueText,
            '^(?<before>-?\d+) -> (?<after>-?\d+) \| delta (?<delta>-?\d+)$',
            [Text.RegularExpressions.RegexOptions]::CultureInvariant)
        if (-not $valueMatch.Success) {
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        $before = [int64]0
        $after = [int64]0
        $delta = [int64]0
        $integerGrammarExact =
            [int64]::TryParse(
                $valueMatch.Groups['before'].Value,
                [Globalization.NumberStyles]::AllowLeadingSign,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$before) -and
            [int64]::TryParse(
                $valueMatch.Groups['after'].Value,
                [Globalization.NumberStyles]::AllowLeadingSign,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$after) -and
            [int64]::TryParse(
                $valueMatch.Groups['delta'].Value,
                [Globalization.NumberStyles]::AllowLeadingSign,
                [Globalization.CultureInfo]::InvariantCulture,
                [ref]$delta)
        if (-not $integerGrammarExact) {
            $grammarExact = $false
            $arithmeticExact = $false
            continue
        }
        if (([decimal]$after - [decimal]$before) -ne [decimal]$delta) {
            $arithmeticExact = $false
        }
        if ($delta -ne 0) {
            $zeroDeltaExact = $false
        }
    }
    return [pscustomobject][ordered]@{
        HeaderExact = $headerExact
        LabelsExact = $labelsExact
        GrammarExact = $grammarExact
        ArithmeticExact = $arithmeticExact
        ZeroDeltaExact = $zeroDeltaExact
        ContractExact = $headerExact -and
            $candidateRows.Count -eq $expectedLabels.Count -and
            $labelsExact -and $grammarExact -and $arithmeticExact -and
            $zeroDeltaExact
        RowCount = $candidateRows.Count
        NonzeroRowCount = $nonzero
    }
}

function Test-CampaignDebugArtifacts {
    param(
        [Parameter(Mandatory = $true)][string]$JsonPath,
        [Parameter(Mandatory = $true)][string]$SummaryPath,
        [Parameter(Mandatory = $true)][string]$StateDiffPath,
        [Parameter(Mandatory = $true)][string]$ExpectedSha,
        [Parameter(Mandatory = $true)][string]$ExpectedUtc,
        [Parameter(Mandatory = $true)][string]$ExpectedLabel,
        [ValidateSet("full_certification", "force_authority")]
        [string]$ExpectedProfile = "full_certification",
        [switch]$RequireCorrectedCanaryContract
    )

    $jsonText = Read-SharedFileText -Path $JsonPath
    $summaryText = Read-SharedFileText -Path $SummaryPath
    $stateDiffText = Read-SharedFileText -Path $StateDiffPath
    $run = $jsonText | ConvertFrom-Json
    $problems = New-Object Collections.Generic.List[string]

    if ([string]::IsNullOrWhiteSpace([string]$run.m_sRunId)) {
        $problems.Add("run-id")
    }
    if ($run.m_sProfile -ne $ExpectedProfile) {
        $problems.Add("profile")
    }
    if ($run.m_sBuildSha -ne $ExpectedSha -or
        $run.m_sBuildUtc -ne $ExpectedUtc -or
        $run.m_sBuildLabel -ne $ExpectedLabel) {
        $problems.Add("build-provenance")
    }
    if (($ExpectedProfile -eq "full_certification" -and
            [int]$run.m_iEndedAtSecond -le 0) -or
        [int]$run.m_iEndedAtSecond -lt [int]$run.m_iStartedAtSecond) {
        $problems.Add("run-ended")
    }

    $metricSha = Get-MetricValue -Metrics $run.m_aMetrics -MetricId "run.build.sha"
    $metricUtc = Get-MetricValue -Metrics $run.m_aMetrics -MetricId "run.build.utc"
    $metricLabel = Get-MetricValue -Metrics $run.m_aMetrics -MetricId "run.build.label"
    $trigger = Get-MetricValue -Metrics $run.m_aMetrics -MetricId "run.trigger"
    if ($metricSha -ne $ExpectedSha -or $metricUtc -ne $ExpectedUtc -or
        $metricLabel -ne $ExpectedLabel -or
        -not (Test-RunMetric -Metrics $run.m_aMetrics -MetricId "run.build.sha" -Value $ExpectedSha -Unit "sha") -or
        -not (Test-RunMetric -Metrics $run.m_aMetrics -MetricId "run.build.utc" -Value $ExpectedUtc -Unit "utc") -or
        -not (Test-RunMetric -Metrics $run.m_aMetrics -MetricId "run.build.label" -Value $ExpectedLabel -Unit "label")) {
        $problems.Add("build-metrics")
    }
    if ($trigger -ne "cli_autostart" -or
        -not (Test-RunMetric -Metrics $run.m_aMetrics -MetricId "run.trigger" -Value "cli_autostart" -Unit "source")) {
        $problems.Add("cli-trigger")
    }

    $cases = @($run.m_aCases)
    $statusCounts = @{
        PASS = @($cases | Where-Object { $_.m_sStatus -eq "PASS" }).Count
        WARN = @($cases | Where-Object { $_.m_sStatus -eq "WARN" }).Count
        FAIL = @($cases | Where-Object { $_.m_sStatus -eq "FAIL" }).Count
        BLOCKED = @($cases | Where-Object { $_.m_sStatus -eq "BLOCKED" }).Count
        SKIPPED = @($cases | Where-Object { $_.m_sStatus -eq "SKIPPED" }).Count
    }
    if ($statusCounts.PASS -ne [int]$run.m_iPassCount -or
        $statusCounts.WARN -ne [int]$run.m_iWarnCount -or
        $statusCounts.FAIL -ne [int]$run.m_iFailCount -or
        $statusCounts.BLOCKED -ne [int]$run.m_iBlockedCount -or
        $statusCounts.SKIPPED -ne [int]$run.m_iSkippedCount) {
        $problems.Add("case-totals")
    }

    $certTotal = [int]$run.m_iCertificationProvenCount +
        [int]$run.m_iCertificationFailCount +
        [int]$run.m_iCertificationBlockedCount +
        [int]$run.m_iCertificationWarnCount
    if ($certTotal -ne [int]$run.m_iCertificationRequiredCount) {
        $problems.Add("certification-totals")
    }

    if ($ExpectedProfile -eq "force_authority") {
        $assertionManifestExact = $false
        $caseSetExact = $false
        $warningContractExact = $false
        $blockedContractExact = $false
        $orphanContractExact = $false
        if ([bool]$run.m_bCertificationPassed) {
            $problems.Add("focused-proof-claimed-full-certification")
        }

        $focusedCases = @(Find-Case -Run $run -CaseId "early_mechanics.force_authority")
        $focusedCase = $null
        if ($focusedCases.Count -ne 1) {
            $problems.Add("focused-force-authority-case")
        }
        else {
            $focusedCase = $focusedCases[0]
            if ([string]$focusedCase.m_sStatus -ne "PASS") {
                $problems.Add("focused-force-authority-case-status")
            }
        }

        $focusedExpectedAssertionIds = @(Get-FocusedForceAuthorityAssertionIds)
        $focusedAssertionStatus = New-Object Collections.Generic.List[object]
        $focusedActualAssertionIds = @()
        if ($focusedCase) {
            $focusedActualAssertionIds = @($focusedCase.m_aAssertions | Where-Object {
                [string]$_.m_sAssertionId -match '^(combat_presence|ownership_transition|town_influence)\.'
            } | ForEach-Object { [string]$_.m_sAssertionId })
            foreach ($assertionId in $focusedExpectedAssertionIds) {
                $status = Get-ExactAssertionStatus -Case $focusedCase -AssertionId $assertionId
                $passed = $status -eq "PASS"
                $focusedAssertionStatus.Add([pscustomobject]@{
                    Id = $assertionId
                    Pass = $passed
                    Status = $status
                    Actual = Get-ExactAssertionActual `
                        -Case $focusedCase `
                        -AssertionId $assertionId
                })
                if (-not $passed) {
                    $problems.Add("focused-$assertionId")
                }
            }
            if ($focusedActualAssertionIds.Count -ne $focusedExpectedAssertionIds.Count) {
                $problems.Add("focused-assertion-count")
            }
            foreach ($assertionId in $focusedActualAssertionIds) {
                if ($focusedExpectedAssertionIds -notcontains $assertionId) {
                    $problems.Add("focused-unexpected-$assertionId")
                }
            }
        }

        $cleanupCases = @(Find-Case -Run $run -CaseId "cleanup.run_leak_snapshot")
        $cleanupOrphans = [int]-1
        $cleanupPass = $false
        if ($cleanupCases.Count -eq 1) {
            $cleanupMetricMatches = @($cleanupCases[0].m_aMetrics | Where-Object {
                [string]$_.m_sMetricId -ceq "cleanup.orphan_active_groups"
            })
            if ($cleanupMetricMatches.Count -eq 1) {
                $cleanupOrphans = [string]$cleanupMetricMatches[0].m_sValue
            }
            $cleanupPass = (Test-ExactPassingAssertion -Case $cleanupCases[0] -AssertionId "cleanup.orphan_active_groups") -and
                $cleanupOrphans -eq "0"
        }
        if (-not $cleanupPass) {
            $problems.Add("final-orphan-cleanup")
        }

        if ($RequireCorrectedCanaryContract) {
            if ($ExpectedProfile -cne "force_authority" -or
                [string]$run.m_sProfile -cne "force_authority") {
                $problems.Add("corrected-canary-profile-contract")
            }

            $expectedCaseManifest = @(
                Get-CorrectedCanaryCaseManifest -RunId ([string]$run.m_sRunId))
            $caseSetExact = $expectedCaseManifest.Count -eq $cases.Count
            if ($caseSetExact) {
                for ($caseIndex = 0; $caseIndex -lt $expectedCaseManifest.Count; $caseIndex++) {
                    $expectedCase = $expectedCaseManifest[$caseIndex]
                    $actualCase = $cases[$caseIndex]
                    if ([string]$actualCase.m_sCaseId -cne [string]$expectedCase.Id -or
                        [string]$actualCase.m_sStatus -cne [string]$expectedCase.Status -or
                        [string]$actualCase.m_sCategory -cne [string]$expectedCase.Category -or
                        [string]$actualCase.m_sFeature -cne [string]$expectedCase.Feature -or
                        [string]$actualCase.m_sStage -cne [string]$expectedCase.Stage) {
                        $caseSetExact = $false
                        break
                    }
                }
            }
            if (-not $caseSetExact) {
                $problems.Add("corrected-canary-case-set")
            }
            if ($cases.Count -ne 11 -or
                $statusCounts.PASS -ne 9 -or
                $statusCounts.WARN -ne 1 -or
                $statusCounts.FAIL -ne 0 -or
                $statusCounts.BLOCKED -ne 1 -or
                $statusCounts.SKIPPED -ne 0) {
                $problems.Add("corrected-canary-case-census")
            }
            if ([int]$run.m_iCertificationRequiredCount -ne 87 -or
                [int]$run.m_iCertificationProvenCount -ne 87 -or
                [int]$run.m_iCertificationFailCount -ne 0 -or
                [int]$run.m_iCertificationBlockedCount -ne 0 -or
                [int]$run.m_iCertificationWarnCount -ne 0 -or
                [bool]$run.m_bCertificationPassed) {
                $problems.Add("corrected-canary-certification-census")
            }

            $canonicalStatuses = @("PASS", "WARN", "FAIL", "BLOCKED", "SKIPPED")
            $strictAssertionRows = New-Object Collections.Generic.List[object]
            $strictCertificationCounts = [ordered]@{
                PASS = 0
                WARN = 0
                FAIL = 0
                BLOCKED = 0
                SKIPPED = 0
            }
            $strictFailedAssertionCount = 0
            $strictSkippedAssertionCount = 0
            foreach ($strictCase in $cases) {
                $strictCaseId = [string]$strictCase.m_sCaseId
                $strictCaseStatus = [string]$strictCase.m_sStatus
                $strictCaseStatusCanonical =
                    $canonicalStatuses -ccontains $strictCaseStatus
                if (-not $strictCaseStatusCanonical) {
                    $problems.Add("corrected-canary-case-status-contract")
                }

                $derivedCaseStatus = "PASS"
                $derivedCaseSeverity = 0
                $seenAssertionIds = New-Object `
                    'Collections.Generic.HashSet[string]' `
                    ([StringComparer]::Ordinal)
                foreach ($strictAssertion in @($strictCase.m_aAssertions)) {
                    $strictAssertionId = [string]$strictAssertion.m_sAssertionId
                    if ([string]::IsNullOrWhiteSpace($strictAssertionId) -or
                        -not $seenAssertionIds.Add($strictAssertionId)) {
                        $problems.Add("corrected-canary-assertion-id-contract")
                    }

                    $strictAssertionStatus = [string]$strictAssertion.m_sStatus
                    $strictAssertionStatusCanonical =
                        $canonicalStatuses -ccontains $strictAssertionStatus
                    if (-not $strictAssertionStatusCanonical) {
                        $problems.Add("corrected-canary-assertion-status-contract")
                    }

                    $certificationProperty = $strictAssertion.PSObject.Properties[
                        'm_bCountsTowardCertification']
                    $countsTowardCertification = $false
                    if (-not $certificationProperty -or
                        $certificationProperty.Value -isnot [bool]) {
                        $problems.Add("corrected-canary-certification-flag-contract")
                    }
                    else {
                        $countsTowardCertification =
                            [bool]$certificationProperty.Value
                    }

                    $strictAssertionRows.Add([pscustomobject]@{
                        CaseId = $strictCaseId
                        CaseCategory = [string]$strictCase.m_sCategory
                        CaseFeature = [string]$strictCase.m_sFeature
                        CaseStage = [string]$strictCase.m_sStage
                        Id = $strictAssertionId
                        Status = $strictAssertionStatus
                        CountsTowardCertification = $countsTowardCertification
                        Expected = [string]$strictAssertion.m_sExpected
                        Actual = [string]$strictAssertion.m_sActual
                        Reason = [string]$strictAssertion.m_sFailureReason
                        ProofLevel = [string]$strictAssertion.m_sProofLevel
                        ObservedPath = [string]$strictAssertion.m_sObservedPath
                        RequiredPath = [string]$strictAssertion.m_sRequiredPath
                    })

                    if ($strictAssertionStatusCanonical) {
                        $assertionSeverity = switch -CaseSensitive (
                            $strictAssertionStatus) {
                            "SKIPPED" { 1 }
                            "WARN" { 2 }
                            "BLOCKED" { 3 }
                            "FAIL" { 4 }
                            default { 0 }
                        }
                        if ($assertionSeverity -gt $derivedCaseSeverity) {
                            $derivedCaseSeverity = $assertionSeverity
                            $derivedCaseStatus = $strictAssertionStatus
                        }
                        if ($strictAssertionStatus -ceq "FAIL") {
                            $strictFailedAssertionCount++
                        }
                        elseif ($strictAssertionStatus -ceq "SKIPPED") {
                            $strictSkippedAssertionCount++
                        }

                        if ($countsTowardCertification) {
                            $strictCertificationCounts[$strictAssertionStatus]++
                        }
                    }
                }

                if ($strictCaseStatusCanonical -and
                    $strictCaseStatus -cne $derivedCaseStatus) {
                    $problems.Add("corrected-canary-case-disposition")
                }
            }

            if ($strictFailedAssertionCount -ne 0 -or
                $strictSkippedAssertionCount -ne 0) {
                $problems.Add("corrected-canary-hidden-fail-or-skip")
            }
            $strictCertificationRequired =
                $strictCertificationCounts.PASS +
                $strictCertificationCounts.WARN +
                $strictCertificationCounts.FAIL +
                $strictCertificationCounts.BLOCKED
            if ($strictCertificationRequired -ne 87 -or
                $strictCertificationCounts.PASS -ne 87 -or
                $strictCertificationCounts.WARN -ne 0 -or
                $strictCertificationCounts.FAIL -ne 0 -or
                $strictCertificationCounts.BLOCKED -ne 0 -or
                $strictCertificationCounts.SKIPPED -ne 0 -or
                $strictCertificationRequired -ne
                    [int]$run.m_iCertificationRequiredCount -or
                $strictCertificationCounts.PASS -ne
                    [int]$run.m_iCertificationProvenCount -or
                $strictCertificationCounts.WARN -ne
                    [int]$run.m_iCertificationWarnCount -or
                $strictCertificationCounts.FAIL -ne
                    [int]$run.m_iCertificationFailCount -or
                $strictCertificationCounts.BLOCKED -ne
                    [int]$run.m_iCertificationBlockedCount) {
                $problems.Add("corrected-canary-certification-assertion-census")
            }

            $expectedAssertionManifest = @(
                Get-CorrectedCanaryAssertionManifest -RunId ([string]$run.m_sRunId))
            $assertionManifestExact =
                $expectedAssertionManifest.Count -eq $strictAssertionRows.Count -and
                @($expectedAssertionManifest | Where-Object {
                    $_.CountsTowardCertification
                }).Count -eq 87
            if ($assertionManifestExact) {
                for ($manifestIndex = 0;
                    $manifestIndex -lt $expectedAssertionManifest.Count;
                    $manifestIndex++) {
                    $expectedAssertion = $expectedAssertionManifest[$manifestIndex]
                    $actualAssertion = $strictAssertionRows[$manifestIndex]
                    if ([string]$actualAssertion.CaseId -cne
                            [string]$expectedAssertion.CaseId -or
                        [string]$actualAssertion.Id -cne [string]$expectedAssertion.Id -or
                        [string]$actualAssertion.Status -cne
                            [string]$expectedAssertion.Status -or
                        [bool]$actualAssertion.CountsTowardCertification -ne
                            [bool]$expectedAssertion.CountsTowardCertification) {
                        $assertionManifestExact = $false
                        break
                    }
                }
            }
            if (-not $assertionManifestExact) {
                $problems.Add("corrected-canary-assertion-manifest")
            }

            $strictFocusedCases = @($cases | Where-Object {
                [string]$_.m_sCaseId -ceq "early_mechanics.force_authority"
            })
            $strictFocusedContractValid = $strictFocusedCases.Count -eq 1
            if ($strictFocusedContractValid) {
                $strictFocusedAssertions = @($strictFocusedCases[0].m_aAssertions)
                $strictExpectedFocusedIds = @(
                    Get-FocusedForceAuthorityAssertionIds | Sort-Object -CaseSensitive)
                $strictActualFocusedIds = @($strictFocusedAssertions |
                    ForEach-Object { [string]$_.m_sAssertionId } |
                    Sort-Object -CaseSensitive)
                $strictFocusedContractValid =
                    $strictFocusedAssertions.Count -eq 35 -and
                    $strictExpectedFocusedIds.Count -eq
                        $strictActualFocusedIds.Count
                if ($strictFocusedContractValid) {
                    for ($focusedIndex = 0;
                        $focusedIndex -lt $strictExpectedFocusedIds.Count;
                        $focusedIndex++) {
                        if ($strictExpectedFocusedIds[$focusedIndex] -cne
                            $strictActualFocusedIds[$focusedIndex]) {
                            $strictFocusedContractValid = $false
                            break
                        }
                    }
                }
                if ($strictFocusedContractValid) {
                    foreach ($strictFocusedAssertion in $strictFocusedAssertions) {
                        $focusedCertificationProperty =
                            $strictFocusedAssertion.PSObject.Properties[
                                'm_bCountsTowardCertification']
                        $expectedFocusedCertification =
                            [string]$strictFocusedAssertion.m_sAssertionId -cne
                                "town_influence.external_completion"
                        if ([string]$strictFocusedAssertion.m_sStatus -cne "PASS" -or
                            -not $focusedCertificationProperty -or
                            $focusedCertificationProperty.Value -isnot [bool] -or
                            [bool]$focusedCertificationProperty.Value -ne
                                $expectedFocusedCertification) {
                            $strictFocusedContractValid = $false
                            break
                        }
                    }
                }
            }
            if (-not $strictFocusedContractValid) {
                $problems.Add("corrected-canary-focused-assertion-contract")
            }

            $warningAssertions = @($strictAssertionRows | Where-Object {
                [string]$_.Status -ceq "WARN"
            })
            $blockedAssertions = @($strictAssertionRows | Where-Object {
                [string]$_.Status -ceq "BLOCKED"
            })
            $warningCases = @($cases | Where-Object {
                [string]$_.m_sStatus -ceq "WARN"
            })
            $blockedCases = @($cases | Where-Object {
                [string]$_.m_sStatus -ceq "BLOCKED"
            })
            $warningContractExact =
                $warningCases.Count -eq 1 -and
                $warningAssertions.Count -eq 1
            if ($warningContractExact) {
                $markerWarnings = @($warningAssertions | Where-Object {
                    [string]$_.Id -ceq "cleanup.player_marker.live"
                })
                $markerWarningCases = @($warningCases | Where-Object {
                    [string]$_.m_sCaseId -ceq
                        "cleanup.player_marker_completion"
                })
                $warningContractExact =
                    $markerWarnings.Count -eq 1 -and
                    $markerWarningCases.Count -eq 1
            }
            if ($warningContractExact) {
                $markerWarning = $markerWarnings[0]
                $warningContractExact =
                    [string]$markerWarning.CaseId -ceq
                        "cleanup.player_marker_completion" -and
                    [string]$markerWarning.CaseCategory -ceq "cleanup" -and
                    [string]$markerWarning.CaseFeature -ceq "player_markers" -and
                    [string]$markerWarning.CaseStage -ceq "final" -and
                    [string]$markerWarning.Id -ceq "cleanup.player_marker.live" -and
                    [string]$markerWarning.Expected -ceq
                        "enabled player marker service has desired/tracked/live marker after cleanup" -and
                    [string]$markerWarning.Actual -cmatch
                        '^enabled [01] \| desired \d+ \| tracked \d+ \| live \d+ \| entry [01]$' -and
                    [string]$markerWarning.Reason -ceq
                        "player marker did not reconcile after campaign debug completion" -and
                    [string]$markerWarning.ProofLevel -ceq "STATE_ONLY" -and
                    [string]$markerWarning.ObservedPath -ceq "diagnostic_only" -and
                    [string]$markerWarning.RequiredPath -ceq
                        "no debug-owned state or world leak" -and
                    -not [bool]$markerWarning.CountsTowardCertification
            }
            if (-not $warningContractExact) {
                $problems.Add("corrected-canary-warning-contract")
            }

            $blockedContractExact =
                $blockedCases.Count -eq 1 -and
                $blockedAssertions.Count -eq 1
            if ($blockedContractExact) {
                $worldBlocks = @($blockedAssertions | Where-Object {
                    [string]$_.Id -ceq "isolation.world_scope"
                })
                $worldBlockedCases = @($blockedCases | Where-Object {
                    [string]$_.m_sCaseId -ceq
                        "cleanup.state_isolation_restore"
                })
                $blockedContractExact =
                    $worldBlocks.Count -eq 1 -and
                    $worldBlockedCases.Count -eq 1
            }
            if ($blockedContractExact) {
                $worldBlock = $worldBlocks[0]
                $blockedContractExact =
                    [string]$worldBlock.CaseId -ceq
                        "cleanup.state_isolation_restore" -and
                    [string]$worldBlock.CaseCategory -ceq "cleanup" -and
                    [string]$worldBlock.CaseFeature -ceq "campaign_debug" -and
                    [string]$worldBlock.CaseStage -ceq "state_restore" -and
                    [string]$worldBlock.Id -ceq "isolation.world_scope" -and
                    [string]$worldBlock.Expected -ceq
                        "runtime certification remains scoped to the disposable development session" -and
                    [string]$worldBlock.Actual -ceq
                        "world runtime, player inventory, health, and service caches require session restart before another certifying run" -and
                    [string]$worldBlock.Reason -ceq
                        "restart the disposable development session before another certification run" -and
                    [string]$worldBlock.ProofLevel -ceq "EXTERNAL_PROCESS" -and
                    [string]$worldBlock.ObservedPath -ceq
                        "manual_external_gap" -and
                    [string]$worldBlock.RequiredPath -ceq
                        "external process restart, reconnect, or long-soak harness" -and
                    -not [bool]$worldBlock.CountsTowardCertification
            }
            if (-not $blockedContractExact) {
                $problems.Add("corrected-canary-blocked-contract")
            }

            $orphanCases = @($cases | Where-Object {
                [string]$_.m_sCaseId -ceq "cleanup.run_leak_snapshot"
            })
            $orphanContractExact = $orphanCases.Count -eq 1
            if ($orphanContractExact) {
                $orphanAssertions = @($orphanCases[0].m_aAssertions | Where-Object {
                    [string]$_.m_sAssertionId -ceq "cleanup.orphan_active_groups"
                })
                $orphanMetrics = @($orphanCases[0].m_aMetrics | Where-Object {
                    [string]$_.m_sMetricId -ceq "cleanup.orphan_active_groups"
                })
                $orphanContractExact =
                    $orphanAssertions.Count -eq 1 -and
                    $orphanMetrics.Count -eq 1 -and
                    [string]$orphanAssertions[0].m_sStatus -ceq "PASS" -and
                    [bool]$orphanAssertions[0].m_bCountsTowardCertification -and
                    [string]$orphanAssertions[0].m_sExpected -ceq
                        "no active groups without zone/mission/support/order/QRF backing" -and
                    [string]$orphanAssertions[0].m_sActual -ceq
                        "0 | total 0 | debug 0 | smoke 0 | other 0" -and
                    [string]$orphanAssertions[0].m_sFailureReason -ceq
                        "orphan active groups remain after debug run" -and
                    [string]$orphanAssertions[0].m_sProofLevel -ceq
                        "STATE_ONLY" -and
                    [string]$orphanAssertions[0].m_sObservedPath -ceq
                        "cleanup_probe" -and
                    [string]$orphanAssertions[0].m_sRequiredPath -ceq
                        "no debug-owned state or world leak" -and
                    [string]$orphanMetrics[0].m_sName -ceq
                        "cleanup.orphan_active_groups" -and
                    [string]$orphanMetrics[0].m_sValue -ceq "0" -and
                    [string]$orphanMetrics[0].m_sUnit -ceq "count" -and
                    [string]$orphanMetrics[0].m_sFeature -ceq "campaign_debug" -and
                    [string]$orphanMetrics[0].m_sStage -ceq "final"
            }
            if (-not $orphanContractExact) {
                $problems.Add("corrected-canary-orphan-contract")
            }
        }

        $restoreCases = @(Find-Case -Run $run -CaseId "cleanup.state_isolation_restore")
        if ($restoreCases.Count -ne 1 -or
            -not (Test-ExactPassingAssertion -Case $restoreCases[0] -AssertionId "isolation.snapshot") -or
            -not (Test-ExactPassingAssertion -Case $restoreCases[0] -AssertionId "isolation.state_restore") -or
            -not (Test-ExactPassingAssertion -Case $restoreCases[0] -AssertionId "isolation.persistence_restore")) {
            $problems.Add("state-isolation-restore")
        }

        $artifactNames = @($run.m_aArtifacts | ForEach-Object {
            ([string]$_).Replace('\', '/') | Split-Path -Leaf
        })
        $jsonName = Split-Path -Leaf $JsonPath
        $summaryName = Split-Path -Leaf $SummaryPath
        $stateDiffName = Split-Path -Leaf $StateDiffPath
        if ($artifactNames.Count -ne 3 -or
            $artifactNames -notcontains $jsonName -or
            $artifactNames -notcontains $summaryName -or
            $artifactNames -notcontains $stateDiffName) {
            $problems.Add("artifact-identities")
        }

        $escapedRunId = [Regex]::Escape([string]$run.m_sRunId)
        if ($summaryText -notmatch "(?m)^Partisan campaign debug complete\s*$" -or
            $summaryText -notmatch "(?m)^run $escapedRunId\s*$" -or
            $summaryText -notmatch "(?m)^profile force_authority\s*$" -or
            $summaryText.IndexOf($ExpectedSha, [StringComparison]::Ordinal) -lt 0 -or
            $summaryText.IndexOf($ExpectedLabel, [StringComparison]::Ordinal) -lt 0) {
            $problems.Add("summary-identity")
        }

        $stateDiffValidation = Get-CampaignDebugStateDiffValidation `
            -Text $stateDiffText `
            -RunId ([string]$run.m_sRunId)
        if (-not $stateDiffValidation.ContractExact -or
            $stateDiffValidation.RowCount -ne 18 -or
            $stateDiffValidation.NonzeroRowCount -ne 0 -or
            $stateDiffText -match "campaign state missing at artifact write") {
            $problems.Add("state-diff")
        }

        return [pscustomobject]@{
            Valid = $problems.Count -eq 0
            Problems = $problems.ToArray()
            RunId = [string]$run.m_sRunId
            Profile = [string]$run.m_sProfile
            ProofScope = "focused_force_authority"
            FullCertification = $false
            BuildSha = [string]$run.m_sBuildSha
            BuildUtc = [string]$run.m_sBuildUtc
            BuildLabel = [string]$run.m_sBuildLabel
            StartedAtSecond = [int]$run.m_iStartedAtSecond
            EndedAtSecond = [int]$run.m_iEndedAtSecond
            CaseCount = $cases.Count
            Pass = [int]$run.m_iPassCount
            Warn = [int]$run.m_iWarnCount
            Fail = [int]$run.m_iFailCount
            Blocked = [int]$run.m_iBlockedCount
            Skipped = [int]$run.m_iSkippedCount
            CertificationRequired = [int]$run.m_iCertificationRequiredCount
            CertificationProven = [int]$run.m_iCertificationProvenCount
            CertificationFail = [int]$run.m_iCertificationFailCount
            CertificationBlocked = [int]$run.m_iCertificationBlockedCount
            CertificationWarn = [int]$run.m_iCertificationWarnCount
            CertificationPassed = [bool]$run.m_bCertificationPassed
            CorrectedCanaryContract = [bool]$RequireCorrectedCanaryContract
            Trigger = $trigger
            ArtifactCount = $artifactNames.Count
            StateDiffRows = $stateDiffValidation.RowCount
            NonzeroStateDiffRows = $stateDiffValidation.NonzeroRowCount
            StateDiffManifestExact = $stateDiffValidation.ContractExact
            CorrectedCanaryAssertionManifestExact = $assertionManifestExact
            CorrectedCanaryCaseSetExact = $caseSetExact
            CorrectedCanaryWarningContractExact = $warningContractExact
            CorrectedCanaryBlockedContractExact = $blockedContractExact
            CorrectedCanaryOrphanContractExact = $orphanContractExact
            Phase17 = @()
            Phase17Metrics = [pscustomobject][ordered]@{}
            Phase24 = @()
            Phase24Metrics = [pscustomobject][ordered]@{}
            StagedCleanup = @()
            FocusedCaseId = if ($focusedCase) { [string]$focusedCase.m_sCaseId } else { $null }
            FocusedCaseStatus = if ($focusedCase) { [string]$focusedCase.m_sStatus } else { $null }
            FocusedAssertions = $focusedAssertionStatus.ToArray()
            IntentionalMissionConvoyAdmissionDiagnosticsProven = $false
            IntentionalMissionConvoySettlementDiagnosticProven = $false
            IntentionalMissionConvoyCorruptionDiagnosticsProven = $false
            IntentionalMissionConvoyWatchdogDiagnosticProven = $false
            FinalOrphanCleanupPass = $cleanupPass
            FinalOrphanActiveGroups = $cleanupOrphans
        }
    }

    $requiredFinalCases = @(
        "phase25.manual_external_gaps",
        "observation.final_report",
        "phase24.phase24_final_report",
        "cleanup.enemy_orders.run_completion",
        "cleanup.player_marker_completion",
        "cleanup.run_leak_snapshot",
        "cleanup.state_isolation_restore"
    )
    foreach ($caseId in $requiredFinalCases) {
        if (@(Find-Case -Run $run -CaseId $caseId).Count -ne 1) {
            $problems.Add("missing-$caseId")
        }
    }

    $phase17Cases = @(Find-Case -Run $run -CaseId "phase17.phase17_report")
    $phase17Ids = @(
        "phase17.counterattack.native_projection.baseline",
        "phase17.counterattack.native_projection.materializing",
        "phase17.counterattack.native_projection.physical",
        "phase17.counterattack.native_projection.fold",
        "phase17.counterattack.native_projection.continuity",
        "phase17.counterattack.native_projection.clock_isolation",
        "phase17.counterattack.native_projection.native_casualty",
        "phase17.counterattack.native_projection.casualty_fold",
        "phase17.counterattack.native_projection.casualty_reentry",
        "phase17.counterattack.native_projection.casualty_replay",
        "phase17.counterattack.native_projection.casualty_continuity"
    )
    $phase17Status = New-Object Collections.Generic.List[object]
    if ($phase17Cases.Count -ne 1) {
        $problems.Add("phase17-case")
    }
    else {
        foreach ($assertionId in $phase17Ids) {
            $passed = Test-ExactPassingAssertion -Case $phase17Cases[0] -AssertionId $assertionId
            $phase17Status.Add([pscustomobject]@{ Id = $assertionId; Pass = $passed })
            if (-not $passed) {
                $problems.Add("phase17-$assertionId")
            }
        }
    }

    $phase24Cases = @(Find-Case -Run $run -CaseId "phase24.phase24_escalation_pressure")
    $phase24Ids = @(
        "phase24.escalation.runtime_owner_classification",
        "phase24.escalation.exact_counterattack_authority"
    )
    $phase24Status = New-Object Collections.Generic.List[object]
    if ($phase24Cases.Count -ne 1) {
        $problems.Add("phase24-case")
    }
    else {
        foreach ($assertionId in $phase24Ids) {
            $status = Get-ExactAssertionStatus -Case $phase24Cases[0] -AssertionId $assertionId
            $actual = Get-ExactAssertionActual -Case $phase24Cases[0] -AssertionId $assertionId
            $passed = $status -eq "PASS"
            $accepted = $passed -or
                ($assertionId -eq "phase24.escalation.exact_counterattack_authority" -and
                    $status -eq "SKIPPED")
            $phase24Status.Add([pscustomobject]@{
                Id = $assertionId
                Pass = $passed
                Accepted = $accepted
                Status = $status
                Actual = $actual
            })
            if (-not $accepted) {
                $problems.Add("phase24-$assertionId")
            }
        }
    }

    $stagedSuffixes = @(
        "partial-cancel_start",
        "partial-cancel_transition",
        "success_member_transition",
        "success_and_failure_fixture_transition",
        "failure_member_transition",
        "same-wave_failure_capture"
    )
    $stagedStatus = New-Object Collections.Generic.List[object]
    $expectedAppliedGrace = @{
        "partial-cancel_start" = 0
        "partial-cancel_transition" = 1
        "success_member_transition" = 1
        "success_and_failure_fixture_transition" = 0
        "failure_member_transition" = 1
        "same-wave_failure_capture" = 1
    }
    foreach ($suffix in $stagedSuffixes) {
        $caseId = "post_case_cleanup.action_mechanic_exact_spawn_adapter_$suffix"
        $matches = @(Find-Case -Run $run -CaseId $caseId)
        $passed = $false
        $caseStatus = $null
        $activeGroupsStatus = $null
        $runtimeFactionsStatus = $null
        $runtimeFactionsActual = $null
        $populationSettledStatus = $null
        $orphanMetric = $null
        $runtimeFactionMismatchMetric = $null
        $graceCandidateMetric = $null
        $pendingPopulationMetric = $null
        if ($matches.Count -eq 1) {
            $caseStatus = [string]$matches[0].m_sStatus
            $activeGroupsStatus = Get-ExactAssertionStatus -Case $matches[0] -AssertionId "post_cleanup.active_groups"
            $runtimeFactionsStatus = Get-ExactAssertionStatus -Case $matches[0] -AssertionId "post_cleanup.runtime_factions"
            $runtimeFactionsActual = Get-ExactAssertionActual -Case $matches[0] -AssertionId "post_cleanup.runtime_factions"
            $populationSettledStatus = Get-ExactAssertionStatus -Case $matches[0] -AssertionId "post_cleanup.runtime_group_population_settled"
            $orphanMetric = Get-MetricValue -Metrics $matches[0].m_aMetrics -MetricId "post_cleanup.orphan_active_groups"
            $runtimeFactionMismatchMetric = Get-MetricValue -Metrics $matches[0].m_aMetrics -MetricId "post_cleanup.runtime_faction_mismatches"
            $graceCandidateMetric = Get-MetricValue -Metrics $matches[0].m_aMetrics -MetricId "post_cleanup.runtime_faction_zero_member_grace_candidates"
            $pendingPopulationMetric = Get-MetricValue -Metrics $matches[0].m_aMetrics -MetricId "post_cleanup.runtime_pending_population_groups"
            $graceCandidateCount = 0
            $graceCandidateValid = [int]::TryParse(
                [string]$graceCandidateMetric,
                [ref]$graceCandidateCount) -and $graceCandidateCount -gt 0
            $appliedGraceValid = $runtimeFactionsActual -match (
                "(?:^|\|\s*)exact staged zero-member grace\s+" +
                [regex]::Escape([string]$expectedAppliedGrace[$suffix]) +
                "(?:\s*\||$)")
            $passed = $caseStatus -eq "PASS" -and
                $activeGroupsStatus -eq "PASS" -and
                $runtimeFactionsStatus -eq "PASS" -and
                $populationSettledStatus -eq "PASS" -and
                $orphanMetric -eq "0" -and
                $runtimeFactionMismatchMetric -eq "0" -and
                $graceCandidateValid -and
                $appliedGraceValid -and
                $pendingPopulationMetric -eq "0"
        }
        $stagedStatus.Add([pscustomobject]@{
            Id = $caseId
            Pass = $passed
            CaseStatus = $caseStatus
            ActiveGroupsStatus = $activeGroupsStatus
            RuntimeFactionsStatus = $runtimeFactionsStatus
            RuntimeFactionsActual = $runtimeFactionsActual
            ExpectedZeroMemberGraceApplied = $expectedAppliedGrace[$suffix]
            RuntimeGroupPopulationSettledStatus = $populationSettledStatus
            OrphanActiveGroups = $orphanMetric
            RuntimeFactionMismatches = $runtimeFactionMismatchMetric
            ZeroMemberGraceCandidates = $graceCandidateMetric
            PendingPopulationGroups = $pendingPopulationMetric
        })
        if (-not $passed) {
            $problems.Add("staged-$suffix")
        }
    }

    $cleanupCases = @(Find-Case -Run $run -CaseId "cleanup.run_leak_snapshot")
    $cleanupOrphans = [int]-1
    $cleanupPass = $false
    if ($cleanupCases.Count -eq 1) {
        $cleanupMetricMatches = @($cleanupCases[0].m_aMetrics | Where-Object {
            [string]$_.m_sMetricId -ceq "cleanup.orphan_active_groups"
        })
        if ($cleanupMetricMatches.Count -eq 1) {
            $cleanupOrphans = [string]$cleanupMetricMatches[0].m_sValue
        }
        $cleanupPass = (Test-ExactPassingAssertion -Case $cleanupCases[0] -AssertionId "cleanup.orphan_active_groups") -and
            $cleanupOrphans -eq "0"
    }
    if (-not $cleanupPass) {
        $problems.Add("final-orphan-cleanup")
    }

    $restoreCases = @(Find-Case -Run $run -CaseId "cleanup.state_isolation_restore")
    if ($restoreCases.Count -ne 1 -or
        -not (Test-ExactPassingAssertion -Case $restoreCases[0] -AssertionId "isolation.snapshot") -or
        -not (Test-ExactPassingAssertion -Case $restoreCases[0] -AssertionId "isolation.state_restore") -or
        -not (Test-ExactPassingAssertion -Case $restoreCases[0] -AssertionId "isolation.persistence_restore")) {
        $problems.Add("state-isolation-restore")
    }

    $artifactNames = @($run.m_aArtifacts | ForEach-Object {
        ([string]$_).Replace('\', '/') | Split-Path -Leaf
    })
    $jsonName = Split-Path -Leaf $JsonPath
    $summaryName = Split-Path -Leaf $SummaryPath
    $stateDiffName = Split-Path -Leaf $StateDiffPath
    if ($artifactNames.Count -ne 3 -or
        $artifactNames -notcontains $jsonName -or
        $artifactNames -notcontains $summaryName -or
        $artifactNames -notcontains $stateDiffName) {
        $problems.Add("artifact-identities")
    }

    $escapedRunId = [Regex]::Escape([string]$run.m_sRunId)
    if ($summaryText -notmatch "(?m)^Partisan campaign debug complete\s*$" -or
        $summaryText -notmatch "(?m)^run $escapedRunId\s*$" -or
        $summaryText -notmatch "(?m)^profile full_certification\s*$" -or
        $summaryText.IndexOf($ExpectedSha, [StringComparison]::Ordinal) -lt 0 -or
        $summaryText.IndexOf($ExpectedLabel, [StringComparison]::Ordinal) -lt 0) {
        $problems.Add("summary-identity")
    }

    $stateDiffValidation = Get-CampaignDebugStateDiffValidation `
        -Text $stateDiffText `
        -RunId ([string]$run.m_sRunId)
    if (-not $stateDiffValidation.ContractExact -or
        $stateDiffValidation.RowCount -ne 18 -or
        $stateDiffValidation.NonzeroRowCount -ne 0 -or
        $stateDiffText -match "campaign state missing at artifact write") {
        $problems.Add("state-diff")
    }

    $phase17Metrics = [ordered]@{}
    if ($phase17Cases.Count -eq 1) {
        foreach ($suffix in @(
            "spawn_ticks",
            "spawn_tick_limit",
            "spawn_deferred_ticks",
            "physical_settle_ticks",
            "casualty_reentry_physical_settle_ticks",
            "survivor_reentry_physical_settle_ticks",
            "physical_settle_limit",
            "casualty_settle_ticks",
            "casualty_settle_limit",
            "elapsed_peak",
            "expected_living"
        )) {
            $id = "phase17.counterattack.native_projection.$suffix"
            $phase17Metrics[$suffix] = Get-MetricValue -Metrics $phase17Cases[0].m_aMetrics -MetricId $id
        }
    }

    $phase24Metrics = [ordered]@{}
    if ($phase24Cases.Count -eq 1) {
        foreach ($suffix in @(
            "runtime_owner_expected",
            "runtime_owner_classified",
            "runtime_owner_snapshot_invariant_failures",
            "exact_counterattack_orders",
            "exact_counterattack_open_orders",
            "exact_counterattack_terminal_ledgers",
            "exact_counterattack_invalid_authority",
            "exact_counterattack_projection_groups",
            "exact_counterattack_virtual_groups",
            "exact_counterattack_materializing_groups",
            "exact_counterattack_physical_groups",
            "exact_counterattack_dematerializing_groups",
            "exact_counterattack_support_leaks"
        )) {
            $id = "phase24.escalation.$suffix"
            $phase24Metrics[$suffix] = Get-MetricValue -Metrics $phase24Cases[0].m_aMetrics -MetricId $id
        }

        $exactAuthorityActual = Get-ExactAssertionActual `
            -Case $phase24Cases[0] `
            -AssertionId "phase24.escalation.exact_counterattack_authority"
        $exactAuthorityMatch = [Regex]::Match(
            [string]$exactAuthorityActual,
            '^\s*orders/open/terminal/invalid\s+(\d+)/(\d+)/(\d+)/(\d+)\s*\|\s*projections\s+(\d+)\s*\|\s*V/M/P/D\s+(\d+)/(\d+)/(\d+)/(\d+)\s*\|\s*support leaks\s+(\d+)\s*$')
        if ($exactAuthorityMatch.Success) {
            $phase24Metrics["exact_counterattack_invalid_authority_rows"] = $exactAuthorityMatch.Groups[4].Value
            $metricContract = @(
                @("exact_counterattack_orders", 1),
                @("exact_counterattack_open_orders", 2),
                @("exact_counterattack_terminal_ledgers", 3),
                @("exact_counterattack_invalid_authority", 4),
                @("exact_counterattack_projection_groups", 5),
                @("exact_counterattack_virtual_groups", 6),
                @("exact_counterattack_materializing_groups", 7),
                @("exact_counterattack_physical_groups", 8),
                @("exact_counterattack_dematerializing_groups", 9),
                @("exact_counterattack_support_leaks", 10)
            )
            foreach ($metricEntry in $metricContract) {
                $metricName = [string]$metricEntry[0]
                $groupIndex = [int]$metricEntry[1]
                if ($phase24Metrics[$metricName] -ne $exactAuthorityMatch.Groups[$groupIndex].Value) {
                    $problems.Add("phase24-exact-counterattack-metrics")
                    break
                }
            }
        }
        else {
            $problems.Add("phase24-exact-counterattack-actual")
            $phase24Metrics["exact_counterattack_invalid_authority_rows"] = $null
        }

        $exactAuthorityDiagnostic = @($phase24Status | Where-Object {
            $_.Id -eq "phase24.escalation.exact_counterattack_authority"
        })
        if ($exactAuthorityDiagnostic.Count -eq 1 -and
            $exactAuthorityDiagnostic[0].Status -eq "SKIPPED") {
            foreach ($zeroMetric in @(
                "exact_counterattack_orders",
                "exact_counterattack_open_orders",
                "exact_counterattack_terminal_ledgers",
                "exact_counterattack_invalid_authority",
                "exact_counterattack_projection_groups",
                "exact_counterattack_virtual_groups",
                "exact_counterattack_materializing_groups",
                "exact_counterattack_physical_groups",
                "exact_counterattack_dematerializing_groups",
                "exact_counterattack_support_leaks"
            )) {
                if ($phase24Metrics[$zeroMetric] -ne "0") {
                    $problems.Add("phase24-exact-counterattack-skipped-nonzero")
                    break
                }
            }
        }
    }

    $intentionalMissionConvoyAdmissionDiagnosticsProven = $false
    $intentionalMissionConvoySettlementDiagnosticProven = $false
    $intentionalMissionConvoyCorruptionDiagnosticsProven = $false
    $intentionalMissionConvoyWatchdogDiagnosticProven = $false
    $forceAuthorityCases = @(Find-Case -Run $run -CaseId "early_mechanics.force_authority")
    if ($forceAuthorityCases.Count -eq 1) {
        $intentionalMissionConvoyAdmissionDiagnosticsProven =
            (Test-ExactPassingAssertion `
                -Case $forceAuthorityCases[0] `
                -AssertionId "mission_convoy.admission") -and
            (Test-ExactPassingAssertion `
                -Case $forceAuthorityCases[0] `
                -AssertionId "mission_convoy.admission_rollback")
        $intentionalMissionConvoySettlementDiagnosticProven =
            Test-ExactPassingAssertion `
                -Case $forceAuthorityCases[0] `
                -AssertionId "mission_convoy.settlement"
        $intentionalMissionConvoyCorruptionDiagnosticsProven =
            Test-ExactPassingAssertion `
                -Case $forceAuthorityCases[0] `
                -AssertionId "mission_convoy.corruption_rejection"
        $intentionalMissionConvoyWatchdogDiagnosticProven =
            Test-ExactPassingAssertion `
                -Case $forceAuthorityCases[0] `
                -AssertionId "mission_convoy.watchdog"
    }

    return [pscustomobject]@{
        Valid = $problems.Count -eq 0
        Problems = $problems.ToArray()
        RunId = [string]$run.m_sRunId
        Profile = [string]$run.m_sProfile
        ProofScope = "full_certification"
        FullCertification = $true
        BuildSha = [string]$run.m_sBuildSha
        BuildUtc = [string]$run.m_sBuildUtc
        BuildLabel = [string]$run.m_sBuildLabel
        StartedAtSecond = [int]$run.m_iStartedAtSecond
        EndedAtSecond = [int]$run.m_iEndedAtSecond
        CaseCount = $cases.Count
        Pass = [int]$run.m_iPassCount
        Warn = [int]$run.m_iWarnCount
        Fail = [int]$run.m_iFailCount
        Blocked = [int]$run.m_iBlockedCount
        Skipped = [int]$run.m_iSkippedCount
        CertificationRequired = [int]$run.m_iCertificationRequiredCount
        CertificationProven = [int]$run.m_iCertificationProvenCount
        CertificationFail = [int]$run.m_iCertificationFailCount
        CertificationBlocked = [int]$run.m_iCertificationBlockedCount
        CertificationWarn = [int]$run.m_iCertificationWarnCount
        CertificationPassed = [bool]$run.m_bCertificationPassed
        CorrectedCanaryContract = $false
        Trigger = $trigger
        ArtifactCount = $artifactNames.Count
        StateDiffRows = $stateDiffValidation.RowCount
        NonzeroStateDiffRows = $stateDiffValidation.NonzeroRowCount
        StateDiffManifestExact = $stateDiffValidation.ContractExact
        Phase17 = $phase17Status.ToArray()
        Phase17Metrics = [pscustomobject]$phase17Metrics
        Phase24 = $phase24Status.ToArray()
        Phase24Metrics = [pscustomobject]$phase24Metrics
        StagedCleanup = $stagedStatus.ToArray()
        FocusedCaseId = $null
        FocusedCaseStatus = $null
        FocusedAssertions = @()
        IntentionalMissionConvoyAdmissionDiagnosticsProven =
            $intentionalMissionConvoyAdmissionDiagnosticsProven
        IntentionalMissionConvoySettlementDiagnosticProven =
            $intentionalMissionConvoySettlementDiagnosticProven
        IntentionalMissionConvoyCorruptionDiagnosticsProven =
            $intentionalMissionConvoyCorruptionDiagnosticsProven
        IntentionalMissionConvoyWatchdogDiagnosticProven =
            $intentionalMissionConvoyWatchdogDiagnosticProven
        FinalOrphanCleanupPass = $cleanupPass
        FinalOrphanActiveGroups = $cleanupOrphans
    }
}

function Invoke-ArtifactValidatorSelfTest {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$ExpectedSha,
        [Parameter(Mandatory = $true)][string]$ExpectedUtc,
        [Parameter(Mandatory = $true)][string]$ExpectedLabel
    )

    New-Item -ItemType Directory -Path $Directory -Force | Out-Null
    $runId = "synthetic_guard_contract"
    $jsonName = "HST_CampaignDebug_" + $runId + ".json"
    $summaryName = "HST_CampaignDebug_" + $runId + "_summary.txt"
    $stateDiffName = "HST_CampaignDebug_" + $runId + "_state_diff.txt"
    $jsonPath = Join-Path $Directory $jsonName
    $summaryPath = Join-Path $Directory $summaryName
    $stateDiffPath = Join-Path $Directory $stateDiffName

    $newAssertion = {
        param(
            [string]$Id,
            [string]$Status = "PASS",
            [string]$Actual = "",
            [bool]$CountsTowardCertification = $false,
            [string]$Expected = "",
            [string]$FailureReason = "",
            [string]$ProofLevel = "STATE_ONLY",
            [string]$ObservedPath = "runtime_state",
            [string]$RequiredPath = "runtime_state"
        )
        [pscustomobject]@{
            m_sAssertionId = $Id
            m_sStatus = $Status
            m_sExpected = $Expected
            m_sActual = $Actual
            m_sFailureReason = $FailureReason
            m_sProofLevel = $ProofLevel
            m_sObservedPath = $ObservedPath
            m_sRequiredPath = $RequiredPath
            m_bCountsTowardCertification = $CountsTowardCertification
        }
    }
    $newMetric = {
        param(
            [string]$Id,
            [string]$Value,
            [string]$Unit = "",
            [string]$Feature = "",
            [string]$Stage = ""
        )
        [pscustomobject]@{
            m_sMetricId = $Id
            m_sName = $Id
            m_sValue = $Value
            m_sUnit = $Unit
            m_sFeature = $Feature
            m_sStage = $Stage
        }
    }
    $newCase = {
        param(
            [string]$Id,
            [string]$Status = "PASS",
            [object[]]$Assertions = @(),
            [object[]]$Metrics = @(),
            [string]$Category = "",
            [string]$Feature = "",
            [string]$Stage = ""
        )
        [pscustomobject]@{
            m_sCaseId = $Id
            m_sStatus = $Status
            m_sCategory = $Category
            m_sFeature = $Feature
            m_sStage = $Stage
            m_aAssertions = $Assertions
            m_aMetrics = $Metrics
        }
    }

    $phase17Ids = @(
        "phase17.counterattack.native_projection.baseline",
        "phase17.counterattack.native_projection.materializing",
        "phase17.counterattack.native_projection.physical",
        "phase17.counterattack.native_projection.fold",
        "phase17.counterattack.native_projection.continuity",
        "phase17.counterattack.native_projection.clock_isolation",
        "phase17.counterattack.native_projection.native_casualty",
        "phase17.counterattack.native_projection.casualty_fold",
        "phase17.counterattack.native_projection.casualty_reentry",
        "phase17.counterattack.native_projection.casualty_replay",
        "phase17.counterattack.native_projection.casualty_continuity"
    )
    $phase17Assertions = @($phase17Ids | ForEach-Object { & $newAssertion $_ })
    $phase17Metrics = @()
    foreach ($suffix in @(
        "spawn_ticks",
        "spawn_tick_limit",
        "spawn_deferred_ticks",
        "physical_settle_ticks",
        "casualty_reentry_physical_settle_ticks",
        "survivor_reentry_physical_settle_ticks",
        "physical_settle_limit",
        "casualty_settle_ticks",
        "casualty_settle_limit",
        "elapsed_peak",
        "expected_living"
    )) {
        $phase17Metrics += & $newMetric "phase17.counterattack.native_projection.$suffix" "1"
    }

    $phase24RuntimeOwnerActual = "expected/classified/invalid 1/1/0"
    $phase24ExactAuthorityActual = "orders/open/terminal/invalid 3/1/2/0 | projections 1 | V/M/P/D 0/1/0/0 | support leaks 0"
    $phase24Assertions = @(
        (& $newAssertion "phase24.escalation.runtime_owner_classification" "PASS" $phase24RuntimeOwnerActual),
        (& $newAssertion "phase24.escalation.exact_counterattack_authority" "PASS" $phase24ExactAuthorityActual)
    )
    $phase24Metrics = @(
        (& $newMetric "phase24.escalation.runtime_owner_expected" "1"),
        (& $newMetric "phase24.escalation.runtime_owner_classified" "1"),
        (& $newMetric "phase24.escalation.runtime_owner_snapshot_invariant_failures" "0"),
        (& $newMetric "phase24.escalation.exact_counterattack_orders" "3"),
        (& $newMetric "phase24.escalation.exact_counterattack_open_orders" "1"),
        (& $newMetric "phase24.escalation.exact_counterattack_terminal_ledgers" "2"),
        (& $newMetric "phase24.escalation.exact_counterattack_invalid_authority" "0"),
        (& $newMetric "phase24.escalation.exact_counterattack_projection_groups" "1"),
        (& $newMetric "phase24.escalation.exact_counterattack_virtual_groups" "0"),
        (& $newMetric "phase24.escalation.exact_counterattack_materializing_groups" "1"),
        (& $newMetric "phase24.escalation.exact_counterattack_physical_groups" "0"),
        (& $newMetric "phase24.escalation.exact_counterattack_dematerializing_groups" "0"),
        (& $newMetric "phase24.escalation.exact_counterattack_support_leaks" "0")
    )

    $cases = @(
        (& $newCase "phase25.manual_external_gaps"),
        (& $newCase "observation.final_report"),
        (& $newCase "phase24.phase24_final_report"),
        (& $newCase "cleanup.enemy_orders.run_completion"),
        (& $newCase "cleanup.player_marker_completion"),
        (& $newCase "phase17.phase17_report" "PASS" $phase17Assertions $phase17Metrics),
        (& $newCase "phase24.phase24_escalation_pressure" "PASS" $phase24Assertions $phase24Metrics)
    )

    foreach ($suffix in @(
        "partial-cancel_start",
        "partial-cancel_transition",
        "success_member_transition",
        "success_and_failure_fixture_transition",
        "failure_member_transition",
        "same-wave_failure_capture"
    )) {
        $syntheticAppliedGrace = 1
        if ($suffix -eq "partial-cancel_start" -or
            $suffix -eq "success_and_failure_fixture_transition") {
            $syntheticAppliedGrace = 0
        }
        $stagedAssertions = @(
            (& $newAssertion "post_cleanup.active_groups"),
            (& $newAssertion "post_cleanup.runtime_factions" "PASS" "runtime groups 1 | checked 1 | mismatches 0 | exact staged zero-member grace $syntheticAppliedGrace | first none"),
            (& $newAssertion "post_cleanup.runtime_group_population_settled")
        )
        $stagedMetrics = @(
            (& $newMetric "post_cleanup.orphan_active_groups" "0"),
            (& $newMetric "post_cleanup.runtime_faction_mismatches" "0"),
            (& $newMetric "post_cleanup.runtime_faction_zero_member_grace_candidates" "1"),
            (& $newMetric "post_cleanup.runtime_pending_population_groups" "0")
        )
        $cases += & $newCase "post_case_cleanup.action_mechanic_exact_spawn_adapter_$suffix" "PASS" $stagedAssertions $stagedMetrics
    }

    $cases += & $newCase "cleanup.run_leak_snapshot" "PASS" @((& $newAssertion "cleanup.orphan_active_groups")) @((& $newMetric "cleanup.orphan_active_groups" "0"))
    $cases += & $newCase "cleanup.state_isolation_restore" "BLOCKED" @(
        (& $newAssertion "isolation.snapshot"),
        (& $newAssertion "isolation.state_restore"),
        (& $newAssertion "isolation.persistence_restore"),
        (& $newAssertion "isolation.world_scope" "BLOCKED")
    )

    $runMetrics = @(
        (& $newMetric "run.build.sha" $ExpectedSha "sha" "campaign_debug" "run"),
        (& $newMetric "run.build.utc" $ExpectedUtc "utc" "campaign_debug" "run"),
        (& $newMetric "run.build.label" $ExpectedLabel "label" "campaign_debug" "run"),
        (& $newMetric "run.trigger" "cli_autostart" "source" "campaign_debug" "run")
    )
    $run = [pscustomobject]@{
        m_sRunId = $runId
        m_sProfile = "full_certification"
        m_sBuildSha = $ExpectedSha
        m_sBuildUtc = $ExpectedUtc
        m_sBuildLabel = $ExpectedLabel
        m_iStartedAtSecond = 1
        m_iEndedAtSecond = 2
        m_iPassCount = @($cases | Where-Object { $_.m_sStatus -eq "PASS" }).Count
        m_iWarnCount = @($cases | Where-Object { $_.m_sStatus -eq "WARN" }).Count
        m_iFailCount = @($cases | Where-Object { $_.m_sStatus -eq "FAIL" }).Count
        m_iBlockedCount = @($cases | Where-Object { $_.m_sStatus -eq "BLOCKED" }).Count
        m_iSkippedCount = @($cases | Where-Object { $_.m_sStatus -eq "SKIPPED" }).Count
        m_iCertificationRequiredCount = 0
        m_iCertificationProvenCount = 0
        m_iCertificationFailCount = 0
        m_iCertificationBlockedCount = 0
        m_iCertificationWarnCount = 0
        m_bCertificationPassed = $false
        m_aCases = $cases
        m_aMetrics = $runMetrics
        m_aArtifacts = @($jsonName, $summaryName, $stateDiffName)
    }

    [IO.File]::WriteAllText(
        $jsonPath,
        ($run | ConvertTo-Json -Depth 12),
        (New-Object Text.UTF8Encoding($false)))
    $summaryLines = @(
        "Partisan campaign debug complete",
        "run $runId",
        "profile full_certification",
        "build source $ExpectedSha | UTC $ExpectedUtc | label $ExpectedLabel"
    )
    [IO.File]::WriteAllLines(
        $summaryPath,
        $summaryLines,
        (New-Object Text.UTF8Encoding($false)))
    $stateDiffLines = New-Object Collections.Generic.List[string]
    $stateDiffLines.Add("Partisan campaign debug state diff")
    $stateDiffLines.Add("run $runId")
    foreach ($label in @(Get-CampaignDebugStateDiffLabels)) {
        $stateDiffLines.Add("$label 0 -> 0 | delta 0")
    }
    [IO.File]::WriteAllLines(
        $stateDiffPath,
        $stateDiffLines.ToArray(),
        (New-Object Text.UTF8Encoding($false)))

    $parameters = @{
        JsonPath = $jsonPath
        SummaryPath = $summaryPath
        StateDiffPath = $stateDiffPath
        ExpectedSha = $ExpectedSha
        ExpectedUtc = $ExpectedUtc
        ExpectedLabel = $ExpectedLabel
    }
    $result = Test-CampaignDebugArtifacts @parameters
    if (-not $result.Valid) {
        throw "Synthetic artifact validator self-test failed."
    }
    $runtimeOwnerDiagnostic = @($result.Phase24 | Where-Object {
        $_.Id -eq "phase24.escalation.runtime_owner_classification"
    })
    $exactAuthorityDiagnostic = @($result.Phase24 | Where-Object {
        $_.Id -eq "phase24.escalation.exact_counterattack_authority"
    })
    if ($runtimeOwnerDiagnostic.Count -ne 1 -or
        $runtimeOwnerDiagnostic[0].Status -ne "PASS" -or
        -not $runtimeOwnerDiagnostic[0].Accepted -or
        $runtimeOwnerDiagnostic[0].Actual -ne $phase24RuntimeOwnerActual -or
        $exactAuthorityDiagnostic.Count -ne 1 -or
        $exactAuthorityDiagnostic[0].Status -ne "PASS" -or
        -not $exactAuthorityDiagnostic[0].Accepted -or
        $exactAuthorityDiagnostic[0].Actual -ne $phase24ExactAuthorityActual -or
        $result.Phase24Metrics.runtime_owner_snapshot_invariant_failures -ne "0" -or
        $result.Phase24Metrics.exact_counterattack_orders -ne "3" -or
        $result.Phase24Metrics.exact_counterattack_invalid_authority -ne "0" -or
        $result.Phase24Metrics.exact_counterattack_invalid_authority_rows -ne "0" -or
        $result.Phase24Metrics.exact_counterattack_support_leaks -ne "0" -or
        $result.Phase24Metrics.exact_counterattack_virtual_groups -ne "0" -or
        $result.Phase24Metrics.exact_counterattack_materializing_groups -ne "1" -or
        $result.Phase24Metrics.exact_counterattack_physical_groups -ne "0" -or
        $result.Phase24Metrics.exact_counterattack_dematerializing_groups -ne "0") {
        throw "Synthetic Phase 24 artifact diagnostics self-test failed."
    }

    $originalJson = $run | ConvertTo-Json -Depth 12
    $skippedExactRun = $originalJson | ConvertFrom-Json
    $skippedExactCase = @($skippedExactRun.m_aCases | Where-Object {
        $_.m_sCaseId -eq "phase24.phase24_escalation_pressure"
    })[0]
    $skippedExactAssertion = @($skippedExactCase.m_aAssertions | Where-Object {
        $_.m_sAssertionId -eq "phase24.escalation.exact_counterattack_authority"
    })[0]
    $skippedExactAssertion.m_sStatus = "SKIPPED"
    $skippedExactAssertion.m_sActual = "orders/open/terminal/invalid 0/0/0/0 | projections 0 | V/M/P/D 0/0/0/0 | support leaks 0"
    foreach ($metric in @($skippedExactCase.m_aMetrics | Where-Object {
        $_.m_sMetricId -like "phase24.escalation.exact_counterattack_*"
    })) {
        $metric.m_sValue = "0"
    }
    [IO.File]::WriteAllText(
        $jsonPath,
        ($skippedExactRun | ConvertTo-Json -Depth 12),
        (New-Object Text.UTF8Encoding($false)))
    $skippedExactResult = Test-CampaignDebugArtifacts @parameters
    $skippedExactDiagnostic = @($skippedExactResult.Phase24 | Where-Object {
        $_.Id -eq "phase24.escalation.exact_counterattack_authority"
    })
    if (-not $skippedExactResult.Valid -or
        $skippedExactDiagnostic.Count -ne 1 -or
        $skippedExactDiagnostic[0].Pass -or
        -not $skippedExactDiagnostic[0].Accepted -or
        $skippedExactDiagnostic[0].Status -ne "SKIPPED") {
        throw "Synthetic zero-evidence Phase 24 exact-authority skip self-test failed."
    }
    (@($skippedExactCase.m_aMetrics | Where-Object {
        $_.m_sMetricId -eq "phase24.escalation.exact_counterattack_orders"
    })[0]).m_sValue = "1"
    [IO.File]::WriteAllText(
        $jsonPath,
        ($skippedExactRun | ConvertTo-Json -Depth 12),
        (New-Object Text.UTF8Encoding($false)))
    $invalidSkippedExactResult = Test-CampaignDebugArtifacts @parameters
    if ($invalidSkippedExactResult.Valid -or
        ($invalidSkippedExactResult.Problems -notcontains "phase24-exact-counterattack-metrics" -and
            $invalidSkippedExactResult.Problems -notcontains "phase24-exact-counterattack-skipped-nonzero")) {
        throw "Synthetic nonzero Phase 24 exact-authority skip self-test failed."
    }
    [IO.File]::WriteAllText(
        $jsonPath,
        $originalJson,
        (New-Object Text.UTF8Encoding($false)))
    $negativeContracts = @(
        "whole-case",
        "active-groups",
        "runtime-factions",
        "population-settled",
        "orphan-active-groups",
        "runtime-faction-mismatches",
        "zero-member-grace-applied",
        "zero-member-grace-candidates",
        "pending-population"
    )
    $negativeChecks = New-Object Collections.Generic.List[string]
    foreach ($contract in $negativeContracts) {
        $mutatedRun = $originalJson | ConvertFrom-Json
        $targetCase = @($mutatedRun.m_aCases | Where-Object {
            $_.m_sCaseId -eq "post_case_cleanup.action_mechanic_exact_spawn_adapter_partial-cancel_start"
        })[0]
        switch ($contract) {
            "whole-case" {
                $targetCase.m_sStatus = "BLOCKED"
            }
            "active-groups" {
                (@($targetCase.m_aAssertions | Where-Object {
                    $_.m_sAssertionId -eq "post_cleanup.active_groups"
                })[0]).m_sStatus = "BLOCKED"
            }
            "runtime-factions" {
                (@($targetCase.m_aAssertions | Where-Object {
                    $_.m_sAssertionId -eq "post_cleanup.runtime_factions"
                })[0]).m_sStatus = "BLOCKED"
            }
            "population-settled" {
                (@($targetCase.m_aAssertions | Where-Object {
                    $_.m_sAssertionId -eq "post_cleanup.runtime_group_population_settled"
                })[0]).m_sStatus = "BLOCKED"
            }
            "orphan-active-groups" {
                (@($targetCase.m_aMetrics | Where-Object {
                    $_.m_sMetricId -eq "post_cleanup.orphan_active_groups"
                })[0]).m_sValue = "1"
            }
            "runtime-faction-mismatches" {
                (@($targetCase.m_aMetrics | Where-Object {
                    $_.m_sMetricId -eq "post_cleanup.runtime_faction_mismatches"
                })[0]).m_sValue = "1"
            }
            "zero-member-grace-applied" {
                (@($targetCase.m_aAssertions | Where-Object {
                    $_.m_sAssertionId -eq "post_cleanup.runtime_factions"
                })[0]).m_sActual = "runtime groups 1 | checked 1 | mismatches 0 | exact staged zero-member grace 1 | first none"
            }
            "zero-member-grace-candidates" {
                (@($targetCase.m_aMetrics | Where-Object {
                    $_.m_sMetricId -eq "post_cleanup.runtime_faction_zero_member_grace_candidates"
                })[0]).m_sValue = "0"
            }
            "pending-population" {
                (@($targetCase.m_aMetrics | Where-Object {
                    $_.m_sMetricId -eq "post_cleanup.runtime_pending_population_groups"
                })[0]).m_sValue = "1"
            }
        }
        [IO.File]::WriteAllText(
            $jsonPath,
            ($mutatedRun | ConvertTo-Json -Depth 12),
            (New-Object Text.UTF8Encoding($false)))
        $negativeResult = Test-CampaignDebugArtifacts @parameters
        if ($negativeResult.Valid -or
            $negativeResult.Problems -notcontains "staged-partial-cancel_start") {
            throw "Synthetic staged-cleanup negative contract self-test failed."
        }
        [void]$negativeChecks.Add($contract)
    }
    [IO.File]::WriteAllText(
        $jsonPath,
        $originalJson,
        (New-Object Text.UTF8Encoding($false)))

    $focusedCases = New-Object Collections.Generic.List[object]
    $focusedCaseManifest = @(Get-CorrectedCanaryCaseManifest -RunId $runId)
    $focusedAssertionManifest = @(
        Get-CorrectedCanaryAssertionManifest -RunId $runId)
    foreach ($caseManifestRow in $focusedCaseManifest) {
        $caseAssertions = New-Object Collections.Generic.List[object]
        foreach ($assertionManifestRow in @($focusedAssertionManifest |
            Where-Object { [string]$_.CaseId -ceq [string]$caseManifestRow.Id })) {
            $assertionParameters = @{
                Id = [string]$assertionManifestRow.Id
                Status = [string]$assertionManifestRow.Status
                Actual = "synthetic accepted result"
                CountsTowardCertification =
                    [bool]$assertionManifestRow.CountsTowardCertification
            }
            if ([string]$assertionManifestRow.Id -ceq
                "cleanup.player_marker.live") {
                $assertionParameters.Expected =
                    "enabled player marker service has desired/tracked/live marker after cleanup"
                $assertionParameters.Actual =
                    "enabled 1 | desired 0 | tracked 0 | live 0 | entry 1"
                $assertionParameters.FailureReason =
                    "player marker did not reconcile after campaign debug completion"
                $assertionParameters.ProofLevel = "STATE_ONLY"
                $assertionParameters.ObservedPath = "diagnostic_only"
                $assertionParameters.RequiredPath =
                    "no debug-owned state or world leak"
            }
            elseif ([string]$assertionManifestRow.Id -ceq
                "isolation.world_scope") {
                $assertionParameters.Expected =
                    "runtime certification remains scoped to the disposable development session"
                $assertionParameters.Actual =
                    "world runtime, player inventory, health, and service caches require session restart before another certifying run"
                $assertionParameters.FailureReason =
                    "restart the disposable development session before another certification run"
                $assertionParameters.ProofLevel = "EXTERNAL_PROCESS"
                $assertionParameters.ObservedPath = "manual_external_gap"
                $assertionParameters.RequiredPath =
                    "external process restart, reconnect, or long-soak harness"
            }
            elseif ([string]$assertionManifestRow.Id -ceq
                "cleanup.orphan_active_groups") {
                $assertionParameters.Expected =
                    "no active groups without zone/mission/support/order/QRF backing"
                $assertionParameters.Actual =
                    "0 | total 0 | debug 0 | smoke 0 | other 0"
                $assertionParameters.FailureReason =
                    "orphan active groups remain after debug run"
                $assertionParameters.ProofLevel = "STATE_ONLY"
                $assertionParameters.ObservedPath = "cleanup_probe"
                $assertionParameters.RequiredPath =
                    "no debug-owned state or world leak"
            }
            [void]$caseAssertions.Add((& $newAssertion @assertionParameters))
        }
        $caseMetrics = @()
        if ([string]$caseManifestRow.Id -ceq "cleanup.run_leak_snapshot") {
            $caseMetrics = @((& $newMetric `
                "cleanup.orphan_active_groups" `
                "0" `
                "count" `
                "campaign_debug" `
                "final"))
        }
        [void]$focusedCases.Add((& $newCase `
            -Id ([string]$caseManifestRow.Id) `
            -Status ([string]$caseManifestRow.Status) `
            -Assertions $caseAssertions.ToArray() `
            -Metrics $caseMetrics `
            -Category ([string]$caseManifestRow.Category) `
            -Feature ([string]$caseManifestRow.Feature) `
            -Stage ([string]$caseManifestRow.Stage)))
    }
    $focusedRun = [pscustomobject]@{
        m_sRunId = $runId
        m_sProfile = "force_authority"
        m_sBuildSha = $ExpectedSha
        m_sBuildUtc = $ExpectedUtc
        m_sBuildLabel = $ExpectedLabel
        m_iStartedAtSecond = 1
        m_iEndedAtSecond = 2
        m_iPassCount = @($focusedCases | Where-Object { $_.m_sStatus -eq "PASS" }).Count
        m_iWarnCount = @($focusedCases | Where-Object { $_.m_sStatus -eq "WARN" }).Count
        m_iFailCount = 0
        m_iBlockedCount = @($focusedCases | Where-Object {
            $_.m_sStatus -eq "BLOCKED"
        }).Count
        m_iSkippedCount = 0
        m_iCertificationRequiredCount = 87
        m_iCertificationProvenCount = 87
        m_iCertificationFailCount = 0
        m_iCertificationBlockedCount = 0
        m_iCertificationWarnCount = 0
        m_bCertificationPassed = $false
        m_aCases = $focusedCases.ToArray()
        m_aMetrics = $runMetrics
        m_aArtifacts = @($jsonName, $summaryName, $stateDiffName)
    }
    $focusedJson = $focusedRun | ConvertTo-Json -Depth 12
    [IO.File]::WriteAllText(
        $jsonPath,
        $focusedJson,
        (New-Object Text.UTF8Encoding($false)))
    [IO.File]::WriteAllLines(
        $summaryPath,
        @(
            "Partisan campaign debug complete",
            "run $runId",
            "profile force_authority",
            "build source $ExpectedSha | UTC $ExpectedUtc | label $ExpectedLabel"
        ),
        (New-Object Text.UTF8Encoding($false)))
    $focusedParameters = @{
        JsonPath = $jsonPath
        SummaryPath = $summaryPath
        StateDiffPath = $stateDiffPath
        ExpectedSha = $ExpectedSha
        ExpectedUtc = $ExpectedUtc
        ExpectedLabel = $ExpectedLabel
        ExpectedProfile = "force_authority"
        RequireCorrectedCanaryContract = $true
    }
    $focusedResult = Test-CampaignDebugArtifacts @focusedParameters
    if (-not $focusedResult.Valid -or
        $focusedResult.FullCertification -or
        -not $focusedResult.CorrectedCanaryContract -or
        -not $focusedResult.StateDiffManifestExact -or
        -not $focusedResult.CorrectedCanaryAssertionManifestExact -or
        -not $focusedResult.CorrectedCanaryWarningContractExact -or
        -not $focusedResult.CorrectedCanaryBlockedContractExact -or
        -not $focusedResult.CorrectedCanaryOrphanContractExact -or
        $focusedResult.ProofScope -ne "focused_force_authority" -or
        $focusedResult.FocusedCaseId -ne "early_mechanics.force_authority" -or
        @($focusedResult.FocusedAssertions).Count -ne @(Get-FocusedForceAuthorityAssertionIds).Count) {
        throw "Synthetic focused force-authority validator self-test failed."
    }

    $focusedNegativeChecks = New-Object Collections.Generic.List[string]
    $assertFocusedMutationRejected = {
        param(
            [Parameter(Mandatory = $true)][string]$Name,
            [Parameter(Mandatory = $true)]$MutatedRun,
            [Parameter(Mandatory = $true)][string[]]$ExpectedProblems
        )

        [IO.File]::WriteAllText(
            $jsonPath,
            ($MutatedRun | ConvertTo-Json -Depth 12),
            (New-Object Text.UTF8Encoding($false)))
        $mutationResult = Test-CampaignDebugArtifacts @focusedParameters
        if ($mutationResult.Valid) {
            throw "Synthetic focused $Name mutation was accepted."
        }
        foreach ($expectedProblem in $ExpectedProblems) {
            if ($mutationResult.Problems -notcontains $expectedProblem) {
                throw "Synthetic focused $Name mutation did not report $expectedProblem."
            }
        }
        [void]$focusedNegativeChecks.Add($Name)
    }

    $failedCaseRun = $focusedJson | ConvertFrom-Json
    (@($failedCaseRun.m_aCases | Where-Object {
        $_.m_sCaseId -eq "early_mechanics.force_authority"
    })[0]).m_sStatus = "FAIL"
    $failedCaseRun.m_iPassCount--
    $failedCaseRun.m_iFailCount++
    [IO.File]::WriteAllText(
        $jsonPath,
        ($failedCaseRun | ConvertTo-Json -Depth 12),
        (New-Object Text.UTF8Encoding($false)))
    $failedCaseResult = Test-CampaignDebugArtifacts @focusedParameters
    if ($failedCaseResult.Valid -or
        $failedCaseResult.Problems -notcontains "focused-force-authority-case-status") {
        throw "Synthetic focused force-authority case-status self-test failed."
    }
    [void]$focusedNegativeChecks.Add("case-status")

    $failedAssertionRun = $focusedJson | ConvertFrom-Json
    $failedAssertionCase = @($failedAssertionRun.m_aCases | Where-Object {
        $_.m_sCaseId -eq "early_mechanics.force_authority"
    })[0]
    (@($failedAssertionCase.m_aAssertions | Where-Object {
        $_.m_sAssertionId -eq "combat_presence.aggregate"
    })[0]).m_sStatus = "FAIL"
    [IO.File]::WriteAllText(
        $jsonPath,
        ($failedAssertionRun | ConvertTo-Json -Depth 12),
        (New-Object Text.UTF8Encoding($false)))
    $failedAssertionResult = Test-CampaignDebugArtifacts @focusedParameters
    if ($failedAssertionResult.Valid -or
        $failedAssertionResult.Problems -notcontains "focused-combat_presence.aggregate") {
        throw "Synthetic focused assertion-failure self-test failed."
    }
    [void]$focusedNegativeChecks.Add("assertion-failure")

    $unexpectedAssertionRun = $focusedJson | ConvertFrom-Json
    $unexpectedAssertionCase = @($unexpectedAssertionRun.m_aCases | Where-Object {
        $_.m_sCaseId -eq "early_mechanics.force_authority"
    })[0]
    $unexpectedAssertionCase.m_aAssertions +=
        & $newAssertion "town_influence.unexpected_contract"
    [IO.File]::WriteAllText(
        $jsonPath,
        ($unexpectedAssertionRun | ConvertTo-Json -Depth 12),
        (New-Object Text.UTF8Encoding($false)))
    $unexpectedAssertionResult = Test-CampaignDebugArtifacts @focusedParameters
    if ($unexpectedAssertionResult.Valid -or
        $unexpectedAssertionResult.Problems -notcontains "focused-unexpected-town_influence.unexpected_contract") {
        throw "Synthetic focused assertion-set self-test failed."
    }
    [void]$focusedNegativeChecks.Add("assertion-set")

    $certificationClaimRun = $focusedJson | ConvertFrom-Json
    $certificationClaimRun.m_bCertificationPassed = $true
    [IO.File]::WriteAllText(
        $jsonPath,
        ($certificationClaimRun | ConvertTo-Json -Depth 12),
        (New-Object Text.UTF8Encoding($false)))
    $certificationClaimResult = Test-CampaignDebugArtifacts @focusedParameters
    if ($certificationClaimResult.Valid -or
        $certificationClaimResult.Problems -notcontains "focused-proof-claimed-full-certification") {
        throw "Synthetic focused certification-claim self-test failed."
    }
    [void]$focusedNegativeChecks.Add("certification-claim")

    $missingCaseRun = $focusedJson | ConvertFrom-Json
    $missingCaseRun.m_aCases = @($missingCaseRun.m_aCases | Where-Object {
        [string]$_.m_sCaseId -cne
            "post_case_cleanup.preflight_state_isolation"
    })
    & $assertFocusedMutationRejected `
        -Name "case-set" `
        -MutatedRun $missingCaseRun `
        -ExpectedProblems @("corrected-canary-case-set")

    $hiddenCertificationFailRun = $focusedJson | ConvertFrom-Json
    $hiddenCertificationFailCase = @(
        $hiddenCertificationFailRun.m_aCases | Where-Object {
            [string]$_.m_sCaseId -ceq "preflight.state_isolation"
        })[0]
    $hiddenCertificationFailCase.m_aAssertions[0].m_sStatus = "FAIL"
    & $assertFocusedMutationRejected `
        -Name "hidden-certification-fail" `
        -MutatedRun $hiddenCertificationFailRun `
        -ExpectedProblems @(
            "corrected-canary-hidden-fail-or-skip",
            "corrected-canary-certification-assertion-census")

    $certificationFlagRun = $focusedJson | ConvertFrom-Json
    $certificationFlagCase = @($certificationFlagRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq "preflight.state_isolation"
        })[0]
    $certificationFlagCase.m_aAssertions[0].m_bCountsTowardCertification =
        $false
    & $assertFocusedMutationRejected `
        -Name "certification-flag" `
        -MutatedRun $certificationFlagRun `
        -ExpectedProblems @(
            "corrected-canary-certification-assertion-census")

    $misplacedWarningRun = $focusedJson | ConvertFrom-Json
    $misplacedWarningSource = @($misplacedWarningRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq "cleanup.player_marker_completion"
        })[0]
    $misplacedWarningTarget = @($misplacedWarningRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq
                "post_case_cleanup.preflight_state_isolation"
        })[0]
    $misplacedWarningAssertion = @($misplacedWarningSource.m_aAssertions |
        Where-Object {
            [string]$_.m_sAssertionId -ceq "cleanup.player_marker.live"
        })[0]
    $misplacedWarningSource.m_aAssertions = @(
        $misplacedWarningSource.m_aAssertions | Where-Object {
            [string]$_.m_sAssertionId -cne "cleanup.player_marker.live"
        })
    $misplacedWarningTarget.m_aAssertions = @($misplacedWarningAssertion)
    & $assertFocusedMutationRejected `
        -Name "warning-parent-link" `
        -MutatedRun $misplacedWarningRun `
        -ExpectedProblems @("corrected-canary-warning-contract")

    $misplacedWorldBlockRun = $focusedJson | ConvertFrom-Json
    $misplacedWorldBlockSource = @($misplacedWorldBlockRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq "cleanup.state_isolation_restore"
        })[0]
    $misplacedWorldBlockTarget = @($misplacedWorldBlockRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq
                "post_case_cleanup.early_mechanics_force_authority"
        })[0]
    $misplacedWorldBlockAssertion = @(
        $misplacedWorldBlockSource.m_aAssertions | Where-Object {
            [string]$_.m_sAssertionId -ceq "isolation.world_scope"
        })[0]
    $misplacedWorldBlockSource.m_aAssertions = @(
        $misplacedWorldBlockSource.m_aAssertions | Where-Object {
            [string]$_.m_sAssertionId -cne "isolation.world_scope"
        })
    $misplacedWorldBlockTarget.m_aAssertions = @(
        $misplacedWorldBlockTarget.m_aAssertions +
        @($misplacedWorldBlockAssertion))
    & $assertFocusedMutationRejected `
        -Name "world-block-parent-link" `
        -MutatedRun $misplacedWorldBlockRun `
        -ExpectedProblems @("corrected-canary-blocked-contract")

    $orphanMetadataRun = $focusedJson | ConvertFrom-Json
    $orphanMetadataCase = @($orphanMetadataRun.m_aCases | Where-Object {
        [string]$_.m_sCaseId -ceq "cleanup.run_leak_snapshot"
    })[0]
    $orphanMetadataAssertion = @($orphanMetadataCase.m_aAssertions |
        Where-Object {
            [string]$_.m_sAssertionId -ceq "cleanup.orphan_active_groups"
        })[0]
    $orphanMetadataAssertion.m_sObservedPath = "Cleanup_probe"
    & $assertFocusedMutationRejected `
        -Name "orphan-metadata-casing" `
        -MutatedRun $orphanMetadataRun `
        -ExpectedProblems @("corrected-canary-orphan-contract")

    $orphanActualRun = $focusedJson | ConvertFrom-Json
    $orphanActualCase = @($orphanActualRun.m_aCases | Where-Object {
        [string]$_.m_sCaseId -ceq "cleanup.run_leak_snapshot"
    })[0]
    $orphanActualAssertion = @($orphanActualCase.m_aAssertions |
        Where-Object {
            [string]$_.m_sAssertionId -ceq "cleanup.orphan_active_groups"
        })[0]
    $orphanActualAssertion.m_sActual =
        "0 | total 1 | debug 0 | smoke 0 | other 1"
    & $assertFocusedMutationRejected `
        -Name "orphan-actual" `
        -MutatedRun $orphanActualRun `
        -ExpectedProblems @("corrected-canary-orphan-contract")

    $orphanMetricNameRun = $focusedJson | ConvertFrom-Json
    $orphanMetricNameCase = @($orphanMetricNameRun.m_aCases | Where-Object {
        [string]$_.m_sCaseId -ceq "cleanup.run_leak_snapshot"
    })[0]
    $orphanMetricNameCase.m_aMetrics[0].m_sName =
        "cleanup.Orphan_active_groups"
    & $assertFocusedMutationRejected `
        -Name "orphan-metric-name-casing" `
        -MutatedRun $orphanMetricNameRun `
        -ExpectedProblems @("corrected-canary-orphan-contract")

    $mixedCaseProfileRun = $focusedJson | ConvertFrom-Json
    $mixedCaseProfileRun.m_sProfile = "Force_Authority"
    & $assertFocusedMutationRejected `
        -Name "profile-casing" `
        -MutatedRun $mixedCaseProfileRun `
        -ExpectedProblems @("corrected-canary-profile-contract")

    $mixedCaseFocusedIdRun = $focusedJson | ConvertFrom-Json
    $mixedCaseFocusedIdCase = @($mixedCaseFocusedIdRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq "early_mechanics.force_authority"
        })[0]
    $mixedCaseFocusedIdCase.m_aAssertions[0].m_sAssertionId =
        ([string]$mixedCaseFocusedIdCase.m_aAssertions[0].m_sAssertionId).
            ToUpperInvariant()
    & $assertFocusedMutationRejected `
        -Name "focused-id-casing" `
        -MutatedRun $mixedCaseFocusedIdRun `
        -ExpectedProblems @(
            "corrected-canary-focused-assertion-contract")

    $mixedCaseFocusedStatusRun = $focusedJson | ConvertFrom-Json
    $mixedCaseFocusedStatusCase = @(
        $mixedCaseFocusedStatusRun.m_aCases | Where-Object {
            [string]$_.m_sCaseId -ceq "early_mechanics.force_authority"
        })[0]
    $mixedCaseFocusedStatusCase.m_aAssertions[0].m_sStatus = "pass"
    & $assertFocusedMutationRejected `
        -Name "focused-status-casing" `
        -MutatedRun $mixedCaseFocusedStatusRun `
        -ExpectedProblems @(
            "corrected-canary-assertion-status-contract",
            "corrected-canary-focused-assertion-contract")

    $hiddenSkippedRun = $focusedJson | ConvertFrom-Json
    $hiddenSkippedCase = @($hiddenSkippedRun.m_aCases | Where-Object {
        [string]$_.m_sCaseId -ceq "cleanup.player_marker_completion"
    })[0]
    $hiddenSkippedCase.m_aAssertions +=
        & $newAssertion "corrected_canary.synthetic_hidden_skip" "SKIPPED"
    & $assertFocusedMutationRejected `
        -Name "hidden-noncertifying-skip" `
        -MutatedRun $hiddenSkippedRun `
        -ExpectedProblems @("corrected-canary-hidden-fail-or-skip")

    $certificationCounterRun = $focusedJson | ConvertFrom-Json
    $certificationCounterRun.m_iCertificationProvenCount = 86
    & $assertFocusedMutationRejected `
        -Name "certification-counter" `
        -MutatedRun $certificationCounterRun `
        -ExpectedProblems @("corrected-canary-certification-census")

    $mixedCaseCaseStatusRun = $focusedJson | ConvertFrom-Json
    $mixedCaseCaseStatus = @($mixedCaseCaseStatusRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq
                "post_case_cleanup.preflight_state_isolation"
        })[0]
    $mixedCaseCaseStatus.m_sStatus = "pass"
    & $assertFocusedMutationRejected `
        -Name "case-status-casing" `
        -MutatedRun $mixedCaseCaseStatusRun `
        -ExpectedProblems @("corrected-canary-case-status-contract")

    $duplicateAssertionIdRun = $focusedJson | ConvertFrom-Json
    $duplicateAssertionIdCase = @($duplicateAssertionIdRun.m_aCases |
        Where-Object {
            [string]$_.m_sCaseId -ceq
                "post_case_cleanup.preflight_state_isolation"
        })[0]
    $duplicateAssertionIdCase.m_aAssertions = @(
        (& $newAssertion "corrected_canary.synthetic_duplicate"),
        (& $newAssertion "corrected_canary.synthetic_duplicate"))
    & $assertFocusedMutationRejected `
        -Name "duplicate-assertion-id" `
        -MutatedRun $duplicateAssertionIdRun `
        -ExpectedProblems @("corrected-canary-assertion-id-contract")

    [IO.File]::WriteAllText(
        $jsonPath,
        $originalJson,
        (New-Object Text.UTF8Encoding($false)))
    [IO.File]::WriteAllLines(
        $summaryPath,
        $summaryLines,
        (New-Object Text.UTF8Encoding($false)))
    $result | Add-Member -NotePropertyName NegativeStagedContractChecks -NotePropertyValue $negativeChecks.ToArray()
    $result | Add-Member -NotePropertyName FocusedValidatorChecks -NotePropertyValue $focusedNegativeChecks.ToArray()
    return $result
}

function Get-CampaignDebugHardDiagnosticCensus {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$ScriptText,

        [AllowEmptyString()]
        [string]$ConsoleText = '',

        [ValidateSet("full_certification", "force_authority")]
        [Parameter(Mandatory = $true)]
        [string]$Profile,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoyAdmissionDiagnosticsProven,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoySettlementDiagnosticProven,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoyCorruptionDiagnosticsProven,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoyWatchdogDiagnosticProven
    )

    $lines = @($ScriptText -split "`r?`n")
    $hardSignalPattern = '(?i)\b(?<channel>SCRIPT|ENGINE)\s*\(E\)'
    $hardPattern =
        '^\s*(?<timestamp>\d{2}:\d{2}:\d{2}\.\d+)\s+' +
        '(?<channel>SCRIPT|ENGINE)\s+\(E\):\s*(?<message>.*?)\s*$'
    $timestampPattern = '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+'
    $diagnostics = New-Object Collections.Generic.List[object]
    $scriptDiagnosticKeyCounts = New-Object `
        'Collections.Generic.Dictionary[string,int]' `
        ([StringComparer]::Ordinal)
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
        $signalMatch = [Regex]::Match($line, $hardSignalPattern)
        if (-not $signalMatch.Success) {
            continue
        }
        $match = [Regex]::Match($line, $hardPattern)
        if (-not $match.Success) {
            $malformedKey = @(
                'malformed',
                $signalMatch.Groups['channel'].Value,
                (($line -replace '\s+', ' ').Trim())
            ) -join ([char]31)
            if (-not $scriptDiagnosticKeyCounts.ContainsKey($malformedKey)) {
                $scriptDiagnosticKeyCounts[$malformedKey] = 0
            }
            $scriptDiagnosticKeyCounts[$malformedKey]++
            [void]$diagnostics.Add([pscustomobject][ordered]@{
                Index = $index
                Timestamp = ''
                Channel = $signalMatch.Groups['channel'].Value
                Message = 'malformed-hard-diagnostic-header'
                Body = @()
                Source = 'script.log'
                CandidateKind = 'malformed-hard-diagnostic-header'
                Approved = $false
            })
            continue
        }

        $body = New-Object Collections.Generic.List[string]
        for ($bodyIndex = $index + 1; $bodyIndex -lt $lines.Count; $bodyIndex++) {
            $bodyLine = [string]$lines[$bodyIndex]
            if ($bodyLine -match $timestampPattern) {
                break
            }
            $normalizedBodyLine = (($bodyLine -replace '\s+', ' ').Trim())
            if (-not [string]::IsNullOrEmpty($normalizedBodyLine)) {
                [void]$body.Add($normalizedBodyLine)
            }
        }
        $diagnosticKey = @(
            $match.Groups['timestamp'].Value,
            $match.Groups['channel'].Value,
            $match.Groups['message'].Value,
            ($body.ToArray() -join "`n")
        ) -join ([char]31)
        if (-not $scriptDiagnosticKeyCounts.ContainsKey($diagnosticKey)) {
            $scriptDiagnosticKeyCounts[$diagnosticKey] = 0
        }
        $scriptDiagnosticKeyCounts[$diagnosticKey]++
        [void]$diagnostics.Add([pscustomobject][ordered]@{
            Index = $index
            Timestamp = $match.Groups['timestamp'].Value
            Channel = $match.Groups['channel'].Value
            Message = $match.Groups['message'].Value
            Body = $body.ToArray()
            Source = 'script.log'
            CandidateKind = ''
            Approved = $false
        })
    }

    $consoleLines = @($ConsoleText -split "`r?`n")
    $consoleDiagnosticKeyCounts = New-Object `
        'Collections.Generic.Dictionary[string,int]' `
        ([StringComparer]::Ordinal)
    for ($index = 0; $index -lt $consoleLines.Count; $index++) {
        $line = [string]$consoleLines[$index]
        $signalMatch = [Regex]::Match($line, $hardSignalPattern)
        if (-not $signalMatch.Success) {
            continue
        }
        $match = [Regex]::Match($line, $hardPattern)
        if (-not $match.Success) {
            $malformedKey = @(
                'malformed',
                $signalMatch.Groups['channel'].Value,
                (($line -replace '\s+', ' ').Trim())
            ) -join ([char]31)
            if (-not $consoleDiagnosticKeyCounts.ContainsKey($malformedKey)) {
                $consoleDiagnosticKeyCounts[$malformedKey] = 0
            }
            $consoleDiagnosticKeyCounts[$malformedKey]++
            $scriptMalformedCount = 0
            if ($scriptDiagnosticKeyCounts.ContainsKey($malformedKey)) {
                $scriptMalformedCount = $scriptDiagnosticKeyCounts[$malformedKey]
            }
            if ($consoleDiagnosticKeyCounts[$malformedKey] -le $scriptMalformedCount) {
                continue
            }
            [void]$diagnostics.Add([pscustomobject][ordered]@{
                Index = $index
                Timestamp = ''
                Channel = $signalMatch.Groups['channel'].Value
                Message = 'malformed-hard-diagnostic-header'
                Body = @()
                Source = 'console.log'
                CandidateKind = 'malformed-hard-diagnostic-header'
                Approved = $false
            })
            continue
        }

        $body = New-Object Collections.Generic.List[string]
        for ($bodyIndex = $index + 1; $bodyIndex -lt $consoleLines.Count; $bodyIndex++) {
            $bodyLine = [string]$consoleLines[$bodyIndex]
            if ($bodyLine -match $timestampPattern) {
                break
            }
            $normalizedBodyLine = (($bodyLine -replace '\s+', ' ').Trim())
            if (-not [string]::IsNullOrEmpty($normalizedBodyLine)) {
                [void]$body.Add($normalizedBodyLine)
            }
        }
        $diagnosticKey = @(
            $match.Groups['timestamp'].Value,
            $match.Groups['channel'].Value,
            $match.Groups['message'].Value,
            ($body.ToArray() -join "`n")
        ) -join ([char]31)
        if (-not $consoleDiagnosticKeyCounts.ContainsKey($diagnosticKey)) {
            $consoleDiagnosticKeyCounts[$diagnosticKey] = 0
        }
        $consoleDiagnosticKeyCounts[$diagnosticKey]++
        $scriptKeyCount = 0
        if ($scriptDiagnosticKeyCounts.ContainsKey($diagnosticKey)) {
            $scriptKeyCount = $scriptDiagnosticKeyCounts[$diagnosticKey]
        }
        if ($consoleDiagnosticKeyCounts[$diagnosticKey] -le $scriptKeyCount) {
            continue
        }
        [void]$diagnostics.Add([pscustomobject][ordered]@{
            Index = $index
            Timestamp = $match.Groups['timestamp'].Value
            Channel = $match.Groups['channel'].Value
            Message = $match.Groups['message'].Value
            Body = $body.ToArray()
            Source = 'console.log'
            CandidateKind = ''
            Approved = $false
        })
    }

    $editableIdentityBody = @(
        'Reason: Wrong parameter value',
        "Class: 'SCR_EditableEntityCore'",
        "Function: 'GetPlayerIdentityId'",
        'Stack trace:',
        'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
        'Scripts/Game/Editor/Core/SCR_EditableEntityCore.c:1189 Function OnPlayerIdentityAvailable',
        'Scripts/Game/GameMode/SCR_BaseGameMode.c:842 Function OnPlayerAuditSuccess'
    ) -join "`n"
    $reconnectIdentityBody = @(
        'Reason: Wrong parameter value',
        "Class: 'SCR_ReconnectComponent'",
        "Function: 'GetPlayerIdentityId'",
        'Stack trace:',
        'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
        'Scripts/Game/GameMode/Components/SCR_ReconnectComponent.c:135 Function HandlePlayerReconnect',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:534 Function ResolveReconnection',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:329 Function OnPlayerDataLoaded_S',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:298 Function RequestPlayerData_S',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:273 Function ExcuteInitialLoadOrSpawn_S',
        'Scripts/Game/Respawn/Logic/SCR_AutoSpawnLogic.c:18 Function OnPlayerAuditSuccess_S',
        'Scripts/Game/GameMode/Respawn/SCR_RespawnSystemComponent.c:258 Function OnPlayerAuditSuccess_S',
        'Scripts/Game/GameMode/SCR_BaseGameMode.c:845 Function OnPlayerAuditSuccess'
    ) -join "`n"
    foreach ($diagnostic in $diagnostics) {
        if ($diagnostic.Channel -cne 'SCRIPT' -or
            $diagnostic.Source -cne 'script.log' -or
            $diagnostic.Message -cne 'Virtual Machine Exception' -or
            [string]::IsNullOrEmpty($diagnostic.Timestamp)) {
            continue
        }
        $bodyText = @($diagnostic.Body) -join "`n"
        if ($bodyText -ceq $editableIdentityBody) {
            $diagnostic.CandidateKind = 'stock-startup-editable-identity'
        }
        elseif ($bodyText -ceq $reconnectIdentityBody) {
            $diagnostic.CandidateKind = 'stock-startup-reconnect-identity'
        }
    }

    $lifecyclePrefix = '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s*'
    $armMessage = if ($Profile -ceq 'full_certification') {
        'Partisan campaign debug CLI | armed exact HST_Dev full certification run'
    }
    else {
        'Partisan campaign debug CLI | armed focused force_authority run ' +
            '(not full certification)'
    }
    $armPattern = $lifecyclePrefix + [Regex]::Escape($armMessage) + '\s*$'
    $startPattern = $lifecyclePrefix +
        [Regex]::Escape("Partisan campaign debug CLI | started $Profile on attempt ") +
        '\d+\s*$'
    $forceResultPattern = $lifecyclePrefix +
        'Partisan\ campaign\ debug\ \|\ ' +
        '(?:PASS|WARN|FAIL|BLOCKED|SKIPPED)\ \|\ ' +
        'early_mechanics\.force_authority\ \|\ .+\s*$'
    $armIndices = New-Object Collections.Generic.List[int]
    $startIndices = New-Object Collections.Generic.List[int]
    $forceResultIndices = New-Object Collections.Generic.List[int]
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ([string]$lines[$index] -cmatch $armPattern) {
            [void]$armIndices.Add($index)
        }
        if ([string]$lines[$index] -cmatch $startPattern) {
            [void]$startIndices.Add($index)
        }
        if ([string]$lines[$index] -cmatch $forceResultPattern) {
            [void]$forceResultIndices.Add($index)
        }
    }
    $lifecycleMarkersValid = $armIndices.Count -eq 1 -and
        $startIndices.Count -eq 1 -and
        $forceResultIndices.Count -eq 1 -and
        $startIndices[0] -gt $armIndices[0] -and
        $forceResultIndices[0] -gt $startIndices[0]
    $armIndex = if ($armIndices.Count -eq 1) { $armIndices[0] } else { -1 }
    $identityCandidates = @($diagnostics | Where-Object {
        $_.CandidateKind -like 'stock-startup-*-identity'
    })
    $identityPairExact = $lifecycleMarkersValid -and
        $identityCandidates.Count -eq 2 -and
        $identityCandidates[0].CandidateKind -ceq
            'stock-startup-editable-identity' -and
        $identityCandidates[1].CandidateKind -ceq
            'stock-startup-reconnect-identity' -and
        $identityCandidates[1].Index -gt $identityCandidates[0].Index -and
        $armIndex -gt $identityCandidates[1].Index
    if ($identityPairExact) {
        $identityCandidates[0].Approved = $true
        $identityCandidates[1].Approved = $true
    }

    $intentionalMessages = @(
        'Partisan exact mission convoy | mission_convoy_proof_admission_rollback failed closed: exact mission convoy road route is missing or too short',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_duplicate failed closed: exact mission convoy admission contains more than one optional cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_invalid_prefab failed closed: exact mission convoy cargo prefab is missing, invalid, or not an entity prefab',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_wrong_kind failed closed: exact mission convoy cargo role or kind is incompatible with the mission',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_disallowed failed closed: exact mission convoy mission kind does not permit a cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_missing failed closed: exact mission convoy mission kind requires exactly one compatible cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_wrong_runtime failed closed: exact mission convoy runtime type is incompatible with frozen convoy authority',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_captive_non_character failed closed: exact mission convoy captive prefab is not a boardable character with compartment access',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_payload_character failed closed: exact mission convoy payload prefab must be a non-character mission-asset entity',
        'Partisan exact mission convoy | mission_convoy_proof_duplicate_corruption failed closed: exact mission convoy authority identity is ambiguous',
        'Partisan exact mission convoy | mission_convoy_proof_hash_corruption failed closed: exact mission convoy manifest hash changed after admission',
        'Partisan exact mission convoy | mission_convoy_proof_missing_backlink_corruption failed closed: exact mission convoy canonical mission authority links conflict',
        'Partisan exact mission convoy | mission_convoy_proof_watchdog failed closed: all-or-nothing convoy materialization did not confirm every required surviving crew/vehicle root before timeout'
    )
    $intentionalCandidates = New-Object Collections.Generic.List[object]
    foreach ($diagnostic in $diagnostics) {
        if ($diagnostic.Source -cne 'script.log' -or
            $diagnostic.Channel -cne 'SCRIPT' -or
            @($diagnostic.Body).Count -ne 0) {
            continue
        }
        $intentionalIndex = [Array]::IndexOf($intentionalMessages, $diagnostic.Message)
        if ($intentionalIndex -lt 0) {
            continue
        }
        $diagnostic.CandidateKind = "intentional-mission-convoy-$intentionalIndex"
        [void]$intentionalCandidates.Add($diagnostic)
    }
    $intentionalSetExact = $intentionalCandidates.Count -eq $intentionalMessages.Count
    if ($intentionalSetExact) {
        for ($index = 0; $index -lt $intentionalCandidates.Count; $index++) {
            if ($intentionalCandidates[$index].Message -cne $intentionalMessages[$index] -or
                ($index -gt 0 -and
                    $intentionalCandidates[$index].Index -le
                        $intentionalCandidates[$index - 1].Index)) {
                $intentionalSetExact = $false
                break
            }
        }
    }
    $intentionalLifecycleBound = $lifecycleMarkersValid -and
        $intentionalSetExact -and
        $intentionalCandidates[0].Index -gt $startIndices[0] -and
        $intentionalCandidates[$intentionalCandidates.Count - 1].Index -lt
            $forceResultIndices[0]
    if ($Profile -ceq 'full_certification' -and $intentionalLifecycleBound) {
        for ($index = 0; $index -lt $intentionalCandidates.Count; $index++) {
            $proofValid = if ($index -le 8) {
                $IntentionalMissionConvoyAdmissionDiagnosticsProven
            }
            elseif ($index -le 11) {
                $IntentionalMissionConvoyCorruptionDiagnosticsProven
            }
            else {
                $IntentionalMissionConvoyWatchdogDiagnosticProven
            }
            if ($proofValid) {
                $intentionalCandidates[$index].Approved = $true
            }
        }
    }

    $unapproved = @($diagnostics | Where-Object { -not $_.Approved })
    $unapprovedKinds = @($unapproved | ForEach-Object {
        if ($_.Message -ceq 'Virtual Machine Exception') {
            'virtual-machine-exception'
        }
        elseif ($_.Message -cmatch '^(?:Partisan|HST(?:_|\b))') {
            'partisan-script-error'
        }
        elseif ($_.Message -cmatch '(?i)fatal|crash|access.violation') {
            'crash-or-fatal-error'
        }
        else {
            'runtime-script-error'
        }
    } | Group-Object | Sort-Object Name | ForEach-Object {
        [pscustomobject][ordered]@{
            kind = $_.Name
            count = $_.Count
        }
    })
    $crashPattern = '(?i)(?:\bACCESS_VIOLATION\b|\bunhandled exception\b|' +
        '\bfatal error\b|\bFATAL\s*:|\bapplication crash\b)'
    $crashCountsBySource = @()
    foreach ($sourceText in @($ScriptText, $ConsoleText)) {
        $sourceCounts = New-Object 'Collections.Generic.Dictionary[string,int]' `
            ([StringComparer]::Ordinal)
        foreach ($sourceLine in @($sourceText -split "`r?`n")) {
            if ([string]$sourceLine -cnotmatch $crashPattern) {
                continue
            }
            $crashKey = (([string]$sourceLine -replace '\s+', ' ').Trim())
            if (-not $sourceCounts.ContainsKey($crashKey)) {
                $sourceCounts[$crashKey] = 0
            }
            $sourceCounts[$crashKey]++
        }
        $crashCountsBySource += ,$sourceCounts
    }
    $crashKeys = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($sourceCounts in $crashCountsBySource) {
        foreach ($crashKey in $sourceCounts.Keys) {
            [void]$crashKeys.Add($crashKey)
        }
    }
    $crashMarkers = 0
    foreach ($crashKey in $crashKeys) {
        $maximumCount = 0
        foreach ($sourceCounts in $crashCountsBySource) {
            if ($sourceCounts.ContainsKey($crashKey) -and
                $sourceCounts[$crashKey] -gt $maximumCount) {
                $maximumCount = $sourceCounts[$crashKey]
            }
        }
        $crashMarkers += $maximumCount
    }
    $partisanSeverityPattern = '(?i)\b(?:Partisan|HST(?:_|\b)).*\b(?:ERROR|FATAL)\s*:'
    $partisanSeverityCountsBySource = @()
    foreach ($sourceText in @($ScriptText, $ConsoleText)) {
        $sourceCounts = New-Object 'Collections.Generic.Dictionary[string,int]' `
            ([StringComparer]::Ordinal)
        foreach ($sourceLine in @($sourceText -split "`r?`n")) {
            if ([string]$sourceLine -cnotmatch $partisanSeverityPattern) {
                continue
            }
            $severityKey = (([string]$sourceLine -replace '\s+', ' ').Trim())
            if (-not $sourceCounts.ContainsKey($severityKey)) {
                $sourceCounts[$severityKey] = 0
            }
            $sourceCounts[$severityKey]++
        }
        $partisanSeverityCountsBySource += ,$sourceCounts
    }
    $partisanSeverityKeys = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($sourceCounts in $partisanSeverityCountsBySource) {
        foreach ($severityKey in $sourceCounts.Keys) {
            [void]$partisanSeverityKeys.Add($severityKey)
        }
    }
    $partisanSeverityLineCount = 0
    foreach ($severityKey in $partisanSeverityKeys) {
        $maximumCount = 0
        foreach ($sourceCounts in $partisanSeverityCountsBySource) {
            if ($sourceCounts.ContainsKey($severityKey) -and
                $sourceCounts[$severityKey] -gt $maximumCount) {
                $maximumCount = $sourceCounts[$severityKey]
            }
        }
        $partisanSeverityLineCount += $maximumCount
    }
    $scriptErrors = @($diagnostics | Where-Object {
        $_.Channel -ceq 'SCRIPT'
    }).Count
    $engineErrors = @($diagnostics | Where-Object {
        $_.Channel -ceq 'ENGINE'
    }).Count
    $partisanErrors = @($diagnostics | Where-Object {
        $_.Message -cmatch '^(?:Partisan|HST(?:_|\b))'
    }).Count
    $approvedStockCount = @($diagnostics | Where-Object {
        $_.Approved -and $_.CandidateKind -like 'stock-startup-*-identity'
    }).Count
    $approvedIntentionalCount = @($diagnostics | Where-Object {
        $_.Approved -and $_.CandidateKind -like 'intentional-mission-convoy-*'
    }).Count
    $intentionalFixtureStructureValid = if ($Profile -ceq 'full_certification') {
        $intentionalLifecycleBound
    }
    else {
        $intentionalCandidates.Count -eq 0
    }
    $intentionalFixtureSetValid = if ($Profile -ceq 'full_certification') {
        $intentionalLifecycleBound -and
            $approvedIntentionalCount -eq $intentionalMessages.Count
    }
    else {
        $intentionalCandidates.Count -eq 0
    }
    $channelArithmeticValid = $diagnostics.Count -eq
        ($scriptErrors + $engineErrors)
    $categoryArithmeticValid = $diagnostics.Count -eq
        ($approvedStockCount + $approvedIntentionalCount + $unapproved.Count)

    return [pscustomobject][ordered]@{
        Valid = $lifecycleMarkersValid -and
            $channelArithmeticValid -and
            $categoryArithmeticValid -and
            $intentionalFixtureSetValid -and
            $unapproved.Count -eq 0 -and
            $crashMarkers -eq 0 -and
            $partisanSeverityLineCount -eq 0
        HardDiagnosticFree = $diagnostics.Count -eq 0 -and
            $crashMarkers -eq 0 -and
            $partisanSeverityLineCount -eq 0
        HardDiagnosticCount = $diagnostics.Count
        ScriptErrors = $scriptErrors
        EngineErrors = $engineErrors
        PartisanErrors = $partisanErrors
        CrashMarkers = $crashMarkers
        PartisanSeverityLineCount = $partisanSeverityLineCount
        ApprovedStockDiagnosticCount = $approvedStockCount
        ApprovedIntentionalDiagnosticCount = $approvedIntentionalCount
        ChannelArithmeticValid = $channelArithmeticValid
        CategoryArithmeticValid = $categoryArithmeticValid
        MalformedHardDiagnosticCount = @($diagnostics | Where-Object {
            $_.CandidateKind -ceq 'malformed-hard-diagnostic-header'
        }).Count
        UnapprovedHardDiagnosticCount = $unapproved.Count
        UnapprovedHardDiagnosticKinds = $unapprovedKinds
        LifecycleMarkersValid = $lifecycleMarkersValid
        IdentityBaselinePairValid = $identityCandidates.Count -eq 0 -or
            $identityPairExact
        IntentionalFixtureStructureExact = $intentionalFixtureStructureValid
        IntentionalFixtureSetValid = $intentionalFixtureSetValid
        IntentionalMissionConvoyAdmissionDiagnosticsProven =
            $IntentionalMissionConvoyAdmissionDiagnosticsProven
        IntentionalMissionConvoySettlementDiagnosticProven =
            $IntentionalMissionConvoySettlementDiagnosticProven
        IntentionalMissionConvoyCorruptionDiagnosticsProven =
            $IntentionalMissionConvoyCorruptionDiagnosticsProven
        IntentionalMissionConvoyWatchdogDiagnosticProven =
            $IntentionalMissionConvoyWatchdogDiagnosticProven
    }
}

function Test-CampaignDebugHardDiagnosticCensus {
    $armLine =
        '17:00:01.000 SCRIPT : Partisan campaign debug CLI | armed exact HST_Dev full certification run'
    $startLine =
        '17:00:01.100 SCRIPT : Partisan campaign debug CLI | started full_certification on attempt 1'
    $forceResultLine =
        '17:00:03.000 SCRIPT : Partisan campaign debug | PASS | early_mechanics.force_authority | assertions passed'
    $fullLifecyclePrefix = @($armLine, $startLine)
    $fullLifecycleSuffix = @($forceResultLine)
    $forceArmLine =
        '17:00:01.000 SCRIPT : Partisan campaign debug CLI | armed focused force_authority run (not full certification)'
    $forceStartLine =
        '17:00:01.100 SCRIPT : Partisan campaign debug CLI | started force_authority on attempt 1'
    $forceResultLine =
        '17:00:03.000 SCRIPT : Partisan campaign debug | PASS | early_mechanics.force_authority | assertions passed'
    $forceLifecycleText = @($forceArmLine, $forceStartLine, $forceResultLine) -join "`n"
    $editableBlock = @(
        '17:00:00.000 SCRIPT (E): Virtual Machine Exception',
        '',
        'Reason: Wrong parameter value',
        '',
        "Class: 'SCR_EditableEntityCore'",
        "Function: 'GetPlayerIdentityId'",
        'Stack trace:',
        'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
        'Scripts/Game/Editor/Core/SCR_EditableEntityCore.c:1189 Function OnPlayerIdentityAvailable',
        'Scripts/Game/GameMode/SCR_BaseGameMode.c:842 Function OnPlayerAuditSuccess'
    )
    $reconnectBlock = @(
        '17:00:00.001 SCRIPT (E): Virtual Machine Exception',
        '',
        'Reason: Wrong parameter value',
        '',
        "Class: 'SCR_ReconnectComponent'",
        "Function: 'GetPlayerIdentityId'",
        'Stack trace:',
        'Scripts/Game/Utilities/SCR_PlayerIdentityUtils.c:29 Function GetPlayerIdentityId',
        'Scripts/Game/GameMode/Components/SCR_ReconnectComponent.c:135 Function HandlePlayerReconnect',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:534 Function ResolveReconnection',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:329 Function OnPlayerDataLoaded_S',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:298 Function RequestPlayerData_S',
        'Scripts/Game/Respawn/Logic/SCR_SpawnLogic.c:273 Function ExcuteInitialLoadOrSpawn_S',
        'Scripts/Game/Respawn/Logic/SCR_AutoSpawnLogic.c:18 Function OnPlayerAuditSuccess_S',
        'Scripts/Game/GameMode/Respawn/SCR_RespawnSystemComponent.c:258 Function OnPlayerAuditSuccess_S',
        'Scripts/Game/GameMode/SCR_BaseGameMode.c:845 Function OnPlayerAuditSuccess'
    )
    $identityText = ($editableBlock + $reconnectBlock +
        @($forceArmLine, $forceStartLine, $forceResultLine)) -join "`n"
    $identityValid = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $identityText `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if (-not $identityValid.Valid -or
        $identityValid.HardDiagnosticFree -or
        -not $identityValid.ChannelArithmeticValid -or
        -not $identityValid.CategoryArithmeticValid -or
        $identityValid.HardDiagnosticCount -ne 2 -or
        $identityValid.ApprovedStockDiagnosticCount -ne 2 -or
        $identityValid.UnapprovedHardDiagnosticCount -ne 0) {
        throw 'Campaign Debug timestamped stock-diagnostic classification self-test failed.'
    }

    $clean = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $forceLifecycleText `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if (-not $clean.Valid -or -not $clean.HardDiagnosticFree) {
        throw 'Campaign Debug clean hard-diagnostic self-test failed.'
    }

    $unknown = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText ($identityText + "`n17:00:02.000 SCRIPT (E): unknown") `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($unknown.Valid -or $unknown.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Campaign Debug unknown hard-diagnostic rejection self-test failed.'
    }

    $malformed = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText 'prefix SCRIPT (E): malformed' `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($malformed.Valid -or
        $malformed.HardDiagnosticCount -ne 1 -or
        $malformed.MalformedHardDiagnosticCount -ne 1 -or
        $malformed.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Campaign Debug malformed hard-diagnostic rejection self-test failed.'
    }

    $spacedColon = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText ($forceLifecycleText + "`n17:00:04.000 SCRIPT (E) : unknown") `
        -ConsoleText '17:00:04.000 SCRIPT (E) : unknown' `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($spacedColon.Valid -or
        $spacedColon.HardDiagnosticCount -ne 1 -or
        $spacedColon.MalformedHardDiagnosticCount -ne 1 -or
        $spacedColon.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Campaign Debug spaced-colon hard-diagnostic rejection self-test failed.'
    }
    $compactChannel = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText ($forceLifecycleText + "`n17:00:04.000 SCRIPT(E): unknown") `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($compactChannel.Valid -or
        $compactChannel.HardDiagnosticCount -ne 1 -or
        $compactChannel.MalformedHardDiagnosticCount -ne 1) {
        throw 'Campaign Debug compact-channel hard-diagnostic rejection self-test failed.'
    }
    $lowerMissingColon = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText ($forceLifecycleText + "`n17:00:04.000 script (E) unknown") `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($lowerMissingColon.Valid -or
        $lowerMissingColon.HardDiagnosticCount -ne 1 -or
        $lowerMissingColon.MalformedHardDiagnosticCount -ne 1) {
        throw 'Campaign Debug lowercase missing-colon hard-diagnostic rejection self-test failed.'
    }

    $reversed = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText (($reconnectBlock + $editableBlock +
            @($forceArmLine, $forceStartLine, $forceResultLine)) -join "`n") `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($reversed.Valid -or $reversed.UnapprovedHardDiagnosticCount -ne 2) {
        throw 'Campaign Debug stock-diagnostic ordering rejection self-test failed.'
    }

    $thirdIdentity = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText (($editableBlock + $reconnectBlock + $editableBlock +
            @($forceArmLine, $forceStartLine, $forceResultLine)) -join "`n") `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($thirdIdentity.Valid -or $thirdIdentity.UnapprovedHardDiagnosticCount -ne 3) {
        throw 'Campaign Debug stock-diagnostic exact-count rejection self-test failed.'
    }

    $intentionalMessages = @(
        'Partisan exact mission convoy | mission_convoy_proof_admission_rollback failed closed: exact mission convoy road route is missing or too short',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_duplicate failed closed: exact mission convoy admission contains more than one optional cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_invalid_prefab failed closed: exact mission convoy cargo prefab is missing, invalid, or not an entity prefab',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_wrong_kind failed closed: exact mission convoy cargo role or kind is incompatible with the mission',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_disallowed failed closed: exact mission convoy mission kind does not permit a cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_missing failed closed: exact mission convoy mission kind requires exactly one compatible cargo row',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_wrong_runtime failed closed: exact mission convoy runtime type is incompatible with frozen convoy authority',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_captive_non_character failed closed: exact mission convoy captive prefab is not a boardable character with compartment access',
        'Partisan exact mission convoy | mission_convoy_proof_cargo_payload_character failed closed: exact mission convoy payload prefab must be a non-character mission-asset entity',
        'Partisan exact mission convoy | mission_convoy_proof_duplicate_corruption failed closed: exact mission convoy authority identity is ambiguous',
        'Partisan exact mission convoy | mission_convoy_proof_hash_corruption failed closed: exact mission convoy manifest hash changed after admission',
        'Partisan exact mission convoy | mission_convoy_proof_missing_backlink_corruption failed closed: exact mission convoy canonical mission authority links conflict',
        'Partisan exact mission convoy | mission_convoy_proof_watchdog failed closed: all-or-nothing convoy materialization did not confirm every required surviving crew/vehicle root before timeout'
    )
    $intentionalLines = New-Object Collections.Generic.List[string]
    for ($index = 0; $index -lt $intentionalMessages.Count; $index++) {
        [void]$intentionalLines.Add(
            ('17:00:02.{0:D3} SCRIPT (E): {1}' -f $index, $intentionalMessages[$index]))
    }
    $intentionalText = ($fullLifecyclePrefix +
        $intentionalLines.ToArray() + $fullLifecycleSuffix) -join "`n"
    $intentionalValid = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if (-not $intentionalValid.Valid -or
        -not $intentionalValid.ChannelArithmeticValid -or
        -not $intentionalValid.CategoryArithmeticValid -or
        $intentionalValid.ApprovedIntentionalDiagnosticCount -ne 13 -or
        $intentionalValid.UnapprovedHardDiagnosticCount -ne 0 -or
        $intentionalValid.HardDiagnosticCount -ne
            ($intentionalValid.ApprovedStockDiagnosticCount +
                $intentionalValid.ApprovedIntentionalDiagnosticCount +
                $intentionalValid.UnapprovedHardDiagnosticCount)) {
        throw 'Campaign Debug intentional-fixture classification self-test failed.'
    }

    $missingIntentional = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText (($fullLifecyclePrefix + $fullLifecycleSuffix) -join "`n") `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($missingIntentional.Valid -or
        $missingIntentional.HardDiagnosticCount -ne 0 -or
        $missingIntentional.IntentionalFixtureSetValid) {
        throw 'Campaign Debug missing full-profile intentional-fixture self-test failed.'
    }

    $duplicateIntentionalText = ($fullLifecyclePrefix +
        @($intentionalLines[0]) + $intentionalLines.ToArray() +
        $fullLifecycleSuffix) -join "`n"
    $duplicateIntentional = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $duplicateIntentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($duplicateIntentional.Valid -or
        $duplicateIntentional.HardDiagnosticCount -ne 14 -or
        $duplicateIntentional.ApprovedIntentionalDiagnosticCount -ne 0 -or
        $duplicateIntentional.UnapprovedHardDiagnosticCount -ne 14) {
        throw 'Campaign Debug duplicate intentional-fixture rejection self-test failed.'
    }

    $settlementNotRequired = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if (-not $settlementNotRequired.Valid -or
        $settlementNotRequired.ApprovedIntentionalDiagnosticCount -ne 13 -or
        $settlementNotRequired.UnapprovedHardDiagnosticCount -ne 0) {
        throw 'Campaign Debug obsolete settlement-diagnostic independence self-test failed.'
    }

    $admissionUnproven = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($admissionUnproven.Valid -or
        $admissionUnproven.ApprovedIntentionalDiagnosticCount -ne 4 -or
        $admissionUnproven.UnapprovedHardDiagnosticCount -ne 9) {
        throw 'Campaign Debug intentional admission-diagnostic boundary self-test failed.'
    }

    $corruptionUnproven = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($corruptionUnproven.Valid -or
        $corruptionUnproven.ApprovedIntentionalDiagnosticCount -ne 10 -or
        $corruptionUnproven.UnapprovedHardDiagnosticCount -ne 3) {
        throw 'Campaign Debug intentional corruption-diagnostic boundary self-test failed.'
    }

    $watchdogUnproven = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($watchdogUnproven.Valid -or
        $watchdogUnproven.ApprovedIntentionalDiagnosticCount -ne 12 -or
        $watchdogUnproven.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Campaign Debug intentional watchdog-diagnostic boundary self-test failed.'
    }

    $intentionalUnproven = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($intentionalUnproven.Valid -or
        $intentionalUnproven.UnapprovedHardDiagnosticCount -ne 13) {
        throw 'Campaign Debug unproven intentional-fixture rejection self-test failed.'
    }

    $intentionalPartial = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText (($fullLifecyclePrefix +
            $intentionalLines.ToArray()[0..11] + $fullLifecycleSuffix) -join "`n") `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($intentionalPartial.Valid -or
        $intentionalPartial.UnapprovedHardDiagnosticCount -ne 12) {
        throw 'Campaign Debug partial intentional-fixture rejection self-test failed.'
    }

    $mutatedIntentionalLines = @($intentionalLines.ToArray())
    $mutatedIntentionalLines[$mutatedIntentionalLines.Count - 1] += ' unexpected'
    $intentionalMutationText = ($fullLifecyclePrefix +
        $mutatedIntentionalLines + $fullLifecycleSuffix) -join "`n"
    $intentionalMutation = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalMutationText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($intentionalMutation.Valid -or
        $intentionalMutation.UnapprovedHardDiagnosticCount -ne 13) {
        throw 'Campaign Debug intentional-fixture suffix-mutation self-test failed.'
    }

    $engineIntentionalLines = @($intentionalLines.ToArray() | ForEach-Object {
        $_ -replace ' SCRIPT \(E\): ', ' ENGINE (E): '
    })
    $engineIntentionalText = ($fullLifecyclePrefix +
        $engineIntentionalLines + $fullLifecycleSuffix) -join "`n"
    $engineIntentional = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $engineIntentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($engineIntentional.Valid -or
        $engineIntentional.ApprovedIntentionalDiagnosticCount -ne 0 -or
        $engineIntentional.EngineErrors -ne 13 -or
        $engineIntentional.UnapprovedHardDiagnosticCount -ne 13) {
        throw 'Campaign Debug intentional-fixture channel-mutation self-test failed.'
    }

    $bodyIntentionalLines = New-Object Collections.Generic.List[string]
    [void]$bodyIntentionalLines.Add($intentionalLines[0])
    [void]$bodyIntentionalLines.Add('unexpected continuation body')
    for ($index = 1; $index -lt $intentionalLines.Count; $index++) {
        [void]$bodyIntentionalLines.Add($intentionalLines[$index])
    }
    $bodyIntentionalText = ($fullLifecyclePrefix +
        $bodyIntentionalLines.ToArray() + $fullLifecycleSuffix) -join "`n"
    $bodyIntentional = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $bodyIntentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($bodyIntentional.Valid -or
        $bodyIntentional.ApprovedIntentionalDiagnosticCount -ne 0 -or
        $bodyIntentional.UnapprovedHardDiagnosticCount -ne 13) {
        throw 'Campaign Debug intentional-fixture continuation-body self-test failed.'
    }

    $outOfBoundsIntentionalText = (@($armLine) +
        $intentionalLines.ToArray() + @($startLine, $forceResultLine)) -join "`n"
    $outOfBoundsIntentional = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $outOfBoundsIntentionalText `
        -Profile full_certification `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($outOfBoundsIntentional.Valid -or
        $outOfBoundsIntentional.ApprovedIntentionalDiagnosticCount -ne 0 -or
        $outOfBoundsIntentional.UnapprovedHardDiagnosticCount -ne 13) {
        throw 'Campaign Debug intentional-fixture lifecycle-boundary self-test failed.'
    }

    $canaryIntentional = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText $intentionalText `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $true `
        -IntentionalMissionConvoySettlementDiagnosticProven $true `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $true `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $true
    if ($canaryIntentional.Valid -or
        $canaryIntentional.UnapprovedHardDiagnosticCount -ne 13) {
        throw 'Campaign Debug wrong-profile intentional-fixture rejection self-test failed.'
    }

    $fatal = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText '17:00:03.000 ENGINE (E): fatal error synthetic' `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($fatal.Valid -or $fatal.CrashMarkers -ne 1) {
        throw 'Campaign Debug crash-marker rejection self-test failed.'
    }

    $normalChannelFatal = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText ($forceLifecycleText + "`n17:00:04.000 SCRIPT : FATAL: synthetic") `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($normalChannelFatal.Valid -or
        $normalChannelFatal.HardDiagnosticFree -or
        $normalChannelFatal.HardDiagnosticCount -ne 0 -or
        $normalChannelFatal.CrashMarkers -ne 1) {
        throw 'Campaign Debug normal-channel fatal-marker rejection self-test failed.'
    }
    $normalChannelPartisanError = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText ($forceLifecycleText +
            "`n17:00:04.000 SCRIPT : Partisan subsystem ERROR: synthetic") `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($normalChannelPartisanError.Valid -or
        $normalChannelPartisanError.HardDiagnosticFree -or
        $normalChannelPartisanError.HardDiagnosticCount -ne 0 -or
        $normalChannelPartisanError.PartisanSeverityLineCount -ne 1) {
        throw 'Campaign Debug normal-channel Partisan severity rejection self-test failed.'
    }
    $mirroredFatalLine = '17:00:04.000 SCRIPT : FATAL: mirrored synthetic'
    $mirroredFatal = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText ($forceLifecycleText + "`n" + $mirroredFatalLine) `
        -ConsoleText $mirroredFatalLine `
        -Profile force_authority `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
        -IntentionalMissionConvoySettlementDiagnosticProven $false `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
        -IntentionalMissionConvoyWatchdogDiagnosticProven $false
    if ($mirroredFatal.Valid -or $mirroredFatal.CrashMarkers -ne 1) {
        throw 'Campaign Debug mirrored fatal-marker multiset self-test failed.'
    }

    $tempBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $tempRoot = Join-Path $tempBase (
        'PartisanCampaignDebugCensusSelfTest_' + [Guid]::NewGuid().ToString('N'))
    if (-not (Test-ContainedPath -Root $tempBase -Candidate $tempRoot)) {
        throw 'Campaign Debug census self-test root escaped the temporary directory.'
    }
    try {
        $logDirectory = Join-Path $tempRoot 'logs\first'
        New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
        $missingCanonical = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($missingCanonical.Valid -or
            $missingCanonical.CanonicalScriptLogCount -ne 0) {
            throw 'Campaign Debug missing canonical script-log self-test failed.'
        }

        $canonicalPath = Join-Path $logDirectory 'script.log'
        [IO.File]::WriteAllText(
            $canonicalPath,
            $forceLifecycleText,
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            (Join-Path $logDirectory 'console.log'),
            'no hard diagnostics',
            (New-Object Text.UTF8Encoding($false)))
        $errorPath = Join-Path $logDirectory 'error.log'
        $crashPath = Join-Path $logDirectory 'crash.log'
        $safeAuxiliaryText = 'no auxiliary hard diagnostics'
        [IO.File]::WriteAllText(
            $errorPath,
            $safeAuxiliaryText,
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            $crashPath,
            $safeAuxiliaryText,
            (New-Object Text.UTF8Encoding($false)))
        $singleCanonical = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if (-not $singleCanonical.Valid -or
            $singleCanonical.CanonicalScriptLogCount -ne 1 -or
            $singleCanonical.CanonicalConsoleLogCount -ne 1 -or
            $singleCanonical.CanonicalErrorLogCount -ne 1 -or
            $singleCanonical.CanonicalCrashLogCount -ne 1 -or
            -not $singleCanonical.AuxiliaryDiagnosticsValid -or
            $singleCanonical.HardDiagnosticCount -ne 0) {
            throw 'Campaign Debug canonical script-log isolation self-test failed.'
        }

        [IO.File]::WriteAllText(
            $errorPath,
            '17:00:03.000 SCRIPT (E): unique auxiliary error',
            (New-Object Text.UTF8Encoding($false)))
        $errorLogRed = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($errorLogRed.Valid -or
            $errorLogRed.ErrorLogProjectionExact -or
            $errorLogRed.AuxiliaryUnapprovedEventCount -ne 1) {
            throw 'Campaign Debug auxiliary error-log rejection self-test failed.'
        }
        [IO.File]::WriteAllText(
            $errorPath,
            $safeAuxiliaryText,
            (New-Object Text.UTF8Encoding($false)))

        [IO.File]::WriteAllText(
            $crashPath,
            'fatal error: unique auxiliary crash',
            (New-Object Text.UTF8Encoding($false)))
        $crashLogRed = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($crashLogRed.Valid -or
            $crashLogRed.CrashLogProjectionExact -or
            $crashLogRed.AuxiliaryUnapprovedEventCount -ne 1) {
            throw 'Campaign Debug auxiliary crash-log rejection self-test failed.'
        }
        [IO.File]::WriteAllText(
            $crashPath,
            $safeAuxiliaryText,
            (New-Object Text.UTF8Encoding($false)))

        [IO.File]::WriteAllText(
            (Join-Path $logDirectory 'console.log'),
            '17:00:04.000 SCRIPT (E): unique console script error',
            (New-Object Text.UTF8Encoding($false)))
        $uniqueConsoleScript = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($uniqueConsoleScript.Valid -or
            $uniqueConsoleScript.ScriptErrors -ne 1 -or
            $uniqueConsoleScript.UnapprovedHardDiagnosticCount -ne 1) {
            throw 'Campaign Debug unique console SCRIPT-diagnostic self-test failed.'
        }

        [IO.File]::WriteAllText(
            $canonicalPath,
            $identityText,
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            (Join-Path $logDirectory 'console.log'),
            $identityText,
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            $errorPath,
            $identityText.Replace("`n", "`r`n"),
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            $crashPath,
            $identityText,
            (New-Object Text.UTF8Encoding($false)))
        $mirroredIdentity = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if (-not $mirroredIdentity.Valid -or
            $mirroredIdentity.HardDiagnosticCount -ne 2 -or
            $mirroredIdentity.ApprovedStockDiagnosticCount -ne 2 -or
            -not $mirroredIdentity.ErrorLogProjectionExact -or
            -not $mirroredIdentity.CrashLogProjectionExact -or
            $mirroredIdentity.UnapprovedHardDiagnosticCount -ne 0) {
            throw 'Campaign Debug canonical mirrored-diagnostic multiset self-test failed.'
        }
        [IO.File]::WriteAllText(
            $canonicalPath,
            $forceLifecycleText,
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            $errorPath,
            $safeAuxiliaryText,
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            $crashPath,
            $safeAuxiliaryText,
            (New-Object Text.UTF8Encoding($false)))

        [IO.File]::WriteAllText(
            (Join-Path $logDirectory 'console.log'),
            '17:00:04.000 ENGINE (E): synthetic console engine error',
            (New-Object Text.UTF8Encoding($false)))
        $consoleEngine = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($consoleEngine.Valid -or
            $consoleEngine.EngineErrors -ne 1 -or
            $consoleEngine.UnapprovedHardDiagnosticCount -ne 1) {
            throw 'Campaign Debug canonical console ENGINE-diagnostic self-test failed.'
        }
        [IO.File]::WriteAllText(
            (Join-Path $logDirectory 'console.log'),
            (@(
                '17:00:04.000 ENGINE (E): duplicate console engine error',
                '17:00:04.000 ENGINE (E): duplicate console engine error'
            ) -join "`n"),
            (New-Object Text.UTF8Encoding($false)))
        $duplicateConsoleEngine = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($duplicateConsoleEngine.Valid -or
            $duplicateConsoleEngine.EngineErrors -ne 2 -or
            $duplicateConsoleEngine.UnapprovedHardDiagnosticCount -ne 2) {
            throw 'Campaign Debug duplicate same-source ENGINE-diagnostic self-test failed.'
        }
        [IO.File]::WriteAllText(
            (Join-Path $logDirectory 'console.log'),
            'no hard diagnostics',
            (New-Object Text.UTF8Encoding($false)))

        $duplicateDirectory = Join-Path $tempRoot 'logs\second'
        New-Item -ItemType Directory -Path $duplicateDirectory -Force | Out-Null
        $firstConsolePath = Join-Path $logDirectory 'console.log'
        $secondConsolePath = Join-Path $duplicateDirectory 'console.log'
        [IO.File]::Move($firstConsolePath, $secondConsolePath)
        $splitCanonical = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($splitCanonical.Valid -or
            $splitCanonical.CanonicalScriptLogCount -ne 1 -or
            $splitCanonical.CanonicalConsoleLogCount -ne 1 -or
            $splitCanonical.CanonicalLogPairSameDirectory) {
            throw 'Campaign Debug split canonical log-pair self-test failed.'
        }
        [IO.File]::Move($secondConsolePath, $firstConsolePath)
        [IO.File]::WriteAllText(
            (Join-Path $duplicateDirectory 'script.log'),
            $forceLifecycleText,
            (New-Object Text.UTF8Encoding($false)))
        [IO.File]::WriteAllText(
            (Join-Path $duplicateDirectory 'console.log'),
            'mirror',
            (New-Object Text.UTF8Encoding($false)))
        $duplicateCanonical = Get-GuardErrorCensus `
            -GuardRoot $tempRoot `
            -Profile force_authority `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven $false `
            -IntentionalMissionConvoySettlementDiagnosticProven $false `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven $false `
            -IntentionalMissionConvoyWatchdogDiagnosticProven $false
        if ($duplicateCanonical.Valid -or
            $duplicateCanonical.CanonicalScriptLogCount -ne 2 -or
            $duplicateCanonical.CanonicalConsoleLogCount -ne 2) {
            throw 'Campaign Debug duplicate canonical log self-test failed.'
        }
    }
    finally {
        if (Test-Path -LiteralPath $tempRoot -PathType Container) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction Stop
        }
    }

    return 38
}

function Get-AuxiliaryDiagnosticProjection {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][int]$ExpectedStockDiagnosticCount
    )

    $eventHeaderPattern =
        '(?im)^(?:\s*\d{2}:\d{2}:\d{2}\.\d+\s+' +
        '(?:SCRIPT|ENGINE)\s+\(E\):\s*)?' +
        'Virtual Machine Exception\s*$'
    $eventHeaders = @([regex]::Matches($Text, $eventHeaderPattern))
    $kinds = New-Object Collections.Generic.List[string]
    $unapprovedEventCount = 0
    for ($index = 0; $index -lt $eventHeaders.Count; $index++) {
        $start = $eventHeaders[$index].Index
        $end = if ($index + 1 -lt $eventHeaders.Count) {
            $eventHeaders[$index + 1].Index
        }
        else {
            $Text.Length
        }
        $eventText = $Text.Substring($start, $end - $start)
        $wrongParameter = $eventText -cmatch '(?m)^\s*Reason:\s*Wrong parameter value\s*$'
        $identityFunction = $eventText -cmatch
            "(?m)^\s*Function:\s*'GetPlayerIdentityId'\s*$"
        $editable = $eventText -cmatch
            "(?m)^\s*Class:\s*'SCR_EditableEntityCore'\s*$"
        $reconnect = $eventText -cmatch
            "(?m)^\s*Class:\s*'SCR_ReconnectComponent'\s*$"
        if ($wrongParameter -and $identityFunction -and $editable -and
            -not $reconnect) {
            [void]$kinds.Add('stock-startup-editable-identity')
        }
        elseif ($wrongParameter -and $identityFunction -and $reconnect -and
            -not $editable) {
            [void]$kinds.Add('stock-startup-reconnect-identity')
        }
        else {
            $unapprovedEventCount++
        }
    }

    $nonVmHardHeaders = @([regex]::Matches(
        $Text,
        '(?im)^[ \t]*(?:\d{2}:\d{2}:\d{2}\.\d+[ \t]+)?' +
            '(?:SCRIPT|ENGINE)[ \t]+\(E\):' +
            '(?![ \t]*Virtual Machine Exception[ \t]*\r?$)[ \t]*.+$')).Count
    $strongFatalMarkers = @([regex]::Matches(
        $Text,
        '(?im)\b(?:access violation|unhandled exception|fatal error|' +
            'assertion failed|segmentation fault|stack overflow|' +
            'out of memory|engine crash|minidump)\b')).Count
    $unapprovedEventCount += $nonVmHardHeaders + $strongFatalMarkers

    $projectionExact = $false
    if ($ExpectedStockDiagnosticCount -eq 0) {
        $projectionExact = $eventHeaders.Count -eq 0 -and
            $unapprovedEventCount -eq 0
    }
    elseif ($ExpectedStockDiagnosticCount -eq 2) {
        $projectionExact =
            $eventHeaders.Count -eq 2 -and
            $kinds.Count -eq 2 -and
            $kinds[0] -ceq 'stock-startup-editable-identity' -and
            $kinds[1] -ceq 'stock-startup-reconnect-identity' -and
            $unapprovedEventCount -eq 0
    }

    return [pscustomobject][ordered]@{
        Valid = $projectionExact
        ProjectionExact = $projectionExact
        StockDiagnosticCount = $kinds.Count
        UnapprovedEventCount = $unapprovedEventCount
        EventKinds = $kinds.ToArray()
    }
}

function Get-GuardErrorCensus {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,

        [ValidateSet("full_certification", "force_authority")]
        [Parameter(Mandatory = $true)]
        [string]$Profile,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoyAdmissionDiagnosticsProven,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoySettlementDiagnosticProven,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoyCorruptionDiagnosticsProven,

        [Parameter(Mandatory = $true)]
        [bool]$IntentionalMissionConvoyWatchdogDiagnosticProven
    )

    $logRoot = Join-Path $GuardRoot 'logs'
    $scriptLogs = @()
    $consoleLogs = @()
    $errorLogs = @()
    $crashLogs = @()
    if (Test-Path -LiteralPath $logRoot -PathType Container) {
        $scriptLogs = @(Get-ChildItem `
            -LiteralPath $logRoot `
            -Recurse `
            -File `
            -Filter 'script.log' `
            -Force `
            -ErrorAction Stop)
        $consoleLogs = @(Get-ChildItem `
            -LiteralPath $logRoot `
            -Recurse `
            -File `
            -Filter 'console.log' `
            -Force `
            -ErrorAction Stop)
        $errorLogs = @(Get-ChildItem `
            -LiteralPath $logRoot `
            -Recurse `
            -File `
            -Filter 'error.log' `
            -Force `
            -ErrorAction Stop)
        $crashLogs = @(Get-ChildItem `
            -LiteralPath $logRoot `
            -Recurse `
            -File `
            -Filter 'crash.log' `
            -Force `
            -ErrorAction Stop)
    }
    $canonicalLogPairSameDirectory = $false
    $auxiliaryLogPairSameDirectory = $false
    if ($scriptLogs.Count -eq 1 -and
        $consoleLogs.Count -eq 1 -and
        $errorLogs.Count -eq 1 -and
        $crashLogs.Count -eq 1) {
        $scriptLogDirectory = [IO.Path]::GetFullPath($scriptLogs[0].DirectoryName)
        $consoleLogDirectory = [IO.Path]::GetFullPath($consoleLogs[0].DirectoryName)
        $errorLogDirectory = [IO.Path]::GetFullPath($errorLogs[0].DirectoryName)
        $crashLogDirectory = [IO.Path]::GetFullPath($crashLogs[0].DirectoryName)
        $canonicalLogPairSameDirectory = $scriptLogDirectory.Equals(
            $consoleLogDirectory,
            [StringComparison]::OrdinalIgnoreCase)
        $auxiliaryLogPairSameDirectory =
            $scriptLogDirectory.Equals(
                $errorLogDirectory,
                [StringComparison]::OrdinalIgnoreCase) -and
            $scriptLogDirectory.Equals(
                $crashLogDirectory,
                [StringComparison]::OrdinalIgnoreCase)
    }
    if ($scriptLogs.Count -ne 1 -or
        $consoleLogs.Count -ne 1 -or
        $errorLogs.Count -ne 1 -or
        $crashLogs.Count -ne 1 -or
        -not $canonicalLogPairSameDirectory -or
        -not $auxiliaryLogPairSameDirectory) {
        return [pscustomobject][ordered]@{
            Valid = $false
            CanonicalScriptLogCount = $scriptLogs.Count
            CanonicalConsoleLogCount = $consoleLogs.Count
            CanonicalErrorLogCount = $errorLogs.Count
            CanonicalCrashLogCount = $crashLogs.Count
            CanonicalLogPairSameDirectory = $canonicalLogPairSameDirectory
            AuxiliaryLogPairSameDirectory = $auxiliaryLogPairSameDirectory
            AuxiliaryDiagnosticsValid = $false
            ErrorLogProjectionExact = $false
            CrashLogProjectionExact = $false
            AuxiliaryUnapprovedEventCount = 0
            HardDiagnosticFree = $false
            HardDiagnosticCount = 0
            ScriptErrors = 0
            EngineErrors = 0
            PartisanErrors = 0
            CrashMarkers = 0
            PartisanSeverityLineCount = 0
            ApprovedStockDiagnosticCount = 0
            ApprovedIntentionalDiagnosticCount = 0
            ChannelArithmeticValid = $false
            CategoryArithmeticValid = $false
            MalformedHardDiagnosticCount = 0
            UnapprovedHardDiagnosticCount = 0
            UnapprovedHardDiagnosticKinds = @()
            LifecycleMarkersValid = $false
            IdentityBaselinePairValid = $false
            IntentionalFixtureStructureExact = $false
            IntentionalFixtureSetValid = $false
            IntentionalMissionConvoyAdmissionDiagnosticsProven =
                $IntentionalMissionConvoyAdmissionDiagnosticsProven
            IntentionalMissionConvoySettlementDiagnosticProven =
                $IntentionalMissionConvoySettlementDiagnosticProven
            IntentionalMissionConvoyCorruptionDiagnosticsProven =
                $IntentionalMissionConvoyCorruptionDiagnosticsProven
            IntentionalMissionConvoyWatchdogDiagnosticProven =
                $IntentionalMissionConvoyWatchdogDiagnosticProven
        }
    }
    if (($scriptLogs[0].Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The canonical Campaign Debug script log must not be a reparse point.'
    }
    if (($consoleLogs[0].Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The canonical Campaign Debug console log must not be a reparse point.'
    }
    if (($errorLogs[0].Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The canonical Campaign Debug error log must not be a reparse point.'
    }
    if (($crashLogs[0].Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The canonical Campaign Debug crash log must not be a reparse point.'
    }
    $census = Get-CampaignDebugHardDiagnosticCensus `
        -ScriptText (Read-SharedFileText -Path $scriptLogs[0].FullName) `
        -ConsoleText (Read-SharedFileText -Path $consoleLogs[0].FullName) `
        -Profile $Profile `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven `
            $IntentionalMissionConvoyAdmissionDiagnosticsProven `
        -IntentionalMissionConvoySettlementDiagnosticProven `
            $IntentionalMissionConvoySettlementDiagnosticProven `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven `
            $IntentionalMissionConvoyCorruptionDiagnosticsProven `
        -IntentionalMissionConvoyWatchdogDiagnosticProven `
            $IntentionalMissionConvoyWatchdogDiagnosticProven
    $census | Add-Member `
        -NotePropertyName CanonicalScriptLogCount `
        -NotePropertyValue 1
    $census | Add-Member `
        -NotePropertyName CanonicalConsoleLogCount `
        -NotePropertyValue 1
    $census | Add-Member `
        -NotePropertyName CanonicalErrorLogCount `
        -NotePropertyValue 1
    $census | Add-Member `
        -NotePropertyName CanonicalCrashLogCount `
        -NotePropertyValue 1
    $census | Add-Member `
        -NotePropertyName CanonicalLogPairSameDirectory `
        -NotePropertyValue $true
    $census | Add-Member `
        -NotePropertyName AuxiliaryLogPairSameDirectory `
        -NotePropertyValue $true
    $errorProjection = Get-AuxiliaryDiagnosticProjection `
        -Text (Read-SharedFileText -Path $errorLogs[0].FullName) `
        -ExpectedStockDiagnosticCount $census.ApprovedStockDiagnosticCount
    $crashProjection = Get-AuxiliaryDiagnosticProjection `
        -Text (Read-SharedFileText -Path $crashLogs[0].FullName) `
        -ExpectedStockDiagnosticCount $census.ApprovedStockDiagnosticCount
    $auxiliaryUnapprovedEventCount =
        [int]$errorProjection.UnapprovedEventCount +
        [int]$crashProjection.UnapprovedEventCount
    $auxiliaryDiagnosticsValid =
        $errorProjection.Valid -and
        $crashProjection.Valid -and
        $auxiliaryUnapprovedEventCount -eq 0
    $census | Add-Member `
        -NotePropertyName AuxiliaryDiagnosticsValid `
        -NotePropertyValue $auxiliaryDiagnosticsValid
    $census | Add-Member `
        -NotePropertyName ErrorLogProjectionExact `
        -NotePropertyValue ([bool]$errorProjection.ProjectionExact)
    $census | Add-Member `
        -NotePropertyName CrashLogProjectionExact `
        -NotePropertyValue ([bool]$crashProjection.ProjectionExact)
    $census | Add-Member `
        -NotePropertyName AuxiliaryUnapprovedEventCount `
        -NotePropertyValue $auxiliaryUnapprovedEventCount
    $census.Valid = [bool]$census.Valid -and $auxiliaryDiagnosticsValid
    return $census
}

function Test-PathOverlap {
    param(
        [Parameter(Mandatory = $true)][string]$First,
        [Parameter(Mandatory = $true)][string]$Second
    )

    return (Test-ContainedPath -Root $First -Candidate $Second -AllowEqual) -or
        (Test-ContainedPath -Root $Second -Candidate $First -AllowEqual)
}

function Write-PortableJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $text = (($Value | ConvertTo-Json -Depth 20).Replace("`r`n", "`n") + "`n")
    [IO.File]::WriteAllText(
        $Path,
        $text,
        (New-Object Text.UTF8Encoding($false)))
}

function Get-ImmutableHarnessBlobSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$Revision,
        [Parameter(Mandatory = $true)][string]$FilePath
    )

    $checkout = [IO.Path]::GetFullPath($CheckoutRoot).TrimEnd('\', '/')
    $prefix = $checkout + [IO.Path]::DirectorySeparatorChar
    $fullPath = [IO.Path]::GetFullPath($FilePath)
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A harness tool path escaped the clean checkout.'
    }
    $relativePath = $fullPath.Substring($prefix.Length).Replace('\', '/')
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = 'git'
    $escapedRoot = $checkout.Replace('"', '\"')
    $escapedSpec = ("{0}:{1}" -f $Revision, $relativePath).Replace('"', '\"')
    $startInfo.Arguments = "-C `"$escapedRoot`" cat-file blob `"$escapedSpec`""
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw 'The immutable harness blob reader could not start.'
        }
        $memory = New-Object IO.MemoryStream
        try {
            $process.StandardOutput.BaseStream.CopyTo($memory)
            $errorText = $process.StandardError.ReadToEnd()
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) {
                throw "Harness tool $relativePath is not an immutable blob at $Revision`: $errorText"
            }
            $sha = [Security.Cryptography.SHA256]::Create()
            try {
                return ([BitConverter]::ToString(
                    $sha.ComputeHash($memory.ToArray()))).Replace('-', '').ToLowerInvariant()
            }
            finally {
                $sha.Dispose()
            }
        }
        finally {
            $memory.Dispose()
        }
    }
    finally {
        $process.Dispose()
    }
}

function Get-HarnessBinding {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$CandidateModulePath,
        [Parameter(Mandatory = $true)][string]$ReleaseIndexProducerPath,
        [Parameter(Mandatory = $true)][string]$ReleaseDocsConsumerPath
    )

    $headLines = @(& git -C $CheckoutRoot rev-parse HEAD)
    $headExit = $LASTEXITCODE
    $statusLines = @(& git -C $CheckoutRoot status --porcelain=v1 --untracked-files=all)
    $statusExit = $LASTEXITCODE
    $head = ($headLines -join '').Trim()
    if ($headExit -ne 0 -or $statusExit -ne 0 -or
        $head -cnotmatch '^[0-9a-f]{40}$') {
        throw 'The Campaign Debug harness Git identity could not be resolved.'
    }

    $runnerSha = (Get-FileHash -LiteralPath $RunnerPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $candidateModuleSha = (Get-FileHash `
        -LiteralPath $CandidateModulePath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $producerSha = (Get-FileHash `
        -LiteralPath $ReleaseIndexProducerPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $consumerSha = (Get-FileHash `
        -LiteralPath $ReleaseDocsConsumerPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    $runnerBlobSha = $runnerSha
    $candidateModuleBlobSha = $candidateModuleSha
    $producerBlobSha = $producerSha
    $consumerBlobSha = $consumerSha
    if ($statusLines.Count -eq 0) {
        $runnerBlobSha = Get-ImmutableHarnessBlobSha256 `
            -CheckoutRoot $CheckoutRoot -Revision $head -FilePath $RunnerPath
        $candidateModuleBlobSha = Get-ImmutableHarnessBlobSha256 `
            -CheckoutRoot $CheckoutRoot -Revision $head -FilePath $CandidateModulePath
        $producerBlobSha = Get-ImmutableHarnessBlobSha256 `
            -CheckoutRoot $CheckoutRoot -Revision $head -FilePath $ReleaseIndexProducerPath
        $consumerBlobSha = Get-ImmutableHarnessBlobSha256 `
            -CheckoutRoot $CheckoutRoot -Revision $head -FilePath $ReleaseDocsConsumerPath
    }

    return [pscustomobject][ordered]@{
        GitHead = $head
        StatusText = $statusLines -join "`n"
        Clean = $statusLines.Count -eq 0
        RunnerSha256 = $runnerSha
        RunnerGitBlobSha256 = $runnerBlobSha
        CandidateModuleSha256 = $candidateModuleSha
        CandidateModuleGitBlobSha256 = $candidateModuleBlobSha
        ReleaseIndexProducerSha256 = $producerSha
        ReleaseIndexProducerGitBlobSha256 = $producerBlobSha
        ReleaseDocsConsumerSha256 = $consumerSha
        ReleaseDocsConsumerGitBlobSha256 = $consumerBlobSha
    }
}

function Assert-HarnessBinding {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$CandidateModulePath,
        [Parameter(Mandatory = $true)][string]$ReleaseIndexProducerPath,
        [Parameter(Mandatory = $true)][string]$ReleaseDocsConsumerPath
    )

    $actual = Get-HarnessBinding `
        -CheckoutRoot $CheckoutRoot `
        -RunnerPath $RunnerPath `
        -CandidateModulePath $CandidateModulePath `
        -ReleaseIndexProducerPath $ReleaseIndexProducerPath `
        -ReleaseDocsConsumerPath $ReleaseDocsConsumerPath
    if ($actual.GitHead -cne $Expected.GitHead -or
        $actual.StatusText -cne $Expected.StatusText -or
        $actual.Clean -ne $Expected.Clean -or
        $actual.RunnerSha256 -cne $Expected.RunnerSha256 -or
        $actual.RunnerGitBlobSha256 -cne $Expected.RunnerGitBlobSha256 -or
        $actual.CandidateModuleSha256 -cne $Expected.CandidateModuleSha256 -or
        $actual.CandidateModuleGitBlobSha256 -cne
            $Expected.CandidateModuleGitBlobSha256 -or
        $actual.ReleaseIndexProducerSha256 -cne
            $Expected.ReleaseIndexProducerSha256 -or
        $actual.ReleaseIndexProducerGitBlobSha256 -cne
            $Expected.ReleaseIndexProducerGitBlobSha256 -or
        $actual.ReleaseDocsConsumerSha256 -cne
            $Expected.ReleaseDocsConsumerSha256 -or
        $actual.ReleaseDocsConsumerGitBlobSha256 -cne
            $Expected.ReleaseDocsConsumerGitBlobSha256) {
        throw 'The Campaign Debug harness identity changed during execution.'
    }
    return $actual
}

function Get-CandidateMountAttestation {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$PackedProjectPath,
        [Parameter(Mandatory = $true)][string]$AddonGuid
    )

    $expectedProject = [IO.Path]::GetFullPath($PackedProjectPath).Replace('\', '/')
    $pattern = "(?im)^\s*\d{2}:\d{2}:\d{2}\.\d{3}\s+ENGINE\s+:\s+" +
        "gproj:\s+'(?<path>[^']+)'\s+guid:\s+'" +
        [regex]::Escape($AddonGuid) + "'\s*(?<mode>\([^)]+\))?\s*$"
    $recordCount = 0
    $exactPathCount = 0
    $packedCount = 0
    $invalidModeCount = 0
    foreach ($consoleFile in @(Get-ChildItem `
            -LiteralPath $GuardRoot `
            -Recurse `
            -File `
            -Filter 'console.log' `
            -Force `
            -ErrorAction SilentlyContinue)) {
        if (($consoleFile.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'A candidate mount-attestation log must not be a reparse point.'
        }
        $consoleText = Read-SharedFileText -Path $consoleFile.FullName
        foreach ($match in @([regex]::Matches($consoleText, $pattern))) {
            $recordCount++
            $recordedProject = $match.Groups['path'].Value.Replace('\', '/')
            if ($recordedProject.Equals(
                    $expectedProject,
                    [StringComparison]::OrdinalIgnoreCase)) {
                $exactPathCount++
            }
            if ($match.Groups['mode'].Value -ceq '(packed)') {
                $packedCount++
            }
            elseif (-not [string]::IsNullOrEmpty($match.Groups['mode'].Value)) {
                $invalidModeCount++
            }
        }
    }

    return [pscustomobject][ordered]@{
        Valid = $recordCount -gt 0 -and
            $exactPathCount -eq $recordCount -and
            $packedCount -gt 0 -and
            $invalidModeCount -eq 0
        RecordCount = $recordCount
        ExactPathCount = $exactPathCount
        PackedCount = $packedCount
        InvalidModeCount = $invalidModeCount
        GuidExact = $recordCount -gt 0
        Packed = $packedCount -gt 0 -and $invalidModeCount -eq 0
    }
}

function Copy-CampaignDebugEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$EvidenceRunRoot,
        [Parameter(Mandatory = $true)][string]$SettingsPath,
        [Parameter(Mandatory = $true)][string]$ExpectedSettingsSha256,
        [Parameter(Mandatory = $true)]$Candidate
    )

    Assert-NoReparsePathAncestry -Path $GuardRoot
    Assert-NoReparsePathAncestry -Path $EvidenceRunRoot
    $rawRoot = Join-Path $EvidenceRunRoot 'raw'
    $identityRoot = Join-Path $EvidenceRunRoot 'identity'
    $configRoot = Join-Path $EvidenceRunRoot 'config'
    New-Item `
        -ItemType Directory `
        -Path $rawRoot, $identityRoot, $configRoot `
        -Force | Out-Null

    $copyRoots = @(
        [pscustomobject]@{ Source = Join-Path $GuardRoot 'logs'; Destination = Join-Path $rawRoot 'logs' },
        [pscustomobject]@{ Source = Join-Path $GuardRoot 'profile\Partisan\debug'; Destination = Join-Path $rawRoot 'campaign-debug' }
    )
    foreach ($copyRoot in $copyRoots) {
        if (-not (Test-Path -LiteralPath $copyRoot.Source -PathType Container)) {
            continue
        }
        foreach ($item in @(Get-ChildItem `
                -LiteralPath $copyRoot.Source `
                -Recurse `
                -Force `
                -ErrorAction Stop)) {
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw 'Guarded runtime evidence must not contain a reparse point.'
            }
        }
        New-Item -ItemType Directory -Path $copyRoot.Destination -Force | Out-Null
        foreach ($item in @(Get-ChildItem `
                -LiteralPath $copyRoot.Source `
                -Force `
                -ErrorAction Stop)) {
            Copy-Item `
                -LiteralPath $item.FullName `
                -Destination $copyRoot.Destination `
                -Recurse `
                -Force `
                -ErrorAction Stop
        }
    }
    $retainedSettingsPath = Join-Path $configRoot 'HST_Settings.json'
    Copy-Item `
        -LiteralPath $SettingsPath `
        -Destination $retainedSettingsPath `
        -Force `
        -ErrorAction Stop
    if ((Get-FileHash `
            -LiteralPath $retainedSettingsPath `
            -Algorithm SHA256).Hash.ToLowerInvariant() -cne $ExpectedSettingsSha256) {
        throw 'The retained Campaign Debug settings differ from the guarded runtime copy.'
    }
    Copy-Item `
        -LiteralPath $Candidate.TrackedManifestPath `
        -Destination (Join-Path $identityRoot 'candidate.json') `
        -Force `
        -ErrorAction Stop
    Copy-Item `
        -LiteralPath $Candidate.TrackedReadyPath `
        -Destination (Join-Path $identityRoot 'candidate.ready.json') `
        -Force `
        -ErrorAction Stop
}

function Get-EvidenceFileRows {
    param([Parameter(Mandatory = $true)][string]$EvidenceRunRoot)

    $root = [IO.Path]::GetFullPath($EvidenceRunRoot).TrimEnd('\', '/')
    $prefix = $root + [IO.Path]::DirectorySeparatorChar
    $rows = New-Object Collections.Generic.List[object]
    foreach ($file in @(Get-ChildItem `
            -LiteralPath $root `
            -Recurse `
            -File `
            -Force `
            -ErrorAction Stop | Where-Object { $_.Name -cne 'run.json' } | Sort-Object FullName)) {
        $full = [IO.Path]::GetFullPath($file.FullName)
        if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A retained evidence file escaped its run root.'
        }
        [void]$rows.Add([pscustomobject][ordered]@{
            path = $full.Substring($prefix.Length).Replace('\', '/')
            length = [long]$file.Length
            sha256 = (Get-FileHash `
                -LiteralPath $full `
                -Algorithm SHA256).Hash.ToLowerInvariant()
        })
    }
    return $rows.ToArray()
}

$hardDiagnosticClassifierChecks = Test-CampaignDebugHardDiagnosticCensus
$candidateModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
$releaseIndexProducerPath = Join-Path `
    $PSScriptRoot `
    'New-PartisanCampaignDebugReleaseIndex.ps1'
$releaseDocsConsumerPath = Join-Path $PSScriptRoot 'update-release-docs.ps1'
Import-Module -Name $candidateModulePath -Force -ErrorAction Stop
$executablePath = Resolve-ExistingPath -Path $Executable -Kind Leaf
$consumerIntent = if ($PreflightOnly -or $ArtifactValidatorSelfTest) {
    'verification'
}
else {
    'runtime'
}
$runtimeRole = if ((Split-Path -Leaf $executablePath) -ceq 'ArmaReforgerSteamDiag.exe') {
    'client'
}
elseif ((Split-Path -Leaf $executablePath) -ceq 'ArmaReforgerServerDiag.exe') {
    'server'
}
else {
    throw 'Executable must be a supported Reforger diagnostic runtime.'
}
$candidateBinding = Assert-PartisanReleaseCandidate `
    -ManifestPath $CandidateManifest `
    -BundleRoot $CandidateBundleRoot `
    -RuntimeAddonRoot $RuntimeAddonRoot `
    -Executable $executablePath `
    -RuntimeRole $runtimeRole `
    -ConsumerIntent $consumerIntent
$candidateIdentity = Get-PartisanPublicCandidateIdentity -Candidate $candidateBinding
$runtimeAddonPath = $candidateBinding.RuntimeAddonRootPath
$addonSearchPath = $candidateBinding.AddonSearchPath
$projectFile = $candidateBinding.PackedProjectPath
$resolvedAddonRoots = @(
    $runtimeAddonPath,
    $candidateBinding.PackageParentPath,
    $candidateBinding.PackedAddonPath)
$ExpectedBuildSha = $candidateBinding.EmbeddedBuildSha
$ExpectedBuildUtc = $candidateBinding.EmbeddedBuildUtc
$ExpectedBuildLabel = $candidateBinding.EmbeddedBuildLabel
$settingsPath = Resolve-ExistingPath -Path $SettingsSource -Kind Leaf
Assert-NoReparsePathAncestry -Path $settingsPath
$settingsSourceSha256 = (Get-FileHash `
    -LiteralPath $settingsPath `
    -Algorithm SHA256).Hash.ToLowerInvariant()

try {
    $settings = Read-SharedFileText -Path $settingsPath | ConvertFrom-Json
}
catch {
    throw "Runtime settings file could not be read and parsed."
}
if ([int]$settings.schemaVersion -ne $candidateBinding.RuntimeSettingsSchema) {
    throw 'Runtime settings schema does not match the sealed candidate identity.'
}
if (-not $settings.membership.membershipEnabled) {
    throw "Membership enforcement must be enabled."
}
if (@($settings.membership.adminIdentityIds).Count -ne 1) {
    throw "Exactly one trusted admin identity is required."
}
if (-not $settings.debug.debugMenuEnabled -or -not $settings.debug.debugLoggingEnabled) {
    throw "Campaign debug menu and logging must be enabled."
}

$normalizedWatchedRoots = @($WatchedRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$normalizedSpillRoots = @($SpillRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$isRealRun = -not $PreflightOnly -and -not $ArtifactValidatorSelfTest
if ($isRealRun -and
    ($normalizedWatchedRoots.Count -eq 0 -or $normalizedSpillRoots.Count -eq 0)) {
    throw "A real guarded run requires explicit nonempty watched and spill roots."
}

$checkoutRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$harnessBinding = Get-HarnessBinding `
    -CheckoutRoot $checkoutRoot `
    -RunnerPath $PSCommandPath `
    -CandidateModulePath $candidateModulePath `
    -ReleaseIndexProducerPath $releaseIndexProducerPath `
    -ReleaseDocsConsumerPath $releaseDocsConsumerPath
$evidenceOutputPath = $null
if ($isRealRun) {
    if (-not $harnessBinding.Clean) {
        throw 'A real candidate run requires a clean, committed harness checkout.'
    }
    if ([string]::IsNullOrWhiteSpace($EvidenceOutputRoot)) {
        throw 'A real candidate run requires an external evidence output root.'
    }
    $evidenceOutputPath = [IO.Path]::GetFullPath($EvidenceOutputRoot)
    foreach ($protectedRoot in @(
            $checkoutRoot,
            $candidateBinding.BundleRootPath,
            $candidateBinding.RuntimeAddonRootPath)) {
        if (Test-PathOverlap -First $evidenceOutputPath -Second $protectedRoot) {
            throw 'The evidence output root must not overlap the checkout, candidate, or runtime roots.'
        }
    }
    foreach ($watchedRoot in $normalizedWatchedRoots) {
        if (Test-PathOverlap -First $evidenceOutputPath -Second $watchedRoot) {
            throw 'The retained evidence root must not overlap a watched default root.'
        }
    }
    foreach ($spillRoot in $normalizedSpillRoots) {
        if (Test-ContainedPath `
                -Root $evidenceOutputPath `
                -Candidate $spillRoot `
                -AllowEqual) {
            throw 'The retained evidence root must not contain a spill-monitoring root.'
        }
    }
    Assert-NoReparsePathAncestry -Path $evidenceOutputPath
    New-Item -ItemType Directory -Path $evidenceOutputPath -Force | Out-Null
    Assert-NoReparsePathAncestry -Path $evidenceOutputPath
}

$watchSnapshots = New-Object Collections.Generic.List[object]
$spillSnapshots = New-Object Collections.Generic.List[object]

$nonce = [Guid]::NewGuid().ToString("N")
$tempBase = [IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetTempPath()) "PartisanCampaignDebug"))
$guardLeaf = "PartisanCampaignDebugGuard_$nonce"
$guardRoot = [IO.Path]::GetFullPath((Join-Path $tempBase $guardLeaf))
Assert-NoReparsePathAncestry -Path $tempBase
$expectedGuardPrefix = $tempBase.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
if (-not $guardRoot.StartsWith($expectedGuardPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Guard path containment failed."
}

$sentinelPath = Join-Path $guardRoot ".partisan-campaign-debug-owner"

$process = $null
$job = $null
$suspendedLauncher = $null
$ownedProcesses = @{}
$rootProcessId = 0
$rootStartUtc = [DateTime]::MinValue
$stopwatch = $null
$armed = $false
$started = $false
$completed = $false
$artifactValidation = $null
$safeArtifactValidation = $null
$artifactSignature = $null
$artifactSignaturePolls = 0
$artifactPaths = $null
$errorCensus = $null
$runError = $null
$cleanupResult = $null
$cleanupPhaseErrors = New-Object Collections.Generic.List[string]
$candidateStage = $null
$candidateBoundaryVerified = $false
$guardedSettingsPath = $null
$guardedSettingsSha256 = $null
$mountAttestation = $null
$evidenceRunRoot = $null
$evidenceState = [pscustomobject]@{ Captured = $false }
$evidenceStartUtc = [DateTime]::UtcNow
$mutex = $null
$mutexAcquired = $false
$guardOwnership = $null
$wrapperStartUtc = [DateTime]::MinValue
$existingProcesses = @()
$unclaimedEngineProcessesObserved = New-Object Collections.Generic.HashSet[string]

try {
    $mutex = New-Object Threading.Mutex($false, "Local\PartisanCampaignDebugGuard")
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw "Another guarded Campaign Debug wrapper is already active."
    }

    $existingProcesses = @(Get-EngineProcessRows)
    if ($existingProcesses.Count -ne 0) {
        throw "Refusing guarded launch while an engine, Workbench, server, or crash-report process is already running."
    }

    if ($isRealRun) {
        $candidateEvidenceRoot = Join-Path `
            $evidenceOutputPath `
            $candidateBinding.CandidateId
        $profileEvidenceRoot = Join-Path $candidateEvidenceRoot 'campaign-debug'
        New-Item `
            -ItemType Directory `
            -Path $candidateEvidenceRoot, $profileEvidenceRoot `
            -Force | Out-Null
        $evidenceLeaf = ([DateTime]::UtcNow.ToString(
            'yyyyMMddTHHmmssZ',
            [Globalization.CultureInfo]::InvariantCulture)) + '-' + $nonce
        $evidenceRunRoot = [IO.Path]::GetFullPath(
            (Join-Path $profileEvidenceRoot $evidenceLeaf))
        if (-not (Test-ContainedPath `
                -Root $profileEvidenceRoot `
                -Candidate $evidenceRunRoot) -or
            (Test-Path -LiteralPath $evidenceRunRoot)) {
            throw 'The retained Campaign Debug evidence run must be fresh and contained.'
        }
        New-Item -ItemType Directory -Path $evidenceRunRoot | Out-Null
        Assert-NoReparsePathAncestry -Path $evidenceRunRoot
    }

    foreach ($root in $normalizedWatchedRoots) {
        $watchSnapshots.Add((New-RootSnapshot -Root $root))
    }
    $spillExclusions = New-Object Collections.Generic.List[string]
    $spillExclusions.Add((Split-Path -Parent $projectFile))
    if ($evidenceOutputPath -and -not $spillExclusions.Contains($evidenceOutputPath)) {
        $spillExclusions.Add($evidenceOutputPath)
    }
    foreach ($snapshot in $watchSnapshots) {
        if (-not $spillExclusions.Contains($snapshot.Root)) {
            $spillExclusions.Add($snapshot.Root)
        }
    }
    foreach ($root in $normalizedSpillRoots) {
        $spillSnapshots.Add((New-RootSnapshot `
            -Root $root `
            -ExcludedRoots $spillExclusions.ToArray()))
    }

    [void](Remove-StaleOwnedGuards -GuardBase $tempBase)
    New-Item -ItemType Directory -Path $guardRoot -Force | Out-Null
    Assert-NoReparsePathAncestry -Path $guardRoot
    $guardItem = Get-Item -LiteralPath $guardRoot -Force
    if (($guardItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Guard root must not be a reparse point."
    }
    $wrapperProcess = Get-Process -Id $PID -ErrorAction Stop
    $wrapperStartUtc = $wrapperProcess.StartTime.ToUniversalTime()
    $ownershipRecord = [ordered]@{
        version = 1
        nonce = $nonce
        guardLeaf = $guardLeaf
        ownerPid = $PID
        ownerStartUtc = $wrapperStartUtc.ToString("o", [Globalization.CultureInfo]::InvariantCulture)
        createdUtc = [DateTime]::UtcNow.ToString("o", [Globalization.CultureInfo]::InvariantCulture)
    }
    [IO.File]::WriteAllText(
        $sentinelPath,
        ($ownershipRecord | ConvertTo-Json -Compress),
        (New-Object Text.UTF8Encoding($false)))
    $guardOwnership = Read-GuardOwnership -Directory $guardRoot -GuardBase $tempBase
    if (-not $guardOwnership -or $guardOwnership.Nonce -cne $nonce) {
        throw "Guard ownership sentinel validation failed."
    }

    $candidateStage = New-PartisanReleaseCandidateStage `
        -Candidate $candidateBinding `
        -GuardRoot $guardRoot
    $addonSearchPath = $candidateStage.AddonSearchPath
    $projectFile = $candidateStage.PackedProjectPath
    $resolvedAddonRoots = @(
        $runtimeAddonPath,
        $candidateBinding.PackageParentPath,
        $candidateBinding.PackedAddonPath,
        $candidateStage.StageRootPath,
        $candidateStage.PackedAddonPath)

    $profileDirectory = Join-Path $guardRoot "profile\Partisan"
    New-Item -ItemType Directory -Path $profileDirectory -Force | Out-Null
    $guardedSettingsPath = Join-Path $profileDirectory 'HST_Settings.json'
    Copy-Item `
        -LiteralPath $settingsPath `
        -Destination $guardedSettingsPath `
        -Force `
        -ErrorAction Stop
    $guardedSettingsSha256 = (Get-FileHash `
        -LiteralPath $guardedSettingsPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($guardedSettingsSha256 -cne $settingsSourceSha256) {
        throw 'The guarded Campaign Debug settings copy differs from its validated source.'
    }
    $guardedWorkingDirectory = Join-Path $guardRoot "working"
    New-Item -ItemType Directory -Path $guardedWorkingDirectory -Force | Out-Null
    $guardedTempDirectory = Join-Path $guardRoot "temp"
    New-Item -ItemType Directory -Path $guardedTempDirectory -Force | Out-Null
    $addonTempDirectory = Join-Path $guardRoot 'addon-temp'
    New-Item -ItemType Directory -Path $addonTempDirectory -Force | Out-Null

    $arguments = @(
        "-addonsDir", $addonSearchPath,
        "-addons", $candidateBinding.AddonGuid,
        "-addonTempDir", $addonTempDirectory,
        "-gproj", $projectFile,
        "-world", $WorldResource,
        "-window",
        "-noFocus",
        "-posX", "20",
        "-posY", "20",
        "-screenWidth", "1280",
        "-screenHeight", "720",
        "-forceupdate",
        "-rpl-timeout-disable",
        "-noThrow",
        "-profile", $guardRoot,
        "-hstCampaignDebugProfile", $Profile,
        "-hstReleaseCandidateId", $candidateBinding.CandidateId,
        "-hstReleasePackageSha256", $candidateBinding.PackageSha256,
        "-hstReleaseManifestSha256", $candidateBinding.ManifestSha256
    )
    $commandLine = ($arguments | ForEach-Object { ConvertTo-NativeArgument ([string]$_) }) -join " "
    $roundTripCommandLine = (ConvertTo-NativeArgument $executablePath) + " " + $commandLine
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $roundTripCommandLine `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $arguments)) {
        throw "Native argument construction did not round-trip exactly."
    }

    if ($ArtifactValidatorSelfTest) {
        $selfTestDirectory = Join-Path $guardRoot "profile\Partisan\debug"
        $selfTestResult = Invoke-ArtifactValidatorSelfTest `
            -Directory $selfTestDirectory `
            -ExpectedSha $ExpectedBuildSha `
            -ExpectedUtc $ExpectedBuildUtc `
            -ExpectedLabel $ExpectedBuildLabel
        $selfTestResult.Phase24[0].Actual = "proof@example.invalid 12345678901234567 " + (Join-Path (Split-Path -Parent $projectFile) "private.txt")
        $safeSelfTestResult = Get-SafeArtifactValidationSummary `
            -Validation $selfTestResult `
            -ExpectedBuildSha $ExpectedBuildSha `
            -ExpectedBuildUtc $ExpectedBuildUtc `
            -ExpectedBuildLabel $ExpectedBuildLabel `
            -GuardRoot $guardRoot `
            -ProjectDirectory (Split-Path -Parent $projectFile) `
            -ResolvedAddonRoots $resolvedAddonRoots
        $safeSelfTestActual = [string]$safeSelfTestResult.Phase24[0].Actual
        if ($safeSelfTestActual -notmatch '<email>' -or
            $safeSelfTestActual -notmatch '<id>' -or
            $safeSelfTestActual -notmatch '<project>') {
            throw "Synthetic artifact-output redaction self-test failed."
        }
        Write-Output ("SELFTEST " + ([pscustomobject]@{
            Valid = $selfTestResult.Valid
            Problems = $selfTestResult.Problems
            CaseCount = $selfTestResult.CaseCount
            Phase17Count = @($selfTestResult.Phase17).Count
            Phase24Count = @($selfTestResult.Phase24).Count
            StagedCleanupCount = @($selfTestResult.StagedCleanup).Count
            NegativeStagedContractChecks = @($selfTestResult.NegativeStagedContractChecks).Count
            FocusedValidatorChecks = @($selfTestResult.FocusedValidatorChecks).Count
            StateDiffRows = $selfTestResult.StateDiffRows
        } | ConvertTo-Json -Compress))
        throw (New-Object OperationCanceledException("guarded-preflight-complete"))
    }

    if ($PreflightOnly) {
        Write-Output ("PREFLIGHT " + ([pscustomobject]@{
            Profile = $Profile
            ProofScope = if ($Profile -eq "full_certification") { "full_certification" } else { "focused_force_authority" }
            FullCertification = $Profile -eq "full_certification"
            Candidate = $candidateIdentity
            EngineProcessesBefore = $existingProcesses.Count
            SettingsSchema = [int]$settings.schemaVersion
            TrustedAdminCount = @($settings.membership.adminIdentityIds).Count
            GuardCreated = $true
            CampaignSaveCopied = $false
            KillOnWrapperClose = $true
            ArgumentTokenCount = $arguments.Count
            HardDiagnosticClassifierChecks = $hardDiagnosticClassifierChecks
        } | ConvertTo-Json -Compress))
        throw (New-Object OperationCanceledException("guarded-preflight-complete"))
    }

    $job = New-Object PartisanGuardedJob
    $prelaunchProcesses = @(Get-EngineProcessRows)
    if ($prelaunchProcesses.Count -ne 0) {
        throw "An engine process appeared during guarded preflight; launch was cancelled."
    }
    $previousTemp = [Environment]::GetEnvironmentVariable(
        "TEMP", [EnvironmentVariableTarget]::Process)
    $previousTmp = [Environment]::GetEnvironmentVariable(
        "TMP", [EnvironmentVariableTarget]::Process)
    try {
        [Environment]::SetEnvironmentVariable(
            "TEMP", $guardedTempDirectory, [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            "TMP", $guardedTempDirectory, [EnvironmentVariableTarget]::Process)
        $suspendedLauncher = New-Object PartisanGuardedSuspendedProcess(
            $executablePath,
            $roundTripCommandLine,
            $guardedWorkingDirectory)
    }
    finally {
        [Environment]::SetEnvironmentVariable(
            "TEMP", $previousTemp, [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            "TMP", $previousTmp, [EnvironmentVariableTarget]::Process)
    }
    $process = $suspendedLauncher.Child
    if (-not $process) {
        throw "Diagnostic runtime process did not start."
    }
    $rootProcessId = $process.Id
    $job.Add($process)
    $rootStartUtc = $process.StartTime.ToUniversalTime()
    $ownedProcesses[$rootProcessId] = $rootStartUtc
    $suspendedLauncher.Resume()
    $suspendedLauncher.Dispose()
    $suspendedLauncher = $null

    Start-Sleep -Milliseconds 500
    $processRow = Get-CimInstance Win32_Process -Filter "ProcessId=$($process.Id)" -ErrorAction Stop
    $privateCommandLine = [string]$processRow.CommandLine
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $privateCommandLine `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $arguments)) {
        throw "Launched process command-line token verification failed."
    }
    $stopwatch = [Diagnostics.Stopwatch]::StartNew()

    Write-Output ("PREFLIGHT " + ([pscustomobject]@{
        Profile = $Profile
        ProofScope = if ($Profile -eq "full_certification") { "full_certification" } else { "focused_force_authority" }
        FullCertification = $Profile -eq "full_certification"
        Candidate = $candidateIdentity
        EngineProcessesBefore = $existingProcesses.Count
        SettingsSchema = [int]$settings.schemaVersion
        TrustedAdminCount = @($settings.membership.adminIdentityIds).Count
        GuardCreated = $true
        CampaignSaveCopied = $false
        KillOnWrapperClose = $true
        ArgumentTokensVerified = $true
        HardDiagnosticClassifierChecks = $hardDiagnosticClassifierChecks
    } | ConvertTo-Json -Compress))

    $nextHeartbeat = 0
    while ($stopwatch.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
        Start-Sleep -Seconds $PollSeconds
        Update-OwnedProcesses `
            -Owned $ownedProcesses `
            -RootProcessId $rootProcessId `
            -RootStartUtc $rootStartUtc `
            -Job $job
        foreach ($engineProcess in @(Get-EngineProcessRows)) {
            if ($ownedProcesses.ContainsKey([int]$engineProcess.Id)) {
                continue
            }
            try {
                [void]$unclaimedEngineProcessesObserved.Add(
                    "$($engineProcess.ProcessName):$($engineProcess.StartTime.ToUniversalTime().Ticks)")
            }
            catch {
                continue
            }
        }
        if ($unclaimedEngineProcessesObserved.Count -gt 0) {
            throw "An unowned engine process appeared during the guarded run; it was left untouched and the run was cancelled."
        }
        $process.Refresh()
        if ($process.HasExited) {
            $launchEvidence = Get-GuardLaunchFailureEvidence `
                -GuardRoot $guardRoot `
                -ProjectDirectory (Split-Path -Parent $projectFile) `
                -ResolvedAddonRoots $resolvedAddonRoots
            throw "Diagnostic runtime exited before complete artifacts were validated (exit code $($process.ExitCode)): $launchEvidence"
        }

        $logTail = Get-GuardLogTail -GuardRoot $guardRoot
        $armedCanary = "Partisan campaign debug CLI | armed exact HST_Dev full certification run"
        if ($Profile -eq "force_authority") {
            $armedCanary = "Partisan campaign debug CLI | armed focused force_authority run (not full certification)"
        }
        if (-not $armed -and $logTail.Contains($armedCanary)) {
            $armed = $true
        }
        if (-not $started -and
            $logTail -match ("Partisan campaign debug CLI \| started " + [regex]::Escape($Profile) + " on attempt \d+")) {
            $started = $true
        }

        if (-not $armed -and $stopwatch.Elapsed.TotalSeconds -gt $ArmedTimeoutSeconds) {
            throw "Campaign Debug CLI did not arm within the startup canary window."
        }
        if ($armed -and -not $started -and
            $stopwatch.Elapsed.TotalSeconds -gt $StartedTimeoutSeconds) {
            throw "Campaign Debug CLI armed but did not start within the startup canary window."
        }

        $debugDirectory = Join-Path $guardRoot "profile\Partisan\debug"
        $jsonCandidates = @()
        if (Test-Path -LiteralPath $debugDirectory -PathType Container) {
            $jsonCandidates = @(Get-ChildItem -LiteralPath $debugDirectory -File -Filter "HST_CampaignDebug_*.json" |
                Sort-Object LastWriteTimeUtc -Descending)
        }

        if ($jsonCandidates.Count -gt 0) {
            $jsonPath = $jsonCandidates[0].FullName
            $base = [IO.Path]::Combine(
                $jsonCandidates[0].DirectoryName,
                [IO.Path]::GetFileNameWithoutExtension($jsonCandidates[0].Name))
            $summaryPath = $base + "_summary.txt"
            $stateDiffPath = $base + "_state_diff.txt"
            if ((Test-Path -LiteralPath $summaryPath -PathType Leaf) -and
                (Test-Path -LiteralPath $stateDiffPath -PathType Leaf) -and
                (Get-Item -LiteralPath $jsonPath).Length -gt 0 -and
                (Get-Item -LiteralPath $summaryPath).Length -gt 0 -and
                (Get-Item -LiteralPath $stateDiffPath).Length -gt 0) {
                $signature = Get-FileSignature -Paths @($jsonPath, $summaryPath, $stateDiffPath)
                if ($signature -eq $artifactSignature) {
                    $artifactSignaturePolls++
                }
                else {
                    $artifactSignature = $signature
                    $artifactSignaturePolls = 1
                }

                if ($artifactSignaturePolls -ge 2) {
                    $artifactPaths = @($jsonPath, $summaryPath, $stateDiffPath)
                    $artifactParameters = @{
                        JsonPath = $jsonPath
                        SummaryPath = $summaryPath
                        StateDiffPath = $stateDiffPath
                        ExpectedSha = $ExpectedBuildSha
                        ExpectedUtc = $ExpectedBuildUtc
                        ExpectedLabel = $ExpectedBuildLabel
                        ExpectedProfile = $Profile
                    }
                    if ($Profile -ceq 'force_authority') {
                        $artifactParameters.RequireCorrectedCanaryContract = $true
                    }
                    $artifactValidation = Test-CampaignDebugArtifacts @artifactParameters
                    $safeArtifactValidation = Get-SafeArtifactValidationSummary `
                        -Validation $artifactValidation `
                        -ExpectedBuildSha $ExpectedBuildSha `
                        -ExpectedBuildUtc $ExpectedBuildUtc `
                        -ExpectedBuildLabel $ExpectedBuildLabel `
                        -GuardRoot $guardRoot `
                        -ProjectDirectory (Split-Path -Parent $projectFile) `
                        -ResolvedAddonRoots $resolvedAddonRoots
                    $completed = $true
                    break
                }
            }
        }

        if ($stopwatch.Elapsed.TotalSeconds -ge $nextHeartbeat) {
            Write-Output ("PROGRESS " + ([pscustomobject]@{
                ElapsedSeconds = [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
                Armed = $armed
                Started = $started
                ArtifactJsonCandidates = $jsonCandidates.Count
                StableArtifactPolls = $artifactSignaturePolls
                OwnedProcesses = $ownedProcesses.Count
            } | ConvertTo-Json -Compress))
            $nextHeartbeat = $stopwatch.Elapsed.TotalSeconds + 30
        }
    }

    if (-not $completed) {
        throw "Campaign Debug runtime exceeded the guarded completion deadline."
    }

    Update-OwnedProcesses `
        -Owned $ownedProcesses `
        -RootProcessId $rootProcessId `
        -RootStartUtc $rootStartUtc `
        -Job $job
    $process.Refresh()
    if (-not $process.HasExited) {
        [void]$process.CloseMainWindow()
        [void]$process.WaitForExit(10000)
    }
    Update-OwnedProcesses `
        -Owned $ownedProcesses `
        -RootProcessId $rootProcessId `
        -RootStartUtc $rootStartUtc `
        -Job $job
    Stop-OwnedProcesses -Owned $ownedProcesses
    Start-Sleep -Seconds 2
    Update-OwnedProcesses `
        -Owned $ownedProcesses `
        -RootProcessId $rootProcessId `
        -RootStartUtc $rootStartUtc `
        -Job $job
    Stop-OwnedProcesses -Owned $ownedProcesses
    $ownedRemainingBeforeCensus = 0
    foreach ($ownedProcessId in @($ownedProcesses.Keys)) {
        if (Test-ProcessIdentityAlive `
            -ProcessId ([int]$ownedProcessId) `
            -StartUtc $ownedProcesses[[int]$ownedProcessId]) {
            $ownedRemainingBeforeCensus++
        }
    }
    if ($ownedRemainingBeforeCensus -ne 0) {
        throw "Owned runtime processes remained alive before the final error census."
    }
    foreach ($engineProcess in @(Get-EngineProcessRows)) {
        if ($ownedProcesses.ContainsKey([int]$engineProcess.Id)) {
            continue
        }
        try {
            [void]$unclaimedEngineProcessesObserved.Add(
                "$($engineProcess.ProcessName):$($engineProcess.StartTime.ToUniversalTime().Ticks)")
        }
        catch { }
    }
    if ($unclaimedEngineProcessesObserved.Count -ne 0) {
        throw "An unowned engine process appeared before the final error census."
    }
    if (-not $guardedSettingsPath -or
        -not (Test-Path -LiteralPath $guardedSettingsPath -PathType Leaf) -or
        (Get-FileHash `
            -LiteralPath $guardedSettingsPath `
            -Algorithm SHA256).Hash.ToLowerInvariant() -cne $guardedSettingsSha256) {
        throw 'The guarded Campaign Debug settings changed during runtime execution.'
    }
    [void](Assert-HarnessBinding `
        -Expected $harnessBinding `
        -CheckoutRoot $checkoutRoot `
        -RunnerPath $PSCommandPath `
        -CandidateModulePath $candidateModulePath `
        -ReleaseIndexProducerPath $releaseIndexProducerPath `
        -ReleaseDocsConsumerPath $releaseDocsConsumerPath)
    [void](Assert-PartisanReleaseCandidateStage `
        -Candidate $candidateBinding `
        -StageRoot $candidateStage.StageRootPath)
    $postCandidateBinding = Assert-PartisanReleaseCandidate `
        -ManifestPath $CandidateManifest `
        -BundleRoot $CandidateBundleRoot `
        -RuntimeAddonRoot $RuntimeAddonRoot `
        -Executable $executablePath `
        -RuntimeRole $runtimeRole `
        -ConsumerIntent $consumerIntent
    if ($postCandidateBinding.PackageSha256 -cne $candidateBinding.PackageSha256 -or
        $postCandidateBinding.ManifestSha256 -cne $candidateBinding.ManifestSha256 -or
        $postCandidateBinding.ReadySha256 -cne $candidateBinding.ReadySha256) {
        throw 'The sealed candidate identity changed during Campaign Debug execution.'
    }
    $candidateBoundaryVerified = $true
    $mountAttestation = Get-CandidateMountAttestation `
        -GuardRoot $guardRoot `
        -PackedProjectPath $candidateStage.PackedProjectPath `
        -AddonGuid $candidateBinding.AddonGuid
    if (-not $mountAttestation.Valid) {
        throw 'Engine logs did not attest the exact guarded packed candidate mount.'
    }
    $errorCensus = Get-GuardErrorCensus `
        -GuardRoot $guardRoot `
        -Profile $Profile `
        -IntentionalMissionConvoyAdmissionDiagnosticsProven `
            $artifactValidation.IntentionalMissionConvoyAdmissionDiagnosticsProven `
        -IntentionalMissionConvoySettlementDiagnosticProven `
            $artifactValidation.IntentionalMissionConvoySettlementDiagnosticProven `
        -IntentionalMissionConvoyCorruptionDiagnosticsProven `
            $artifactValidation.IntentionalMissionConvoyCorruptionDiagnosticsProven `
        -IntentionalMissionConvoyWatchdogDiagnosticProven `
            $artifactValidation.IntentionalMissionConvoyWatchdogDiagnosticProven
    if (-not $artifactValidation.Valid) {
        throw "Campaign Debug artifacts completed but failed the exact validation contract."
    }
    if (-not $errorCensus.Valid) {
        throw "Campaign Debug runtime completed with unapproved hard diagnostics."
    }

    Write-Output ("RESULT " + ([pscustomobject]@{
        Candidate = $candidateIdentity
        CandidateBoundaryVerified = $candidateBoundaryVerified
        RuntimeSeconds = [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
        Armed = $armed
        Started = $started
        ShutdownCensus = $true
        MountAttestation = $mountAttestation
        ArtifactsStable = $artifactSignaturePolls -ge 2
        ArtifactBytes = @($artifactPaths | ForEach-Object { (Get-Item -LiteralPath $_).Length })
        Validation = $safeArtifactValidation
        ErrorCensus = $errorCensus
        HardDiagnosticClassifierChecks = $hardDiagnosticClassifierChecks
    } | ConvertTo-Json -Depth 8 -Compress))
}
catch {
    if ($_.Exception -is [OperationCanceledException] -and
        $_.Exception.Message -eq "guarded-preflight-complete") {
        $runError = $null
    }
    else {
        $runError = $_.Exception.Message
    }
    if ($artifactValidation) {
        Write-Output ("RESULT " + ([pscustomobject]@{
            Candidate = $candidateIdentity
            CandidateBoundaryVerified = $candidateBoundaryVerified
            RuntimeSeconds = [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
            Armed = $armed
            Started = $started
            ShutdownCensus = [bool]$errorCensus
            MountAttestation = $mountAttestation
            ArtifactsStable = $artifactSignaturePolls -ge 2
            Validation = $safeArtifactValidation
            ErrorCensus = $errorCensus
            HardDiagnosticClassifierChecks = $hardDiagnosticClassifierChecks
        } | ConvertTo-Json -Depth 8 -Compress))
    }
}
finally {
    $cleanupState = [ordered]@{
        GuardRemaining = -1
        OwnedProcessesRemaining = -1
        NewEngineProcessesRemaining = -1
        NewDefaultEntriesRemaining = -1
        ModifiedDefaultFiles = -1
        DeletedDefaultEntries = -1
        MissingDefaultRoots = -1
        ExternalSpillEntriesRemaining = -1
        ModifiedSpillFiles = -1
        DeletedSpillEntries = -1
        MissingSpillRoots = -1
    }

    Invoke-IsolatedCleanupPhase -Name "discover-owned-processes" -Errors $cleanupPhaseErrors -Action {
        if ($rootProcessId -gt 0 -and $rootStartUtc -ne [DateTime]::MinValue) {
            Update-OwnedProcesses `
                -Owned $ownedProcesses `
                -RootProcessId $rootProcessId `
                -RootStartUtc $rootStartUtc `
                -Job $job
        }
    }
    Invoke-IsolatedCleanupPhase -Name "dispose-suspended-launcher" -Errors $cleanupPhaseErrors -Action {
        if ($suspendedLauncher) {
            $suspendedLauncher.Dispose()
            $suspendedLauncher = $null
        }
    }
    Invoke-IsolatedCleanupPhase -Name "request-root-stop" -Errors $cleanupPhaseErrors -Action {
        if ($process) {
            $process.Refresh()
            if (-not $process.HasExited) {
                [void]$process.CloseMainWindow()
                if (-not $process.WaitForExit(10000)) {
                    Stop-OwnedProcesses -Owned $ownedProcesses
                }
            }
        }
    }
    Invoke-IsolatedCleanupPhase -Name "force-owned-stop" -Errors $cleanupPhaseErrors -Action {
        if ($rootProcessId -gt 0 -and $rootStartUtc -ne [DateTime]::MinValue) {
            Update-OwnedProcesses `
                -Owned $ownedProcesses `
                -RootProcessId $rootProcessId `
                -RootStartUtc $rootStartUtc `
                -Job $job
        }
        Stop-OwnedProcesses -Owned $ownedProcesses
        Start-Sleep -Milliseconds 500
        Stop-OwnedProcesses -Owned $ownedProcesses
    }
    Invoke-IsolatedCleanupPhase -Name "close-process-job" -Errors $cleanupPhaseErrors -Action {
        if ($job) {
            $job.Dispose()
        }
    }
    Invoke-IsolatedCleanupPhase -Name "dispose-root-process" -Errors $cleanupPhaseErrors -Action {
        if ($process) {
            $process.Dispose()
        }
    }
    Invoke-IsolatedCleanupPhase -Name 'verify-candidate-boundaries' -Errors $cleanupPhaseErrors -Action {
        if ($candidateStage) {
            [void](Assert-PartisanReleaseCandidateStage `
                -Candidate $candidateBinding `
                -StageRoot $candidateStage.StageRootPath)
        }
        $cleanupCandidateBinding = Assert-PartisanReleaseCandidate `
            -ManifestPath $CandidateManifest `
            -BundleRoot $CandidateBundleRoot `
            -RuntimeAddonRoot $RuntimeAddonRoot `
            -Executable $executablePath `
            -RuntimeRole $runtimeRole `
            -ConsumerIntent $consumerIntent
        if ($cleanupCandidateBinding.PackageSha256 -cne $candidateBinding.PackageSha256 -or
            $cleanupCandidateBinding.ManifestSha256 -cne $candidateBinding.ManifestSha256 -or
            $cleanupCandidateBinding.ReadySha256 -cne $candidateBinding.ReadySha256) {
            throw 'The sealed candidate identity changed across the guarded run.'
        }
    }
    Invoke-IsolatedCleanupPhase -Name 'verify-settings-binding' -Errors $cleanupPhaseErrors -Action {
        if (-not $guardedSettingsPath -or
            -not (Test-Path -LiteralPath $guardedSettingsPath -PathType Leaf) -or
            (Get-FileHash `
                -LiteralPath $guardedSettingsPath `
                -Algorithm SHA256).Hash.ToLowerInvariant() -cne $guardedSettingsSha256) {
            throw 'The guarded Campaign Debug settings identity changed.'
        }
    }
    Invoke-IsolatedCleanupPhase -Name 'verify-harness-binding' -Errors $cleanupPhaseErrors -Action {
        [void](Assert-HarnessBinding `
            -Expected $harnessBinding `
            -CheckoutRoot $checkoutRoot `
            -RunnerPath $PSCommandPath `
            -CandidateModulePath $candidateModulePath `
            -ReleaseIndexProducerPath $releaseIndexProducerPath `
            -ReleaseDocsConsumerPath $releaseDocsConsumerPath)
    }
    Invoke-IsolatedCleanupPhase -Name 'retain-runtime-evidence' -Errors $cleanupPhaseErrors -Action {
        if ($evidenceRunRoot) {
            if (-not (Test-Path -LiteralPath $guardRoot -PathType Container)) {
                throw 'The guarded runtime root disappeared before evidence retention.'
            }
            Copy-CampaignDebugEvidence `
                -GuardRoot $guardRoot `
                -EvidenceRunRoot $evidenceRunRoot `
                -SettingsPath $guardedSettingsPath `
                -ExpectedSettingsSha256 $guardedSettingsSha256 `
                -Candidate $candidateBinding
            $evidenceState.Captured = $true
        }
    }
    Invoke-IsolatedCleanupPhase -Name "remove-owned-guard" -Errors $cleanupPhaseErrors -Action {
        Assert-NoReparsePathAncestry -Path $tempBase
        if (-not $guardOwnership -and
            (Test-Path -LiteralPath $guardRoot -PathType Container)) {
            $candidateOwnership = Read-GuardOwnership -Directory $guardRoot -GuardBase $tempBase
            if ($candidateOwnership -and
                $candidateOwnership.Nonce -ceq $nonce -and
                $candidateOwnership.OwnerPid -eq $PID -and
                $wrapperStartUtc -ne [DateTime]::MinValue -and
                $candidateOwnership.OwnerStartUtc.Ticks -eq $wrapperStartUtc.Ticks) {
                $guardOwnership = $candidateOwnership
            }
        }
        if ($guardOwnership -and
            -not (Remove-ExactOwnedGuard -Ownership $guardOwnership -GuardBase $tempBase)) {
            throw "Exact owned guard removal failed."
        }
    }
    Invoke-IsolatedCleanupPhase -Name "remove-empty-guard-base" -Errors $cleanupPhaseErrors -Action {
        Assert-NoReparsePathAncestry -Path $tempBase
        for ($attempt = 1; $attempt -le 3; $attempt++) {
            if (-not (Test-Path -LiteralPath $tempBase -PathType Container)) {
                break
            }
            $baseItem = Get-Item -LiteralPath $tempBase -Force
            if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Guard base became a reparse point."
            }
            if (@(Get-ChildItem -LiteralPath $tempBase -Force -ErrorAction Stop).Count -ne 0) {
                break
            }
            try {
                Remove-Item -LiteralPath $tempBase -Force -ErrorAction Stop
            }
            catch {
                if ($attempt -eq 3) {
                    throw
                }
                Start-Sleep -Milliseconds 250
            }
        }
    }
    Invoke-IsolatedCleanupPhase -Name "audit-owned-processes" -Errors $cleanupPhaseErrors -Action {
        $remaining = 0
        foreach ($ownedProcessId in @($ownedProcesses.Keys)) {
            if (Test-ProcessIdentityAlive `
                -ProcessId ([int]$ownedProcessId) `
                -StartUtc $ownedProcesses[[int]$ownedProcessId]) {
                $remaining++
            }
        }
        $cleanupState.OwnedProcessesRemaining = $remaining
    }
    Invoke-IsolatedCleanupPhase -Name "audit-engine-processes" -Errors $cleanupPhaseErrors -Action {
        $cleanupState.NewEngineProcessesRemaining = @(Get-EngineProcessRows).Count
    }
    Invoke-IsolatedCleanupPhase -Name "audit-external-boundaries" -Errors $cleanupPhaseErrors -Action {
        $defaultNew = 0
        $defaultModified = 0
        $defaultDeleted = 0
        $defaultMissing = 0
        foreach ($snapshot in $watchSnapshots) {
            $delta = Get-RootSnapshotDelta -Snapshot $snapshot
            $defaultNew += $delta.NewEntries
            $defaultModified += $delta.ModifiedFiles
            $defaultDeleted += $delta.DeletedEntries
            $defaultMissing += $delta.MissingRoot
        }
        $spillNew = 0
        $spillModified = 0
        $spillDeleted = 0
        $spillMissing = 0
        foreach ($snapshot in $spillSnapshots) {
            $delta = Get-RootSnapshotDelta -Snapshot $snapshot
            $spillNew += $delta.NewEntries
            $spillModified += $delta.ModifiedFiles
            $spillDeleted += $delta.DeletedEntries
            $spillMissing += $delta.MissingRoot
        }
        $cleanupState.NewDefaultEntriesRemaining = $defaultNew
        $cleanupState.ModifiedDefaultFiles = $defaultModified
        $cleanupState.DeletedDefaultEntries = $defaultDeleted
        $cleanupState.MissingDefaultRoots = $defaultMissing
        $cleanupState.ExternalSpillEntriesRemaining = $spillNew
        $cleanupState.ModifiedSpillFiles = $spillModified
        $cleanupState.DeletedSpillEntries = $spillDeleted
        $cleanupState.MissingSpillRoots = $spillMissing
    }
    Invoke-IsolatedCleanupPhase -Name "audit-guard" -Errors $cleanupPhaseErrors -Action {
        $cleanupState.GuardRemaining = [int](Test-Path -LiteralPath $guardRoot)
    }
    Invoke-IsolatedCleanupPhase -Name "release-wrapper-lock" -Errors $cleanupPhaseErrors -Action {
        if ($mutexAcquired -and $mutex) {
            $mutex.ReleaseMutex()
            $mutexAcquired = $false
        }
        if ($mutex) {
            $mutex.Dispose()
        }
    }

    $cleanupResult = [pscustomobject]@{
        GuardRemaining = $cleanupState.GuardRemaining
        OwnedProcessesRemaining = $cleanupState.OwnedProcessesRemaining
        NewEngineProcessesRemaining = $cleanupState.NewEngineProcessesRemaining
        UnclaimedEngineProcessesObserved = $unclaimedEngineProcessesObserved.Count
        NewDefaultEntriesRemaining = $cleanupState.NewDefaultEntriesRemaining
        ModifiedDefaultFiles = $cleanupState.ModifiedDefaultFiles
        DeletedDefaultEntries = $cleanupState.DeletedDefaultEntries
        MissingDefaultRoots = $cleanupState.MissingDefaultRoots
        ExternalSpillEntriesRemaining = $cleanupState.ExternalSpillEntriesRemaining
        ModifiedSpillFiles = $cleanupState.ModifiedSpillFiles
        DeletedSpillEntries = $cleanupState.DeletedSpillEntries
        MissingSpillRoots = $cleanupState.MissingSpillRoots
        CleanupPhaseErrorCount = $cleanupPhaseErrors.Count
        CleanupPhaseErrors = $cleanupPhaseErrors.ToArray()
        MonitoringRootsAreDetectionOnly = $true
    }
    Write-Output ("CLEANUP " + ($cleanupResult | ConvertTo-Json -Compress))
}

$cleanupPassed = $cleanupResult -and
    $cleanupResult.GuardRemaining -eq 0 -and
    $cleanupResult.OwnedProcessesRemaining -eq 0 -and
    $cleanupResult.NewEngineProcessesRemaining -eq 0 -and
    $cleanupResult.UnclaimedEngineProcessesObserved -eq 0 -and
    $cleanupResult.NewDefaultEntriesRemaining -eq 0 -and
    $cleanupResult.ModifiedDefaultFiles -eq 0 -and
    $cleanupResult.DeletedDefaultEntries -eq 0 -and
    $cleanupResult.MissingDefaultRoots -eq 0 -and
    $cleanupResult.ExternalSpillEntriesRemaining -eq 0 -and
    $cleanupResult.ModifiedSpillFiles -eq 0 -and
    $cleanupResult.DeletedSpillEntries -eq 0 -and
    $cleanupResult.MissingSpillRoots -eq 0 -and
    $cleanupResult.CleanupPhaseErrorCount -eq 0

if ($evidenceRunRoot) {
    try {
        $evidenceEndUtc = [DateTime]::UtcNow
        $safeEvidenceError = ''
        if ($runError) {
            $safeEvidenceError = ConvertTo-SafeEvidenceLine `
                -Line $runError `
                -GuardRoot $guardRoot `
                -ProjectDirectory (Split-Path -Parent $projectFile) `
                -ResolvedAddonRoots $resolvedAddonRoots
        }
        $safeCleanupErrors = @($cleanupResult.CleanupPhaseErrors | ForEach-Object {
            ConvertTo-SafeEvidenceLine `
                -Line ([string]$_) `
                -GuardRoot $guardRoot `
                -ProjectDirectory (Split-Path -Parent $projectFile) `
                -ResolvedAddonRoots $resolvedAddonRoots
        })
        $portableCleanup = [ordered]@{
            guardRemaining = $cleanupResult.GuardRemaining
            ownedProcessesRemaining = $cleanupResult.OwnedProcessesRemaining
            newEngineProcessesRemaining = $cleanupResult.NewEngineProcessesRemaining
            unclaimedEngineProcessesObserved = $cleanupResult.UnclaimedEngineProcessesObserved
            newDefaultEntriesRemaining = $cleanupResult.NewDefaultEntriesRemaining
            modifiedDefaultFiles = $cleanupResult.ModifiedDefaultFiles
            deletedDefaultEntries = $cleanupResult.DeletedDefaultEntries
            missingDefaultRoots = $cleanupResult.MissingDefaultRoots
            externalSpillEntriesRemaining = $cleanupResult.ExternalSpillEntriesRemaining
            modifiedSpillFiles = $cleanupResult.ModifiedSpillFiles
            deletedSpillEntries = $cleanupResult.DeletedSpillEntries
            missingSpillRoots = $cleanupResult.MissingSpillRoots
            cleanupPhaseErrorCount = $cleanupResult.CleanupPhaseErrorCount
            cleanupPhaseErrors = $safeCleanupErrors
            monitoringRootsAreDetectionOnly = $true
        }
        $rawRows = Get-EvidenceFileRows -EvidenceRunRoot $evidenceRunRoot
        $envelope = [ordered]@{
            schemaVersion = 2
            evidenceKind = 'packaged-campaign-debug'
            startedUtc = $evidenceStartUtc.ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
            completedUtc = $evidenceEndUtc.ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
            candidate = $candidateIdentity
            harness = [ordered]@{
                gitHead = $harnessBinding.GitHead
                dirty = -not $harnessBinding.Clean
                campaignRunnerSha256 = $harnessBinding.RunnerSha256
                campaignRunnerGitBlobSha256 =
                    $harnessBinding.RunnerGitBlobSha256
                candidateModuleSha256 = $harnessBinding.CandidateModuleSha256
                candidateModuleGitBlobSha256 =
                    $harnessBinding.CandidateModuleGitBlobSha256
                releaseIndexProducerSha256 =
                    $harnessBinding.ReleaseIndexProducerSha256
                releaseIndexProducerGitBlobSha256 =
                    $harnessBinding.ReleaseIndexProducerGitBlobSha256
                releaseDocsConsumerSha256 =
                    $harnessBinding.ReleaseDocsConsumerSha256
                releaseDocsConsumerGitBlobSha256 =
                    $harnessBinding.ReleaseDocsConsumerGitBlobSha256
            }
            launch = [ordered]@{
                profile = $Profile
                proofScope = if ($Profile -ceq 'full_certification') {
                    'full_certification'
                }
                else {
                    'focused_force_authority'
                }
                worldResource = $WorldResource
                stagedPackage = $true
                addonSearchRootCount = 2
                addonGuid = $candidateBinding.AddonGuid
                packageSha256 = if ($candidateStage) {
                    $candidateStage.PackageSha256
                }
                else {
                    $candidateBinding.PackageSha256
                }
                diagnosticExecutable = $candidateBinding.DiagnosticExecutable
                recordedRuntimeExecutable = $candidateBinding.RecordedRuntimeExecutable
            }
            outcome = [ordered]@{
                success = -not $runError -and $cleanupPassed
                armed = $armed
                started = $started
                completed = $completed
                candidateBoundaryVerified = $candidateBoundaryVerified
                mountAttestation = $mountAttestation
                artifactsStable = $artifactSignaturePolls -ge 2
                evidenceCaptured = $evidenceState.Captured
                hardDiagnosticClassifierChecks = $hardDiagnosticClassifierChecks
                runtimeSeconds = if ($stopwatch) {
                    [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
                }
                else {
                    0
                }
                error = $safeEvidenceError
                validation = $safeArtifactValidation
                errorCensus = $errorCensus
            }
            settings = [ordered]@{
                schemaVersion = [int]$settings.schemaVersion
                sha256 = $guardedSettingsSha256
                guardedRuntimeCopy = $true
            }
            cleanup = $portableCleanup
            files = $rawRows
        }
        $envelopePath = Join-Path $evidenceRunRoot 'run.json'
        Write-PortableJson -Path $envelopePath -Value $envelope
        $envelopeSha = (Get-FileHash `
            -LiteralPath $envelopePath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        $evidenceReceipt = [ordered]@{
            CandidateId = $candidateBinding.CandidateId
            Profile = $Profile
            FileCount = $rawRows.Count
            EnvelopeSha256 = $envelopeSha
        }
        $releaseIndexRows = @(& $releaseIndexProducerPath `
            -RunEnvelopePath $envelopePath `
            -OutputPath (Join-Path $evidenceRunRoot 'release-index.json'))
        if ($releaseIndexRows.Count -ne 1 -or
            [string]::IsNullOrWhiteSpace(
                [string]$releaseIndexRows[0].ReleaseIndexSha256)) {
            throw 'The portable Campaign Debug release-index producer returned an invalid result.'
        }
        $evidenceReceipt.ReleaseIndexSha256 =
            $releaseIndexRows[0].ReleaseIndexSha256
        $evidenceReceipt.ReleaseIndexStatus = $releaseIndexRows[0].Status
        Write-Output ('EVIDENCE ' + ([pscustomobject]$evidenceReceipt |
            ConvertTo-Json -Compress))
    }
    catch {
        $evidenceError = $_.Exception.Message
        if ($runError) {
            $runError += ' | evidence retention: ' + $evidenceError
        }
        else {
            $runError = 'Runtime evidence retention failed: ' + $evidenceError
        }
    }
}

if ($runError) {
    $safeRunError = ConvertTo-SafeEvidenceLine `
        -Line $runError `
        -GuardRoot $guardRoot `
        -ProjectDirectory (Split-Path -Parent $projectFile) `
        -ResolvedAddonRoots $resolvedAddonRoots
    Write-Error $safeRunError
    exit 1
}
if (-not $cleanupPassed) {
    Write-Error "Guarded Campaign Debug cleanup did not return every tracked boundary to zero."
    exit 2
}
exit 0

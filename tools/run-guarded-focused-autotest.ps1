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
    [ValidatePattern('^[A-Za-z_][A-Za-z0-9_]*$')]
    [string]$TestCase,

    [string[]]$RequiredLogPatterns = @(),
    [string[]]$DefaultRoots = @(),
    [string[]]$SpillRoots = @(),
    [string]$EvidenceOutputRoot = '',

    [ValidateRange(1, 3600)]
    [int]$TimeoutSeconds = 240,

    [ValidateRange(50, 5000)]
    [int]$PollMilliseconds = 500,

    [switch]$PreflightOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:FocusedRequiredPatternMarker =
    'PARTISAN_REQUIRED_LOG_PATTERN_B64 '
$script:FocusedSuiteTestCases = [ordered]@{
    HST_EnemyCounterattackAutotestSuite = @(
        'HST_TEST_EnemyCounterattack_FrozenPlanning',
        'HST_TEST_EnemyCounterattack_Admission',
        'HST_TEST_EnemyCounterattack_VirtualTravel',
        'HST_TEST_EnemyCounterattack_VirtualCombat',
        'HST_TEST_EnemyCounterattack_PhysicalHandoff',
        'HST_TEST_EnemyCounterattack_OwnershipRetry',
        'HST_TEST_EnemyCounterattack_SettlementReplay',
        'HST_TEST_EnemyCounterattack_SupportSettlement',
        'HST_TEST_EnemyCounterattack_RestoreLifecycle',
        'HST_TEST_EnemyCounterattack_ResourceAuthorityQuarantine',
        'HST_TEST_EnemyCounterattack_AmbiguityHold',
        'HST_TEST_EnemyCounterattack_OwnershipCorrelationQuarantine',
        'HST_TEST_EnemyCounterattack_Schema69Quarantine',
        'HST_TEST_EnemyCounterattack_QuarantineRetention'
    )
    HST_EnemyGarrisonRebuildAutotestSuite = @(
        'HST_TEST_EnemyGarrisonRebuild_AdmissionCapacity',
        'HST_TEST_EnemyGarrisonRebuild_DeliveryHeld',
        'HST_TEST_EnemyGarrisonRebuild_CasualtyContinuity',
        'HST_TEST_EnemyGarrisonRebuild_Restore',
        'HST_TEST_EnemyGarrisonRebuild_OwnershipTerminal',
        'HST_TEST_EnemyGarrisonRebuild_AdmissionRollback',
        'HST_TEST_EnemyGarrisonRebuild_PrearrivalRefund',
        'HST_TEST_EnemyGarrisonRebuild_SettlementCrashResume',
        'HST_TEST_EnemyGarrisonRebuild_HistoricalIsolation',
        'HST_TEST_EnemyGarrisonRebuild_Schema70Quarantine',
        'HST_TEST_EnemyGarrisonRebuild_OrphanRuntimeQuarantine',
        'HST_TEST_EnemyGarrisonRebuild_QuarantineRetention',
        'HST_TEST_EnemyGarrisonRebuild_SelectedOwnershipABA'
    )
    HST_EnemyPlanningCommitmentAutotestSuite = @(
        'HST_TEST_EnemyPlanning_Pre68Baseline',
        'HST_TEST_EnemyPlanning_IndependentCadence',
        'HST_TEST_EnemyPlanning_BeginReplayConflict',
        'HST_TEST_EnemyPlanning_CommitmentPermutation',
        'HST_TEST_EnemyPlanning_CommitmentAwareSelection',
        'HST_TEST_EnemyPlanning_AllCommittedSkip',
        'HST_TEST_EnemyPlanning_CommitmentRaceRejection',
        'HST_TEST_EnemyPlanning_FrozenDecision',
        'HST_TEST_EnemyPlanning_RetryEnvelope',
        'HST_TEST_EnemyPlanning_PreparedPressureCrashWindow',
        'HST_TEST_EnemyPlanning_PreparedOrderAdoption',
        'HST_TEST_EnemyPlanning_RetryTamperQuarantine',
        'HST_TEST_EnemyPlanning_ZeroTargetSkip',
        'HST_TEST_EnemyPlanning_CommittedRoundtrip',
        'HST_TEST_EnemyPlanning_CurrentQuarantine',
        'HST_TEST_EnemyPlanning_FreshBootstrap',
        'HST_TEST_EnemyPlanning_UnavailableLogThrottle'
    )
    HST_EnemyQRFAutotestSuite = @(
        'HST_TEST_EnemyQRF_Admission',
        'HST_TEST_EnemyQRF_LegacyIsolation',
        'HST_TEST_EnemyQRF_Projection',
        'HST_TEST_EnemyQRF_Settlement',
        'HST_TEST_EnemyQRF_Restore',
        'HST_TEST_EnemyQRF_Rejection'
    )
    HST_CampaignProfileJournalAuthorityAutotestSuite = @(
        'HST_TEST_CampaignProfileJournalAuthority_GenerationAdvance',
        'HST_TEST_CampaignProfileJournalAuthority_CanonicalGenerationOnePreserved',
        'HST_TEST_CampaignProfileJournalAuthority_TruncatedNewestFallback',
        'HST_TEST_CampaignProfileJournalAuthority_BadFingerprintFallback',
        'HST_TEST_CampaignProfileJournalAuthority_BothInvalidRejected',
        'HST_TEST_CampaignProfileJournalAuthority_BothInvalidSourceFatal',
        'HST_TEST_CampaignProfileJournalAuthority_FutureEnvelopeRejected',
        'HST_TEST_CampaignProfileJournalAuthority_UnknownMagicRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureSchemaRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureRawRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureArtifactWriteNonMutating',
        'HST_TEST_CampaignProfileJournalAuthority_LegacyRawUpgrade',
        'HST_TEST_CampaignProfileJournalAuthority_SplitBrainRejected',
        'HST_TEST_CampaignProfileJournalAuthority_BrokenChainRejected',
        'HST_TEST_CampaignProfileJournalAuthority_GenerationOneParentGenerationRejected',
        'HST_TEST_CampaignProfileJournalAuthority_AdjacentWrongParentRejected',
        'HST_TEST_CampaignProfileJournalAuthority_NonAdjacentParentFingerprintRejected',
        'HST_TEST_CampaignProfileJournalAuthority_DuplicateMetadataRejected',
        'HST_TEST_CampaignProfileJournalAuthority_FutureWriteNonMutating',
        'HST_TEST_CampaignProfileJournalAuthority_SelectedReadOnly',
        'HST_TEST_CampaignProfileJournalAuthority_DegradedNativeRecovery',
        'HST_TEST_CampaignProfileJournalAuthority_FallbackOnlyCheckpoint',
        'HST_TEST_CampaignProfileJournalAuthority_FailedNativeCallbackNonMutating',
        'HST_TEST_CampaignProfileJournalAuthority_ValidNativeInvalidJournal',
        'HST_TEST_CampaignProfileJournalAuthority_ValidNativeFutureJournal',
        'HST_TEST_CampaignProfileJournalAuthority_FutureNativeAuthorityRejected',
        'HST_TEST_CampaignProfileJournalAuthority_LegacyNativeFingerprintAccepted',
        'HST_TEST_CampaignProfileJournalAuthority_NativeV1LoadClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NativeV2LoadClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NativeInvalidFingerprintClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NativeFutureEnvelopeClassification',
        'HST_TEST_CampaignProfileJournalAuthority_NewerJournalAuthority',
        'HST_TEST_CampaignProfileJournalAuthority_NewerNativeAuthority',
        'HST_TEST_CampaignProfileJournalAuthority_EqualOrderConflictRejected',
        'HST_TEST_CampaignProfileJournalAuthority_LastSaveSecondNewerJournal',
        'HST_TEST_CampaignProfileJournalAuthority_LastSaveSecondNewerNative',
        'HST_TEST_CampaignProfileJournalAuthority_EqualOrderSameFingerprintNative',
        'HST_TEST_CampaignProfileJournalAuthority_LegacyRawEqualOrderNative',
        'HST_TEST_CampaignProfileJournalAuthority_CheckpointSequenceOrdering',
        'HST_TEST_CampaignProfileJournalAuthority_AuthorityJournalMetadata',
        'HST_TEST_CampaignProfileJournalAuthority_Cleanup'
    )
}

function Get-FocusedExpectedTestCases {
    param([Parameter(Mandatory = $true)][string]$Selector)

    if ($script:FocusedSuiteTestCases.Contains($Selector)) {
        return @($script:FocusedSuiteTestCases[$Selector])
    }
    return @($Selector)
}

function Get-NormalizedRequiredLogPatterns {
    param([AllowEmptyCollection()][string[]]$Patterns)

    if ($null -eq $Patterns -or $Patterns.Count -eq 0 -or
        $Patterns.Count -gt 64) {
        throw 'Focused autotest requires between one and 64 required log patterns.'
    }
    $normalized = New-Object Collections.Generic.List[string]
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($patternValue in $Patterns) {
        $pattern = [string]$patternValue
        if ([string]::IsNullOrWhiteSpace($pattern) -or
            $pattern.Length -gt 4096 -or
            $pattern -match '[\x00\r\n]' -or
            -not $seen.Add($pattern)) {
            throw 'Focused required log patterns must be unique, nonempty, single-line text.'
        }
        [void]$normalized.Add($pattern)
    }
    return $normalized.ToArray()
}

function Write-RequiredLogPatternContract {
    param(
        [Parameter(Mandatory = $true)][string]$ConsolePath,
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)][string]$OriginalConsoleText,
        [Parameter(Mandatory = $true)][string[]]$Patterns
    )

    $utf8 = New-Object Text.UTF8Encoding($false, $true)
    $contractLines = @($Patterns | ForEach-Object {
        $script:FocusedRequiredPatternMarker +
            [Convert]::ToBase64String($utf8.GetBytes($_))
    })
    $separator = if ($OriginalConsoleText.EndsWith(
            "`n",
            [StringComparison]::Ordinal)) {
        ''
    }
    else {
        "`n"
    }
    [IO.File]::AppendAllText(
        $ConsolePath,
        $separator + ($contractLines -join "`n") + "`n",
        $utf8)
}

if (-not ('PartisanFocusedAutotestJob' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public sealed class PartisanFocusedAutotestJob : IDisposable
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

    public PartisanFocusedAutotestJob()
    {
        _handle = CreateJobObject(IntPtr.Zero, null);
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException("Unable to create focused-autotest job.");

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
            new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
        info.BasicLimitInformation.LimitFlags = 0x00002000;
        int size = Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
        IntPtr pointer = Marshal.AllocHGlobal(size);
        try
        {
            Marshal.StructureToPtr(info, pointer, false);
            if (!SetInformationJobObject(_handle, 9, pointer, (UInt32)size))
                throw new InvalidOperationException("Unable to configure focused-autotest job.");
        }
        catch
        {
            CloseHandle(_handle);
            _handle = IntPtr.Zero;
            throw;
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
            throw new InvalidOperationException("Unable to contain focused-autotest process.");
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
                    throw new InvalidOperationException("Unable to query focused-autotest job.");
                if (assigned > capacity || listed > capacity)
                {
                    capacity *= 2;
                    continue;
                }

                List<Int32> result = new List<Int32>();
                for (int index = 0; index < listed; index++)
                {
                    long value = Marshal.ReadIntPtr(
                        pointer,
                        8 + (index * IntPtr.Size)).ToInt64();
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

        throw new InvalidOperationException(
            "Focused-autotest job membership exceeded its safety limit.");
    }

    public void Dispose()
    {
        if (_handle == IntPtr.Zero)
            return;
        CloseHandle(_handle);
        _handle = IntPtr.Zero;
    }
}

public sealed class PartisanFocusedAutotestSuspendedProcess : IDisposable
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

    public PartisanFocusedAutotestSuspendedProcess(
        string applicationName,
        string commandLine,
        string currentDirectory)
    {
        STARTUPINFO startup = new STARTUPINFO();
        startup.cb = (UInt32)Marshal.SizeOf(typeof(STARTUPINFO));
        PROCESS_INFORMATION information;
        const UInt32 CREATE_SUSPENDED = 0x00000004;
        const UInt32 CREATE_NO_WINDOW = 0x08000000;
        if (!CreateProcessW(
            applicationName,
            new StringBuilder(commandLine),
            IntPtr.Zero,
            IntPtr.Zero,
            false,
            CREATE_SUSPENDED | CREATE_NO_WINDOW,
            IntPtr.Zero,
            currentDirectory,
            ref startup,
            out information))
        {
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to create suspended focused-autotest process.");
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
            throw new InvalidOperationException(
                "Suspended focused-autotest thread is unavailable.");
        if (ResumeThread(_threadHandle) == UInt32.MaxValue)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to resume focused-autotest process.");
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

public static class PartisanFocusedAutotestNativeCommandLine
{
    [DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern IntPtr CommandLineToArgvW(
        string commandLine,
        out int argumentCount);

    [DllImport("kernel32.dll")]
    private static extern IntPtr LocalFree(IntPtr memory);

    public static string[] Split(string commandLine)
    {
        if (commandLine == null)
            throw new ArgumentNullException("commandLine");

        int count;
        IntPtr pointer = CommandLineToArgvW(commandLine, out count);
        if (pointer == IntPtr.Zero)
            throw new InvalidOperationException(
                "Unable to parse focused-autotest native command line.");
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
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][ValidateSet('Leaf', 'Container')][string]$Kind
    )

    if (-not (Test-Path -LiteralPath $Path -PathType $Kind)) {
        throw "A required $Kind path is unavailable."
    }
    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'A required path boundary must not be a reparse point.'
    }
    return [IO.Path]::GetFullPath($item.FullName)
}

function Test-ContainedPath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Candidate,
        [switch]$AllowEqual
    )

    $rootFull = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $candidateFull = [IO.Path]::GetFullPath($Candidate).TrimEnd('\', '/')
    if ($AllowEqual -and $candidateFull.Equals(
        $rootFull,
        [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    return $candidateFull.StartsWith(
        $rootFull + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)
}

function Test-PathOverlap {
    param(
        [Parameter(Mandatory = $true)][string]$First,
        [Parameter(Mandatory = $true)][string]$Second
    )

    return (Test-ContainedPath -Root $First -Candidate $Second -AllowEqual) -or
        (Test-ContainedPath -Root $Second -Candidate $First -AllowEqual)
}

function Assert-NoReparsePathAncestry {
    param([Parameter(Mandatory = $true)][string]$Path)

    $cursor = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    while (-not (Test-Path -LiteralPath $cursor)) {
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A safe existing ancestor could not be resolved for the guarded path.'
        }
        $cursor = $parent
    }
    while (-not [string]::IsNullOrWhiteSpace($cursor)) {
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'A guarded path ancestor must not be a reparse point.'
        }
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $cursor = $parent
    }
}

function ConvertTo-NativeArgument {
    param([AllowEmptyString()][string]$Value)

    if ($null -eq $Value -or $Value.Length -eq 0) {
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
            [void]$builder.Append(('\' * ($slashes * 2 + 1)))
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

function Test-ExactNativeArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$CommandLine,
        [Parameter(Mandatory = $true)][string]$ExpectedExecutable,
        [Parameter(Mandatory = $true)][string[]]$ExpectedArguments
    )

    $tokens = @([PartisanFocusedAutotestNativeCommandLine]::Split($CommandLine))
    if ($tokens.Count -ne ($ExpectedArguments.Count + 1)) {
        return $false
    }
    if (-not ([IO.Path]::GetFullPath($tokens[0])).Equals(
        [IO.Path]::GetFullPath($ExpectedExecutable),
        [StringComparison]::OrdinalIgnoreCase)) {
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

function ConvertTo-SafeDiagnosticText {
    param(
        [AllowEmptyString()][string]$Text,
        [string[]]$RedactedPaths = @()
    )

    $safe = [string]$Text
    foreach ($path in @($RedactedPaths | Sort-Object Length -Descending)) {
        if ([string]::IsNullOrWhiteSpace([string]$path)) {
            continue
        }
        $safe = $safe.Replace([string]$path, '<local-path>')
    }
    $safe = [regex]::Replace(
        $safe,
        '(?i)(?<![A-Z0-9._%+-])[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}(?![A-Z0-9.-])',
        '<email>')
    $safe = [regex]::Replace($safe, '(?<!\d)\d{15,20}(?!\d)', '<identity>')
    $safe = [regex]::Replace(
        $safe,
        '(?i)(?:(?<![A-Z0-9_])[A-Z]:[\\/]|\\\\)[^<>\r\n|"'']+',
        '<local-path>')
    return $safe
}

function Get-EngineProcesses {
    $names = @(
        'ArmaReforger',
        'ArmaReforger_BE',
        'ArmaReforgerSteam',
        'ArmaReforgerSteamDiag',
        'ArmaReforgerServer',
        'ArmaReforgerServerDiag',
        'ArmaReforgerWorkbench',
        'ArmaReforgerWorkbenchDiag',
        'ArmaReforgerWorkbenchSteam',
        'ArmaReforgerWorkbenchSteamDiag',
        'CrashReporter',
        'CrashReportClient'
    )
    return @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
        $names -contains $_.ProcessName
    })
}

function Get-SafeSnapshotEntries {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$ExcludedRoots = @()
    )

    $rootFull = [IO.Path]::GetFullPath($Root)
    $result = New-Object `
        'Collections.Generic.Dictionary[string,object]' `
        ([StringComparer]::OrdinalIgnoreCase)
    $pending = New-Object Collections.Generic.Queue[string]
    $pending.Enqueue($rootFull)
    while ($pending.Count -gt 0) {
        $directory = $pending.Dequeue()
        foreach ($item in @(
            Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop)) {
            $itemFull = [IO.Path]::GetFullPath($item.FullName)
            if (-not (Test-ContainedPath -Root $rootFull -Candidate $itemFull)) {
                throw 'A snapshot entry escaped its explicit monitoring root.'
            }
            $excluded = $false
            foreach ($excludedRoot in $ExcludedRoots) {
                if (Test-ContainedPath `
                    -Root $excludedRoot `
                    -Candidate $itemFull `
                    -AllowEqual) {
                    $excluded = $true
                    break
                }
            }
            if ($excluded) {
                continue
            }
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw 'A monitored snapshot tree must not contain a reparse point.'
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

    $resolved = Resolve-ExistingPath -Path $Root -Kind Container
    Assert-NoReparsePathAncestry -Path $resolved
    $resolvedExclusions = New-Object Collections.Generic.List[string]
    foreach ($excludedRoot in $ExcludedRoots) {
        if ([string]::IsNullOrWhiteSpace([string]$excludedRoot)) {
            continue
        }
        $resolvedExclusion = [IO.Path]::GetFullPath($excludedRoot)
        if ($resolvedExclusion.Equals(
            $resolved,
            [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A monitoring root must not be fully excluded.'
        }
        if (-not $resolvedExclusions.Contains($resolvedExclusion)) {
            $resolvedExclusions.Add($resolvedExclusion)
        }
    }
    return [pscustomobject]@{
        Root = $resolved
        ExcludedRoots = $resolvedExclusions.ToArray()
        Entries = Get-SafeSnapshotEntries `
            -Root $resolved `
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
        throw 'A monitoring root became a reparse point.'
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

function Update-OwnedProcessIds {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Owned,
        $Job
    )

    if (-not $Job) {
        return
    }
    foreach ($jobId in @($Job.GetProcessIds())) {
        $Owned[[int]$jobId] = $true
    }
}

function Update-UnclaimedEngineProcesses {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Owned,
        [Parameter(Mandatory = $true)]$Observed
    )

    foreach ($candidate in @(Get-EngineProcesses)) {
        if ($Owned.ContainsKey([int]$candidate.Id)) {
            continue
        }
        $identity = $candidate.ProcessName + ':' + $candidate.Id
        try {
            $identity += ':' + $candidate.StartTime.ToUniversalTime().Ticks
        }
        catch { }
        [void]$Observed.Add($identity)
    }
}

function Get-JUnitEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$ExpectedTestCase
    )

    [xml]$document = Get-Content -LiteralPath $Path -Raw
    $root = $document.DocumentElement
    if (-not $root -or
        ($root.LocalName -cne 'testsuites' -and
            $root.LocalName -cne 'testsuite')) {
        throw 'Focused autotest JUnit root is unavailable.'
    }
    $tests = 0
    $failures = 0
    $errors = 0
    $skipped = 0
    $rootTests = [string]$root.GetAttribute('tests')
    $rootFailures = [string]$root.GetAttribute('failures')
    $rootErrors = [string]$root.GetAttribute('errors')
    $rootSkipped = [string]$root.GetAttribute('skipped')
    if (-not [string]::IsNullOrEmpty($rootTests)) {
        $tests = [int]$rootTests
        if (-not [string]::IsNullOrEmpty($rootFailures)) {
            $failures = [int]$rootFailures
        }
        if (-not [string]::IsNullOrEmpty($rootErrors)) {
            $errors = [int]$rootErrors
        }
        if (-not [string]::IsNullOrEmpty($rootSkipped)) {
            $skipped = [int]$rootSkipped
        }
    }
    else {
        $suiteNodes = @()
        if ($root.LocalName -ceq 'testsuite') {
            $suiteNodes = @($root)
        }
        else {
            $suiteNodes = @($root.SelectNodes('./testsuite'))
        }
        foreach ($suite in $suiteNodes) {
            $suiteTests = [string]$suite.GetAttribute('tests')
            $suiteFailures = [string]$suite.GetAttribute('failures')
            $suiteErrors = [string]$suite.GetAttribute('errors')
            $suiteSkipped = [string]$suite.GetAttribute('skipped')
            if (-not [string]::IsNullOrEmpty($suiteTests)) {
                $tests += [int]$suiteTests
            }
            if (-not [string]::IsNullOrEmpty($suiteFailures)) {
                $failures += [int]$suiteFailures
            }
            if (-not [string]::IsNullOrEmpty($suiteErrors)) {
                $errors += [int]$suiteErrors
            }
            if (-not [string]::IsNullOrEmpty($suiteSkipped)) {
                $skipped += [int]$suiteSkipped
            }
        }
    }
    $testCases = @($document.SelectNodes('//testcase'))
    $expectedCases = @(Get-FocusedExpectedTestCases `
        -Selector $ExpectedTestCase)
    $expectedSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($expectedCase in $expectedCases) {
        if (-not $expectedSet.Add([string]$expectedCase)) {
            throw 'Focused autotest expected-case manifest contains a duplicate.'
        }
    }
    $actualSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $caseName = if ($expectedCases.Count -eq 1) {
        [string]$expectedCases[0]
    }
    else {
        $ExpectedTestCase
    }
    $caseClassName = ''
    $caseFailures = 0
    $caseErrors = 0
    $caseSkipped = 0
    $caseFailureText = ''
    $caseErrorText = ''
    $caseIdentityExact = $testCases.Count -eq $expectedCases.Count
    foreach ($testCase in $testCases) {
        $actualName = [string]$testCase.GetAttribute('name')
        $actualClassName = [string]$testCase.GetAttribute('classname')
        if ([string]::IsNullOrEmpty($actualClassName)) {
            $actualClassName = [string]$testCase.GetAttribute('class')
        }
        if (-not $actualSet.Add($actualName) -or
            -not $expectedSet.Contains($actualName)) {
            $caseIdentityExact = $false
        }
        if ($expectedCases.Count -gt 1 -and
            $actualClassName -cne $ExpectedTestCase) {
            $caseIdentityExact = $false
        }
        if ($expectedCases.Count -eq 1 -and
            $actualName -cne $ExpectedTestCase -and
            $actualClassName -cne $ExpectedTestCase) {
            $caseIdentityExact = $false
        }
        $failureNodes = @($testCase.SelectNodes('./failure'))
        $errorNodes = @($testCase.SelectNodes('./error'))
        $caseFailures += $failureNodes.Count
        $caseErrors += $errorNodes.Count
        $caseSkipped += @($testCase.SelectNodes('./skipped')).Count
        if ($failureNodes.Count -gt 0 -and
            [string]::IsNullOrEmpty($caseFailureText)) {
            $caseFailureText = [string]$failureNodes[0].GetAttribute('message')
            if ([string]::IsNullOrWhiteSpace($caseFailureText)) {
                $caseFailureText = [string]$failureNodes[0].InnerText
            }
        }
        if ($errorNodes.Count -gt 0 -and
            [string]::IsNullOrEmpty($caseErrorText)) {
            $caseErrorText = [string]$errorNodes[0].GetAttribute('message')
            if ([string]::IsNullOrWhiteSpace($caseErrorText)) {
                $caseErrorText = [string]$errorNodes[0].InnerText
            }
        }
    }
    if ($actualSet.Count -ne $expectedSet.Count) {
        $caseIdentityExact = $false
    }
    foreach ($expectedCase in $expectedCases) {
        if (-not $actualSet.Contains([string]$expectedCase)) {
            $caseIdentityExact = $false
        }
    }
    if ($tests -ne $testCases.Count -or
        $failures -ne $caseFailures -or
        $errors -ne $caseErrors -or
        $skipped -ne $caseSkipped) {
        $caseIdentityExact = $false
    }
    if ($expectedCases.Count -gt 1) {
        $caseClassName = $ExpectedTestCase
    }
    elseif ($testCases.Count -eq 1) {
        $caseClassName = [string]$testCases[0].GetAttribute('classname')
        if ([string]::IsNullOrEmpty($caseClassName)) {
            $caseClassName = [string]$testCases[0].GetAttribute('class')
        }
    }
    return [pscustomobject]@{
        Tests = $tests
        Failures = $failures
        Errors = $errors
        TestCaseCount = $testCases.Count
        CaseName = $caseName
        CaseClassName = $caseClassName
        CaseIdentityExact = $caseIdentityExact
        CaseFailures = $caseFailures
        CaseErrors = $caseErrors
        CaseSkipped = $caseSkipped
        CaseFailureText = $caseFailureText
        CaseErrorText = $caseErrorText
    }
}

function Get-FocusedHardDiagnosticCensus {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$ConsoleText,

        [Parameter(Mandatory = $true)]
        [string]$ExpectedTestCase
    )

    $lines = @($ConsoleText -split "`r?`n")
    $expectedCases = @(Get-FocusedExpectedTestCases `
        -Selector $ExpectedTestCase)
    $expectedCaseSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($expectedCase in $expectedCases) {
        [void]$expectedCaseSet.Add([string]$expectedCase)
    }
    $successfulCaseSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $successfulCaseRows = New-Object Collections.Generic.List[object]
    $profileNonMutatingTokenIndices = New-Object Collections.Generic.List[int]
    $profileExactSeamTokenIndices = New-Object Collections.Generic.List[int]
    $successfulCaseEventCount = 0
    $suiteStartedIndex = -1
    $testSuccessIndex = -1
    $runnerFinishedIndex = -1
    $junitSavedIndex = -1
    $failedListSavedIndex = -1
    $profileNonMutatingTokenIndex = -1
    $profileExactSeamTokenIndex = -1
    $profileNonMutatingTokenCount = 0
    $profileExactSeamTokenCount = 0
	$suiteStartedCount = 0
	$allSuiteStartedCount = 0
	$allTestSuccessCount = 0
	$runnerFinishedCount = 0
	$junitSavedCount = 0
	$failedListSavedCount = 0
	$expectedSuite = if ($script:FocusedSuiteTestCases.Contains(
			$ExpectedTestCase)) {
		$ExpectedTestCase
	}
	else {
		''
	}
	$timestampedScriptPrefix =
		'^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+'
	$allSuiteStartedPattern = $timestampedScriptPrefix +
		'TestSuite #(?<suite>[^\r\n]+) started\s*$'
	$allTestSuccessPattern = $timestampedScriptPrefix +
		'(?:✅\s+)?(?<case>HST_TEST_[A-Za-z0-9_]+): SUCCESS\s*$'
	$runnerFinishedPattern = $timestampedScriptPrefix +
		'SCR_TestRunner has finished running\s*$'
	$junitSavedPattern =
		'^\s*(?:\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+)?' +
		'Autotest JUnit XML saved to:(?:\s+.*)?$'
	$failedListSavedPattern =
		'^\s*(?:\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+)?' +
		'Autotest failed list saved to:(?:\s+.*)?$'
    $profileNonMutatingPattern =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+' +
        '(?:.* \| )?failed native callback non-mutating 1(?: \| .*)?$'
    $profileExactSeamPattern =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+' +
        'setup/seam/request/bytes/journal 1/1/1/1/1(?: \| .*)?$'
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
		$suiteMatch = [regex]::Match($line, $allSuiteStartedPattern)
		if ($suiteMatch.Success) {
			$allSuiteStartedCount++
			$actualSuite = [string]$suiteMatch.Groups['suite'].Value
			if ([string]::IsNullOrEmpty($expectedSuite) -or
				$actualSuite -ceq $expectedSuite) {
				$suiteStartedIndex = $index
				$suiteStartedCount++
			}
        }
		$successMatch = [regex]::Match($line, $allTestSuccessPattern)
		if ($successMatch.Success) {
			$allTestSuccessCount++
			$successfulCaseName =
				[string]$successMatch.Groups['case'].Value
			if ($expectedCaseSet.Contains($successfulCaseName)) {
				$successfulCaseEventCount++
				[void]$successfulCaseSet.Add($successfulCaseName)
				[void]$successfulCaseRows.Add(
					[pscustomobject][ordered]@{
						Name = $successfulCaseName
						Index = $index
					})
				$testSuccessIndex = $index
			}
        }
		if ($line -cmatch $runnerFinishedPattern) {
            $runnerFinishedIndex = $index
			$runnerFinishedCount++
        }
		if ($line -cmatch $junitSavedPattern) {
            $junitSavedIndex = $index
			$junitSavedCount++
        }
		if ($line -cmatch $failedListSavedPattern) {
            $failedListSavedIndex = $index
			$failedListSavedCount++
        }
        if ($line -cmatch $profileNonMutatingPattern) {
            $profileNonMutatingTokenIndex = $index
            $profileNonMutatingTokenCount++
            [void]$profileNonMutatingTokenIndices.Add($index)
        }
        if ($line -cmatch $profileExactSeamPattern) {
            $profileExactSeamTokenIndex = $index
            $profileExactSeamTokenCount++
            [void]$profileExactSeamTokenIndices.Add($index)
        }
    }

    $profileJournalCase = $ExpectedTestCase -ceq
            'HST_TEST_CampaignProfileJournalAuthority' -or
        $ExpectedTestCase -ceq
            'HST_CampaignProfileJournalAuthorityAutotestSuite'
    $expectedIntentionalFaultCount = if ($profileJournalCase) {
        $expectedCases.Count
    }
    else {
        0
    }
    $hardPattern = '\b(?:SCRIPT|ENGINE)\s+\(E\):'
    $stockFilterPattern =
        "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): " +
        "Can't instantiate class 'SCR_FilterCategory', constructor is not public\s*$"
    $intentionalNativeFailurePattern =
        "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): " +
        "string failureDetail = 'Partisan persistence \| native save " +
        'callback failure \| sequence/type/flags 1/0/0 \| ' +
        'manager/enabled/allowed/busy/active/playthrough 1/1/1/0/0/0 \| ' +
        'types/persistence/state/loaded/tracked/config/staged ' +
        '5/1/2/0/0/0/1 \| replication mode 0 \| snapshot fingerprint ''\s*$'
    $intentionalIndices = New-Object Collections.Generic.List[int]
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ([string]$lines[$index] -cmatch
            $intentionalNativeFailurePattern) {
            [void]$intentionalIndices.Add($index)
        }
    }
    $approvedIntentionalIndices = New-Object `
        'Collections.Generic.HashSet[int]'
    $profileProofTokensSeen = -not $profileJournalCase -and
        $profileNonMutatingTokenCount -eq 0 -and
        $profileExactSeamTokenCount -eq 0 -and
        $intentionalIndices.Count -eq 0
    if ($profileJournalCase -and
        $successfulCaseRows.Count -eq $expectedCases.Count -and
        $profileNonMutatingTokenCount -eq $expectedCases.Count -and
        $profileExactSeamTokenCount -eq $expectedCases.Count -and
        $intentionalIndices.Count -eq $expectedCases.Count) {
        $profileProofTokensSeen = $true
        $intervalFloor = $suiteStartedIndex
        foreach ($successRow in $successfulCaseRows) {
            $intervalCeiling = [int]$successRow.Index
            $intervalIntentional = @($intentionalIndices | Where-Object {
                $_ -gt $intervalFloor -and $_ -lt $intervalCeiling
            })
            $intervalNonMutating = @(
                $profileNonMutatingTokenIndices | Where-Object {
                    $_ -gt $intervalFloor -and $_ -lt $intervalCeiling
                })
            $intervalExactSeam = @(
                $profileExactSeamTokenIndices | Where-Object {
                    $_ -gt $intervalFloor -and $_ -lt $intervalCeiling
                })
            if ($intervalIntentional.Count -ne 1 -or
                $intervalNonMutating.Count -ne 1 -or
                $intervalExactSeam.Count -ne 1 -or
                $intervalIntentional[0] -ge $intervalNonMutating[0] -or
                $intervalIntentional[0] -ge $intervalExactSeam[0]) {
                $profileProofTokensSeen = $false
                break
            }
            [void]$approvedIntentionalIndices.Add(
                [int]$intervalIntentional[0])
            $intervalFloor = $intervalCeiling
        }
    }
    $approvedStockFilterCount = 0
    $approvedIntentionalFaultCount = 0
    $hardDiagnosticCount = 0
    $unapproved = New-Object Collections.Generic.List[string]
    $completionFloor = [math]::Max(
        $runnerFinishedIndex,
        [math]::Max($junitSavedIndex, $failedListSavedIndex))

    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
        if ($line -cnotmatch $hardPattern) {
            continue
        }
        $hardDiagnosticCount++
        if ($line -cmatch $stockFilterPattern) {
            if ($completionFloor -ge 0 -and
                $index -gt $completionFloor -and
                $approvedStockFilterCount -lt 2) {
                $approvedStockFilterCount++
            }
            else {
                [void]$unapproved.Add($line)
            }
            continue
        }
        if ($line -cmatch $intentionalNativeFailurePattern) {
            if ($profileJournalCase -and $profileProofTokensSeen -and
                $approvedIntentionalIndices.Contains($index)) {
                $approvedIntentionalFaultCount++
            }
            else {
                [void]$unapproved.Add($line)
            }
            continue
        }
        [void]$unapproved.Add($line)
    }

	$markerOrderExact = $suiteStartedCount -eq 1 -and
		$allSuiteStartedCount -eq 1 -and
		$suiteStartedIndex -ge 0 -and
        $testSuccessIndex -gt $suiteStartedIndex -and
        $successfulCaseEventCount -eq $expectedCases.Count -and
        $successfulCaseSet.Count -eq $expectedCaseSet.Count -and
		$allTestSuccessCount -eq $expectedCases.Count -and
		$runnerFinishedCount -eq 1 -and
        $runnerFinishedIndex -gt $testSuccessIndex -and
		$junitSavedCount -eq 1 -and
        $junitSavedIndex -gt $runnerFinishedIndex -and
		$failedListSavedCount -eq 1 -and
        $failedListSavedIndex -gt $junitSavedIndex
    $valid = $markerOrderExact -and
        $testSuccessIndex -ge 0 -and
        $approvedStockFilterCount -eq 2 -and
        $approvedIntentionalFaultCount -eq $expectedIntentionalFaultCount -and
        $unapproved.Count -eq 0
    return [pscustomobject][ordered]@{
        Valid = $valid
        HardDiagnosticFree = $hardDiagnosticCount -eq 0
        HardDiagnosticCount = $hardDiagnosticCount
        ApprovedStockFilterCount = $approvedStockFilterCount
        ApprovedIntentionalFaultCount = $approvedIntentionalFaultCount
        UnapprovedHardDiagnosticCount = $unapproved.Count
        UnapprovedHardDiagnosticLines = $unapproved.ToArray()
        MarkerOrderExact = $markerOrderExact
        ProfileProofTokensSeen = $profileProofTokensSeen
    }
}

function Test-FocusedHardDiagnosticCensus {
    $standardCase = 'HST_TEST_EnemyCounterattackAuthority'
    $profileCase = 'HST_TEST_CampaignProfileJournalAuthority'
    $standardPrefix = @(
        '17:00:00.000 SCRIPT       : TestSuite #Example started',
        ('17:00:00.001 SCRIPT       : ' + $standardCase + ': SUCCESS'),
        '17:00:00.002 SCRIPT       : SCR_TestRunner has finished running',
        '17:00:00.003 SCRIPT       : Autotest JUnit XML saved to: $logs:/junit.xml',
        '17:00:00.004 SCRIPT       : Autotest failed list saved to: $logs:/autotest_failed.log'
    )
    $stockLine =
        "17:00:00.005 SCRIPT    (E): Can't instantiate class " +
        "'SCR_FilterCategory', constructor is not public"
    $standardValidText = ($standardPrefix + @($stockLine, $stockLine)) -join "`n"
    $standardValid = Get-FocusedHardDiagnosticCensus `
        -ConsoleText $standardValidText `
        -ExpectedTestCase $standardCase
    if (-not $standardValid.Valid -or
        $standardValid.HardDiagnosticFree -or
        $standardValid.HardDiagnosticCount -ne 2 -or
        $standardValid.ApprovedStockFilterCount -ne 2 -or
        $standardValid.UnapprovedHardDiagnosticCount -ne 0) {
        throw 'Focused hard-diagnostic stock classification self-test failed.'
    }

    $profileText = @(
        '17:00:00.000 SCRIPT       : TestSuite #Profile started',
        "17:00:00.001 SCRIPT    (E): string failureDetail = 'Partisan persistence | native save callback failure | sequence/type/flags 1/0/0 | manager/enabled/allowed/busy/active/playthrough 1/1/1/0/0/0 | types/persistence/state/loaded/tracked/config/staged 5/1/2/0/0/0/1 | replication mode 0 | snapshot fingerprint '",
        '17:00:00.002 SCRIPT       : setup/seam/request/bytes/journal 1/1/1/1/1 | exact',
        '17:00:00.003 SCRIPT       : exact | failed native callback non-mutating 1 | exact',
        ('17:00:00.004 SCRIPT       : ' + $profileCase + ': SUCCESS'),
        '17:00:00.005 SCRIPT       : SCR_TestRunner has finished running',
        '17:00:00.006 SCRIPT       : Autotest JUnit XML saved to: $logs:/junit.xml',
        '17:00:00.007 SCRIPT       : Autotest failed list saved to: $logs:/autotest_failed.log',
        $stockLine,
        $stockLine
    ) -join "`n"
    $profileValid = Get-FocusedHardDiagnosticCensus `
        -ConsoleText $profileText `
        -ExpectedTestCase $profileCase
    if (-not $profileValid.Valid -or
        $profileValid.HardDiagnosticCount -ne 3 -or
        $profileValid.ApprovedIntentionalFaultCount -ne 1) {
        throw 'Focused hard-diagnostic intentional-fault classification self-test failed.'
    }

    $unknown = Get-FocusedHardDiagnosticCensus `
        -ConsoleText ($standardValidText + "`n17:00:00.006 ENGINE (E): unknown") `
        -ExpectedTestCase $standardCase
    if ($unknown.Valid -or $unknown.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Focused hard-diagnostic unknown-error rejection self-test failed.'
    }
    $thirdStock = Get-FocusedHardDiagnosticCensus `
        -ConsoleText ($standardValidText + "`n" + $stockLine) `
        -ExpectedTestCase $standardCase
    if ($thirdStock.Valid -or $thirdStock.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Focused hard-diagnostic third-stock-error rejection self-test failed.'
    }
    $wrongOrder = Get-FocusedHardDiagnosticCensus `
        -ConsoleText (($standardPrefix[0], $stockLine, $stockLine) +
            $standardPrefix[1..4] -join "`n") `
        -ExpectedTestCase $standardCase
    if ($wrongOrder.Valid -or $wrongOrder.UnapprovedHardDiagnosticCount -ne 2) {
        throw 'Focused hard-diagnostic ordering rejection self-test failed.'
    }
    $wrongCase = Get-FocusedHardDiagnosticCensus `
        -ConsoleText $profileText `
        -ExpectedTestCase $standardCase
    if ($wrongCase.Valid -or $wrongCase.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Focused hard-diagnostic testcase rejection self-test failed.'
    }
    $missingProfileProof = Get-FocusedHardDiagnosticCensus `
        -ConsoleText ($profileText.Replace(
            'failed native callback non-mutating 1',
            'failed native callback non-mutating 0')) `
        -ExpectedTestCase $profileCase
    if ($missingProfileProof.Valid -or
        $missingProfileProof.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Focused hard-diagnostic proof-token rejection self-test failed.'
    }
    $missingStock = Get-FocusedHardDiagnosticCensus `
        -ConsoleText ($standardPrefix -join "`n") `
        -ExpectedTestCase $standardCase
    if ($missingStock.Valid -or
        $missingStock.UnapprovedHardDiagnosticCount -ne 0) {
        throw 'Focused hard-diagnostic exact-stock-count self-test failed.'
    }
    $suffixMutation = Get-FocusedHardDiagnosticCensus `
        -ConsoleText ($profileText.Replace(
            "snapshot fingerprint '",
            "snapshot fingerprint ' unexpected")) `
        -ExpectedTestCase $profileCase
    if ($suffixMutation.Valid -or
        $suffixMutation.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Focused hard-diagnostic suffix-mutation rejection self-test failed.'
    }
    $tokenTen = Get-FocusedHardDiagnosticCensus `
        -ConsoleText ($profileText.Replace(
            'failed native callback non-mutating 1',
            'failed native callback non-mutating 10')) `
        -ExpectedTestCase $profileCase
    if ($tokenTen.Valid -or $tokenTen.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Focused hard-diagnostic token-boundary rejection self-test failed.'
    }
    $tokensAfterSuccess = Get-FocusedHardDiagnosticCensus `
        -ConsoleText (@(
            $profileText -split "`n" | Where-Object {
                $_ -notmatch 'setup/seam/request/bytes/journal|failed native callback non-mutating'
            }
        ) + @(
            '17:00:00.008 SCRIPT       : setup/seam/request/bytes/journal 1/1/1/1/1 | exact',
            '17:00:00.009 SCRIPT       : exact | failed native callback non-mutating 1 | exact'
        ) -join "`n") `
        -ExpectedTestCase $profileCase
    if ($tokensAfterSuccess.Valid -or
        $tokensAfterSuccess.UnapprovedHardDiagnosticCount -ne 1) {
        throw 'Focused hard-diagnostic proof-order rejection self-test failed.'
    }
    $successAfterCompletionText = @(
        '17:00:00.000 SCRIPT       : TestSuite #Example started',
        '17:00:00.001 SCRIPT       : SCR_TestRunner has finished running',
        '17:00:00.002 SCRIPT       : Autotest JUnit XML saved to: $logs:/junit.xml',
        '17:00:00.003 SCRIPT       : Autotest failed list saved to: $logs:/autotest_failed.log',
        ('17:00:00.004 SCRIPT       : ' + $standardCase + ': SUCCESS'),
        $stockLine,
        $stockLine
    ) -join "`n"
    $successAfterCompletion = Get-FocusedHardDiagnosticCensus `
        -ConsoleText $successAfterCompletionText `
        -ExpectedTestCase $standardCase
    if ($successAfterCompletion.Valid) {
        throw 'Focused hard-diagnostic completion-order rejection self-test failed.'
    }
    return 12
}

function Test-FocusedSuiteSelectorContract {
    $expectedCounts = [ordered]@{
        HST_EnemyCounterattackAutotestSuite = 14
        HST_EnemyGarrisonRebuildAutotestSuite = 13
        HST_EnemyPlanningCommitmentAutotestSuite = 17
        HST_EnemyQRFAutotestSuite = 6
        HST_CampaignProfileJournalAuthorityAutotestSuite = 41
    }
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $total = 0
    foreach ($suite in $expectedCounts.Keys) {
        $cases = @(Get-FocusedExpectedTestCases -Selector $suite)
        if ($cases.Count -ne [int]$expectedCounts[$suite]) {
            throw 'Focused suite manifest count self-test failed.'
        }
        foreach ($caseName in $cases) {
            if ([string]$caseName -cnotmatch
                    '^HST_TEST_[A-Za-z0-9_]+$' -or
                -not $seen.Add([string]$caseName)) {
                throw 'Focused suite manifest identity self-test failed.'
            }
        }
        $total += $cases.Count
    }
    if ($total -ne 91 -or $seen.Count -ne 91) {
        throw 'Focused suite manifest total self-test failed.'
    }
    $individual = 'HST_TEST_EnemyQRF_Admission'
    $individualCases = @(Get-FocusedExpectedTestCases `
        -Selector $individual)
    if ($individualCases.Count -ne 1 -or
        [string]$individualCases[0] -cne $individual) {
        throw 'Focused individual selector self-test failed.'
    }

	$syntheticRoot = Join-Path ([IO.Path]::GetTempPath()) 'Partisan Test'
	$executable = Join-Path $syntheticRoot 'ArmaReforgerSteamDiag.exe'
    $suiteSelector = 'HST_CampaignProfileJournalAuthorityAutotestSuite'
    $arguments = @(
		'-profile', (Join-Path $syntheticRoot 'profile'),
        '-autotest', $suiteSelector,
        '-noThrow'
    )
    $commandLine = (ConvertTo-NativeArgument -Value $executable) + ' ' +
        (($arguments | ForEach-Object {
            ConvertTo-NativeArgument -Value ([string]$_)
        }) -join ' ')
    if (-not (Test-ExactNativeArgumentVector `
            -CommandLine $commandLine `
            -ExpectedExecutable $executable `
            -ExpectedArguments $arguments)) {
        throw 'Focused suite selector native argument self-test failed.'
    }
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

function Get-HarnessBinding {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$CandidateModulePath
    )

    $headLines = @(& git -C $CheckoutRoot rev-parse HEAD)
    $headExit = $LASTEXITCODE
    $statusLines = @(& git -C $CheckoutRoot status --porcelain=v1 --untracked-files=all)
    $statusExit = $LASTEXITCODE
    $head = ($headLines -join '').Trim()
    if ($headExit -ne 0 -or $statusExit -ne 0 -or
        $head -cnotmatch '^[0-9a-f]{40}$') {
        throw 'The focused-autotest harness Git identity could not be resolved.'
    }

    return [pscustomobject][ordered]@{
        GitHead = $head
        StatusText = $statusLines -join "`n"
        Clean = $statusLines.Count -eq 0
        RunnerSha256 = (Get-FileHash `
            -LiteralPath $RunnerPath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        CandidateModuleSha256 = (Get-FileHash `
            -LiteralPath $CandidateModulePath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Assert-HarnessBinding {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$CandidateModulePath
    )

    $actual = Get-HarnessBinding `
        -CheckoutRoot $CheckoutRoot `
        -RunnerPath $RunnerPath `
        -CandidateModulePath $CandidateModulePath
    if ($actual.GitHead -cne $Expected.GitHead -or
        $actual.StatusText -cne $Expected.StatusText -or
        $actual.Clean -ne $Expected.Clean -or
        $actual.RunnerSha256 -cne $Expected.RunnerSha256 -or
        $actual.CandidateModuleSha256 -cne $Expected.CandidateModuleSha256) {
        throw 'The focused-autotest harness identity changed during execution.'
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
        "gproj:\s+'(?<path>[^']+)'\s+guid:\s+'(?<guid>[^']+)'" +
        "\s*(?<mode>\([^)]+\))?\s*$"
    $recordCount = 0
    $exactPathCount = 0
    $packedCount = 0
    $invalidModeCount = 0
    $guidExactCount = 0
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
        $consoleText = [IO.File]::ReadAllText($consoleFile.FullName)
        foreach ($match in @([regex]::Matches($consoleText, $pattern))) {
            $recordCount++
            $recordedProject = $match.Groups['path'].Value.Replace('\', '/')
            if ($recordedProject.Equals(
                    $expectedProject,
                    [StringComparison]::OrdinalIgnoreCase)) {
                $exactPathCount++
            }
            if ($match.Groups['guid'].Value.Equals(
                    $AddonGuid,
                    [StringComparison]::Ordinal)) {
                $guidExactCount++
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
            $invalidModeCount -eq 0 -and
            $guidExactCount -eq $recordCount
        RecordCount = $recordCount
        ExactPathCount = $exactPathCount
        PackedCount = $packedCount
        InvalidModeCount = $invalidModeCount
        GuidExact = $recordCount -gt 0 -and $guidExactCount -eq $recordCount
        Packed = $packedCount -gt 0 -and $invalidModeCount -eq 0
    }
}

function Copy-FocusedAutotestEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$EvidenceRunRoot,
        [Parameter(Mandatory = $true)]$Candidate
    )

    Assert-NoReparsePathAncestry -Path $GuardRoot
    Assert-NoReparsePathAncestry -Path $EvidenceRunRoot
    $guardFull = [IO.Path]::GetFullPath($GuardRoot).TrimEnd('\', '/')
    $guardPrefix = $guardFull + [IO.Path]::DirectorySeparatorChar
    $rawRoot = Join-Path $EvidenceRunRoot 'raw'
    $identityRoot = Join-Path $EvidenceRunRoot 'identity'
    New-Item -ItemType Directory -Path $rawRoot, $identityRoot -Force | Out-Null
    foreach ($file in @(Get-ChildItem `
            -LiteralPath $guardFull `
            -Recurse `
            -File `
            -Force `
            -ErrorAction Stop | Where-Object {
                $_.Extension -cin @('.log', '.rpt', '.xml') -or
                $_.Name -ceq 'autotest_failed.log'
            })) {
        if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Focused-autotest evidence must not contain a reparse point.'
        }
        $full = [IO.Path]::GetFullPath($file.FullName)
        if (-not $full.StartsWith($guardPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Focused-autotest evidence escaped its guarded root.'
        }
        $relative = $full.Substring($guardPrefix.Length)
        $destination = Join-Path $rawRoot $relative
        New-Item `
            -ItemType Directory `
            -Path (Split-Path -Parent $destination) `
            -Force | Out-Null
        Copy-Item `
            -LiteralPath $full `
            -Destination $destination `
            -Force `
            -ErrorAction Stop
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
            throw 'A focused evidence file escaped its retained root.'
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

$normalizedRequiredLogPatterns = Get-NormalizedRequiredLogPatterns `
    -Patterns $RequiredLogPatterns
$hardDiagnosticClassifierChecks = Test-FocusedHardDiagnosticCensus
Test-FocusedSuiteSelectorContract
$candidateModulePath = Join-Path $PSScriptRoot 'Partisan.ReleaseCandidate.psm1'
Import-Module -Name $candidateModulePath -Force -ErrorAction Stop
$executablePath = Resolve-ExistingPath -Path $Executable -Kind Leaf
$consumerIntent = if ($PreflightOnly) {
    'verification'
}
else {
    'runtime'
}
if ((Split-Path -Leaf $executablePath) -ine 'ArmaReforgerSteamDiag.exe') {
    throw 'Focused autotest requires ArmaReforgerSteamDiag.exe.'
}
$candidateBinding = Assert-PartisanReleaseCandidate `
    -ManifestPath $CandidateManifest `
    -BundleRoot $CandidateBundleRoot `
    -RuntimeAddonRoot $RuntimeAddonRoot `
    -Executable $executablePath `
    -RuntimeRole client `
    -ConsumerIntent $consumerIntent
$candidateIdentity = Get-PartisanPublicCandidateIdentity -Candidate $candidateBinding
$expectedBuildSummary = 'sha ' + $candidateBinding.EmbeddedBuildSha +
    ' | utc ' + $candidateBinding.EmbeddedBuildUtc +
    ' | label ' + $candidateBinding.EmbeddedBuildLabel
$projectFile = $candidateBinding.PackedProjectPath
$projectDirectory = Split-Path -Parent $projectFile
$addonDirectory = $candidateBinding.RuntimeAddonRootPath
$addonSearchPath = $candidateBinding.AddonSearchPath

$normalizedDefaultRoots = @($DefaultRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$normalizedSpillRoots = @($SpillRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
if (-not $PreflightOnly -and
    ($normalizedDefaultRoots.Count -eq 0 -or $normalizedSpillRoots.Count -eq 0)) {
    throw 'Focused autotest requires explicit default-root and spill-root monitoring.'
}
$checkoutRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$harnessBinding = Get-HarnessBinding `
    -CheckoutRoot $checkoutRoot `
    -RunnerPath $PSCommandPath `
    -CandidateModulePath $candidateModulePath
$evidenceOutputPath = $null
if (-not $PreflightOnly) {
    if (-not $harnessBinding.Clean) {
        throw 'A real candidate focused run requires a clean, committed harness checkout.'
    }
    if ([string]::IsNullOrWhiteSpace($EvidenceOutputRoot)) {
        throw 'A real candidate focused run requires an external evidence output root.'
    }
    $evidenceOutputPath = [IO.Path]::GetFullPath($EvidenceOutputRoot)
    foreach ($protectedRoot in @(
            $checkoutRoot,
            $candidateBinding.BundleRootPath,
            $candidateBinding.RuntimeAddonRootPath)) {
        if (Test-PathOverlap -First $evidenceOutputPath -Second $protectedRoot) {
            throw 'The focused evidence root must not overlap a protected runtime root.'
        }
    }
    foreach ($defaultRoot in $normalizedDefaultRoots) {
        if (Test-PathOverlap -First $evidenceOutputPath -Second $defaultRoot) {
            throw 'The focused evidence root must not overlap a monitored default root.'
        }
    }
    foreach ($spillRoot in $normalizedSpillRoots) {
        if (Test-ContainedPath `
                -Root $evidenceOutputPath `
                -Candidate $spillRoot `
                -AllowEqual) {
            throw 'The focused evidence root must not contain a spill-monitoring root.'
        }
    }
    Assert-NoReparsePathAncestry -Path $evidenceOutputPath
    New-Item -ItemType Directory -Path $evidenceOutputPath -Force | Out-Null
    Assert-NoReparsePathAncestry -Path $evidenceOutputPath
}
$diagnosticRedactedPaths = @(
    $executablePath,
    $projectFile,
    $projectDirectory,
    $addonDirectory,
    $candidateBinding.PackageParentPath,
    $candidateBinding.PackedAddonPath
) + $normalizedDefaultRoots + $normalizedSpillRoots

$guardBase = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) 'PartisanFocusedAutotest'))
$guardNonce = [Guid]::NewGuid().ToString('N')
$guardRoot = [IO.Path]::GetFullPath(
    (Join-Path $guardBase $guardNonce))
if (-not (Test-ContainedPath -Root $guardBase -Candidate $guardRoot)) {
    throw 'Focused-autotest guard containment failed.'
}
$sentinelPath = Join-Path $guardRoot '.partisan-focused-owner'

$mutex = $null
$mutexAcquired = $false
$process = $null
$job = $null
$suspendedLauncher = $null
$ownedProcessIds = @{}
$unclaimedProcessesObserved = New-Object Collections.Generic.HashSet[string]
$oldTemp = $env:TEMP
$oldTmp = $env:TMP
$defaultSnapshots = New-Object Collections.Generic.List[object]
$spillSnapshots = New-Object Collections.Generic.List[object]
$runError = $null
$result = $null
$candidateStage = $null
$candidateBoundaryVerified = $false
$mountAttestation = $null
$evidenceRunRoot = $null
$evidenceState = [pscustomobject]@{ Captured = $false }
$evidenceStartUtc = [DateTime]::UtcNow
$diagnosticTail = @()
$cleanup = [ordered]@{
    GuardRemaining = -1
    GuardBaseRemaining = -1
    EngineProcessesRemaining = -1
    OwnedProcessesRemaining = -1
    UnclaimedEngineProcessesObserved = -1
    NewDefaultEntriesRemaining = -1
    ModifiedDefaultFiles = -1
    DeletedDefaultEntries = -1
    MissingDefaultRoots = -1
    ExternalSpillEntriesRemaining = -1
    ModifiedSpillFiles = -1
    DeletedSpillEntries = -1
    MissingSpillRoots = -1
    MonitoringRootsAreDetectionOnly = $true
    CleanupErrors = @()
}

try {
    $mutex = New-Object Threading.Mutex($false, 'Local\PartisanFocusedAutotestGuard')
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw 'Another guarded focused autotest is already active.'
    }
    if (@(Get-EngineProcesses).Count -ne 0) {
        throw 'Refusing focused autotest while an engine process is already running.'
    }

    if (-not $PreflightOnly) {
        $candidateEvidenceRoot = Join-Path `
            $evidenceOutputPath `
            $candidateBinding.CandidateId
        $focusedEvidenceRoot = Join-Path $candidateEvidenceRoot 'focused-autotest'
        $caseEvidenceRoot = Join-Path $focusedEvidenceRoot $TestCase
        New-Item `
            -ItemType Directory `
            -Path $candidateEvidenceRoot, $focusedEvidenceRoot, $caseEvidenceRoot `
            -Force | Out-Null
        $evidenceLeaf = ([DateTime]::UtcNow.ToString(
            'yyyyMMddTHHmmssZ',
            [Globalization.CultureInfo]::InvariantCulture)) + '-' + $guardNonce
        $evidenceRunRoot = [IO.Path]::GetFullPath(
            (Join-Path $caseEvidenceRoot $evidenceLeaf))
        if (-not (Test-ContainedPath `
                -Root $caseEvidenceRoot `
                -Candidate $evidenceRunRoot) -or
            (Test-Path -LiteralPath $evidenceRunRoot)) {
            throw 'The retained focused-autotest evidence run must be fresh and contained.'
        }
        New-Item -ItemType Directory -Path $evidenceRunRoot | Out-Null
        Assert-NoReparsePathAncestry -Path $evidenceRunRoot
    }

    foreach ($root in $normalizedDefaultRoots) {
        $defaultSnapshots.Add((New-RootSnapshot -Root $root))
    }
    $spillExclusions = New-Object Collections.Generic.List[string]
    $spillExclusions.Add($projectDirectory)
    if ($evidenceOutputPath -and -not $spillExclusions.Contains($evidenceOutputPath)) {
        $spillExclusions.Add($evidenceOutputPath)
    }
    foreach ($snapshot in $defaultSnapshots) {
        if (-not $spillExclusions.Contains($snapshot.Root)) {
            $spillExclusions.Add($snapshot.Root)
        }
    }
    foreach ($root in $normalizedSpillRoots) {
        $spillSnapshots.Add((New-RootSnapshot `
            -Root $root `
            -ExcludedRoots $spillExclusions.ToArray()))
    }

    Assert-NoReparsePathAncestry -Path $guardBase
    if (Test-Path -LiteralPath $guardRoot) {
        throw 'The nonce-owned focused-autotest guard directory was not fresh.'
    }
    New-Item -ItemType Directory -Path $guardRoot | Out-Null
    Assert-NoReparsePathAncestry -Path $guardRoot
    $guardItem = Get-Item -LiteralPath $guardRoot -Force -ErrorAction Stop
    if (($guardItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Focused-autotest guard root must not be a reparse point.'
    }
    [IO.File]::WriteAllText($sentinelPath, $guardNonce)
    $candidateStage = New-PartisanReleaseCandidateStage `
        -Candidate $candidateBinding `
        -GuardRoot $guardRoot
    $projectFile = $candidateStage.PackedProjectPath
    $projectDirectory = Split-Path -Parent $projectFile
    $addonSearchPath = $candidateStage.AddonSearchPath
    $diagnosticRedactedPaths += @(
        $candidateStage.StageRootPath,
        $candidateStage.PackedAddonPath)
    $workingDirectory = Join-Path $guardRoot 'work'
    $tempDirectory = Join-Path $guardRoot 'temp'
    $profileDirectory = Join-Path $guardRoot 'profile'
    New-Item -ItemType Directory -Path $workingDirectory, $tempDirectory, $profileDirectory -Force | Out-Null
    $profileProofSentinelPath = Join-Path $profileDirectory '.partisan-focused-owner'
    [IO.File]::WriteAllText($profileProofSentinelPath, 'owned')
    $addonTempDirectory = Join-Path $guardRoot 'addon-temp'
    New-Item -ItemType Directory -Path $addonTempDirectory -Force | Out-Null

    $arguments = @(
        '-addonsDir', $addonSearchPath,
        '-addons', $candidateBinding.AddonGuid,
        '-addonTempDir', $addonTempDirectory,
        '-gproj', $projectFile,
        '-profile', $guardRoot,
        '-window',
        '-noFocus',
        '-forceupdate',
        '-rpl-timeout-disable',
        '-noThrow',
        '-autotest', $TestCase,
        '-hstReleaseCandidateId', $candidateBinding.CandidateId,
        '-hstReleasePackageSha256', $candidateBinding.PackageSha256,
        '-hstReleaseManifestSha256', $candidateBinding.ManifestSha256
    )
    $argumentLine = ($arguments | ForEach-Object {
        ConvertTo-NativeArgument -Value ([string]$_)
    }) -join ' '

    $fullCommandLine = (ConvertTo-NativeArgument -Value $executablePath) +
        ' ' + $argumentLine
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $fullCommandLine `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $arguments)) {
        throw 'Focused-autotest native argument construction did not round-trip exactly.'
    }

    if ($PreflightOnly) {
        $result = [ordered]@{
            Success = $true
            PreflightOnly = $true
            Candidate = $candidateIdentity
            CandidateBoundaryVerified = $true
            TestCase = $TestCase
            ArgumentTokenCount = $arguments.Count
            HardDiagnosticClassifierChecks = $hardDiagnosticClassifierChecks
        }
        throw (New-Object OperationCanceledException('guarded-focused-preflight-complete'))
    }

    $job = New-Object PartisanFocusedAutotestJob
    if (@(Get-EngineProcesses).Count -ne 0) {
        throw 'An engine process appeared during focused-autotest preflight.'
    }

    try {
        $env:TEMP = $tempDirectory
        $env:TMP = $tempDirectory
        $suspendedLauncher = New-Object PartisanFocusedAutotestSuspendedProcess(
            $executablePath,
            $fullCommandLine,
            $workingDirectory)
    }
    finally {
        $env:TEMP = $oldTemp
        $env:TMP = $oldTmp
    }
    $process = $suspendedLauncher.Child
    if (-not $process) {
        throw 'Diagnostic focused-autotest process did not start.'
    }
    $ownedProcessIds[[int]$process.Id] = $true
    $job.Add($process)
    $suspendedLauncher.Resume()
    $suspendedLauncher.Dispose()
    $suspendedLauncher = $null

    Start-Sleep -Milliseconds 250
    $process.Refresh()
    if ($process.HasExited) {
        throw 'Focused autotest exited before its exact launch vector could be verified.'
    }
    $processRow = Get-CimInstance `
        Win32_Process `
        -Filter "ProcessId=$($process.Id)" `
        -ErrorAction Stop
    if (-not $processRow -or
        -not (Test-ExactNativeArgumentVector `
            -CommandLine ([string]$processRow.CommandLine) `
            -ExpectedExecutable $executablePath `
            -ExpectedArguments $arguments)) {
        throw 'Launched focused-autotest command-line token verification failed.'
    }

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        Update-OwnedProcessIds -Owned $ownedProcessIds -Job $job
        Update-UnclaimedEngineProcesses `
            -Owned $ownedProcessIds `
            -Observed $unclaimedProcessesObserved
        if ($unclaimedProcessesObserved.Count -ne 0) {
            throw 'An unowned engine process appeared; it was left untouched.'
        }
        $activeJobIds = @($job.GetProcessIds())
        $process.Refresh()
        if ($process.HasExited -and $activeJobIds.Count -eq 0) {
            break
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    Update-OwnedProcessIds -Owned $ownedProcessIds -Job $job
    $process.Refresh()
    if (-not $process.HasExited -or @($job.GetProcessIds()).Count -ne 0) {
        throw 'Focused autotest timed out.'
    }
    $exitCode = $process.ExitCode
    Start-Sleep -Milliseconds 1000

    [void](Assert-HarnessBinding `
        -Expected $harnessBinding `
        -CheckoutRoot $checkoutRoot `
        -RunnerPath $PSCommandPath `
        -CandidateModulePath $candidateModulePath)
    [void](Assert-PartisanReleaseCandidateStage `
        -Candidate $candidateBinding `
        -StageRoot $candidateStage.StageRootPath)
    $postCandidateBinding = Assert-PartisanReleaseCandidate `
        -ManifestPath $CandidateManifest `
        -BundleRoot $CandidateBundleRoot `
        -RuntimeAddonRoot $RuntimeAddonRoot `
        -Executable $executablePath `
        -RuntimeRole client `
        -ConsumerIntent $consumerIntent
    if ($postCandidateBinding.PackageSha256 -cne $candidateBinding.PackageSha256 -or
        $postCandidateBinding.ManifestSha256 -cne $candidateBinding.ManifestSha256 -or
        $postCandidateBinding.ReadySha256 -cne $candidateBinding.ReadySha256) {
        throw 'The sealed candidate identity changed during the focused autotest.'
    }
    $candidateBoundaryVerified = $true
    $mountAttestation = Get-CandidateMountAttestation `
        -GuardRoot $guardRoot `
        -PackedProjectPath $candidateStage.PackedProjectPath `
        -AddonGuid $candidateBinding.AddonGuid
    if (-not $mountAttestation.Valid) {
        throw 'Engine logs did not attest the exact guarded packed candidate mount.'
    }

    $junitFiles = @(Get-ChildItem `
        -LiteralPath $guardRoot `
        -Recurse `
        -File `
        -Filter 'junit.xml' `
        -ErrorAction SilentlyContinue)
    $failedFiles = @(Get-ChildItem `
        -LiteralPath $guardRoot `
        -Recurse `
        -File `
        -Filter 'autotest_failed.log' `
        -ErrorAction SilentlyContinue)
    $consoleFiles = @(Get-ChildItem `
        -LiteralPath $guardRoot `
        -Recurse `
        -File `
        -Filter 'console.log' `
        -ErrorAction SilentlyContinue)

    if ($consoleFiles.Count -ne 1) {
        throw "Focused autotest produced $($consoleFiles.Count) console logs."
    }
    $consoleText = [IO.File]::ReadAllText($consoleFiles[0].FullName)
    $hardDiagnosticCensus = Get-FocusedHardDiagnosticCensus `
        -ConsoleText $consoleText `
        -ExpectedTestCase $TestCase
    $unapprovedHardDiagnosticEvidence = @(
        $hardDiagnosticCensus.UnapprovedHardDiagnosticLines | ForEach-Object {
            ConvertTo-SafeDiagnosticText `
                -Text ([string]$_) `
                -RedactedPaths ($diagnosticRedactedPaths + @($guardRoot))
        })
    $diagnosticTail = @($consoleText -split "`r?`n" | Where-Object {
        $_ -match 'HST_|Autotest|autotest|Test Result|SCRIPT\s+\(E\)|ENGINE\s+\(E\)'
    } | Select-Object -Last 80 | ForEach-Object {
        ConvertTo-SafeDiagnosticText `
            -Text $_ `
            -RedactedPaths ($diagnosticRedactedPaths + @($guardRoot))
    })

    if ($junitFiles.Count -ne 1) {
        throw "Focused autotest produced $($junitFiles.Count) JUnit files."
    }
    $junit = Get-JUnitEvidence `
        -Path $junitFiles[0].FullName `
        -ExpectedTestCase $TestCase
    $expectedTestCases = @(Get-FocusedExpectedTestCases `
        -Selector $TestCase)
    $junitFailureEvidence = ConvertTo-SafeDiagnosticText `
        -Text $junit.CaseFailureText `
        -RedactedPaths ($diagnosticRedactedPaths + @($guardRoot))
    $junitErrorEvidence = ConvertTo-SafeDiagnosticText `
        -Text $junit.CaseErrorText `
        -RedactedPaths ($diagnosticRedactedPaths + @($guardRoot))
    if (-not [string]::IsNullOrWhiteSpace($junitFailureEvidence)) {
        $diagnosticTail += 'JUNIT FAILURE: ' + $junitFailureEvidence
    }
    if (-not [string]::IsNullOrWhiteSpace($junitErrorEvidence)) {
        $diagnosticTail += 'JUNIT ERROR: ' + $junitErrorEvidence
    }
    $failedListBytes = 0L
    foreach ($failedFile in $failedFiles) {
        $failedListBytes += $failedFile.Length
    }

    $requiredPatternsSeen = $true
    foreach ($pattern in $normalizedRequiredLogPatterns) {
        if ($consoleText.IndexOf($pattern, [StringComparison]::Ordinal) -lt 0) {
            $requiredPatternsSeen = $false
        }
    }
    $buildProvenanceSeen = $consoleText.IndexOf(
        $expectedBuildSummary,
        [StringComparison]::Ordinal) -ge 0

    $result = [ordered]@{
        Candidate = $candidateIdentity
        CandidateBoundaryVerified = $candidateBoundaryVerified
        MountAttestation = $mountAttestation
        Success = $exitCode -eq 0 -and
            $mountAttestation.Valid -and
            $junit.Tests -eq $expectedTestCases.Count -and
            $junit.Failures -eq 0 -and
            $junit.Errors -eq 0 -and
            $junit.TestCaseCount -eq $expectedTestCases.Count -and
            $junit.CaseIdentityExact -and
            $junit.CaseFailures -eq 0 -and
            $junit.CaseErrors -eq 0 -and
            $junit.CaseSkipped -eq 0 -and
            $failedFiles.Count -eq 1 -and
            $failedListBytes -eq 0 -and
            $requiredPatternsSeen -and
            $buildProvenanceSeen -and
            $hardDiagnosticCensus.Valid
        ExitCode = $exitCode
        Tests = $junit.Tests
        Failures = $junit.Failures
        Errors = $junit.Errors
        JUnitTestCaseCount = $junit.TestCaseCount
        JUnitCaseName = $junit.CaseName
        JUnitCaseClassName = $junit.CaseClassName
        JUnitCaseIdentityExact = $junit.CaseIdentityExact
        JUnitCaseFailures = $junit.CaseFailures
        JUnitCaseErrors = $junit.CaseErrors
        JUnitCaseSkipped = $junit.CaseSkipped
        JUnitFailureEvidence = $junitFailureEvidence
        JUnitErrorEvidence = $junitErrorEvidence
        FailedListFileCount = $failedFiles.Count
        FailedListBytes = $failedListBytes
        RequiredPatternsSeen = $requiredPatternsSeen
        BuildProvenanceSeen = $buildProvenanceSeen
        ConsoleTestCaseSeen = $consoleText.IndexOf(
            $TestCase,
            [StringComparison]::Ordinal) -ge 0
        HardDiagnosticClassifierChecks = $hardDiagnosticClassifierChecks
        HardDiagnosticClassificationValid = $hardDiagnosticCensus.Valid
        HardDiagnosticFree = $hardDiagnosticCensus.HardDiagnosticFree
        HardDiagnosticCount = $hardDiagnosticCensus.HardDiagnosticCount
        ApprovedStockFilterDiagnosticCount =
            $hardDiagnosticCensus.ApprovedStockFilterCount
        ApprovedIntentionalFaultDiagnosticCount =
            $hardDiagnosticCensus.ApprovedIntentionalFaultCount
        UnapprovedHardDiagnosticCount =
            $hardDiagnosticCensus.UnapprovedHardDiagnosticCount
        UnapprovedHardDiagnosticEvidence = $unapprovedHardDiagnosticEvidence
    }
    Write-RequiredLogPatternContract `
        -ConsolePath $consoleFiles[0].FullName `
        -OriginalConsoleText $consoleText `
        -Patterns $normalizedRequiredLogPatterns
    if (-not $result.Success) {
        throw 'Focused autotest evidence did not pass.'
    }
}
catch {
    if ($_.Exception -is [OperationCanceledException] -and
        $_.Exception.Message -ceq 'guarded-focused-preflight-complete') {
        $runError = $null
    }
    else {
        $runError = ConvertTo-SafeDiagnosticText `
            -Text $_.Exception.Message `
            -RedactedPaths ($diagnosticRedactedPaths + @($guardRoot))
    }
}
finally {
    $env:TEMP = $oldTemp
    $env:TMP = $oldTmp

    if ($suspendedLauncher) {
        try {
            $suspendedLauncher.Dispose()
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
        $suspendedLauncher = $null
    }
    if ($job) {
        try {
            Update-OwnedProcessIds -Owned $ownedProcessIds -Job $job
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
        try {
            $job.Dispose()
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
        $job = $null
    }
    if ($process) {
        try {
            $process.Dispose()
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
        $process = $null
    }
    for ($settleAttempt = 0; $settleAttempt -lt 50; $settleAttempt++) {
        $ownedRemaining = @(
            Get-EngineProcesses | Where-Object {
                $ownedProcessIds.ContainsKey([int]$_.Id)
            }).Count
        if ($ownedRemaining -eq 0) {
            break
        }
        Start-Sleep -Milliseconds 100
    }

    Update-UnclaimedEngineProcesses `
        -Owned $ownedProcessIds `
        -Observed $unclaimedProcessesObserved
    $cleanup.UnclaimedEngineProcessesObserved = $unclaimedProcessesObserved.Count
    $cleanup.OwnedProcessesRemaining = @(
        Get-EngineProcesses | Where-Object {
            $ownedProcessIds.ContainsKey([int]$_.Id)
        }).Count

    if ($cleanup.OwnedProcessesRemaining -eq 0) {
        try {
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
                -RuntimeRole client `
                -ConsumerIntent $consumerIntent
            if ($cleanupCandidateBinding.PackageSha256 -cne $candidateBinding.PackageSha256 -or
                $cleanupCandidateBinding.ManifestSha256 -cne $candidateBinding.ManifestSha256 -or
                $cleanupCandidateBinding.ReadySha256 -cne $candidateBinding.ReadySha256) {
                throw 'The sealed candidate identity changed across the focused run.'
            }
            $candidateBoundaryVerified = $true
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
        try {
            [void](Assert-HarnessBinding `
                -Expected $harnessBinding `
                -CheckoutRoot $checkoutRoot `
                -RunnerPath $PSCommandPath `
                -CandidateModulePath $candidateModulePath)
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
        try {
            if ($evidenceRunRoot) {
                if (-not (Test-Path -LiteralPath $guardRoot -PathType Container)) {
                    throw 'The focused guard disappeared before evidence retention.'
                }
                Copy-FocusedAutotestEvidence `
                    -GuardRoot $guardRoot `
                    -EvidenceRunRoot $evidenceRunRoot `
                    -Candidate $candidateBinding
                $evidenceState.Captured = $true
            }
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
        try {
            if (Test-Path -LiteralPath $guardRoot) {
                if (-not (Test-Path -LiteralPath $sentinelPath -PathType Leaf)) {
                    throw 'Focused-autotest cleanup ownership sentinel is missing.'
                }
                $sentinelItem = Get-Item -LiteralPath $sentinelPath -Force -ErrorAction Stop
                if (($sentinelItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw 'Focused-autotest cleanup ownership sentinel became a reparse point.'
                }
                if ([IO.File]::ReadAllText($sentinelPath) -cne $guardNonce) {
                    throw 'Focused-autotest cleanup ownership sentinel identity changed.'
                }
                if (-not (Test-ContainedPath -Root $guardBase -Candidate $guardRoot)) {
                    throw 'Focused-autotest cleanup containment failed.'
                }
                Assert-NoReparsePathAncestry -Path $guardRoot
                $guardItem = Get-Item -LiteralPath $guardRoot -Force -ErrorAction Stop
                if (($guardItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw 'Focused-autotest guard root became a reparse point.'
                }
                [void](Get-SafeSnapshotEntries -Root $guardRoot)
                Remove-Item -LiteralPath $guardRoot -Recurse -Force
            }
            if ((Test-Path -LiteralPath $guardBase) -and
                @(Get-ChildItem -LiteralPath $guardBase -Force).Count -eq 0) {
                Assert-NoReparsePathAncestry -Path $guardBase
                if ((Split-Path -Leaf $guardBase) -cne 'PartisanFocusedAutotest') {
                    throw 'Focused-autotest guard-base identity changed.'
                }
                $guardBaseItem = Get-Item -LiteralPath $guardBase -Force -ErrorAction Stop
                if (($guardBaseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw 'Focused-autotest guard base became a reparse point.'
                }
                Remove-Item -LiteralPath $guardBase -Force
            }
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
    }
    else {
        $cleanup.CleanupErrors +=
            'Focused-autotest guard removal was skipped while its owned process remained live.'
    }

    $cleanup.GuardRemaining = [int](Test-Path -LiteralPath $guardRoot)
    $cleanup.GuardBaseRemaining = [int](Test-Path -LiteralPath $guardBase)
    $cleanup.EngineProcessesRemaining = @(Get-EngineProcesses).Count

    $cleanup.NewDefaultEntriesRemaining = 0
    $cleanup.ModifiedDefaultFiles = 0
    $cleanup.DeletedDefaultEntries = 0
    $cleanup.MissingDefaultRoots = 0
    foreach ($snapshot in $defaultSnapshots) {
        try {
            $snapshotResult = Get-RootSnapshotDelta -Snapshot $snapshot
            $cleanup.NewDefaultEntriesRemaining += $snapshotResult.NewEntries
            $cleanup.ModifiedDefaultFiles += $snapshotResult.ModifiedFiles
            $cleanup.DeletedDefaultEntries += $snapshotResult.DeletedEntries
            $cleanup.MissingDefaultRoots += $snapshotResult.MissingRoot
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
    }
    $cleanup.ExternalSpillEntriesRemaining = 0
    $cleanup.ModifiedSpillFiles = 0
    $cleanup.DeletedSpillEntries = 0
    $cleanup.MissingSpillRoots = 0
    foreach ($snapshot in $spillSnapshots) {
        try {
            $snapshotResult = Get-RootSnapshotDelta -Snapshot $snapshot
            $cleanup.ExternalSpillEntriesRemaining += $snapshotResult.NewEntries
            $cleanup.ModifiedSpillFiles += $snapshotResult.ModifiedFiles
            $cleanup.DeletedSpillEntries += $snapshotResult.DeletedEntries
            $cleanup.MissingSpillRoots += $snapshotResult.MissingRoot
        }
        catch {
            $cleanup.CleanupErrors += $_.Exception.Message
        }
    }

    Update-UnclaimedEngineProcesses `
        -Owned $ownedProcessIds `
        -Observed $unclaimedProcessesObserved
    $cleanup.UnclaimedEngineProcessesObserved = $unclaimedProcessesObserved.Count
    $cleanup.OwnedProcessesRemaining = @(
        Get-EngineProcesses | Where-Object {
            $ownedProcessIds.ContainsKey([int]$_.Id)
        }).Count
    $cleanup.EngineProcessesRemaining = @(Get-EngineProcesses).Count

    $cleanup.CleanupErrors = @($cleanup.CleanupErrors | ForEach-Object {
        ConvertTo-SafeDiagnosticText `
            -Text $_ `
            -RedactedPaths ($diagnosticRedactedPaths + @($guardRoot))
    })

    if ($mutex) {
        if ($mutexAcquired) {
            try {
                $mutex.ReleaseMutex()
            }
            catch {
                $cleanup.CleanupErrors += $_.Exception.Message
            }
        }
        $mutex.Dispose()
    }
}

Write-Output ('RESULT ' + ($result | ConvertTo-Json -Compress))
if ($diagnosticTail.Count -gt 0) {
    Write-Output ('DIAGNOSTIC ' + ($diagnosticTail -join "`n"))
}
Write-Output ('CLEANUP ' + ($cleanup | ConvertTo-Json -Compress))

$cleanupFailed = $cleanup.GuardRemaining -ne 0 -or
    $cleanup.GuardBaseRemaining -ne 0 -or
    $cleanup.EngineProcessesRemaining -ne 0 -or
    $cleanup.OwnedProcessesRemaining -ne 0 -or
    $cleanup.UnclaimedEngineProcessesObserved -ne 0 -or
    $cleanup.NewDefaultEntriesRemaining -ne 0 -or
    $cleanup.ModifiedDefaultFiles -ne 0 -or
    $cleanup.DeletedDefaultEntries -ne 0 -or
    $cleanup.MissingDefaultRoots -ne 0 -or
    $cleanup.ExternalSpillEntriesRemaining -ne 0 -or
    $cleanup.ModifiedSpillFiles -ne 0 -or
    $cleanup.DeletedSpillEntries -ne 0 -or
    $cleanup.MissingSpillRoots -ne 0 -or
    $cleanup.CleanupErrors.Count -ne 0
if ($evidenceRunRoot) {
    try {
        $rawRows = Get-EvidenceFileRows -EvidenceRunRoot $evidenceRunRoot
        $envelope = [ordered]@{
            schemaVersion = 1
            evidenceKind = 'packaged-focused-autotest'
            startedUtc = $evidenceStartUtc.ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
            completedUtc = [DateTime]::UtcNow.ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
            candidate = $candidateIdentity
            harness = [ordered]@{
                gitHead = $harnessBinding.GitHead
                dirty = -not $harnessBinding.Clean
                focusedRunnerSha256 = $harnessBinding.RunnerSha256
                candidateModuleSha256 = $harnessBinding.CandidateModuleSha256
            }
            launch = [ordered]@{
                testCase = $TestCase
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
                success = -not $runError -and -not $cleanupFailed -and
                    $result -and $result.Success
                candidateBoundaryVerified = $candidateBoundaryVerified
                mountAttestation = $mountAttestation
                evidenceCaptured = $evidenceState.Captured
                result = $result
                diagnosticTail = $diagnosticTail
                error = $runError
            }
            cleanup = $cleanup
            files = $rawRows
        }
        $envelopePath = Join-Path $evidenceRunRoot 'run.json'
        Write-PortableJson -Path $envelopePath -Value $envelope
        $envelopeSha = (Get-FileHash `
            -LiteralPath $envelopePath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        Write-Output ('EVIDENCE ' + ([pscustomobject]@{
            CandidateId = $candidateBinding.CandidateId
            TestCase = $TestCase
            FileCount = $rawRows.Count
            EnvelopeSha256 = $envelopeSha
        } | ConvertTo-Json -Compress))
    }
    catch {
        if ($runError) {
            $runError += ' | evidence retention failed'
        }
        else {
            $runError = 'Focused-autotest evidence retention failed.'
        }
    }
}
if ($cleanupFailed) {
    throw 'Focused-autotest cleanup did not converge to zero.'
}
if ($runError) {
    throw $runError
}
if (-not $result -or -not $result.Success) {
    throw 'Focused autotest did not pass.'
}

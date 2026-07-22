[CmdletBinding()]
param(
    [string]$Executable = '',
    [string]$RuntimeAddonRoot = '',
    [string]$EvidenceOutputRoot = '',
    [string]$SourceGitHead = '',

    [ValidateRange(30, 3600)]
    [int]$TimeoutSecondsPerSuite = 300,

    [ValidateRange(50, 5000)]
    [int]$PollMilliseconds = 250,

    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:SourceFocusedSuiteCases = [ordered]@{
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

$script:SourceFocusedSuiteFiles = [ordered]@{
    HST_EnemyCounterattackAutotestSuite =
        'Scripts/Game/HST/Tests/HST_EnemyCounterattackAutotest.c'
    HST_EnemyGarrisonRebuildAutotestSuite =
        'Scripts/Game/HST/Tests/HST_EnemyGarrisonRebuildAutotest.c'
    HST_EnemyPlanningCommitmentAutotestSuite =
        'Scripts/Game/HST/Tests/HST_EnemyPlanningCommitmentAutotest.c'
    HST_EnemyQRFAutotestSuite =
        'Scripts/Game/HST/Tests/HST_EnemyQRFAutotest.c'
    HST_CampaignProfileJournalAuthorityAutotestSuite =
        'Scripts/Game/HST/Tests/HST_CampaignProfileJournalAuthorityAutotest.c'
}

$script:SourceFocusedProfileProofSuite =
    'HST_CampaignProfileJournalAuthorityAutotestSuite'
$script:SourceFocusedProfileProofSentinel = '.partisan-focused-owner'
$script:SourceFocusedProfileProofSentinelValue = 'owned'

$script:PublishInputPaths = @(
    'Assets',
    'Configs',
    'Missions',
    'Prefabs',
    'Scripts',
    'UI',
    'Worlds',
    'addon.gproj',
    'thumbnail.png'
)

# The diagnostic executable can emit this exact stock UI teardown set. It may
# be absent, but if any row is present the complete multiset must match. Every
# other error/fatal row, including errors from channels other than SCRIPT and
# ENGINE, rejects the focused result.
$script:SourceFocusedApprovedAmbientErrors = @(
    "GUI`tE`tUnknown class 'SCR_WidgetExportRuleRoot' at offset 282(0x11a)",
    "RESOURCES`tE`t==== Resource leaks ====",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-48_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_mouse/icons_mouse-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_gamepad/icons_gamepad-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_keyboard/icons_keyboard-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_keyboard/icons_keyboard-glow-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-64_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/DeployMenu/Objectives-Briefing/Objectives-briefing-400_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-glow-400_atlas.edds   1",
    "RESOURCES`tE`tUI/Fonts/RobotoCondensed/RobotoCondensed_Regular.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-400_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_place_geometry_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_transform_geometry_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/cursors_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_objective_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_waypoint_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_rotate_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_transform_snap_disabled_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_transform_snap_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_move_camera_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_wait_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_transform_disabled_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_transform_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_place_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_select_uiuc.edds   1",
    "RESOURCES`tE`tUI/Textures/Cursor/arrow_raw_uiuc.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Panels/Shadow/panels_fadedBackground-16-100_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Panels/Outline/panels_roundedOutline-400_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Panels/panels_roundedCorners-400_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/WeaponInfo/panel_style_magazines.edds   1",
    "RESOURCES`tE`tUI/Imagesets/debugUI.edds   1",
    "RESOURCES`tE`tUI/Textures/WidgetLibrary/ScrollBarRectangles.edds   1",
    "RESOURCES`tE`tUI/Imagesets/default.edds   1",
    "RESOURCES`tE`tui/fonts/robotomono_msdf_28.edds   1"
)

if (-not ('PartisanSourceFocusedJob' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public sealed class PartisanSourceFocusedJob : IDisposable
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
        IntPtr job, int informationClass, IntPtr information,
        UInt32 informationLength);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool QueryInformationJobObject(
        IntPtr job, int informationClass, IntPtr information,
        UInt32 informationLength, out UInt32 returnLength);

    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr handle);

    public PartisanSourceFocusedJob()
    {
        _handle = CreateJobObject(IntPtr.Zero, null);
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException("Unable to create source-focused job.");

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
            new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
        info.BasicLimitInformation.LimitFlags = 0x00002000;
        int size = Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
        IntPtr pointer = Marshal.AllocHGlobal(size);
        try
        {
            Marshal.StructureToPtr(info, pointer, false);
            if (!SetInformationJobObject(_handle, 9, pointer, (UInt32)size))
                throw new InvalidOperationException("Unable to configure source-focused job.");
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
            throw new InvalidOperationException("Unable to contain source-focused process.");
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
                    _handle, 3, pointer, (UInt32)size, out returned);
                int assigned = Marshal.ReadInt32(pointer, 0);
                int listed = Marshal.ReadInt32(pointer, 4);
                if (!ok && assigned <= capacity)
                    throw new InvalidOperationException("Unable to query source-focused job.");
                if (assigned > capacity || listed > capacity)
                {
                    capacity *= 2;
                    continue;
                }
                List<Int32> result = new List<Int32>();
                for (int index = 0; index < listed; index++)
                {
                    long value = Marshal.ReadIntPtr(
                        pointer, 8 + (index * IntPtr.Size)).ToInt64();
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
            "Source-focused job membership exceeded its safety limit.");
    }

    public void Dispose()
    {
        if (_handle == IntPtr.Zero)
            return;
        CloseHandle(_handle);
        _handle = IntPtr.Zero;
    }
}

public sealed class PartisanSourceFocusedSuspendedProcess : IDisposable
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
        string applicationName, StringBuilder commandLine,
        IntPtr processAttributes, IntPtr threadAttributes,
        bool inheritHandles, UInt32 creationFlags, IntPtr environment,
        string currentDirectory, ref STARTUPINFO startupInfo,
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

    public PartisanSourceFocusedSuspendedProcess(
        string applicationName, string commandLine, string currentDirectory)
    {
        STARTUPINFO startup = new STARTUPINFO();
        startup.cb = (UInt32)Marshal.SizeOf(typeof(STARTUPINFO));
        PROCESS_INFORMATION information;
        const UInt32 CREATE_SUSPENDED = 0x00000004;
        const UInt32 CREATE_NO_WINDOW = 0x08000000;
        if (!CreateProcessW(
            applicationName, new StringBuilder(commandLine), IntPtr.Zero,
            IntPtr.Zero, false, CREATE_SUSPENDED | CREATE_NO_WINDOW,
            IntPtr.Zero, currentDirectory, ref startup, out information))
        {
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to create suspended source-focused process.");
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
                "Suspended source-focused thread is unavailable.");
        if (ResumeThread(_threadHandle) == UInt32.MaxValue)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to resume source-focused process.");
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

public static class PartisanSourceFocusedNativeArguments
{
    [DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern IntPtr CommandLineToArgvW(
        string commandLine, out int argumentCount);

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
                "Unable to parse source-focused native command line.");
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

function Resolve-SourceFocusedPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]
        [ValidateSet('Leaf', 'Container')]
        [string]$Kind
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

function Test-SourceFocusedContainedPath {
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

function Test-SourceFocusedPathOverlap {
    param(
        [Parameter(Mandatory = $true)][string]$First,
        [Parameter(Mandatory = $true)][string]$Second
    )

    return (Test-SourceFocusedContainedPath `
        -Root $First -Candidate $Second -AllowEqual) -or
        (Test-SourceFocusedContainedPath `
            -Root $Second -Candidate $First -AllowEqual)
}

function Assert-SourceFocusedNoReparseAncestry {
    param([Parameter(Mandatory = $true)][string]$Path)

    $cursor = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    while (-not (Test-Path -LiteralPath $cursor)) {
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A safe existing ancestor could not be resolved.'
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

function Assert-SourceFocusedTreeHasNoReparsePoint {
    param([Parameter(Mandatory = $true)][string]$Root)

    Assert-SourceFocusedNoReparseAncestry -Path $Root
    foreach ($item in @(Get-ChildItem `
            -LiteralPath $Root -Recurse -Force -ErrorAction Stop)) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'A guarded source-focused tree contains a reparse point.'
        }
    }
}

function ConvertTo-SourceFocusedNativeArgument {
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

function Test-SourceFocusedNativeArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$CommandLine,
        [Parameter(Mandatory = $true)][string]$ExpectedExecutable,
        [Parameter(Mandatory = $true)][string[]]$ExpectedArguments
    )

    $tokens = @([PartisanSourceFocusedNativeArguments]::Split($CommandLine))
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

function Get-SourceFocusedEngineProcesses {
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

function Get-SourceFocusedProcessRows {
    param([object[]]$Processes = @())

    return @($Processes | Sort-Object ProcessName, Id | ForEach-Object {
        $startedUtc = ''
        try {
            $startedUtc = $_.StartTime.ToUniversalTime().ToString(
                'o', [Globalization.CultureInfo]::InvariantCulture)
        }
        catch { }
        [pscustomobject][ordered]@{
            name = [string]$_.ProcessName
            id = [int]$_.Id
            startedUtc = $startedUtc
        }
    })
}

function Get-SourceFocusedSha256Text {
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text)

    $utf8 = New-Object Text.UTF8Encoding($false, $true)
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = $utf8.GetBytes($Text)
        return ([BitConverter]::ToString(
            $sha.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Get-SourceFocusedNormalizedTextFileSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    $text = [IO.File]::ReadAllText($Path)
    $normalized = $text.Replace("`r`n", "`n").Replace("`r", "`n")
    return Get-SourceFocusedSha256Text -Text $normalized
}

function Get-SourceFocusedFileIdentity {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [switch]$IncludeVersion
    )

    $resolved = Resolve-SourceFocusedPath -Path $Path -Kind Leaf
    $item = Get-Item -LiteralPath $resolved -Force -ErrorAction Stop
    $identity = [ordered]@{
        path = $resolved
        length = [long]$item.Length
        lastWriteUtc = $item.LastWriteTimeUtc.ToString(
            'o', [Globalization.CultureInfo]::InvariantCulture)
        sha256 = (Get-FileHash `
            -LiteralPath $resolved -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    if ($IncludeVersion) {
        $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($resolved)
        $identity.fileVersion = [string]$version.FileVersion
        $identity.productVersion = [string]$version.ProductVersion
        $identity.productName = [string]$version.ProductName
    }
    return [pscustomobject]$identity
}

function Test-SourceFocusedFileIdentityEqual {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    return [string]$Expected.path -ceq [string]$Actual.path -and
        [long]$Expected.length -eq [long]$Actual.length -and
        [string]$Expected.lastWriteUtc -ceq [string]$Actual.lastWriteUtc -and
        [string]$Expected.sha256 -ceq [string]$Actual.sha256
}

function Write-SourceFocusedJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $text = (($Value | ConvertTo-Json -Depth 40).Replace("`r`n", "`n") + "`n")
    [IO.File]::WriteAllText(
        $Path,
        $text,
        (New-Object Text.UTF8Encoding($false, $true)))
}

function Invoke-SourceFocusedGit {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $output = @(& git -C $CheckoutRoot @Arguments)
    if ($LASTEXITCODE -ne 0) {
        throw 'A required Git identity command failed.'
    }
    return $output
}

function Get-SourceFocusedPublishInputTree {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$Head
    )

    $arguments = @('ls-tree', '-r', '--full-tree', $Head, '--') +
        $script:PublishInputPaths
    $rows = @(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments $arguments)
    if ($rows.Count -eq 0) {
        throw 'The canonical publish-input tree is empty.'
    }
    $canonicalByPath = New-Object `
        'Collections.Generic.Dictionary[string,string]' `
        ([StringComparer]::Ordinal)
    $entryByPath = New-Object `
        'Collections.Generic.Dictionary[string,object]' `
        ([StringComparer]::Ordinal)
    $seenPathsIgnoreCase = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($row in $rows) {
        $match = [regex]::Match(
            [string]$row,
            '^(?<mode>100644|100755|120000) (?<type>blob) ' +
                '(?<oid>[0-9a-f]{40})\t(?<path>[^\r\n]+)$')
        if (-not $match.Success) {
            throw 'The canonical publish-input tree contains a malformed row.'
        }
        $path = $match.Groups['path'].Value.Replace('\', '/')
        if ([string]::IsNullOrWhiteSpace($path) -or
            $path.StartsWith('/', [StringComparison]::Ordinal) -or
            $canonicalByPath.ContainsKey($path) -or
            -not $seenPathsIgnoreCase.Add($path)) {
            throw 'The canonical publish-input tree contains an invalid path.'
        }
        $canonicalByPath[$path] = $match.Groups['mode'].Value + ' ' +
            $match.Groups['type'].Value + ' ' +
            $match.Groups['oid'].Value + "`t" + $path
        $entryByPath[$path] = [pscustomobject][ordered]@{
            path = $path
            mode = $match.Groups['mode'].Value
            oid = $match.Groups['oid'].Value
        }
    }
    $canonicalPaths = [string[]]@($canonicalByPath.Keys)
    [Array]::Sort($canonicalPaths, [StringComparer]::Ordinal)
    $canonicalText = (($canonicalPaths | ForEach-Object {
        $canonicalByPath[$_]
    }) -join "`n") + "`n"
    return [pscustomobject][ordered]@{
        algorithm = 'git-ls-tree-sha256-v1'
        paths = @($script:PublishInputPaths)
        rowCount = $rows.Count
        sha256 = Get-SourceFocusedSha256Text -Text $canonicalText
        entries = @($canonicalPaths | ForEach-Object {
            $entryByPath[$_]
        })
    }
}

function Assert-SourceFocusedWorktreePublishInputs {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)]$PublishInputTree
    )

    $entries = @($PublishInputTree.entries)
    if ($entries.Count -ne [int]$PublishInputTree.rowCount -or
        $entries.Count -le 0) {
        throw 'The publish-input worktree manifest is incomplete.'
    }
    $extraPublishPaths = @(
        Invoke-SourceFocusedGit `
            -CheckoutRoot $CheckoutRoot `
            -Arguments (@(
                'ls-files', '--others', '--exclude-standard', '--') +
                $script:PublishInputPaths)
        Invoke-SourceFocusedGit `
            -CheckoutRoot $CheckoutRoot `
            -Arguments (@(
                'ls-files', '--others', '--ignored', '--exclude-standard',
                '--') + $script:PublishInputPaths)
    )
    if ($extraPublishPaths.Count -ne 0) {
        throw 'The executed publish-input worktree contains an unbound extra file.'
    }
    $paths = [string[]]@($entries | ForEach-Object { [string]$_.path })
    foreach ($path in $paths) {
        $full = [IO.Path]::GetFullPath((Join-Path $CheckoutRoot $path))
        if (-not (Test-Path -LiteralPath $full -PathType Leaf) -or
            ((Get-Item -LiteralPath $full -Force).Attributes -band
                [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'A publish-input worktree file is missing or redirected.'
        }
    }
    $worktreeOids = @($paths |
        & git -C $CheckoutRoot hash-object --stdin-paths)
    if ($LASTEXITCODE -ne 0 -or $worktreeOids.Count -ne $entries.Count) {
        throw 'Unable to hash the publish-input worktree exactly.'
    }
    for ($index = 0; $index -lt $entries.Count; $index++) {
        if ([string]$worktreeOids[$index] -cne [string]$entries[$index].oid) {
            throw 'The executed publish-input worktree differs from its Git blob.'
        }
    }
}

function Get-SourceFocusedResourceDatabaseIdentity {
    param([Parameter(Mandatory = $true)][string]$CheckoutRoot)

    $identity = Get-SourceFocusedFileIdentity `
        -Path (Join-Path $CheckoutRoot 'resourceDatabase.rdb')
    if ([long]$identity.length -le 0) {
        throw 'The Workbench resource database is empty.'
    }
    return [pscustomobject][ordered]@{
        name = 'resourceDatabase.rdb'
        length = [long]$identity.length
        sha256 = [string]$identity.sha256
    }
}

function Get-SourceFocusedGitBinding {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath
    )

    $head = (@(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('rev-parse', '--verify', 'HEAD')) -join '').Trim()
    $tree = (@(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('rev-parse', 'HEAD^{tree}')) -join '').Trim()
    $branch = (@(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('branch', '--show-current')) -join '').Trim()
    $status = @(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('status', '--porcelain=v1', '--untracked-files=all'))
    if ($head -cnotmatch '^[0-9a-f]{40}$' -or
        $tree -cnotmatch '^[0-9a-f]{40}$' -or
        [string]::IsNullOrWhiteSpace($branch)) {
        throw 'The source-focused Git checkpoint identity is invalid.'
    }

    $runnerRelative = [IO.Path]::GetFullPath($RunnerPath).Substring(
        [IO.Path]::GetFullPath($CheckoutRoot).TrimEnd('\', '/').Length + 1
    ).Replace('\', '/')
    [void](Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('ls-files', '--error-unmatch', '--', $runnerRelative))
    $runnerGitBlobOid = (@(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('rev-parse', ($head + ':' + $runnerRelative))) -join '').Trim()
    $runnerWorktreeOid = (@(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('hash-object', '--', $runnerRelative)) -join '').Trim()
    if ($runnerGitBlobOid -cnotmatch '^[0-9a-f]{40}$' -or
        $runnerWorktreeOid -cne $runnerGitBlobOid) {
        throw 'The executed focused runner differs from its committed Git blob.'
    }
    $publishInputTree = Get-SourceFocusedPublishInputTree `
        -CheckoutRoot $CheckoutRoot `
        -Head $head
    Assert-SourceFocusedWorktreePublishInputs `
        -CheckoutRoot $CheckoutRoot `
        -PublishInputTree $publishInputTree
    Assert-SourceFocusedNoPackageFiles -CheckoutRoot $CheckoutRoot
    $resourceDatabase = Get-SourceFocusedResourceDatabaseIdentity `
        -CheckoutRoot $CheckoutRoot

    return [pscustomobject][ordered]@{
        head = $head
        tree = $tree
        branch = $branch
        clean = $status.Count -eq 0
        status = @($status)
        runnerPath = $runnerRelative
        runnerGitBlobOid = $runnerGitBlobOid
        runnerHashPolicy = 'normalized-utf8-lf-sha256-v1'
        runnerSha256 = Get-SourceFocusedNormalizedTextFileSha256 `
            -Path $RunnerPath
        publishInputTree = $publishInputTree
        resourceDatabase = $resourceDatabase
    }
}

function Assert-SourceFocusedGitBinding {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath
    )

    $actual = Get-SourceFocusedGitBinding `
        -CheckoutRoot $CheckoutRoot `
        -RunnerPath $RunnerPath
    if (-not $actual.clean -or
        [string]$actual.head -cne [string]$Expected.head -or
        [string]$actual.tree -cne [string]$Expected.tree -or
        [string]$actual.branch -cne [string]$Expected.branch -or
        [string]$actual.runnerGitBlobOid -cne
            [string]$Expected.runnerGitBlobOid -or
        [string]$actual.runnerHashPolicy -cne
            [string]$Expected.runnerHashPolicy -or
        [string]$actual.runnerSha256 -cne [string]$Expected.runnerSha256 -or
        [string]$actual.publishInputTree.algorithm -cne
            [string]$Expected.publishInputTree.algorithm -or
        [int]$actual.publishInputTree.rowCount -ne
            [int]$Expected.publishInputTree.rowCount -or
        [string]$actual.publishInputTree.sha256 -cne
            [string]$Expected.publishInputTree.sha256 -or
        [long]$actual.resourceDatabase.length -ne
            [long]$Expected.resourceDatabase.length -or
        [string]$actual.resourceDatabase.sha256 -cne
            [string]$Expected.resourceDatabase.sha256) {
        throw 'The clean source checkpoint changed during focused execution.'
    }
    return $actual
}

function Get-SourceFocusedCheckpointBinding {
    param(
        [AllowEmptyString()]
        [Parameter(Mandatory = $true)]
        [string]$RequestedHead,

        [Parameter(Mandatory = $true)]$HarnessBinding,
        [Parameter(Mandatory = $true)][string]$CheckoutRoot
    )

    $sourceHead = $RequestedHead
    if ([string]::IsNullOrWhiteSpace($sourceHead)) {
        $sourceHead = [string]$HarnessBinding.head
    }
    if ($sourceHead -cnotmatch '^[0-9a-f]{40}$') {
        throw 'SourceGitHead must be a full lowercase Git commit SHA.'
    }
    $resolved = (@(Invoke-SourceFocusedGit `
        -CheckoutRoot $CheckoutRoot `
        -Arguments @('rev-parse', '--verify', ($sourceHead + '^{commit}'))) `
        -join '').Trim()
    if ($resolved -cne $sourceHead) {
        throw 'SourceGitHead did not resolve to its exact requested commit.'
    }
    & git -C $CheckoutRoot merge-base --is-ancestor `
        $sourceHead $HarnessBinding.head
    $ancestorExit = $LASTEXITCODE
    if ($ancestorExit -eq 1) {
        throw 'SourceGitHead is not an ancestor of the clean harness commit.'
    }
    if ($ancestorExit -ne 0) {
        throw 'The source-to-harness ancestry check failed.'
    }
    $publishInput = Get-SourceFocusedPublishInputTree `
        -CheckoutRoot $CheckoutRoot `
        -Head $sourceHead
    if ([string]$publishInput.algorithm -cne
            [string]$HarnessBinding.publishInputTree.algorithm -or
        [int]$publishInput.rowCount -ne
            [int]$HarnessBinding.publishInputTree.rowCount -or
        [string]$publishInput.sha256 -cne
            [string]$HarnessBinding.publishInputTree.sha256) {
        throw 'The harness commit changed the frozen Workshop publish inputs.'
    }
    return [pscustomobject][ordered]@{
        sourceGitHead = $sourceHead
        harnessGitHead = [string]$HarnessBinding.head
        sourceIsHarnessAncestor = $true
        publishInputsMatchHarness = $true
        publishInputTree = $publishInput
    }
}

function Assert-SourceFocusedNoPackageFiles {
    param([Parameter(Mandatory = $true)][string]$CheckoutRoot)

    $packagePathspecs = @(
        ':(icase,glob)*.pak',
        ':(icase,glob)**/*.pak')
    $queries = @(
        [pscustomobject]@{
            arguments = @('ls-files', '--') + $packagePathspecs
        },
        [pscustomobject]@{
            arguments = @(
                'ls-files', '--others', '--exclude-standard', '--') +
                $packagePathspecs
        },
        [pscustomobject]@{
            arguments = @(
                'ls-files', '--others', '--ignored', '--exclude-standard',
                '--') + $packagePathspecs
        }
    )
    foreach ($query in $queries) {
        $matches = @(Invoke-SourceFocusedGit `
            -CheckoutRoot $CheckoutRoot `
            -Arguments $query.arguments)
        if ($matches.Count -ne 0) {
            throw 'The source checkout contains a package file.'
        }
    }
}

function Get-SourceFocusedProjectBinding {
    param([Parameter(Mandatory = $true)][string]$ProjectPath)

    $text = [IO.File]::ReadAllText($ProjectPath)
    $idMatches = [regex]::Matches(
        $text, '(?m)^\s*ID\s+"(?<value>[A-Za-z0-9_-]+)"\s*$')
    $guidMatches = [regex]::Matches(
        $text, '(?m)^\s*GUID\s+"(?<value>[0-9A-F]{16})"\s*$')
    $dependencyMatches = [regex]::Matches(
        $text, '(?m)^\s*"(?<value>[0-9A-F]{16})"\s*$')
    if ($idMatches.Count -ne 1 -or $guidMatches.Count -ne 1 -or
        $dependencyMatches.Count -ne 1 -or
        $idMatches[0].Groups['value'].Value -cne 'histasi' -or
        $guidMatches[0].Groups['value'].Value -cne '698532771130111D' -or
        $dependencyMatches[0].Groups['value'].Value -cne '58D0FB3206B6F859') {
        throw 'The canonical source addon project identity is invalid.'
    }
    return [pscustomobject][ordered]@{
        id = $idMatches[0].Groups['value'].Value
        guid = $guidMatches[0].Groups['value'].Value
        dependencies = @($dependencyMatches | ForEach-Object {
            $_.Groups['value'].Value
        })
        file = Get-SourceFocusedFileIdentity -Path $ProjectPath
    }
}

function Get-SourceFocusedBuildIdentity {
    param([Parameter(Mandatory = $true)][string]$Path)

    $text = [IO.File]::ReadAllText($Path)
    $sha = [regex]::Match(
        $text,
        '(?m)^\s*static\s+const\s+string\s+BUILD_SHA\s*=\s*"(?<value>[0-9a-f]{40})";\s*$')
    $utc = [regex]::Match(
        $text,
        '(?m)^\s*static\s+const\s+string\s+BUILD_UTC\s*=\s*"(?<value>[^"\r\n]+)";\s*$')
    $label = [regex]::Match(
        $text,
        '(?m)^\s*static\s+const\s+string\s+BUILD_LABEL\s*=\s*"(?<value>[^"\r\n]+)";\s*$')
    $parsedUtc = [DateTimeOffset]::MinValue
    if (-not $sha.Success -or -not $utc.Success -or -not $label.Success -or
        -not [DateTimeOffset]::TryParseExact(
            $utc.Groups['value'].Value,
            'yyyy-MM-ddTHH:mm:ssZ',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::AssumeUniversal,
            [ref]$parsedUtc)) {
        throw 'The embedded source build identity is invalid.'
    }
    return [pscustomobject][ordered]@{
        sha = $sha.Groups['value'].Value
        utc = $utc.Groups['value'].Value
        label = $label.Groups['value'].Value
        summary = 'sha ' + $sha.Groups['value'].Value +
            ' | utc ' + $utc.Groups['value'].Value +
            ' | label ' + $label.Groups['value'].Value
        file = Get-SourceFocusedFileIdentity -Path $Path
    }
}

function Get-SourceFocusedSuiteDefinitions {
    param([Parameter(Mandatory = $true)][string]$CheckoutRoot)

    $rows = New-Object Collections.Generic.List[object]
    $seenCases = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $total = 0
    foreach ($suite in $script:SourceFocusedSuiteCases.Keys) {
        $relative = [string]$script:SourceFocusedSuiteFiles[$suite]
        $path = Resolve-SourceFocusedPath `
            -Path (Join-Path $CheckoutRoot $relative) `
            -Kind Leaf
        $text = [IO.File]::ReadAllText($path)
        $suitePattern = '(?m)^\s*class\s+' + [regex]::Escape($suite) +
            '\s*:\s*SCR_AutotestSuiteBase\s*$'
        if ([regex]::Matches($text, $suitePattern).Count -ne 1) {
            throw "The source suite declaration is invalid: $suite"
        }
        $registrationPattern = '(?s)\[Test\(suite:\s*' +
            [regex]::Escape($suite) +
            '\s*\)\]\s*class\s+(?<case>HST_TEST_[A-Za-z0-9_]+)'
        $registrations = @([regex]::Matches($text, $registrationPattern) |
            ForEach-Object { $_.Groups['case'].Value })
        $expected = @($script:SourceFocusedSuiteCases[$suite])
        $actualSet = New-Object 'Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
        foreach ($caseName in $registrations) {
            if (-not $actualSet.Add([string]$caseName)) {
                throw "The source suite repeats a case: $suite"
            }
        }
        if ($registrations.Count -ne $expected.Count) {
            throw "The source suite case count is invalid: $suite"
        }
        foreach ($caseName in $expected) {
            if (-not $actualSet.Contains([string]$caseName) -or
                -not $seenCases.Add([string]$caseName)) {
                throw "The source suite case identity is invalid: $suite"
            }
        }
        $total += $expected.Count
        [void]$rows.Add([pscustomobject][ordered]@{
            suite = $suite
            sourcePath = $relative
            sourceSha256 = (Get-FileHash `
                -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
            expectedCaseCount = $expected.Count
            expectedCases = @($expected)
        })
    }
    if ($total -ne 91 -or $seenCases.Count -ne 91) {
        throw 'The exact source-focused suite manifest must contain 91 unique cases.'
    }
    return $rows.ToArray()
}

function ConvertTo-SourceFocusedNormalizedPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [IO.Path]::GetFullPath($Path.Replace('/', '\')).TrimEnd('\', '/')
}

function Get-SourceFocusedJUnitContract {
    param(
        [Parameter(Mandatory = $true)][string]$XmlText,
        [Parameter(Mandatory = $true)][string]$Suite,
        [Parameter(Mandatory = $true)][string[]]$ExpectedCases
    )

    $settings = New-Object Xml.XmlReaderSettings
    $settings.DtdProcessing = [Xml.DtdProcessing]::Prohibit
    $settings.XmlResolver = $null
    $reader = [Xml.XmlReader]::Create(
        (New-Object IO.StringReader($XmlText)), $settings)
    $document = New-Object Xml.XmlDocument
    $document.XmlResolver = $null
    try {
        $document.Load($reader)
    }
    finally {
        $reader.Dispose()
    }
    $root = $document.DocumentElement
    if (-not $root -or $root.LocalName -cne 'testsuites') {
        throw 'Source-focused JUnit must use a testsuites root.'
    }
    $suiteNodes = @($root.SelectNodes('./testsuite'))
    if ($suiteNodes.Count -ne 1 -or
        [string]$suiteNodes[0].GetAttribute('name') -cne $Suite) {
        throw 'Source-focused JUnit suite identity is not exact.'
    }
    $suiteNode = $suiteNodes[0]
    $testsText = [string]$suiteNode.GetAttribute('tests')
    if ($testsText -cnotmatch '^\d+$' -or [int]$testsText -ne $ExpectedCases.Count) {
        throw 'Source-focused JUnit suite test count is not exact.'
    }
    foreach ($attributeName in @('failures', 'errors', 'skipped')) {
        $value = [string]$suiteNode.GetAttribute($attributeName)
        if (-not [string]::IsNullOrEmpty($value) -and $value -cne '0') {
            throw "Source-focused JUnit suite $attributeName is nonzero."
        }
    }
    $caseNodes = @($suiteNode.SelectNodes('./testcase'))
    if ($caseNodes.Count -ne $ExpectedCases.Count) {
        throw 'Source-focused JUnit testcase count is not exact.'
    }
    $expectedSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($caseName in $ExpectedCases) {
        if (-not $expectedSet.Add([string]$caseName)) {
            throw 'Source-focused expected testcase manifest contains a duplicate.'
        }
    }
    $actualSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $actualCases = New-Object Collections.Generic.List[string]
    foreach ($caseNode in $caseNodes) {
        $name = [string]$caseNode.GetAttribute('name')
        $className = [string]$caseNode.GetAttribute('classname')
        if ($className -cne $Suite -or
            -not $expectedSet.Contains($name) -or
            -not $actualSet.Add($name) -or
            @($caseNode.SelectNodes('./failure')).Count -ne 0 -or
            @($caseNode.SelectNodes('./error')).Count -ne 0 -or
            @($caseNode.SelectNodes('./skipped')).Count -ne 0) {
            throw 'Source-focused JUnit testcase identity or outcome is invalid.'
        }
        [void]$actualCases.Add($name)
    }
    if ($actualSet.Count -ne $expectedSet.Count) {
        throw 'Source-focused JUnit testcase set is not exact.'
    }
    return [pscustomobject][ordered]@{
        valid = $true
        suite = $Suite
        tests = $ExpectedCases.Count
        failures = 0
        errors = 0
        skipped = 0
        actualCases = $actualCases.ToArray()
    }
}

function Get-SourceFocusedMountContract {
    param(
        [Parameter(Mandatory = $true)][string]$ConsoleText,
        [Parameter(Mandatory = $true)][string]$ProjectPath,
        [Parameter(Mandatory = $true)][string]$ProjectGuid,
        [Parameter(Mandatory = $true)][string]$ProjectId
    )

    $pattern = "(?m)^\s*\d{2}:\d{2}:\d{2}\.\d+\s+ENGINE\s+:\s+" +
        "gproj:\s+'(?<path>[^']+)'\s+guid:\s+'(?<guid>[0-9A-Fa-f]{16})'" +
        "(?<packed>\s+\(packed\))?\s*$"
    $expectedPath = ConvertTo-SourceFocusedNormalizedPath -Path $ProjectPath
    $records = New-Object Collections.Generic.List[object]
    foreach ($match in [regex]::Matches($ConsoleText, $pattern)) {
        $candidatePath = ConvertTo-SourceFocusedNormalizedPath `
            -Path $match.Groups['path'].Value
        if (-not $candidatePath.Equals(
                $expectedPath, [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        [void]$records.Add([pscustomobject][ordered]@{
            pathRole = 'source-addon-project'
            guid = $match.Groups['guid'].Value.ToUpperInvariant()
            packed = $match.Groups['packed'].Success
        })
    }
    $sourceRoot = Split-Path -Parent $expectedPath
    $escapedRoot = [regex]::Escape($sourceRoot.Replace('\', '/'))
    $escapedRootAlternate = [regex]::Escape($sourceRoot.Replace('/', '\'))
    $relativePattern = "(?im)^.*FileSystem: Adding relative directory '" +
        "(?:$escapedRoot|$escapedRootAlternate)' to filesystem under name " +
        [regex]::Escape($ProjectId) + "\s*$"
    $packagePattern = "(?im)^.*FileSystem: Adding package '" +
        "(?:$escapedRoot|$escapedRootAlternate)(?:[/\\])?'"
    $relativeCount = [regex]::Matches($ConsoleText, $relativePattern).Count
    $sourcePackageCount = [regex]::Matches($ConsoleText, $packagePattern).Count
    $expectedDatabasePath = ConvertTo-SourceFocusedNormalizedPath `
        -Path (Join-Path $sourceRoot 'resourceDatabase.rdb')
    $databasePattern =
        '(?m)^\s*\d{2}:\d{2}:\d{2}\.\d+\s+RESOURCES\s+:\s+' +
        'ResourceDB: loading cache \(id=(?<id>[0-9]+) ' +
        'name=(?<name>[^ )]+) path=(?<path>.+)\)\s*$'
    $databaseRows = New-Object Collections.Generic.List[object]
    foreach ($match in [regex]::Matches($ConsoleText, $databasePattern)) {
        $candidatePath = ConvertTo-SourceFocusedNormalizedPath `
            -Path $match.Groups['path'].Value
        if ($match.Groups['name'].Value -ceq $ProjectId -or
            $candidatePath.Equals(
                $expectedDatabasePath,
                [StringComparison]::OrdinalIgnoreCase)) {
            [void]$databaseRows.Add([pscustomobject][ordered]@{
                id = [int]$match.Groups['id'].Value
                name = $match.Groups['name'].Value
                pathExact = $candidatePath.Equals(
                    $expectedDatabasePath,
                    [StringComparison]::OrdinalIgnoreCase)
            })
        }
    }
    $databaseLoadExact = $databaseRows.Count -eq 1 -and
        $databaseRows[0].id -eq 0 -and
        $databaseRows[0].name -ceq $ProjectId -and
        $databaseRows[0].pathExact
    $guidExact = @($records | Where-Object {
        $_.guid -ceq $ProjectGuid
    }).Count -eq 2
    $packedCount = @($records | Where-Object { $_.packed }).Count
    $valid = $records.Count -eq 2 -and $guidExact -and
        $packedCount -eq 0 -and $relativeCount -eq 1 -and
        $sourcePackageCount -eq 0 -and $databaseLoadExact
    return [pscustomobject][ordered]@{
        valid = $valid
        sourceProjectRecordCount = $records.Count
        sourceGuidExact = $guidExact
        sourcePackedRecordCount = $packedCount
        relativeDirectoryRecordCount = $relativeCount
        sourcePackageRecordCount = $sourcePackageCount
        resourceDatabaseLoadCount = $databaseRows.Count
        resourceDatabaseLoadExact = $databaseLoadExact
        records = $records.ToArray()
    }
}

function ConvertTo-SourceFocusedSafeText {
    param(
        [AllowEmptyString()][string]$Text,
        [string[]]$RedactedPaths = @()
    )

    $safe = [string]$Text
    foreach ($path in @($RedactedPaths | Sort-Object Length -Descending)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$path)) {
            $safe = $safe.Replace([string]$path, '<local-path>')
            $safe = $safe.Replace(
                ([string]$path).Replace('\', '/'), '<local-path>')
        }
    }
    $safe = [regex]::Replace(
        $safe,
        '(?i)(?:(?<![A-Z0-9_])[A-Z]:[\\/]|\\\\)[^<>\r\n|"'']+',
        '<local-path>')
    return $safe
}

function Get-SourceFocusedConsoleContract {
    param(
        [Parameter(Mandatory = $true)][string]$ConsoleText,
        [Parameter(Mandatory = $true)][string]$Suite,
        [Parameter(Mandatory = $true)][string[]]$ExpectedCases,
        [Parameter(Mandatory = $true)][string]$BuildSummary
    )

    $lines = @($ConsoleText -split "`r?`n")
    $timestampedScriptPrefix =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+:\s+'
    $cliPattern = $timestampedScriptPrefix +
        'CLI autotest suite:\s*(?<suite>[A-Za-z_][A-Za-z0-9_]*)\s*$'
    $suitePattern = $timestampedScriptPrefix +
        'TestSuite #(?<suite>[A-Za-z_][A-Za-z0-9_]*) started\s*$'
    $successPattern = $timestampedScriptPrefix +
        '(?:\u2705\s+)?(?<case>HST_TEST_[A-Za-z0-9_]+): SUCCESS\s*$'
    $runnerPattern = $timestampedScriptPrefix +
        'SCR_TestRunner has finished running\s*$'
    $junitPattern = $timestampedScriptPrefix +
        'Autotest JUnit XML saved to:\s*\$logs:/junit\.xml\s*$'
    $failedPattern = $timestampedScriptPrefix +
        'Autotest failed list saved to:\s*\$logs:/autotest_failed\.log\s*$'
    $stockPattern =
        "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): " +
        "Can't instantiate class 'SCR_FilterCategory', constructor is not public\s*$"
    $intentionalPattern =
        "^\s*\d{2}:\d{2}:\d{2}\.\d+\s+SCRIPT\s+\(E\): " +
        "string failureDetail = 'Partisan persistence \| native save " +
        'callback failure \| sequence/type/flags 1/0/0 \| ' +
        'manager/enabled/allowed/busy/active/playthrough 1/1/1/0/0/0 \| ' +
        'types/persistence/state/loaded/tracked/config/staged ' +
        '5/1/2/0/0/0/1 \| replication mode 0 \| snapshot fingerprint ''\s*$'
    $summaryPattern = 'failed native callback non-mutating 1'
    $seamPattern = 'setup/seam/request/bytes/journal 1/1/1/1/1'

    $cliRows = New-Object Collections.Generic.List[object]
    $suiteRows = New-Object Collections.Generic.List[object]
    $successRows = New-Object Collections.Generic.List[object]
    $runnerRows = New-Object Collections.Generic.List[int]
    $junitRows = New-Object Collections.Generic.List[int]
    $failedRows = New-Object Collections.Generic.List[int]
    $hardRows = New-Object Collections.Generic.List[object]
    $stockRows = New-Object Collections.Generic.List[int]
    $intentionalRows = New-Object Collections.Generic.List[int]
    $ambientRows = New-Object Collections.Generic.List[string]
    $unapprovedRows = New-Object Collections.Generic.List[int]
    $malformedHardRows = New-Object Collections.Generic.List[int]
    $hardRowPattern =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+' +
        '(?<channel>[A-Z][A-Z0-9_ ]*?)\s+' +
        '\((?<severity>[EF])\):\s*(?<message>.*)$'
    $hardTokenPattern =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+.*\([EF]\):'
    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = [string]$lines[$index]
        $match = [regex]::Match($line, $cliPattern)
        if ($match.Success) {
            [void]$cliRows.Add([pscustomobject]@{
                index = $index
                value = $match.Groups['suite'].Value
            })
        }
        $match = [regex]::Match($line, $suitePattern)
        if ($match.Success) {
            [void]$suiteRows.Add([pscustomobject]@{
                index = $index
                value = $match.Groups['suite'].Value
            })
        }
        $match = [regex]::Match($line, $successPattern)
        if ($match.Success) {
            [void]$successRows.Add([pscustomobject]@{
                index = $index
                value = $match.Groups['case'].Value
            })
        }
        if ($line -cmatch $runnerPattern) {
            [void]$runnerRows.Add($index)
        }
        if ($line -cmatch $junitPattern) {
            [void]$junitRows.Add($index)
        }
        if ($line -cmatch $failedPattern) {
            [void]$failedRows.Add($index)
        }
        $hardMatch = [regex]::Match($line, $hardRowPattern)
        if ($hardMatch.Success) {
            [void]$hardRows.Add([pscustomobject]@{
                index = $index
                line = $line
            })
            if ($line -cmatch $stockPattern) {
                [void]$stockRows.Add($index)
            }
            elseif ($line -cmatch $intentionalPattern) {
                [void]$intentionalRows.Add($index)
            }
            else {
                $signature =
                    $hardMatch.Groups['channel'].Value.Trim() + "`t" +
                    $hardMatch.Groups['severity'].Value + "`t" +
                    $hardMatch.Groups['message'].Value
                if ($script:SourceFocusedApprovedAmbientErrors -ccontains
                        $signature) {
                    [void]$ambientRows.Add($signature)
                }
                else {
                    [void]$unapprovedRows.Add($index)
                }
            }
        }
        elseif ($line -cmatch $hardTokenPattern) {
            [void]$malformedHardRows.Add($index)
        }
    }

    $expectedSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($caseName in $ExpectedCases) {
        [void]$expectedSet.Add([string]$caseName)
    }
    $successSet = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $successIdentityExact = $successRows.Count -eq $ExpectedCases.Count
    foreach ($row in $successRows) {
        if (-not $expectedSet.Contains([string]$row.value) -or
            -not $successSet.Add([string]$row.value)) {
            $successIdentityExact = $false
        }
    }
    if ($successSet.Count -ne $expectedSet.Count) {
        $successIdentityExact = $false
    }

    $markerOrderExact = $cliRows.Count -eq 1 -and
        $cliRows[0].value -ceq $Suite -and
        $suiteRows.Count -eq 1 -and
        $suiteRows[0].value -ceq $Suite -and
        $successRows.Count -gt 0 -and
        $runnerRows.Count -eq 1 -and
        $junitRows.Count -eq 1 -and
        $failedRows.Count -eq 1 -and
        $cliRows[0].index -lt $suiteRows[0].index -and
        $suiteRows[0].index -lt $successRows[0].index -and
        $successRows[$successRows.Count - 1].index -lt $runnerRows[0] -and
        $runnerRows[0] -lt $junitRows[0] -and
        $junitRows[0] -lt $failedRows[0]

    $profileSuite = $Suite -ceq
        'HST_CampaignProfileJournalAuthorityAutotestSuite'
    $profileProofExact = -not $profileSuite -and
        $intentionalRows.Count -eq 0 -and
        @($lines | Where-Object { $_ -cmatch $summaryPattern }).Count -eq 0 -and
        @($lines | Where-Object { $_ -cmatch $seamPattern }).Count -eq 0
    if ($profileSuite -and $successIdentityExact -and
        $intentionalRows.Count -eq $ExpectedCases.Count) {
        $profileProofExact = $true
        $intervalFloor = $suiteRows[0].index
        foreach ($successRow in $successRows) {
            $intentionalInInterval = @($intentionalRows | Where-Object {
                $_ -gt $intervalFloor -and $_ -lt $successRow.index
            })
            $summaryInInterval = @((($intervalFloor + 1)..($successRow.index - 1)) |
                Where-Object { [string]$lines[$_] -cmatch $summaryPattern })
            $seamInInterval = @((($intervalFloor + 1)..($successRow.index - 1)) |
                Where-Object { [string]$lines[$_] -cmatch $seamPattern })
            $requiresSeam = [string]$successRow.value -ceq
                'HST_TEST_CampaignProfileJournalAuthority_FailedNativeCallbackNonMutating'
            if ($intentionalInInterval.Count -ne 1 -or
                $summaryInInterval.Count -ne 1 -or
                $intentionalInInterval[0] -ge $summaryInInterval[0] -or
                ($requiresSeam -and
                    ($seamInInterval.Count -ne 1 -or
                        $summaryInInterval[0] -ge $seamInInterval[0])) -or
                (-not $requiresSeam -and $seamInInterval.Count -ne 0)) {
                $profileProofExact = $false
                break
            }
            $intervalFloor = $successRow.index
        }
    }

    $stockExact = $stockRows.Count -eq 2 -and
        $failedRows.Count -eq 1 -and
        $stockRows[0] -gt $failedRows[0] -and
        $stockRows[1] -gt $failedRows[0]
    $expectedHardCount = 2
    if ($profileSuite) {
        $expectedHardCount += $ExpectedCases.Count
    }
    $ambientExact = $ambientRows.Count -eq 0
    if ($ambientRows.Count -eq
            $script:SourceFocusedApprovedAmbientErrors.Count) {
        $actualAmbient = [string[]]@($ambientRows)
        $expectedAmbient = [string[]]@(
            $script:SourceFocusedApprovedAmbientErrors)
        [Array]::Sort($actualAmbient, [StringComparer]::Ordinal)
        [Array]::Sort($expectedAmbient, [StringComparer]::Ordinal)
        $ambientExact = $true
        for ($ambientIndex = 0;
                $ambientIndex -lt $expectedAmbient.Count;
                $ambientIndex++) {
            if ($actualAmbient[$ambientIndex] -cne
                    $expectedAmbient[$ambientIndex]) {
                $ambientExact = $false
                break
            }
        }
    }
    $expectedHardCount += $ambientRows.Count
    $hardDiagnosticsExact = $stockExact -and $profileProofExact -and
        $ambientExact -and $unapprovedRows.Count -eq 0 -and
        $malformedHardRows.Count -eq 0 -and
        $hardRows.Count -eq $expectedHardCount
    $buildSummaryCount = [regex]::Matches(
        $ConsoleText, [regex]::Escape($BuildSummary)).Count
    $valid = $successIdentityExact -and $markerOrderExact -and
        $hardDiagnosticsExact -and $buildSummaryCount -ge 1
    return [pscustomobject][ordered]@{
        valid = $valid
        markerOrderExact = $markerOrderExact
        successIdentityExact = $successIdentityExact
        successfulCaseCount = $successRows.Count
        successfulCases = @($successRows | ForEach-Object { $_.value })
        buildSummaryCount = $buildSummaryCount
        hardDiagnosticsExact = $hardDiagnosticsExact
        hardDiagnosticCount = $hardRows.Count
        approvedStockDiagnosticCount =
            $stockRows.Count + $ambientRows.Count
        approvedScriptStockDiagnosticCount = $stockRows.Count
        approvedAmbientDiagnosticCount = $ambientRows.Count
        approvedIntentionalDiagnosticCount = $intentionalRows.Count
        unapprovedHardDiagnosticCount = $unapprovedRows.Count
        malformedHardDiagnosticCount = $malformedHardRows.Count
        ambientDiagnosticsExact = $ambientExact
        profileProofExact = $profileProofExact
    }
}

function Copy-SourceFocusedRawEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$ScratchRoot,
        [Parameter(Mandatory = $true)][string]$RawRoot
    )

    Assert-SourceFocusedTreeHasNoReparsePoint -Root $ScratchRoot
    New-Item -ItemType Directory -Path $RawRoot -Force | Out-Null
    $scratchFull = [IO.Path]::GetFullPath($ScratchRoot).TrimEnd('\', '/')
    $scratchPrefix = $scratchFull + [IO.Path]::DirectorySeparatorChar
    $copied = New-Object Collections.Generic.List[object]
    foreach ($file in @(Get-ChildItem `
            -LiteralPath $scratchFull `
            -Recurse -File -Force -ErrorAction Stop | Where-Object {
                $_.Extension -cin @('.log', '.rpt', '.xml')
            } | Sort-Object FullName)) {
        $full = [IO.Path]::GetFullPath($file.FullName)
        if (-not $full.StartsWith(
                $scratchPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A raw source-focused artifact escaped its scratch root.'
        }
        $relative = $full.Substring($scratchPrefix.Length).Replace('\', '/')
        $destination = Join-Path $RawRoot $relative
        New-Item -ItemType Directory `
            -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $full -Destination $destination -Force
        $sourceHash = (Get-FileHash `
            -LiteralPath $full -Algorithm SHA256).Hash.ToLowerInvariant()
        $destinationHash = (Get-FileHash `
            -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($sourceHash -cne $destinationHash) {
            throw 'A retained source-focused artifact changed during copy.'
        }
        [void]$copied.Add([pscustomobject][ordered]@{
            relativePath = $relative
            length = [long]$file.Length
            sha256 = $sourceHash
        })
    }
    return $copied.ToArray()
}

function Get-SourceFocusedArtifactIntegrity {
    param([Parameter(Mandatory = $true)][string]$EvidenceRunRoot)

    $root = [IO.Path]::GetFullPath($EvidenceRunRoot).TrimEnd('\', '/')
    $prefix = $root + [IO.Path]::DirectorySeparatorChar
    $filesByPath = New-Object `
        'Collections.Generic.Dictionary[string,object]' `
        ([StringComparer]::Ordinal)
    $seenPathsIgnoreCase = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in @(Get-ChildItem `
            -LiteralPath $root -Recurse -File -Force -ErrorAction Stop |
            Where-Object {
                $_.Name -cne 'run.json' -and
                $_.Name -cne 'run.json.sha256'
            })) {
        $full = [IO.Path]::GetFullPath($file.FullName)
        if (-not $full.StartsWith(
                $prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A retained source-focused artifact escaped its run root.'
        }
        $relative = $full.Substring($prefix.Length).Replace('\', '/')
        if ($filesByPath.ContainsKey($relative) -or
            -not $seenPathsIgnoreCase.Add($relative)) {
            throw 'A retained source-focused artifact path is duplicated.'
        }
        $filesByPath.Add($relative, [pscustomobject][ordered]@{
            path = $relative
            length = [long]$file.Length
            sha256 = (Get-FileHash `
                -LiteralPath $full -Algorithm SHA256).Hash.ToLowerInvariant()
        })
    }
    $sortedPaths = [string[]]@($filesByPath.Keys)
    [Array]::Sort($sortedPaths, [StringComparer]::Ordinal)
    $files = New-Object Collections.Generic.List[object]
    foreach ($relative in $sortedPaths) {
        [void]$files.Add($filesByPath[$relative])
    }
    $canonical = (($files | ForEach-Object {
        [string]$_.path + "`t" + [string]$_.length + "`t" +
            [string]$_.sha256
    }) -join "`n")
    if ($files.Count -gt 0) {
        $canonical += "`n"
    }
    return [pscustomobject][ordered]@{
        hashAlgorithm = 'sha256-file-set-v1'
        artifactCount = $files.Count
        artifactSetSha256 = Get-SourceFocusedSha256Text -Text $canonical
        files = $files.ToArray()
    }
}

function Get-SourceFocusedArgumentBinding {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$Suite
    )

    $canonical = ($Arguments -join ([char]0)) + [char]0
    return [pscustomobject][ordered]@{
        policy = 'utf8-nul-separated-terminal-nul-v1'
        actualVectorSha256 = Get-SourceFocusedSha256Text -Text $canonical
        template = @(
            '-addonsDir', '<installed-base-addon-root>',
            '-gproj', '<source-checkout>/addon.gproj',
            '-addonTempDir', '<suite-scratch>/addon-temp',
            '-window',
            '-noFocus',
            '-forceupdate',
            '-rpl-timeout-disable',
            '-noThrow',
            '-profile', '<suite-scratch>',
            '-autotest', $Suite
        )
    }
}

function Get-SourceFocusedProfileMountBinding {
    param(
        [Parameter(Mandatory = $true)][string]$Suite,
        [Parameter(Mandatory = $true)][string]$ScratchRoot
    )

    $argumentPath = [IO.Path]::GetFullPath($ScratchRoot)
    $profilePath = [IO.Path]::GetFullPath(
        (Join-Path $argumentPath 'profile'))
    if (-not (Test-SourceFocusedContainedPath `
            -Root $argumentPath -Candidate $profilePath)) {
        throw 'The source-focused profile mount escaped its suite scratch root.'
    }
    $proofAuthorizationRequired =
        $Suite -ceq $script:SourceFocusedProfileProofSuite
    $proofSentinelPath = ''
    if ($proofAuthorizationRequired) {
        $proofSentinelPath = Join-Path `
            $profilePath $script:SourceFocusedProfileProofSentinel
        if (-not (Test-SourceFocusedContainedPath `
                -Root $profilePath -Candidate $proofSentinelPath)) {
            throw 'The source-focused profile proof sentinel escaped its mount.'
        }
    }
    return [pscustomobject][ordered]@{
        policy = 'profile-argument-parent-mount-v1'
        argumentPath = $argumentPath
        profilePath = $profilePath
        proofAuthorizationRequired = $proofAuthorizationRequired
        proofSentinelPath = $proofSentinelPath
        proofSentinelValueSha256 = if ($proofAuthorizationRequired) {
            Get-SourceFocusedSha256Text `
                -Text $script:SourceFocusedProfileProofSentinelValue
        }
        else { '' }
    }
}

function Initialize-SourceFocusedProfileProofAuthorization {
    param([Parameter(Mandatory = $true)]$ProfileMountBinding)

    if (-not [bool]$ProfileMountBinding.proofAuthorizationRequired) {
        return $false
    }
    if (-not (Test-Path `
            -LiteralPath $ProfileMountBinding.profilePath `
            -PathType Container) -or
        -not (Test-SourceFocusedContainedPath `
            -Root $ProfileMountBinding.profilePath `
            -Candidate $ProfileMountBinding.proofSentinelPath)) {
        throw 'The source-focused profile proof authorization boundary is invalid.'
    }
    Assert-SourceFocusedTreeHasNoReparsePoint `
        -Root $ProfileMountBinding.profilePath
    [IO.File]::WriteAllText(
        $ProfileMountBinding.proofSentinelPath,
        $script:SourceFocusedProfileProofSentinelValue,
        (New-Object Text.UTF8Encoding($false, $true)))
    $sentinelIdentity = Get-SourceFocusedFileIdentity `
        -Path $ProfileMountBinding.proofSentinelPath
    if ([IO.File]::ReadAllText(
            $ProfileMountBinding.proofSentinelPath) -cne
            $script:SourceFocusedProfileProofSentinelValue -or
        [string]$sentinelIdentity.sha256 -cne
            [string]$ProfileMountBinding.proofSentinelValueSha256) {
        throw 'The source-focused profile proof authorization sentinel is invalid.'
    }
    return $true
}

function Get-SourceFocusedSchemaBinding {
    param([Parameter(Mandatory = $true)][string]$CheckoutRoot)

    $definitions = @(
        [pscustomobject]@{
            name = 'campaign'
            relativePath = 'Scripts/Game/HST/State/HST_CampaignState.c'
            expected = 71
        },
        [pscustomobject]@{
            name = 'runtime-settings'
            relativePath = 'Scripts/Game/HST/Config/HST_RuntimeSettings.c'
            expected = 24
        }
    )
    $result = [ordered]@{}
    foreach ($definition in $definitions) {
        $path = Resolve-SourceFocusedPath `
            -Path (Join-Path $CheckoutRoot $definition.relativePath) `
            -Kind Leaf
        $text = [IO.File]::ReadAllText($path)
        $matches = [regex]::Matches(
            $text,
            '(?m)^\s*static\s+const\s+int\s+SCHEMA_VERSION\s*=\s*(?<value>\d+)\s*;\s*$')
        if ($matches.Count -ne 1 -or
            [int]$matches[0].Groups['value'].Value -ne $definition.expected) {
            throw "The $($definition.name) schema contract is invalid."
        }
        $result[$definition.name] = [pscustomobject][ordered]@{
            value = [int]$matches[0].Groups['value'].Value
            relativePath = $definition.relativePath
            sha256 = (Get-FileHash `
                -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
    return [pscustomobject]$result
}

function Get-SourceFocusedRuntimeToolchainBinding {
    param(
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonRootPath
    )

    $coreProject = Join-Path $RuntimeAddonRootPath 'core/core.gproj'
    $dataProject = Join-Path $RuntimeAddonRootPath 'data/ArmaReforger.gproj'
    $coreIdentity = Get-SourceFocusedFileIdentity -Path $coreProject
    $dataIdentity = Get-SourceFocusedFileIdentity -Path $dataProject
    $coreText = [IO.File]::ReadAllText($coreProject)
    $dataText = [IO.File]::ReadAllText($dataProject)
    if ($coreText -cnotmatch '(?m)^\s*GUID\s+"5614BBCCBB55ED1C"\s*$' -or
        $dataText -cnotmatch '(?m)^\s*GUID\s+"58D0FB3206B6F859"\s*$') {
        throw 'The installed base-addon project identities are invalid.'
    }
    return [pscustomobject][ordered]@{
        executable = Get-SourceFocusedFileIdentity `
            -Path $ExecutablePath -IncludeVersion
        runtimeAddonRootPath = $RuntimeAddonRootPath
        runtimeAddonRootPathSha256 = Get-SourceFocusedSha256Text `
            -Text ((ConvertTo-SourceFocusedNormalizedPath `
                -Path $RuntimeAddonRootPath).ToLowerInvariant() + "`n")
        coreProject = $coreIdentity
        dataProject = $dataIdentity
    }
}

function Assert-SourceFocusedRuntimeToolchainBinding {
    param([Parameter(Mandatory = $true)]$Expected)

    $actual = Get-SourceFocusedRuntimeToolchainBinding `
        -ExecutablePath $Expected.executable.path `
        -RuntimeAddonRootPath $Expected.runtimeAddonRootPath
    if (-not (Test-SourceFocusedFileIdentityEqual `
            -Expected $Expected.executable -Actual $actual.executable) -or
        -not (Test-SourceFocusedFileIdentityEqual `
            -Expected $Expected.coreProject -Actual $actual.coreProject) -or
        -not (Test-SourceFocusedFileIdentityEqual `
            -Expected $Expected.dataProject -Actual $actual.dataProject) -or
        [string]$Expected.runtimeAddonRootPathSha256 -cne
            [string]$actual.runtimeAddonRootPathSha256) {
        throw 'The diagnostic executable or installed base-addon roots changed.'
    }
    return $actual
}

function ConvertTo-SourceFocusedPortableFileIdentity {
    param(
        [Parameter(Mandatory = $true)]$Identity,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $result = [ordered]@{
        name = $Name
        length = [long]$Identity.length
        lastWriteUtc = [string]$Identity.lastWriteUtc
        sha256 = [string]$Identity.sha256
    }
    if ($Identity.PSObject.Properties.Name -contains 'fileVersion') {
        $result.fileVersion = [string]$Identity.fileVersion
        $result.productVersion = [string]$Identity.productVersion
        $result.productName = [string]$Identity.productName
    }
    return [pscustomobject]$result
}

function Invoke-SourceFocusedSuite {
    param(
        [Parameter(Mandatory = $true)][int]$Ordinal,
        [Parameter(Mandatory = $true)][string]$Suite,
        [Parameter(Mandatory = $true)][string[]]$ExpectedCases,
        [Parameter(Mandatory = $true)][string]$RunRoot,
        [Parameter(Mandatory = $true)][string]$RunNonce,
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonRootPath,
        [Parameter(Mandatory = $true)][string]$ProjectPath,
        [Parameter(Mandatory = $true)]$ProjectBinding,
        [Parameter(Mandatory = $true)]$BuildIdentity,
        [Parameter(Mandatory = $true)]$GitBinding,
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)]$ToolchainBinding,
        [Parameter(Mandatory = $true)][int]$TimeoutSeconds,
        [Parameter(Mandatory = $true)][int]$PollIntervalMilliseconds
    )

    $suiteLeaf = ('{0:d2}-{1}' -f $Ordinal, $Suite)
    $suiteRoot = [IO.Path]::GetFullPath((Join-Path $RunRoot "suites/$suiteLeaf"))
    $scratchRoot = [IO.Path]::GetFullPath((Join-Path $suiteRoot 'scratch'))
    $rawRoot = [IO.Path]::GetFullPath((Join-Path $suiteRoot 'raw'))
    if (-not (Test-SourceFocusedContainedPath `
            -Root $RunRoot -Candidate $suiteRoot) -or
        -not (Test-SourceFocusedContainedPath `
            -Root $suiteRoot -Candidate $scratchRoot) -or
        (Test-Path -LiteralPath $suiteRoot)) {
        throw 'The source-focused suite evidence root is not fresh and contained.'
    }
    New-Item -ItemType Directory -Path $scratchRoot -Force | Out-Null
    Assert-SourceFocusedTreeHasNoReparsePoint -Root $scratchRoot
    $sentinelPath = Join-Path $scratchRoot '.partisan-source-focused-owner'
    $sentinelValue = $RunNonce + ':' + $Suite
    [IO.File]::WriteAllText(
        $sentinelPath,
        $sentinelValue,
        (New-Object Text.UTF8Encoding($false, $true)))
    $workingDirectory = Join-Path $scratchRoot 'work'
    $tempDirectory = Join-Path $scratchRoot 'temp'
    $profileMountBinding = Get-SourceFocusedProfileMountBinding `
        -Suite $Suite -ScratchRoot $scratchRoot
    $profileDirectory = $profileMountBinding.profilePath
    $addonTempDirectory = Join-Path $scratchRoot 'addon-temp'
    New-Item -ItemType Directory `
        -Path $workingDirectory, $tempDirectory, $profileDirectory,
            $addonTempDirectory `
        -Force | Out-Null
    $profileProofAuthorized =
        Initialize-SourceFocusedProfileProofAuthorization `
            -ProfileMountBinding $profileMountBinding

    $arguments = @(
        '-addonsDir', $RuntimeAddonRootPath,
        '-gproj', $ProjectPath,
        '-addonTempDir', $addonTempDirectory,
        '-window',
        '-noFocus',
        '-forceupdate',
        '-rpl-timeout-disable',
        '-noThrow',
        '-profile', $profileMountBinding.argumentPath,
        '-autotest', $Suite
    )
    $argumentBinding = Get-SourceFocusedArgumentBinding `
        -Arguments $arguments -Suite $Suite
    $commandLine = (ConvertTo-SourceFocusedNativeArgument `
        -Value $ExecutablePath) + ' ' + (($arguments | ForEach-Object {
        ConvertTo-SourceFocusedNativeArgument -Value ([string]$_)
    }) -join ' ')
    if (-not (Test-SourceFocusedNativeArgumentVector `
            -CommandLine $commandLine `
            -ExpectedExecutable $ExecutablePath `
            -ExpectedArguments $arguments)) {
        throw 'The source-focused argument vector did not round-trip exactly.'
    }

    $startedUtc = [DateTime]::UtcNow
    $completedUtc = $null
    $oldTemp = $env:TEMP
    $oldTmp = $env:TMP
    $job = $null
    $launcher = $null
    $process = $null
    $ownedIds = @{}
    $ownedRows = New-Object `
        'Collections.Generic.Dictionary[int,object]'
    $unclaimedRows = New-Object `
        'Collections.Generic.Dictionary[string,object]' `
        ([StringComparer]::Ordinal)
    $beforeRows = @(Get-SourceFocusedProcessRows `
        -Processes (Get-SourceFocusedEngineProcesses))
    $afterRows = @()
    $exitCode = $null
    $junit = $null
    $console = $null
    $mount = $null
    $failedListBytes = $null
    $commandLineVerified = $false
    $sourceBindingsStable = $false
    $rawArtifacts = @()
    $cleanupErrors = New-Object Collections.Generic.List[string]
    $runError = ''

    try {
        if ($beforeRows.Count -ne 0) {
            throw 'An engine process was already active before the suite.'
        }
        [void](Assert-SourceFocusedGitBinding `
            -Expected $GitBinding `
            -CheckoutRoot $CheckoutRoot `
            -RunnerPath $RunnerPath)
        [void](Assert-SourceFocusedRuntimeToolchainBinding `
            -Expected $ToolchainBinding)

        $job = New-Object PartisanSourceFocusedJob
        if (@(Get-SourceFocusedEngineProcesses).Count -ne 0) {
            throw 'An engine process appeared during source-focused preflight.'
        }
        try {
            $env:TEMP = $tempDirectory
            $env:TMP = $tempDirectory
            $launcher = New-Object PartisanSourceFocusedSuspendedProcess(
                $ExecutablePath, $commandLine, $workingDirectory)
        }
        finally {
            $env:TEMP = $oldTemp
            $env:TMP = $oldTmp
        }
        $process = $launcher.Child
        if (-not $process) {
            throw 'The diagnostic source-focused process did not start.'
        }
        $ownedIds[[int]$process.Id] = $true
        $job.Add($process)
        $launcher.Resume()
        $launcher.Dispose()
        $launcher = $null

        Start-Sleep -Milliseconds 250
        $process.Refresh()
        if ($process.HasExited) {
            throw 'The source-focused process exited before launch verification.'
        }
        $processRow = Get-CimInstance `
            Win32_Process `
            -Filter "ProcessId=$($process.Id)" `
            -ErrorAction Stop
        if (-not $processRow -or
            -not (Test-SourceFocusedNativeArgumentVector `
                -CommandLine ([string]$processRow.CommandLine) `
                -ExpectedExecutable $ExecutablePath `
                -ExpectedArguments $arguments)) {
            throw 'The live source-focused argument vector was not exact.'
        }
        $commandLineVerified = $true

        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        while ([DateTime]::UtcNow -lt $deadline) {
            foreach ($jobId in @($job.GetProcessIds())) {
                $ownedIds[[int]$jobId] = $true
            }
            foreach ($candidate in @(Get-SourceFocusedEngineProcesses)) {
                $candidateId = [int]$candidate.Id
                if ($ownedIds.ContainsKey($candidateId)) {
                    if (-not $ownedRows.ContainsKey($candidateId)) {
                        $ownedRows[$candidateId] = @(
                            Get-SourceFocusedProcessRows -Processes @($candidate))[0]
                    }
                    continue
                }
                $identity = $candidate.ProcessName + ':' + $candidateId
                if (-not $unclaimedRows.ContainsKey($identity)) {
                    $unclaimedRows[$identity] = @(
                        Get-SourceFocusedProcessRows -Processes @($candidate))[0]
                }
            }
            if ($unclaimedRows.Count -ne 0) {
                throw 'An unowned engine process appeared during the suite.'
            }
            $process.Refresh()
            if ($process.HasExited -and @($job.GetProcessIds()).Count -eq 0) {
                break
            }
            Start-Sleep -Milliseconds $PollIntervalMilliseconds
        }
        foreach ($jobId in @($job.GetProcessIds())) {
            $ownedIds[[int]$jobId] = $true
        }
        $process.Refresh()
        if (-not $process.HasExited -or @($job.GetProcessIds()).Count -ne 0) {
            throw 'The source-focused suite timed out.'
        }
        $exitCode = [int]$process.ExitCode
        Start-Sleep -Milliseconds 1000

        [void](Assert-SourceFocusedGitBinding `
            -Expected $GitBinding `
            -CheckoutRoot $CheckoutRoot `
            -RunnerPath $RunnerPath)
        [void](Assert-SourceFocusedRuntimeToolchainBinding `
            -Expected $ToolchainBinding)
        $projectAfter = Get-SourceFocusedProjectBinding -ProjectPath $ProjectPath
        $buildAfter = Get-SourceFocusedBuildIdentity `
            -Path $BuildIdentity.file.path
        if (-not (Test-SourceFocusedFileIdentityEqual `
                -Expected $ProjectBinding.file -Actual $projectAfter.file) -or
            -not (Test-SourceFocusedFileIdentityEqual `
                -Expected $BuildIdentity.file -Actual $buildAfter.file)) {
            throw 'The source addon identity changed during the suite.'
        }
        $sourceBindingsStable = $true

        $junitFiles = @(Get-ChildItem `
            -LiteralPath $scratchRoot -Recurse -File `
            -Filter 'junit.xml' -ErrorAction SilentlyContinue)
        $failedFiles = @(Get-ChildItem `
            -LiteralPath $scratchRoot -Recurse -File `
            -Filter 'autotest_failed.log' -ErrorAction SilentlyContinue)
        $consoleFiles = @(Get-ChildItem `
            -LiteralPath $scratchRoot -Recurse -File `
            -Filter 'console.log' -ErrorAction SilentlyContinue)
        if ($junitFiles.Count -ne 1 -or $failedFiles.Count -ne 1 -or
            $consoleFiles.Count -ne 1) {
            throw 'The source-focused suite did not produce one exact evidence triplet.'
        }
        $junit = Get-SourceFocusedJUnitContract `
            -XmlText ([IO.File]::ReadAllText($junitFiles[0].FullName)) `
            -Suite $Suite `
            -ExpectedCases $ExpectedCases
        $consoleText = [IO.File]::ReadAllText($consoleFiles[0].FullName)
        $console = Get-SourceFocusedConsoleContract `
            -ConsoleText $consoleText `
            -Suite $Suite `
            -ExpectedCases $ExpectedCases `
            -BuildSummary $BuildIdentity.summary
        $mount = Get-SourceFocusedMountContract `
            -ConsoleText $consoleText `
            -ProjectPath $ProjectPath `
            -ProjectGuid $ProjectBinding.guid `
            -ProjectId $ProjectBinding.id
        $failedListBytes = [long]$failedFiles[0].Length
        if ($exitCode -ne 0 -or -not $junit.valid -or -not $console.valid -or
            -not $mount.valid -or $failedListBytes -ne 0) {
            throw 'The source-focused suite evidence did not pass its exact contract.'
        }
    }
    catch {
        $runError = ConvertTo-SourceFocusedSafeText `
            -Text $_.Exception.Message `
            -RedactedPaths @(
                $CheckoutRoot,
                $ExecutablePath,
                $RuntimeAddonRootPath,
                $RunRoot,
                $scratchRoot)
    }
    finally {
        $env:TEMP = $oldTemp
        $env:TMP = $oldTmp
        if ($launcher) {
            try { $launcher.Dispose() }
            catch { [void]$cleanupErrors.Add($_.Exception.Message) }
            $launcher = $null
        }
        if ($job) {
            try {
                foreach ($jobId in @($job.GetProcessIds())) {
                    $ownedIds[[int]$jobId] = $true
                }
            }
            catch { [void]$cleanupErrors.Add($_.Exception.Message) }
            try { $job.Dispose() }
            catch { [void]$cleanupErrors.Add($_.Exception.Message) }
            $job = $null
        }
        if ($process) {
            try { $process.Dispose() }
            catch { [void]$cleanupErrors.Add($_.Exception.Message) }
            $process = $null
        }
        for ($attempt = 0; $attempt -lt 50; $attempt++) {
            $remaining = @(Get-SourceFocusedEngineProcesses)
            if ($remaining.Count -eq 0) {
                break
            }
            Start-Sleep -Milliseconds 100
        }
        $afterProcesses = @(Get-SourceFocusedEngineProcesses)
        $afterRows = @(Get-SourceFocusedProcessRows -Processes $afterProcesses)
        foreach ($candidate in $afterProcesses) {
            $candidateId = [int]$candidate.Id
            if (-not $ownedIds.ContainsKey($candidateId)) {
                $identity = $candidate.ProcessName + ':' + $candidateId
                if (-not $unclaimedRows.ContainsKey($identity)) {
                    $unclaimedRows[$identity] = @(
                        Get-SourceFocusedProcessRows -Processes @($candidate))[0]
                }
            }
        }
        $profileProofSentinelStable =
            -not $profileMountBinding.proofAuthorizationRequired
        if ($profileMountBinding.proofAuthorizationRequired) {
            try {
                $profileProofSentinelStable =
                    (Test-Path `
                        -LiteralPath $profileMountBinding.proofSentinelPath `
                        -PathType Leaf) -and
                    ([IO.File]::ReadAllText(
                        $profileMountBinding.proofSentinelPath) -ceq
                        $script:SourceFocusedProfileProofSentinelValue) -and
                    ((Get-FileHash `
                        -LiteralPath $profileMountBinding.proofSentinelPath `
                        -Algorithm SHA256).Hash.ToLowerInvariant() -ceq
                        [string]$profileMountBinding.proofSentinelValueSha256)
            }
            catch { $profileProofSentinelStable = $false }
        }
        try {
            if (Test-Path -LiteralPath $scratchRoot -PathType Container) {
                $rawArtifacts = @(Copy-SourceFocusedRawEvidence `
                    -ScratchRoot $scratchRoot -RawRoot $rawRoot)
            }
        }
        catch { [void]$cleanupErrors.Add($_.Exception.Message) }
        if ($afterRows.Count -eq 0) {
            try {
                if (-not (Test-Path -LiteralPath $sentinelPath -PathType Leaf) -or
                    [IO.File]::ReadAllText($sentinelPath) -cne $sentinelValue -or
                    -not $profileProofSentinelStable -or
                    -not (Test-SourceFocusedContainedPath `
                        -Root $suiteRoot -Candidate $scratchRoot)) {
                    throw 'The suite scratch ownership boundary changed.'
                }
                Assert-SourceFocusedTreeHasNoReparsePoint -Root $scratchRoot
                Remove-Item -LiteralPath $scratchRoot -Recurse -Force
            }
            catch { [void]$cleanupErrors.Add($_.Exception.Message) }
        }
        else {
            [void]$cleanupErrors.Add(
                'Suite scratch cleanup was skipped while an engine process remained.')
        }
        try {
            [void](Assert-SourceFocusedGitBinding `
                -Expected $GitBinding `
                -CheckoutRoot $CheckoutRoot `
                -RunnerPath $RunnerPath)
            [void](Assert-SourceFocusedRuntimeToolchainBinding `
                -Expected $ToolchainBinding)
        }
        catch { [void]$cleanupErrors.Add($_.Exception.Message) }
        $completedUtc = [DateTime]::UtcNow
    }

    $safeCleanupErrors = @($cleanupErrors | ForEach-Object {
        ConvertTo-SourceFocusedSafeText `
            -Text ([string]$_) `
            -RedactedPaths @(
                $CheckoutRoot,
                $ExecutablePath,
                $RuntimeAddonRootPath,
                $RunRoot,
                $scratchRoot)
    })
    $cleanup = [pscustomobject][ordered]@{
        scratchRemaining = [int](Test-Path -LiteralPath $scratchRoot)
        engineProcessesRemaining = $afterRows.Count
        unclaimedEngineProcessesObserved = $unclaimedRows.Count
        cleanupErrorCount = $safeCleanupErrors.Count
        errors = $safeCleanupErrors
    }
    $success = [string]::IsNullOrEmpty($runError) -and
        $commandLineVerified -and $sourceBindingsStable -and
        $exitCode -eq 0 -and $junit -and $junit.valid -and
        $console -and $console.valid -and $mount -and $mount.valid -and
        $failedListBytes -eq 0 -and $rawArtifacts.Count -ge 3 -and
        $cleanup.scratchRemaining -eq 0 -and
        $cleanup.engineProcessesRemaining -eq 0 -and
        $cleanup.unclaimedEngineProcessesObserved -eq 0 -and
        $cleanup.cleanupErrorCount -eq 0

    return [pscustomobject][ordered]@{
        ordinal = $Ordinal
        suite = $Suite
        status = if ($success) { 'passed' } else { 'failed' }
        startedUtc = $startedUtc.ToString(
            'o', [Globalization.CultureInfo]::InvariantCulture)
        completedUtc = $completedUtc.ToString(
            'o', [Globalization.CultureInfo]::InvariantCulture)
        expectedCaseCount = $ExpectedCases.Count
        expectedCases = @($ExpectedCases)
        launch = [pscustomobject][ordered]@{
            arguments = $argumentBinding
            commandLineVerified = $commandLineVerified
            timeoutSeconds = $TimeoutSeconds
            exitCode = $exitCode
        }
        junit = $junit
        console = $console
        sourceMount = $mount
        failedListBytes = $failedListBytes
        sourceBindingsStable = $sourceBindingsStable
        rawArtifactCount = $rawArtifacts.Count
        processCensus = [pscustomobject][ordered]@{
            before = $beforeRows
            observedOwned = @($ownedRows.Values | Sort-Object id)
            observedUnclaimed = @($unclaimedRows.Values | Sort-Object id)
            after = $afterRows
        }
        cleanup = $cleanup
        profileMount = [pscustomobject][ordered]@{
            policy = $profileMountBinding.policy
            profileArgument = '<suite-scratch>'
            resolvedProfilePath = '<suite-scratch>/profile'
            proofAuthorizationRequired =
                $profileMountBinding.proofAuthorizationRequired
            proofAuthorizationEstablished = $profileProofAuthorized
            proofSentinelStableThroughExit = $profileProofSentinelStable
            proofSentinelPath = if (
                $profileMountBinding.proofAuthorizationRequired) {
                '<suite-scratch>/profile/.partisan-focused-owner'
            }
            else { '' }
            proofSentinelValueSha256 =
                $profileMountBinding.proofSentinelValueSha256
        }
        error = $runError
        success = $success
    }
}

function Test-SourceFocusedContracts {
    $checks = 0
    $seen = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $total = 0
    foreach ($suite in $script:SourceFocusedSuiteCases.Keys) {
        if ($suite -cnotmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
            throw 'Source-focused suite manifest self-test failed.'
        }
        foreach ($caseName in @($script:SourceFocusedSuiteCases[$suite])) {
            if ([string]$caseName -cnotmatch '^HST_TEST_[A-Za-z0-9_]+$' -or
                -not $seen.Add([string]$caseName)) {
                throw 'Source-focused case manifest self-test failed.'
            }
            $total++
        }
        $checks++
    }
    if ($total -ne 91 -or $seen.Count -ne 91 -or
        $script:SourceFocusedSuiteCases.Count -ne 5) {
        throw 'Source-focused manifest total self-test failed.'
    }
    $checks++

    if ((Get-SourceFocusedSha256Text -Text 'abc') -cne
        'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad') {
        throw 'Source-focused UTF-8 SHA-256 self-test failed.'
    }
    $checks++

    $syntheticRoot = Join-Path ([IO.Path]::GetTempPath()) 'Partisan Source Test'
    $syntheticExecutable = Join-Path `
        $syntheticRoot 'ArmaReforgerSteamDiag.exe'
    $syntheticRuntimeRoot = Join-Path $syntheticRoot 'Runtime Addons'
    $syntheticProject = Join-Path $syntheticRoot 'Source Root/addon.gproj'
    $syntheticArguments = @(
        '-addonsDir', $syntheticRuntimeRoot,
        '-gproj', $syntheticProject,
        '-autotest', 'HST_EnemyQRFAutotestSuite',
        '-noThrow'
    )
    $syntheticCommandLine =
        (ConvertTo-SourceFocusedNativeArgument -Value $syntheticExecutable) +
        ' ' + (($syntheticArguments | ForEach-Object {
            ConvertTo-SourceFocusedNativeArgument -Value ([string]$_)
        }) -join ' ')
    if (-not (Test-SourceFocusedNativeArgumentVector `
            -CommandLine $syntheticCommandLine `
            -ExpectedExecutable $syntheticExecutable `
            -ExpectedArguments $syntheticArguments)) {
        throw 'Source-focused native argument self-test failed.'
    }
    $checks++

    $profileSuite = $script:SourceFocusedProfileProofSuite
    $mountTestRoot = Join-Path `
        ([IO.Path]::GetTempPath()) `
        ('partisan-source-focused-profile-selftest-' +
            [Guid]::NewGuid().ToString('N'))
    if (Test-Path -LiteralPath $mountTestRoot) {
        throw 'Source-focused profile mount self-test root is not fresh.'
    }
    New-Item -ItemType Directory -Path $mountTestRoot -Force | Out-Null
    try {
        $profileMount = Get-SourceFocusedProfileMountBinding `
            -Suite $profileSuite -ScratchRoot $mountTestRoot
        New-Item -ItemType Directory `
            -Path $profileMount.profilePath -Force | Out-Null
        $profileAuthorized =
            Initialize-SourceFocusedProfileProofAuthorization `
                -ProfileMountBinding $profileMount
        $mountArguments = @(
            '-profile', $profileMount.argumentPath,
            '-autotest', $profileSuite)
        $mountArgumentBinding = Get-SourceFocusedArgumentBinding `
            -Arguments $mountArguments -Suite $profileSuite
        $templateProfileIndex = [Array]::IndexOf(
            [object[]]$mountArgumentBinding.template,
            '-profile')
        if (-not $profileAuthorized -or
            -not $profileMount.proofAuthorizationRequired -or
            [string]$profileMount.argumentPath -cne
                [IO.Path]::GetFullPath($mountTestRoot) -or
            [string]$profileMount.profilePath -cne
                [IO.Path]::GetFullPath((Join-Path $mountTestRoot 'profile')) -or
            [string]$profileMount.proofSentinelPath -cne
                [IO.Path]::GetFullPath((Join-Path `
                    $mountTestRoot 'profile/.partisan-focused-owner')) -or
            [IO.File]::ReadAllText(
                $profileMount.proofSentinelPath) -cne 'owned' -or
            $templateProfileIndex -lt 0 -or
            [string]$mountArgumentBinding.template[
                $templateProfileIndex + 1] -cne '<suite-scratch>') {
            throw 'Source-focused profile mount authorization self-test failed.'
        }
        $nonProfileMount = Get-SourceFocusedProfileMountBinding `
            -Suite 'HST_EnemyQRFAutotestSuite' `
            -ScratchRoot $mountTestRoot
        if ($nonProfileMount.proofAuthorizationRequired -or
            -not [string]::IsNullOrEmpty(
                [string]$nonProfileMount.proofSentinelPath)) {
            throw 'Source-focused profile authorization scope self-test failed.'
        }
    }
    finally {
        if (Test-Path -LiteralPath $mountTestRoot -PathType Container) {
            Assert-SourceFocusedTreeHasNoReparsePoint -Root $mountTestRoot
            Remove-Item -LiteralPath $mountTestRoot -Recurse -Force
        }
    }
    $checks++

    $qrfSuite = 'HST_EnemyQRFAutotestSuite'
    $qrfCases = @($script:SourceFocusedSuiteCases[$qrfSuite])
    $junitBuilder = New-Object Text.StringBuilder
    [void]$junitBuilder.Append('<testsuites><testsuite name="')
    [void]$junitBuilder.Append($qrfSuite)
    [void]$junitBuilder.Append('" tests="6">')
    foreach ($caseName in $qrfCases) {
        [void]$junitBuilder.Append('<testcase classname="')
        [void]$junitBuilder.Append($qrfSuite)
        [void]$junitBuilder.Append('" name="')
        [void]$junitBuilder.Append($caseName)
        [void]$junitBuilder.Append('" />')
    }
    [void]$junitBuilder.Append('</testsuite></testsuites>')
    $validJUnit = Get-SourceFocusedJUnitContract `
        -XmlText $junitBuilder.ToString() `
        -Suite $qrfSuite `
        -ExpectedCases $qrfCases
    if (-not $validJUnit.valid -or $validJUnit.tests -ne 6) {
        throw 'Source-focused green JUnit self-test failed.'
    }
    $checks++
    $junitRejected = $false
    try {
        [void](Get-SourceFocusedJUnitContract `
            -XmlText $junitBuilder.ToString().Replace(
                $qrfCases[0], 'HST_TEST_EnemyQRF_Unexpected') `
            -Suite $qrfSuite `
            -ExpectedCases $qrfCases)
    }
    catch { $junitRejected = $true }
    if (-not $junitRejected) {
        throw 'Source-focused JUnit mutation rejection self-test failed.'
    }
    $checks++

    $projectPath = $syntheticProject
    $projectForward = $projectPath.Replace('\', '/')
    $sourceRootForward = (Split-Path -Parent $projectPath).Replace('\', '/')
    $mountText = @(
        "17:00:00.000 ENGINE       : gproj: '$projectForward' guid: '698532771130111D'",
        "17:00:00.001 ENGINE       : FileSystem: Adding relative directory '$sourceRootForward' to filesystem under name histasi",
        "17:00:00.002 ENGINE       : gproj: '$projectForward' guid: '698532771130111D'",
        "17:00:00.003 RESOURCES    : ResourceDB: loading cache (id=0 name=histasi path=$sourceRootForward/resourceDatabase.rdb)"
    ) -join "`n"
    $mount = Get-SourceFocusedMountContract `
        -ConsoleText $mountText `
        -ProjectPath $projectPath `
        -ProjectGuid '698532771130111D' `
        -ProjectId 'histasi'
    if (-not $mount.valid -or $mount.sourcePackedRecordCount -ne 0) {
        throw 'Source-focused loose-source mount self-test failed.'
    }
    $checks++
    $packedMount = Get-SourceFocusedMountContract `
        -ConsoleText ($mountText.Replace(
            "guid: '698532771130111D'",
            "guid: '698532771130111D' (packed)")) `
        -ProjectPath $projectPath `
        -ProjectGuid '698532771130111D' `
        -ProjectId 'histasi'
    if ($packedMount.valid -or $packedMount.sourcePackedRecordCount -ne 2) {
        throw 'Source-focused packed-mount rejection self-test failed.'
    }
    $checks++

    $buildSummary = 'sha 0123456789012345678901234567890123456789' +
        ' | utc 2026-07-22T00:00:00Z | label self-test'
    $stockLine =
        "17:00:10.001 SCRIPT    (E): Can't instantiate class " +
        "'SCR_FilterCategory', constructor is not public"
    $qrfLines = New-Object Collections.Generic.List[string]
    [void]$qrfLines.Add('17:00:00.000 SCRIPT       : CLI autotest suite: ' + $qrfSuite)
    [void]$qrfLines.Add('17:00:00.001 SCRIPT       : TestSuite #' + $qrfSuite + ' started')
    $clock = 2
    foreach ($caseName in $qrfCases) {
        [void]$qrfLines.Add(('17:00:00.{0:d3} SCRIPT       : ' -f $clock) +
            $buildSummary)
        $clock++
        [void]$qrfLines.Add(('17:00:00.{0:d3} SCRIPT       : ' -f $clock) +
            ([string][char]0x2705) + ' ' + $caseName + ': SUCCESS')
        $clock++
    }
    [void]$qrfLines.Add('17:00:01.000 SCRIPT       : SCR_TestRunner has finished running')
    [void]$qrfLines.Add('17:00:01.001 SCRIPT       : Autotest JUnit XML saved to: $logs:/junit.xml')
    [void]$qrfLines.Add('17:00:01.002 SCRIPT       : Autotest failed list saved to: $logs:/autotest_failed.log')
    [void]$qrfLines.Add($stockLine)
    [void]$qrfLines.Add($stockLine)
    $qrfConsole = Get-SourceFocusedConsoleContract `
        -ConsoleText ($qrfLines.ToArray() -join "`n") `
        -Suite $qrfSuite `
        -ExpectedCases $qrfCases `
        -BuildSummary $buildSummary
    if (-not $qrfConsole.valid -or
        $qrfConsole.successfulCaseCount -ne $qrfCases.Count) {
        throw 'Source-focused console contract self-test failed.'
    }
    $checks++
    $unknownConsole = Get-SourceFocusedConsoleContract `
        -ConsoleText (($qrfLines.ToArray() +
            '17:00:10.002 RESOURCES (E): unexpected') -join "`n") `
        -Suite $qrfSuite `
        -ExpectedCases $qrfCases `
        -BuildSummary $buildSummary
    if ($unknownConsole.valid) {
        throw 'Source-focused unknown diagnostic rejection self-test failed.'
    }
    $checks++

    $ambientLines = New-Object Collections.Generic.List[string]
    foreach ($signature in $script:SourceFocusedApprovedAmbientErrors) {
        $parts = [string]$signature -split "`t", 3
        [void]$ambientLines.Add(
            '17:00:11.000 ' + $parts[0] + ' (' + $parts[1] + '): ' +
            $parts[2])
    }
    $ambientConsole = Get-SourceFocusedConsoleContract `
        -ConsoleText (($qrfLines.ToArray() + $ambientLines.ToArray()) -join "`n") `
        -Suite $qrfSuite `
        -ExpectedCases $qrfCases `
        -BuildSummary $buildSummary
    if (-not $ambientConsole.valid -or
        -not $ambientConsole.ambientDiagnosticsExact -or
        $ambientConsole.approvedAmbientDiagnosticCount -ne
            $script:SourceFocusedApprovedAmbientErrors.Count -or
        $ambientConsole.unapprovedHardDiagnosticCount -ne 0) {
        throw 'Source-focused complete ambient diagnostic self-test failed.'
    }
    $checks++
    $partialAmbientConsole = Get-SourceFocusedConsoleContract `
        -ConsoleText (($qrfLines.ToArray() + $ambientLines[0]) -join "`n") `
        -Suite $qrfSuite `
        -ExpectedCases $qrfCases `
        -BuildSummary $buildSummary
    if ($partialAmbientConsole.valid -or
        $partialAmbientConsole.ambientDiagnosticsExact) {
        throw 'Source-focused partial ambient diagnostic rejection self-test failed.'
    }
    $checks++

    $profileCases = @($script:SourceFocusedSuiteCases[$profileSuite])
    $intentionalLine =
        "17:01:00.010 SCRIPT    (E): string failureDetail = 'Partisan persistence | " +
        'native save callback failure | sequence/type/flags 1/0/0 | ' +
        'manager/enabled/allowed/busy/active/playthrough 1/1/1/0/0/0 | ' +
        'types/persistence/state/loaded/tracked/config/staged 5/1/2/0/0/0/1 | ' +
        "replication mode 0 | snapshot fingerprint '"
    $profileLines = New-Object Collections.Generic.List[string]
    [void]$profileLines.Add('17:01:00.000 SCRIPT       : CLI autotest suite: ' + $profileSuite)
    [void]$profileLines.Add('17:01:00.001 SCRIPT       : TestSuite #' + $profileSuite + ' started')
    foreach ($caseName in $profileCases) {
        [void]$profileLines.Add($intentionalLine)
        [void]$profileLines.Add('17:01:00.011 SCRIPT       : exact | failed native callback non-mutating 1 | exact')
        if ($caseName -ceq
            'HST_TEST_CampaignProfileJournalAuthority_FailedNativeCallbackNonMutating') {
            [void]$profileLines.Add('17:01:00.012 SCRIPT       : setup/seam/request/bytes/journal 1/1/1/1/1 | exact')
        }
        [void]$profileLines.Add('17:01:00.013 SCRIPT       : ' + $buildSummary)
        [void]$profileLines.Add('17:01:00.014 SCRIPT       : ' +
            ([string][char]0x2705) + ' ' + $caseName + ': SUCCESS')
    }
    [void]$profileLines.Add('17:01:01.000 SCRIPT       : SCR_TestRunner has finished running')
    [void]$profileLines.Add('17:01:01.001 SCRIPT       : Autotest JUnit XML saved to: $logs:/junit.xml')
    [void]$profileLines.Add('17:01:01.002 SCRIPT       : Autotest failed list saved to: $logs:/autotest_failed.log')
    [void]$profileLines.Add($stockLine)
    [void]$profileLines.Add($stockLine)
    $profileConsoleText = $profileLines.ToArray() -join "`n"
    $profileConsole = Get-SourceFocusedConsoleContract `
        -ConsoleText $profileConsoleText `
        -Suite $profileSuite `
        -ExpectedCases $profileCases `
        -BuildSummary $buildSummary
    if (-not $profileConsole.valid -or
        -not $profileConsole.profileProofExact -or
        $profileConsole.approvedIntentionalDiagnosticCount -ne 41) {
        throw 'Source-focused profile diagnostic self-test failed.'
    }
    $checks++
    $profileMutation = Get-SourceFocusedConsoleContract `
        -ConsoleText $profileConsoleText.Replace(
            'failed native callback non-mutating 1',
            'failed native callback non-mutating 0') `
        -Suite $profileSuite `
        -ExpectedCases $profileCases `
        -BuildSummary $buildSummary
    if ($profileMutation.valid -or $profileMutation.profileProofExact) {
        throw 'Source-focused profile proof rejection self-test failed.'
    }
    $checks++

    Write-Output "PASS: source-focused contract self-test ($checks checks)"
}

if ($SelfTest) {
    Test-SourceFocusedContracts
    return
}

if ([string]::IsNullOrWhiteSpace($Executable) -or
    [string]::IsNullOrWhiteSpace($RuntimeAddonRoot) -or
    [string]::IsNullOrWhiteSpace($EvidenceOutputRoot)) {
    throw 'Executable, RuntimeAddonRoot, and EvidenceOutputRoot are required.'
}

$runnerPath = Resolve-SourceFocusedPath -Path $PSCommandPath -Kind Leaf
$checkoutRoot = Resolve-SourceFocusedPath `
    -Path (Join-Path $PSScriptRoot '..') -Kind Container
$gitTopLevel = (@(Invoke-SourceFocusedGit `
    -CheckoutRoot $checkoutRoot `
    -Arguments @('rev-parse', '--show-toplevel')) -join '').Trim()
if (-not ([IO.Path]::GetFullPath($gitTopLevel)).Equals(
        $checkoutRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The runner must execute from the canonical addon checkout.'
}
$projectPath = Resolve-SourceFocusedPath `
    -Path (Join-Path $checkoutRoot 'addon.gproj') -Kind Leaf
$executablePath = Resolve-SourceFocusedPath -Path $Executable -Kind Leaf
if ((Split-Path -Leaf $executablePath) -cne 'ArmaReforgerSteamDiag.exe') {
    throw 'Source-focused execution requires ArmaReforgerSteamDiag.exe.'
}
$runtimeAddonRootPath = Resolve-SourceFocusedPath `
    -Path $RuntimeAddonRoot -Kind Container
$evidenceOutputPath = [IO.Path]::GetFullPath($EvidenceOutputRoot)
foreach ($protectedRoot in @(
        $checkoutRoot,
        $runtimeAddonRootPath,
        (Split-Path -Parent $executablePath))) {
    if (Test-SourceFocusedPathOverlap `
            -First $evidenceOutputPath -Second $protectedRoot) {
        throw 'The external evidence root overlaps a protected source/runtime root.'
    }
}
Assert-SourceFocusedNoReparseAncestry -Path $evidenceOutputPath
New-Item -ItemType Directory -Path $evidenceOutputPath -Force | Out-Null
Assert-SourceFocusedNoReparseAncestry -Path $evidenceOutputPath

$gitBinding = Get-SourceFocusedGitBinding `
    -CheckoutRoot $checkoutRoot -RunnerPath $runnerPath
if (-not $gitBinding.clean) {
    throw 'Source-focused execution requires a clean, committed Git checkpoint.'
}
$sourceCheckpoint = Get-SourceFocusedCheckpointBinding `
    -RequestedHead $SourceGitHead `
    -HarnessBinding $gitBinding `
    -CheckoutRoot $checkoutRoot
Assert-SourceFocusedNoPackageFiles -CheckoutRoot $checkoutRoot
$projectBinding = Get-SourceFocusedProjectBinding -ProjectPath $projectPath
$buildInfoPath = Resolve-SourceFocusedPath `
    -Path (Join-Path $checkoutRoot 'Scripts/Game/HST/HST_BuildInfo.c') `
    -Kind Leaf
$buildIdentity = Get-SourceFocusedBuildIdentity -Path $buildInfoPath
$schemaBinding = Get-SourceFocusedSchemaBinding -CheckoutRoot $checkoutRoot
$suiteDefinitions = @(Get-SourceFocusedSuiteDefinitions `
    -CheckoutRoot $checkoutRoot)
$toolchainBinding = Get-SourceFocusedRuntimeToolchainBinding `
    -ExecutablePath $executablePath `
    -RuntimeAddonRootPath $runtimeAddonRootPath

$preRunProcesses = @(Get-SourceFocusedEngineProcesses)
if ($preRunProcesses.Count -ne 0) {
    throw 'Refusing source-focused execution while an engine process is active.'
}

$runStartedUtc = [DateTime]::UtcNow
$runNonce = [Guid]::NewGuid().ToString('N')
$runId = 'source-focused-' +
    $sourceCheckpoint.sourceGitHead.Substring(0, 12) + '-' +
    $runStartedUtc.ToString(
        'yyyyMMddTHHmmssZ', [Globalization.CultureInfo]::InvariantCulture) +
    '-' + $runNonce.Substring(0, 8)
$runRoot = [IO.Path]::GetFullPath((Join-Path $evidenceOutputPath $runId))
if (-not (Test-SourceFocusedContainedPath `
        -Root $evidenceOutputPath -Candidate $runRoot) -or
    (Test-Path -LiteralPath $runRoot)) {
    throw 'The retained source-focused run root is not fresh and contained.'
}
New-Item -ItemType Directory -Path $runRoot | Out-Null
Assert-SourceFocusedTreeHasNoReparsePoint -Root $runRoot

$suiteResults = New-Object Collections.Generic.List[object]
$globalError = ''
$mutex = $null
$mutexAcquired = $false
try {
    $mutex = New-Object Threading.Mutex(
        $false, 'Local\PartisanSourceFocusedAutotestGuard')
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw 'Another source-focused autotest run is already active.'
    }
    $ordinal = 0
    foreach ($suite in $script:SourceFocusedSuiteCases.Keys) {
        $ordinal++
        Write-Host "Running source-focused suite $ordinal/5: $suite"
        $suiteResult = Invoke-SourceFocusedSuite `
            -Ordinal $ordinal `
            -Suite $suite `
            -ExpectedCases @($script:SourceFocusedSuiteCases[$suite]) `
            -RunRoot $runRoot `
            -RunNonce $runNonce `
            -ExecutablePath $executablePath `
            -RuntimeAddonRootPath $runtimeAddonRootPath `
            -ProjectPath $projectPath `
            -ProjectBinding $projectBinding `
            -BuildIdentity $buildIdentity `
            -GitBinding $gitBinding `
            -CheckoutRoot $checkoutRoot `
            -RunnerPath $runnerPath `
            -ToolchainBinding $toolchainBinding `
            -TimeoutSeconds $TimeoutSecondsPerSuite `
            -PollIntervalMilliseconds $PollMilliseconds
        [void]$suiteResults.Add($suiteResult)
        if (-not $suiteResult.success) {
            throw "Source-focused suite failed: $suite"
        }
    }
}
catch {
    $globalError = ConvertTo-SourceFocusedSafeText `
        -Text $_.Exception.Message `
        -RedactedPaths @(
            $checkoutRoot,
            $executablePath,
            $runtimeAddonRootPath,
            $evidenceOutputPath,
            $runRoot)
}
finally {
    if ($mutex) {
        if ($mutexAcquired) {
            try { $mutex.ReleaseMutex() }
            catch {
                if ([string]::IsNullOrEmpty($globalError)) {
                    $globalError = 'The source-focused mutex could not be released.'
                }
            }
        }
        $mutex.Dispose()
    }
}

$runCompletedUtc = [DateTime]::UtcNow
$postRunProcesses = @(Get-SourceFocusedEngineProcesses)
$postBindingsStable = $false
try {
    [void](Assert-SourceFocusedGitBinding `
        -Expected $gitBinding `
        -CheckoutRoot $checkoutRoot `
        -RunnerPath $runnerPath)
    [void](Assert-SourceFocusedRuntimeToolchainBinding `
        -Expected $toolchainBinding)
    $postBindingsStable = $true
}
catch {
    if ([string]::IsNullOrEmpty($globalError)) {
        $globalError = ConvertTo-SourceFocusedSafeText `
            -Text $_.Exception.Message `
            -RedactedPaths @(
                $checkoutRoot,
                $executablePath,
                $runtimeAddonRootPath,
                $evidenceOutputPath,
                $runRoot)
    }
}

$integrity = Get-SourceFocusedArtifactIntegrity -EvidenceRunRoot $runRoot
$passedSuites = @($suiteResults | Where-Object { $_.success }).Count
$totalTests = 0
$totalFailures = 0
$totalErrors = 0
$totalSkipped = 0
foreach ($suiteResult in $suiteResults) {
    if ($suiteResult.junit) {
        $totalTests += [int]$suiteResult.junit.tests
        $totalFailures += [int]$suiteResult.junit.failures
        $totalErrors += [int]$suiteResult.junit.errors
        $totalSkipped += [int]$suiteResult.junit.skipped
    }
}
$runPassed = [string]::IsNullOrEmpty($globalError) -and
    $suiteResults.Count -eq 5 -and $passedSuites -eq 5 -and
    $totalTests -eq 91 -and $totalFailures -eq 0 -and
    $totalErrors -eq 0 -and $totalSkipped -eq 0 -and
    $postRunProcesses.Count -eq 0 -and $postBindingsStable -and
    $integrity.artifactCount -gt 0

$summary = [pscustomobject][ordered]@{
    schemaVersion = 1
    evidenceKind = 'source-gate1-focused-five-suite'
    source = [pscustomobject][ordered]@{
        sourceGitHead = $sourceCheckpoint.sourceGitHead
        publishInputPolicy = $sourceCheckpoint.publishInputTree.algorithm
        publishInputTreeSha256 = $sourceCheckpoint.publishInputTree.sha256
        publishInputRowCount = $sourceCheckpoint.publishInputTree.rowCount
        addonGuid = $projectBinding.guid
        campaignSchema = $schemaBinding.campaign.value
        runtimeSettingsSchema = $schemaBinding.'runtime-settings'.value
        embeddedImplementationSha = $buildIdentity.sha
        embeddedBuildUtc = $buildIdentity.utc
        embeddedBuildLabel = $buildIdentity.label
    }
    harness = [pscustomobject][ordered]@{
        gitHead = $gitBinding.head
        dirty = $false
        runnerPath = $gitBinding.runnerPath
        runnerGitBlobOid = $gitBinding.runnerGitBlobOid
        runnerHashPolicy = $gitBinding.runnerHashPolicy
        runnerSha256 = $gitBinding.runnerSha256
    }
    toolchain = [pscustomobject][ordered]@{
        diagnosticExecutable = ConvertTo-SourceFocusedPortableFileIdentity `
            -Identity $toolchainBinding.executable `
            -Name (Split-Path -Leaf $executablePath)
        runtimeAddonRoot = [pscustomobject][ordered]@{
            role = 'installed-base-addon-root'
            pathBindingPolicy = 'normalized-lowercase-utf8-lf-sha256-v1'
            pathSha256 = $toolchainBinding.runtimeAddonRootPathSha256
            coreProject = ConvertTo-SourceFocusedPortableFileIdentity `
                -Identity $toolchainBinding.coreProject `
                -Name 'core/core.gproj'
            dataProject = ConvertTo-SourceFocusedPortableFileIdentity `
                -Identity $toolchainBinding.dataProject `
                -Name 'data/ArmaReforger.gproj'
        }
        sourceProject = [pscustomobject][ordered]@{
            role = 'source-checkout/addon.gproj'
            id = $projectBinding.id
            guid = $projectBinding.guid
            sha256 = $projectBinding.file.sha256
        }
        sourceResourceDatabase = $gitBinding.resourceDatabase
    }
    capture = [pscustomobject][ordered]@{
        runId = $runId
        startedUtc = $runStartedUtc.ToString(
            'o', [Globalization.CultureInfo]::InvariantCulture)
        completedUtc = $runCompletedUtc.ToString(
            'o', [Globalization.CultureInfo]::InvariantCulture)
    }
    result = [pscustomobject][ordered]@{
        status = if ($runPassed) { 'passed-noncertifying' } else { 'failed' }
        success = $runPassed
        noncertifying = $true
        expectedSuiteCount = 5
        completedSuiteCount = $suiteResults.Count
        passedSuiteCount = $passedSuites
        expectedTestCount = 91
        tests = $totalTests
        failures = $totalFailures
        errors = $totalErrors
        skipped = $totalSkipped
        sourceBindingsStable = $postBindingsStable
        sourceIsHarnessAncestor = $sourceCheckpoint.sourceIsHarnessAncestor
        publishInputsMatchHarness = $sourceCheckpoint.publishInputsMatchHarness
        finalEngineProcessCount = $postRunProcesses.Count
        suiteDefinitions = $suiteDefinitions
        suites = $suiteResults.ToArray()
        error = $globalError
    }
    integrity = $integrity
}

$summaryText = (($summary | ConvertTo-Json -Depth 40).Replace("`r`n", "`n") + "`n")
if ($summaryText -cmatch '(?i)[A-Z]:(?:\\\\|/)' -or
    $summaryText -cmatch '\\\\\\\\[^<]') {
    throw 'The source-focused summary contains an absolute local path.'
}
$summaryPath = Join-Path $runRoot 'run.json'
[IO.File]::WriteAllText(
    $summaryPath,
    $summaryText,
    (New-Object Text.UTF8Encoding($false, $true)))
$summarySha256 = (Get-FileHash `
    -LiteralPath $summaryPath -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText(
    (Join-Path $runRoot 'run.json.sha256'),
    $summarySha256 + "`n",
    (New-Object Text.UTF8Encoding($false, $true)))

Write-Output ('EVIDENCE ' + ([pscustomobject][ordered]@{
    runId = $runId
    root = $runRoot
    summarySha256 = $summarySha256
    artifactCount = $integrity.artifactCount
} | ConvertTo-Json -Compress))
Write-Output ('RESULT ' + ([pscustomobject][ordered]@{
    status = $summary.result.status
    suites = $suiteResults.Count
    tests = $totalTests
    failures = $totalFailures
    errors = $totalErrors
    skipped = $totalSkipped
} | ConvertTo-Json -Compress))

if (-not $runPassed) {
    throw 'The five-suite source-focused Gate 1 run failed closed.'
}

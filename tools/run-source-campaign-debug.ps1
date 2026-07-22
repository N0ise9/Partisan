[CmdletBinding(DefaultParameterSetName = 'Run')]
param(
    [Parameter(Mandatory = $true, ParameterSetName = 'Run')]
    [string]$Executable,

    [Parameter(Mandatory = $true, ParameterSetName = 'Run')]
    [string]$AddonDirectory,

    [Parameter(Mandatory = $true, ParameterSetName = 'Run')]
    [string]$SettingsSource,

    [Parameter(Mandatory = $true, ParameterSetName = 'Run')]
    [string]$EvidenceOutputRoot,

    [Parameter(ParameterSetName = 'Run')]
    [ValidateSet('full_certification', 'force_authority')]
    [string]$Profile = 'full_certification',

    [Parameter(ParameterSetName = 'Run')]
    [string]$ProjectFile = '',

    [Parameter(ParameterSetName = 'Run')]
    [string]$SourceGitHead = '',

    [Parameter(ParameterSetName = 'Run')]
    [string]$WorldResource = 'Worlds/HST_Dev/HST_Dev.ent',

    [Parameter(ParameterSetName = 'Run')]
    [ValidateRange(60, 3600)]
    [int]$TimeoutSeconds = 1080,

    [Parameter(ParameterSetName = 'Run')]
    [ValidateRange(10, 600)]
    [int]$ArmedTimeoutSeconds = 90,

    [Parameter(ParameterSetName = 'Run')]
    [ValidateRange(10, 900)]
    [int]$StartedTimeoutSeconds = 180,

    [Parameter(ParameterSetName = 'Run')]
    [ValidateRange(1, 30)]
    [int]$PollSeconds = 5,

    [Parameter(Mandatory = $true, ParameterSetName = 'SelfTest')]
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# SCRIPT and ENGINE hard diagnostics remain owned by the imported guarded
# classifier. These exact, lifecycle-bound families cover the other channels
# without turning a channel name or a broad regex into an allowlist.
$script:SourceCampaignStartupGuiError =
    "GUI`tE`tUnknown class 'SCR_WidgetExportRuleRoot' at offset 282(0x11a)"
$script:SourceCampaignMapMaskError =
    "GUI`tE`tImageWidget 'm_Outline': Can't load mask from ImageSet 'UI/Textures/Icons/icons_mouse/icons_mouse.imageset'"
$script:SourceCampaignStartupPathfindingErrors = @(
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <405, 66>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <343, 53>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <343, 51>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <342, 53>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <341, 53>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <340, 53>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <340, 52>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <339, 52>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <338, 53>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <337, 50>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <337, 49>"
)
$script:SourceCampaignRuntimePathfindingFamilyA = @(
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <308, 251>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <312, 250>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <311, 250>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <310, 253>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <310, 249>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <309, 253>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <308, 251>"
)
$script:SourceCampaignRuntimePathfindingFamilyB = @(
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <325, 219>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <335, 216>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <335, 215>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <332, 218>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <325, 219>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <336, 216>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <336, 215>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <335, 217>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <335, 216>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <335, 215>",
    "PATHFINDING`tE`tIncorrect tile position calculated for idx: <325, 219>"
)
$script:SourceCampaignFullIntentionalResourceErrors = @(
    "RESOURCES`tE`tWrong GUID/name for resource @`"{0000000000000000}Prefabs/Groups/HST_Missing_Force_Debug.et`" in property `"resourceName`"",
    "RESOURCES`tE`tFailed to open",
    "RESOURCES`tE`tWrong GUID/name for resource @`"{0000000000000000}Prefabs/Groups/HST_Missing_Force_Debug.et`" in property `"resourceName`"",
    "RESOURCES`tE`tFailed to open",
    "RESOURCES`tE`tWrong GUID/name for resource @`"{0000000000000000}Prefabs/Invalid/HST_MissingConvoyCargo.et`" in property `"cargoResourceName`"",
    "RESOURCES`tE`tFailed to open",
    "RESOURCES`tE`tWrong GUID/name for resource @`"{0000000000000000}Prefabs/Characters/HST/HST_CampaignDebug_MissingSpawnMember.et`" in property `"resourceName`"",
    "RESOURCES`tE`tFailed to open"
)
$script:SourceCampaignFullIntentionalResourceBindings = @(
    [pscustomobject][ordered]@{
        resourcePath = 'Prefabs/Groups/HST_Missing_Force_Debug.et'
        previousMarker = 'Partisan campaign debug | PASS | post_case_cleanup.authorization_commander_disconnect_handoff_runtime | assertions passed'
        nextMarker = 'Partisan campaign debug | PASS | force_composition.contract.runtime | assertions passed'
    },
    [pscustomobject][ordered]@{
        resourcePath = 'Prefabs/Groups/HST_Missing_Force_Debug.et'
        previousMarker = 'Partisan campaign debug | PASS | post_case_cleanup.force_composition_contract_runtime | assertions passed'
        nextMarker = 'Partisan campaign debug | PASS | observation.force_composition | assertions passed'
    },
    [pscustomobject][ordered]@{
        resourcePath = 'Prefabs/Invalid/HST_MissingConvoyCargo.et'
        previousMarker = 'Partisan exact mission convoy | mission_convoy_proof_cargo_duplicate failed closed: exact mission convoy admission contains more than one optional cargo row'
        nextMarker = 'Partisan exact mission convoy | mission_convoy_proof_cargo_invalid_prefab failed closed: exact mission convoy cargo prefab is missing, invalid, or not an entity prefab'
    },
    [pscustomobject][ordered]@{
        resourcePath = 'Prefabs/Characters/HST/HST_CampaignDebug_MissingSpawnMember.et'
        previousMarker = 'Partisan campaign debug | PASS | post_case_cleanup.action_mechanic_exact_spawn_adapter_failure_member_transition | assertions passed'
        nextMarker = 'Partisan campaign debug | PASS | action.mechanic_exact_spawn_adapter_same-wave_failure_capture | assertions passed'
    }
)
$script:SourceCampaignFocusedTeardownResourceErrors = @(
    "RESOURCES`tE`t==== Resource leaks ====",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-48_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/VehicleInfo/AnalogGaugeImageset_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/WeaponInfo/WeaponInfo-750_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/WeaponInfo/WeaponInfo_Glow-750_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/WeaponInfo/WeaponInfo_Ammo-800_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/WeaponInfo/WeaponInfo_Ammo_Glow-800_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Editor/editor_icons_map.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Conflict/conflict-icons-bw_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Map/topographicIcons/icons_topographic_map_atlasSRGB.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Notifications/NotificationIcons-400_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Tasks/Task_Icons-100_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Chat/chat_32_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Chat/chat_badge_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_mouse/icons_mouse-glow-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Chat/chat_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_mouse/icons_mouse-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_gamepad/icons_gamepad-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_keyboard/icons_keyboard-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_keyboard/icons_keyboard-glow-200_atlas.edds   1",
    "RESOURCES`tE`tUI/Imagesets/Conflict/conflict-no-signal_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/MilitaryIcons/MilitaryIcons-400_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-64_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-64-glow_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-32-glow_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Icons/icons_wrapperUI-32_atlas.edds   1",
    "RESOURCES`tE`tUI/Textures/Editor/Logos/Editor-Logo-400_atlas.edds   1",
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
$script:SourceCampaignFullTeardownResourceErrors = @(
    $script:SourceCampaignFocusedTeardownResourceErrors[0..9] +
    @(
        "RESOURCES`tE`tUI/Imagesets/MilitarySymbol/ICO_Land_atlas.edds   1",
        "RESOURCES`tE`tUI/Imagesets/MilitarySymbol/ID_D_atlas.edds   1",
        "RESOURCES`tE`tUI/Textures/Icons/icons_mapMarkersUI-400_atlas.edds   1",
        "RESOURCES`tE`tUI/Textures/Icons/icons_mapMarkersUI-glow-400_atlas.edds   1"
    ) +
    $script:SourceCampaignFocusedTeardownResourceErrors[10..52]
)

if (-not ('PartisanSourceCampaignDebugJob' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public sealed class PartisanSourceCampaignDebugJob : IDisposable
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

    public PartisanSourceCampaignDebugJob()
    {
        _handle = CreateJobObject(IntPtr.Zero, null);
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException("Unable to create source Campaign Debug process job.");

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
            new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
        info.BasicLimitInformation.LimitFlags = 0x00002000;
        int size = Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
        IntPtr pointer = Marshal.AllocHGlobal(size);
        try
        {
            Marshal.StructureToPtr(info, pointer, false);
            if (!SetInformationJobObject(_handle, 9, pointer, (UInt32)size))
                throw new InvalidOperationException("Unable to configure source Campaign Debug process job.");
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
            throw new InvalidOperationException("Unable to assign the runtime to its source Campaign Debug process job.");
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
                    throw new InvalidOperationException("Unable to query source Campaign Debug process job.");
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

        throw new InvalidOperationException("Source Campaign Debug process-job membership exceeded the safety limit.");
    }

    public void Dispose()
    {
        if (_handle == IntPtr.Zero)
            return;
        CloseHandle(_handle);
        _handle = IntPtr.Zero;
    }
}

public sealed class PartisanSourceCampaignDebugSuspendedProcess : IDisposable
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

    public PartisanSourceCampaignDebugSuspendedProcess(
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
                "Unable to create the suspended source Campaign Debug runtime.");
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
            throw new InvalidOperationException("The suspended source runtime thread is unavailable.");
        if (ResumeThread(_threadHandle) == UInt32.MaxValue)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to resume the source Campaign Debug runtime.");
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

public static class PartisanSourceCampaignDebugNativeCommandLine
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
            throw new InvalidOperationException("Unable to parse the native command line.");
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
        throw "Required $Kind path does not exist."
    }
    return [IO.Path]::GetFullPath((Get-Item -LiteralPath $Path -Force).FullName)
}

function Test-PathContained {
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

    return (Test-PathContained -Root $First -Candidate $Second -AllowEqual) -or
        (Test-PathContained -Root $Second -Candidate $First -AllowEqual)
}

function Assert-NoReparsePathAncestry {
    param([Parameter(Mandatory = $true)][string]$Path)

    $cursor = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    while (-not (Test-Path -LiteralPath $cursor)) {
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A safe existing path ancestor could not be resolved.'
        }
        $cursor = $parent
    }
    while (-not [string]::IsNullOrWhiteSpace($cursor)) {
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'Source Campaign Debug paths must not traverse a reparse point.'
        }
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $cursor = $parent
    }
}

function Write-Utf8Text {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text
    )

    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false)))
}

function Write-PortableJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $text = ($Value | ConvertTo-Json -Depth 30).Replace("`r`n", "`n") + "`n"
    Write-Utf8Text -Path $Path -Text $text
}

function Write-PortableSummaryJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $text = ($Value | ConvertTo-Json -Depth 30).Replace("`r`n", "`n") + "`n"
    if ($text -match '(?i)[a-z]:(?:\\\\|/)' -or
        $text -match '(?m)\\\\\\\\[^\\]') {
        throw 'A portable evidence summary attempted to retain an absolute local path.'
    }
    Write-Utf8Text -Path $Path -Text $text
}

function Get-TextSha256 {
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text)

    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($Text)
        return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Get-FileIdentity {
    param([Parameter(Mandatory = $true)][string]$Path)

    $resolved = Resolve-ExistingPath -Path $Path -Kind Leaf
    $item = Get-Item -LiteralPath $resolved -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'An identity-bound file must not be a reparse point.'
    }
    return [pscustomobject][ordered]@{
        path = $resolved
        bytes = [long]$item.Length
        lastWriteUtc = $item.LastWriteTimeUtc.ToString(
            'o',
            [Globalization.CultureInfo]::InvariantCulture)
        sha256 = (Get-FileHash -LiteralPath $resolved -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Test-FileIdentityEqual {
    param(
        [Parameter(Mandatory = $true)]$First,
        [Parameter(Mandatory = $true)]$Second
    )

    return [long]$First.bytes -eq [long]$Second.bytes -and
        [string]$First.sha256 -ceq [string]$Second.sha256
}

function ConvertTo-PortableFileIdentity {
    param([Parameter(Mandatory = $true)]$Identity)

    return [pscustomobject][ordered]@{
        length = [long]$Identity.bytes
        lastWriteUtc = [string]$Identity.lastWriteUtc
        sha256 = [string]$Identity.sha256
    }
}

function Get-SourceResourceDatabaseIdentity {
    param([Parameter(Mandatory = $true)][string]$CheckoutRoot)

    $path = Join-Path $CheckoutRoot 'resourceDatabase.rdb'
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw 'The Workbench-generated source resource database is missing.'
    }
    Assert-NoReparsePathAncestry -Path $path
    $identity = Get-FileIdentity -Path $path
    if ([long]$identity.bytes -le 0) {
        throw 'The Workbench-generated source resource database is empty.'
    }
    return $identity
}

function Test-NativeJsonBoolean {
    param($Value)

    return $Value -is [bool]
}

function Test-NativeJsonInteger {
    param($Value)

    return $Value -is [byte] -or $Value -is [sbyte] -or
        $Value -is [int16] -or $Value -is [uint16] -or
        $Value -is [int32] -or $Value -is [uint32] -or
        $Value -is [int64] -or $Value -is [uint64]
}

function Get-ArgumentVectorBinding {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$ResolvedAddonDirectory,
        [Parameter(Mandatory = $true)][string]$AddonTempDirectory,
        [Parameter(Mandatory = $true)][string]$ResolvedProjectFile,
        [Parameter(Mandatory = $true)][string]$IsolationRoot,
        [Parameter(Mandatory = $true)][string]$CommandLine
    )

    $portable = @($Arguments | ForEach-Object {
        $value = [string]$_
        if ($value.Equals($ResolvedAddonDirectory, [StringComparison]::OrdinalIgnoreCase)) {
            '<runtime-addons>'
        }
        elseif ($value.Equals($AddonTempDirectory, [StringComparison]::OrdinalIgnoreCase)) {
            '<evidence-run>/isolation/addon-temp'
        }
        elseif ($value.Equals($ResolvedProjectFile, [StringComparison]::OrdinalIgnoreCase)) {
            '<checkout>/addon.gproj'
        }
        elseif ($value.Equals($IsolationRoot, [StringComparison]::OrdinalIgnoreCase)) {
            '<evidence-run>/isolation'
        }
        else {
            $value
        }
    })
    $separator = [string][char]0
    $canonicalVector = ($Arguments -join $separator) + $separator
    return [pscustomobject][ordered]@{
        hashAlgorithm = 'sha256-utf8-nul-argument-vector-v1'
        tokenCount = $Arguments.Count
        sha256 = Get-TextSha256 -Text $canonicalVector
        portableTokens = $portable
        nativeCommandLineSha256 = Get-TextSha256 -Text ($CommandLine + "`n")
    }
}

function Invoke-GitText {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string[]]$GitArguments
    )

    $output = @(& git -C $CheckoutRoot @GitArguments 2>&1)
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        $detail = (@($output | ForEach-Object { [string]$_ }) -join ' ').Trim()
        throw "Git command failed with exit code $exitCode`: $detail"
    }
    return @($output | ForEach-Object { [string]$_ })
}

function Get-GitBlobSha256 {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$Revision,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = 'git'
    $escapedRoot = ([IO.Path]::GetFullPath($CheckoutRoot)).Replace('"', '\"')
    $escapedSpec = ("{0}:{1}" -f $Revision, $RelativePath).Replace('"', '\"')
    $startInfo.Arguments = "-C `"$escapedRoot`" cat-file blob `"$escapedSpec`""
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw 'The immutable Git blob reader could not start.'
        }
        $memory = New-Object IO.MemoryStream
        try {
            $process.StandardOutput.BaseStream.CopyTo($memory)
            $errorText = $process.StandardError.ReadToEnd()
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) {
                throw "Git blob $RelativePath could not be read at $Revision`: $errorText"
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

function Get-RepositoryRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $root = [IO.Path]::GetFullPath($CheckoutRoot).TrimEnd('\', '/')
    $full = [IO.Path]::GetFullPath($Path)
    $prefix = $root + [IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A source-bound path escaped the checkout.'
    }
    return $full.Substring($prefix.Length).Replace('\', '/')
}

function Get-PublishInputFingerprint {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$Revision
    )

    $pathspecs = @(
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
    $arguments = @('ls-tree', '-r', '--full-tree', $Revision, '--') + $pathspecs
    [string[]]$unsortedRows = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments $arguments)
    $rowByPath = New-Object `
        'Collections.Generic.Dictionary[string,object]' `
        ([StringComparer]::Ordinal)
    $seenPathsIgnoreCase = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($row in $unsortedRows) {
        $match = [Regex]::Match(
            $row,
            '^(?<mode>[0-9]{6})\s+(?<type>blob)\s+(?<oid>[0-9a-f]{40})\t(?<path>.+)$')
        if (-not $match.Success -or
            $match.Groups['mode'].Value -cne '100644' -or
            $rowByPath.ContainsKey($match.Groups['path'].Value) -or
            -not $seenPathsIgnoreCase.Add($match.Groups['path'].Value)) {
            throw 'The canonical publish-input tree contains a malformed or duplicate row.'
        }
        $path = $match.Groups['path'].Value.Replace('\', '/')
        $rowByPath.Add($path, [pscustomobject][ordered]@{
            mode = $match.Groups['mode'].Value
            type = $match.Groups['type'].Value
            oid = $match.Groups['oid'].Value
            path = $path
            canonical = $match.Groups['mode'].Value + ' ' +
                $match.Groups['type'].Value + ' ' +
                $match.Groups['oid'].Value + "`t" + $path
        })
    }
    [string[]]$paths = @($rowByPath.Keys)
    [Array]::Sort($paths, [StringComparer]::Ordinal)
    [object[]]$rows = @($paths | ForEach-Object { $rowByPath[$_] })
    if ($rows.Count -eq 0 -or
        @($rows | Where-Object { $_.path -ceq 'addon.gproj' }).Count -ne 1) {
        throw 'The canonical publish-input tree is missing its project row.'
    }
    $canonicalText = (@($rows | ForEach-Object { $_.canonical }) -join "`n") + "`n"
    return [pscustomobject][ordered]@{
        pathspecs = $pathspecs
        rowCount = $rows.Count
        sha256 = Get-TextSha256 -Text $canonicalText
        text = $canonicalText
        rows = $rows
    }
}

function Get-WorkingGitObjectId {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$WorkingPath
    )

    $rows = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @("hash-object", "--path=$RelativePath", '--', $WorkingPath))
    if ($rows.Count -ne 1 -or $rows[0].Trim() -cnotmatch '^[0-9a-f]{40}$') {
        throw 'A working publish input could not be normalized to one Git blob identity.'
    }
    return $rows[0].Trim()
}

function Get-WorkingPublishInputBinding {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)]$ExpectedPublishInput
    )

    $pathspecs = [string[]]@($ExpectedPublishInput.pathspecs)
    $untracked = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments (@('ls-files', '--others', '--exclude-standard', '--') + $pathspecs))
    $ignored = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments (@(
                'ls-files', '--others', '--ignored', '--exclude-standard', '--') +
            $pathspecs))
    if ($untracked.Count -ne 0 -or $ignored.Count -ne 0) {
        throw 'The executed publish-input scope contains an untracked or ignored extra.'
    }

    $indexRows = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments (@('ls-files', '-v', '--') + $pathspecs))
    if ($indexRows.Count -ne [int]$ExpectedPublishInput.rowCount -or
        @($indexRows | Where-Object { $_ -cnotmatch '^H\s.+' }).Count -ne 0) {
        throw 'The executed publish-input scope has a skip-worktree, assume-unchanged, or index-shape mismatch.'
    }

    $indexPaths = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $indexPathsIgnoreCase = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($indexRow in $indexRows) {
        $path = $indexRow.Substring(2).Replace('\', '/')
        if (-not $indexPaths.Add($path) -or -not $indexPathsIgnoreCase.Add($path)) {
            throw 'The executed publish-input index contains a duplicate path.'
        }
    }

    $canonicalRows = New-Object Collections.Generic.List[string]
    foreach ($expectedRow in @($ExpectedPublishInput.rows)) {
        $relative = [string]$expectedRow.path
        if (-not $indexPaths.Contains($relative)) {
            throw 'The executed publish-input index differs from the selected source checkpoint.'
        }
        $workingPath = Join-Path $CheckoutRoot $relative.Replace('/', '\')
        if (-not (Test-Path -LiteralPath $workingPath -PathType Leaf)) {
            throw 'An executed publish input is missing from the working checkout.'
        }
        Assert-NoReparsePathAncestry -Path $workingPath
        $workingObject = Get-WorkingGitObjectId `
            -CheckoutRoot $CheckoutRoot `
            -RelativePath $relative `
            -WorkingPath $workingPath
        if ($workingObject -cne [string]$expectedRow.oid) {
            throw 'An executed publish input differs from the selected source Git blob.'
        }
        [void]$canonicalRows.Add(
            [string]$expectedRow.mode + ' ' + [string]$expectedRow.type + ' ' +
            $workingObject + "`t" + $relative)
    }
    $canonicalText = ($canonicalRows.ToArray() -join "`n") + "`n"
    $sha256 = Get-TextSha256 -Text $canonicalText
    if ($canonicalRows.Count -ne [int]$ExpectedPublishInput.rowCount -or
        $sha256 -cne [string]$ExpectedPublishInput.sha256 -or
        $canonicalText -cne [string]$ExpectedPublishInput.text) {
        throw 'The executed publish-input worktree does not reproduce the selected source identity.'
    }
    return [pscustomobject][ordered]@{
        rowCount = $canonicalRows.Count
        sha256 = $sha256
        text = $canonicalText
        untrackedExtraCount = 0
        ignoredExtraCount = 0
        nonstandardIndexFlagCount = 0
    }
}

function Get-BuildIdentity {
    param([Parameter(Mandatory = $true)][string]$BuildInfoPath)

    $text = [IO.File]::ReadAllText($BuildInfoPath)
    $shaMatch = [Regex]::Match(
        $text,
        'static\s+const\s+string\s+BUILD_SHA\s*=\s*"(?<value>[0-9a-f]{40})"\s*;',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    $utcMatch = [Regex]::Match(
        $text,
        'static\s+const\s+string\s+BUILD_UTC\s*=\s*"(?<value>[^"]+)"\s*;',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    $labelMatch = [Regex]::Match(
        $text,
        'static\s+const\s+string\s+BUILD_LABEL\s*=\s*"(?<value>[^"]+)"\s*;',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    if (-not $shaMatch.Success -or -not $utcMatch.Success -or -not $labelMatch.Success) {
        throw 'The source build identity could not be parsed exactly.'
    }
    $parsedUtc = [DateTime]::MinValue
    if (-not [DateTime]::TryParseExact(
            $utcMatch.Groups['value'].Value,
            'yyyy-MM-ddTHH:mm:ssZ',
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::AssumeUniversal,
            [ref]$parsedUtc)) {
        throw 'The source build UTC is not canonical.'
    }
    return [pscustomobject][ordered]@{
        sha = $shaMatch.Groups['value'].Value
        utc = $utcMatch.Groups['value'].Value
        label = $labelMatch.Groups['value'].Value
    }
}

function Get-RuntimeSettingsSchema {
    param([Parameter(Mandatory = $true)][string]$RuntimeSettingsPath)

    $text = [IO.File]::ReadAllText($RuntimeSettingsPath)
    $match = [Regex]::Match(
        $text,
        'class\s+HST_RuntimeSettings\s*\{[\s\S]*?static\s+const\s+int\s+SCHEMA_VERSION\s*=\s*(?<value>\d+)\s*;',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    if (-not $match.Success) {
        throw 'The runtime-settings schema could not be parsed exactly.'
    }
    return [int]$match.Groups['value'].Value
}

function Get-CampaignSchema {
    param([Parameter(Mandatory = $true)][string]$CampaignStatePath)

    $text = [IO.File]::ReadAllText($CampaignStatePath)
    $match = [Regex]::Match(
        $text,
        'class\s+HST_CampaignState\s*\{[\s\S]*?static\s+const\s+int\s+SCHEMA_VERSION\s*=\s*(?<value>\d+)\s*;',
        [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    if (-not $match.Success) {
        throw 'The campaign schema could not be parsed exactly.'
    }
    return [int]$match.Groups['value'].Value
}

function Get-SourceBinding {
    param(
        [Parameter(Mandatory = $true)][string]$CheckoutRoot,
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$ClassifierPath,
        [Parameter(Mandatory = $true)][string]$ProjectPath,
        [string]$RequestedSourceGitHead = ''
    )

    $status = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('status', '--porcelain=v1', '--untracked-files=all'))
    if ($status.Count -ne 0) {
        throw 'A source Campaign Debug run requires a clean, committed checkout.'
    }

    $harnessHead = @(Invoke-GitText -CheckoutRoot $CheckoutRoot -GitArguments @('rev-parse', 'HEAD'))[0].Trim()
    if ([string]::IsNullOrWhiteSpace($RequestedSourceGitHead)) {
        $sourceHead = $harnessHead
    }
    else {
        if ($RequestedSourceGitHead -cnotmatch '^[0-9a-f]{40}$') {
            throw 'SourceGitHead must be a lowercase full Git commit SHA.'
        }
        $sourceHead = @(Invoke-GitText `
            -CheckoutRoot $CheckoutRoot `
            -GitArguments @('rev-parse', "$RequestedSourceGitHead`^{commit}"))[0].Trim()
        if ($sourceHead -cne $RequestedSourceGitHead) {
            throw 'SourceGitHead did not resolve to the exact requested commit.'
        }
    }
    & git -C $CheckoutRoot merge-base --is-ancestor $sourceHead $harnessHead 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw 'The selected source checkpoint is not an ancestor of the clean harness checkpoint.'
    }
    $harnessTree = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('rev-parse', 'HEAD^{tree}'))[0].Trim()
    $sourceTree = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('rev-parse', "$sourceHead`^{tree}"))[0].Trim()
    $branch = @(Invoke-GitText -CheckoutRoot $CheckoutRoot -GitArguments @('rev-parse', '--abbrev-ref', 'HEAD'))[0].Trim()
    $harnessCommitUtc = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('show', '-s', '--format=%cI', $harnessHead))[0].Trim()
    $sourceCommitUtc = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('show', '-s', '--format=%cI', $sourceHead))[0].Trim()
    $runnerRelative = Get-RepositoryRelativePath -CheckoutRoot $CheckoutRoot -Path $RunnerPath
    $classifierRelative = Get-RepositoryRelativePath -CheckoutRoot $CheckoutRoot -Path $ClassifierPath
    $projectRelative = Get-RepositoryRelativePath -CheckoutRoot $CheckoutRoot -Path $ProjectPath
    if ($projectRelative -cne 'addon.gproj') {
        throw 'Source Campaign Debug must execute the checkout-root addon.gproj.'
    }
    foreach ($relative in @($runnerRelative, $classifierRelative, $projectRelative)) {
        [void](Invoke-GitText `
            -CheckoutRoot $CheckoutRoot `
            -GitArguments @('ls-files', '--error-unmatch', '--', $relative))
    }
    $harnessIndexFlags = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('ls-files', '-v', '--', $runnerRelative, $classifierRelative))
    if ($harnessIndexFlags.Count -ne 2 -or
        @($harnessIndexFlags | Where-Object { $_ -cnotmatch '^H\s.+' }).Count -ne 0) {
        throw 'The runner or classifier has a skip-worktree, assume-unchanged, or index-shape mismatch.'
    }
    $archiveExtension = 'p' + 'ak'
    $archivePathspecs = @(
        ":(icase,glob)**/*.$archiveExtension",
        ":(icase,glob)*.$archiveExtension")
    $archiveRows = @(
        @(Invoke-GitText `
            -CheckoutRoot $CheckoutRoot `
            -GitArguments (@('ls-files', '--cached', '--') + $archivePathspecs))
        @(Invoke-GitText `
            -CheckoutRoot $CheckoutRoot `
            -GitArguments (@(
                    'ls-files',
                    '--others',
                    '--exclude-standard',
                    '--') + $archivePathspecs))
        @(Invoke-GitText `
            -CheckoutRoot $CheckoutRoot `
            -GitArguments (@(
                    'ls-files',
                    '--others',
                    '--ignored',
                    '--exclude-standard',
                    '--') + $archivePathspecs))
    ) | Sort-Object -Unique
    if (@($archiveRows).Count -ne 0) {
        throw 'The source checkout contains a generated archive.'
    }

    $buildInfoPath = Join-Path $CheckoutRoot 'Scripts\Game\HST\HST_BuildInfo.c'
    $runtimeSettingsPath = Join-Path $CheckoutRoot 'Scripts\Game\HST\Config\HST_RuntimeSettings.c'
    $campaignStatePath = Join-Path $CheckoutRoot 'Scripts\Game\HST\State\HST_CampaignState.c'
    $build = Get-BuildIdentity -BuildInfoPath $buildInfoPath
    & git -C $CheckoutRoot merge-base --is-ancestor $build.sha $sourceHead 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw 'The embedded source build identity is not an ancestor of the selected source checkpoint.'
    }
    $publish = Get-PublishInputFingerprint `
        -CheckoutRoot $CheckoutRoot `
        -Revision $sourceHead
    $harnessPublish = Get-PublishInputFingerprint `
        -CheckoutRoot $CheckoutRoot `
        -Revision $harnessHead
    if ([string]$publish.sha256 -cne [string]$harnessPublish.sha256 -or
        [int]$publish.rowCount -ne [int]$harnessPublish.rowCount -or
        [string]$publish.text -cne [string]$harnessPublish.text) {
        throw 'The clean harness checkpoint changed canonical publish inputs after SourceGitHead.'
    }
    $workingPublish = Get-WorkingPublishInputBinding `
        -CheckoutRoot $CheckoutRoot `
        -ExpectedPublishInput $publish

    $runnerGitObject = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('rev-parse', "$harnessHead`:$runnerRelative"))[0].Trim()
    $classifierGitObject = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('rev-parse', "$harnessHead`:$classifierRelative"))[0].Trim()
    $projectGitObject = @(Invoke-GitText `
        -CheckoutRoot $CheckoutRoot `
        -GitArguments @('rev-parse', "$sourceHead`:$projectRelative"))[0].Trim()
    $runnerWorkingObject = Get-WorkingGitObjectId `
        -CheckoutRoot $CheckoutRoot `
        -RelativePath $runnerRelative `
        -WorkingPath $RunnerPath
    $classifierWorkingObject = Get-WorkingGitObjectId `
        -CheckoutRoot $CheckoutRoot `
        -RelativePath $classifierRelative `
        -WorkingPath $ClassifierPath
    $projectWorkingObject = Get-WorkingGitObjectId `
        -CheckoutRoot $CheckoutRoot `
        -RelativePath $projectRelative `
        -WorkingPath $ProjectPath
    if ($runnerWorkingObject -cne $runnerGitObject -or
        $classifierWorkingObject -cne $classifierGitObject -or
        $projectWorkingObject -cne $projectGitObject) {
        throw 'An executed runner, classifier, or project file differs from its reported Git blob.'
    }

    return [pscustomobject][ordered]@{
        clean = $true
        sourceHead = $sourceHead
        sourceTree = $sourceTree
        sourceCommitUtc = $sourceCommitUtc
        harnessHead = $harnessHead
        harnessTree = $harnessTree
        harnessCommitUtc = $harnessCommitUtc
        branch = $branch
        publishInput = [pscustomobject][ordered]@{
            pathspecs = $publish.pathspecs
            rowCount = $publish.rowCount
            sha256 = $publish.sha256
        }
        publishInputText = $publish.text
        workingPublishInput = $workingPublish
        build = $build
        campaignSchema = Get-CampaignSchema -CampaignStatePath $campaignStatePath
        runtimeSettingsSchema = Get-RuntimeSettingsSchema -RuntimeSettingsPath $runtimeSettingsPath
        runner = [pscustomobject][ordered]@{
            relativePath = $runnerRelative
            workingFile = Get-FileIdentity -Path $RunnerPath
            workingGitObject = $runnerWorkingObject
            gitObject = $runnerGitObject
            gitBlobSha256 = Get-GitBlobSha256 `
                -CheckoutRoot $CheckoutRoot `
                -Revision $harnessHead `
                -RelativePath $runnerRelative
        }
        classifier = [pscustomobject][ordered]@{
            relativePath = $classifierRelative
            workingFile = Get-FileIdentity -Path $ClassifierPath
            workingGitObject = $classifierWorkingObject
            gitObject = $classifierGitObject
            gitBlobSha256 = Get-GitBlobSha256 `
                -CheckoutRoot $CheckoutRoot `
                -Revision $harnessHead `
                -RelativePath $classifierRelative
        }
        project = [pscustomobject][ordered]@{
            relativePath = $projectRelative
            workingFile = Get-FileIdentity -Path $ProjectPath
            workingGitObject = $projectWorkingObject
            gitObject = $projectGitObject
            gitBlobSha256 = Get-GitBlobSha256 `
                -CheckoutRoot $CheckoutRoot `
                -Revision $sourceHead `
                -RelativePath $projectRelative
        }
    }
}

function Test-SourceBindingEqual {
    param(
        [Parameter(Mandatory = $true)]$First,
        [Parameter(Mandatory = $true)]$Second
    )

    return [bool]$Second.clean -and
        [string]$First.sourceHead -ceq [string]$Second.sourceHead -and
        [string]$First.sourceTree -ceq [string]$Second.sourceTree -and
        [string]$First.harnessHead -ceq [string]$Second.harnessHead -and
        [string]$First.harnessTree -ceq [string]$Second.harnessTree -and
        [string]$First.publishInput.sha256 -ceq [string]$Second.publishInput.sha256 -and
        [int]$First.publishInput.rowCount -eq [int]$Second.publishInput.rowCount -and
        [string]$First.workingPublishInput.sha256 -ceq
            [string]$Second.workingPublishInput.sha256 -and
        [int]$First.workingPublishInput.rowCount -eq
            [int]$Second.workingPublishInput.rowCount -and
        [string]$First.build.sha -ceq [string]$Second.build.sha -and
        [string]$First.build.utc -ceq [string]$Second.build.utc -and
        [string]$First.build.label -ceq [string]$Second.build.label -and
        [int]$First.campaignSchema -eq [int]$Second.campaignSchema -and
        [int]$First.runtimeSettingsSchema -eq [int]$Second.runtimeSettingsSchema -and
        [string]$First.runner.workingFile.sha256 -ceq [string]$Second.runner.workingFile.sha256 -and
        [string]$First.runner.workingGitObject -ceq [string]$Second.runner.workingGitObject -and
        [string]$First.runner.gitObject -ceq [string]$Second.runner.gitObject -and
        [string]$First.classifier.workingFile.sha256 -ceq [string]$Second.classifier.workingFile.sha256 -and
        [string]$First.classifier.workingGitObject -ceq [string]$Second.classifier.workingGitObject -and
        [string]$First.classifier.gitObject -ceq [string]$Second.classifier.gitObject -and
        [string]$First.project.workingFile.sha256 -ceq [string]$Second.project.workingFile.sha256 -and
        [string]$First.project.workingGitObject -ceq [string]$Second.project.workingGitObject -and
        [string]$First.project.gitObject -ceq [string]$Second.project.gitObject
}

function Import-CampaignDebugClassifierLibrary {
    param(
        [Parameter(Mandatory = $true)][string]$ClassifierPath,
        [switch]$IncludeSelfTests
    )

    $tokens = $null
    $parseErrors = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile(
        $ClassifierPath,
        [ref]$tokens,
        [ref]$parseErrors)
    if (@($parseErrors).Count -ne 0) {
        throw 'The Campaign Debug classifier source has parser errors.'
    }
    $definitions = @{}
    foreach ($statement in @($ast.EndBlock.Statements)) {
        if ($statement -is [Management.Automation.Language.FunctionDefinitionAst]) {
            $definitions[$statement.Name] = $statement
        }
    }

    $roots = @('Test-CampaignDebugArtifacts', 'Get-GuardErrorCensus')
    if ($IncludeSelfTests) {
        $roots += @(
            'Test-CampaignDebugHardDiagnosticCensus',
            'Invoke-ArtifactValidatorSelfTest')
    }
    $queue = New-Object 'Collections.Generic.Queue[string]'
    $selected = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::OrdinalIgnoreCase)
    foreach ($root in $roots) {
        $queue.Enqueue($root)
    }
    while ($queue.Count -gt 0) {
        $name = $queue.Dequeue()
        if (-not $selected.Add($name)) {
            continue
        }
        if (-not $definitions.ContainsKey($name)) {
            throw "Required Campaign Debug classifier function $name is missing."
        }
        $commands = @($definitions[$name].Body.FindAll(
                { param($node) $node -is [Management.Automation.Language.CommandAst] },
                $true) |
            ForEach-Object { $_.GetCommandName() } |
            Where-Object { $_ -and $definitions.ContainsKey($_) } |
            Sort-Object -Unique)
        foreach ($command in $commands) {
            if (-not $selected.Contains($command)) {
                $queue.Enqueue($command)
            }
        }
    }
    foreach ($name in @($selected | Sort-Object)) {
        Set-Item `
            -Path ("Function:\script:{0}" -f $name) `
            -Value $definitions[$name].Body.GetScriptBlock() `
            -Force
    }
    return [pscustomobject][ordered]@{
        parserErrors = 0
        importedFunctionCount = $selected.Count
        importedFunctions = @($selected | Sort-Object)
    }
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

function Test-ExactNativeArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$CommandLine,
        [Parameter(Mandatory = $true)][string]$ExpectedExecutable,
        [Parameter(Mandatory = $true)][string[]]$ExpectedArguments
    )

    $tokens = @([PartisanSourceCampaignDebugNativeCommandLine]::Split($CommandLine))
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
    if (-not $actualExecutable.Equals(
            $expectedExecutablePath,
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

function New-SourceRuntimeArguments {
    param(
        [Parameter(Mandatory = $true)][string]$ResolvedAddonDirectory,
        [Parameter(Mandatory = $true)][string]$AddonTempDirectory,
        [Parameter(Mandatory = $true)][string]$ResolvedProjectFile,
        [Parameter(Mandatory = $true)][string]$ResolvedWorldResource,
        [Parameter(Mandatory = $true)][string]$IsolationRoot,
        [Parameter(Mandatory = $true)][string]$ResolvedProfile
    )

    return @(
        '-addonsDir', $ResolvedAddonDirectory,
        '-addonTempDir', $AddonTempDirectory,
        '-gproj', $ResolvedProjectFile,
        '-world', $ResolvedWorldResource,
        '-window',
        '-noFocus',
        '-posX', '20',
        '-posY', '20',
        '-screenWidth', '1280',
        '-screenHeight', '720',
        '-forceupdate',
        '-rpl-timeout-disable',
        '-noThrow',
        '-profile', $IsolationRoot,
        '-hstCampaignDebugProfile', $ResolvedProfile
    )
}

function Get-EngineProcessRows {
    $processNames = @(
        'ArmaReforger',
        'ArmaReforger_BE',
        'ArmaReforgerSteam',
        'ArmaReforgerSteamDiag',
        'ArmaReforgerServer',
        'ArmaReforgerServerDiag',
        'ArmaReforgerWorkbench',
        'ArmaReforgerWorkbenchDiag',
        'ArmaReforgerWorkbenchSteamDiag',
        'CrashReporter'
    )
    return @(Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $processNames -contains $_.ProcessName })
}

function ConvertTo-ProcessEvidenceRows {
    param([object[]]$Processes = @())

    $rows = New-Object Collections.Generic.List[object]
    foreach ($process in @($Processes)) {
        $started = ''
        try {
            $started = $process.StartTime.ToUniversalTime().ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
        }
        catch { }
        [void]$rows.Add([pscustomobject][ordered]@{
            id = [int]$process.Id
            name = [string]$process.ProcessName
            startUtc = $started
        })
    }
    return $rows.ToArray()
}

function New-SourceCampaignProcessCensus {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [object[]]$BeforeProcesses,
        [Parameter(Mandatory = $true)][int]$RootProcessId,
        [Parameter(Mandatory = $true)][datetime]$RootStartUtc,
        [Parameter(Mandatory = $true)][int]$MaximumOwnedProcesses,
        [Parameter(Mandatory = $true)][int]$OwnedProcessesRemaining,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [string[]]$UnclaimedEngineProcessesObserved,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [object[]]$AfterProcesses,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [string[]]$CleanupErrors
    )

    return [pscustomobject][ordered]@{
        before = @(ConvertTo-ProcessEvidenceRows -Processes $BeforeProcesses)
        rootProcessId = $RootProcessId
        rootStartUtc = if ($RootStartUtc -ne [DateTime]::MinValue) {
            $RootStartUtc.ToString('o', [Globalization.CultureInfo]::InvariantCulture)
        } else { '' }
        maximumOwnedProcesses = $MaximumOwnedProcesses
        ownedProcessesRemaining = $OwnedProcessesRemaining
        unclaimedEngineProcessesObserved = @($UnclaimedEngineProcessesObserved)
        after = @(ConvertTo-ProcessEvidenceRows -Processes $AfterProcesses)
        cleanupErrors = @($CleanupErrors)
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

function Update-OwnedProcesses {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Owned,
        [Parameter(Mandatory = $true)][int]$RootProcessId,
        [Parameter(Mandatory = $true)][datetime]$RootStartUtc,
        $Job
    )

    $rows = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $jobIds = @()
    if ($Job) {
        try { $jobIds = @($Job.GetProcessIds()) } catch { $jobIds = @() }
    }
    foreach ($jobId in $jobIds) {
        $candidateId = [int]$jobId
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
        catch { }
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
            catch { }
        }
    }
    if (-not $Owned.ContainsKey($RootProcessId)) {
        try {
            $root = Get-Process -Id $RootProcessId -ErrorAction Stop
            $actualStart = $root.StartTime.ToUniversalTime()
            if ($actualStart.Ticks -eq $RootStartUtc.Ticks) {
                $Owned[$RootProcessId] = $actualStart
            }
        }
        catch { }
    }
}

function Stop-OwnedProcesses {
    param([Parameter(Mandatory = $true)][hashtable]$Owned)

    foreach ($processId in @($Owned.Keys)) {
        try {
            $process = Get-Process -Id ([int]$processId) -ErrorAction Stop
            $expectedStart = $Owned[[int]$processId]
            if ($process.StartTime.ToUniversalTime().Ticks -ne
                $expectedStart.ToUniversalTime().Ticks) {
                continue
            }
            $process.Kill()
        }
        catch { }
    }
}

function Get-SharedFileText {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
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
        if ($reader) { $reader.Dispose() } elseif ($stream) { $stream.Dispose() }
    }
}

function Get-ScriptLogTail {
    param([Parameter(Mandatory = $true)][string]$IsolationRoot)

    $logRoot = Join-Path $IsolationRoot 'logs'
    if (-not (Test-Path -LiteralPath $logRoot -PathType Container)) {
        return ''
    }
    $logs = @(Get-ChildItem `
        -LiteralPath $logRoot `
        -Recurse `
        -File `
        -Filter 'script.log' `
        -Force `
        -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending)
    if ($logs.Count -eq 0) {
        return ''
    }
    return Get-SharedFileText -Path $logs[0].FullName -MaximumBytes 4194304
}

function Get-FileSignature {
    param([Parameter(Mandatory = $true)][string[]]$Paths)

    $parts = New-Object Collections.Generic.List[string]
    foreach ($path in $Paths) {
        $file = Get-Item -LiteralPath $path -Force
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        [void]$parts.Add("$($file.Length):$($file.LastWriteTimeUtc.Ticks):$hash")
    }
    return $parts -join '|'
}

function Test-SourceCampaignExactSequence {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]]$Expected,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]]$Actual
    )

    if ($Expected.Count -ne $Actual.Count) { return $false }
    for ($index = 0; $index -lt $Expected.Count; $index++) {
        if ($Expected[$index] -cne $Actual[$index]) { return $false }
    }
    return $true
}

function Test-SourceCampaignExactMultiset {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]]$Expected,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]]$Actual
    )

    if ($Expected.Count -ne $Actual.Count) { return $false }
    [string[]]$expectedRows = @($Expected)
    [string[]]$actualRows = @($Actual)
    [Array]::Sort($expectedRows, [StringComparer]::Ordinal)
    [Array]::Sort($actualRows, [StringComparer]::Ordinal)
    return Test-SourceCampaignExactSequence -Expected $expectedRows -Actual $actualRows
}

function Get-SourceCampaignTimestampSeconds {
    param([Parameter(Mandatory = $true)][string]$Timestamp)

    $parts = $Timestamp -split '[:.]'
    if ($parts.Count -ne 4) { return [double]-1 }
    $hours = 0
    $minutes = 0
    $seconds = 0
    $fraction = [double]0
    if (-not [int]::TryParse($parts[0], [ref]$hours) -or
        -not [int]::TryParse($parts[1], [ref]$minutes) -or
        -not [int]::TryParse($parts[2], [ref]$seconds) -or
        -not [double]::TryParse(
            ('0.' + $parts[3]),
            [Globalization.NumberStyles]::AllowDecimalPoint,
            [Globalization.CultureInfo]::InvariantCulture,
            [ref]$fraction)) {
        return [double]-1
    }
    if ($hours -lt 0 -or $hours -gt 23 -or
        $minutes -lt 0 -or $minutes -gt 59 -or
        $seconds -lt 0 -or $seconds -gt 59 -or
        $fraction -lt 0 -or $fraction -ge 1) {
        return [double]-1
    }
    return [double]($hours * 3600 + $minutes * 60 + $seconds) + $fraction
}

function Get-SourceCampaignForwardSecondsDelta {
    param(
        [Parameter(Mandatory = $true)][double]$Earlier,
        [Parameter(Mandatory = $true)][double]$Later
    )

    if ($Earlier -lt 0 -or $Later -lt 0) {
        return [double]::PositiveInfinity
    }
    $delta = $Later - $Earlier
    if ($delta -lt 0) { $delta += 86400 }
    return $delta
}

function Test-SourceCampaignIntentionalResourceBindings {
    param(
        [AllowEmptyString()][Parameter(Mandatory = $true)][string[]]$Lines,
        [AllowEmptyCollection()][Parameter(Mandatory = $true)][object[]]$Rows,
        [Parameter(Mandatory = $true)][int]$StartIndex,
        [Parameter(Mandatory = $true)][int]$DoneIndex
    )

    $bindings = @($script:SourceCampaignFullIntentionalResourceBindings)
    if ($StartIndex -lt 0 -or $DoneIndex -le $StartIndex -or
        $Rows.Count -ne ($bindings.Count * 2)) {
        return $false
    }
    $boundaryPattern =
        'Partisan campaign debug \| (?:PASS|WARN|FAIL|BLOCKED|SKIPPED) \| |' +
        'Partisan exact mission convoy \|'
    for ($bindingIndex = 0; $bindingIndex -lt $bindings.Count; $bindingIndex++) {
        $binding = $bindings[$bindingIndex]
        $firstRow = $Rows[$bindingIndex * 2]
        $secondRow = $Rows[$bindingIndex * 2 + 1]
        if ([int]$secondRow.lineIndex -ne ([int]$firstRow.lineIndex + 2)) {
            return $false
        }

        $normalRowPattern =
            '^\s*(?<timestamp>\d{2}:\d{2}:\d{2}\.\d+)\s+RESOURCES\s+:\s+' +
            [regex]::Escape(
                "GetResourceObject '$([string]$binding.resourcePath)'") +
            '\s*$'
        $normalRow = [regex]::Match(
            [string]$Lines[[int]$firstRow.lineIndex + 1],
            $normalRowPattern)
        $normalTimestampSeconds = if ($normalRow.Success) {
            Get-SourceCampaignTimestampSeconds `
                -Timestamp $normalRow.Groups['timestamp'].Value
        }
        else {
            [double]-1
        }
        if (-not $normalRow.Success -or $normalTimestampSeconds -lt 0 -or
            (Get-SourceCampaignForwardSecondsDelta `
                -Earlier $firstRow.timestampSeconds `
                -Later $normalTimestampSeconds) -gt 1.0 -or
            (Get-SourceCampaignForwardSecondsDelta `
                -Earlier $normalTimestampSeconds `
                -Later $secondRow.timestampSeconds) -gt 1.0 -or
            (Get-SourceCampaignForwardSecondsDelta `
                -Earlier $firstRow.timestampSeconds `
                -Later $secondRow.timestampSeconds) -gt 1.0) {
            return $false
        }

        $previousBoundary = -1
        for ($candidateIndex = [int]$firstRow.lineIndex - 1;
            $candidateIndex -ge [Math]::Max(0, [int]$firstRow.lineIndex - 20);
            $candidateIndex--) {
            if ([string]$Lines[$candidateIndex] -cmatch $boundaryPattern) {
                $previousBoundary = $candidateIndex
                break
            }
        }
        $nextBoundary = -1
        for ($candidateIndex = [int]$secondRow.lineIndex + 1;
            $candidateIndex -le [Math]::Min(
                $Lines.Count - 1,
                [int]$secondRow.lineIndex + 20);
            $candidateIndex++) {
            if ([string]$Lines[$candidateIndex] -cmatch $boundaryPattern) {
                $nextBoundary = $candidateIndex
                break
            }
        }
        if ($previousBoundary -le $StartIndex -or
            $nextBoundary -le $StartIndex -or
            $previousBoundary -ge $DoneIndex -or
            $nextBoundary -ge $DoneIndex -or
            -not ([string]$Lines[$previousBoundary]).TrimEnd().EndsWith(
                [string]$binding.previousMarker,
                [StringComparison]::Ordinal) -or
            -not ([string]$Lines[$nextBoundary]).TrimEnd().EndsWith(
                [string]$binding.nextMarker,
                [StringComparison]::Ordinal)) {
            return $false
        }
    }
    return $true
}

function Get-SourceCampaignRuntimePathBoundaryIndex {
    param(
        [AllowEmptyString()][Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][int]$FirstLineIndex,
        [Parameter(Mandatory = $true)][int]$LastLineIndex,
        [Parameter(Mandatory = $true)][int]$StartIndex,
        [Parameter(Mandatory = $true)][int]$DoneIndex
    )

    if ($StartIndex -lt 0 -or $DoneIndex -le $StartIndex -or
        $FirstLineIndex -le $StartIndex -or $LastLineIndex -ge $DoneIndex) {
        return -1
    }
    $caseBoundaryPattern =
        'Partisan campaign debug \| (?<status>PASS|WARN|FAIL|BLOCKED|SKIPPED) \| ' +
        '(?<caseId>[^|]+) \|'
    for ($candidateIndex = $FirstLineIndex;
        $candidateIndex -le $LastLineIndex;
        $candidateIndex++) {
        if ([string]$Lines[$candidateIndex] -cmatch $caseBoundaryPattern) {
            return -1
        }
    }
    $missionIdPattern =
        '(?:logistics_(?:ammo|weapons)_truck|rescue_refugees|' +
        'dynamic_(?:defend_petros|stop_tower_rebuild|minor_city_task))'
    $allowedCasePattern =
        '^(?:post_case_cleanup\.(?:mission_cleanup|mission_sweep_(?:start|runtime)|' +
        'primitive_runtime)_|mission_cleanup\.|mission_sweep\.(?:start|runtime)\.|' +
        'primitive_runtime\.)' + $missionIdPattern + '(?:[._].*)?$'
    for ($distance = 1; $distance -le 40; $distance++) {
        $candidates = New-Object Collections.Generic.List[object]
        foreach ($candidateIndex in @(
                ($FirstLineIndex - $distance),
                ($LastLineIndex + $distance))) {
            if ($candidateIndex -lt 0 -or $candidateIndex -ge $Lines.Count) {
                continue
            }
            if ($candidateIndex -le $StartIndex -or
                $candidateIndex -ge $DoneIndex) {
                continue
            }
            $match = [regex]::Match(
                [string]$Lines[$candidateIndex],
                $caseBoundaryPattern)
            if ($match.Success) {
                [void]$candidates.Add([pscustomobject][ordered]@{
                    index = $candidateIndex
                    status = $match.Groups['status'].Value
                    caseId = $match.Groups['caseId'].Value.Trim()
                })
            }
        }
        if ($candidates.Count -eq 0) { continue }
        $candidateMissionIds = New-Object Collections.Generic.List[string]
        foreach ($candidate in $candidates) {
            if ([string]$candidate.status -cne 'PASS' -and
                [string]$candidate.status -cne 'WARN') {
                return -1
            }
            if ([string]$candidate.caseId -cnotmatch $allowedCasePattern) {
                return -1
            }
            $missionMatch = [regex]::Match(
                [string]$candidate.caseId,
                $missionIdPattern)
            if (-not $missionMatch.Success) { return -1 }
            [void]$candidateMissionIds.Add($missionMatch.Value)
        }
        if (@($candidateMissionIds | Sort-Object -Unique).Count -ne 1) {
            return -1
        }
        return [int]$candidates[0].index
    }
    return -1
}

function Get-SourceCampaignAmbientErrorCensusFromText {
    param(
        [AllowEmptyString()][Parameter(Mandatory = $true)][string]$ConsoleText,
        [Parameter(Mandatory = $true)]
        [ValidateSet('full_certification', 'force_authority')]
        [string]$Profile
    )

    $lines = @($ConsoleText -split "`r?`n")
    $ambientRows = New-Object Collections.Generic.List[object]
    $malformedRows = New-Object Collections.Generic.List[string]
    $problems = New-Object Collections.Generic.List[string]
    $hardRowPattern =
        '^\s*(?<timestamp>\d{2}:\d{2}:\d{2}\.\d+)\s+' +
        '(?<channel>[A-Z][A-Z0-9_ ]*?)\s*' +
        '\((?<severity>[EF])\):\s*(?<message>.*)$'
    $hardTokenPattern =
        '^\s*\d{2}:\d{2}:\d{2}\.\d+\s+.*\([EF]\):'
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        $line = [string]$lines[$lineIndex]
        $match = [regex]::Match($line, $hardRowPattern)
        if ($match.Success) {
            $channel = $match.Groups['channel'].Value.Trim()
            if ($channel -ceq 'SCRIPT' -or $channel -ceq 'ENGINE') { continue }
            $signature = $channel + "`t" +
                $match.Groups['severity'].Value + "`t" +
                $match.Groups['message'].Value
            $timestampSeconds = Get-SourceCampaignTimestampSeconds `
                -Timestamp $match.Groups['timestamp'].Value
            if ($timestampSeconds -lt 0) {
                [void]$malformedRows.Add((Get-TextSha256 -Text $line))
                continue
            }
            [void]$ambientRows.Add([pscustomobject][ordered]@{
                lineIndex = $lineIndex
                timestamp = $match.Groups['timestamp'].Value
                timestampSeconds = $timestampSeconds
                channel = $channel
                severity = $match.Groups['severity'].Value
                signature = $signature
            })
        }
        elseif ($line -cmatch $hardTokenPattern) {
            [void]$malformedRows.Add((Get-TextSha256 -Text $line))
        }
    }

    $armNeedle = if ($Profile -ceq 'force_authority') {
        'Partisan campaign debug CLI | armed focused force_authority run'
    }
    else {
        'Partisan campaign debug CLI | armed exact HST_Dev full certification run'
    }
    $startNeedle =
        "Partisan campaign debug CLI | started $Profile on attempt "
    $armIndexes = @()
    $startIndexes = @()
    $doneIndexes = @()
    $destroyIndexes = @()
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        $line = [string]$lines[$lineIndex]
        if ($line.IndexOf($armNeedle, [StringComparison]::Ordinal) -ge 0) {
            $armIndexes += $lineIndex
        }
        if ($line.IndexOf($startNeedle, [StringComparison]::Ordinal) -ge 0 -and
            $line -cmatch ([regex]::Escape($startNeedle) + '\d+$')) {
            $startIndexes += $lineIndex
        }
        if ($line.IndexOf(
                'Partisan campaign debug | DONE |',
                [StringComparison]::Ordinal) -ge 0) {
            $doneIndexes += $lineIndex
        }
        if ($line -cmatch 'ENGINE\s+:\s+Game destroyed\.$') {
            $destroyIndexes += $lineIndex
        }
    }
    $lifecycleExact = $armIndexes.Count -eq 1 -and $startIndexes.Count -eq 1 -and
        $doneIndexes.Count -eq 1 -and $destroyIndexes.Count -eq 1 -and
        $armIndexes[0] -lt $startIndexes[0] -and
        $startIndexes[0] -lt $doneIndexes[0] -and
        $doneIndexes[0] -lt $destroyIndexes[0]
    if (-not $lifecycleExact) {
        [void]$problems.Add('lifecycle')
    }
    $armIndex = if ($armIndexes.Count -eq 1) { $armIndexes[0] } else { -1 }
    $startIndex = if ($startIndexes.Count -eq 1) { $startIndexes[0] } else { -1 }
    $doneIndex = if ($doneIndexes.Count -eq 1) { $doneIndexes[0] } else { -1 }
    $destroyIndex = if ($destroyIndexes.Count -eq 1) { $destroyIndexes[0] } else { -1 }
    $approvedIndexes = New-Object 'Collections.Generic.HashSet[int]'

    if ($ambientRows.Count -eq 0) {
        if ($Profile -ceq 'full_certification') {
            [void]$problems.Add('runtime-resources')
        }
        $validAbsent = $lifecycleExact -and $problems.Count -eq 0 -and
            $malformedRows.Count -eq 0
        return [pscustomobject][ordered]@{
            valid = $validAbsent
            completeOrAbsent = $validAbsent
            lifecycleExact = $lifecycleExact
            approvedAmbientDiagnosticCount = 0
            expectedCompleteDiagnosticCount = if (
                $Profile -ceq 'full_certification') {
                $script:SourceCampaignFullIntentionalResourceErrors.Count
            }
            else { 0 }
            expectedTeardownResourceCount = 0
            startupGuiCount = 0
            startupPathfindingCount = 0
            mapMaskCount = 0
            intentionalResourceCount = 0
            runtimePathfindingBurstCount = 0
            teardownResourceCount = 0
            unapprovedAmbientDiagnosticCount = 0
            malformedHardDiagnosticCount = $malformedRows.Count
            problemCodes = @($problems | Sort-Object -Unique)
            unapprovedSignatureSha256 = @()
            malformedLineSha256 = $malformedRows.ToArray()
        }
    }

    $startupGuiRows = @($ambientRows | Where-Object {
        $_.lineIndex -lt $armIndex -and
        [string]$_.signature -ceq $script:SourceCampaignStartupGuiError
    })
    if ($startupGuiRows.Count -le 1) {
        foreach ($row in $startupGuiRows) { [void]$approvedIndexes.Add($row.lineIndex) }
    }
    else {
        [void]$problems.Add('startup-gui')
    }
    $startupPathRows = @($ambientRows | Where-Object {
        $_.lineIndex -lt $armIndex -and [string]$_.channel -ceq 'PATHFINDING'
    })
    $startupPathExact = $startupPathRows.Count -eq 0 -or
        ((Test-SourceCampaignExactMultiset `
                -Expected $script:SourceCampaignStartupPathfindingErrors `
                -Actual @($startupPathRows | ForEach-Object signature)) -and
            ((Get-SourceCampaignForwardSecondsDelta `
                    -Earlier $startupPathRows[0].timestampSeconds `
                    -Later $startupPathRows[-1].timestampSeconds) -le 1.0))
    if ($startupPathExact) {
        foreach ($row in $startupPathRows) { [void]$approvedIndexes.Add($row.lineIndex) }
    }
    else {
        [void]$problems.Add('startup-pathfinding')
    }

    $mapMaskRows = @($ambientRows | Where-Object {
        $_.lineIndex -gt $startIndex -and $_.lineIndex -lt $doneIndex -and
        [string]$_.signature -ceq $script:SourceCampaignMapMaskError
    })
    $usedMapBindings = New-Object 'Collections.Generic.HashSet[int]'
    foreach ($row in $mapMaskRows) {
        $bindingIndex = -1
        for ($candidateIndex = $row.lineIndex - 1;
            $candidateIndex -ge [Math]::Max(
                $startIndex + 1,
                $row.lineIndex - 20);
            $candidateIndex--) {
            $candidate = [string]$lines[$candidateIndex]
            if ($candidate.IndexOf(
                    'Partisan player map marker debug | refresh requested player spawned',
                    [StringComparison]::Ordinal) -ge 0 -or
                $candidate.IndexOf(
                    'Partisan request bridge debug | map ui ready',
                    [StringComparison]::Ordinal) -ge 0) {
                $bindingIndex = $candidateIndex
                break
            }
        }
        if ($bindingIndex -ge 0 -and $usedMapBindings.Add($bindingIndex)) {
            [void]$approvedIndexes.Add($row.lineIndex)
        }
        else {
            [void]$problems.Add('map-mask-binding')
        }
    }

    $runtimeResourceRows = @($ambientRows | Where-Object {
        $_.lineIndex -gt $startIndex -and $_.lineIndex -lt $doneIndex -and
        [string]$_.channel -ceq 'RESOURCES'
    })
    $runtimeResourcesExact = $Profile -ceq 'force_authority' -and
        $runtimeResourceRows.Count -eq 0
    $runtimeResourceBindingsExact = $runtimeResourcesExact
    if ($Profile -ceq 'full_certification') {
        $runtimeResourcesExact = Test-SourceCampaignExactSequence `
            -Expected $script:SourceCampaignFullIntentionalResourceErrors `
            -Actual @($runtimeResourceRows | ForEach-Object signature)
        $runtimeResourceBindingsExact = $runtimeResourcesExact -and
            (Test-SourceCampaignIntentionalResourceBindings `
                -Lines $lines `
                -Rows $runtimeResourceRows `
                -StartIndex $startIndex `
                -DoneIndex $doneIndex)
    }
    if ($runtimeResourcesExact -and $runtimeResourceBindingsExact) {
        foreach ($row in $runtimeResourceRows) { [void]$approvedIndexes.Add($row.lineIndex) }
    }
    else {
        [void]$problems.Add('runtime-resources')
        if ($runtimeResourcesExact -and -not $runtimeResourceBindingsExact) {
            [void]$problems.Add('runtime-resource-binding')
        }
    }

    $runtimePathRows = @($ambientRows | Where-Object {
        $_.lineIndex -gt $startIndex -and $_.lineIndex -lt $doneIndex -and
        [string]$_.channel -ceq 'PATHFINDING'
    })
    $runtimeBursts = New-Object Collections.Generic.List[object]
    $currentBurst = New-Object Collections.Generic.List[object]
    foreach ($row in $runtimePathRows) {
        if ($currentBurst.Count -gt 0 -and
            (Get-SourceCampaignForwardSecondsDelta `
                -Earlier $currentBurst[$currentBurst.Count - 1].timestampSeconds `
                -Later $row.timestampSeconds) -gt 1.0) {
            [void]$runtimeBursts.Add($currentBurst.ToArray())
            $currentBurst = New-Object Collections.Generic.List[object]
        }
        [void]$currentBurst.Add($row)
    }
    if ($currentBurst.Count -gt 0) { [void]$runtimeBursts.Add($currentBurst.ToArray()) }
    $usedPathBindings = New-Object 'Collections.Generic.HashSet[int]'
    $familyBCount = 0
    foreach ($burst in $runtimeBursts) {
        $burstRows = @($burst)
        $burstSignatures = @($burstRows | ForEach-Object signature)
        $familyA = Test-SourceCampaignExactMultiset `
            -Expected $script:SourceCampaignRuntimePathfindingFamilyA `
            -Actual $burstSignatures
        $familyB = Test-SourceCampaignExactMultiset `
            -Expected $script:SourceCampaignRuntimePathfindingFamilyB `
            -Actual $burstSignatures
        $bindingIndex = -1
        if ($Profile -ceq 'full_certification' -and $familyA) {
            $bindingIndex = Get-SourceCampaignRuntimePathBoundaryIndex `
                -Lines $lines `
                -FirstLineIndex $burstRows[0].lineIndex `
                -LastLineIndex $burstRows[-1].lineIndex `
                -StartIndex $startIndex `
                -DoneIndex $doneIndex
        }
        elseif ($Profile -ceq 'full_certification' -and $familyB -and
            $familyBCount -eq 0) {
            $familyBBindings = New-Object Collections.Generic.List[int]
            $familyBDisallowedBoundaries = New-Object Collections.Generic.List[int]
            for ($candidateIndex = [Math]::Max($startIndex + 1,
                    $burstRows[0].lineIndex - 40);
                $candidateIndex -le [Math]::Min($doneIndex - 1,
                    $burstRows[-1].lineIndex + 40);
                $candidateIndex++) {
                if ([string]$lines[$candidateIndex] -cmatch
                    'Partisan campaign debug \| PASS \| phase18\.phase18_counterattack \|') {
                    [void]$familyBBindings.Add($candidateIndex)
                }
                if ([string]$lines[$candidateIndex] -cmatch
                    'Partisan campaign debug \| (?:FAIL|BLOCKED|SKIPPED) \|') {
                    [void]$familyBDisallowedBoundaries.Add($candidateIndex)
                }
            }
            if ($familyBBindings.Count -eq 1 -and
                $familyBDisallowedBoundaries.Count -eq 0) {
                $bindingIndex = $familyBBindings[0]
                $familyBCount++
            }
        }
        if ($bindingIndex -ge 0 -and $usedPathBindings.Add($bindingIndex)) {
            foreach ($row in $burstRows) { [void]$approvedIndexes.Add($row.lineIndex) }
        }
        else {
            [void]$problems.Add('runtime-pathfinding-binding')
        }
    }

    $teardownRows = @($ambientRows | Where-Object {
        $_.lineIndex -gt $destroyIndex
    })
    $expectedTeardownRows = if ($Profile -ceq 'force_authority') {
        @($script:SourceCampaignFocusedTeardownResourceErrors)
    }
    else {
        @($script:SourceCampaignFullTeardownResourceErrors)
    }
    $teardownExact = $teardownRows.Count -eq 0
    if ($teardownRows.Count -eq $expectedTeardownRows.Count) {
        $teardownExact = Test-SourceCampaignExactMultiset `
            -Expected $expectedTeardownRows `
            -Actual @($teardownRows | ForEach-Object signature)
        if ($teardownExact) {
            $teardownSpanSeconds = [double]0
            for ($index = 0; $index -lt $teardownRows.Count; $index++) {
                if ([string]$teardownRows[$index].channel -cne 'RESOURCES' -or
                    ($index -gt 0 -and
                        $teardownRows[$index].lineIndex -ne
                            ($teardownRows[$index - 1].lineIndex + 1))) {
                    $teardownExact = $false
                    break
                }
                if ($index -gt 0) {
                    $teardownStepSeconds = Get-SourceCampaignForwardSecondsDelta `
                        -Earlier $teardownRows[$index - 1].timestampSeconds `
                        -Later $teardownRows[$index].timestampSeconds
                    if ($teardownStepSeconds -gt 1.0) {
                        $teardownExact = $false
                        break
                    }
                    $teardownSpanSeconds += $teardownStepSeconds
                    if ($teardownSpanSeconds -gt 1.0) {
                        $teardownExact = $false
                        break
                    }
                }
            }
        }
    }
    if ($teardownExact) {
        foreach ($row in $teardownRows) { [void]$approvedIndexes.Add($row.lineIndex) }
    }
    else {
        [void]$problems.Add('teardown-resources')
    }

    $unapprovedRows = @($ambientRows | Where-Object {
        -not $approvedIndexes.Contains([int]$_.lineIndex)
    })
    $teardownExpectedContribution = if ($teardownRows.Count -eq 0) {
        0
    }
    else {
        $expectedTeardownRows.Count
    }
    $valid = $lifecycleExact -and $problems.Count -eq 0 -and
        $unapprovedRows.Count -eq 0 -and $malformedRows.Count -eq 0
    return [pscustomobject][ordered]@{
        valid = $valid
        completeOrAbsent = $valid
        lifecycleExact = $lifecycleExact
        approvedAmbientDiagnosticCount = $approvedIndexes.Count
        expectedCompleteDiagnosticCount =
            $startupGuiRows.Count + $startupPathRows.Count + $mapMaskRows.Count +
            $runtimeResourceRows.Count + $runtimePathRows.Count +
            $teardownExpectedContribution
        expectedTeardownResourceCount = $teardownExpectedContribution
        startupGuiCount = $startupGuiRows.Count
        startupPathfindingCount = $startupPathRows.Count
        mapMaskCount = $mapMaskRows.Count
        intentionalResourceCount = $runtimeResourceRows.Count
        runtimePathfindingBurstCount = $runtimeBursts.Count
        teardownResourceCount = $teardownRows.Count
        unapprovedAmbientDiagnosticCount = $unapprovedRows.Count
        malformedHardDiagnosticCount = $malformedRows.Count
        problemCodes = @($problems | Sort-Object -Unique)
        unapprovedSignatureSha256 = @($unapprovedRows | ForEach-Object {
                Get-TextSha256 -Text ([string]$_.signature)
            })
        malformedLineSha256 = $malformedRows.ToArray()
    }
}

function Get-SourceCampaignAmbientErrorCensus {
    param(
        [Parameter(Mandatory = $true)][string]$IsolationRoot,
        [Parameter(Mandatory = $true)]
        [ValidateSet('full_certification', 'force_authority')]
        [string]$Profile
    )

    $logRoot = Join-Path $IsolationRoot 'logs'
    $consoleLogs = @()
    if (Test-Path -LiteralPath $logRoot -PathType Container) {
        $consoleLogs = @(Get-ChildItem `
            -LiteralPath $logRoot `
            -Recurse `
            -File `
            -Filter 'console.log' `
            -Force `
            -ErrorAction Stop)
    }
    if ($consoleLogs.Count -ne 1) {
        return [pscustomobject][ordered]@{
            valid = $false
            consoleLogCount = $consoleLogs.Count
            completeOrAbsent = $false
            lifecycleExact = $false
            approvedAmbientDiagnosticCount = 0
            expectedCompleteDiagnosticCount = 0
            expectedTeardownResourceCount = 0
            startupGuiCount = 0
            startupPathfindingCount = 0
            mapMaskCount = 0
            intentionalResourceCount = 0
            runtimePathfindingBurstCount = 0
            teardownResourceCount = 0
            unapprovedAmbientDiagnosticCount = 0
            malformedHardDiagnosticCount = 0
            problemCodes = @('console-log-count')
            unapprovedSignatureSha256 = @()
            malformedLineSha256 = @()
        }
    }
    $result = Get-SourceCampaignAmbientErrorCensusFromText `
        -ConsoleText (Get-SharedFileText -Path $consoleLogs[0].FullName) `
        -Profile $Profile
    $result | Add-Member -NotePropertyName consoleLogCount -NotePropertyValue 1
    return $result
}

function Get-SourceMountAttestation {
    param(
        [Parameter(Mandatory = $true)][string]$IsolationRoot,
        [Parameter(Mandatory = $true)][string]$ExpectedProjectPath,
        [Parameter(Mandatory = $true)][string]$ExpectedAddonGuid,
        [Parameter(Mandatory = $true)][string]$ExpectedResourceDatabasePath,
        [Parameter(Mandatory = $true)]$ExpectedResourceDatabaseIdentity
    )

    $logRoot = Join-Path $IsolationRoot 'logs'
    $consoleLogs = @()
    if (Test-Path -LiteralPath $logRoot -PathType Container) {
        $consoleLogs = @(Get-ChildItem `
            -LiteralPath $logRoot `
            -Recurse `
            -File `
            -Filter 'console.log' `
            -Force `
            -ErrorAction Stop)
    }
    $expectedPath = [IO.Path]::GetFullPath($ExpectedProjectPath).Replace('\', '/')
    $expectedResourceDatabase = [IO.Path]::GetFullPath(
        $ExpectedResourceDatabasePath).Replace('\', '/')
    $checkoutRoot = [IO.Path]::GetFullPath(
        (Split-Path -Parent $ExpectedProjectPath)).TrimEnd('\', '/').Replace('\', '/')
    $projectRows = New-Object Collections.Generic.List[object]
    $resourceRows = New-Object Collections.Generic.List[object]
    foreach ($log in $consoleLogs) {
        foreach ($line in @((Get-SharedFileText -Path $log.FullName) -split "`r?`n")) {
            $projectCandidate = [Regex]::Match(
                [string]$line,
                "gproj:\s+'(?<path>[^']+)'(?<rest>.*)$")
            if ($projectCandidate.Success) {
                $observedProject = $projectCandidate.Groups['path'].Value.Replace('\', '/')
                $pathMatches = $observedProject.Equals(
                    $expectedPath,
                    [StringComparison]::OrdinalIgnoreCase)
                $projectDetails = [Regex]::Match(
                    $projectCandidate.Groups['rest'].Value,
                    "^\s+guid:\s+'(?<guid>[^']+)'(?<suffix>.*)$")
                $guidMatches = $projectDetails.Success -and
                    $projectDetails.Groups['guid'].Value -ceq $ExpectedAddonGuid
                $guidReferenced = ([string]$line).IndexOf(
                    $ExpectedAddonGuid,
                    [StringComparison]::Ordinal) -ge 0
                if ($pathMatches -or $guidMatches -or $guidReferenced) {
                    [void]$projectRows.Add([pscustomobject][ordered]@{
                        syntaxExact = $projectDetails.Success
                        projectMatches = $pathMatches
                        guidMatches = $guidMatches
                        generatedArchive =
                            [string]$line -match '(?i)\(packed\)'
                    })
                }
            }
            $resourceMatch = [Regex]::Match(
                [string]$line,
                'ResourceDB:\s+loading cache\s+\([^\r\n]*?\bpath=(?<path>.+)\)\s*$')
            if ($resourceMatch.Success) {
                $observedResource = $resourceMatch.Groups['path'].Value.Replace('\', '/')
                $isExpected = $observedResource.Equals(
                    $expectedResourceDatabase,
                    [StringComparison]::OrdinalIgnoreCase)
                $isCheckoutResource = $observedResource.StartsWith(
                    $checkoutRoot + '/',
                    [StringComparison]::OrdinalIgnoreCase) -and
                    [IO.Path]::GetFileName($observedResource).Equals(
                        'resourceDatabase.rdb',
                        [StringComparison]::OrdinalIgnoreCase)
                if ($isExpected -or $isCheckoutResource) {
                    [void]$resourceRows.Add([pscustomobject][ordered]@{
                        syntaxExact = $true
                        expectedPath = $isExpected
                        checkoutResource = $isCheckoutResource
                    })
                }
            }
            elseif ([string]$line -match 'ResourceDB:\s+loading cache' -and
                (([string]$line).Replace('\', '/').IndexOf(
                        $expectedResourceDatabase,
                        [StringComparison]::OrdinalIgnoreCase) -ge 0 -or
                    ([string]$line).Replace('\', '/').IndexOf(
                        $checkoutRoot + '/',
                        [StringComparison]::OrdinalIgnoreCase) -ge 0)) {
                [void]$resourceRows.Add([pscustomobject][ordered]@{
                    syntaxExact = $false
                    expectedPath = $false
                    checkoutResource = $true
                })
            }
        }
    }
    $valid = $consoleLogs.Count -eq 1 -and $projectRows.Count -ge 1 -and
        $resourceRows.Count -eq 1
    foreach ($row in $projectRows) {
        if (-not [bool]$row.syntaxExact -or -not [bool]$row.projectMatches -or
            -not [bool]$row.guidMatches -or
            [bool]$row.generatedArchive) {
            $valid = $false
        }
    }
    foreach ($row in $resourceRows) {
        if (-not [bool]$row.syntaxExact -or -not [bool]$row.expectedPath -or
            -not [bool]$row.checkoutResource) {
            $valid = $false
        }
    }
    return [pscustomobject][ordered]@{
        valid = $valid
        consoleLogCount = $consoleLogs.Count
        sourceProjectRelevantRecordCount = $projectRows.Count
        expectedGuid = $ExpectedAddonGuid
        everyProjectRecordExact = @($projectRows | Where-Object {
            -not [bool]$_.syntaxExact -or -not [bool]$_.projectMatches -or
                -not [bool]$_.guidMatches
        }).Count -eq 0
        generatedArchiveMatchCount = @($projectRows | Where-Object {
            [bool]$_.generatedArchive
        }).Count
        sourceResourceDatabaseRecordCount = $resourceRows.Count
        everyResourceDatabaseRecordExact = @($resourceRows | Where-Object {
            -not [bool]$_.syntaxExact -or -not [bool]$_.expectedPath -or
                -not [bool]$_.checkoutResource
        }).Count -eq 0
        sourceResourceDatabase = ConvertTo-PortableFileIdentity `
            -Identity $ExpectedResourceDatabaseIdentity
    }
}

function Get-AddonGuid {
    param([Parameter(Mandatory = $true)][string]$ResolvedProjectFile)

    $text = [IO.File]::ReadAllText($ResolvedProjectFile)
    $matches = @([Regex]::Matches(
        $text,
        '(?m)^\s*GUID\s+"(?<value>[0-9A-F]{16})"\s*$'))
    if ($matches.Count -ne 1) {
        throw 'The source project GUID could not be parsed exactly.'
    }
    return $matches[0].Groups['value'].Value
}

function Get-ExecutableIdentity {
    param([Parameter(Mandatory = $true)][string]$ExecutablePath)

    $file = Get-FileIdentity -Path $ExecutablePath
    $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($ExecutablePath)
    $signatureStatus = ''
    $signerThumbprint = ''
    $signerSubject = ''
    try {
        $signature = Get-AuthenticodeSignature -LiteralPath $ExecutablePath
        $signatureStatus = [string]$signature.Status
        if ($signature.SignerCertificate) {
            $signerThumbprint = [string]$signature.SignerCertificate.Thumbprint
            $signerSubject = [string]$signature.SignerCertificate.Subject
        }
    }
    catch {
        $signatureStatus = 'unavailable'
    }
    return [pscustomobject][ordered]@{
        path = $file.path
        bytes = $file.bytes
        lastWriteUtc = $file.lastWriteUtc
        sha256 = $file.sha256
        fileVersion = [string]$version.FileVersion
        productVersion = [string]$version.ProductVersion
        productName = [string]$version.ProductName
        companyName = [string]$version.CompanyName
        originalFilename = [string]$version.OriginalFilename
        signatureStatus = $signatureStatus
        signerThumbprint = $signerThumbprint
        signerSubject = $signerSubject
    }
}

function Test-TrustedDiagnosticExecutableIdentity {
    param([Parameter(Mandatory = $true)]$Identity)

    return [string]$Identity.signatureStatus -ceq 'Valid' -and
        [string]$Identity.signerThumbprint -cmatch '^[0-9A-F]{40}$' -and
        [string]$Identity.signerSubject -cmatch
            '(?:^|,\s*)O=BOHEMIA INTERACTIVE a[.]s[.](?:,|$)' -and
        [string]$Identity.companyName -ceq 'Bohemia Interactive Studio' -and
        [string]$Identity.productName -ceq 'Arma Reforger' -and
        [string]$Identity.originalFilename -ceq 'ArmaReforgerSteam' -and
        [string]$Identity.fileVersion -cmatch '^\d+(?:[.]\d+){3}$'
}

function Test-ExecutableIdentityEqual {
    param(
        [Parameter(Mandatory = $true)]$First,
        [Parameter(Mandatory = $true)]$Second
    )

    return [long]$First.bytes -eq [long]$Second.bytes -and
        [string]$First.sha256 -ceq [string]$Second.sha256 -and
        [string]$First.fileVersion -ceq [string]$Second.fileVersion -and
        [string]$First.productVersion -ceq [string]$Second.productVersion -and
        [string]$First.productName -ceq [string]$Second.productName -and
        [string]$First.companyName -ceq [string]$Second.companyName -and
        [string]$First.originalFilename -ceq [string]$Second.originalFilename -and
        [string]$First.signatureStatus -ceq [string]$Second.signatureStatus -and
        [string]$First.signerThumbprint -ceq [string]$Second.signerThumbprint -and
        [string]$First.signerSubject -ceq [string]$Second.signerSubject
}

function Get-EvidenceFileRows {
    param([Parameter(Mandatory = $true)][string]$RunRoot)

    $root = [IO.Path]::GetFullPath($RunRoot).TrimEnd('\', '/')
    $prefix = $root + [IO.Path]::DirectorySeparatorChar
    $sets = @(
        [pscustomobject]@{ category = 'identity'; path = Join-Path $root 'identity' },
        [pscustomobject]@{ category = 'config'; path = Join-Path $root 'config' },
        [pscustomobject]@{ category = 'logs'; path = Join-Path $root 'isolation\logs' },
        [pscustomobject]@{ category = 'debug'; path = Join-Path $root 'isolation\profile\Partisan\debug' }
    )
    $rows = New-Object Collections.Generic.List[object]
    foreach ($set in $sets) {
        if (-not (Test-Path -LiteralPath $set.path -PathType Container)) {
            continue
        }
        foreach ($file in @(Get-ChildItem `
                -LiteralPath $set.path `
                -Recurse `
                -File `
                -Force `
                -ErrorAction Stop |
                Sort-Object FullName)) {
            if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw 'Retained Campaign Debug evidence must not contain reparse points.'
            }
            $full = [IO.Path]::GetFullPath($file.FullName)
            if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
                throw 'A retained evidence file escaped its run root.'
            }
            [void]$rows.Add([pscustomobject][ordered]@{
                category = $set.category
                relativePath = $full.Substring($prefix.Length).Replace('\', '/')
                bytes = [long]$file.Length
                sha256 = (Get-FileHash `
                    -LiteralPath $full `
                    -Algorithm SHA256).Hash.ToLowerInvariant()
                lastWriteUtc = $file.LastWriteTimeUtc.ToString(
                    'o',
                    [Globalization.CultureInfo]::InvariantCulture)
            })
        }
    }
    $runtimeConfigPath = Join-Path `
        $root `
        'isolation\profile\Partisan\HST_Settings.json'
    if (Test-Path -LiteralPath $runtimeConfigPath -PathType Leaf) {
        $runtimeConfig = Get-Item -LiteralPath $runtimeConfigPath -Force
        if (($runtimeConfig.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'The retained runtime settings file must not be a reparse point.'
        }
        $full = [IO.Path]::GetFullPath($runtimeConfig.FullName)
        [void]$rows.Add([pscustomobject][ordered]@{
            category = 'runtime-config'
            relativePath = $full.Substring($prefix.Length).Replace('\', '/')
            bytes = [long]$runtimeConfig.Length
            sha256 = (Get-FileHash `
                -LiteralPath $full `
                -Algorithm SHA256).Hash.ToLowerInvariant()
            lastWriteUtc = $runtimeConfig.LastWriteTimeUtc.ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
        })
    }
    return $rows.ToArray()
}

function Get-ArtifactSetIdentity {
    param([Parameter(Mandatory = $true)][object[]]$FileRows)

    $rowByPath = New-Object `
        'Collections.Generic.Dictionary[string,object]' `
        ([StringComparer]::Ordinal)
    foreach ($row in $FileRows) {
        $path = [string]$row.relativePath
        if ([string]::IsNullOrWhiteSpace($path) -or $rowByPath.ContainsKey($path)) {
            throw 'The evidence file set contains a blank or duplicate path.'
        }
        $rowByPath.Add($path, $row)
    }
    [string[]]$paths = @($rowByPath.Keys)
    [Array]::Sort($paths, [StringComparer]::Ordinal)
    $portableRows = @($paths | ForEach-Object {
        $row = $rowByPath[$_]
        [pscustomobject][ordered]@{
            path = [string]$row.relativePath
            length = [long]$row.bytes
            sha256 = [string]$row.sha256
        }
    })
    $canonicalLines = @($portableRows | ForEach-Object {
        ([string]$_.path) + "`t" + ([string]$_.length) + "`t" + ([string]$_.sha256)
    })
    $canonicalText = if ($canonicalLines.Count -gt 0) {
        ($canonicalLines -join "`n") + "`n"
    }
    else {
        ''
    }
    return [pscustomobject][ordered]@{
        hashAlgorithm = 'sha256-file-set-v1'
        artifactCount = $portableRows.Count
        artifactSetSha256 = Get-TextSha256 -Text $canonicalText
        files = $portableRows
    }
}

function Get-ScalarProperty {
    param(
        [AllowNull()]$Object,
        [Parameter(Mandatory = $true)][string]$Name,
        $Default = $null
    )

    if ($null -eq $Object) {
        return $Default
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $Default
    }
    return $property.Value
}

function Test-ExactStringSet {
    param(
        [AllowEmptyCollection()][Parameter(Mandatory = $true)][string[]]$Expected,
        [AllowEmptyCollection()][Parameter(Mandatory = $true)][string[]]$Actual
    )

    if ($Expected.Count -ne $Actual.Count -or
        @($Actual | Group-Object -CaseSensitive | Where-Object Count -ne 1).Count -ne 0) {
        return $false
    }
    $expectedSet = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    foreach ($value in $Expected) {
        [void]$expectedSet.Add($value)
    }
    foreach ($value in $Actual) {
        if (-not $expectedSet.Contains($value)) {
            return $false
        }
    }
    return $true
}

function Get-SourceCampaignDebugAcceptance {
    param(
        [Parameter(Mandatory = $true)][string]$JsonPath,
        [Parameter(Mandatory = $true)][ValidateSet('full_certification', 'force_authority')][string]$Profile,
        [Parameter(Mandatory = $true)]$ArtifactValidation,
        [Parameter(Mandatory = $true)]$ErrorCensus,
        [Parameter(Mandatory = $true)][int]$ClassifierChecks,
        [Parameter(Mandatory = $true)][bool]$CaptureAxesPassed
    )

    $redAxes = New-Object Collections.Generic.List[string]
    try {
        $raw = (Get-SharedFileText -Path $JsonPath) | ConvertFrom-Json
    }
    catch {
        return [pscustomobject][ordered]@{
            accepted = $false
            disposition = 'rejected'
            correctedCanaryAccepted = $false
            proofCommonPassed = $false
            acceptedFull = $false
            acceptedInternal = $false
            diagnosticAxisPassed = $false
            captureAxesPassed = $CaptureAxesPassed
            redAxes = @('raw-artifact-json')
        }
    }

    $caseCounts = [ordered]@{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0; SKIPPED = 0 }
    $certCounts = [ordered]@{ PASS = 0; WARN = 0; FAIL = 0; BLOCKED = 0 }
    $warningRecords = New-Object Collections.Generic.List[object]
    $blockedRecords = New-Object Collections.Generic.List[object]
    $failedAssertionIds = New-Object Collections.Generic.List[string]
    $skippedAssertionIds = New-Object Collections.Generic.List[string]
    $assertionCount = 0
    $seenCases = New-Object `
        'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $cases = @($raw.m_aCases)
    foreach ($case in $cases) {
        $caseId = [string](Get-ScalarProperty -Object $case -Name 'm_sCaseId' -Default '')
        $caseStatus = [string](Get-ScalarProperty -Object $case -Name 'm_sStatus' -Default '')
        if ([string]::IsNullOrWhiteSpace($caseId) -or -not $seenCases.Add($caseId)) {
            [void]$redAxes.Add('raw-case-identity')
        }
        if (-not $caseCounts.Contains($caseStatus)) {
            [void]$redAxes.Add('raw-case-status')
            continue
        }
        $caseCounts[$caseStatus]++
        $derivedStatus = 'PASS'
        $derivedSeverity = 0
        $seenAssertions = New-Object `
            'Collections.Generic.HashSet[string]' `
            ([StringComparer]::Ordinal)
        foreach ($assertion in @($case.m_aAssertions)) {
            $assertionCount++
            $assertionId = [string](Get-ScalarProperty `
                -Object $assertion `
                -Name 'm_sAssertionId' `
                -Default '')
            $status = [string](Get-ScalarProperty `
                -Object $assertion `
                -Name 'm_sStatus' `
                -Default '')
            if ([string]::IsNullOrWhiteSpace($assertionId) -or
                -not $seenAssertions.Add($assertionId)) {
                [void]$redAxes.Add('raw-assertion-identity')
            }
            $severity = switch ($status) {
                'SKIPPED' { 1 }
                'WARN' { 2 }
                'BLOCKED' { 3 }
                'FAIL' { 4 }
                'PASS' { 0 }
                default {
                    [void]$redAxes.Add('raw-assertion-status')
                    5
                }
            }
            if ($severity -gt $derivedSeverity) {
                $derivedSeverity = $severity
                $derivedStatus = $status
            }
            $certificationProperty = $assertion.PSObject.Properties[
                'm_bCountsTowardCertification']
            $certifying = $false
            if ($null -eq $certificationProperty -or
                -not (Test-NativeJsonBoolean -Value $certificationProperty.Value)) {
                [void]$redAxes.Add('certification-flag-type')
            }
            else {
                $certifying = [bool]$certificationProperty.Value
            }
            if ($certifying) {
                if ($certCounts.Contains($status)) {
                    $certCounts[$status]++
                }
                else {
                    [void]$redAxes.Add('certification-status')
                }
            }
            $record = [pscustomobject][ordered]@{
                id = $assertionId
                caseId = $caseId
                category = [string](Get-ScalarProperty -Object $case -Name 'm_sCategory' -Default '')
                feature = [string](Get-ScalarProperty -Object $case -Name 'm_sFeature' -Default '')
                stage = [string](Get-ScalarProperty -Object $case -Name 'm_sStage' -Default '')
                expected = [string](Get-ScalarProperty -Object $assertion -Name 'm_sExpected' -Default '')
                actual = [string](Get-ScalarProperty -Object $assertion -Name 'm_sActual' -Default '')
                reason = [string](Get-ScalarProperty -Object $assertion -Name 'm_sFailureReason' -Default '')
                proofLevel = [string](Get-ScalarProperty -Object $assertion -Name 'm_sProofLevel' -Default '')
                observedPath = [string](Get-ScalarProperty -Object $assertion -Name 'm_sObservedPath' -Default '')
                requiredPath = [string](Get-ScalarProperty -Object $assertion -Name 'm_sRequiredPath' -Default '')
                certifying = $certifying
            }
            switch ($status) {
                'FAIL' { [void]$failedAssertionIds.Add($assertionId) }
                'BLOCKED' { [void]$blockedRecords.Add($record) }
                'WARN' { [void]$warningRecords.Add($record) }
                'SKIPPED' { [void]$skippedAssertionIds.Add($assertionId) }
            }
        }
        if ($caseStatus -cne $derivedStatus) {
            [void]$redAxes.Add('raw-case-derived-status')
        }
    }

    $certRequired = $certCounts.PASS + $certCounts.WARN +
        $certCounts.FAIL + $certCounts.BLOCKED
    $headerBindings = @(
        @('m_iPassCount', $caseCounts.PASS, 'raw-pass-count'),
        @('m_iWarnCount', $caseCounts.WARN, 'raw-warn-count'),
        @('m_iFailCount', $caseCounts.FAIL, 'raw-fail-count'),
        @('m_iBlockedCount', $caseCounts.BLOCKED, 'raw-blocked-count'),
        @('m_iSkippedCount', $caseCounts.SKIPPED, 'raw-skipped-count'),
        @('m_iCertificationRequiredCount', $certRequired, 'cert-required-count'),
        @('m_iCertificationProvenCount', $certCounts.PASS, 'cert-proven-count'),
        @('m_iCertificationFailCount', $certCounts.FAIL, 'cert-fail-count'),
        @('m_iCertificationBlockedCount', $certCounts.BLOCKED, 'cert-blocked-count'),
        @('m_iCertificationWarnCount', $certCounts.WARN, 'cert-warn-count')
    )
    foreach ($binding in $headerBindings) {
        $property = $raw.PSObject.Properties[[string]$binding[0]]
        if ($null -eq $property -or
            -not (Test-NativeJsonInteger -Value $property.Value)) {
            [void]$redAxes.Add([string]$binding[2] + '-type')
        }
        elseif ([long]$property.Value -ne [long]$binding[1]) {
            [void]$redAxes.Add([string]$binding[2])
        }
    }
    $certificationPassedProperty = $raw.PSObject.Properties[
        'm_bCertificationPassed']
    $rawCertificationPassed = $false
    if ($null -eq $certificationPassedProperty -or
        -not (Test-NativeJsonBoolean -Value $certificationPassedProperty.Value)) {
        [void]$redAxes.Add('certification-boolean-type')
    }
    else {
        $rawCertificationPassed = [bool]$certificationPassedProperty.Value
    }
    $derivedCertificationPassed = if ($Profile -ceq 'force_authority') {
        $false
    }
    else {
        $certRequired -eq $certCounts.PASS -and
            $certCounts.FAIL -eq 0 -and
            $certCounts.BLOCKED -eq 0 -and
            $certCounts.WARN -eq 0
    }
    if ($rawCertificationPassed -ne $derivedCertificationPassed) {
        [void]$redAxes.Add('certification-boolean')
    }
    $rawContractPassed = $redAxes.Count -eq 0

    $diagnosticIntegers = [ordered]@{}
    foreach ($field in @(
            'HardDiagnosticCount', 'ScriptErrors', 'EngineErrors', 'PartisanErrors',
            'CrashMarkers', 'PartisanSeverityLineCount', 'ApprovedStockDiagnosticCount',
            'ApprovedShutdownCatalogDiagnosticCount',
            'ApprovedIntentionalDiagnosticCount', 'MalformedHardDiagnosticCount',
            'UnapprovedHardDiagnosticCount', 'CanonicalScriptLogCount',
            'CanonicalConsoleLogCount', 'CanonicalErrorLogCount',
            'CanonicalCrashLogCount', 'AuxiliaryUnapprovedEventCount')) {
        $diagnosticIntegers[$field] = [int](Get-ScalarProperty `
            -Object $ErrorCensus `
            -Name $field `
            -Default -1)
    }
    $diagnosticBooleans = [ordered]@{}
    foreach ($field in @(
            'Valid', 'HardDiagnosticFree', 'ChannelArithmeticValid',
            'CategoryArithmeticValid', 'LifecycleMarkersValid',
            'IdentityBaselinePairValid', 'ShutdownCatalogPairValid',
            'IntentionalFixtureStructureExact',
            'IntentionalFixtureSetValid', 'CanonicalLogPairSameDirectory',
            'AuxiliaryLogPairSameDirectory', 'AuxiliaryDiagnosticsValid',
            'ErrorLogProjectionExact', 'CrashLogProjectionExact',
            'IntentionalMissionConvoyAdmissionDiagnosticsProven',
            'IntentionalMissionConvoySettlementDiagnosticProven',
            'IntentionalMissionConvoyCorruptionDiagnosticsProven',
            'IntentionalMissionConvoyWatchdogDiagnosticProven')) {
        $diagnosticBooleans[$field] = [bool](Get-ScalarProperty `
            -Object $ErrorCensus `
            -Name $field `
            -Default $false)
    }
    $unapprovedKindTotal = 0
    foreach ($kind in @((Get-ScalarProperty `
                -Object $ErrorCensus `
                -Name 'UnapprovedHardDiagnosticKinds' `
                -Default @()))) {
        $unapprovedKindTotal += [int](Get-ScalarProperty `
            -Object $kind `
            -Name 'count' `
            -Default 0)
    }
    $diagnosticCommon = $diagnosticBooleans.Valid -and
        $diagnosticBooleans.ChannelArithmeticValid -and
        $diagnosticBooleans.CategoryArithmeticValid -and
        $diagnosticIntegers.HardDiagnosticCount -eq
            ($diagnosticIntegers.ScriptErrors + $diagnosticIntegers.EngineErrors) -and
        $diagnosticIntegers.HardDiagnosticCount -eq
            ($diagnosticIntegers.ApprovedStockDiagnosticCount +
                $diagnosticIntegers.ApprovedIntentionalDiagnosticCount +
                $diagnosticIntegers.UnapprovedHardDiagnosticCount) -and
        $unapprovedKindTotal -eq $diagnosticIntegers.UnapprovedHardDiagnosticCount -and
        $diagnosticBooleans.HardDiagnosticFree -eq
            ($diagnosticIntegers.HardDiagnosticCount -eq 0) -and
        $diagnosticBooleans.LifecycleMarkersValid -and
        $diagnosticBooleans.IdentityBaselinePairValid -and
        $diagnosticBooleans.ShutdownCatalogPairValid -and
        $diagnosticIntegers.ApprovedShutdownCatalogDiagnosticCount -in @(0, 2) -and
        $diagnosticBooleans.IntentionalFixtureStructureExact -and
        $diagnosticBooleans.IntentionalFixtureSetValid -and
        $diagnosticBooleans.CanonicalLogPairSameDirectory -and
        $diagnosticBooleans.AuxiliaryLogPairSameDirectory -and
        $diagnosticBooleans.AuxiliaryDiagnosticsValid -and
        $diagnosticBooleans.ErrorLogProjectionExact -and
        $diagnosticBooleans.CrashLogProjectionExact -and
        $diagnosticIntegers.CrashMarkers -eq 0 -and
        $diagnosticIntegers.PartisanSeverityLineCount -eq 0 -and
        $diagnosticIntegers.MalformedHardDiagnosticCount -eq 0 -and
        $diagnosticIntegers.CanonicalScriptLogCount -eq 1 -and
        $diagnosticIntegers.CanonicalConsoleLogCount -eq 1 -and
        $diagnosticIntegers.CanonicalErrorLogCount -eq 1 -and
        $diagnosticIntegers.CanonicalCrashLogCount -eq 1 -and
        $diagnosticIntegers.AuxiliaryUnapprovedEventCount -eq 0 -and
        $diagnosticIntegers.UnapprovedHardDiagnosticCount -eq 0 -and
        $ClassifierChecks -eq 55
    if (-not $diagnosticCommon) {
        [void]$redAxes.Add('diagnostic-common')
    }
    $intentionalValidationFlags = @(
        [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'IntentionalMissionConvoyAdmissionDiagnosticsProven' -Default $false),
        [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'IntentionalMissionConvoySettlementDiagnosticProven' -Default $false),
        [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'IntentionalMissionConvoyCorruptionDiagnosticsProven' -Default $false),
        [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'IntentionalMissionConvoyWatchdogDiagnosticProven' -Default $false))
    $intentionalCensusFlags = @(
        $diagnosticBooleans.IntentionalMissionConvoyAdmissionDiagnosticsProven,
        $diagnosticBooleans.IntentionalMissionConvoySettlementDiagnosticProven,
        $diagnosticBooleans.IntentionalMissionConvoyCorruptionDiagnosticsProven,
        $diagnosticBooleans.IntentionalMissionConvoyWatchdogDiagnosticProven)
    $diagnosticProfileExact = $false
    $shutdownCatalogCount =
        $diagnosticIntegers.ApprovedShutdownCatalogDiagnosticCount
    $expectedStockDiagnosticCount = 2 + $shutdownCatalogCount
    if ($Profile -ceq 'force_authority') {
        $diagnosticProfileExact = $diagnosticIntegers.HardDiagnosticCount -eq
                (2 + $shutdownCatalogCount) -and
            $diagnosticIntegers.ScriptErrors -eq (2 + $shutdownCatalogCount) -and
            $diagnosticIntegers.EngineErrors -eq 0 -and
            $diagnosticIntegers.PartisanErrors -eq 0 -and
            $diagnosticIntegers.ApprovedStockDiagnosticCount -eq
                $expectedStockDiagnosticCount -and
            $diagnosticIntegers.ApprovedIntentionalDiagnosticCount -eq 0 -and
            @($intentionalCensusFlags | Where-Object { $_ }).Count -eq 0 -and
            @($intentionalValidationFlags | Where-Object { $_ }).Count -eq 0
    }
    else {
        $diagnosticProfileExact = $diagnosticIntegers.HardDiagnosticCount -eq
                (15 + $shutdownCatalogCount) -and
            $diagnosticIntegers.ScriptErrors -eq (15 + $shutdownCatalogCount) -and
            $diagnosticIntegers.EngineErrors -eq 0 -and
            $diagnosticIntegers.PartisanErrors -eq 13 -and
            $diagnosticIntegers.ApprovedStockDiagnosticCount -eq
                $expectedStockDiagnosticCount -and
            $diagnosticIntegers.ApprovedIntentionalDiagnosticCount -eq 13 -and
            @($intentionalCensusFlags | Where-Object { -not $_ }).Count -eq 0 -and
            @($intentionalValidationFlags | Where-Object { -not $_ }).Count -eq 0
    }
    $diagnosticAxisPassed = $diagnosticCommon -and $diagnosticProfileExact
    if (-not $diagnosticProfileExact) {
        [void]$redAxes.Add('diagnostic-profile')
    }

    $artifactValid = [bool](Get-ScalarProperty `
        -Object $ArtifactValidation `
        -Name 'Valid' `
        -Default $false)
    $sourceArtifactContract = [bool](Get-ScalarProperty `
        -Object $ArtifactValidation `
        -Name 'SourceArtifactContract' `
        -Default $false)
    $stateDiffExact = $sourceArtifactContract -and [bool](Get-ScalarProperty `
        -Object $ArtifactValidation `
        -Name 'StateDiffManifestExact' `
        -Default $false) -and
        [int](Get-ScalarProperty -Object $ArtifactValidation -Name 'StateDiffRows' -Default -1) -eq 18 -and
        [int](Get-ScalarProperty -Object $ArtifactValidation -Name 'NonzeroStateDiffRows' -Default -1) -eq 0
    $finalOrphanProperty = $ArtifactValidation.PSObject.Properties[
        'FinalOrphanActiveGroups']
    $cleanupExact = [bool](Get-ScalarProperty `
        -Object $ArtifactValidation `
        -Name 'FinalOrphanCleanupPass' `
        -Default $false) -and
        $null -ne $finalOrphanProperty -and
        (Test-NativeJsonInteger -Value $finalOrphanProperty.Value) -and
        [long]$finalOrphanProperty.Value -eq 0
    if (-not $artifactValid) { [void]$redAxes.Add('artifact-validation') }
    if (-not $stateDiffExact) { [void]$redAxes.Add('state-diff') }
    if (-not $cleanupExact) { [void]$redAxes.Add('final-cleanup') }
    if (-not $CaptureAxesPassed) { [void]$redAxes.Add('capture-boundary') }

    $acceptedCorrectedCanary = $false
    $proofCommonPassed = $false
    $acceptedFull = $false
    $acceptedInternal = $false
    $unsupportedWarningIds = @()
    $unsupportedSkippedIds = @()
    $externalAdvisoryIds = @()
    if ($Profile -ceq 'force_authority') {
        $warningIds = @($warningRecords | ForEach-Object { [string]$_.id })
        $blockedIds = @($blockedRecords | ForEach-Object { [string]$_.id })
        $focusedAssertions = @((Get-ScalarProperty `
                -Object $ArtifactValidation `
                -Name 'FocusedAssertions' `
                -Default @()))
        $canaryProof = $rawContractPassed -and $artifactValid -and
            $sourceArtifactContract -and
            [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'SourceCanaryContract' -Default $false) -and
            [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'SourceCanaryCaseSetExact' -Default $false) -and
            [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'SourceCanaryAssertionManifestExact' -Default $false) -and
            [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'SourceCanaryWarningContractExact' -Default $false) -and
            [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'SourceCanaryNoBlockedAssertions' -Default $false) -and
            [bool](Get-ScalarProperty -Object $ArtifactValidation -Name 'SourceCanaryOrphanContractExact' -Default $false) -and
            $stateDiffExact -and $cleanupExact -and
            $cases.Count -eq 11 -and $caseCounts.PASS -eq 9 -and
            $caseCounts.WARN -eq 2 -and $caseCounts.FAIL -eq 0 -and
            $caseCounts.BLOCKED -eq 0 -and $caseCounts.SKIPPED -eq 0 -and
            $assertionCount -eq 91 -and
            (Test-ExactStringSet -Expected @(
                    'cleanup.player_marker.live',
                    'isolation.world_scope') -Actual $warningIds) -and
            $blockedIds.Count -eq 0 -and
            $skippedAssertionIds.Count -eq 0 -and
            [string](Get-ScalarProperty -Object $ArtifactValidation -Name 'FocusedCaseStatus' -Default '') -ceq 'PASS' -and
            $focusedAssertions.Count -eq 35 -and
            @($focusedAssertions | Where-Object {
                -not [bool](Get-ScalarProperty -Object $_ -Name 'Pass' -Default $false) -or
                [string](Get-ScalarProperty -Object $_ -Name 'Status' -Default '') -cne 'PASS'
            }).Count -eq 0 -and
            $certRequired -eq 87 -and $certCounts.PASS -eq 87 -and
            $certCounts.FAIL -eq 0 -and $certCounts.BLOCKED -eq 0 -and
            $certCounts.WARN -eq 0 -and -not $rawCertificationPassed
        if (-not $canaryProof) {
            [void]$redAxes.Add('source-canary-proof')
        }
        $acceptedCorrectedCanary = $canaryProof -and
            $diagnosticAxisPassed -and $CaptureAxesPassed
    }
    else {
        $externalContracts = [ordered]@{
            'isolation.world_scope' = [ordered]@{
                caseId = 'cleanup.state_isolation_restore'; category = 'cleanup'
                feature = 'campaign_debug'; stage = 'state_restore'
                expected = 'runtime certification remains scoped to the disposable development session'
                actual = 'world runtime, player inventory, health, and service caches require session restart before another certifying run'
                reason = 'restart the disposable development session before another certification run'
            }
            'persistence.real_restart' = [ordered]@{
                caseId = 'persistence.seeded_roundtrip.phase12'; category = 'persistence'
                feature = 'persistence_smoke'; stage = 'early_phase'
                expected = 'external process restart / reconnect remains an explicit later-gate scenario'
                actual = 'non-certifying external advisory | restart/fault gate'
                reason = 'run the Workshop-published addon through the external restart matrix before claiming restart certification'
            }
            'phase25.real_restart' = [ordered]@{
                caseId = 'phase25.manual_external_gaps'; category = 'soak'
                feature = 'external_harness'; stage = 'final'
                expected = 'real restart-after-primitive remains an explicit later-gate external scenario'
                actual = 'non-certifying external advisory | restart/fault gate'
                reason = 'run the Workshop-published addon through the external restart matrix before claiming restart certification'
            }
            'phase25.second_client' = [ordered]@{
                caseId = 'phase25.manual_external_gaps'; category = 'soak'
                feature = 'external_harness'; stage = 'final'
                expected = 'second-client join/reconnect remains an explicit later-gate external scenario'
                actual = 'non-certifying external advisory | multiplayer/JIP gate'
                reason = 'run the Workshop-published addon with the required clients before claiming multiplayer certification'
            }
            'phase25.two_hour_soak' = [ordered]@{
                caseId = 'phase25.manual_external_gaps'; category = 'soak'
                feature = 'external_harness'; stage = 'final'
                expected = 'two-hour endurance remains an explicit later-gate external scenario'
                actual = 'non-certifying external advisory | soak gate'
                reason = 'run the Workshop-published addon for the required duration before claiming soak certification'
            }
        }
        $externalIds = @($externalContracts.Keys)
        $approvedSkips = @(
            'phase24.escalation.support_physicalization',
            'phase24.escalation.group_physicalization')
        $warningIds = @($warningRecords | ForEach-Object { [string]$_.id })
        $externalAdvisoryIds = @($warningIds | Where-Object {
            $externalIds -ccontains $_
        })
        $unsupportedWarningIds = @($warningIds | Where-Object {
            $externalIds -cnotcontains $_
        })
        $unsupportedSkippedIds = @($skippedAssertionIds | Where-Object {
            $approvedSkips -cnotcontains $_
        })
        if (@($skippedAssertionIds | Group-Object -CaseSensitive |
                Where-Object Count -ne 1).Count -ne 0) {
            $unsupportedSkippedIds += '<duplicate-skip-id>'
        }
        $externalLinkageValid = $true
        foreach ($record in @($warningRecords | Where-Object {
                    $externalIds -ccontains [string]$_.id
                })) {
            $contract = $externalContracts[[string]$record.id]
            if ([string]$record.caseId -cne [string]$contract.caseId -or
                [string]$record.category -cne [string]$contract.category -or
                [string]$record.feature -cne [string]$contract.feature -or
                [string]$record.stage -cne [string]$contract.stage -or
                [string]$record.expected -cne [string]$contract.expected -or
                [string]$record.actual -cne [string]$contract.actual -or
                [string]$record.reason -cne [string]$contract.reason -or
                [string]$record.proofLevel -cne 'EXTERNAL_PROCESS' -or
                [string]$record.observedPath -cne 'manual_external_gap' -or
                [string]$record.requiredPath -cne
                    'external process restart, reconnect, or long-soak harness' -or
                [bool]$record.certifying) {
                $externalLinkageValid = $false
            }
        }
        $proofCommonPassed = $rawContractPassed -and
            $caseCounts.FAIL -eq 0 -and
            $caseCounts.BLOCKED -eq 0 -and
            $failedAssertionIds.Count -eq 0 -and
            $blockedRecords.Count -eq 0 -and
            $unsupportedWarningIds.Count -eq 0 -and
            $unsupportedSkippedIds.Count -eq 0 -and
            $artifactValid -and $rawCertificationPassed -and
            $certCounts.FAIL -eq 0 -and $certCounts.BLOCKED -eq 0 -and
            $certCounts.WARN -eq 0 -and $stateDiffExact -and $cleanupExact
        if (-not $proofCommonPassed) {
            [void]$redAxes.Add('full-proof-common')
        }
        $acceptedFull = $proofCommonPassed -and
            $warningRecords.Count -eq 0 -and $caseCounts.WARN -eq 0 -and
            $diagnosticAxisPassed -and $CaptureAxesPassed
        $externalSetExact = Test-ExactStringSet `
            -Expected $externalIds `
            -Actual $externalAdvisoryIds
        $acceptedInternal = -not $acceptedFull -and $proofCommonPassed -and
            $externalSetExact -and $externalLinkageValid -and
            $caseCounts.WARN -gt 0 -and $diagnosticAxisPassed -and
            $CaptureAxesPassed
        if (-not $acceptedFull -and -not $acceptedInternal) {
            if ($unsupportedWarningIds.Count -ne 0) {
                [void]$redAxes.Add('unsupported-warning')
            }
            if ($unsupportedSkippedIds.Count -ne 0) {
                [void]$redAxes.Add('unsupported-skip')
            }
            if (-not $externalSetExact -and $warningRecords.Count -ne 0) {
                [void]$redAxes.Add('external-advisory-set')
            }
            if (-not $externalLinkageValid) {
                [void]$redAxes.Add('external-advisory-linkage')
            }
        }
    }
    $accepted = $acceptedCorrectedCanary -or $acceptedFull -or $acceptedInternal
    if (-not $accepted) {
        [void]$redAxes.Add('acceptance-disposition')
    }
    $disposition = if ($acceptedCorrectedCanary) {
        'accepted-corrected-canary'
    }
    elseif ($acceptedFull) {
        'accepted-full'
    }
    elseif ($acceptedInternal) {
        'accepted-internal'
    }
    else {
        'rejected'
    }
    return [pscustomobject][ordered]@{
        accepted = $accepted
        disposition = $disposition
        correctedCanaryAccepted = $acceptedCorrectedCanary
        proofCommonPassed = $proofCommonPassed
        acceptedFull = $acceptedFull
        acceptedInternal = $acceptedInternal
        diagnosticAxisPassed = $diagnosticAxisPassed
        captureAxesPassed = $CaptureAxesPassed
        rawCaseCounts = [pscustomobject][ordered]@{
            caseCount = $cases.Count
            pass = $caseCounts.PASS
            warn = $caseCounts.WARN
            fail = $caseCounts.FAIL
            blocked = $caseCounts.BLOCKED
            skipped = $caseCounts.SKIPPED
        }
        rawAssertionCount = $assertionCount
        certificationCounts = [pscustomobject][ordered]@{
            required = $certRequired
            proven = $certCounts.PASS
            fail = $certCounts.FAIL
            blocked = $certCounts.BLOCKED
            warn = $certCounts.WARN
            passed = $rawCertificationPassed
        }
        externalAdvisoryIds = $externalAdvisoryIds
        unsupportedWarningIds = $unsupportedWarningIds
        skippedAssertionIds = $skippedAssertionIds.ToArray()
        unsupportedSkippedIds = $unsupportedSkippedIds
        redAxes = @($redAxes | Sort-Object -Unique)
    }
}

function Invoke-SourceRunnerSelfTest {
    param(
        [Parameter(Mandatory = $true)][string]$RunnerPath,
        [Parameter(Mandatory = $true)][string]$ClassifierPath,
        [Parameter(Mandatory = $true)][string]$CheckoutRoot
    )

    $tokens = $null
    $parseErrors = $null
    [void][Management.Automation.Language.Parser]::ParseFile(
        $RunnerPath,
        [ref]$tokens,
        [ref]$parseErrors)
    if (@($parseErrors).Count -ne 0) {
        throw 'The source Campaign Debug runner parser self-test failed.'
    }
    $runnerText = [IO.File]::ReadAllText($RunnerPath)
    $forbiddenTokens = @(
        ('data' + [char]46 + ('p' + 'ak')),
        ('-hstRelease' + 'PackageSha256'),
        ('-hstRelease' + 'CandidateId'),
        ('Candidate' + 'Manifest')
    )
    foreach ($token in $forbiddenTokens) {
        if ($runnerText.IndexOf($token, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
            throw 'The source Campaign Debug runner contains a forbidden distribution dependency.'
        }
    }

    $emptyProcessCensus = (New-SourceCampaignProcessCensus `
            -BeforeProcesses @() `
            -RootProcessId 0 `
            -RootStartUtc ([DateTime]::MinValue) `
            -MaximumOwnedProcesses 0 `
            -OwnedProcessesRemaining 0 `
            -UnclaimedEngineProcessesObserved @() `
            -AfterProcesses @() `
            -CleanupErrors @() |
        ConvertTo-Json -Compress |
        ConvertFrom-Json)
    if ($emptyProcessCensus.before -isnot [Array] -or
        $emptyProcessCensus.unclaimedEngineProcessesObserved -isnot [Array] -or
        $emptyProcessCensus.after -isnot [Array] -or
        $emptyProcessCensus.cleanupErrors -isnot [Array] -or
        @($emptyProcessCensus.before).Count -ne 0 -or
        @($emptyProcessCensus.unclaimedEngineProcessesObserved).Count -ne 0 -or
        @($emptyProcessCensus.after).Count -ne 0 -or
        @($emptyProcessCensus.cleanupErrors).Count -ne 0) {
        throw 'The empty process-census JSON array self-test failed.'
    }

    $library = Import-CampaignDebugClassifierLibrary `
        -ClassifierPath $ClassifierPath `
        -IncludeSelfTests
    $classifierChecks = Test-CampaignDebugHardDiagnosticCensus
    if ([int]$classifierChecks -ne 55) {
        throw 'The Campaign Debug diagnostic-classifier self-test count drifted.'
    }
    $tempRoot = Join-Path `
        ([IO.Path]::GetTempPath()) `
        ('PartisanSourceCampaignDebugSelfTest_' + [Guid]::NewGuid().ToString('N'))
    try {
        $artifactResult = Invoke-ArtifactValidatorSelfTest `
            -Directory $tempRoot `
            -ExpectedSha ('a' * 40) `
            -ExpectedUtc '2026-01-01T00:00:00Z' `
            -ExpectedLabel 'source-runner-self-test'
        if (-not $artifactResult.Valid -or
            @($artifactResult.NegativeStagedContractChecks).Count -eq 0 -or
            @($artifactResult.FocusedValidatorChecks).Count -eq 0 -or
            @($artifactResult.SourceValidatorChecks).Count -ne 5 -or
            -not $artifactResult.SourceCanaryValidation.Valid) {
            throw 'The Campaign Debug artifact-classifier self-test failed.'
        }

        $newAssertion = {
            param(
                [string]$Id,
                [string]$Status = 'PASS',
                [bool]$Certifying = $true,
                [string]$ProofLevel = 'STATE_ONLY',
                [string]$ObservedPath = 'runtime_state',
                [string]$RequiredPath = 'runtime_state',
                [string]$Expected = '',
                [string]$Actual = '',
                [string]$Reason = ''
            )
            [pscustomobject][ordered]@{
                m_sAssertionId = $Id
                m_sStatus = $Status
                m_sExpected = $Expected
                m_sActual = $Actual
                m_sFailureReason = $Reason
                m_sProofLevel = $ProofLevel
                m_sObservedPath = $ObservedPath
                m_sRequiredPath = $RequiredPath
                m_bCountsTowardCertification = $Certifying
            }
        }
        $newCase = {
            param(
                [string]$Id,
                [string]$Status,
                [object[]]$Assertions,
                [string]$Category = 'self_test',
                [string]$Feature = 'self_test',
                [string]$Stage = 'self_test'
            )
            [pscustomobject][ordered]@{
                m_sCaseId = $Id
                m_sStatus = $Status
                m_sCategory = $Category
                m_sFeature = $Feature
                m_sStage = $Stage
                m_aAssertions = @($Assertions)
            }
        }
        $newRaw = {
            param(
                [string]$Profile,
                [object[]]$Cases,
                [bool]$CertificationPassed
            )
            $allAssertions = @($Cases | ForEach-Object { @($_.m_aAssertions) })
            $certifying = @($allAssertions | Where-Object {
                [bool]$_.m_bCountsTowardCertification
            })
            [pscustomobject][ordered]@{
                m_sRunId = 'seed1_t1_p1_u1'
                m_sProfile = $Profile
                m_aCases = @($Cases)
                m_iPassCount = @($Cases | Where-Object m_sStatus -ceq 'PASS').Count
                m_iWarnCount = @($Cases | Where-Object m_sStatus -ceq 'WARN').Count
                m_iFailCount = @($Cases | Where-Object m_sStatus -ceq 'FAIL').Count
                m_iBlockedCount = @($Cases | Where-Object m_sStatus -ceq 'BLOCKED').Count
                m_iSkippedCount = @($Cases | Where-Object m_sStatus -ceq 'SKIPPED').Count
                m_iCertificationRequiredCount = $certifying.Count
                m_iCertificationProvenCount = @($certifying | Where-Object m_sStatus -ceq 'PASS').Count
                m_iCertificationFailCount = @($certifying | Where-Object m_sStatus -ceq 'FAIL').Count
                m_iCertificationBlockedCount = @($certifying | Where-Object m_sStatus -ceq 'BLOCKED').Count
                m_iCertificationWarnCount = @($certifying | Where-Object m_sStatus -ceq 'WARN').Count
                m_bCertificationPassed = $CertificationPassed
            }
        }
        $newValidation = {
            param([string]$Profile)
            $intentional = $Profile -ceq 'full_certification'
            $focusedRows = @()
            if (-not $intentional) {
                for ($index = 0; $index -lt 35; $index++) {
                    $focusedRows += [pscustomobject][ordered]@{
                        Id = "focused.$index"
                        Pass = $true
                        Status = 'PASS'
                    }
                }
            }
            [pscustomobject][ordered]@{
                Valid = $true
                StateDiffManifestExact = $true
                StateDiffRows = 18
                NonzeroStateDiffRows = 0
                FinalOrphanCleanupPass = $true
                FinalOrphanActiveGroups = 0
                CorrectedCanaryContract = $false
                SourceArtifactContract = $true
                SourceCanaryContract = -not $intentional
                SourceCanaryCaseSetExact = -not $intentional
                SourceCanaryAssertionManifestExact = -not $intentional
                SourceCanaryWarningContractExact = -not $intentional
                SourceCanaryNoBlockedAssertions = -not $intentional
                SourceCanaryOrphanContractExact = -not $intentional
                FocusedCaseStatus = if ($intentional) { '' } else { 'PASS' }
                FocusedAssertions = $focusedRows
                IntentionalMissionConvoyAdmissionDiagnosticsProven = $intentional
                IntentionalMissionConvoySettlementDiagnosticProven = $intentional
                IntentionalMissionConvoyCorruptionDiagnosticsProven = $intentional
                IntentionalMissionConvoyWatchdogDiagnosticProven = $intentional
            }
        }
        $newCensus = {
            param([string]$Profile)
            $full = $Profile -ceq 'full_certification'
            [pscustomobject][ordered]@{
                Valid = $true
                HardDiagnosticFree = $false
                HardDiagnosticCount = if ($full) { 15 } else { 2 }
                ScriptErrors = if ($full) { 15 } else { 2 }
                EngineErrors = 0
                PartisanErrors = if ($full) { 13 } else { 0 }
                CrashMarkers = 0
                PartisanSeverityLineCount = 0
                ApprovedStockDiagnosticCount = 2
                ApprovedShutdownCatalogDiagnosticCount = 0
                ApprovedIntentionalDiagnosticCount = if ($full) { 13 } else { 0 }
                MalformedHardDiagnosticCount = 0
                UnapprovedHardDiagnosticCount = 0
                UnapprovedHardDiagnosticKinds = @()
                ChannelArithmeticValid = $true
                CategoryArithmeticValid = $true
                LifecycleMarkersValid = $true
                IdentityBaselinePairValid = $true
                ShutdownCatalogPairValid = $true
                IntentionalFixtureStructureExact = $true
                IntentionalFixtureSetValid = $true
                CanonicalScriptLogCount = 1
                CanonicalConsoleLogCount = 1
                CanonicalErrorLogCount = 1
                CanonicalCrashLogCount = 1
                CanonicalLogPairSameDirectory = $true
                AuxiliaryLogPairSameDirectory = $true
                AuxiliaryDiagnosticsValid = $true
                ErrorLogProjectionExact = $true
                CrashLogProjectionExact = $true
                AuxiliaryUnapprovedEventCount = 0
                IntentionalMissionConvoyAdmissionDiagnosticsProven = $full
                IntentionalMissionConvoySettlementDiagnosticProven = $full
                IntentionalMissionConvoyCorruptionDiagnosticsProven = $full
                IntentionalMissionConvoyWatchdogDiagnosticProven = $full
            }
        }
        $invokeAcceptance = {
            param(
                [string]$Name,
                [string]$Profile,
                $Raw,
                $Validation,
                $Census
            )
            $path = Join-Path $tempRoot ($Name + '.json')
            Write-PortableJson -Path $path -Value $Raw
            Get-SourceCampaignDebugAcceptance `
                -JsonPath $path `
                -Profile $Profile `
                -ArtifactValidation $Validation `
                -ErrorCensus $Census `
                -ClassifierChecks 55 `
                -CaptureAxesPassed $true
        }

        $fullCases = @(& $newCase `
            -Id 'full.pass' `
            -Status 'PASS' `
            -Assertions @(& $newAssertion -Id 'full.certifying'))
        $fullRaw = & $newRaw `
            -Profile 'full_certification' `
            -Cases $fullCases `
            -CertificationPassed $true
        $fullAcceptance = & $invokeAcceptance `
            -Name 'accepted-full' `
            -Profile 'full_certification' `
            -Raw $fullRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census (& $newCensus 'full_certification')
        if (-not $fullAcceptance.acceptedFull -or
            $fullAcceptance.disposition -cne 'accepted-full') {
            throw 'The strict full-profile acceptance self-test failed.'
        }

        $externalAssertionParameters = @(
            [pscustomobject]@{ Id = 'isolation.world_scope'; CaseId = 'cleanup.state_isolation_restore'; Category = 'cleanup'; Feature = 'campaign_debug'; Stage = 'state_restore'; Expected = 'runtime certification remains scoped to the disposable development session'; Actual = 'world runtime, player inventory, health, and service caches require session restart before another certifying run'; Reason = 'restart the disposable development session before another certification run' },
            [pscustomobject]@{ Id = 'persistence.real_restart'; CaseId = 'persistence.seeded_roundtrip.phase12'; Category = 'persistence'; Feature = 'persistence_smoke'; Stage = 'early_phase'; Expected = 'external process restart / reconnect remains an explicit later-gate scenario'; Actual = 'non-certifying external advisory | restart/fault gate'; Reason = 'run the Workshop-published addon through the external restart matrix before claiming restart certification' },
            [pscustomobject]@{ Id = 'phase25.real_restart'; CaseId = 'phase25.manual_external_gaps'; Category = 'soak'; Feature = 'external_harness'; Stage = 'final'; Expected = 'real restart-after-primitive remains an explicit later-gate external scenario'; Actual = 'non-certifying external advisory | restart/fault gate'; Reason = 'run the Workshop-published addon through the external restart matrix before claiming restart certification' },
            [pscustomobject]@{ Id = 'phase25.second_client'; CaseId = 'phase25.manual_external_gaps'; Category = 'soak'; Feature = 'external_harness'; Stage = 'final'; Expected = 'second-client join/reconnect remains an explicit later-gate external scenario'; Actual = 'non-certifying external advisory | multiplayer/JIP gate'; Reason = 'run the Workshop-published addon with the required clients before claiming multiplayer certification' },
            [pscustomobject]@{ Id = 'phase25.two_hour_soak'; CaseId = 'phase25.manual_external_gaps'; Category = 'soak'; Feature = 'external_harness'; Stage = 'final'; Expected = 'two-hour endurance remains an explicit later-gate external scenario'; Actual = 'non-certifying external advisory | soak gate'; Reason = 'run the Workshop-published addon for the required duration before claiming soak certification' })
        $internalCases = New-Object Collections.Generic.List[object]
        [void]$internalCases.Add($fullCases[0])
        foreach ($group in @($externalAssertionParameters | Group-Object CaseId)) {
            $rows = @($group.Group)
            $assertions = @($rows | ForEach-Object {
                & $newAssertion `
                    -Id ([string]$_.Id) `
                    -Status 'WARN' `
                    -Certifying $false `
                    -ProofLevel 'EXTERNAL_PROCESS' `
                    -ObservedPath 'manual_external_gap' `
                    -RequiredPath 'external process restart, reconnect, or long-soak harness' `
                    -Expected ([string]$_.Expected) `
                    -Actual ([string]$_.Actual) `
                    -Reason ([string]$_.Reason)
            })
            if ([string]$rows[0].CaseId -ceq 'phase25.manual_external_gaps') {
                $assertions += @(
                    (& $newAssertion -Id 'phase24.escalation.support_physicalization' -Status 'SKIPPED' -Certifying $false),
                    (& $newAssertion -Id 'phase24.escalation.group_physicalization' -Status 'SKIPPED' -Certifying $false))
            }
            [void]$internalCases.Add((& $newCase `
                -Id ([string]$rows[0].CaseId) `
                -Status 'WARN' `
                -Assertions $assertions `
                -Category ([string]$rows[0].Category) `
                -Feature ([string]$rows[0].Feature) `
                -Stage ([string]$rows[0].Stage)))
        }
        $internalRaw = & $newRaw `
            -Profile 'full_certification' `
            -Cases $internalCases.ToArray() `
            -CertificationPassed $true
        $internalAcceptance = & $invokeAcceptance `
            -Name 'accepted-internal' `
            -Profile 'full_certification' `
            -Raw $internalRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census (& $newCensus 'full_certification')
        if (-not $internalAcceptance.acceptedInternal -or
            $internalAcceptance.disposition -cne 'accepted-internal') {
            throw 'The strict internal-profile acceptance self-test failed.'
        }

        $canaryCases = New-Object Collections.Generic.List[object]
        $nextAssertion = 0
        for ($caseIndex = 0; $caseIndex -lt 9; $caseIndex++) {
            $caseAssertionCount = if ($caseIndex -eq 0) { 81 } else { 1 }
            $caseAssertions = New-Object Collections.Generic.List[object]
            for ($caseAssertionIndex = 0;
                $caseAssertionIndex -lt $caseAssertionCount;
                $caseAssertionIndex++) {
                $certifying = $nextAssertion -lt 87
                [void]$caseAssertions.Add((& $newAssertion `
                    -Id "canary.pass.$nextAssertion" `
                    -Status 'PASS' `
                    -Certifying $certifying))
                $nextAssertion++
            }
            [void]$canaryCases.Add((& $newCase `
                -Id "canary.case.$caseIndex" `
                -Status 'PASS' `
                -Assertions $caseAssertions.ToArray()))
        }
        [void]$canaryCases.Add((& $newCase `
            -Id 'cleanup.player_marker_completion' `
            -Status 'WARN' `
            -Assertions @(& $newAssertion `
                -Id 'cleanup.player_marker.live' `
                -Status 'WARN' `
                -Certifying $false)))
        [void]$canaryCases.Add((& $newCase `
            -Id 'cleanup.state_isolation_restore' `
            -Status 'WARN' `
            -Assertions @(& $newAssertion `
                -Id 'isolation.world_scope' `
                -Status 'WARN' `
                -Certifying $false)))
        $canaryRaw = & $newRaw `
            -Profile 'force_authority' `
            -Cases $canaryCases.ToArray() `
            -CertificationPassed $false
        $canaryAcceptance = & $invokeAcceptance `
            -Name 'accepted-canary' `
            -Profile 'force_authority' `
            -Raw $canaryRaw `
            -Validation (& $newValidation 'force_authority') `
            -Census (& $newCensus 'force_authority')
        if (-not $canaryAcceptance.correctedCanaryAccepted -or
            $canaryAcceptance.disposition -cne 'accepted-corrected-canary') {
            throw 'The corrected-canary acceptance self-test failed.'
        }

        $shutdownCanaryCensus = & $newCensus 'force_authority'
        $shutdownCanaryCensus.ApprovedShutdownCatalogDiagnosticCount = 2
        $shutdownCanaryCensus.ApprovedStockDiagnosticCount = 4
        $shutdownCanaryCensus.HardDiagnosticCount = 4
        $shutdownCanaryCensus.ScriptErrors = 4
        $shutdownCanaryAcceptance = & $invokeAcceptance `
            -Name 'accepted-canary-with-shutdown-catalog-pair' `
            -Profile 'force_authority' `
            -Raw $canaryRaw `
            -Validation (& $newValidation 'force_authority') `
            -Census $shutdownCanaryCensus
        if (-not $shutdownCanaryAcceptance.correctedCanaryAccepted -or
            $shutdownCanaryAcceptance.disposition -cne
                'accepted-corrected-canary') {
            throw 'The optional shutdown catalog pair canary self-test failed.'
        }

        $shutdownFullCensus = & $newCensus 'full_certification'
        $shutdownFullCensus.ApprovedShutdownCatalogDiagnosticCount = 2
        $shutdownFullCensus.ApprovedStockDiagnosticCount = 4
        $shutdownFullCensus.HardDiagnosticCount = 17
        $shutdownFullCensus.ScriptErrors = 17
        $shutdownFullAcceptance = & $invokeAcceptance `
            -Name 'accepted-full-with-shutdown-catalog-pair' `
            -Profile 'full_certification' `
            -Raw $fullRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census $shutdownFullCensus
        if (-not $shutdownFullAcceptance.acceptedFull -or
            $shutdownFullAcceptance.disposition -cne 'accepted-full') {
            throw 'The optional shutdown catalog pair full self-test failed.'
        }

        $partialShutdownCensus = & $newCensus 'force_authority'
        $partialShutdownCensus.ApprovedShutdownCatalogDiagnosticCount = 1
        $partialShutdownCensus.ApprovedStockDiagnosticCount = 3
        $partialShutdownCensus.HardDiagnosticCount = 3
        $partialShutdownCensus.ScriptErrors = 3
        $partialShutdownCensus.ShutdownCatalogPairValid = $false
        $partialShutdownAcceptance = & $invokeAcceptance `
            -Name 'rejected-partial-shutdown-catalog-pair' `
            -Profile 'force_authority' `
            -Raw $canaryRaw `
            -Validation (& $newValidation 'force_authority') `
            -Census $partialShutdownCensus
        if ($partialShutdownAcceptance.accepted -or
            @($partialShutdownAcceptance.redAxes) -cnotcontains
                'diagnostic-common') {
            throw 'The partial shutdown catalog pair rejection self-test failed.'
        }

        $stringOrphanValidation = & $newValidation 'force_authority'
        $stringOrphanValidation.FinalOrphanActiveGroups = '0'
        $stringOrphanAcceptance = & $invokeAcceptance `
            -Name 'rejected-string-final-orphan-count' `
            -Profile 'force_authority' `
            -Raw $canaryRaw `
            -Validation $stringOrphanValidation `
            -Census (& $newCensus 'force_authority')
        if ($stringOrphanAcceptance.accepted -or
            @($stringOrphanAcceptance.redAxes) -cnotcontains 'final-cleanup') {
            throw 'The string final-orphan count rejection self-test failed.'
        }

        $sourceCanaryAcceptance = & $invokeAcceptance `
            -Name 'accepted-production-shaped-source-canary' `
            -Profile 'force_authority' `
            -Raw $artifactResult.SourceCanaryRun `
            -Validation $artifactResult.SourceCanaryValidation `
            -Census (& $newCensus 'force_authority')
        if (-not $sourceCanaryAcceptance.correctedCanaryAccepted -or
            $sourceCanaryAcceptance.disposition -cne 'accepted-corrected-canary' -or
            @($sourceCanaryAcceptance.redAxes).Count -ne 0) {
            throw 'The production-shaped source-canary acceptance self-test failed.'
        }

        $redRaw = $fullRaw | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        $redRaw.m_bCertificationPassed = $false
        $redAcceptance = & $invokeAcceptance `
            -Name 'rejected-red-full' `
            -Profile 'full_certification' `
            -Raw $redRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census (& $newCensus 'full_certification')
        if ($redAcceptance.accepted -or
            @($redAcceptance.redAxes) -cnotcontains 'full-proof-common') {
            throw 'The mechanically valid red full-profile rejection self-test failed.'
        }

        $stringBooleanRaw = $fullRaw | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        $stringBooleanRaw.m_bCertificationPassed = 'true'
        $stringBooleanAcceptance = & $invokeAcceptance `
            -Name 'rejected-string-boolean' `
            -Profile 'full_certification' `
            -Raw $stringBooleanRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census (& $newCensus 'full_certification')
        if ($stringBooleanAcceptance.accepted -or
            @($stringBooleanAcceptance.redAxes) -cnotcontains
                'certification-boolean-type') {
            throw 'The native JSON Boolean rejection self-test failed.'
        }
        $stringIntegerRaw = $fullRaw | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        $stringIntegerRaw.m_iPassCount = '1'
        $stringIntegerAcceptance = & $invokeAcceptance `
            -Name 'rejected-string-integer' `
            -Profile 'full_certification' `
            -Raw $stringIntegerRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census (& $newCensus 'full_certification')
        if ($stringIntegerAcceptance.accepted -or
            @($stringIntegerAcceptance.redAxes) -cnotcontains 'raw-pass-count-type') {
            throw 'The native JSON integer rejection self-test failed.'
        }
        $stringFlagRaw = $fullRaw | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        $stringFlagRaw.m_aCases[0].m_aAssertions[0].m_bCountsTowardCertification = 'true'
        $stringFlagAcceptance = & $invokeAcceptance `
            -Name 'rejected-string-certification-flag' `
            -Profile 'full_certification' `
            -Raw $stringFlagRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census (& $newCensus 'full_certification')
        if ($stringFlagAcceptance.accepted -or
            @($stringFlagAcceptance.redAxes) -cnotcontains 'certification-flag-type') {
            throw 'The native JSON certification-flag rejection self-test failed.'
        }
        $badLinkRaw = $internalRaw | ConvertTo-Json -Depth 20 | ConvertFrom-Json
        $badLinkAssertion = @($badLinkRaw.m_aCases | ForEach-Object {
                @($_.m_aAssertions) | Where-Object {
                    [string]$_.m_sAssertionId -ceq 'phase25.two_hour_soak'
                }
            })[0]
        $badLinkAssertion.m_sActual = 'incorrect external advisory meaning'
        $badLinkAcceptance = & $invokeAcceptance `
            -Name 'rejected-external-linkage' `
            -Profile 'full_certification' `
            -Raw $badLinkRaw `
            -Validation (& $newValidation 'full_certification') `
            -Census (& $newCensus 'full_certification')
        if ($badLinkAcceptance.accepted -or
            @($badLinkAcceptance.redAxes) -cnotcontains
                'external-advisory-linkage') {
            throw 'The external-advisory exact-linkage rejection self-test failed.'
        }
        $acceptanceChecks = 13

        $appendAmbientRows = {
            param(
                [Parameter(Mandatory = $true)]
                [AllowEmptyCollection()]
                [Collections.Generic.List[string]]$Lines,
                [Parameter(Mandatory = $true)]
                [AllowEmptyCollection()][string[]]$Signatures,
                [Parameter(Mandatory = $true)][string]$Timestamp,
                [bool]$CompactHeader = $false
            )
            $channelGap = if ($CompactHeader) { '' } else { ' ' }
            foreach ($signature in $Signatures) {
                $parts = $signature -split "`t", 3
                [void]$Lines.Add(('{0} {1}{2}({3}): {4}' -f
                        $Timestamp,
                        $parts[0],
                        $channelGap,
                        $parts[1],
                        $parts[2]))
            }
        }
        $appendIntentionalResourcePair = {
            param(
                [Parameter(Mandatory = $true)]
                [Collections.Generic.List[string]]$Lines,
                [Parameter(Mandatory = $true)][int]$PairIndex,
                [Parameter(Mandatory = $true)][string]$Timestamp
            )
            $signatureIndex = $PairIndex * 2
            & $appendAmbientRows `
                -Lines $Lines `
                -Signatures @(
                    $script:SourceCampaignFullIntentionalResourceErrors[$signatureIndex]) `
                -Timestamp $Timestamp
            $resourcePath = [string](
                $script:SourceCampaignFullIntentionalResourceBindings[$PairIndex].resourcePath)
            [void]$Lines.Add(
                "$Timestamp RESOURCES : GetResourceObject '$resourcePath'")
            & $appendAmbientRows `
                -Lines $Lines `
                -Signatures @(
                    $script:SourceCampaignFullIntentionalResourceErrors[$signatureIndex + 1]) `
                -Timestamp $Timestamp
        }
        $copyAmbientLines = {
            param([Parameter(Mandatory = $true)][string[]]$Lines)
            $copy = New-Object Collections.Generic.List[string]
            $copy.AddRange($Lines)
            return ,$copy
        }
        $assertAmbientRejected = {
            param(
                [Parameter(Mandatory = $true)][string]$Name,
                [Parameter(Mandatory = $true)][string[]]$Lines,
                [Parameter(Mandatory = $true)][string]$Profile
            )
            $census = Get-SourceCampaignAmbientErrorCensusFromText `
                -ConsoleText ($Lines -join "`n") `
                -Profile $Profile
            if ($census.valid) {
                throw "The ambient-diagnostic negative self-test passed unexpectedly: $Name"
            }
        }

        $forceArmLine =
            '12:00:01.000 SCRIPT : Partisan campaign debug CLI | armed focused force_authority run'
        $forceStartLine =
            '12:00:01.100 SCRIPT : Partisan campaign debug CLI | started force_authority on attempt 1'
        $forceMapBindingLine =
            '12:00:02.000 SCRIPT : Partisan player map marker debug | refresh requested player spawned'
        $forceDoneLine =
            '12:00:05.000 SCRIPT : Partisan campaign debug | DONE | focused source canary complete'
        $forceDestroyLine = '12:00:06.000 ENGINE : Game destroyed.'
        $ambientAbsentForce = Get-SourceCampaignAmbientErrorCensusFromText `
            -ConsoleText (@(
                    $forceArmLine,
                    $forceStartLine,
                    $forceDoneLine,
                    $forceDestroyLine) -join "`n") `
            -Profile 'force_authority'
        $forceAmbientLines = New-Object Collections.Generic.List[string]
        & $appendAmbientRows `
            -Lines $forceAmbientLines `
            -Signatures @($script:SourceCampaignStartupGuiError) `
            -Timestamp '12:00:00.000'
        & $appendAmbientRows `
            -Lines $forceAmbientLines `
            -Signatures $script:SourceCampaignStartupPathfindingErrors `
            -Timestamp '12:00:00.100' `
            -CompactHeader $true
        [void]$forceAmbientLines.Add($forceArmLine)
        [void]$forceAmbientLines.Add($forceStartLine)
        [void]$forceAmbientLines.Add($forceMapBindingLine)
        & $appendAmbientRows `
            -Lines $forceAmbientLines `
            -Signatures @($script:SourceCampaignMapMaskError) `
            -Timestamp '12:00:02.100'
        [void]$forceAmbientLines.Add($forceDoneLine)
        [void]$forceAmbientLines.Add($forceDestroyLine)
        & $appendAmbientRows `
            -Lines $forceAmbientLines `
            -Signatures $script:SourceCampaignFocusedTeardownResourceErrors `
            -Timestamp '12:00:06.100'
        $forceAmbient = Get-SourceCampaignAmbientErrorCensusFromText `
            -ConsoleText ($forceAmbientLines.ToArray() -join "`n") `
            -Profile 'force_authority'
        $expectedForceAmbientCount = 1 +
            $script:SourceCampaignStartupPathfindingErrors.Count + 1 +
            $script:SourceCampaignFocusedTeardownResourceErrors.Count
        if (-not $ambientAbsentForce.valid -or
            -not $forceAmbient.valid -or -not $forceAmbient.lifecycleExact -or
            $forceAmbient.approvedAmbientDiagnosticCount -ne
                $expectedForceAmbientCount -or
            $forceAmbient.startupPathfindingCount -ne 11 -or
            $forceAmbient.mapMaskCount -ne 1 -or
            $forceAmbient.teardownResourceCount -ne 53) {
            throw 'The focused captured-shape ambient-diagnostic self-test failed.'
        }
        $focusedStartupOnlyLines = @(
            '12:00:00.000 GUI (E): Unknown class ''SCR_WidgetExportRuleRoot'' at offset 282(0x11a)',
            $forceArmLine,
            $forceStartLine,
            $forceDoneLine,
            $forceDestroyLine)
        $focusedStartupOnly = Get-SourceCampaignAmbientErrorCensusFromText `
            -ConsoleText ($focusedStartupOnlyLines -join "`n") `
            -Profile 'force_authority'
        if (-not $focusedStartupOnly.valid -or
            $focusedStartupOnly.approvedAmbientDiagnosticCount -ne 1 -or
            $focusedStartupOnly.expectedCompleteDiagnosticCount -ne 1 -or
            $focusedStartupOnly.expectedTeardownResourceCount -ne 0 -or
            $focusedStartupOnly.teardownResourceCount -ne 0) {
            throw 'The focused nonempty no-teardown count self-test failed.'
        }

        $fullArmLine =
            '12:01:01.000 SCRIPT : Partisan campaign debug CLI | armed exact HST_Dev full certification run'
        $fullStartLine =
            '12:01:01.100 SCRIPT : Partisan campaign debug CLI | started full_certification on attempt 1'
        $fullMapBindingLine =
            '12:01:02.000 SCRIPT : Partisan request bridge debug | map ui ready'
        $fullResourcePair1PreviousLine =
            '12:01:02.110 SCRIPT : Partisan campaign debug | PASS | post_case_cleanup.authorization_commander_disconnect_handoff_runtime | assertions passed'
        $fullResourcePair1NextLine =
            '12:01:02.210 SCRIPT : Partisan campaign debug | PASS | force_composition.contract.runtime | assertions passed'
        $fullResourcePair2PreviousLine =
            '12:01:02.220 SCRIPT : Partisan campaign debug | PASS | post_case_cleanup.force_composition_contract_runtime | assertions passed'
        $fullResourcePair2NextLine =
            '12:01:02.310 SCRIPT : Partisan campaign debug | PASS | observation.force_composition | assertions passed'
        $fullResourcePair3PreviousLine =
            '12:01:02.320 SCRIPT (E): Partisan exact mission convoy | mission_convoy_proof_cargo_duplicate failed closed: exact mission convoy admission contains more than one optional cargo row'
        $fullResourcePair3NextLine =
            '12:01:02.410 SCRIPT (E): Partisan exact mission convoy | mission_convoy_proof_cargo_invalid_prefab failed closed: exact mission convoy cargo prefab is missing, invalid, or not an entity prefab'
        $fullResourcePair4PreviousLine =
            '12:01:02.420 SCRIPT : Partisan campaign debug | PASS | post_case_cleanup.action_mechanic_exact_spawn_adapter_failure_member_transition | assertions passed'
        $fullResourcePair4NextLine =
            '12:01:02.510 SCRIPT : Partisan campaign debug | PASS | action.mechanic_exact_spawn_adapter_same-wave_failure_capture | assertions passed'
        $fullPathBindingLine =
            '12:01:03.000 SCRIPT : Partisan campaign debug | PASS | post_case_cleanup.mission_cleanup_logistics_ammo_truck_1 | cleanup complete'
        $fullPhase18Line =
            '12:01:06.050 SCRIPT : Partisan campaign debug | PASS | phase18.phase18_counterattack | assertions passed'
        $fullDoneLine =
            '12:01:08.000 SCRIPT : Partisan campaign debug | DONE | full source proof complete'
        $fullDestroyLine = '12:01:09.000 ENGINE : Game destroyed.'
        $ambientAbsentFull = Get-SourceCampaignAmbientErrorCensusFromText `
            -ConsoleText (@(
                    $fullArmLine,
                    $fullStartLine,
                    $fullDoneLine,
                    $fullDestroyLine) -join "`n") `
            -Profile 'full_certification'
        if ($ambientAbsentFull.valid -or
            @($ambientAbsentFull.problemCodes) -cnotcontains 'runtime-resources') {
            throw 'The full-profile missing-intentional-resource self-test failed.'
        }
        $fullAmbientLines = New-Object Collections.Generic.List[string]
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures @($script:SourceCampaignStartupGuiError) `
            -Timestamp '12:01:00.000'
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures $script:SourceCampaignStartupPathfindingErrors `
            -Timestamp '12:01:00.100'
        [void]$fullAmbientLines.Add($fullArmLine)
        [void]$fullAmbientLines.Add($fullStartLine)
        [void]$fullAmbientLines.Add($fullMapBindingLine)
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures @($script:SourceCampaignMapMaskError) `
            -Timestamp '12:01:02.100'
        [void]$fullAmbientLines.Add($fullResourcePair1PreviousLine)
        & $appendIntentionalResourcePair `
            -Lines $fullAmbientLines -PairIndex 0 -Timestamp '12:01:02.200'
        [void]$fullAmbientLines.Add($fullResourcePair1NextLine)
        [void]$fullAmbientLines.Add($fullResourcePair2PreviousLine)
        & $appendIntentionalResourcePair `
            -Lines $fullAmbientLines -PairIndex 1 -Timestamp '12:01:02.300'
        [void]$fullAmbientLines.Add($fullResourcePair2NextLine)
        [void]$fullAmbientLines.Add($fullResourcePair3PreviousLine)
        & $appendIntentionalResourcePair `
            -Lines $fullAmbientLines -PairIndex 2 -Timestamp '12:01:02.400'
        [void]$fullAmbientLines.Add($fullResourcePair3NextLine)
        [void]$fullAmbientLines.Add($fullResourcePair4PreviousLine)
        & $appendIntentionalResourcePair `
            -Lines $fullAmbientLines -PairIndex 3 -Timestamp '12:01:02.500'
        [void]$fullAmbientLines.Add($fullResourcePair4NextLine)
        $fullResourceOnlyLines = New-Object Collections.Generic.List[string]
        [void]$fullResourceOnlyLines.Add($fullArmLine)
        [void]$fullResourceOnlyLines.Add($fullStartLine)
        $fullResourcePreviousLines = @(
            $fullResourcePair1PreviousLine,
            $fullResourcePair2PreviousLine,
            $fullResourcePair3PreviousLine,
            $fullResourcePair4PreviousLine)
        $fullResourceNextLines = @(
            $fullResourcePair1NextLine,
            $fullResourcePair2NextLine,
            $fullResourcePair3NextLine,
            $fullResourcePair4NextLine)
        $fullResourceTimestamps = @(
            '12:01:02.200',
            '12:01:02.300',
            '12:01:02.400',
            '12:01:02.500')
        for ($pairIndex = 0; $pairIndex -lt 4; $pairIndex++) {
            [void]$fullResourceOnlyLines.Add(
                $fullResourcePreviousLines[$pairIndex])
            & $appendIntentionalResourcePair `
                -Lines $fullResourceOnlyLines `
                -PairIndex $pairIndex `
                -Timestamp $fullResourceTimestamps[$pairIndex]
            [void]$fullResourceOnlyLines.Add(
                $fullResourceNextLines[$pairIndex])
        }
        [void]$fullResourceOnlyLines.Add($fullDoneLine)
        [void]$fullResourceOnlyLines.Add($fullDestroyLine)
        $fullResourceOnly = Get-SourceCampaignAmbientErrorCensusFromText `
            -ConsoleText ($fullResourceOnlyLines.ToArray() -join "`n") `
            -Profile 'full_certification'
        if (-not $fullResourceOnly.valid -or
            $fullResourceOnly.approvedAmbientDiagnosticCount -ne 8 -or
            $fullResourceOnly.expectedCompleteDiagnosticCount -ne 8 -or
            $fullResourceOnly.expectedTeardownResourceCount -ne 0 -or
            $fullResourceOnly.intentionalResourceCount -ne 8 -or
            $fullResourceOnly.teardownResourceCount -ne 0) {
            throw 'The full nonempty no-teardown count self-test failed.'
        }

        $interleavedIntentionalResources =
            & $copyAmbientLines $fullResourceOnlyLines.ToArray()
        $interleavedPair4PreviousIndex =
            $interleavedIntentionalResources.IndexOf(
                $fullResourcePair4PreviousLine)
        if ($interleavedPair4PreviousIndex -lt 0) {
            throw 'The benign intentional-resource interleaving fixture is incomplete.'
        }
        for ($index = 0; $index -lt 8; $index++) {
            $interleavedIntentionalResources.Insert(
                $interleavedPair4PreviousIndex + 1 + $index,
                "12:01:02.45$index WORLD : benign resource-proof interleave $index")
        }
        $interleavedIntentionalResourceCensus =
            Get-SourceCampaignAmbientErrorCensusFromText `
                -ConsoleText (
                    $interleavedIntentionalResources.ToArray() -join "`n") `
                -Profile 'full_certification'
        if (-not $interleavedIntentionalResourceCensus.valid -or
            $interleavedIntentionalResourceCensus.approvedAmbientDiagnosticCount -ne 8 -or
            $interleavedIntentionalResourceCensus.intentionalResourceCount -ne 8) {
            throw 'The benign intentional-resource interleaving self-test failed.'
        }

        $crossedStartIntentionalResources =
            & $copyAmbientLines $fullResourceOnlyLines.ToArray()
        [void]$crossedStartIntentionalResources.Remove(
            $fullResourcePair1PreviousLine)
        $crossedStartIntentionalResources.Insert(
            $crossedStartIntentionalResources.IndexOf($fullStartLine),
            $fullResourcePair1PreviousLine)
        & $assertAmbientRejected `
            -Name 'crossed-start-intentional-resource-binding' `
            -Lines $crossedStartIntentionalResources.ToArray() `
            -Profile 'full_certification'

        [void]$fullAmbientLines.Add($fullPathBindingLine)
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures $script:SourceCampaignRuntimePathfindingFamilyA `
            -Timestamp '12:01:04.000'
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures $script:SourceCampaignRuntimePathfindingFamilyB[0..9] `
            -Timestamp '12:01:06.000'
        [void]$fullAmbientLines.Add($fullPhase18Line)
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures @($script:SourceCampaignRuntimePathfindingFamilyB[10]) `
            -Timestamp '12:01:06.100'
        [void]$fullAmbientLines.Add($fullDoneLine)
        [void]$fullAmbientLines.Add($fullDestroyLine)
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures $script:SourceCampaignFullTeardownResourceErrors[0..28] `
            -Timestamp '12:01:09.100'
        & $appendAmbientRows `
            -Lines $fullAmbientLines `
            -Signatures $script:SourceCampaignFullTeardownResourceErrors[29..56] `
            -Timestamp '12:01:09.102'
        $fullAmbient = Get-SourceCampaignAmbientErrorCensusFromText `
            -ConsoleText ($fullAmbientLines.ToArray() -join "`n") `
            -Profile 'full_certification'
        $expectedFullAmbientCount = 1 +
            $script:SourceCampaignStartupPathfindingErrors.Count + 1 +
            $script:SourceCampaignFullIntentionalResourceErrors.Count +
            $script:SourceCampaignRuntimePathfindingFamilyA.Count +
            $script:SourceCampaignRuntimePathfindingFamilyB.Count +
            $script:SourceCampaignFullTeardownResourceErrors.Count
        if (-not $fullAmbient.valid -or -not $fullAmbient.lifecycleExact -or
            $fullAmbient.approvedAmbientDiagnosticCount -ne
                $expectedFullAmbientCount -or
            $fullAmbient.intentionalResourceCount -ne 8 -or
            $fullAmbient.runtimePathfindingBurstCount -ne 2 -or
            $fullAmbient.teardownResourceCount -ne 57) {
            throw 'The full captured-shape ambient-diagnostic self-test failed.'
        }

        $partialTeardown = & $copyAmbientLines $forceAmbientLines.ToArray()
        $partialTeardown.RemoveAt($partialTeardown.Count - 1)
        & $assertAmbientRejected `
            -Name 'partial-teardown' `
            -Lines $partialTeardown.ToArray() `
            -Profile 'force_authority'

        $reorderedTeardown = & $copyAmbientLines $forceAmbientLines.ToArray()
        $firstTeardownIndex = $reorderedTeardown.IndexOf($forceDestroyLine) + 1
        $lastTeardownIndex = $reorderedTeardown.Count - 1
        $firstTeardownRow = $reorderedTeardown[$firstTeardownIndex]
        $reorderedTeardown[$firstTeardownIndex] =
            $reorderedTeardown[$lastTeardownIndex]
        $reorderedTeardown[$lastTeardownIndex] = $firstTeardownRow
        $reorderedTeardownCensus = Get-SourceCampaignAmbientErrorCensusFromText `
            -ConsoleText ($reorderedTeardown.ToArray() -join "`n") `
            -Profile 'force_authority'
        if (-not $reorderedTeardownCensus.valid -or
            -not $reorderedTeardownCensus.lifecycleExact -or
            $reorderedTeardownCensus.teardownResourceCount -ne
                $script:SourceCampaignFocusedTeardownResourceErrors.Count) {
            throw 'The reordered teardown-resource multiset self-test failed.'
        }

        $duplicateTeardown = & $copyAmbientLines $forceAmbientLines.ToArray()
        $firstTeardownIndex = $duplicateTeardown.IndexOf($forceDestroyLine) + 1
        $duplicateTeardown[$duplicateTeardown.Count - 1] =
            $duplicateTeardown[$firstTeardownIndex]
        & $assertAmbientRejected `
            -Name 'duplicate-teardown-resource' `
            -Lines $duplicateTeardown.ToArray() `
            -Profile 'force_authority'

        $missingStart = & $copyAmbientLines $forceAmbientLines.ToArray()
        [void]$missingStart.Remove($forceStartLine)
        & $assertAmbientRejected `
            -Name 'missing-start' `
            -Lines $missingStart.ToArray() `
            -Profile 'force_authority'

        $duplicateStart = & $copyAmbientLines $forceAmbientLines.ToArray()
        $duplicateStart.Insert(
            $duplicateStart.IndexOf($forceStartLine) + 1,
            '12:00:01.101 SCRIPT : Partisan campaign debug CLI | started force_authority on attempt 2')
        & $assertAmbientRejected `
            -Name 'duplicate-start' `
            -Lines $duplicateStart.ToArray() `
            -Profile 'force_authority'

        $preStartAmbient = & $copyAmbientLines $forceAmbientLines.ToArray()
        $preStartBindingLine =
            '12:00:01.050 SCRIPT : Partisan request bridge debug | map ui ready'
        $mapMaskParts = $script:SourceCampaignMapMaskError -split "`t", 3
        $preStartAmbient.Insert(
            $preStartAmbient.IndexOf($forceStartLine),
            $preStartBindingLine)
        $preStartAmbient.Insert(
            $preStartAmbient.IndexOf($forceStartLine),
            ('12:00:01.060 {0} ({1}): {2}' -f
                $mapMaskParts[0], $mapMaskParts[1], $mapMaskParts[2]))
        & $assertAmbientRejected `
            -Name 'pre-start-ambient-family' `
            -Lines $preStartAmbient.ToArray() `
            -Profile 'force_authority'

        $crossedStartMapBinding =
            & $copyAmbientLines $forceAmbientLines.ToArray()
        [void]$crossedStartMapBinding.Remove($forceMapBindingLine)
        $crossedStartMapBinding.Insert(
            $crossedStartMapBinding.IndexOf($forceStartLine),
            $preStartBindingLine)
        & $assertAmbientRejected `
            -Name 'crossed-start-map-mask-binding' `
            -Lines $crossedStartMapBinding.ToArray() `
            -Profile 'force_authority'

        $unexpectedWorld = & $copyAmbientLines $forceAmbientLines.ToArray()
        $unexpectedWorld.Insert(
            $unexpectedWorld.IndexOf($forceDoneLine),
            '12:00:04.000 WORLD (E): unexpected world diagnostic')
        & $assertAmbientRejected `
            -Name 'unexpected-world' `
            -Lines $unexpectedWorld.ToArray() `
            -Profile 'force_authority'

        $wrongStartupPath = & $copyAmbientLines $forceAmbientLines.ToArray()
        for ($index = 0; $index -lt $wrongStartupPath.Count; $index++) {
            if ($wrongStartupPath[$index].Contains('<405, 66>')) {
                $wrongStartupPath[$index] =
                    $wrongStartupPath[$index].Replace('<405, 66>', '<405, 67>')
                break
            }
        }
        & $assertAmbientRejected `
            -Name 'wrong-startup-path-coordinate' `
            -Lines $wrongStartupPath.ToArray() `
            -Profile 'force_authority'

        $unboundMapMask = & $copyAmbientLines $forceAmbientLines.ToArray()
        [void]$unboundMapMask.Remove($forceMapBindingLine)
        & $assertAmbientRejected `
            -Name 'unbound-map-mask' `
            -Lines $unboundMapMask.ToArray() `
            -Profile 'force_authority'

        $forceIntentionalResource = & $copyAmbientLines $forceAmbientLines.ToArray()
        $resourceParts = $script:SourceCampaignFullIntentionalResourceErrors[0] -split "`t", 3
        $forceIntentionalResource.Insert(
            $forceIntentionalResource.IndexOf($forceDoneLine),
            ('12:00:04.000 {0} ({1}): {2}' -f
                $resourceParts[0], $resourceParts[1], $resourceParts[2]))
        & $assertAmbientRejected `
            -Name 'focused-intentional-resource' `
            -Lines $forceIntentionalResource.ToArray() `
            -Profile 'force_authority'

        $unboundRuntimePath = & $copyAmbientLines $fullAmbientLines.ToArray()
        [void]$unboundRuntimePath.Remove($fullPathBindingLine)
        & $assertAmbientRejected `
            -Name 'unbound-runtime-pathfinding' `
            -Lines $unboundRuntimePath.ToArray() `
            -Profile 'full_certification'

        $familyARows = New-Object Collections.Generic.List[string]
        & $appendAmbientRows `
            -Lines $familyARows `
            -Signatures $script:SourceCampaignRuntimePathfindingFamilyA `
            -Timestamp '12:01:04.000'
        $crossedStartFamilyABinding =
            & $copyAmbientLines $fullAmbientLines.ToArray()
        [void]$crossedStartFamilyABinding.Remove($fullPathBindingLine)
        foreach ($familyARow in $familyARows) {
            if (-not $crossedStartFamilyABinding.Remove($familyARow)) {
                throw 'The crossed-start Family A fixture is incomplete.'
            }
        }
        $crossedStartFamilyABinding.Insert(
            $crossedStartFamilyABinding.IndexOf($fullStartLine),
            $fullPathBindingLine)
        $crossedStartFamilyABinding.InsertRange(
            $crossedStartFamilyABinding.IndexOf($fullStartLine) + 1,
            $familyARows.ToArray())
        & $assertAmbientRejected `
            -Name 'crossed-start-family-a-binding' `
            -Lines $crossedStartFamilyABinding.ToArray() `
            -Profile 'full_certification'

        $familyBRows = New-Object Collections.Generic.List[string]
        & $appendAmbientRows `
            -Lines $familyBRows `
            -Signatures $script:SourceCampaignRuntimePathfindingFamilyB[0..9] `
            -Timestamp '12:01:06.000'
        & $appendAmbientRows `
            -Lines $familyBRows `
            -Signatures @($script:SourceCampaignRuntimePathfindingFamilyB[10]) `
            -Timestamp '12:01:06.100'
        $crossedStartFamilyBBinding =
            & $copyAmbientLines $fullAmbientLines.ToArray()
        [void]$crossedStartFamilyBBinding.Remove($fullPhase18Line)
        foreach ($familyBRow in $familyBRows) {
            if (-not $crossedStartFamilyBBinding.Remove($familyBRow)) {
                throw 'The crossed-start Family B fixture is incomplete.'
            }
        }
        $crossedStartFamilyBBinding.Insert(
            $crossedStartFamilyBBinding.IndexOf($fullStartLine),
            $fullPhase18Line)
        $crossedStartFamilyBBinding.InsertRange(
            $crossedStartFamilyBBinding.IndexOf($fullStartLine) + 1,
            $familyBRows.ToArray())
        & $assertAmbientRejected `
            -Name 'crossed-start-family-b-binding' `
            -Lines $crossedStartFamilyBBinding.ToArray() `
            -Profile 'full_certification'

        $skippedRuntimePathBoundary =
            & $copyAmbientLines $fullAmbientLines.ToArray()
        $skippedRuntimePathBoundary.Insert(
            $skippedRuntimePathBoundary.IndexOf($fullPathBindingLine) + 1,
            '12:01:03.100 SCRIPT : Partisan campaign debug | SKIPPED | post_case_cleanup.mission_cleanup_logistics_ammo_truck_synthetic | skipped cleanup boundary')
        & $assertAmbientRejected `
            -Name 'skipped-runtime-pathfinding-boundary' `
            -Lines $skippedRuntimePathBoundary.ToArray() `
            -Profile 'full_certification'

        $internalPassRuntimePathBoundary =
            & $copyAmbientLines $fullAmbientLines.ToArray()
        $internalPassRuntimePathBoundary.Insert(
            $internalPassRuntimePathBoundary.IndexOf($fullPathBindingLine) + 2,
            '12:01:03.200 SCRIPT : Partisan campaign debug | PASS | post_case_cleanup.mission_cleanup_logistics_ammo_truck_synthetic | internal pass boundary')
        & $assertAmbientRejected `
            -Name 'internal-pass-runtime-pathfinding-boundary' `
            -Lines $internalPassRuntimePathBoundary.ToArray() `
            -Profile 'full_certification'

        $internalSkippedRuntimePathBoundary =
            & $copyAmbientLines $fullAmbientLines.ToArray()
        $internalSkippedRuntimePathBoundary.Insert(
            $internalSkippedRuntimePathBoundary.IndexOf($fullPathBindingLine) + 2,
            '12:01:03.200 SCRIPT : Partisan campaign debug | SKIPPED | post_case_cleanup.mission_cleanup_logistics_ammo_truck_synthetic | internal skipped boundary')
        & $assertAmbientRejected `
            -Name 'internal-skipped-runtime-pathfinding-boundary' `
            -Lines $internalSkippedRuntimePathBoundary.ToArray() `
            -Profile 'full_certification'

        $unboundIntentionalResources =
            & $copyAmbientLines $fullResourceOnlyLines.ToArray()
        [void]$unboundIntentionalResources.Remove($fullResourcePair2NextLine)
        & $assertAmbientRejected `
            -Name 'unbound-intentional-resources' `
            -Lines $unboundIntentionalResources.ToArray() `
            -Profile 'full_certification'

        $reorderedIntentionalResources =
            & $copyAmbientLines $fullAmbientLines.ToArray()
        $firstIntentionalResourceIndex = -1
        for ($index = 0;
            $index -lt $reorderedIntentionalResources.Count - 1;
            $index++) {
            if ($reorderedIntentionalResources[$index].Contains(
                    'HST_Missing_Force_Debug.et')) {
                $firstIntentionalResourceIndex = $index
                break
            }
        }
        if ($firstIntentionalResourceIndex -lt 0) {
            throw 'The reordered intentional-resource self-test fixture is incomplete.'
        }
        $firstIntentionalResource =
            $reorderedIntentionalResources[$firstIntentionalResourceIndex]
        $reorderedIntentionalResources[$firstIntentionalResourceIndex] =
            $reorderedIntentionalResources[$firstIntentionalResourceIndex + 1]
        $reorderedIntentionalResources[$firstIntentionalResourceIndex + 1] =
            $firstIntentionalResource
        & $assertAmbientRejected `
            -Name 'reordered-intentional-resources' `
            -Lines $reorderedIntentionalResources.ToArray() `
            -Profile 'full_certification'

        $wrongProfileTeardown = New-Object Collections.Generic.List[string]
        [void]$wrongProfileTeardown.Add($forceArmLine)
        [void]$wrongProfileTeardown.Add($forceStartLine)
        [void]$wrongProfileTeardown.Add($forceDoneLine)
        [void]$wrongProfileTeardown.Add($forceDestroyLine)
        & $appendAmbientRows `
            -Lines $wrongProfileTeardown `
            -Signatures $script:SourceCampaignFullTeardownResourceErrors `
            -Timestamp '12:00:06.100'
        & $assertAmbientRejected `
            -Name 'wrong-profile-teardown' `
            -Lines $wrongProfileTeardown.ToArray() `
            -Profile 'force_authority'

        $postDoneHardRow = & $copyAmbientLines $forceAmbientLines.ToArray()
        $postDoneHardRow.Insert(
            $postDoneHardRow.IndexOf($forceDestroyLine),
            '12:00:05.500 GUI (E): unexpected post-DONE diagnostic')
        & $assertAmbientRejected `
            -Name 'post-done-hard-row' `
            -Lines $postDoneHardRow.ToArray() `
            -Profile 'force_authority'

        & $assertAmbientRejected `
            -Name 'malformed-hard-header' `
            -Lines @('12:00:00.000 gui(E): malformed hard diagnostic') `
            -Profile 'force_authority'

        $duplicateRuntimeFamily = & $copyAmbientLines $fullAmbientLines.ToArray()
        $familyBFirstLineIndex = -1
        for ($index = 0; $index -lt $duplicateRuntimeFamily.Count; $index++) {
            if ($duplicateRuntimeFamily[$index].Contains('<325, 219>')) {
                $familyBFirstLineIndex = $index
                break
            }
        }
        if ($familyBFirstLineIndex -lt 0) {
            throw 'The duplicate runtime-family self-test fixture is incomplete.'
        }
        $skippedFamilyBBoundary = & $copyAmbientLines $fullAmbientLines.ToArray()
        $skippedFamilyBBoundary.Insert(
            $familyBFirstLineIndex,
            '12:01:05.900 SCRIPT : Partisan campaign debug | SKIPPED | phase18.phase18_counterattack | skipped counterattack boundary')
        & $assertAmbientRejected `
            -Name 'skipped-family-b-boundary' `
            -Lines $skippedFamilyBBoundary.ToArray() `
            -Profile 'full_certification'
        $duplicateFamilyRows = New-Object Collections.Generic.List[string]
        & $appendAmbientRows `
            -Lines $duplicateFamilyRows `
            -Signatures $script:SourceCampaignRuntimePathfindingFamilyA `
            -Timestamp '12:01:05.000'
        $duplicateRuntimeFamily.InsertRange(
            $familyBFirstLineIndex,
            $duplicateFamilyRows.ToArray())
        & $assertAmbientRejected `
            -Name 'duplicate-runtime-family-binding' `
            -Lines $duplicateRuntimeFamily.ToArray() `
            -Profile 'full_certification'
        $ambientChecks = 32

        $resourceDatabaseIdentity = Get-SourceResourceDatabaseIdentity `
            -CheckoutRoot $CheckoutRoot
        if ($resourceDatabaseIdentity.bytes -le 0 -or
            $resourceDatabaseIdentity.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'The source resource-database identity self-test failed.'
        }
        $mountRoot = Join-Path $tempRoot 'source-mount'
        $mountLogRoot = Join-Path $mountRoot 'logs\session'
        New-Item -ItemType Directory -Path $mountLogRoot -Force | Out-Null
        $projectPath = Join-Path $CheckoutRoot 'addon.gproj'
        $resourceDatabasePath = Join-Path $CheckoutRoot 'resourceDatabase.rdb'
        $portableProjectPath = [IO.Path]::GetFullPath($projectPath).Replace('\', '/')
        $portableResourceDatabasePath =
            [IO.Path]::GetFullPath($resourceDatabasePath).Replace('\', '/')
        $validMountText = @(
            "12:00:00.000 ENGINE : gproj: '$portableProjectPath' guid: '698532771130111D'",
            "12:00:00.001 RESOURCES : ResourceDB: loading cache (id=0 name=histasi path=$portableResourceDatabasePath)") -join "`n"
        $mountLogPath = Join-Path $mountLogRoot 'console.log'
        Write-Utf8Text -Path $mountLogPath -Text $validMountText
        $validMount = Get-SourceMountAttestation `
            -IsolationRoot $mountRoot `
            -ExpectedProjectPath $projectPath `
            -ExpectedAddonGuid '698532771130111D' `
            -ExpectedResourceDatabasePath $resourceDatabasePath `
            -ExpectedResourceDatabaseIdentity $resourceDatabaseIdentity
        Write-Utf8Text `
            -Path $mountLogPath `
            -Text ($validMountText + "`n" +
                "12:00:00.002 ENGINE : gproj: '$portableProjectPath' guid: '0000000000000000'")
        $conflictingMount = Get-SourceMountAttestation `
            -IsolationRoot $mountRoot `
            -ExpectedProjectPath $projectPath `
            -ExpectedAddonGuid '698532771130111D' `
            -ExpectedResourceDatabasePath $resourceDatabasePath `
            -ExpectedResourceDatabaseIdentity $resourceDatabaseIdentity
        Write-Utf8Text `
            -Path $mountLogPath `
            -Text ($validMountText.Replace("guid: '698532771130111D'",
                    "guid: '698532771130111D' (packed)"))
        $packedMount = Get-SourceMountAttestation `
            -IsolationRoot $mountRoot `
            -ExpectedProjectPath $projectPath `
            -ExpectedAddonGuid '698532771130111D' `
            -ExpectedResourceDatabasePath $resourceDatabasePath `
            -ExpectedResourceDatabaseIdentity $resourceDatabaseIdentity
        $wrongResourcePath = Join-Path $CheckoutRoot 'nested\resourceDatabase.rdb'
        Write-Utf8Text `
            -Path $mountLogPath `
            -Text ($validMountText.Replace(
                    $portableResourceDatabasePath,
                    ([IO.Path]::GetFullPath($wrongResourcePath).Replace('\', '/'))))
        $wrongResourceMount = Get-SourceMountAttestation `
            -IsolationRoot $mountRoot `
            -ExpectedProjectPath $projectPath `
            -ExpectedAddonGuid '698532771130111D' `
            -ExpectedResourceDatabasePath $resourceDatabasePath `
            -ExpectedResourceDatabaseIdentity $resourceDatabaseIdentity
        if (-not $validMount.valid -or $conflictingMount.valid -or
            $packedMount.valid -or $wrongResourceMount.valid) {
            throw 'The exact source mount and resource-database self-test failed.'
        }
        $mountChecks = 4

        $trustedExecutableMock = [pscustomobject][ordered]@{
            signatureStatus = 'Valid'; signerThumbprint = 'A' * 40
            signerSubject = 'CN=BOHEMIA INTERACTIVE a.s., O=BOHEMIA INTERACTIVE a.s., C=CZ'
            companyName = 'Bohemia Interactive Studio'; productName = 'Arma Reforger'
            originalFilename = 'ArmaReforgerSteam'; fileVersion = '1.7.0.54'
        }
        $invalidExecutableMock = $trustedExecutableMock | ConvertTo-Json | ConvertFrom-Json
        $invalidExecutableMock.signatureStatus = 'NotSigned'
        if (-not (Test-TrustedDiagnosticExecutableIdentity `
                -Identity $trustedExecutableMock) -or
            (Test-TrustedDiagnosticExecutableIdentity `
                -Identity $invalidExecutableMock)) {
            throw 'The trusted diagnostic-executable self-test failed.'
        }
        $executableTrustChecks = 2

        $artifactFinalizationRoot = Join-Path $tempRoot 'artifact-finalization'
        New-Item -ItemType Directory -Path $artifactFinalizationRoot -Force | Out-Null
        $artifactFinalizationPaths = @(
            (Join-Path $artifactFinalizationRoot 'run.json'),
            (Join-Path $artifactFinalizationRoot 'run_summary.txt'),
            (Join-Path $artifactFinalizationRoot 'run_state_diff.txt'))
        foreach ($path in $artifactFinalizationPaths) {
            Write-Utf8Text -Path $path -Text ([IO.Path]::GetFileName($path))
        }
        $liveFinalizationSignature = Get-FileSignature `
            -Paths $artifactFinalizationPaths
        $stableFinalizationSignature = Get-FileSignature `
            -Paths $artifactFinalizationPaths
        Write-Utf8Text -Path $artifactFinalizationPaths[0] -Text 'mutated-after-live-poll'
        $driftedFinalizationSignature = Get-FileSignature `
            -Paths $artifactFinalizationPaths
        $unexpectedArtifactPath = Join-Path $artifactFinalizationRoot 'unexpected.txt'
        Write-Utf8Text -Path $unexpectedArtifactPath -Text 'unexpected'
        if ($liveFinalizationSignature -cne $stableFinalizationSignature -or
            $liveFinalizationSignature -ceq $driftedFinalizationSignature -or
            (Test-ExactStringSet `
                -Expected $artifactFinalizationPaths `
                -Actual ($artifactFinalizationPaths + $unexpectedArtifactPath))) {
            throw 'The post-shutdown artifact finalization self-test failed.'
        }
        $artifactFinalizationChecks = 3

        $bindingRepository = Join-Path $tempRoot 'binding-repository'
        New-Item -ItemType Directory -Path $bindingRepository -Force | Out-Null
        [void](Invoke-GitText -CheckoutRoot $bindingRepository -GitArguments @('init', '-q'))
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('config', 'user.name', 'Source Runner SelfTest'))
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('config', 'user.email', 'source-runner-selftest@example.invalid'))
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('config', 'commit.gpgSign', 'false'))
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('config', 'core.autocrlf', 'false'))
        New-Item `
            -ItemType Directory `
            -Path (Join-Path $bindingRepository 'Scripts') `
            -Force | Out-Null
        Write-Utf8Text `
            -Path (Join-Path $bindingRepository 'addon.gproj') `
            -Text "GameProject {`n}`n"
        Write-Utf8Text `
            -Path (Join-Path $bindingRepository '.gitignore') `
            -Text "Scripts/*.tmp`n"
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('add', '--', 'addon.gproj', '.gitignore'))
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('commit', '-q', '-m', 'selftest binding'))
        $bindingPublish = Get-PublishInputFingerprint `
            -CheckoutRoot $bindingRepository `
            -Revision 'HEAD'
        $cleanBinding = Get-WorkingPublishInputBinding `
            -CheckoutRoot $bindingRepository `
            -ExpectedPublishInput $bindingPublish
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('update-index', '--assume-unchanged', 'addon.gproj'))
        $flagRejected = $false
        try {
            [void](Get-WorkingPublishInputBinding `
                -CheckoutRoot $bindingRepository `
                -ExpectedPublishInput $bindingPublish)
        }
        catch {
            $flagRejected = $true
        }
        [void](Invoke-GitText `
            -CheckoutRoot $bindingRepository `
            -GitArguments @('update-index', '--no-assume-unchanged', 'addon.gproj'))
        Write-Utf8Text `
            -Path (Join-Path $bindingRepository 'Scripts\untracked.c') `
            -Text 'untracked'
        Write-Utf8Text `
            -Path (Join-Path $bindingRepository 'Scripts\ignored.tmp') `
            -Text 'ignored'
        $extraRejected = $false
        try {
            [void](Get-WorkingPublishInputBinding `
                -CheckoutRoot $bindingRepository `
                -ExpectedPublishInput $bindingPublish)
        }
        catch {
            $extraRejected = $true
        }
        if ($cleanBinding.rowCount -ne 1 -or -not $flagRejected -or
            -not $extraRejected) {
            throw 'The executed publish-input binding self-test failed.'
        }
        $workingBindingChecks = 3

        $portableSummaryPath = Join-Path $tempRoot 'portable-summary.json'
        Write-PortableSummaryJson `
            -Path $portableSummaryPath `
            -Value ([pscustomobject]@{ path = 'identity/source-binding.json' })
        $absoluteSummaryRejected = $false
        try {
            Write-PortableSummaryJson `
                -Path $portableSummaryPath `
                -Value ([pscustomobject]@{
                    path = 'C' + ':' + [IO.Path]::DirectorySeparatorChar + 'forbidden'
                })
        }
        catch {
            $absoluteSummaryRejected = $true
        }
        if (-not $absoluteSummaryRejected) {
            throw 'The portable-summary absolute-path rejection self-test failed.'
        }
        $portableSummaryChecks = 2
    }
    finally {
        if (Test-Path -LiteralPath $tempRoot -PathType Container) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction Stop
        }
    }

    $dummyExecutable = Join-Path $CheckoutRoot 'runtime with spaces.exe'
    $dummyArguments = New-SourceRuntimeArguments `
        -ResolvedAddonDirectory (Join-Path $CheckoutRoot 'base addons') `
        -AddonTempDirectory (Join-Path $CheckoutRoot 'temp addons') `
        -ResolvedProjectFile (Join-Path $CheckoutRoot 'addon.gproj') `
        -ResolvedWorldResource 'Worlds/HST_Dev/HST_Dev.ent' `
        -IsolationRoot (Join-Path $CheckoutRoot 'profile root') `
        -ResolvedProfile 'force_authority'
    $dummyCommandLine = (ConvertTo-NativeArgument $dummyExecutable) + ' ' +
        (($dummyArguments | ForEach-Object {
                    ConvertTo-NativeArgument ([string]$_)
                }) -join ' ')
    if (-not (Test-ExactNativeArgumentVector `
            -CommandLine $dummyCommandLine `
            -ExpectedExecutable $dummyExecutable `
            -ExpectedArguments $dummyArguments)) {
        throw 'The source Campaign Debug native-argument self-test failed.'
    }
    $publish = Get-PublishInputFingerprint -CheckoutRoot $CheckoutRoot -Revision 'HEAD'
    if ($publish.rowCount -le 0 -or $publish.sha256 -cnotmatch '^[0-9a-f]{64}$') {
        throw 'The canonical publish-input fingerprint self-test failed.'
    }
    Write-Output ('SELFTEST ' + ([pscustomobject][ordered]@{
        parserErrors = 0
        classifierParserErrors = $library.parserErrors
        importedClassifierFunctions = $library.importedFunctionCount
        diagnosticClassifierChecks = [int]$classifierChecks
        artifactValidatorValid = [bool]$artifactResult.Valid
        artifactNegativeChecks = @($artifactResult.NegativeStagedContractChecks).Count
        focusedValidatorChecks = @($artifactResult.FocusedValidatorChecks).Count
        sourceValidatorChecks = @($artifactResult.SourceValidatorChecks).Count
        acceptanceChecks = $acceptanceChecks
        ambientDiagnosticChecks = $ambientChecks
        sourceMountChecks = $mountChecks
        executableTrustChecks = $executableTrustChecks
        artifactFinalizationChecks = $artifactFinalizationChecks
        workingBindingChecks = $workingBindingChecks
        sourceResourceDatabaseBytes = [long]$resourceDatabaseIdentity.bytes
        sourceResourceDatabaseSha256 = [string]$resourceDatabaseIdentity.sha256
        portableSummaryChecks = $portableSummaryChecks
        emptyProcessArrayChecks = 4
        argumentRoundTrip = $true
        publishInputRows = $publish.rowCount
        publishInputSha256 = $publish.sha256
    } | ConvertTo-Json -Compress))
}

$checkoutRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$runnerPath = [IO.Path]::GetFullPath($PSCommandPath)
$classifierPath = Resolve-ExistingPath `
    -Path (Join-Path $PSScriptRoot 'run-guarded-campaign-debug.ps1') `
    -Kind Leaf

if ($SelfTest) {
    Invoke-SourceRunnerSelfTest `
        -RunnerPath $runnerPath `
        -ClassifierPath $classifierPath `
        -CheckoutRoot $checkoutRoot
    return
}

$Profile = $Profile.ToLowerInvariant()
$executablePath = Resolve-ExistingPath -Path $Executable -Kind Leaf
if ((Split-Path -Leaf $executablePath) -cne 'ArmaReforgerSteamDiag.exe') {
    throw 'Source Campaign Debug requires the diagnostic game executable.'
}
$addonDirectoryPath = Resolve-ExistingPath -Path $AddonDirectory -Kind Container
$settingsSourcePath = Resolve-ExistingPath -Path $SettingsSource -Kind Leaf
if ([string]::IsNullOrWhiteSpace($ProjectFile)) {
    $ProjectFile = Join-Path $checkoutRoot 'addon.gproj'
}
$projectPath = Resolve-ExistingPath -Path $ProjectFile -Kind Leaf
if (-not (Test-PathContained `
        -Root $checkoutRoot `
        -Candidate $projectPath)) {
    throw 'The source project must be inside the checkout.'
}
$evidenceRoot = [IO.Path]::GetFullPath($EvidenceOutputRoot)
foreach ($path in @(
        $executablePath,
        $addonDirectoryPath,
        $settingsSourcePath,
        $projectPath,
        $evidenceRoot)) {
    Assert-NoReparsePathAncestry -Path $path
}
foreach ($protectedRoot in @(
        $checkoutRoot,
        $addonDirectoryPath,
        (Split-Path -Parent $executablePath))) {
    if (Test-PathOverlap -First $evidenceRoot -Second $protectedRoot) {
        throw 'The external evidence root must not overlap source or runtime roots.'
    }
}

$libraryBinding = Import-CampaignDebugClassifierLibrary `
    -ClassifierPath $classifierPath `
    -IncludeSelfTests
$classifierChecks = Test-CampaignDebugHardDiagnosticCensus
if ([int]$classifierChecks -ne 55) {
    throw 'The Campaign Debug diagnostic classifier failed its preflight self-tests.'
}
$sourceBinding = Get-SourceBinding `
    -CheckoutRoot $checkoutRoot `
    -RunnerPath $runnerPath `
    -ClassifierPath $classifierPath `
    -ProjectPath $projectPath `
    -RequestedSourceGitHead $SourceGitHead

try {
    $settingsText = [IO.File]::ReadAllText($settingsSourcePath)
    $settings = $settingsText | ConvertFrom-Json
}
catch {
    throw 'Runtime settings could not be read and parsed.'
}
if (-not (Test-NativeJsonInteger -Value $settings.schemaVersion) -or
    [long]$settings.schemaVersion -ne [long]$sourceBinding.runtimeSettingsSchema) {
    throw 'Runtime settings do not match the source settings schema.'
}
$adminIdentityIds = @($settings.membership.adminIdentityIds)
if (-not (Test-NativeJsonBoolean `
        -Value $settings.membership.membershipEnabled) -or
    -not [bool]$settings.membership.membershipEnabled -or
    $adminIdentityIds.Count -ne 1 -or
    $adminIdentityIds[0] -isnot [string] -or
    [string]::IsNullOrWhiteSpace([string]$adminIdentityIds[0])) {
    throw 'Source Campaign Debug requires membership enforcement and exactly one trusted admin identity.'
}
if (-not (Test-NativeJsonBoolean -Value $settings.debug.debugMenuEnabled) -or
    -not (Test-NativeJsonBoolean -Value $settings.debug.debugLoggingEnabled) -or
    -not [bool]$settings.debug.debugMenuEnabled -or
    -not [bool]$settings.debug.debugLoggingEnabled) {
    throw 'Source Campaign Debug requires debug menu and logging settings.'
}

$engineBefore = @(Get-EngineProcessRows)
if ($engineBefore.Count -ne 0) {
    throw 'Refusing source Campaign Debug while an engine, Workbench, server, or crash-report process is running.'
}

New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
Assert-NoReparsePathAncestry -Path $evidenceRoot
$nonce = [Guid]::NewGuid().ToString('N')
$startUtc = [DateTime]::UtcNow
$runLeaf = 'source-campaign-debug-' + $Profile + '-' +
    $startUtc.ToString('yyyyMMddTHHmmssZ', [Globalization.CultureInfo]::InvariantCulture) +
    '-' + $nonce
$runRoot = [IO.Path]::GetFullPath((Join-Path $evidenceRoot $runLeaf))
if (-not (Test-PathContained -Root $evidenceRoot -Candidate $runRoot) -or
    (Test-Path -LiteralPath $runRoot)) {
    throw 'The source Campaign Debug evidence directory must be fresh and contained.'
}
$identityRoot = Join-Path $runRoot 'identity'
$configRoot = Join-Path $runRoot 'config'
$isolationRoot = Join-Path $runRoot 'isolation'
$profileDirectory = Join-Path $isolationRoot 'profile\Partisan'
$workingDirectory = Join-Path $isolationRoot 'working'
$tempDirectory = Join-Path $isolationRoot 'temp'
$addonTempDirectory = Join-Path $isolationRoot 'addon-temp'
New-Item `
    -ItemType Directory `
    -Path $identityRoot, $configRoot, $profileDirectory, $workingDirectory,
        $tempDirectory, $addonTempDirectory `
    -Force | Out-Null
Assert-NoReparsePathAncestry -Path $runRoot

$retainedSettingsPath = Join-Path $configRoot 'HST_Settings.json'
$runtimeSettingsPath = Join-Path $profileDirectory 'HST_Settings.json'
Copy-Item -LiteralPath $settingsSourcePath -Destination $retainedSettingsPath -Force
Copy-Item -LiteralPath $settingsSourcePath -Destination $runtimeSettingsPath -Force
$settingsSourceIdentity = Get-FileIdentity -Path $settingsSourcePath
$retainedSettingsIdentity = Get-FileIdentity -Path $retainedSettingsPath
$runtimeSettingsIdentity = Get-FileIdentity -Path $runtimeSettingsPath
if (-not (Test-FileIdentityEqual -First $settingsSourceIdentity -Second $retainedSettingsIdentity) -or
    -not (Test-FileIdentityEqual -First $settingsSourceIdentity -Second $runtimeSettingsIdentity)) {
    throw 'Isolated Campaign Debug settings differ from their source bytes.'
}

$addonGuid = Get-AddonGuid -ResolvedProjectFile $projectPath
$sourceResourceDatabasePath = Join-Path $checkoutRoot 'resourceDatabase.rdb'
$sourceResourceDatabaseIdentity = Get-SourceResourceDatabaseIdentity `
    -CheckoutRoot $checkoutRoot
$executableIdentity = Get-ExecutableIdentity -ExecutablePath $executablePath
if (-not (Test-TrustedDiagnosticExecutableIdentity -Identity $executableIdentity)) {
    throw 'Source Campaign Debug requires the trusted, validly signed diagnostic executable.'
}
$arguments = New-SourceRuntimeArguments `
    -ResolvedAddonDirectory $addonDirectoryPath `
    -AddonTempDirectory $addonTempDirectory `
    -ResolvedProjectFile $projectPath `
    -ResolvedWorldResource $WorldResource `
    -IsolationRoot $isolationRoot `
    -ResolvedProfile $Profile
$commandLine = (ConvertTo-NativeArgument $executablePath) + ' ' +
    (($arguments | ForEach-Object {
                ConvertTo-NativeArgument ([string]$_)
            }) -join ' ')
if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $commandLine `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $arguments)) {
    throw 'The source Campaign Debug native argument vector did not round-trip.'
}
$argumentVectorBinding = Get-ArgumentVectorBinding `
    -Arguments $arguments `
    -ResolvedAddonDirectory $addonDirectoryPath `
    -AddonTempDirectory $addonTempDirectory `
    -ResolvedProjectFile $projectPath `
    -IsolationRoot $isolationRoot `
    -CommandLine $commandLine

Write-Utf8Text `
    -Path (Join-Path $identityRoot 'publish-input-tree.txt') `
    -Text $sourceBinding.publishInputText
$sourceSummary = [pscustomobject][ordered]@{
    sourceGitHead = $sourceBinding.sourceHead
    publishInputPolicy = 'git-ls-tree-sha256-v1'
    publishInputTreeSha256 = $sourceBinding.publishInput.sha256
    publishInputRowCount = $sourceBinding.publishInput.rowCount
    executedPublishInputTreeSha256 = $sourceBinding.workingPublishInput.sha256
    executedPublishInputRowCount = $sourceBinding.workingPublishInput.rowCount
    sourceResourceDatabase = [pscustomobject][ordered]@{
        length = [long]$sourceResourceDatabaseIdentity.bytes
        sha256 = [string]$sourceResourceDatabaseIdentity.sha256
    }
    addonGuid = $addonGuid
    campaignSchema = $sourceBinding.campaignSchema
    runtimeSettingsSchema = $sourceBinding.runtimeSettingsSchema
    embeddedImplementationSha = $sourceBinding.build.sha
    embeddedBuildUtc = $sourceBinding.build.utc
    embeddedBuildLabel = $sourceBinding.build.label
}
$harnessSummary = [pscustomobject][ordered]@{
    gitHead = $sourceBinding.harnessHead
    dirty = $false
    runnerPath = $sourceBinding.runner.relativePath
    runnerSha256 = $sourceBinding.runner.gitBlobSha256
    runnerHashPolicy = 'sha256-git-blob-bytes-v1'
}
$sourceBindingForJson = [pscustomobject][ordered]@{
    source = $sourceSummary
    harness = $harnessSummary
    repository = [pscustomobject][ordered]@{
        sourceTree = $sourceBinding.sourceTree
        harnessTree = $sourceBinding.harnessTree
        branch = $sourceBinding.branch
        sourceCommitUtc = $sourceBinding.sourceCommitUtc
        harnessCommitUtc = $sourceBinding.harnessCommitUtc
        executedPublishInput = [pscustomobject][ordered]@{
            rowCount = $sourceBinding.workingPublishInput.rowCount
            sha256 = $sourceBinding.workingPublishInput.sha256
            untrackedExtraCount = $sourceBinding.workingPublishInput.untrackedExtraCount
            ignoredExtraCount = $sourceBinding.workingPublishInput.ignoredExtraCount
            nonstandardIndexFlagCount =
                $sourceBinding.workingPublishInput.nonstandardIndexFlagCount
        }
    }
    runner = [pscustomobject][ordered]@{
        path = $sourceBinding.runner.relativePath
        gitObject = $sourceBinding.runner.gitObject
        workingGitObject = $sourceBinding.runner.workingGitObject
        gitBlobSha256 = $sourceBinding.runner.gitBlobSha256
        workingFile = ConvertTo-PortableFileIdentity `
            -Identity $sourceBinding.runner.workingFile
    }
    classifier = [pscustomobject][ordered]@{
        path = $sourceBinding.classifier.relativePath
        gitObject = $sourceBinding.classifier.gitObject
        workingGitObject = $sourceBinding.classifier.workingGitObject
        gitBlobSha256 = $sourceBinding.classifier.gitBlobSha256
        workingFile = ConvertTo-PortableFileIdentity `
            -Identity $sourceBinding.classifier.workingFile
    }
    project = [pscustomobject][ordered]@{
        path = $sourceBinding.project.relativePath
        gitObject = $sourceBinding.project.gitObject
        workingGitObject = $sourceBinding.project.workingGitObject
        gitBlobSha256 = $sourceBinding.project.gitBlobSha256
        workingFile = ConvertTo-PortableFileIdentity `
            -Identity $sourceBinding.project.workingFile
    }
}
Write-PortableSummaryJson `
    -Path (Join-Path $identityRoot 'source-binding.json') `
    -Value $sourceBindingForJson
Write-PortableSummaryJson `
    -Path (Join-Path $identityRoot 'launch-binding.json') `
    -Value ([pscustomobject][ordered]@{
        profile = $Profile
        worldResource = $WorldResource
        addonGuid = $addonGuid
        executable = [pscustomobject][ordered]@{
            fileName = Split-Path -Leaf $executablePath
            identity = ConvertTo-PortableFileIdentity -Identity $executableIdentity
            fileVersion = $executableIdentity.fileVersion
            productVersion = $executableIdentity.productVersion
            productName = $executableIdentity.productName
            companyName = $executableIdentity.companyName
            originalFilename = $executableIdentity.originalFilename
            signatureStatus = $executableIdentity.signatureStatus
            signerThumbprint = $executableIdentity.signerThumbprint
            signerSubject = $executableIdentity.signerSubject
        }
        sourceResourceDatabase = [pscustomobject][ordered]@{
            length = [long]$sourceResourceDatabaseIdentity.bytes
            sha256 = [string]$sourceResourceDatabaseIdentity.sha256
        }
        arguments = $argumentVectorBinding
        settings = [pscustomobject][ordered]@{
            schemaVersion = [int]$settings.schemaVersion
            trustedAdminCount = @($settings.membership.adminIdentityIds).Count
            source = ConvertTo-PortableFileIdentity -Identity $settingsSourceIdentity
            retained = ConvertTo-PortableFileIdentity -Identity $retainedSettingsIdentity
            runtimeCopy = ConvertTo-PortableFileIdentity -Identity $runtimeSettingsIdentity
        }
        startedUtc = $startUtc.ToString('o', [Globalization.CultureInfo]::InvariantCulture)
    })

$mutex = $null
$mutexAcquired = $false
$job = $null
$suspendedLauncher = $null
$process = $null
$rootProcessId = 0
$rootStartUtc = [DateTime]::MinValue
$ownedProcesses = @{}
$maximumOwnedProcesses = 0
$unclaimedObserved = New-Object `
    'Collections.Generic.HashSet[string]' `
    ([StringComparer]::Ordinal)
$argumentTokensVerified = $false
$armed = $false
$started = $false
$completed = $false
$artifactSignature = ''
$finalArtifactSignature = ''
$postValidationArtifactSignature = ''
$artifactStablePolls = 0
$artifactPaths = @()
$artifactValidatorArguments = $null
$artifactValidation = $null
$artifactBytesStable = $false
$artifactSetExact = $false
$errorCensus = $null
$ambientErrorCensus = $null
$mountAttestation = $null
$sourceStable = $false
$sourceResourceDatabaseStable = $false
$settingsStable = $false
$executableStable = $false
$runError = $null
$cleanupErrors = New-Object Collections.Generic.List[string]
$stopwatch = [Diagnostics.Stopwatch]::StartNew()

try {
    $mutex = New-Object Threading.Mutex($false, 'Local\PartisanSourceCampaignDebugGuard')
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw 'Another source Campaign Debug runner is active.'
    }
    if (@(Get-EngineProcessRows).Count -ne 0) {
        throw 'An engine process appeared during source Campaign Debug preflight.'
    }

    $job = New-Object PartisanSourceCampaignDebugJob
    $previousTemp = [Environment]::GetEnvironmentVariable(
        'TEMP',
        [EnvironmentVariableTarget]::Process)
    $previousTmp = [Environment]::GetEnvironmentVariable(
        'TMP',
        [EnvironmentVariableTarget]::Process)
    try {
        [Environment]::SetEnvironmentVariable(
            'TEMP',
            $tempDirectory,
            [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            'TMP',
            $tempDirectory,
            [EnvironmentVariableTarget]::Process)
        $suspendedLauncher = New-Object PartisanSourceCampaignDebugSuspendedProcess(
            $executablePath,
            $commandLine,
            $workingDirectory)
    }
    finally {
        [Environment]::SetEnvironmentVariable(
            'TEMP',
            $previousTemp,
            [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            'TMP',
            $previousTmp,
            [EnvironmentVariableTarget]::Process)
    }
    $process = $suspendedLauncher.Child
    if (-not $process) {
        throw 'The source Campaign Debug runtime did not start.'
    }
    $rootProcessId = $process.Id
    $job.Add($process)
    $rootStartUtc = $process.StartTime.ToUniversalTime()
    $ownedProcesses[$rootProcessId] = $rootStartUtc
    $maximumOwnedProcesses = 1
    $suspendedLauncher.Resume()
    $suspendedLauncher.Dispose()
    $suspendedLauncher = $null

    Start-Sleep -Milliseconds 500
    $processRow = Get-CimInstance `
        Win32_Process `
        -Filter "ProcessId=$rootProcessId" `
        -ErrorAction Stop
    $argumentTokensVerified = Test-ExactNativeArgumentVector `
        -CommandLine ([string]$processRow.CommandLine) `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $arguments
    if (-not $argumentTokensVerified) {
        throw 'The launched source runtime argument vector differed from preflight.'
    }

    Write-Output ('PREFLIGHT ' + ([pscustomobject][ordered]@{
        profile = $Profile
        sourceHead = $sourceBinding.sourceHead
        publishInputSha256 = $sourceBinding.publishInput.sha256
        publishInputRows = $sourceBinding.publishInput.rowCount
        embeddedBuildSha = $sourceBinding.build.sha
        settingsSchema = [int]$settings.schemaVersion
        engineProcessesBefore = $engineBefore.Count
        killOnWrapperClose = $true
        argumentTokensVerified = $true
        diagnosticClassifierChecks = [int]$classifierChecks
    } | ConvertTo-Json -Compress))

    $nextHeartbeat = 0
    while ($stopwatch.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
        Start-Sleep -Seconds $PollSeconds
        Update-OwnedProcesses `
            -Owned $ownedProcesses `
            -RootProcessId $rootProcessId `
            -RootStartUtc $rootStartUtc `
            -Job $job
        $maximumOwnedProcesses = [Math]::Max($maximumOwnedProcesses, $ownedProcesses.Count)
        foreach ($engineProcess in @(Get-EngineProcessRows)) {
            if ($ownedProcesses.ContainsKey([int]$engineProcess.Id)) {
                continue
            }
            try {
                [void]$unclaimedObserved.Add(
                    "$($engineProcess.ProcessName):$($engineProcess.Id):$($engineProcess.StartTime.ToUniversalTime().Ticks)")
            }
            catch { }
        }
        if ($unclaimedObserved.Count -ne 0) {
            throw 'An unowned engine process appeared; the source run was cancelled and that process was left untouched.'
        }
        $process.Refresh()
        if ($process.HasExited) {
            throw "The diagnostic runtime exited before stable artifacts were accepted (exit $($process.ExitCode))."
        }

        $scriptTail = Get-ScriptLogTail -IsolationRoot $isolationRoot
        $armedMessage = 'Partisan campaign debug CLI | armed exact HST_Dev full certification run'
        if ($Profile -ceq 'force_authority') {
            $armedMessage = 'Partisan campaign debug CLI | armed focused force_authority run (not full certification)'
        }
        if (-not $armed -and $scriptTail.Contains($armedMessage)) {
            $armed = $true
        }
        if (-not $started -and
            $scriptTail -match ('Partisan campaign debug CLI \| started ' +
                [Regex]::Escape($Profile) + ' on attempt \d+')) {
            $started = $true
        }
        if (-not $armed -and $stopwatch.Elapsed.TotalSeconds -gt $ArmedTimeoutSeconds) {
            throw 'Campaign Debug did not arm within its startup window.'
        }
        if ($armed -and -not $started -and
            $stopwatch.Elapsed.TotalSeconds -gt $StartedTimeoutSeconds) {
            throw 'Campaign Debug armed but did not start within its startup window.'
        }

        $debugDirectory = Join-Path $isolationRoot 'profile\Partisan\debug'
        $jsonCandidates = @()
        if (Test-Path -LiteralPath $debugDirectory -PathType Container) {
            $jsonCandidates = @(Get-ChildItem `
                -LiteralPath $debugDirectory `
                -File `
                -Filter 'HST_CampaignDebug_*.json' `
                -Force `
                -ErrorAction Stop |
                Sort-Object LastWriteTimeUtc -Descending)
        }
        if ($jsonCandidates.Count -gt 1) {
            throw 'Campaign Debug produced more than one JSON artifact in the fresh run root.'
        }
        if ($jsonCandidates.Count -gt 0) {
            $jsonPath = $jsonCandidates[0].FullName
            $base = [IO.Path]::Combine(
                $jsonCandidates[0].DirectoryName,
                [IO.Path]::GetFileNameWithoutExtension($jsonCandidates[0].Name))
            $summaryPath = $base + '_summary.txt'
            $stateDiffPath = $base + '_state_diff.txt'
            $campaignArtifactFiles = @(Get-ChildItem `
                -LiteralPath $debugDirectory `
                -File `
                -Filter 'HST_CampaignDebug_*' `
                -Force `
                -ErrorAction Stop)
            if ($campaignArtifactFiles.Count -gt 3) {
                throw 'Campaign Debug produced more than one exact artifact set.'
            }
            if ((Test-Path -LiteralPath $summaryPath -PathType Leaf) -and
                (Test-Path -LiteralPath $stateDiffPath -PathType Leaf) -and
                (Get-Item -LiteralPath $jsonPath).Length -gt 0 -and
                (Get-Item -LiteralPath $summaryPath).Length -gt 0 -and
                (Get-Item -LiteralPath $stateDiffPath).Length -gt 0) {
                $expectedArtifactPaths = @(
                    [IO.Path]::GetFullPath($jsonPath),
                    [IO.Path]::GetFullPath($summaryPath),
                    [IO.Path]::GetFullPath($stateDiffPath))
                $actualArtifactPaths = @($campaignArtifactFiles | ForEach-Object {
                        [IO.Path]::GetFullPath($_.FullName)
                    })
                if (-not (Test-ExactStringSet `
                        -Expected $expectedArtifactPaths `
                        -Actual $actualArtifactPaths)) {
                    throw 'Campaign Debug artifact files do not form one exact triplet.'
                }
                $signature = Get-FileSignature `
                    -Paths @($jsonPath, $summaryPath, $stateDiffPath)
                if ($signature -ceq $artifactSignature) {
                    $artifactStablePolls++
                }
                else {
                    $artifactSignature = $signature
                    $artifactStablePolls = 1
                }
                if ($artifactStablePolls -ge 2) {
                    $artifactPaths = @($jsonPath, $summaryPath, $stateDiffPath)
                    $artifactValidatorArguments = @{
                        JsonPath = $jsonPath
                        SummaryPath = $summaryPath
                        StateDiffPath = $stateDiffPath
                        ExpectedSha = $sourceBinding.build.sha
                        ExpectedUtc = $sourceBinding.build.utc
                        ExpectedLabel = $sourceBinding.build.label
                        ExpectedProfile = $Profile
                        RequireSourceArtifactContract = $true
                    }
                    if ($Profile -ceq 'force_authority') {
                        $artifactValidatorArguments.RequireSourceCanaryContract = $true
                    }
                    $artifactValidation = Test-CampaignDebugArtifacts `
                        @artifactValidatorArguments
                    $artifactSetExact = $true
                    $completed = $true
                    break
                }
            }
        }
        if ($stopwatch.Elapsed.TotalSeconds -ge $nextHeartbeat) {
            Write-Output ('PROGRESS ' + ([pscustomobject][ordered]@{
                elapsedSeconds = [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
                armed = $armed
                started = $started
                artifactJsonCandidates = $jsonCandidates.Count
                stableArtifactPolls = $artifactStablePolls
                ownedProcesses = $ownedProcesses.Count
            } | ConvertTo-Json -Compress))
            $nextHeartbeat = $stopwatch.Elapsed.TotalSeconds + 30
        }
    }
    if (-not $completed) {
        throw 'Campaign Debug exceeded its guarded completion deadline.'
    }
}
catch {
    $runError = $_.Exception.Message
}
finally {
    if ($suspendedLauncher) {
        try { $suspendedLauncher.Dispose() } catch { [void]$cleanupErrors.Add('suspended-launcher') }
        $suspendedLauncher = $null
    }
    if ($process) {
        try {
            $process.Refresh()
            if (-not $process.HasExited) {
                [void]$process.CloseMainWindow()
                [void]$process.WaitForExit(10000)
            }
        }
        catch { [void]$cleanupErrors.Add('graceful-runtime-stop') }
    }
    if ($rootProcessId -gt 0 -and $rootStartUtc -ne [DateTime]::MinValue) {
        try {
            Update-OwnedProcesses `
                -Owned $ownedProcesses `
                -RootProcessId $rootProcessId `
                -RootStartUtc $rootStartUtc `
                -Job $job
        }
        catch { [void]$cleanupErrors.Add('owned-process-refresh') }
    }
    try { Stop-OwnedProcesses -Owned $ownedProcesses } catch { [void]$cleanupErrors.Add('owned-process-stop') }
    if ($job) {
        try { $job.Dispose() } catch { [void]$cleanupErrors.Add('process-job-dispose') }
        $job = $null
    }
    Start-Sleep -Seconds 2
    try { Stop-OwnedProcesses -Owned $ownedProcesses } catch { [void]$cleanupErrors.Add('owned-process-final-stop') }
    if ($process) {
        try { $process.Dispose() } catch { [void]$cleanupErrors.Add('runtime-dispose') }
        $process = $null
    }
    if ($mutexAcquired -and $mutex) {
        try { $mutex.ReleaseMutex() } catch { [void]$cleanupErrors.Add('mutex-release') }
    }
    if ($mutex) {
        try { $mutex.Dispose() } catch { [void]$cleanupErrors.Add('mutex-dispose') }
    }
    $stopwatch.Stop()
}

$postSourceBinding = $null
$postRuntimeSettingsIdentity = $null
$postRetainedSettingsIdentity = $null
$postSourceResourceDatabaseIdentity = $null
$postExecutableIdentity = $null
$ownedRemaining = 0
foreach ($ownedId in @($ownedProcesses.Keys)) {
    if (Test-ProcessIdentityAlive `
            -ProcessId ([int]$ownedId) `
            -StartUtc $ownedProcesses[[int]$ownedId]) {
        $ownedRemaining++
    }
}
$engineAfter = @(Get-EngineProcessRows)
if ($ownedRemaining -ne 0) {
    [void]$cleanupErrors.Add('owned-process-residue')
}
if ($engineAfter.Count -ne 0) {
    [void]$cleanupErrors.Add('engine-process-residue')
}

try {
    $postSourceBinding = Get-SourceBinding `
        -CheckoutRoot $checkoutRoot `
        -RunnerPath $runnerPath `
        -ClassifierPath $classifierPath `
        -ProjectPath $projectPath `
        -RequestedSourceGitHead $sourceBinding.sourceHead
    $sourceStable = Test-SourceBindingEqual `
        -First $sourceBinding `
        -Second $postSourceBinding
}
catch {
    $postSourceBinding = $null
    [void]$cleanupErrors.Add('source-binding-recheck')
}
try {
    $postRuntimeSettingsIdentity = Get-FileIdentity -Path $runtimeSettingsPath
    $postRetainedSettingsIdentity = Get-FileIdentity -Path $retainedSettingsPath
    $settingsStable = (Test-FileIdentityEqual `
            -First $settingsSourceIdentity `
            -Second $postRuntimeSettingsIdentity) -and
        (Test-FileIdentityEqual `
            -First $settingsSourceIdentity `
            -Second $postRetainedSettingsIdentity)
}
catch {
    $settingsStable = $false
}
try {
    $postSourceResourceDatabaseIdentity = Get-SourceResourceDatabaseIdentity `
        -CheckoutRoot $checkoutRoot
    $sourceResourceDatabaseStable = Test-FileIdentityEqual `
        -First $sourceResourceDatabaseIdentity `
        -Second $postSourceResourceDatabaseIdentity
}
catch {
    $postSourceResourceDatabaseIdentity = $null
    $sourceResourceDatabaseStable = $false
}
try {
    $postExecutableIdentity = Get-ExecutableIdentity -ExecutablePath $executablePath
    $executableStable =
        (Test-TrustedDiagnosticExecutableIdentity -Identity $postExecutableIdentity) -and
        (Test-ExecutableIdentityEqual `
            -First $executableIdentity `
            -Second $postExecutableIdentity)
}
catch {
    $postExecutableIdentity = $null
    $executableStable = $false
}
try {
    $mountAttestation = Get-SourceMountAttestation `
        -IsolationRoot $isolationRoot `
        -ExpectedProjectPath $projectPath `
        -ExpectedAddonGuid $addonGuid `
        -ExpectedResourceDatabasePath $sourceResourceDatabasePath `
        -ExpectedResourceDatabaseIdentity $sourceResourceDatabaseIdentity
}
catch {
    $mountAttestation = [pscustomobject][ordered]@{
        valid = $false
        problem = 'mount-attestation-exception'
        problemSha256 = Get-TextSha256 -Text $_.Exception.Message
    }
}
$artifactFinalizationProblemSha256 = ''
if ($completed -and $artifactPaths.Count -eq 3 -and
    $null -ne $artifactValidatorArguments) {
    try {
        $debugDirectory = Split-Path -Parent $artifactPaths[0]
        $finalArtifactFiles = @(Get-ChildItem `
            -LiteralPath $debugDirectory `
            -File `
            -Filter 'HST_CampaignDebug_*' `
            -Force `
            -ErrorAction Stop)
        $finalExpectedPaths = @($artifactPaths | ForEach-Object {
                [IO.Path]::GetFullPath($_)
            })
        $finalActualPaths = @($finalArtifactFiles | ForEach-Object {
                [IO.Path]::GetFullPath($_.FullName)
            })
        $artifactSetExact = Test-ExactStringSet `
            -Expected $finalExpectedPaths `
            -Actual $finalActualPaths
        $finalArtifactSignature = Get-FileSignature -Paths $artifactPaths
        $artifactBytesStable = $artifactSetExact -and
            $finalArtifactSignature -ceq $artifactSignature
        if (-not $artifactBytesStable) {
            throw 'Campaign Debug artifacts changed after their live stable poll.'
        }
        $artifactValidation = Test-CampaignDebugArtifacts `
            @artifactValidatorArguments
        $postValidationArtifactSignature = Get-FileSignature -Paths $artifactPaths
        if ($postValidationArtifactSignature -cne $finalArtifactSignature) {
            throw 'Campaign Debug artifacts changed during final validation.'
        }
    }
    catch {
        $artifactValidation = $null
        $artifactBytesStable = $false
        $artifactSetExact = $false
        $artifactFinalizationProblemSha256 = Get-TextSha256 -Text $_.Exception.Message
    }
}
else {
    $artifactValidation = $null
    $artifactBytesStable = $false
    $artifactSetExact = $false
}
try {
    $ambientErrorCensus = Get-SourceCampaignAmbientErrorCensus `
        -IsolationRoot $isolationRoot `
        -Profile $Profile
}
catch {
    $ambientErrorCensus = [pscustomobject][ordered]@{
        valid = $false
        problem = 'ambient-diagnostic-census-exception'
        problemSha256 = Get-TextSha256 -Text $_.Exception.Message
    }
}
if ($artifactValidation) {
    try {
        $errorCensus = Get-GuardErrorCensus `
            -GuardRoot $isolationRoot `
            -Profile $Profile `
            -IntentionalMissionConvoyAdmissionDiagnosticsProven `
                ([bool]$artifactValidation.IntentionalMissionConvoyAdmissionDiagnosticsProven) `
            -IntentionalMissionConvoySettlementDiagnosticProven `
                ([bool]$artifactValidation.IntentionalMissionConvoySettlementDiagnosticProven) `
            -IntentionalMissionConvoyCorruptionDiagnosticsProven `
                ([bool]$artifactValidation.IntentionalMissionConvoyCorruptionDiagnosticsProven) `
            -IntentionalMissionConvoyWatchdogDiagnosticProven `
                ([bool]$artifactValidation.IntentionalMissionConvoyWatchdogDiagnosticProven)
    }
    catch {
        $errorCensus = [pscustomobject][ordered]@{
            Valid = $false
            Problem = 'diagnostic-census-exception'
            ProblemSha256 = Get-TextSha256 -Text $_.Exception.Message
        }
    }
}

$processAxesPassed = $ownedRemaining -eq 0 -and $engineAfter.Count -eq 0 -and
    $unclaimedObserved.Count -eq 0 -and $cleanupErrors.Count -eq 0
$captureAxesPassed = [string]::IsNullOrWhiteSpace($runError) -and
    $completed -and $artifactSetExact -and $artifactBytesStable -and
    $sourceStable -and $sourceResourceDatabaseStable -and $settingsStable -and
    $executableStable -and $argumentTokensVerified -and $armed -and $started -and
    $processAxesPassed -and $mountAttestation -and [bool]$mountAttestation.valid -and
    $ambientErrorCensus -and [bool]$ambientErrorCensus.valid
$acceptance = $null
if ($artifactValidation -and $errorCensus -and $artifactPaths.Count -eq 3) {
    try {
        $acceptance = Get-SourceCampaignDebugAcceptance `
            -JsonPath $artifactPaths[0] `
            -Profile $Profile `
            -ArtifactValidation $artifactValidation `
            -ErrorCensus $errorCensus `
            -ClassifierChecks ([int]$classifierChecks) `
            -CaptureAxesPassed $captureAxesPassed
    }
    catch {
        $acceptance = [pscustomobject][ordered]@{
            accepted = $false
            disposition = 'rejected'
            correctedCanaryAccepted = $false
            proofCommonPassed = $false
            acceptedFull = $false
            acceptedInternal = $false
            diagnosticAxisPassed = $false
            captureAxesPassed = $captureAxesPassed
            redAxes = @('acceptance-evaluation')
        }
    }
}
else {
    $acceptance = [pscustomobject][ordered]@{
        accepted = $false
        disposition = 'rejected'
        correctedCanaryAccepted = $false
        proofCommonPassed = $false
        acceptedFull = $false
        acceptedInternal = $false
        diagnosticAxisPassed = $false
        captureAxesPassed = $captureAxesPassed
        redAxes = @('acceptance-inputs')
    }
}

$failureReasons = New-Object Collections.Generic.List[string]
if (-not [string]::IsNullOrWhiteSpace($runError)) {
    [void]$failureReasons.Add('runtime-error')
}
if (-not $completed) { [void]$failureReasons.Add('artifacts-not-complete') }
if (-not $artifactSetExact) { [void]$failureReasons.Add('artifact-set-not-exact') }
if (-not $artifactBytesStable) { [void]$failureReasons.Add('artifact-shutdown-drift') }
if (-not $artifactValidation -or -not [bool]$artifactValidation.Valid) {
    [void]$failureReasons.Add('artifact-contract')
}
if (-not $errorCensus -or -not [bool]$errorCensus.Valid) {
    [void]$failureReasons.Add('hard-diagnostic-census')
}
if (-not $ambientErrorCensus -or -not [bool]$ambientErrorCensus.valid) {
    [void]$failureReasons.Add('ambient-diagnostic-census')
}
if (-not $mountAttestation -or -not [bool]$mountAttestation.valid) {
    [void]$failureReasons.Add('source-mount-attestation')
}
if (-not $sourceStable) { [void]$failureReasons.Add('source-binding-drift') }
if (-not $sourceResourceDatabaseStable) {
    [void]$failureReasons.Add('source-resource-database-drift')
}
if (-not $settingsStable) { [void]$failureReasons.Add('settings-drift') }
if (-not $executableStable) { [void]$failureReasons.Add('executable-drift') }
if (-not $argumentTokensVerified) { [void]$failureReasons.Add('argument-vector') }
if (-not $armed) { [void]$failureReasons.Add('not-armed') }
if (-not $started) { [void]$failureReasons.Add('not-started') }
if ($ownedRemaining -ne 0 -or $engineAfter.Count -ne 0 -or
    $unclaimedObserved.Count -ne 0 -or $cleanupErrors.Count -ne 0) {
    [void]$failureReasons.Add('process-cleanup-census')
}
if (-not [bool]$acceptance.accepted) {
    foreach ($redAxis in @($acceptance.redAxes)) {
        [void]$failureReasons.Add('acceptance:' + [string]$redAxis)
    }
}
$success = $failureReasons.Count -eq 0
$endUtc = [DateTime]::UtcNow

$evidenceFiles = @(Get-EvidenceFileRows -RunRoot $runRoot)
$artifactSet = Get-ArtifactSetIdentity -FileRows $evidenceFiles
$evidenceFilesPath = Join-Path $runRoot 'evidence-files.json'
Write-PortableSummaryJson `
    -Path $evidenceFilesPath `
    -Value ([pscustomobject][ordered]@{
        schemaVersion = 1
        integrity = $artifactSet
    })
$evidenceFilesIdentity = Get-FileIdentity -Path $evidenceFilesPath
$portableExecutableBefore = [pscustomobject][ordered]@{
    fileName = Split-Path -Leaf $executablePath
    identity = ConvertTo-PortableFileIdentity -Identity $executableIdentity
    fileVersion = $executableIdentity.fileVersion
    productVersion = $executableIdentity.productVersion
    productName = $executableIdentity.productName
    companyName = $executableIdentity.companyName
    originalFilename = $executableIdentity.originalFilename
    signatureStatus = $executableIdentity.signatureStatus
    signerThumbprint = $executableIdentity.signerThumbprint
    signerSubject = $executableIdentity.signerSubject
}
$portableExecutableAfter = if ($postExecutableIdentity) {
    [pscustomobject][ordered]@{
        fileName = Split-Path -Leaf $executablePath
        identity = ConvertTo-PortableFileIdentity -Identity $postExecutableIdentity
        fileVersion = $postExecutableIdentity.fileVersion
        productVersion = $postExecutableIdentity.productVersion
        productName = $postExecutableIdentity.productName
        companyName = $postExecutableIdentity.companyName
        originalFilename = $postExecutableIdentity.originalFilename
        signatureStatus = $postExecutableIdentity.signatureStatus
        signerThumbprint = $postExecutableIdentity.signerThumbprint
        signerSubject = $postExecutableIdentity.signerSubject
    }
}
else {
    $null
}
$portableResourceDatabaseAfter = if ($postSourceResourceDatabaseIdentity) {
    ConvertTo-PortableFileIdentity -Identity $postSourceResourceDatabaseIdentity
}
else {
    $null
}
$portableSettingsAfter = if ($postRuntimeSettingsIdentity) {
    ConvertTo-PortableFileIdentity -Identity $postRuntimeSettingsIdentity
}
else {
    $null
}
$runId = if ($artifactValidation) { [string]$artifactValidation.RunId } else { '' }
$resultStatus = if (-not $success) {
    'failed'
}
elseif ([bool]$acceptance.acceptedFull) {
    'passed'
}
else {
    'passed-noncertifying'
}
$result = [pscustomobject][ordered]@{
    schemaVersion = 1
    evidenceKind = 'source-gate1-campaign-debug'
    source = $sourceSummary
    harness = $harnessSummary
    toolchain = [pscustomobject][ordered]@{
        runtimeExecutable = [pscustomobject][ordered]@{
            before = $portableExecutableBefore
            after = $portableExecutableAfter
            stable = $executableStable
        }
        sourceResourceDatabase = [pscustomobject][ordered]@{
            before = ConvertTo-PortableFileIdentity `
                -Identity $sourceResourceDatabaseIdentity
            after = $portableResourceDatabaseAfter
            stable = $sourceResourceDatabaseStable
        }
        classifier = [pscustomobject][ordered]@{
            path = $sourceBinding.classifier.relativePath
            gitObject = $sourceBinding.classifier.gitObject
            sha256 = $sourceBinding.classifier.gitBlobSha256
            hashPolicy = 'sha256-git-blob-bytes-v1'
            importedFunctionCount = $libraryBinding.importedFunctionCount
            selfTestChecks = [int]$classifierChecks
        }
        project = [pscustomobject][ordered]@{
            path = $sourceBinding.project.relativePath
            gitObject = $sourceBinding.project.gitObject
            sha256 = $sourceBinding.project.gitBlobSha256
            hashPolicy = 'sha256-git-blob-bytes-v1'
        }
    }
    capture = [pscustomobject][ordered]@{
        runId = $runId
        profile = $Profile
        startedUtc = $startUtc.ToString('o', [Globalization.CultureInfo]::InvariantCulture)
        completedUtc = $endUtc.ToString('o', [Globalization.CultureInfo]::InvariantCulture)
    }
    result = [pscustomobject][ordered]@{
        status = $resultStatus
        runtimeSeconds = [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
        sourceStable = $sourceStable
        sourceResourceDatabaseStable = $sourceResourceDatabaseStable
        settings = [pscustomobject][ordered]@{
            schemaVersion = [int]$settings.schemaVersion
            source = ConvertTo-PortableFileIdentity -Identity $settingsSourceIdentity
            retained = ConvertTo-PortableFileIdentity -Identity $retainedSettingsIdentity
            runtimeCopyBefore = ConvertTo-PortableFileIdentity -Identity $runtimeSettingsIdentity
            runtimeCopyAfter = $portableSettingsAfter
            stable = $settingsStable
        }
        launch = [pscustomobject][ordered]@{
            worldResource = $WorldResource
            arguments = $argumentVectorBinding
            argumentTokensVerified = $argumentTokensVerified
            armed = $armed
            started = $started
            artifactsStablePolls = $artifactStablePolls
            artifactSetExact = $artifactSetExact
            artifactBytesStableAfterShutdown = $artifactBytesStable
            liveStableSignatureSha256 = if ([string]::IsNullOrWhiteSpace(
                    $artifactSignature)) { '' } else {
                Get-TextSha256 -Text $artifactSignature
            }
            finalSignatureSha256 = if ([string]::IsNullOrWhiteSpace(
                    $finalArtifactSignature)) { '' } else {
                Get-TextSha256 -Text $finalArtifactSignature
            }
            postValidationSignatureSha256 = if ([string]::IsNullOrWhiteSpace(
                    $postValidationArtifactSignature)) { '' } else {
                Get-TextSha256 -Text $postValidationArtifactSignature
            }
            artifactFinalizationProblemSha256 = $artifactFinalizationProblemSha256
        }
        artifactValidation = $artifactValidation
        hardDiagnosticCensus = $errorCensus
        ambientDiagnosticCensus = $ambientErrorCensus
        acceptance = $acceptance
        mountAttestation = $mountAttestation
        processCensus = New-SourceCampaignProcessCensus `
            -BeforeProcesses $engineBefore `
            -RootProcessId $rootProcessId `
            -RootStartUtc $rootStartUtc `
            -MaximumOwnedProcesses $maximumOwnedProcesses `
            -OwnedProcessesRemaining $ownedRemaining `
            -UnclaimedEngineProcessesObserved @($unclaimedObserved) `
            -AfterProcesses $engineAfter `
            -CleanupErrors $cleanupErrors.ToArray()
        retainedArtifactPaths = @($artifactPaths | ForEach-Object {
                $full = [IO.Path]::GetFullPath($_)
                $prefix = [IO.Path]::GetFullPath($runRoot).TrimEnd('\', '/') +
                    [IO.Path]::DirectorySeparatorChar
                $full.Substring($prefix.Length).Replace('\', '/')
            })
        evidenceFilesManifest = [pscustomobject][ordered]@{
            path = 'evidence-files.json'
            length = $evidenceFilesIdentity.bytes
            sha256 = $evidenceFilesIdentity.sha256
        }
        runtimeErrorSha256 = if ([string]::IsNullOrWhiteSpace($runError)) {
            ''
        } else { Get-TextSha256 -Text $runError }
        failureReasons = $failureReasons.ToArray()
    }
    integrity = $artifactSet
}
$resultPath = Join-Path $runRoot 'result.json'
Write-PortableSummaryJson -Path $resultPath -Value $result

Write-Output ('RESULT ' + ([pscustomobject][ordered]@{
    status = $result.result.status
    profile = $Profile
    sourceHead = $sourceBinding.sourceHead
    publishInputSha256 = $sourceBinding.publishInput.sha256
    publishInputRows = $sourceBinding.publishInput.rowCount
    embeddedBuildSha = $sourceBinding.build.sha
    runtimeSeconds = $result.result.runtimeSeconds
    runId = $result.capture.runId
    evidenceRunLeaf = $runLeaf
    evidenceManifestSha256 = $evidenceFilesIdentity.sha256
    failureReasons = $failureReasons.ToArray()
} | ConvertTo-Json -Compress))

if (-not $success) {
    throw ('Source Campaign Debug failed closed: ' + ($failureReasons -join '; '))
}

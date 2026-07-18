[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeAddonRoot,

    [Parameter(Mandatory = $true)]
    [string]$WorkbenchExecutable,

    [string]$ProjectPath = "",

    [string]$WorldResource = "Worlds/HST_Everon/HST_Everon.ent",

    [string[]]$WatchedRoots = @(),

    [string[]]$SpillRoots = @(),

    [ValidateRange(30, 3600)]
    [int]$StageTimeoutSeconds = 600,

    [ValidateRange(30, 900)]
    [int]$PackTimeoutSeconds = 180,

    [ValidateRange(100, 5000)]
    [int]$PollMilliseconds = 500,

    [ValidateRange(1, 60)]
    [int]$ResultGraceSeconds = 5,

    [string]$ClientExecutable = "",

    [switch]$PreflightOnly,

    [switch]$LibraryOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:Stages = @(
    "autosave_checkpoint",
    "manual_checkpoint",
    "shutdown_checkpoint",
    "native_shutdown_verify",
    "profile_fallback_verify")
$script:StageOrdinals = @{
    autosave_checkpoint = 0
    manual_checkpoint = 1
    shutdown_checkpoint = 2
    native_shutdown_verify = 3
    profile_fallback_verify = 4
}
$script:ExpectedSources = @{
    autosave_checkpoint = "new_campaign"
    manual_checkpoint = "native"
    shutdown_checkpoint = "native"
    native_shutdown_verify = "native"
    profile_fallback_verify = "profile_fallback"
}
$script:ExpectedSaveTypes = @{
    autosave_checkpoint = "auto"
    manual_checkpoint = "manual"
    shutdown_checkpoint = "shutdown"
    native_shutdown_verify = "none"
    profile_fallback_verify = "none"
}
$script:ExpectedSaveNames = @{
    autosave_checkpoint = "Partisan autosave"
    manual_checkpoint = "Partisan manual checkpoint"
    shutdown_checkpoint = "Partisan controlled shutdown"
    native_shutdown_verify = ""
    profile_fallback_verify = ""
}
$script:ExpectedRequestFlags = @{
    autosave_checkpoint = 0
    manual_checkpoint = 0
    shutdown_checkpoint = 0
    native_shutdown_verify = 0
    profile_fallback_verify = 0
}
$script:ExpectedActiveSaveTypes = @{
    autosave_checkpoint = ""
    manual_checkpoint = "auto"
    shutdown_checkpoint = "manual"
    native_shutdown_verify = "shutdown"
    profile_fallback_verify = ""
}
$script:ExpectedActiveSaveNames = @{
    autosave_checkpoint = ""
    manual_checkpoint = "Partisan autosave"
    shutdown_checkpoint = "Partisan manual checkpoint"
    native_shutdown_verify = "Partisan controlled shutdown"
    profile_fallback_verify = ""
}
$script:ExpectedSentinelGenerations = @{
    autosave_checkpoint = 1
    manual_checkpoint = 2
    shutdown_checkpoint = 3
    native_shutdown_verify = 3
    profile_fallback_verify = 3
}
$script:ExpectedSourceJournalGenerations = @{
    autosave_checkpoint = -1
    manual_checkpoint = 1
    shutdown_checkpoint = 2
    native_shutdown_verify = 3
    profile_fallback_verify = 3
}
$script:ExpectedSourceJournalSlots = @{
    autosave_checkpoint = ""
    manual_checkpoint = "canonical"
    shutdown_checkpoint = "recovery"
    native_shutdown_verify = "canonical"
    profile_fallback_verify = "canonical"
}
$script:ExpectedSourceJournalValidSlotCounts = @{
    autosave_checkpoint = 0
    manual_checkpoint = 1
    shutdown_checkpoint = 2
    native_shutdown_verify = 2
    profile_fallback_verify = 2
}
$script:MissionHeader = "Missions/HST_Everon.conf"
$script:WorldSystemsConfig =
    "Configs/HST/Persistence/HST_CampaignSystems.conf"
$script:ProjectId = "698532771130111D"
$script:OwnerMagic = "partisan_ordinary_campaign_persistence_owner_v1"
$script:GuardMagic = "partisan_ordinary_campaign_persistence_guard_v1"
$script:CarrierMagic = "partisan_ordinary_campaign_persistence_carrier_v1"
$script:ResultMagic = "partisan_ordinary_campaign_persistence_result_v1"
$script:EndBridgeReceiptMagic =
    "partisan_ordinary_campaign_end_bridge_receipt_v1"
$script:MixedNativeReadyMagic =
    "partisan_ordinary_campaign_mixed_native_ready_v1"
$script:OwnerPurpose = "ordinary_campaign_persistence"
$script:AuthorityVersion = 1
$script:SentinelVersion = 1
$script:HSTAutosaveSchedulerIntervalSeconds = 60
$script:HSTAutosaveSchedulerDebounceSeconds = 120
$script:HSTAutosaveSchedulerRemarkSeconds = 30
$script:FieldVehiclePrefab =
    "{4AE9D080927D3CB9}Prefabs/Vehicles/Wheeled/S1203/S1203_base.et"
$script:MixedNativeCarrierPrefab =
    "{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et"
$script:FieldVehicleCargoPrefab =
    "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et"
$script:ExpectedFieldVehicleResults = @{
    autosave_checkpoint = @{
        Phase = "prepare"
        DurableRows = 2
        LiveRoots = 2
        DeletedRows = 0
        CargoRows = 2
        RestoreEligibleRows = 0
        RestoreInactiveRows = 0
        RestoreRoots = 0
        RestoreTrackedRoots = 0
        ShutdownQuiescedRoots = 0
        MutationApplied = $false
    }
    manual_checkpoint = @{
        Phase = "recover_mutate"
        DurableRows = 2
        LiveRoots = 1
        DeletedRows = 1
        CargoRows = 2
        RestoreEligibleRows = 2
        RestoreInactiveRows = 0
        RestoreRoots = 2
        RestoreTrackedRoots = 2
        ShutdownQuiescedRoots = 0
        MutationApplied = $true
    }
    shutdown_checkpoint = @{
        Phase = "replay_shutdown"
        DurableRows = 2
        LiveRoots = 1
        DeletedRows = 1
        CargoRows = 2
        RestoreEligibleRows = 1
        RestoreInactiveRows = 1
        RestoreRoots = 1
        RestoreTrackedRoots = 1
        ShutdownQuiescedRoots = 1
        MutationApplied = $false
    }
    native_shutdown_verify = @{
        Phase = "replay_native"
        DurableRows = 2
        LiveRoots = 1
        DeletedRows = 1
        CargoRows = 2
        RestoreEligibleRows = 1
        RestoreInactiveRows = 1
        RestoreRoots = 1
        RestoreTrackedRoots = 1
        ShutdownQuiescedRoots = 0
        MutationApplied = $false
    }
    profile_fallback_verify = @{
        Phase = "replay_fallback"
        DurableRows = 2
        LiveRoots = 1
        DeletedRows = 1
        CargoRows = 2
        RestoreEligibleRows = 1
        RestoreInactiveRows = 1
        RestoreRoots = 1
        RestoreTrackedRoots = 1
        ShutdownQuiescedRoots = 0
        MutationApplied = $false
    }
}
$script:GuardBaseLeaf = "PartisanOrdinaryCampaignPersistence"
$script:GuardLeafPrefix = "PartisanOrdinaryCampaignPersistenceGuard_"
$script:GuardSentinelLeaf = ".partisan-ordinary-persistence-owner"
$script:WorkspacePackScratchLeaf = ".tmp-native-pack"
$script:WorkspacePackScratchSentinelLeaf = ".partisan-native-pack-owner"
$script:MutexName = "Local\PartisanOrdinaryCampaignPersistenceGuard"
$script:SnapshotHashMaximumBytes = 65536

if (-not ("PartisanOrdinaryPersistenceJob" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public sealed class PartisanOrdinaryPersistenceJob : IDisposable
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

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(
        UInt32 desiredAccess,
        bool inheritHandle,
        Int32 processId);

    public PartisanOrdinaryPersistenceJob()
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
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to assign process to guarded job.");
    }

    public void AddById(Int32 processId)
    {
        const UInt32 PROCESS_TERMINATE = 0x0001;
        const UInt32 PROCESS_SET_QUOTA = 0x0100;
        IntPtr processHandle = OpenProcess(
            PROCESS_TERMINATE | PROCESS_SET_QUOTA,
            false,
            processId);
        if (processHandle == IntPtr.Zero)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to open process for guarded job assignment.");
        try
        {
            if (!AssignProcessToJobObject(_handle, processHandle))
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to assign process to guarded job.");
        }
        finally
        {
            CloseHandle(processHandle);
        }
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
                    throw new InvalidOperationException("Unable to query guarded process job.");
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
            "Guarded process job membership exceeded its safety limit.");
    }

    public void Dispose()
    {
        if (_handle == IntPtr.Zero)
            return;
        CloseHandle(_handle);
        _handle = IntPtr.Zero;
    }
}

public static class PartisanWindowsCommandLine
{
    [DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern IntPtr CommandLineToArgvW(
        string commandLine,
        out Int32 argumentCount);

    [DllImport("kernel32.dll")]
    private static extern IntPtr LocalFree(IntPtr memory);

    public static string[] Split(string commandLine)
    {
        if (String.IsNullOrWhiteSpace(commandLine))
            return new string[0];

        Int32 argumentCount;
        IntPtr argumentVector = CommandLineToArgvW(
            commandLine,
            out argumentCount);
        if (argumentVector == IntPtr.Zero)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to parse a native Windows command line.");

        try
        {
            string[] result = new string[argumentCount];
            for (Int32 index = 0; index < argumentCount; index++)
            {
                IntPtr argument = Marshal.ReadIntPtr(
                    argumentVector,
                    index * IntPtr.Size);
                result[index] = Marshal.PtrToStringUni(argument);
            }
            return result;
        }
        finally
        {
            LocalFree(argumentVector);
        }
    }
}

public static class PartisanProcessInspection
{
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(
        UInt32 desiredAccess,
        bool inheritHandle,
        Int32 processId);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool QueryFullProcessImageName(
        IntPtr process,
        UInt32 flags,
        StringBuilder executablePath,
        ref UInt32 characterCount);

    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr handle);

    public static string QueryImagePath(Int32 processId)
    {
        const UInt32 PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;
        IntPtr processHandle = OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION,
            false,
            processId);
        if (processHandle == IntPtr.Zero)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to open process for limited image inspection.");
        try
        {
            UInt32 capacity = 32768;
            StringBuilder executablePath = new StringBuilder((Int32)capacity);
            if (!QueryFullProcessImageName(
                processHandle,
                0,
                executablePath,
                ref capacity))
            {
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to query process image path.");
            }
            return executablePath.ToString();
        }
        finally
        {
            CloseHandle(processHandle);
        }
    }
}

public sealed class PartisanOrdinaryPersistenceSuspendedProcess : IDisposable
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

    public PartisanOrdinaryPersistenceSuspendedProcess(
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
                "Unable to create suspended guarded process.");
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
                "Suspended guarded process thread is unavailable.");
        if (ResumeThread(_threadHandle) == UInt32.MaxValue)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to resume guarded process.");
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
"@
}

function Split-GuardedNativeCommandLine {
    param([Parameter(Mandatory = $true)][string]$CommandLine)

    # Use Windows' own parser because Steam may legally rewrite equivalent
    # quote and backslash forms when it creates the game process.
    return [PartisanWindowsCommandLine]::Split($CommandLine)
}

function Resolve-ExistingPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]
        [ValidateSet("Leaf", "Container")][string]$Kind
    )

    if (-not (Test-Path -LiteralPath $Path -PathType $Kind)) {
        throw "A required $Kind path is unavailable."
    }
    return [IO.Path]::GetFullPath(
        (Get-Item -LiteralPath $Path -Force).FullName)
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

    $tokens = @(Split-GuardedNativeCommandLine -CommandLine $CommandLine)
    if ($tokens.Count -ne ($ExpectedArguments.Count + 1)) {
        return $false
    }
    try {
        $actualExecutable = [IO.Path]::GetFullPath($tokens[0])
        $expectedExecutable = [IO.Path]::GetFullPath($ExpectedExecutable)
    }
    catch {
        return $false
    }
    if (-not $actualExecutable.Equals(
        $expectedExecutable,
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

function Read-SharedFileText {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [int]$MaximumBytes = 16777216
    )

    $stream = [IO.File]::Open(
        $Path,
        [IO.FileMode]::Open,
        [IO.FileAccess]::Read,
        [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
    try {
        if ($stream.Length -gt $MaximumBytes) {
            throw "A guarded evidence file exceeded its size limit."
        }
        $reader = New-Object IO.StreamReader(
            $stream, [Text.Encoding]::UTF8, $true, 4096, $true)
        try {
            return $reader.ReadToEnd()
        }
        finally {
            $reader.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
}

function Read-JsonArtifact {
    param([Parameter(Mandatory = $true)][string]$Path)

    try {
        return (Read-SharedFileText -Path $Path | ConvertFrom-Json)
    }
    catch {
        throw "A guarded JSON artifact could not be read and parsed."
    }
}

function Write-JsonUtf8NoBom {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $encoding = New-Object Text.UTF8Encoding($false)
    [IO.File]::WriteAllText(
        $Path,
        ($Value | ConvertTo-Json -Depth 16),
        $encoding)
}

function Assert-JsonProperty {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$PropertyName,
        [Parameter(Mandatory = $true)][string]$ArtifactLabel
    )

    if ($null -eq $Value -or
        $Value.PSObject.Properties.Name -notcontains $PropertyName) {
        throw "$ArtifactLabel is missing required property $PropertyName."
    }
}

function Get-FieldVehicleVectorSignature {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$ArtifactLabel
    )

    $components = New-Object Collections.Generic.List[double]
    if ($Value -is [string]) {
        $matches = [regex]::Matches(
            [string]$Value,
            '[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?')
        if ($matches.Count -ne 3) {
            throw "$ArtifactLabel does not contain an exact three-component vector."
        }
        foreach ($match in $matches) {
            try {
                $components.Add([double]::Parse(
                    $match.Value,
                    [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture))
            }
            catch {
                throw "$ArtifactLabel contains an invalid vector component."
            }
        }
    }
    elseif ($Value -is [array]) {
        if ($Value.Count -ne 3) {
            throw "$ArtifactLabel does not contain an exact three-component vector."
        }
        foreach ($component in $Value) {
            try {
                $components.Add([Convert]::ToDouble(
                    $component,
                    [Globalization.CultureInfo]::InvariantCulture))
            }
            catch {
                throw "$ArtifactLabel contains an invalid vector component."
            }
        }
    }
    else {
        foreach ($axis in @("x", "y", "z")) {
            $property = $Value.PSObject.Properties[$axis]
            if (-not $property) {
                throw "$ArtifactLabel does not contain an exact three-component vector."
            }
            try {
                $components.Add([Convert]::ToDouble(
                    $property.Value,
                    [Globalization.CultureInfo]::InvariantCulture))
            }
            catch {
                throw "$ArtifactLabel contains an invalid vector component."
            }
        }
    }

    foreach ($component in $components) {
        if ([double]::IsNaN($component) -or
            [double]::IsInfinity($component)) {
            throw "$ArtifactLabel contains a non-finite vector component."
        }
    }
    $signatureParts = foreach ($component in $components) {
        $component.ToString(
            "R",
            [Globalization.CultureInfo]::InvariantCulture)
    }
    return [pscustomobject]@{
        IsZero = $components[0] -eq 0.0 -and
            $components[1] -eq 0.0 -and
            $components[2] -eq 0.0
        Signature = $signatureParts -join ","
    }
}

function Read-CheckoutBuildIdentity {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)

    $buildInfoPath = Join-Path $RepositoryRoot "Scripts\Game\HST\HST_BuildInfo.c"
    $campaignStatePath = Join-Path $RepositoryRoot (
        "Scripts\Game\HST\State\HST_CampaignState.c")
    $runtimeSettingsPath = Join-Path $RepositoryRoot (
        "Scripts\Game\HST\Config\HST_RuntimeSettings.c")
    $buildInfoText = Read-SharedFileText -Path $buildInfoPath
    $campaignStateText = Read-SharedFileText -Path $campaignStatePath
    $runtimeSettingsText = Read-SharedFileText -Path $runtimeSettingsPath
    $shaMatch = [regex]::Match(
        $buildInfoText,
        'static\s+const\s+string\s+BUILD_SHA\s*=\s*"([^"]+)"')
    $utcMatch = [regex]::Match(
        $buildInfoText,
        'static\s+const\s+string\s+BUILD_UTC\s*=\s*"([^"]+)"')
    $labelMatch = [regex]::Match(
        $buildInfoText,
        'static\s+const\s+string\s+BUILD_LABEL\s*=\s*"([^"]+)"')
    $schemaMatch = [regex]::Match(
        $campaignStateText,
        'static\s+const\s+int\s+SCHEMA_VERSION\s*=\s*([0-9]+)')
    $settingsSchemaMatch = [regex]::Match(
        $runtimeSettingsText,
        'static\s+const\s+int\s+SCHEMA_VERSION\s*=\s*([0-9]+)')
    if (-not $shaMatch.Success -or -not $utcMatch.Success -or
        -not $labelMatch.Success -or -not $schemaMatch.Success -or
        -not $settingsSchemaMatch.Success -or
        $shaMatch.Groups[1].Value -notmatch '^[0-9a-f]{40}$') {
        throw "Checkout build identity constants could not be parsed."
    }
    return [pscustomobject]@{
        BuildSha = $shaMatch.Groups[1].Value
        BuildUtc = $utcMatch.Groups[1].Value
        BuildLabel = $labelMatch.Groups[1].Value
        CampaignSchemaVersion = [int]$schemaMatch.Groups[1].Value
        SettingsSchemaVersion = [int]$settingsSchemaMatch.Groups[1].Value
    }
}

function Assert-BuildIdentity {
    param(
        [Parameter(Mandatory = $true)]$Artifact,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ArtifactLabel
    )

    foreach ($property in @(
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion")) {
        Assert-JsonProperty $Artifact $property $ArtifactLabel
    }
    if ([string]$Artifact.m_sBuildSha -cne $ExpectedBuild.BuildSha -or
        [string]$Artifact.m_sBuildUtc -cne $ExpectedBuild.BuildUtc -or
        [string]$Artifact.m_sBuildLabel -cne $ExpectedBuild.BuildLabel -or
        [int]$Artifact.m_iCampaignSchemaVersion -ne
            $ExpectedBuild.CampaignSchemaVersion -or
        [int]$Artifact.m_iSettingsSchemaVersion -ne
            $ExpectedBuild.SettingsSchemaVersion) {
        throw "$ArtifactLabel build provenance does not match this checkout."
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
    if ($AllowEqual -and $candidateFull.Equals(
        $rootFull, [StringComparison]::OrdinalIgnoreCase)) {
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
            throw "A safe existing ancestor could not be resolved."
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

function Assert-NoReparseDescendant {
    param([Parameter(Mandatory = $true)][string]$Root)

    Assert-NoReparsePathAncestry -Path $Root
    $found = Get-ChildItem -LiteralPath $Root -Recurse -Force -ErrorAction Stop |
        Where-Object {
            ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
        } |
        Select-Object -First 1
    if ($found) {
        throw "A guarded tree contains a reparse point."
    }
}

function Get-FileSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-FileSignature {
    param([Parameter(Mandatory = $true)][string]$Path)

    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    return "$($item.Length):$(Get-FileSha256 -Path $Path)"
}

function Get-CampaignJournalFileState {
    param(
        [Parameter(Mandatory = $true)][string]$CanonicalPath,
        [Parameter(Mandatory = $true)][string]$RecoveryPath
    )

    $canonicalPresent = Test-Path -LiteralPath $CanonicalPath -PathType Leaf
    $recoveryPresent = Test-Path -LiteralPath $RecoveryPath -PathType Leaf
    $canonicalSignature = ""
    $recoverySignature = ""
    if ($canonicalPresent) {
        $canonicalSignature = Get-FileSignature -Path $CanonicalPath
    }
    if ($recoveryPresent) {
        $recoverySignature = Get-FileSignature -Path $RecoveryPath
    }
    return [pscustomobject]@{
        CanonicalPresent = $canonicalPresent
        CanonicalSignature = $canonicalSignature
        RecoveryPresent = $recoveryPresent
        RecoverySignature = $recoverySignature
        FileCount = [int]$canonicalPresent + [int]$recoveryPresent
    }
}

function Test-CampaignJournalFileStateExact {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    return [bool]$Expected.CanonicalPresent -eq
            [bool]$Actual.CanonicalPresent -and
        [string]$Expected.CanonicalSignature -ceq
            [string]$Actual.CanonicalSignature -and
        [bool]$Expected.RecoveryPresent -eq
            [bool]$Actual.RecoveryPresent -and
        [string]$Expected.RecoverySignature -ceq
            [string]$Actual.RecoverySignature
}

function Get-SnapshotEntries {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$ExcludedRoots = @()
    )

    $rootFull = [IO.Path]::GetFullPath($Root)
    Assert-NoReparseDescendant -Root $rootFull
    $entries = @{}
    foreach ($item in @(
        Get-ChildItem -LiteralPath $rootFull -Recurse -Force -ErrorAction Stop)) {
        $itemFull = [IO.Path]::GetFullPath($item.FullName)
        $excluded = $false
        foreach ($excludedRoot in $ExcludedRoots) {
            if (Test-ContainedPath $excludedRoot $itemFull -AllowEqual) {
                $excluded = $true
                break
            }
        }
        if ($excluded) {
            continue
        }
        $signature = if ($item.PSIsContainer) {
            "D"
        }
        elseif ($item.Length -le $script:SnapshotHashMaximumBytes) {
            "F:$($item.Length):$(Get-FileSha256 -Path $itemFull)"
        }
        else {
            "F:$($item.Length):$($item.LastWriteTimeUtc.Ticks)"
        }
        $entries[$itemFull] = $signature
    }
    return $entries
}

function New-RootSnapshot {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$ExcludedRoots = @()
    )

    $rootFull = Resolve-ExistingPath -Path $Root -Kind Container
    $resolvedExclusions = @($ExcludedRoots | ForEach-Object {
        [IO.Path]::GetFullPath($_)
    })
    return [pscustomobject]@{
        Root = $rootFull
        ExcludedRoots = $resolvedExclusions
        Entries = Get-SnapshotEntries $rootFull $resolvedExclusions
    }
}

function Get-CheckoutWatchExclusions {
	param(
		[Parameter(Mandatory = $true)][string]$RepositoryRoot,
		[Parameter(Mandatory = $true)][string]$WatchRoot
	)

	$repositoryFull = [IO.Path]::GetFullPath($RepositoryRoot)
	$watchFull = [IO.Path]::GetFullPath($WatchRoot)
	if (-not (Test-ContainedPath `
		-Root $watchFull `
		-Candidate $repositoryFull `
		-AllowEqual)) {
		return @()
	}

	# The IDE may refresh Git metadata while Workbench refreshes these two
	# ignored root caches during a native pack. They are neither mod source nor
	# retained proof output. Keep this list exact rather than honoring every
	# ignored path, so all real checkout files remain under the cleanup guard.
	return @(
		(Join-Path $repositoryFull ".git"),
		(Join-Path $repositoryFull "UserMaps.desc"),
		(Join-Path $repositoryFull "resourceDatabase.rdb"))
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
    $current = Get-SnapshotEntries $Snapshot.Root $Snapshot.ExcludedRoots
    $newEntries = 0
    $modifiedFiles = 0
    $deletedEntries = 0
    foreach ($path in $current.Keys) {
        if (-not $Snapshot.Entries.ContainsKey($path)) {
            $newEntries++
        }
        elseif ([string]$current[$path] -cne [string]$Snapshot.Entries[$path]) {
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

function Assert-LowerHexNonce {
    param(
        [Parameter(Mandatory = $true)][string]$Nonce,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Nonce -cnotmatch '^[0-9a-f]{32}$') {
        throw "$Label must be one lowercase 32-character nonce."
    }
}

function Assert-NativeSavePointId {
    param(
        [Parameter(Mandatory = $true)][string]$SavePointId,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($SavePointId -cnotmatch
        '^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$') {
        throw "$Label must be one lowercase native save UUID."
    }
    $parsed = [Guid]::Empty
    if (-not [Guid]::TryParseExact($SavePointId, "D", [ref]$parsed) -or
        $parsed -eq [Guid]::Empty) {
        throw "$Label is not a valid non-empty native save UUID."
    }
}

function Get-EngineProcessRows {
    $names = @(
        "ArmaReforger",
        "ArmaReforger_BE",
        "ArmaReforgerSteam",
        "ArmaReforgerSteamDiag",
        "ArmaReforgerServer",
        "ArmaReforgerServerDiag",
        "ArmaReforgerWorkbench",
        "ArmaReforgerWorkbenchDiag",
        "ArmaReforgerWorkbenchSteamDiag",
        "CrashReporter")
    return @(Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $names -contains $_.ProcessName })
}

function Test-ProcessIdentityAlive {
    param(
        [Parameter(Mandatory = $true)][int]$ProcessId,
        [Parameter(Mandatory = $true)][datetime]$StartUtc
    )

    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $process) {
        return $false
    }
    try {
        return $process.StartTime.ToUniversalTime().Ticks -eq $StartUtc.Ticks
    }
    catch {
        return $true
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
        if ($Owned.ContainsKey($candidateId) -and
            (Test-ProcessIdentityAlive `
                -ProcessId $candidateId `
                -StartUtc $Owned[$candidateId])) {
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
            if ($Owned.ContainsKey($candidateId) -and
                (Test-ProcessIdentityAlive `
                    -ProcessId $candidateId `
                    -StartUtc $Owned[$candidateId])) {
                continue
            }
            if (-not $Owned.ContainsKey($parentId) -or
                -not (Test-ProcessIdentityAlive `
                    -ProcessId $parentId `
                    -StartUtc $Owned[$parentId])) {
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
    if (-not $Owned.ContainsKey($RootProcessId) -and
        (Test-ProcessIdentityAlive $RootProcessId $RootStartUtc)) {
        $Owned[$RootProcessId] = $RootStartUtc
    }
}

function Get-LiveOwnedProcessCount {
    param([Parameter(Mandatory = $true)][hashtable]$Owned)

    $count = 0
    foreach ($processId in @($Owned.Keys)) {
        if (Test-ProcessIdentityAlive ([int]$processId) $Owned[$processId]) {
            $count++
        }
    }
    return $count
}

function Stop-OwnedProcesses {
    param([Parameter(Mandatory = $true)][hashtable]$Owned)

    foreach ($processId in @($Owned.Keys)) {
        try {
            $process = Get-Process -Id ([int]$processId) -ErrorAction Stop
            if ($process.StartTime.ToUniversalTime().Ticks -eq
                $Owned[$processId].Ticks) {
                $process.Kill()
            }
        }
        catch { }
    }
}

function Wait-StableJsonArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][datetime]$DeadlineUtc,
        [int]$StablePolls = 2
    )

    $lastSignature = ""
    $same = 0
    while ([DateTime]::UtcNow -lt $DeadlineUtc) {
        if (Test-Path -LiteralPath $Path -PathType Leaf) {
            try {
                $signature = Get-FileSignature -Path $Path
                if ($signature -ceq $lastSignature) {
                    $same++
                }
                else {
                    $lastSignature = $signature
                    $same = 1
                }
                if ($same -ge $StablePolls) {
                    return Read-JsonArtifact -Path $Path
                }
            }
            catch {
                $lastSignature = ""
                $same = 0
            }
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    throw "A stable guarded JSON artifact did not arrive before its deadline."
}

function Read-GuardOwnership {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$GuardBase
    )

    try {
        $guardBaseFull = [IO.Path]::GetFullPath($GuardBase).TrimEnd('\', '/')
        if ((Split-Path -Leaf $guardBaseFull) -cne $script:GuardBaseLeaf) {
            return $null
        }
        $directoryFull = [IO.Path]::GetFullPath($Directory)
        if (-not (Test-ContainedPath $guardBaseFull $directoryFull)) {
            return $null
        }
        if (-not ([IO.Path]::GetFullPath(
            (Split-Path -Parent $directoryFull)).TrimEnd('\', '/')).Equals(
                $guardBaseFull,
                [StringComparison]::OrdinalIgnoreCase)) {
            return $null
        }
        $leaf = Split-Path -Leaf $directoryFull
        $pattern = '^' + [regex]::Escape($script:GuardLeafPrefix) +
            '([0-9a-f]{32})$'
        if ($leaf -notmatch $pattern -or
            -not (Test-Path -LiteralPath $directoryFull -PathType Container)) {
            return $null
        }
        Assert-NoReparsePathAncestry -Path $directoryFull
        $sentinel = Join-Path $directoryFull $script:GuardSentinelLeaf
        if (-not (Test-Path -LiteralPath $sentinel -PathType Leaf)) {
            return $null
        }
        $value = Read-JsonArtifact -Path $sentinel
        foreach ($property in @(
            "version",
            "purpose",
            "nonce",
            "guardLeaf",
            "ownerPid",
            "ownerStartUtc")) {
            if ($value.PSObject.Properties.Name -notcontains $property) {
                return $null
            }
        }
        if ([int]$value.version -ne $script:SentinelVersion -or
            [string]$value.purpose -cne $script:OwnerPurpose -or
            [string]$value.nonce -cne [string]$matches[1] -or
            [string]$value.guardLeaf -cne $leaf -or
            [int]$value.ownerPid -le 0) {
            return $null
        }
        $startUtc = [DateTime]::Parse(
            [string]$value.ownerStartUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        return [pscustomobject]@{
            Directory = $directoryFull
            Sentinel = $sentinel
            Nonce = [string]$value.nonce
            OwnerPid = [int]$value.ownerPid
            OwnerStartUtc = $startUtc
        }
    }
    catch {
        return $null
    }
}

function Remove-ExactOwnedGuard {
    param(
        [Parameter(Mandatory = $true)]$Ownership,
        [Parameter(Mandatory = $true)][string]$GuardBase
    )

    for ($attempt = 1; $attempt -le 4; $attempt++) {
        if (-not (Test-Path -LiteralPath $Ownership.Directory)) {
            return $true
        }
        $current = Read-GuardOwnership $Ownership.Directory $GuardBase
        if (-not $current -or
            $current.Nonce -cne $Ownership.Nonce -or
            $current.OwnerPid -ne $Ownership.OwnerPid -or
            $current.OwnerStartUtc.Ticks -ne $Ownership.OwnerStartUtc.Ticks) {
            return $false
        }
        Assert-NoReparseDescendant -Root $current.Directory
        $sentinelBytes = [IO.File]::ReadAllBytes($current.Sentinel)
        try {
            foreach ($child in @(Get-ChildItem `
                -LiteralPath $current.Directory `
                -Force `
                -ErrorAction Stop)) {
                $childFull = [IO.Path]::GetFullPath($child.FullName)
                if ($childFull.Equals(
                    [IO.Path]::GetFullPath($current.Sentinel),
                    [StringComparison]::OrdinalIgnoreCase)) {
                    continue
                }
                if (-not (Test-ContainedPath `
                    -Root $current.Directory `
                    -Candidate $childFull)) {
                    throw "A guard descendant escaped its owned root."
                }
                Remove-Item `
                    -LiteralPath $childFull `
                    -Recurse `
                    -Force `
                    -ErrorAction Stop
            }

            $final = Read-GuardOwnership `
                -Directory $Ownership.Directory `
                -GuardBase $GuardBase
            if (-not $final -or
                $final.Nonce -cne $Ownership.Nonce -or
                $final.OwnerPid -ne $Ownership.OwnerPid -or
                $final.OwnerStartUtc.Ticks -ne
                    $Ownership.OwnerStartUtc.Ticks) {
                throw "Guard ownership changed during cleanup."
            }
            $remaining = @(Get-ChildItem `
                -LiteralPath $final.Directory `
                -Force `
                -ErrorAction Stop)
            if ($remaining.Count -ne 1 -or
                -not ([IO.Path]::GetFullPath($remaining[0].FullName)).Equals(
                    [IO.Path]::GetFullPath($final.Sentinel),
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw "Guard cleanup did not isolate its ownership sentinel."
            }

            Remove-Item `
                -LiteralPath $final.Sentinel `
                -Force `
                -ErrorAction Stop
            try {
                Remove-Item `
                    -LiteralPath $final.Directory `
                    -Force `
                    -ErrorAction Stop
            }
            catch {
                if ((Test-Path `
                        -LiteralPath $final.Directory `
                        -PathType Container) -and
                    -not (Test-Path -LiteralPath $final.Sentinel)) {
                    Assert-NoReparseDescendant -Root $final.Directory
                    [IO.File]::WriteAllBytes(
                        $final.Sentinel,
                        $sentinelBytes)
                }
                throw
            }
        }
        catch {
            if ($attempt -lt 4) {
                Start-Sleep -Milliseconds 250
            }
        }
    }
    return -not (Test-Path -LiteralPath $Ownership.Directory)
}

function Read-WorkspacePackScratchOwnership {
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot
    )

    try {
        $repositoryFull = [IO.Path]::GetFullPath($RepositoryRoot)
        $expected = [IO.Path]::GetFullPath(
            (Join-Path $repositoryFull $script:WorkspacePackScratchLeaf))
        $resolved = [IO.Path]::GetFullPath($Directory)
        if (-not $resolved.Equals(
                $expected,
                [StringComparison]::OrdinalIgnoreCase) -or
            -not (Test-ContainedPath `
                -Root $repositoryFull `
                -Candidate $resolved) -or
            -not (Test-Path -LiteralPath $resolved -PathType Container)) {
            return $null
        }
        Assert-NoReparsePathAncestry -Path $resolved
        $directoryItem = Get-Item -LiteralPath $resolved -Force
        if (($directoryItem.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }
        $sentinel = Join-Path `
            $resolved `
            $script:WorkspacePackScratchSentinelLeaf
        if (-not (Test-Path -LiteralPath $sentinel -PathType Leaf)) {
            return $null
        }
        $sentinelItem = Get-Item -LiteralPath $sentinel -Force
        if (($sentinelItem.Attributes -band
            [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }
        $value = Read-JsonArtifact -Path $sentinel
        foreach ($property in @(
            "version",
            "purpose",
            "nonce",
            "ownerPid",
            "ownerStartUtc")) {
            if ($value.PSObject.Properties.Name -notcontains $property) {
                return $null
            }
        }
        if ([int]$value.version -ne $script:SentinelVersion -or
            [string]$value.purpose -cne "native_workbench_pack_scratch" -or
            [string]$value.nonce -cnotmatch '^[0-9a-f]{32}$' -or
            [int]$value.ownerPid -le 0) {
            return $null
        }
        $startUtc = [DateTime]::Parse(
            [string]$value.ownerStartUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        return [pscustomobject]@{
            Directory = $resolved
            Sentinel = $sentinel
            Nonce = [string]$value.nonce
            OwnerPid = [int]$value.ownerPid
            OwnerStartUtc = $startUtc
        }
    }
    catch {
        return $null
    }
}

function Remove-WorkspacePackScratch {
    param(
        [Parameter(Mandatory = $true)]$Ownership,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot
    )

    for ($attempt = 1; $attempt -le 4; $attempt++) {
        if (-not (Test-Path -LiteralPath $Ownership.Directory)) {
            return $true
        }
        $current = Read-WorkspacePackScratchOwnership `
            -Directory $Ownership.Directory `
            -RepositoryRoot $RepositoryRoot
        if (-not $current -or
            $current.Nonce -cne $Ownership.Nonce -or
            $current.OwnerPid -ne $Ownership.OwnerPid -or
            $current.OwnerStartUtc.Ticks -ne $Ownership.OwnerStartUtc.Ticks) {
            return $false
        }
        Assert-NoReparseDescendant -Root $current.Directory
        try {
            Remove-Item -LiteralPath $current.Directory -Recurse -Force `
                -ErrorAction Stop
        }
        catch {
            if ($attempt -lt 4) {
                Start-Sleep -Milliseconds 250
            }
        }
    }
    return -not (Test-Path -LiteralPath $Ownership.Directory)
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
        $replacements.Add([pscustomobject]@{
            Value = $GuardRoot
            Label = "<guard>"
        })
    }
    if (-not [string]::IsNullOrWhiteSpace($ProjectDirectory)) {
        $replacements.Add([pscustomobject]@{
            Value = $ProjectDirectory
            Label = "<project>"
        })
    }
    foreach ($root in $ResolvedAddonRoots) {
        if (-not [string]::IsNullOrWhiteSpace($root)) {
            $replacements.Add([pscustomobject]@{
                Value = $root
                Label = "<addon>"
            })
        }
    }
    foreach ($replacement in @(
        $replacements.ToArray() | Sort-Object { $_.Value.Length } -Descending)) {
        $safe = $safe.Replace($replacement.Value, $replacement.Label)
        $safe = $safe.Replace(
            $replacement.Value.Replace('\', '/'),
            $replacement.Label)
    }
    $safe = [regex]::Replace(
        $safe,
        '(?i)(["''])\\\\[^"'']+\1',
        '$1<path>$1')
    $safe = [regex]::Replace($safe, '(?i)\\\\[^\s;,)]+', '<path>')
    $safe = [regex]::Replace(
        $safe,
        '(?i)(["''])[A-Z]:[\\/][^"'']+\1',
        '$1<path>$1')
    $safe = [regex]::Replace(
        $safe,
        '(?i)\b[A-Z]:[\\/][^\s;,)]+',
        '<path>')
    $safe = [regex]::Replace(
        $safe,
        '(?i)\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b',
        '<email>')
    $safe = [regex]::Replace(
        $safe,
        '(?i)\b[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}\b',
        '<id>')
    $safe = [regex]::Replace($safe, '(?i)\b[0-9A-F]{24,}\b', '<id>')
    $safe = [regex]::Replace($safe, '\b[0-9]{15,20}\b', '<id>')
    $safe = [regex]::Replace(
        $safe,
        '(?<![0-9])(?:[0-9]{1,3}\.){3}[0-9]{1,3}(?![0-9])',
        '<ip>')
    $safe = [regex]::Replace(
        $safe,
        '(?i)(?<![0-9A-F:])(?:[0-9A-F]{0,4}:){3,7}[0-9A-F]{0,4}(?![0-9A-F:])',
        '<ip>')
    $safe = [regex]::Replace(
        $safe,
        '(?i)\b(token|ticket|auth(?:entication)?|session|peer|account|identity|hostname|machine(?:name)?|user(?:name)?)\s*[:=]\s*(?:"[^"]*"|''[^'']*''|[^\s,;]+)',
        '$1=<redacted>')
    if ($safe.Length -gt 500) {
        $safe = $safe.Substring(0, 500)
    }
    return $safe
}

function Get-GuardedProcessDiagnosticSummary {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots,
        [ValidateSet("Server", "Client")][string]$LogScope = "Server"
    )

    $guardRoot = [IO.Path]::GetFullPath(
        (Split-Path -Parent $WorkingDirectory))
    if (-not (Test-Path -LiteralPath $guardRoot -PathType Container)) {
        return "no guarded diagnostics"
    }
    $partisanLines = New-Object Collections.Generic.List[object]
    $persistenceLines = New-Object Collections.Generic.List[object]
    $transportLines = New-Object Collections.Generic.List[object]
    $diagnosticLines = New-Object Collections.Generic.List[object]
    $fallbackLines = New-Object Collections.Generic.List[object]
    $allFiles = @(Get-ChildItem -LiteralPath $guardRoot -Recurse -File -Force `
        -ErrorAction SilentlyContinue | Where-Object {
            $_.Extension -in @(".log", ".txt", ".rpt")
        })
    if ($LogScope -ceq "Client") {
        $allFiles = @($allFiles | Where-Object {
            $_.FullName -match '(?i)loopback-client'
        })
    }
    else {
        $allFiles = @($allFiles | Where-Object {
            $_.FullName -notmatch '(?i)loopback-client'
        })
    }
    # Process the recent file set from oldest to newest so Select-Object -Last
    # below really returns the newest evidence across rotated logs.
    $files = @($allFiles | Sort-Object LastWriteTimeUtc, FullName |
        Select-Object -Last 160)
    $sequence = 0
    foreach ($file in $files) {
        if ($file.Length -le 0 -or $file.Length -gt 33554432) {
            continue
        }
        try {
            foreach ($line in @(Get-Content -LiteralPath $file.FullName -Tail 4000 `
                -ErrorAction Stop)) {
                $sequence++
                $text = ConvertTo-SafeEvidenceLine `
                    -Line ([string]$line) `
                    -GuardRoot $guardRoot `
                    -ProjectDirectory $ProjectDirectory `
                    -ResolvedAddonRoots $ResolvedAddonRoots
                if ([string]::IsNullOrWhiteSpace($text)) {
                    continue
                }
                $record = [pscustomobject]@{
                    Sequence = $sequence
                    Text = $text
                }
                if ($text -match '(?i)((save(?:game| game| point| data)?|persistence|storage|serializ(?:e|ed|er|ers|ing|ation)?|backend|database)[^\r\n]*(?:\s\(E\):|error|failed|failure|rejected|invalid|unavailable|denied|cannot|could not)|(?:\s\(E\):|error|failed|failure|rejected|invalid|unavailable|denied|cannot|could not)[^\r\n]*(save(?:game| game| point| data)?|persistence|storage|serializ(?:e|ed|er|ers|ing|ation)?|backend|database))') {
                    [void]$persistenceLines.Add($record)
                }
                elseif ($text -match '(?i)(Partisan vehicle persistence|Partisan ordinary campaign persistence proof|Partisan controlled campaign end|Partisan mixed-native shutdown proof|Partisan exact garrison rebuild external restart)') {
                    [void]$partisanLines.Add($record)
                }
                elseif ($text -match '(?i)(handshake timeout|unable to connect as client|validationerror|checksum|loaded addons or their order|(?:client|server)impl event|connection[^\r\n]*(?:failed|rejected|timed out|mismatch|validation)|(?:socket|network|replication|rpl)[^\r\n]*(?:error|failed|rejected|timeout|disconnect|mismatch|validation)|(?:error|failed|rejected|timeout|disconnect|mismatch|validation)[^\r\n]*(?:socket|network|replication|rpl)|battl.?eye[^\r\n]*(?:error|failed|reject|kick|disconnect|mismatch|validation))') {
                    [void]$transportLines.Add($record)
                }
                elseif ($text -match "(?i)(\s\(E\):|fatal|can't compile|cannot be packed|failed script compilation|broken expression|invalid statement|syntax error|unexpected scope|unknown type|undefined)") {
                    [void]$diagnosticLines.Add($record)
                }
                elseif ($text -match '(?i)(SCRIPT\s*\(W\)|failed|gproj|ResourceManager)') {
                    [void]$fallbackLines.Add($record)
                }
            }
        }
        catch {}
    }
    $primaryLines = @(
        @($persistenceLines.ToArray()) + @($diagnosticLines.ToArray()) |
            Sort-Object Sequence | Select-Object -Last 10)
    if ($primaryLines.Count -gt 0) {
        $selectedLines = New-Object Collections.Generic.List[string]
        foreach ($record in $primaryLines) {
            [void]$selectedLines.Add([string]$record.Text)
        }
        if ($partisanLines.Count -gt 0) {
            foreach ($record in @($partisanLines | Select-Object -Last 2)) {
                [void]$selectedLines.Add([string]$record.Text)
            }
        }
        foreach ($record in @($transportLines | Select-Object -Last 3)) {
            $selected = [string]$record.Text
            if ($selected.Length -gt 180) {
                $selected = $selected.Substring(0, 180)
            }
            [void]$selectedLines.Add($selected)
        }
        return ($selectedLines.ToArray() -join " | ")
    }
    if ($partisanLines.Count -gt 0) {
        $selectedLines = @($partisanLines | Select-Object -Last 12 |
            ForEach-Object { [string]$_.Text })
        foreach ($record in @($transportLines | Select-Object -Last 3)) {
            $selectedLines += [string]$record.Text
        }
        return ($selectedLines -join " | ")
    }
    if ($transportLines.Count -gt 0) {
        return (@($transportLines | Select-Object -Last 3 |
            ForEach-Object { [string]$_.Text }) -join " | ")
    }
    if ($diagnosticLines.Count -eq 0) {
        if ($fallbackLines.Count -eq 0) {
            return "no matching guarded diagnostic lines"
        }
        return (@($fallbackLines | Select-Object -Last 12 |
            ForEach-Object { [string]$_.Text }) -join " | ")
    }
    return (@($diagnosticLines | ForEach-Object { [string]$_.Text }) -join " | ")
}

function Get-GuardedServerClientDiagnosticSummary {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    $server = Get-GuardedProcessDiagnosticSummary `
        -WorkingDirectory $WorkingDirectory `
        -ProjectDirectory $ProjectDirectory `
        -ResolvedAddonRoots $ResolvedAddonRoots `
        -LogScope Server
    $client = Get-GuardedProcessDiagnosticSummary `
        -WorkingDirectory $WorkingDirectory `
        -ProjectDirectory $ProjectDirectory `
        -ResolvedAddonRoots $ResolvedAddonRoots `
        -LogScope Client
    $selected = New-Object Collections.Generic.List[string]
    if ($server -ne "no guarded diagnostics" -and
        $server -ne "no matching guarded diagnostic lines") {
        [void]$selected.Add("server $server")
    }
    if ($client -ne "no guarded diagnostics" -and
        $client -ne "no matching guarded diagnostic lines") {
        [void]$selected.Add("client $client")
    }
    if ($selected.Count -eq 0) {
        return "no matching guarded diagnostic lines"
    }
    return ($selected.ToArray() -join " | ")
}

function Get-NativeSaveLogDetail {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    $guardRoot = [IO.Path]::GetFullPath(
        (Split-Path -Parent $WorkingDirectory))
    $contextLines = New-Object Collections.Generic.List[string]
    $seenLines = New-Object 'Collections.Generic.HashSet[string]' `
        ([StringComparer]::Ordinal)
    $matchingErrorCount = 0
    if (Test-Path -LiteralPath $guardRoot -PathType Container) {
        $files = @(Get-ChildItem -LiteralPath $guardRoot -Recurse -File -Force `
            -ErrorAction SilentlyContinue | Where-Object {
                $_.Extension -in @(".log", ".txt", ".rpt") -and
                $_.FullName -notmatch '(?i)loopback-client' -and
                $_.Length -gt 0 -and $_.Length -le 33554432
            } | Sort-Object LastWriteTimeUtc, FullName | Select-Object -Last 80)
        foreach ($file in $files) {
            try {
                $lines = @(Get-Content -LiteralPath $file.FullName -Tail 5000 `
                    -ErrorAction Stop)
                for ($index = 0; $index -lt $lines.Count; $index++) {
                    $line = [string]$lines[$index]
                    if ($line -notmatch '(?i)(save(?:game| game| point| data)?|persistence|storage|serializ(?:e|ed|er|ers|ing|ation)?|backend|database)' -or
                        $line -notmatch "(?i)(\s\(E\):|error|failed|failure|rejected|invalid|unavailable|denied|cannot|could not|fatal)") {
                        continue
                    }
                    $matchingErrorCount++
                    $firstContext = [Math]::Max(0, $index - 2)
                    $lastContext = [Math]::Min($lines.Count - 1, $index + 2)
                    for ($contextIndex = $firstContext;
                        $contextIndex -le $lastContext;
                        $contextIndex++) {
                        $context = [string]$lines[$contextIndex]
                        if ($contextIndex -ne $index -and
                            ($context -match '(?i)(player|steam|account|identity|username|display\s*name|profile\s*name|chat|peer|client\s*name|platform)' -or
                            $context -notmatch '(?i)(save|persistence|storage|serializ|backend|database|SCRIPT|ENGINE|GAME|RPL|DEFAULT|RESOURCES)')) {
                            continue
                        }
                        $safe = ConvertTo-SafeEvidenceLine `
                            -Line $context `
                            -GuardRoot $guardRoot `
                            -ProjectDirectory $ProjectDirectory `
                            -ResolvedAddonRoots $ResolvedAddonRoots
                        if (-not [string]::IsNullOrWhiteSpace($safe) -and
                            $seenLines.Add($safe)) {
                            [void]$contextLines.Add($safe)
                        }
                    }
                }
            }
            catch {}
        }
    }
    return [pscustomobject]@{
        Scope = "server"
        MatchingErrorCount = $matchingErrorCount
        ContextLines = @($contextLines | Select-Object -Last 16)
        CaptureSucceeded = $true
    }
}

function Read-ProcessIdentity {
    param([Parameter(Mandatory = $true)][int]$ProcessId)

    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $process) {
        return $null
    }
    try {
        $startUtc = $process.StartTime.ToUniversalTime()
        $row = @(Get-CimInstance Win32_Process `
            -Filter "ProcessId=$ProcessId" -ErrorAction Stop)
        if ($row.Count -ne 1) {
            return $null
        }
        $executablePath = [string]$row[0].ExecutablePath
        if ([string]::IsNullOrWhiteSpace($executablePath)) {
            try {
                $executablePath =
                    [PartisanProcessInspection]::QueryImagePath($ProcessId)
            }
            catch { }
        }
        if ([string]::IsNullOrWhiteSpace($executablePath)) {
            return $null
        }
        return [pscustomobject]@{
            ProcessId = $ProcessId
            ProcessName = [string]$process.ProcessName
            ParentProcessId = [int]$row[0].ParentProcessId
            StartUtc = $startUtc
            ExecutablePath = [IO.Path]::GetFullPath($executablePath)
            CommandLine = [string]$row[0].CommandLine
        }
    }
    catch {
        return $null
    }
}

function Resolve-RunningSteamBootstrapIdentity {
    $rows = @(Get-CimInstance Win32_Process `
        -Filter "Name='steam.exe'" -ErrorAction Stop)
    if ($rows.Count -ne 1) {
        throw "Exactly one running Steam bootstrap process is required."
    }
    $identity = Read-ProcessIdentity -ProcessId ([int]$rows[0].ProcessId)
    if (-not $identity -or
        (Split-Path -Leaf $identity.ExecutablePath) -ine "steam.exe" -or
        [string]::IsNullOrWhiteSpace($identity.CommandLine)) {
        throw "The running Steam bootstrap identity is unavailable."
    }
    return $identity
}

function Assert-SteamExcludedFromOwnership {
    param(
        [Parameter(Mandatory = $true)]$SteamIdentity,
        [Parameter(Mandatory = $true)]$Job,
        [Parameter(Mandatory = $true)][hashtable]$Owned
    )

    if ($Owned.ContainsKey([int]$SteamIdentity.ProcessId) -or
        @($Job.GetProcessIds()) -contains [int]$SteamIdentity.ProcessId) {
        throw "Steam crossed the guarded process ownership boundary."
    }
}

function Start-GuardedJobProcess {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$TempDirectory,
        [Parameter(Mandatory = $true)]$Job,
        [Parameter(Mandatory = $true)][hashtable]$Owned
    )

    if ($ExecutablePath.Contains('"') -or
        @($Arguments | Where-Object {
            ([string]$_).Contains('"')
        }).Count -ne 0) {
        throw "$Label contains an unsupported embedded quote character."
    }
    $argumentString = ($Arguments | ForEach-Object {
        ConvertTo-NativeArgument ([string]$_)
    }) -join " "
    $expectedCommandLine = (ConvertTo-NativeArgument $ExecutablePath) +
        " " + $argumentString
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $expectedCommandLine `
        -ExpectedExecutable $ExecutablePath `
        -ExpectedArguments $Arguments)) {
        throw "$Label arguments did not round-trip exactly."
    }

    $suspendedLauncher = $null
    try {
        $previousTemp = [Environment]::GetEnvironmentVariable(
            "TEMP", [EnvironmentVariableTarget]::Process)
        $previousTmp = [Environment]::GetEnvironmentVariable(
            "TMP", [EnvironmentVariableTarget]::Process)
        try {
            [Environment]::SetEnvironmentVariable(
                "TEMP", $TempDirectory, [EnvironmentVariableTarget]::Process)
            [Environment]::SetEnvironmentVariable(
                "TMP", $TempDirectory, [EnvironmentVariableTarget]::Process)
            $suspendedLauncher = New-Object `
                PartisanOrdinaryPersistenceSuspendedProcess(
                    $ExecutablePath,
                    $expectedCommandLine,
                    $WorkingDirectory)
        }
        finally {
            [Environment]::SetEnvironmentVariable(
                "TEMP", $previousTemp, [EnvironmentVariableTarget]::Process)
            [Environment]::SetEnvironmentVariable(
                "TMP", $previousTmp, [EnvironmentVariableTarget]::Process)
        }

        $process = $suspendedLauncher.Child
        if (-not $process) {
            throw "$Label did not create its guarded process."
        }
        $rootId = $process.Id
        $Job.Add($process)
        $rootStartUtc = $process.StartTime.ToUniversalTime()
        $Owned[$rootId] = $rootStartUtc
        $suspendedLauncher.Resume()
        $suspendedLauncher.Dispose()
        $suspendedLauncher = $null

        Start-Sleep -Milliseconds 500
        $process.Refresh()
        if ($process.HasExited) {
            throw "$Label exited before command-line validation."
        }
        $row = Get-CimInstance Win32_Process `
            -Filter "ProcessId=$rootId" -ErrorAction Stop
        if (-not $row -or
            -not (Test-ExactNativeArgumentVector `
                -CommandLine ([string]$row.CommandLine) `
                -ExpectedExecutable $ExecutablePath `
                -ExpectedArguments $Arguments)) {
            throw "$Label launched with a non-exact argument vector."
        }
        return [pscustomobject]@{
            Process = $process
            RootId = [int]$rootId
            RootStartUtc = $rootStartUtc
        }
    }
    finally {
        if ($suspendedLauncher) {
            $suspendedLauncher.Dispose()
        }
    }
}

function Assert-GuardedEngineOwnership {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][hashtable]$Owned,
        [Parameter(Mandatory = $true)]$Job,
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    $jobProcessIds = @()
    try {
        $jobProcessIds = @($Job.GetProcessIds())
    }
    catch { }
    foreach ($engineProcess in @(Get-EngineProcessRows)) {
        $engineProcessId = [int]$engineProcess.Id
        if ($Owned.ContainsKey($engineProcessId) -and
            (Test-ProcessIdentityAlive `
                -ProcessId $engineProcessId `
                -StartUtc $Owned[$engineProcessId])) {
            continue
        }
        if ($jobProcessIds -contains $engineProcessId) {
            try {
                $Owned[$engineProcessId] =
                    $engineProcess.StartTime.ToUniversalTime()
                continue
            }
            catch { }
        }
        [void]$UnclaimedEngineProcessesObserved.Add(
            "$($engineProcess.ProcessName):$($engineProcess.StartTime.ToUniversalTime().Ticks)")
    }
    if ($UnclaimedEngineProcessesObserved.Count -ne 0) {
        $unclaimedNames = @($UnclaimedEngineProcessesObserved |
            ForEach-Object { ([string]$_).Split(':')[0] } |
            Sort-Object -Unique) -join ","
        throw "An unowned engine process appeared during $Label " +
            "(names=$unclaimedNames)."
    }
}

function Invoke-GuardedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$TempDirectory,
        [Parameter(Mandatory = $true)][int]$TimeoutSeconds,
        [string]$ResultPath = "",
        [scriptblock]$ResultValidator,
        [Parameter(Mandatory = $true)][string]$DiagnosticProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$DiagnosticAddonRoots,
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    if (@(Get-EngineProcessRows).Count -ne 0) {
        throw "An engine process appeared before $Label."
    }
    if ($ExecutablePath.Contains('"') -or
        @($Arguments | Where-Object {
            ([string]$_).Contains('"')
        }).Count -ne 0) {
        throw "$Label contains an unsupported embedded quote character."
    }
    $argumentString = ($Arguments | ForEach-Object {
        ConvertTo-NativeArgument ([string]$_)
    }) -join " "
    $expectedCommandLine = (ConvertTo-NativeArgument $ExecutablePath) +
        " " + $argumentString
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $expectedCommandLine `
        -ExpectedExecutable $ExecutablePath `
        -ExpectedArguments $Arguments)) {
        throw "$Label arguments did not round-trip exactly."
    }

    $process = $null
    $job = $null
    $suspendedLauncher = $null
    $owned = @{}
    $rootId = 0
    $rootStartUtc = [DateTime]::MinValue
    $exitCode = $null
    $result = $null
    $runError = $null
    $cleanupErrors = New-Object Collections.Generic.List[string]
    $startedUtc = [DateTime]::UtcNow
    try {
        $job = New-Object PartisanOrdinaryPersistenceJob
        if (@(Get-EngineProcessRows).Count -ne 0) {
            throw "An engine process appeared during $Label preflight."
        }

        $previousTemp = [Environment]::GetEnvironmentVariable(
            "TEMP", [EnvironmentVariableTarget]::Process)
        $previousTmp = [Environment]::GetEnvironmentVariable(
            "TMP", [EnvironmentVariableTarget]::Process)
        try {
            [Environment]::SetEnvironmentVariable(
                "TEMP", $TempDirectory, [EnvironmentVariableTarget]::Process)
            [Environment]::SetEnvironmentVariable(
                "TMP", $TempDirectory, [EnvironmentVariableTarget]::Process)
            $suspendedLauncher = New-Object `
                PartisanOrdinaryPersistenceSuspendedProcess(
                    $ExecutablePath,
                    $expectedCommandLine,
                    $WorkingDirectory)
        }
        finally {
            [Environment]::SetEnvironmentVariable(
                "TEMP", $previousTemp, [EnvironmentVariableTarget]::Process)
            [Environment]::SetEnvironmentVariable(
                "TMP", $previousTmp, [EnvironmentVariableTarget]::Process)
        }

        $process = $suspendedLauncher.Child
        if (-not $process) {
            throw "$Label did not create its guarded process."
        }
        $rootId = $process.Id
        $job.Add($process)
        $rootStartUtc = $process.StartTime.ToUniversalTime()
        $owned[$rootId] = $rootStartUtc
        $suspendedLauncher.Resume()
        $suspendedLauncher.Dispose()
        $suspendedLauncher = $null

        Start-Sleep -Milliseconds 500
        $process.Refresh()
        if ($process.HasExited) {
            throw "$Label exited before command-line validation."
        }
        $row = Get-CimInstance Win32_Process `
            -Filter "ProcessId=$rootId" -ErrorAction Stop
        if (-not $row -or
            -not (Test-ExactNativeArgumentVector `
                -CommandLine ([string]$row.CommandLine) `
                -ExpectedExecutable $ExecutablePath `
                -ExpectedArguments $Arguments)) {
            throw "$Label launched with a non-exact argument vector."
        }

        $deadlineUtc = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $exitObservedUtc = [DateTime]::MinValue
        $lastResultSignature = ""
        $stableResultPolls = 0
        while ([DateTime]::UtcNow -lt $deadlineUtc) {
            Update-OwnedProcesses $owned $rootId $rootStartUtc $job
            foreach ($engineProcess in @(Get-EngineProcessRows)) {
                $engineProcessId = [int]$engineProcess.Id
                if ($owned.ContainsKey($engineProcessId) -and
                    (Test-ProcessIdentityAlive `
                        -ProcessId $engineProcessId `
                        -StartUtc $owned[$engineProcessId])) {
                    continue
                }
                [void]$UnclaimedEngineProcessesObserved.Add(
                    "$($engineProcess.ProcessName):$($engineProcess.StartTime.ToUniversalTime().Ticks)")
            }
            if ($UnclaimedEngineProcessesObserved.Count -ne 0) {
                throw "An unowned engine process appeared during $Label."
            }

            if (-not [string]::IsNullOrWhiteSpace($ResultPath) -and
                (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
                try {
                    $signature = Get-FileSignature -Path $ResultPath
                    if ($signature -ceq $lastResultSignature) {
                        $stableResultPolls++
                    }
                    else {
                        $lastResultSignature = $signature
                        $stableResultPolls = 1
                        $result = $null
                    }
                    if ($stableResultPolls -ge 2) {
                        $candidate = Read-JsonArtifact -Path $ResultPath
                        if ($ResultValidator) {
                            $result = & $ResultValidator $candidate
                        }
                        else {
                            $result = $candidate
                        }
                    }
                }
                catch {
                    if ($stableResultPolls -ge 2) {
                        throw
                    }
                    $lastResultSignature = ""
                    $stableResultPolls = 0
                    $result = $null
                }
            }
            elseif (-not [string]::IsNullOrWhiteSpace($ResultPath) -and
                $result) {
                $lastResultSignature = ""
                $stableResultPolls = 0
                $result = $null
            }

            $process.Refresh()
            if ($process.HasExited -and
                $exitObservedUtc -eq [DateTime]::MinValue) {
                $exitObservedUtc = [DateTime]::UtcNow
                $exitCode = $process.ExitCode
            }
            $liveOwned = Get-LiveOwnedProcessCount $owned
            $resultReady = [string]::IsNullOrWhiteSpace($ResultPath) -or $result
            if ($null -ne $exitCode -and $liveOwned -eq 0 -and $resultReady) {
                break
            }
            if ($exitObservedUtc -ne [DateTime]::MinValue -and
                -not $resultReady -and
                ([DateTime]::UtcNow - $exitObservedUtc).TotalSeconds -ge
                    $ResultGraceSeconds) {
                $diagnostic = Get-GuardedProcessDiagnosticSummary `
                    -WorkingDirectory $WorkingDirectory `
                    -ProjectDirectory $DiagnosticProjectDirectory `
                    -ResolvedAddonRoots $DiagnosticAddonRoots
                throw "$Label exited without a stable exact result | $diagnostic"
            }
            Start-Sleep -Milliseconds $PollMilliseconds
        }

        if ($null -eq $exitCode -or
            (Get-LiveOwnedProcessCount $owned) -ne 0) {
            $diagnostic = Get-GuardedProcessDiagnosticSummary `
                -WorkingDirectory $WorkingDirectory `
                -ProjectDirectory $DiagnosticProjectDirectory `
                -ResolvedAddonRoots $DiagnosticAddonRoots
            throw "$Label exceeded its guarded process deadline | $diagnostic"
        }
        if ([int]$exitCode -ne 0) {
            $diagnostic = Get-GuardedProcessDiagnosticSummary `
                -WorkingDirectory $WorkingDirectory `
                -ProjectDirectory $DiagnosticProjectDirectory `
                -ResolvedAddonRoots $DiagnosticAddonRoots
            throw "$Label returned a nonzero exit code | $diagnostic"
        }
        if (-not [string]::IsNullOrWhiteSpace($ResultPath) -and -not $result) {
            throw "$Label did not produce a valid result."
        }
        if (-not [string]::IsNullOrWhiteSpace($ResultPath)) {
            if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf) -or
                (Get-FileSignature -Path $ResultPath) -cne
                    $lastResultSignature) {
                throw "$Label final result artifact changed after validation."
            }
            $finalCandidate = Read-JsonArtifact -Path $ResultPath
            if ($ResultValidator) {
                $result = & $ResultValidator $finalCandidate
            }
            else {
                $result = $finalCandidate
            }
        }
    }
    catch {
        $runError = $_.Exception.Message
        $failureDiagnostic = Get-GuardedProcessDiagnosticSummary `
            -WorkingDirectory $WorkingDirectory `
            -ProjectDirectory $DiagnosticProjectDirectory `
            -ResolvedAddonRoots $DiagnosticAddonRoots
        if ($failureDiagnostic -ne "no guarded diagnostics" -and
            $failureDiagnostic -ne "no matching guarded diagnostic lines") {
            $runError += " | diagnostics $failureDiagnostic"
        }
    }
    finally {
        try {
            if ($suspendedLauncher) {
                $suspendedLauncher.Dispose()
                $suspendedLauncher = $null
            }
        }
        catch { [void]$cleanupErrors.Add("dispose-suspended-launcher") }
        try {
            if ($rootId -gt 0 -and $rootStartUtc -ne [DateTime]::MinValue) {
                Update-OwnedProcesses $owned $rootId $rootStartUtc $job
            }
        }
        catch { [void]$cleanupErrors.Add("discover-owned") }
        try {
            Stop-OwnedProcesses $owned
            Start-Sleep -Milliseconds 300
            Stop-OwnedProcesses $owned
        }
        catch { [void]$cleanupErrors.Add("stop-owned") }
        if ($runError) {
            try {
                $postStopDiagnostic = Get-GuardedProcessDiagnosticSummary `
                    -WorkingDirectory $WorkingDirectory `
                    -ProjectDirectory $DiagnosticProjectDirectory `
                    -ResolvedAddonRoots $DiagnosticAddonRoots
                if ($postStopDiagnostic -ne "no guarded diagnostics" -and
                    $postStopDiagnostic -ne
                        "no matching guarded diagnostic lines") {
                    $diagnosticMarker = " | diagnostics "
                    $diagnosticIndex = $runError.IndexOf(
                        $diagnosticMarker,
                        [StringComparison]::Ordinal)
                    if ($diagnosticIndex -ge 0) {
                        $runError = $runError.Substring(0, $diagnosticIndex)
                    }
                    $runError += $diagnosticMarker + $postStopDiagnostic
                }
            }
            catch { [void]$cleanupErrors.Add("capture-post-stop-diagnostic") }
        }
        try {
            if ($job) {
                $job.Dispose()
                $job = $null
            }
        }
        catch { [void]$cleanupErrors.Add("dispose-job") }
        try {
            Start-Sleep -Milliseconds 300
            Stop-OwnedProcesses $owned
        }
        catch { [void]$cleanupErrors.Add("final-owned-stop") }
        try {
            if ($process) {
                $process.Dispose()
                $process = $null
            }
        }
        catch { [void]$cleanupErrors.Add("dispose-process") }
    }

    $ownedRemaining = Get-LiveOwnedProcessCount $owned
    $engineAfter = @(Get-EngineProcessRows).Count
    if ($cleanupErrors.Count -ne 0 -or $ownedRemaining -ne 0 -or
        $engineAfter -ne 0) {
        throw "$Label cleanup failed (phases=$($cleanupErrors -join ','); " +
            "owned=$ownedRemaining; engine=$engineAfter)."
    }
    if ($runError) {
        throw $runError
    }
    return [pscustomobject]@{
        Result = $result
        ExitCode = [int]$exitCode
        EngineAfter = $engineAfter
        OwnedProcessesRemaining = $ownedRemaining
        ElapsedSeconds = [Math]::Round(
            ([DateTime]::UtcNow - $startedUtc).TotalSeconds, 3)
    }
}

function Invoke-GuardedServerWithLoopbackClient {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][string]$ServerExecutablePath,
        [Parameter(Mandatory = $true)][string[]]$ServerArguments,
        [Parameter(Mandatory = $true)][string]$SteamExecutablePath,
        [Parameter(Mandatory = $true)][string]$ClientExecutablePath,
        [Parameter(Mandatory = $true)][string[]]$ClientArguments,
        [Parameter(Mandatory = $true)][string]$ServerWorkingDirectory,
        [Parameter(Mandatory = $true)][string]$ServerTempDirectory,
        [Parameter(Mandatory = $true)][string]$ClientWorkingDirectory,
        [Parameter(Mandatory = $true)][string]$ClientTempDirectory,
        [Parameter(Mandatory = $true)][int]$TimeoutSeconds,
        [Parameter(Mandatory = $true)][string]$ReadinessPath,
        [Parameter(Mandatory = $true)][scriptblock]$ReadinessValidator,
        [Parameter(Mandatory = $true)][string]$ResultPath,
        [Parameter(Mandatory = $true)][scriptblock]$ResultValidator,
        [Parameter(Mandatory = $true)][string]$DiagnosticProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$DiagnosticAddonRoots,
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    if (@(Get-EngineProcessRows).Count -ne 0) {
        throw "An engine process appeared before $Label."
    }
    if (Test-Path -LiteralPath $ReadinessPath) {
        throw "$Label readiness receipt path was not fresh."
    }
    $job = $null
    $serverLaunch = $null
    $clientLaunch = $null
    $owned = @{}
    $serverExitCode = $null
    $result = $null
    $runError = $null
    $readinessObserved = $false
    $cleanupErrors = New-Object Collections.Generic.List[string]
    $startedUtc = [DateTime]::UtcNow
    try {
        $job = New-Object PartisanOrdinaryPersistenceJob
        if (@(Get-EngineProcessRows).Count -ne 0) {
            throw "An engine process appeared during $Label preflight."
        }
        $serverLaunch = Start-GuardedJobProcess `
            -Label "$Label server" `
            -ExecutablePath $ServerExecutablePath `
            -Arguments $ServerArguments `
            -WorkingDirectory $ServerWorkingDirectory `
            -TempDirectory $ServerTempDirectory `
            -Job $job `
            -Owned $owned

        $deadlineUtc = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $exitObservedUtc = [DateTime]::MinValue
        $lastReadinessSignature = ""
        $stableReadinessPolls = 0
        $lastResultSignature = ""
        $stableResultPolls = 0
        while ([DateTime]::UtcNow -lt $deadlineUtc) {
            Update-OwnedProcesses `
                $owned `
                $serverLaunch.RootId `
                $serverLaunch.RootStartUtc `
                $job
            Assert-GuardedEngineOwnership `
                -Label $Label `
                -Owned $owned `
                -Job $job `
                -UnclaimedEngineProcessesObserved `
                    $UnclaimedEngineProcessesObserved

            $serverLaunch.Process.Refresh()
            if ($serverLaunch.Process.HasExited -and
                $exitObservedUtc -eq [DateTime]::MinValue) {
                $exitObservedUtc = [DateTime]::UtcNow
                $serverExitCode = $serverLaunch.Process.ExitCode
            }
            if ($clientLaunch) {
                $clientLaunch.Process.Refresh()
                if ($clientLaunch.Process.HasExited -and
                    -not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
                    throw "$Label client exited before the guarded result."
                }
            }

            if (-not $clientLaunch -and
                $exitObservedUtc -eq [DateTime]::MinValue -and
                (Test-Path -LiteralPath $ReadinessPath -PathType Leaf)) {
                try {
                    $readinessSignature = Get-FileSignature `
                        -Path $ReadinessPath
                    if ($readinessSignature -ceq
                            $lastReadinessSignature) {
                        $stableReadinessPolls++
                    }
                    else {
                        $lastReadinessSignature = $readinessSignature
                        $stableReadinessPolls = 1
                    }
                    if ($stableReadinessPolls -ge 2) {
                        [void](& $ReadinessValidator (
                            Read-JsonArtifact -Path $ReadinessPath))
                        $readinessObserved = $true
                        $steamIdentity = Resolve-RunningSteamBootstrapIdentity
                        if (-not $steamIdentity.ExecutablePath.Equals(
                                [IO.Path]::GetFullPath($SteamExecutablePath),
                                [StringComparison]::OrdinalIgnoreCase)) {
                            throw "$Label Steam backend prerequisite changed."
                        }
                        Assert-SteamExcludedFromOwnership `
                            -SteamIdentity $steamIdentity `
                            -Job $job `
                            -Owned $owned
                        $clientLaunch = Start-GuardedJobProcess `
                            -Label "$Label client" `
                            -ExecutablePath $ClientExecutablePath `
                            -Arguments $ClientArguments `
                            -WorkingDirectory $ClientWorkingDirectory `
                            -TempDirectory $ClientTempDirectory `
                            -Job $job `
                            -Owned $owned
                        Update-OwnedProcesses `
                            $owned `
                            $clientLaunch.RootId `
                            $clientLaunch.RootStartUtc `
                            $job
                        Update-OwnedProcesses `
                            $owned `
                            $serverLaunch.RootId `
                            $serverLaunch.RootStartUtc `
                            $job
                        Assert-SteamExcludedFromOwnership `
                            -SteamIdentity $steamIdentity `
                            -Job $job `
                            -Owned $owned
                        Assert-GuardedEngineOwnership `
                            -Label $Label `
                            -Owned $owned `
                            -Job $job `
                            -UnclaimedEngineProcessesObserved `
                                $UnclaimedEngineProcessesObserved
                    }
                }
                catch {
                    if ($stableReadinessPolls -ge 2) {
                        throw
                    }
                    $lastReadinessSignature = ""
                    $stableReadinessPolls = 0
                }
            }

            if (Test-Path -LiteralPath $ResultPath -PathType Leaf) {
                try {
                    $signature = Get-FileSignature -Path $ResultPath
                    if ($signature -ceq $lastResultSignature) {
                        $stableResultPolls++
                    }
                    else {
                        $lastResultSignature = $signature
                        $stableResultPolls = 1
                        $result = $null
                    }
                    if ($stableResultPolls -ge 2) {
                        $result = & $ResultValidator (
                            Read-JsonArtifact -Path $ResultPath)
                    }
                }
                catch {
                    if ($stableResultPolls -ge 2) {
                        throw
                    }
                    $lastResultSignature = ""
                    $stableResultPolls = 0
                    $result = $null
                }
            }
            elseif ($result) {
                $lastResultSignature = ""
                $stableResultPolls = 0
                $result = $null
            }

            if ($null -ne $serverExitCode -and $result) {
                break
            }
            if ($exitObservedUtc -ne [DateTime]::MinValue -and
                -not $result -and
                ([DateTime]::UtcNow - $exitObservedUtc).TotalSeconds -ge
                    $ResultGraceSeconds) {
                $diagnostic = Get-GuardedServerClientDiagnosticSummary `
                    -WorkingDirectory $ServerWorkingDirectory `
                    -ProjectDirectory $DiagnosticProjectDirectory `
                    -ResolvedAddonRoots $DiagnosticAddonRoots
                throw "$Label server exited without a stable exact result | $diagnostic"
            }
            Start-Sleep -Milliseconds $PollMilliseconds
        }

        if (-not $readinessObserved -or -not $clientLaunch) {
            $diagnostic = Get-GuardedServerClientDiagnosticSummary `
                -WorkingDirectory $ServerWorkingDirectory `
                -ProjectDirectory $DiagnosticProjectDirectory `
                -ResolvedAddonRoots $DiagnosticAddonRoots
            throw "$Label did not reach its guarded client readiness gate | $diagnostic"
        }
        if ($null -eq $serverExitCode) {
            $diagnostic = Get-GuardedServerClientDiagnosticSummary `
                -WorkingDirectory $ServerWorkingDirectory `
                -ProjectDirectory $DiagnosticProjectDirectory `
                -ResolvedAddonRoots $DiagnosticAddonRoots
            throw "$Label exceeded its guarded process deadline | $diagnostic"
        }
        if ([int]$serverExitCode -ne 0) {
            $diagnostic = Get-GuardedServerClientDiagnosticSummary `
                -WorkingDirectory $ServerWorkingDirectory `
                -ProjectDirectory $DiagnosticProjectDirectory `
                -ResolvedAddonRoots $DiagnosticAddonRoots
            throw "$Label server returned a nonzero exit code | $diagnostic"
        }
        if (-not $result) {
            throw "$Label did not produce a valid result."
        }
        if (-not (Test-Path -LiteralPath $ReadinessPath -PathType Leaf) -or
            (Get-FileSignature -Path $ReadinessPath) -cne
                $lastReadinessSignature) {
            throw "$Label readiness receipt changed after validation."
        }
        [void](& $ReadinessValidator (
            Read-JsonArtifact -Path $ReadinessPath))
        if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf) -or
            (Get-FileSignature -Path $ResultPath) -cne $lastResultSignature) {
            throw "$Label final result artifact changed after validation."
        }
        $result = & $ResultValidator (Read-JsonArtifact -Path $ResultPath)
    }
    catch {
        $runError = $_.Exception.Message
        $failureDiagnostic = Get-GuardedServerClientDiagnosticSummary `
            -WorkingDirectory $ServerWorkingDirectory `
            -ProjectDirectory $DiagnosticProjectDirectory `
            -ResolvedAddonRoots $DiagnosticAddonRoots
        if ($failureDiagnostic -ne "no guarded diagnostics" -and
            $failureDiagnostic -ne "no matching guarded diagnostic lines") {
            $runError += " | diagnostics $failureDiagnostic"
        }
    }
    finally {
        try {
            if ($serverLaunch) {
                Update-OwnedProcesses `
                    $owned `
                    $serverLaunch.RootId `
                    $serverLaunch.RootStartUtc `
                    $job
            }
        }
        catch { [void]$cleanupErrors.Add("discover-owned-server-client") }
        try {
            Stop-OwnedProcesses $owned
            Start-Sleep -Milliseconds 300
            Stop-OwnedProcesses $owned
        }
        catch { [void]$cleanupErrors.Add("stop-owned-server-client") }
        if ($runError) {
            try {
                $postStopDiagnostic = Get-GuardedServerClientDiagnosticSummary `
                    -WorkingDirectory $ServerWorkingDirectory `
                    -ProjectDirectory $DiagnosticProjectDirectory `
                    -ResolvedAddonRoots $DiagnosticAddonRoots
                if ($postStopDiagnostic -ne "no guarded diagnostics" -and
                    $postStopDiagnostic -ne
                        "no matching guarded diagnostic lines") {
                    $diagnosticMarker = " | diagnostics "
                    $diagnosticIndex = $runError.IndexOf(
                        $diagnosticMarker,
                        [StringComparison]::Ordinal)
                    if ($diagnosticIndex -ge 0) {
                        $runError = $runError.Substring(0, $diagnosticIndex)
                    }
                    $runError += $diagnosticMarker + $postStopDiagnostic
                }
            }
            catch { [void]$cleanupErrors.Add("capture-server-client-diagnostic") }
            try {
                $nativeSaveDetail = Get-NativeSaveLogDetail `
                    -WorkingDirectory $ServerWorkingDirectory `
                    -ProjectDirectory $DiagnosticProjectDirectory `
                    -ResolvedAddonRoots $DiagnosticAddonRoots
                Write-Host ("NATIVE_SAVE_LOG_DETAIL " +
                    ($nativeSaveDetail | ConvertTo-Json -Compress -Depth 3))
            }
            catch {
                Write-Host ('NATIVE_SAVE_LOG_DETAIL ' +
                    '{"Scope":"server","MatchingErrorCount":0,' +
                    '"ContextLines":[],"CaptureSucceeded":false}')
            }
        }
        try {
            if ($job) {
                $job.Dispose()
                $job = $null
            }
        }
        catch { [void]$cleanupErrors.Add("dispose-server-client-job") }
        try {
            Start-Sleep -Milliseconds 300
            Stop-OwnedProcesses $owned
        }
        catch { [void]$cleanupErrors.Add("final-server-client-stop") }
        foreach ($launch in @($clientLaunch, $serverLaunch)) {
            try {
                if ($launch -and $launch.Process) {
                    $launch.Process.Dispose()
                }
            }
            catch { [void]$cleanupErrors.Add("dispose-server-client-process") }
        }
    }

    $ownedRemaining = Get-LiveOwnedProcessCount $owned
    $engineAfter = @(Get-EngineProcessRows).Count
    if ($cleanupErrors.Count -ne 0 -or
        $ownedRemaining -ne 0 -or
        $engineAfter -ne 0) {
        throw "$Label cleanup failed (phases=$($cleanupErrors -join ','); " +
            "owned=$ownedRemaining; engine=$engineAfter)."
    }
    if ($runError) {
        throw $runError
    }
    return [pscustomobject]@{
        Result = $result
        ExitCode = [int]$serverExitCode
        EngineAfter = $engineAfter
        OwnedProcessesRemaining = $ownedRemaining
        ClientLaunched = $null -ne $clientLaunch
        ReadinessObserved = $readinessObserved
        ElapsedSeconds = [Math]::Round(
            ([DateTime]::UtcNow - $startedUtc).TotalSeconds, 3)
    }
}

function Get-PackArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectFile,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$PackProfilePath,
        [Parameter(Mandatory = $true)][string]$PackedAddonPath
    )

    return @(
        "-gproj", $ProjectFile,
        "-addonsDir", $RuntimeAddonPath,
        "-profile", $PackProfilePath,
        "-wbModule=ResourceManager",
        "-packAddon",
        "-packAddonDir", $PackedAddonPath,
        "-noThrow")
}

function Assert-PackedAddonLayout {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent
    )

    $parent = Resolve-ExistingPath $PackedAddonsParent Container
    if (-not (Test-ContainedPath $GuardRoot $parent)) {
        throw "The packed add-on parent escaped the nonce-owned guard."
    }
    $parentEntries = @(Get-ChildItem -LiteralPath $parent -Force)
    if ($parentEntries.Count -ne 1 -or
        -not $parentEntries[0].PSIsContainer -or
        [string]$parentEntries[0].Name -cne "Partisan") {
        throw "The packed add-on parent does not contain one exact add-on."
    }
    $addon = [IO.Path]::GetFullPath($parentEntries[0].FullName)
    Assert-NoReparseDescendant -Root $addon
    foreach ($leaf in @("addon.gproj", "data.pak", "resourceDatabase.rdb")) {
        $path = Join-Path $addon $leaf
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or
            (Get-Item -LiteralPath $path -Force).Length -le 0) {
            throw "Workbench packing omitted a required artifact."
        }
    }
    $files = @(Get-ChildItem -LiteralPath $addon -File -Force)
    $directories = @(Get-ChildItem -LiteralPath $addon -Directory -Force)
    if ($directories.Count -ne 0 -or
        @($files | Where-Object { $_.Extension -ceq ".pak" }).Count -ne 1 -or
        @($files | Where-Object { $_.Extension -ceq ".gproj" }).Count -ne 1 -or
        @($files | Where-Object { $_.Extension -ceq ".rdb" }).Count -ne 1) {
        throw "The packed add-on artifact family is not exact."
    }
    return [pscustomobject]@{
        AddonPath = $addon
        FileCount = $files.Count
        PakCount = @($files | Where-Object { $_.Extension -ceq ".pak" }).Count
        ProjectCount = @($files | Where-Object { $_.Extension -ceq ".gproj" }).Count
        ResourceDatabaseCount = @($files | Where-Object {
            $_.Extension -ceq ".rdb"
        }).Count
    }
}

function Assert-EngineOwner {
    param(
        [Parameter(Mandatory = $true)]$Owner,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $label = "ordinary persistence engine owner"
    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sPurpose",
        "m_sSessionNonce",
        "m_sRunId",
        "m_sPayloadNonce",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion",
        "m_sWorld",
        "m_iExpectedStageCount",
        "m_bDisposableProfile",
        "m_bMixedNativeProofRequired")) {
        Assert-JsonProperty $Owner $property $label
    }
    Assert-LowerHexNonce $SessionNonce "session nonce"
    Assert-LowerHexNonce $PayloadNonce "payload nonce"
    if ([string]$Owner.m_sMagic -cne $script:OwnerMagic -or
        [int]$Owner.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Owner.m_sPurpose -cne $script:OwnerPurpose -or
        [string]$Owner.m_sSessionNonce -cne $SessionNonce -or
        [string]$Owner.m_sRunId -cne $RunId -or
        [string]$Owner.m_sPayloadNonce -cne $PayloadNonce -or
        [string]$Owner.m_sWorld -cne $ExpectedWorld -or
        [int]$Owner.m_iExpectedStageCount -ne 5 -or
        $Owner.m_bDisposableProfile -isnot [bool] -or
        -not [bool]$Owner.m_bDisposableProfile -or
        $Owner.m_bMixedNativeProofRequired -isnot [bool] -or
        -not [bool]$Owner.m_bMixedNativeProofRequired) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity $Owner $ExpectedBuild $label
}

function Assert-EngineGuard {
    param(
        [Parameter(Mandatory = $true)]$Guard,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $label = "$Stage one-use engine guard"
    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sSessionNonce",
        "m_sStageNonce",
        "m_sRunId",
        "m_sPayloadNonce",
        "m_sRequestedStage",
        "m_iStageOrdinal",
        "m_sExpectedSource",
        "m_iExpectedSentinelGeneration",
        "m_sExpectedSaveType",
        "m_sExpectedSaveName",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion",
        "m_sWorld",
        "m_bAllowCanonicalCampaignOverwrite",
        "m_bMixedNativeProofRequired")) {
        Assert-JsonProperty $Guard $property $label
    }
    $allowWrite = $script:StageOrdinals[$Stage] -le 2
    if ([string]$Guard.m_sMagic -cne $script:GuardMagic -or
        [int]$Guard.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Guard.m_sSessionNonce -cne $SessionNonce -or
        [string]$Guard.m_sStageNonce -cne $StageNonce -or
        [string]$Guard.m_sRunId -cne $RunId -or
        [string]$Guard.m_sPayloadNonce -cne $PayloadNonce -or
        [string]$Guard.m_sRequestedStage -cne $Stage -or
        [int]$Guard.m_iStageOrdinal -ne $script:StageOrdinals[$Stage] -or
        [string]$Guard.m_sExpectedSource -cne
            $script:ExpectedSources[$Stage] -or
        [int]$Guard.m_iExpectedSentinelGeneration -ne
            $script:ExpectedSentinelGenerations[$Stage] -or
        [string]$Guard.m_sExpectedSaveType -cne
            $script:ExpectedSaveTypes[$Stage] -or
        [string]$Guard.m_sExpectedSaveName -cne
            $script:ExpectedSaveNames[$Stage] -or
        [string]$Guard.m_sWorld -cne $ExpectedWorld -or
        $Guard.m_bAllowCanonicalCampaignOverwrite -isnot [bool] -or
        [bool]$Guard.m_bAllowCanonicalCampaignOverwrite -ne $allowWrite -or
        $Guard.m_bMixedNativeProofRequired -isnot [bool] -or
        -not [bool]$Guard.m_bMixedNativeProofRequired) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity $Guard $ExpectedBuild $label
}

function Assert-StageResult {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld,
        [AllowEmptyString()][string]$ExpectedPriorSavePointId,
        [AllowEmptyString()][string]$ExpectedSourceFingerprint = "",
        [AllowEmptyString()][string]$ExpectedSentinelFingerprint = ""
    )

    $label = "$Stage result"
    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sSessionNonce",
        "m_sStageNonce",
        "m_sRunId",
        "m_sPayloadNonce",
        "m_sStage",
        "m_iStageOrdinal",
        "m_bSuccess",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion",
        "m_sWorld",
        "m_sExpectedSource",
        "m_sSource",
        "m_sSourceFingerprint",
        "m_bSourceExact",
        "m_bPersistenceSystemAvailable",
        "m_bWasDataLoaded",
        "m_bNativeRecordPresent",
        "m_bNativeRecordValid",
        "m_bDegradedNativeRecovery",
        "m_sDegradedNativeRecoveryReason",
        "m_iSourceJournalGeneration",
        "m_sSourceJournalSlot",
        "m_iSourceJournalValidSlotCount",
        "m_bSourceJournalLegacyRaw",
        "m_bSourceJournalChainExact",
        "m_iExpectedSentinelGeneration",
        "m_iSentinelGeneration",
        "m_sExpectedSentinelFingerprint",
        "m_sSentinelFingerprint",
        "m_bSentinelExact",
        "m_sExpectedPriorSavePointId",
        "m_sObservedPriorSavePointId",
        "m_sActiveSaveType",
        "m_sActiveSaveName",
        "m_sExpectedCurrentSavePointId",
        "m_sCreatedSavePointId",
        "m_sActiveSavePointId",
        "m_bPriorSavePointExact",
        "m_bActiveSavePointExact",
        "m_sExpectedSaveType",
        "m_sCreatedSaveType",
        "m_sExpectedSaveName",
        "m_sCreatedSaveName",
        "m_bNativePayloadPrepared",
        "m_bSavePointRequested",
        "m_iExpectedRequestFlags",
        "m_iObservedRequestFlags",
        "m_bRequestFlagsExact",
        "m_bCompletionCallbackSucceeded",
        "m_bOnAfterSaveObserved",
        "m_bOnAfterSaveSucceeded",
        "m_bOnSaveCreatedObserved",
        "m_bSchedulerExercised",
        "m_bSchedulerThresholdCrossed",
        "m_bSchedulerMajorChangePendingAtAttempt",
        "m_bSchedulerDebounceRemarked",
        "m_bSchedulerDebounceHeld",
        "m_sSchedulerOrigin",
        "m_iSchedulerAttemptSequence",
        "m_iSchedulerTickCountAtAttempt",
        "m_iSchedulerAutosaveIntervalSeconds",
        "m_iSchedulerMajorChangeDebounceSeconds",
        "m_fSchedulerCumulativeSecondsAtAttempt",
        "m_fSchedulerDebounceRemarkElapsedSeconds",
        "m_fSchedulerAutosaveElapsedBeforeSeconds",
        "m_fSchedulerAutosaveElapsedAtAttemptSeconds",
        "m_fSchedulerMajorChangeElapsedBeforeSeconds",
        "m_fSchedulerMajorChangeElapsedAtAttemptSeconds",
        "m_sExpectedProfileFallbackFingerprint",
        "m_sProfileFallbackReadBackFingerprint",
        "m_bProfileFallbackReadBackExact",
        "m_iProfileJournalGeneration",
        "m_sProfileJournalSlot",
        "m_iProfileJournalValidSlotCount",
        "m_bProfileJournalLegacyRaw",
        "m_bProfileJournalChainExact",
        "m_sFieldVehicleProofPhase",
        "m_iFieldVehicleExpectedDurableRows",
        "m_iFieldVehicleObservedDurableRows",
        "m_iFieldVehicleExpectedLiveRoots",
        "m_iFieldVehicleObservedLiveRoots",
        "m_iFieldVehicleExpectedDeletedRows",
        "m_iFieldVehicleObservedDeletedRows",
        "m_iFieldVehicleExpectedCargoRows",
        "m_iFieldVehicleObservedCargoRows",
        "m_iFieldVehicleRestoreEligibleRows",
        "m_iFieldVehicleRestoreInactiveRows",
        "m_iFieldVehicleRetiredNativeTombstoneRoots",
        "m_iFieldVehicleRestoreAdoptedRoots",
        "m_iFieldVehicleRestoreSpawnedRoots",
        "m_iFieldVehicleRestoreTrackedRoots",
        "m_iFieldVehicleRestoreFailedRows",
        "m_iFieldVehicleRestoreAmbiguousRows",
        "m_iFieldVehicleNativeTrackedRoots",
        "m_iFieldVehicleShutdownQuiescedRoots",
        "m_bFieldVehicleRestoreExact",
        "m_bFieldVehicleStateExact",
        "m_bFieldVehiclePhysicalExact",
        "m_bFieldVehicleCargoExact",
        "m_bFieldVehicleNoDuplicateRoots",
        "m_bFieldVehicleNativeAuthorityDetached",
        "m_bFieldVehicleShutdownQuiescenceRequired",
        "m_bFieldVehicleShutdownQuiescenceExact",
        "m_bFieldVehicleMutationApplied",
        "m_bFieldVehicleProofExact",
        "m_sFieldVehicleEvidence",
        "m_bMixedNativeProofRequired",
        "m_sMixedNativeProofPhase",
        "m_sMixedNativeExpectedFingerprint",
        "m_sMixedNativeObservedFingerprint",
        "m_iMixedNativeExpectedCaptiveCount",
        "m_iMixedNativeObservedCaptiveCount",
        "m_iMixedNativeExpectedCarrierCount",
        "m_iMixedNativeObservedCarrierCount",
        "m_iMixedNativeExpectedActiveGroupCount",
        "m_iMixedNativeObservedActiveGroupCount",
        "m_iMixedNativeExpectedGuardLivingCount",
        "m_iMixedNativeObservedGuardLivingCount",
        "m_iMixedNativeExpectedAdapterHandleCount",
        "m_iMixedNativeObservedAdapterHandleCount",
        "m_bMixedNativeClientConnected",
        "m_bMixedNativePlayerSpawned",
        "m_bMixedNativeForeignOccupantRejected",
        "m_bMixedNativeForeignOccupantCleanupExact",
        "m_bMixedNativePlayerReleaseRejected",
        "m_bMixedNativePlayerReleased",
        "m_bMixedNativeProductionRetryObserved",
        "m_bMixedNativeReadOnlyPreflightExact",
        "m_bMixedNativeLatchesClearOnRejection",
        "m_bMixedNativeFollowingExact",
        "m_bMixedNativeSeatlessBoardingExact",
        "m_bMixedNativeBoardedSeatExact",
        "m_bMixedNativeCarrierScopeExact",
        "m_bMixedNativeActiveGroupExact",
        "m_bMixedNativeLootLatchExact",
        "m_bMixedNativeActiveGroupLatchExact",
        "m_bMixedNativeFieldVehicleLatchExact",
        "m_bMixedNativeRescueLatchExact",
        "m_bMixedNativeMaintainExact",
        "m_bMixedNativeQuiescenceExact",
        "m_bMixedNativeFieldVehicleCorrelationExact",
        "m_bMixedNativeDurableCountsExact",
        "m_bMixedNativePoseExact",
        "m_bMixedNativeTopologyExact",
        "m_bMixedNativeLogicalFingerprintExact",
        "m_bMixedNativeProofExact",
        "m_sMixedNativeEvidence",
        "m_sEvidence")) {
        Assert-JsonProperty $Result $property $label
    }
    if ([string]$Result.m_sMagic -cne $script:ResultMagic -or
        [int]$Result.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Result.m_sSessionNonce -cne $SessionNonce -or
        [string]$Result.m_sStageNonce -cne $StageNonce -or
        [string]$Result.m_sRunId -cne $RunId -or
        [string]$Result.m_sPayloadNonce -cne $PayloadNonce -or
        [string]$Result.m_sStage -cne $Stage -or
        [int]$Result.m_iStageOrdinal -ne $script:StageOrdinals[$Stage] -or
        [string]$Result.m_sWorld -cne $ExpectedWorld -or
        [string]$Result.m_sExpectedSource -cne
            $script:ExpectedSources[$Stage] -or
        [string]$Result.m_sSource -cne $script:ExpectedSources[$Stage] -or
        [string]$Result.m_sExpectedSaveType -cne
            $script:ExpectedSaveTypes[$Stage] -or
        [string]$Result.m_sExpectedSaveName -cne
            $script:ExpectedSaveNames[$Stage] -or
        [string]$Result.m_sActiveSaveType -cne
            $script:ExpectedActiveSaveTypes[$Stage] -or
        [string]$Result.m_sActiveSaveName -cne
            $script:ExpectedActiveSaveNames[$Stage] -or
        [int]$Result.m_iExpectedSentinelGeneration -ne
            $script:ExpectedSentinelGenerations[$Stage] -or
        [int]$Result.m_iSentinelGeneration -ne
            $script:ExpectedSentinelGenerations[$Stage] -or
        [string]$Result.m_sExpectedPriorSavePointId -cne
            $ExpectedPriorSavePointId) {
        $identityEvidence = [string]$Result.m_sEvidence
        if ($identityEvidence.Length -gt 300) {
            $identityEvidence = $identityEvidence.Substring(0, 300)
        }
        $identityEvidence = ConvertTo-SafeEvidenceLine -Line $identityEvidence
        $identityDetail = [ordered]@{
            MagicExact = [string]$Result.m_sMagic -ceq $script:ResultMagic
            VersionExact =
                [int]$Result.m_iVersion -eq $script:AuthorityVersion
            NoncesExact =
                [string]$Result.m_sSessionNonce -ceq $SessionNonce -and
                [string]$Result.m_sStageNonce -ceq $StageNonce -and
                [string]$Result.m_sRunId -ceq $RunId -and
                [string]$Result.m_sPayloadNonce -ceq $PayloadNonce
            Stage = [string]$Result.m_sStage
            StageOrdinal = [int]$Result.m_iStageOrdinal
            WorldExact = [string]$Result.m_sWorld -ceq $ExpectedWorld
            ExpectedSource = [string]$Result.m_sExpectedSource
            Source = [string]$Result.m_sSource
            ExpectedSaveType = [string]$Result.m_sExpectedSaveType
            ExpectedSaveName = [string]$Result.m_sExpectedSaveName
            ActiveSaveType = [string]$Result.m_sActiveSaveType
            ActiveSaveName = [string]$Result.m_sActiveSaveName
            ExpectedSentinelGeneration =
                [int]$Result.m_iExpectedSentinelGeneration
            SentinelGeneration = [int]$Result.m_iSentinelGeneration
            PriorIdExact =
                [string]$Result.m_sExpectedPriorSavePointId -ceq
                    $ExpectedPriorSavePointId
            EvidenceHead = $identityEvidence
        } | ConvertTo-Json -Compress
        Write-Host ("IDENTITY_DETAIL " + $identityDetail)
        throw "$label identity or source selection is not exact."
    }
    Assert-BuildIdentity $Result $ExpectedBuild $label
    foreach ($property in @(
        "m_bSuccess",
        "m_bSourceExact",
        "m_bPersistenceSystemAvailable",
        "m_bWasDataLoaded",
        "m_bNativeRecordPresent",
        "m_bNativeRecordValid",
        "m_bDegradedNativeRecovery",
        "m_bSourceJournalLegacyRaw",
        "m_bSourceJournalChainExact",
        "m_bSentinelExact",
        "m_bPriorSavePointExact",
        "m_bActiveSavePointExact",
        "m_bNativePayloadPrepared",
        "m_bSavePointRequested",
        "m_bRequestFlagsExact",
        "m_bCompletionCallbackSucceeded",
        "m_bOnAfterSaveObserved",
        "m_bOnAfterSaveSucceeded",
        "m_bOnSaveCreatedObserved",
        "m_bSchedulerExercised",
        "m_bSchedulerThresholdCrossed",
        "m_bSchedulerMajorChangePendingAtAttempt",
        "m_bSchedulerDebounceRemarked",
        "m_bSchedulerDebounceHeld",
        "m_bProfileFallbackReadBackExact",
        "m_bProfileJournalLegacyRaw",
        "m_bProfileJournalChainExact",
        "m_bFieldVehicleRestoreExact",
        "m_bFieldVehicleStateExact",
        "m_bFieldVehiclePhysicalExact",
        "m_bFieldVehicleCargoExact",
        "m_bFieldVehicleNoDuplicateRoots",
        "m_bFieldVehicleNativeAuthorityDetached",
        "m_bFieldVehicleShutdownQuiescenceRequired",
        "m_bFieldVehicleShutdownQuiescenceExact",
        "m_bFieldVehicleMutationApplied",
        "m_bFieldVehicleProofExact",
        "m_bMixedNativeProofRequired",
        "m_bMixedNativeClientConnected",
        "m_bMixedNativePlayerSpawned",
        "m_bMixedNativeForeignOccupantRejected",
        "m_bMixedNativeForeignOccupantCleanupExact",
        "m_bMixedNativePlayerReleaseRejected",
        "m_bMixedNativePlayerReleased",
        "m_bMixedNativeProductionRetryObserved",
        "m_bMixedNativeReadOnlyPreflightExact",
        "m_bMixedNativeLatchesClearOnRejection",
        "m_bMixedNativeFollowingExact",
        "m_bMixedNativeSeatlessBoardingExact",
        "m_bMixedNativeBoardedSeatExact",
        "m_bMixedNativeCarrierScopeExact",
        "m_bMixedNativeActiveGroupExact",
        "m_bMixedNativeLootLatchExact",
        "m_bMixedNativeActiveGroupLatchExact",
        "m_bMixedNativeFieldVehicleLatchExact",
        "m_bMixedNativeRescueLatchExact",
        "m_bMixedNativeMaintainExact",
        "m_bMixedNativeQuiescenceExact",
        "m_bMixedNativeFieldVehicleCorrelationExact",
        "m_bMixedNativeDurableCountsExact",
        "m_bMixedNativePoseExact",
        "m_bMixedNativeTopologyExact",
        "m_bMixedNativeLogicalFingerprintExact",
        "m_bMixedNativeProofExact")) {
        if ($Result.$property -isnot [bool]) {
            throw "$label contains a non-boolean invariant."
        }
    }
    if (-not [bool]$Result.m_bSuccess) {
        $safeFieldEvidence = ConvertTo-SafeEvidenceLine `
            -Line ([string]$Result.m_sFieldVehicleEvidence)
        if (-not [string]::IsNullOrEmpty($safeFieldEvidence) -and
            $safeFieldEvidence.Length -gt 300) {
            $safeFieldEvidence = $safeFieldEvidence.Substring(
                $safeFieldEvidence.Length - 300)
        }
        $failureEvidence = [string]$Result.m_sEvidence
        $failureHead = $failureEvidence
        $failureTail = $failureEvidence
        if ($failureHead.Length -gt 640) {
            $failureHead = $failureHead.Substring(0, 640)
        }
        if ($failureTail.Length -gt 320) {
            $failureTail = $failureTail.Substring($failureTail.Length - 320)
        }
        $safeFailureHead = ConvertTo-SafeEvidenceLine `
            -Line $failureHead
        $safeFailureEvidence = ConvertTo-SafeEvidenceLine `
            -Line $failureTail
        $completionDetail = ""
        foreach ($completionMarker in @(
                "native commit completed",
                "native replication completed",
                "profile journal save failed",
                "checkpoint commit timed out")) {
            $completionIndex = $failureEvidence.IndexOf(
                $completionMarker,
                [System.StringComparison]::OrdinalIgnoreCase)
            if ($completionIndex -lt 0) {
                continue
            }
            $completionLength = [Math]::Min(
                420,
                $failureEvidence.Length - $completionIndex)
            $completionDetail = ConvertTo-SafeEvidenceLine `
                -Line $failureEvidence.Substring(
                    $completionIndex,
                    $completionLength)
            break
        }
        $failureFlags = "source/prior/native/request/flags/fallback/callback/after/created/active/scheduler/field/sentinel={0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}/{11}/{12}" -f `
            [bool]$Result.m_bSourceExact,
            [bool]$Result.m_bPriorSavePointExact,
            [bool]$Result.m_bNativePayloadPrepared,
            [bool]$Result.m_bSavePointRequested,
            [bool]$Result.m_bRequestFlagsExact,
            [bool]$Result.m_bProfileFallbackReadBackExact,
            [bool]$Result.m_bCompletionCallbackSucceeded,
            [bool]$Result.m_bOnAfterSaveSucceeded,
            [bool]$Result.m_bOnSaveCreatedObserved,
            [bool]$Result.m_bActiveSavePointExact,
            [bool]$Result.m_bSchedulerExercised,
            [bool]$Result.m_bFieldVehicleProofExact,
            [bool]$Result.m_bSentinelExact
        $failureDetail = [ordered]@{
            EvidenceHead = $safeFailureHead
            CompletionDetail = $completionDetail
            FieldTail = $safeFieldEvidence
            EvidenceTail = $safeFailureEvidence
            MixedPortable = "{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}" -f `
                [bool]$Result.m_bMixedNativeFollowingExact,
                [bool]$Result.m_bMixedNativeSeatlessBoardingExact,
                [bool]$Result.m_bMixedNativeBoardedSeatExact,
                [bool]$Result.m_bMixedNativeCarrierScopeExact,
                [bool]$Result.m_bMixedNativeActiveGroupExact,
                [bool]$Result.m_bMixedNativeFieldVehicleCorrelationExact,
                [bool]$Result.m_bMixedNativeDurableCountsExact,
                [bool]$Result.m_bMixedNativePoseExact,
                [bool]$Result.m_bMixedNativeTopologyExact,
                [bool]$Result.m_bMixedNativeLogicalFingerprintExact,
                [bool]$Result.m_bMixedNativeProofExact
            MixedExpectedFingerprint =
                [string]$Result.m_sMixedNativeExpectedFingerprint
            MixedObservedFingerprint =
                [string]$Result.m_sMixedNativeObservedFingerprint
            JournalGeneration = [int]$Result.m_iProfileJournalGeneration
            JournalSlot = [string]$Result.m_sProfileJournalSlot
            JournalValid = [int]$Result.m_iProfileJournalValidSlotCount
            JournalChain = [bool]$Result.m_bProfileJournalChainExact
            FingerprintsEqual =
                [string]$Result.m_sExpectedProfileFallbackFingerprint -ceq
                    [string]$Result.m_sProfileFallbackReadBackFingerprint
        } | ConvertTo-Json -Compress
        Write-Host ("FAILURE_DETAIL " + $failureDetail)
        throw "$label reported failure | $failureFlags"
    }
    if (-not [bool]$Result.m_bMixedNativeProofRequired -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sMixedNativeEvidence)) {
        throw "$label omitted required mixed-native proof authority."
    }
    $mixedPortableFlags = @(
        "m_bMixedNativeFollowingExact",
        "m_bMixedNativeSeatlessBoardingExact",
        "m_bMixedNativeBoardedSeatExact",
        "m_bMixedNativeCarrierScopeExact",
        "m_bMixedNativeActiveGroupExact",
        "m_bMixedNativeFieldVehicleCorrelationExact",
        "m_bMixedNativeDurableCountsExact",
        "m_bMixedNativePoseExact",
        "m_bMixedNativeTopologyExact",
        "m_bMixedNativeLogicalFingerprintExact",
        "m_bMixedNativeProofExact")
    $mixedShutdownFlags = @(
        "m_bMixedNativeClientConnected",
        "m_bMixedNativePlayerSpawned",
        "m_bMixedNativeForeignOccupantRejected",
        "m_bMixedNativeForeignOccupantCleanupExact",
        "m_bMixedNativePlayerReleaseRejected",
        "m_bMixedNativePlayerReleased",
        "m_bMixedNativeProductionRetryObserved",
        "m_bMixedNativeReadOnlyPreflightExact",
        "m_bMixedNativeLatchesClearOnRejection",
        "m_bMixedNativeLootLatchExact",
        "m_bMixedNativeActiveGroupLatchExact",
        "m_bMixedNativeFieldVehicleLatchExact",
        "m_bMixedNativeRescueLatchExact",
        "m_bMixedNativeMaintainExact",
        "m_bMixedNativeQuiescenceExact")
    $mixedCountProperties = @(
        "m_iMixedNativeExpectedCaptiveCount",
        "m_iMixedNativeObservedCaptiveCount",
        "m_iMixedNativeExpectedCarrierCount",
        "m_iMixedNativeObservedCarrierCount",
        "m_iMixedNativeExpectedActiveGroupCount",
        "m_iMixedNativeObservedActiveGroupCount",
        "m_iMixedNativeExpectedGuardLivingCount",
        "m_iMixedNativeObservedGuardLivingCount",
        "m_iMixedNativeExpectedAdapterHandleCount",
        "m_iMixedNativeObservedAdapterHandleCount")
    if ($Stage -in @("autosave_checkpoint", "manual_checkpoint")) {
        if ([string]$Result.m_sMixedNativeProofPhase -cne
                "not_applicable" -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sMixedNativeExpectedFingerprint) -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sMixedNativeObservedFingerprint)) {
            throw "$label leaked mixed-native authority into an inactive stage."
        }
        foreach ($property in $mixedCountProperties) {
            if ([int]$Result.$property -ne 0) {
                throw "$label leaked mixed-native counts into an inactive stage."
            }
        }
        foreach ($property in @($mixedPortableFlags + $mixedShutdownFlags)) {
            if ([bool]$Result.$property) {
                throw "$label leaked mixed-native evidence into an inactive stage."
            }
        }
    }
    else {
        $expectedMixedPhase = @{
            shutdown_checkpoint = "shutdown_native"
            native_shutdown_verify = "native_restart"
            profile_fallback_verify = "fallback_restart"
        }[$Stage]
        if ([string]::IsNullOrWhiteSpace($expectedMixedPhase) -or
            [string]$Result.m_sMixedNativeProofPhase -cne
                $expectedMixedPhase -or
            [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sMixedNativeExpectedFingerprint) -or
            [string]$Result.m_sMixedNativeObservedFingerprint -cne
                [string]$Result.m_sMixedNativeExpectedFingerprint -or
            [int]$Result.m_iMixedNativeExpectedCaptiveCount -ne 3 -or
            [int]$Result.m_iMixedNativeObservedCaptiveCount -ne 3 -or
            [int]$Result.m_iMixedNativeExpectedCarrierCount -ne 1 -or
            [int]$Result.m_iMixedNativeObservedCarrierCount -ne 1 -or
            [int]$Result.m_iMixedNativeExpectedActiveGroupCount -ne 1 -or
            [int]$Result.m_iMixedNativeObservedActiveGroupCount -ne 1 -or
            [int]$Result.m_iMixedNativeExpectedGuardLivingCount -lt 1 -or
            [int]$Result.m_iMixedNativeObservedGuardLivingCount -ne
                [int]$Result.m_iMixedNativeExpectedGuardLivingCount -or
            [int]$Result.m_iMixedNativeExpectedAdapterHandleCount -lt
                [int]$Result.m_iMixedNativeExpectedGuardLivingCount -or
            [int]$Result.m_iMixedNativeObservedAdapterHandleCount -ne
                [int]$Result.m_iMixedNativeExpectedAdapterHandleCount) {
            throw "$label mixed-native portable counts are not exact."
        }
        foreach ($property in $mixedPortableFlags) {
            if (-not [bool]$Result.$property) {
                throw "$label omitted portable mixed-native evidence $property."
            }
        }
        $expectShutdownEvidence = $Stage -ceq "shutdown_checkpoint"
        foreach ($property in $mixedShutdownFlags) {
            if ([bool]$Result.$property -ne $expectShutdownEvidence) {
                throw "$label has non-exact shutdown-only evidence $property."
            }
        }
    }
    if (-not [bool]$Result.m_bSuccess -or
        -not [bool]$Result.m_bSourceExact -or
        -not [bool]$Result.m_bPersistenceSystemAvailable -or
        -not [bool]$Result.m_bSentinelExact -or
        -not [bool]$Result.m_bProfileFallbackReadBackExact -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sExpectedSentinelFingerprint) -or
        [string]$Result.m_sExpectedSentinelFingerprint -cne
            [string]$Result.m_sSentinelFingerprint -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sExpectedProfileFallbackFingerprint) -or
        [string]$Result.m_sExpectedProfileFallbackFingerprint -cne
            [string]$Result.m_sProfileFallbackReadBackFingerprint -or
        [bool]$Result.m_bDegradedNativeRecovery -or
        -not [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sDegradedNativeRecoveryReason) -or
        -not [bool]$Result.m_bFieldVehicleRestoreExact -or
        -not [bool]$Result.m_bFieldVehicleStateExact -or
        -not [bool]$Result.m_bFieldVehiclePhysicalExact -or
        -not [bool]$Result.m_bFieldVehicleCargoExact -or
        -not [bool]$Result.m_bFieldVehicleNoDuplicateRoots -or
        -not [bool]$Result.m_bFieldVehicleNativeAuthorityDetached -or
        -not [bool]$Result.m_bFieldVehicleShutdownQuiescenceExact -or
        -not [bool]$Result.m_bFieldVehicleProofExact -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sFieldVehicleEvidence) -or
        [string]::IsNullOrWhiteSpace([string]$Result.m_sEvidence)) {
        throw "$label omitted a required success invariant."
    }
    $expectedJournalGeneration =
        [int]$script:ExpectedSentinelGenerations[$Stage]
    $expectedJournalSlot = "canonical"
    if (($expectedJournalGeneration % 2) -eq 0) {
        $expectedJournalSlot = "recovery"
    }
    $expectedJournalFileCount = 2
    if ($expectedJournalGeneration -eq 1) {
        $expectedJournalFileCount = 1
    }
    if ([int]$Result.m_iProfileJournalGeneration -ne
            $expectedJournalGeneration -or
        [string]$Result.m_sProfileJournalSlot -cne
            $expectedJournalSlot -or
        [int]$Result.m_iProfileJournalValidSlotCount -ne
            $expectedJournalFileCount -or
        [bool]$Result.m_bProfileJournalLegacyRaw -or
        -not [bool]$Result.m_bProfileJournalChainExact) {
        throw "$label did not prove its exact alternating journal generation."
    }
    $expectedFieldVehicle = $script:ExpectedFieldVehicleResults[$Stage]
    if (-not $expectedFieldVehicle) {
        throw "$label has no field-vehicle expectation."
    }
    $restoredFieldVehicleRoots =
        [int]$Result.m_iFieldVehicleRestoreAdoptedRoots +
        [int]$Result.m_iFieldVehicleRestoreSpawnedRoots
    if ([string]$Result.m_sFieldVehicleProofPhase -cne
            [string]$expectedFieldVehicle.Phase -or
        [int]$Result.m_iFieldVehicleExpectedDurableRows -ne
            [int]$expectedFieldVehicle.DurableRows -or
        [int]$Result.m_iFieldVehicleObservedDurableRows -ne
            [int]$expectedFieldVehicle.DurableRows -or
        [int]$Result.m_iFieldVehicleExpectedLiveRoots -ne
            [int]$expectedFieldVehicle.LiveRoots -or
        [int]$Result.m_iFieldVehicleObservedLiveRoots -ne
            [int]$expectedFieldVehicle.LiveRoots -or
        [int]$Result.m_iFieldVehicleExpectedDeletedRows -ne
            [int]$expectedFieldVehicle.DeletedRows -or
        [int]$Result.m_iFieldVehicleObservedDeletedRows -ne
            [int]$expectedFieldVehicle.DeletedRows -or
        [int]$Result.m_iFieldVehicleExpectedCargoRows -ne
            [int]$expectedFieldVehicle.CargoRows -or
        [int]$Result.m_iFieldVehicleObservedCargoRows -ne
            [int]$expectedFieldVehicle.CargoRows -or
        [int]$Result.m_iFieldVehicleRestoreEligibleRows -ne
            [int]$expectedFieldVehicle.RestoreEligibleRows -or
        [int]$Result.m_iFieldVehicleRestoreInactiveRows -ne
            [int]$expectedFieldVehicle.RestoreInactiveRows -or
        [int]$Result.m_iFieldVehicleRetiredNativeTombstoneRoots -ne 0 -or
        [int]$Result.m_iFieldVehicleRestoreAdoptedRoots -ne 0 -or
        [int]$Result.m_iFieldVehicleRestoreSpawnedRoots -ne
            [int]$expectedFieldVehicle.RestoreRoots -or
        $restoredFieldVehicleRoots -ne
            [int]$expectedFieldVehicle.RestoreRoots -or
        [int]$Result.m_iFieldVehicleRestoreTrackedRoots -ne
            [int]$expectedFieldVehicle.RestoreTrackedRoots -or
        [int]$Result.m_iFieldVehicleRestoreFailedRows -ne 0 -or
        [int]$Result.m_iFieldVehicleRestoreAmbiguousRows -ne 0 -or
        [int]$Result.m_iFieldVehicleNativeTrackedRoots -ne 0 -or
        [int]$Result.m_iFieldVehicleShutdownQuiescedRoots -ne
            [int]$expectedFieldVehicle.ShutdownQuiescedRoots -or
        [bool]$Result.m_bFieldVehicleShutdownQuiescenceRequired -ne
            ($Stage -ceq "shutdown_checkpoint") -or
        [bool]$Result.m_bFieldVehicleMutationApplied -ne
            [bool]$expectedFieldVehicle.MutationApplied) {
        throw "$label did not prove its exact field-vehicle phase."
    }
    $expectedSourceJournalGeneration =
        [int]$script:ExpectedSourceJournalGenerations[$Stage]
    $expectedSourceJournalSlot =
        [string]$script:ExpectedSourceJournalSlots[$Stage]
    $expectedSourceJournalValidSlotCount =
        [int]$script:ExpectedSourceJournalValidSlotCounts[$Stage]
    $expectedSourceJournalChainExact =
        $expectedSourceJournalGeneration -ge 1
    if ([int]$Result.m_iSourceJournalGeneration -ne
            $expectedSourceJournalGeneration -or
        [string]$Result.m_sSourceJournalSlot -cne
            $expectedSourceJournalSlot -or
        [int]$Result.m_iSourceJournalValidSlotCount -ne
            $expectedSourceJournalValidSlotCount -or
        [bool]$Result.m_bSourceJournalLegacyRaw -or
        [bool]$Result.m_bSourceJournalChainExact -ne
            $expectedSourceJournalChainExact) {
        throw "$label did not prove the exact startup journal authority."
    }
    if ($Stage -ceq "autosave_checkpoint") {
        if (-not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sSourceFingerprint) -or
            [bool]$Result.m_bWasDataLoaded -or
            [bool]$Result.m_bNativeRecordPresent -or
            [bool]$Result.m_bNativeRecordValid) {
            throw "$label invented startup persistence authority."
        }
    }
    elseif ($Stage -ceq "profile_fallback_verify") {
        if ([string]::IsNullOrWhiteSpace(
                [string]$Result.m_sSourceFingerprint) -or
            [bool]$Result.m_bWasDataLoaded -or
            [bool]$Result.m_bNativeRecordPresent -or
            [bool]$Result.m_bNativeRecordValid) {
            throw "$label did not isolate the profile fallback source."
        }
    }
    elseif ([string]::IsNullOrWhiteSpace(
            [string]$Result.m_sSourceFingerprint) -or
        -not [bool]$Result.m_bWasDataLoaded -or
        -not [bool]$Result.m_bNativeRecordPresent -or
        -not [bool]$Result.m_bNativeRecordValid) {
        throw "$label did not prove a valid native restore."
    }
    if ([string]$Result.m_sSourceFingerprint -cne
        $ExpectedSourceFingerprint) {
        throw "$label did not continue the exact expected source fingerprint."
    }
    if (-not [string]::IsNullOrWhiteSpace($ExpectedSentinelFingerprint) -and
        [string]$Result.m_sSentinelFingerprint -cne
            $ExpectedSentinelFingerprint) {
        throw "$label sentinel fingerprint changed during verification."
    }
    if ($Stage -ceq "autosave_checkpoint") {
        if (-not [bool]$Result.m_bSchedulerExercised -or
            -not [bool]$Result.m_bSchedulerThresholdCrossed -or
            -not [bool]$Result.m_bSchedulerMajorChangePendingAtAttempt -or
            -not [bool]$Result.m_bSchedulerDebounceRemarked -or
            -not [bool]$Result.m_bSchedulerDebounceHeld -or
            [string]$Result.m_sSchedulerOrigin -cne "periodic_autosave" -or
            [int]$Result.m_iSchedulerAttemptSequence -ne 1 -or
            [int]$Result.m_iSchedulerTickCountAtAttempt -le 1 -or
            [int]$Result.m_iSchedulerAutosaveIntervalSeconds -ne
                $script:HSTAutosaveSchedulerIntervalSeconds -or
            [int]$Result.m_iSchedulerMajorChangeDebounceSeconds -ne
                $script:HSTAutosaveSchedulerDebounceSeconds -or
            [double]$Result.m_fSchedulerCumulativeSecondsAtAttempt -lt
                $script:HSTAutosaveSchedulerIntervalSeconds -or
            [double]$Result.m_fSchedulerDebounceRemarkElapsedSeconds -lt
                $script:HSTAutosaveSchedulerRemarkSeconds -or
            [double]$Result.m_fSchedulerDebounceRemarkElapsedSeconds -gt
                ($script:HSTAutosaveSchedulerRemarkSeconds + 5) -or
            [double]$Result.m_fSchedulerAutosaveElapsedBeforeSeconds -ge
                $script:HSTAutosaveSchedulerIntervalSeconds -or
            [double]$Result.m_fSchedulerAutosaveElapsedAtAttemptSeconds -lt
                $script:HSTAutosaveSchedulerIntervalSeconds -or
            [double]$Result.m_fSchedulerAutosaveElapsedAtAttemptSeconds -gt
                ($script:HSTAutosaveSchedulerIntervalSeconds + 10) -or
            [double]$Result.m_fSchedulerMajorChangeElapsedBeforeSeconds -ge
                $script:HSTAutosaveSchedulerDebounceSeconds -or
            [double]$Result.m_fSchedulerMajorChangeElapsedAtAttemptSeconds -lt
                $script:HSTAutosaveSchedulerIntervalSeconds -or
            [double]$Result.m_fSchedulerMajorChangeElapsedAtAttemptSeconds -ge
                $script:HSTAutosaveSchedulerDebounceSeconds) {
            throw "$label did not prove the periodic Tick threshold and held first-edge debounce."
        }
    }
    elseif ([bool]$Result.m_bSchedulerExercised -or
        [bool]$Result.m_bSchedulerThresholdCrossed -or
        [bool]$Result.m_bSchedulerMajorChangePendingAtAttempt -or
        [bool]$Result.m_bSchedulerDebounceRemarked -or
        [bool]$Result.m_bSchedulerDebounceHeld -or
        -not [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sSchedulerOrigin) -or
        [int]$Result.m_iSchedulerAttemptSequence -ne 0 -or
        [int]$Result.m_iSchedulerTickCountAtAttempt -ne 0 -or
        [int]$Result.m_iSchedulerAutosaveIntervalSeconds -ne 0 -or
        [int]$Result.m_iSchedulerMajorChangeDebounceSeconds -ne 0 -or
        [double]$Result.m_fSchedulerCumulativeSecondsAtAttempt -ne 0 -or
        [double]$Result.m_fSchedulerDebounceRemarkElapsedSeconds -ne 0 -or
        [double]$Result.m_fSchedulerAutosaveElapsedBeforeSeconds -ne 0 -or
        [double]$Result.m_fSchedulerAutosaveElapsedAtAttemptSeconds -ne 0 -or
        [double]$Result.m_fSchedulerMajorChangeElapsedBeforeSeconds -ne 0 -or
        [double]$Result.m_fSchedulerMajorChangeElapsedAtAttemptSeconds -ne 0) {
        throw "$label leaked scheduler evidence into a direct or no-save stage."
    }
    $checkpointStage = $script:StageOrdinals[$Stage] -le 2
    if ($checkpointStage) {
        if ([string]$Result.m_sCreatedSaveType -cne
                $script:ExpectedSaveTypes[$Stage] -or
            [string]$Result.m_sCreatedSaveName -cne
                $script:ExpectedSaveNames[$Stage] -or
            -not [bool]$Result.m_bNativePayloadPrepared -or
            -not [bool]$Result.m_bSavePointRequested -or
            [int]$Result.m_iExpectedRequestFlags -ne
                $script:ExpectedRequestFlags[$Stage] -or
            [int]$Result.m_iObservedRequestFlags -ne
                $script:ExpectedRequestFlags[$Stage] -or
            -not [bool]$Result.m_bRequestFlagsExact -or
            -not [bool]$Result.m_bCompletionCallbackSucceeded -or
            -not [bool]$Result.m_bOnAfterSaveObserved -or
            -not [bool]$Result.m_bOnAfterSaveSucceeded -or
            -not [bool]$Result.m_bOnSaveCreatedObserved -or
            -not [bool]$Result.m_bPriorSavePointExact -or
            -not [bool]$Result.m_bActiveSavePointExact -or
            [string]$Result.m_sObservedPriorSavePointId -cne
                $ExpectedPriorSavePointId) {
            throw "$label did not prove the production checkpoint lifecycle."
        }
        Assert-NativeSavePointId `
            ([string]$Result.m_sCreatedSavePointId) `
            "$label committed save point"
        if ([string]$Result.m_sActiveSavePointId -cne
                [string]$Result.m_sCreatedSavePointId -or
            [string]$Result.m_sExpectedCurrentSavePointId -cne
                [string]$Result.m_sCreatedSavePointId -or
            (-not [string]::IsNullOrWhiteSpace($ExpectedPriorSavePointId) -and
                [string]$Result.m_sCreatedSavePointId -ceq
                    $ExpectedPriorSavePointId)) {
            throw "$label did not commit a successor native save."
        }
    }
    else {
        if ([string]$Result.m_sCreatedSaveType -cne "none" -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sCreatedSaveName) -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sCreatedSavePointId) -or
            [bool]$Result.m_bNativePayloadPrepared -or
            [bool]$Result.m_bSavePointRequested -or
            [int]$Result.m_iExpectedRequestFlags -ne 0 -or
            [int]$Result.m_iObservedRequestFlags -ne 0 -or
            [bool]$Result.m_bRequestFlagsExact -or
            [bool]$Result.m_bCompletionCallbackSucceeded -or
            [bool]$Result.m_bOnAfterSaveObserved -or
            [bool]$Result.m_bOnAfterSaveSucceeded -or
            [bool]$Result.m_bOnSaveCreatedObserved -or
            -not [bool]$Result.m_bPriorSavePointExact -or
            -not [bool]$Result.m_bActiveSavePointExact) {
            throw "$label violated its no-save verification gate."
        }
        if ($Stage -ceq "native_shutdown_verify") {
            if ([string]$Result.m_sObservedPriorSavePointId -cne
                    $ExpectedPriorSavePointId -or
                [string]$Result.m_sExpectedCurrentSavePointId -cne
                    $ExpectedPriorSavePointId -or
                [string]$Result.m_sActiveSavePointId -cne
                    $ExpectedPriorSavePointId) {
                throw "$label did not verify the exact shutdown UUID."
            }
        }
        elseif (-not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sObservedPriorSavePointId) -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sExpectedCurrentSavePointId) -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$Result.m_sActiveSavePointId)) {
            throw "$label leaked native-save authority into fallback startup."
        }
    }
    return $Result
}

function Assert-MixedNativeReadyReceipt {
    param(
        [Parameter(Mandatory = $true)]$Receipt,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $label = "shutdown mixed-native readiness receipt"
    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sSessionNonce",
        "m_sStageNonce",
        "m_sRunId",
        "m_sPayloadNonce",
        "m_sStage",
        "m_iStageOrdinal",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion",
        "m_sWorld",
        "m_sPhase",
        "m_bReady",
        "m_sEvidence")) {
        Assert-JsonProperty $Receipt $property $label
    }
    if ([string]$Receipt.m_sMagic -cne $script:MixedNativeReadyMagic -or
        [int]$Receipt.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Receipt.m_sSessionNonce -cne $SessionNonce -or
        [string]$Receipt.m_sStageNonce -cne $StageNonce -or
        [string]$Receipt.m_sRunId -cne $RunId -or
        [string]$Receipt.m_sPayloadNonce -cne $PayloadNonce -or
        [string]$Receipt.m_sStage -cne "shutdown_checkpoint" -or
        [int]$Receipt.m_iStageOrdinal -ne
            $script:StageOrdinals.shutdown_checkpoint -or
        [string]$Receipt.m_sWorld -cne $ExpectedWorld -or
        [string]$Receipt.m_sPhase -cne "wait_client" -or
        $Receipt.m_bReady -isnot [bool] -or
        -not [bool]$Receipt.m_bReady -or
        [string]::IsNullOrWhiteSpace([string]$Receipt.m_sEvidence)) {
        throw "$label authority is not exact."
    }
    Assert-BuildIdentity $Receipt $ExpectedBuild $label
    return $Receipt
}

function Assert-EndBridgeReceipt {
    param(
        [Parameter(Mandatory = $true)]$Receipt,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld,
        [Parameter(Mandatory = $true)][string]$ExpectedShutdownSavePointId
    )

    $label = "shutdown end bridge receipt"
    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sSessionNonce",
        "m_sStageNonce",
        "m_sRunId",
        "m_sPayloadNonce",
        "m_sStage",
        "m_iStageOrdinal",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion",
        "m_sWorld",
        "m_bSuccess",
        "m_bEndGameModeIntercepted",
        "m_bStableCheckpointObserved",
        "m_bRetentionHandlerExecuted",
        "m_bKeepSessionSaveCLIAbsent",
        "m_bPersistenceKeepSessionDataDisabled",
        "m_sExpectedShutdownSavePointId",
        "m_sObservedShutdownSavePointId",
        "m_bShutdownSavePointExact",
        "m_sEvidence")) {
        Assert-JsonProperty $Receipt $property $label
    }
    if ([string]$Receipt.m_sMagic -cne $script:EndBridgeReceiptMagic -or
        [int]$Receipt.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Receipt.m_sSessionNonce -cne $SessionNonce -or
        [string]$Receipt.m_sStageNonce -cne $StageNonce -or
        [string]$Receipt.m_sRunId -cne $RunId -or
        [string]$Receipt.m_sPayloadNonce -cne $PayloadNonce -or
        [string]$Receipt.m_sStage -cne "shutdown_checkpoint" -or
        [int]$Receipt.m_iStageOrdinal -ne
            $script:StageOrdinals.shutdown_checkpoint -or
        [int]$Receipt.m_iSettingsSchemaVersion -ne
            $ExpectedBuild.SettingsSchemaVersion -or
        [string]$Receipt.m_sWorld -cne $ExpectedWorld -or
        [string]::IsNullOrWhiteSpace([string]$Receipt.m_sEvidence)) {
        throw "$label authority is not exact."
    }
    Assert-BuildIdentity $Receipt $ExpectedBuild $label
    foreach ($property in @(
        "m_bSuccess",
        "m_bEndGameModeIntercepted",
        "m_bStableCheckpointObserved",
        "m_bRetentionHandlerExecuted",
        "m_bKeepSessionSaveCLIAbsent",
        "m_bPersistenceKeepSessionDataDisabled",
        "m_bShutdownSavePointExact")) {
        if ($Receipt.$property -isnot [bool] -or
            -not [bool]$Receipt.$property) {
            throw "$label did not prove every controlled-end invariant."
        }
    }
    Assert-NativeSavePointId `
        $ExpectedShutdownSavePointId `
        "$label expected shutdown UUID"
    Assert-NativeSavePointId `
        ([string]$Receipt.m_sExpectedShutdownSavePointId) `
        "$label recorded expected shutdown UUID"
    Assert-NativeSavePointId `
        ([string]$Receipt.m_sObservedShutdownSavePointId) `
        "$label observed shutdown UUID"
    if ([string]$Receipt.m_sExpectedShutdownSavePointId -cne
            $ExpectedShutdownSavePointId -or
        [string]$Receipt.m_sObservedShutdownSavePointId -cne
            $ExpectedShutdownSavePointId) {
        throw "$label did not correlate the exact committed shutdown UUID."
    }
    return $Receipt
}

function Assert-Carrier {
    param(
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld,
        [Parameter(Mandatory = $true)]$Result,
        [AllowEmptyString()][string]$ExpectedLatestSavePointId
    )

    $label = "$Stage carrier"
    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sSessionNonce",
        "m_sRunId",
        "m_sPayloadNonce",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_iSettingsSchemaVersion",
        "m_sWorld",
        "m_sSentinelTaskId",
        "m_iCurrentSentinelGeneration",
        "m_sGeneration1SentinelFingerprint",
        "m_sGeneration2SentinelFingerprint",
        "m_sGeneration3SentinelFingerprint",
        "m_sAutosaveSavePointId",
        "m_sManualSavePointId",
        "m_sShutdownSavePointId",
        "m_sPriorSavePointId",
        "m_sCurrentSavePointId",
        "m_sAutosaveSource",
        "m_sAutosaveSourceFingerprint",
        "m_bAutosaveWasDataLoaded",
        "m_bAutosaveNativeRecordPresent",
        "m_bAutosaveNativeRecordValid",
        "m_sManualSource",
        "m_sManualSourceFingerprint",
        "m_bManualWasDataLoaded",
        "m_bManualNativeRecordPresent",
        "m_bManualNativeRecordValid",
        "m_sShutdownSource",
        "m_sShutdownSourceFingerprint",
        "m_bShutdownWasDataLoaded",
        "m_bShutdownNativeRecordPresent",
        "m_bShutdownNativeRecordValid",
        "m_sAutosaveCreatedSaveType",
        "m_sAutosaveCreatedSaveName",
        "m_sManualCreatedSaveType",
        "m_sManualCreatedSaveName",
        "m_sShutdownCreatedSaveType",
        "m_sShutdownCreatedSaveName",
        "m_sGeneration1ProfileFallbackFingerprint",
        "m_sGeneration2ProfileFallbackFingerprint",
        "m_sGeneration3ProfileFallbackFingerprint",
        "m_bGeneration1ProfileFallbackExact",
        "m_bGeneration2ProfileFallbackExact",
        "m_bGeneration3ProfileFallbackExact",
        "m_sLatestProfileFallbackFingerprint",
        "m_sFieldVehiclePrefab",
        "m_sFieldVehicleCargoPrefab",
        "m_sFieldVehicleAId",
        "m_sFieldVehicleBId",
        "m_vFieldVehicleAInitialPosition",
        "m_vFieldVehicleBInitialPosition",
        "m_vFieldVehicleAMovedPosition",
        "m_vFieldVehicleAShutdownPosition",
        "m_vFieldVehicleAInitialAngles",
        "m_vFieldVehicleBInitialAngles",
        "m_vFieldVehicleAMovedAngles",
        "m_vFieldVehicleAShutdownAngles",
        "m_iFieldVehicleACargoCount",
        "m_iFieldVehicleBCargoCount",
        "m_bFieldVehiclePrepared",
        "m_bFieldVehicleRecoveredAndMutated",
        "m_bFieldVehicleReplayVerified",
        "m_bMixedNativeProofRequired",
        "m_sMixedNativeMissionInstanceId",
        "m_sMixedNativeOperationId",
        "m_sMixedNativeManifestId",
        "m_sMixedNativeBatchId",
        "m_sMixedNativeGuardGroupId",
        "m_sMixedNativeCarrierRuntimeId",
        "m_sMixedNativeCarrierPrefab",
        "m_sMixedNativeFollowingCaptiveId",
        "m_sMixedNativeBoardingCaptiveId",
        "m_sMixedNativeBoardedCaptiveId",
        "m_sMixedNativeSeatToken",
        "m_sMixedNativeShutdownFingerprint",
        "m_vMixedNativeCarrierShutdownPosition",
        "m_vMixedNativeCarrierShutdownAngles",
        "m_vMixedNativeFollowingShutdownPosition",
        "m_vMixedNativeBoardingShutdownPosition",
        "m_vMixedNativeBoardedShutdownPosition",
        "m_iMixedNativeCaptiveCount",
        "m_iMixedNativeCarrierCount",
        "m_iMixedNativeActiveGroupCount",
        "m_iMixedNativeGuardLivingCount",
        "m_iMixedNativeAdapterHandleCount",
        "m_bMixedNativeShutdownPrepared")) {
        Assert-JsonProperty $Carrier $property $label
    }
    if ([string]$Carrier.m_sMagic -cne $script:CarrierMagic -or
        [int]$Carrier.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Carrier.m_sSessionNonce -cne $SessionNonce -or
        [string]$Carrier.m_sRunId -cne $RunId -or
        [string]$Carrier.m_sPayloadNonce -cne $PayloadNonce -or
        [string]$Carrier.m_sWorld -cne $ExpectedWorld -or
        [string]$Carrier.m_sSentinelTaskId -cne
            "hst_ordinary_campaign_persistence_proof_sentinel" -or
        [int]$Carrier.m_iCurrentSentinelGeneration -ne
            $script:ExpectedSentinelGenerations[$Stage]) {
        throw "$label is not exact."
    }
    Assert-BuildIdentity $Carrier $ExpectedBuild $label
    $generation = $script:ExpectedSentinelGenerations[$Stage]
    foreach ($property in @(
        "m_bFieldVehiclePrepared",
        "m_bFieldVehicleRecoveredAndMutated",
        "m_bFieldVehicleReplayVerified",
        "m_bMixedNativeProofRequired",
        "m_bMixedNativeShutdownPrepared")) {
        if ($Carrier.$property -isnot [bool]) {
            throw "$label contains a non-boolean field-vehicle invariant."
        }
    }
    if (-not [bool]$Carrier.m_bMixedNativeProofRequired) {
        throw "$label omitted required mixed-native proof authority."
    }
    $mixedCarrierStringProperties = @(
        "m_sMixedNativeMissionInstanceId",
        "m_sMixedNativeOperationId",
        "m_sMixedNativeManifestId",
        "m_sMixedNativeBatchId",
        "m_sMixedNativeGuardGroupId",
        "m_sMixedNativeCarrierRuntimeId",
        "m_sMixedNativeCarrierPrefab",
        "m_sMixedNativeFollowingCaptiveId",
        "m_sMixedNativeBoardingCaptiveId",
        "m_sMixedNativeBoardedCaptiveId",
        "m_sMixedNativeSeatToken",
        "m_sMixedNativeShutdownFingerprint")
    $mixedCarrierCountProperties = @(
        "m_iMixedNativeCaptiveCount",
        "m_iMixedNativeCarrierCount",
        "m_iMixedNativeActiveGroupCount",
        "m_iMixedNativeGuardLivingCount",
        "m_iMixedNativeAdapterHandleCount")
    $mixedCarrierVectorProperties = @(
        "m_vMixedNativeCarrierShutdownPosition",
        "m_vMixedNativeCarrierShutdownAngles",
        "m_vMixedNativeFollowingShutdownPosition",
        "m_vMixedNativeBoardingShutdownPosition",
        "m_vMixedNativeBoardedShutdownPosition")
    if ($generation -lt 3) {
        foreach ($property in $mixedCarrierStringProperties) {
            if (-not [string]::IsNullOrWhiteSpace(
                [string]$Carrier.$property)) {
                throw "$label leaked a mixed-native plan into an early stage."
            }
        }
        foreach ($property in $mixedCarrierCountProperties) {
            if ([int]$Carrier.$property -ne 0) {
                throw "$label leaked mixed-native counts into an early stage."
            }
        }
        foreach ($property in $mixedCarrierVectorProperties) {
            $vector = Get-FieldVehicleVectorSignature `
                -Value $Carrier.$property `
                -ArtifactLabel "$label $property"
            if (-not $vector.IsZero) {
                throw "$label leaked mixed-native poses into an early stage."
            }
        }
        if ([bool]$Carrier.m_bMixedNativeShutdownPrepared) {
            throw "$label leaked mixed-native preparation into an early stage."
        }
    }
    else {
        foreach ($property in $mixedCarrierStringProperties) {
            if ([string]::IsNullOrWhiteSpace([string]$Carrier.$property)) {
                throw "$label omitted mixed-native plan identity $property."
            }
        }
        $followingId = [string]$Carrier.m_sMixedNativeFollowingCaptiveId
        $boardingId = [string]$Carrier.m_sMixedNativeBoardingCaptiveId
        $boardedId = [string]$Carrier.m_sMixedNativeBoardedCaptiveId
        if (-not [bool]$Carrier.m_bMixedNativeShutdownPrepared -or
            [string]$Carrier.m_sMixedNativeCarrierPrefab -cne
                $script:MixedNativeCarrierPrefab -or
            -not ([string]$Carrier.m_sMixedNativeCarrierRuntimeId).StartsWith(
                "vehicle_", [StringComparison]::Ordinal) -or
            ([string]$Carrier.m_sMixedNativeCarrierRuntimeId).StartsWith(
                "vehicle_local_", [StringComparison]::Ordinal) -or
			-not ([string]$Carrier.m_sMixedNativeSeatToken).StartsWith(
				"seat_v2_", [StringComparison]::Ordinal) -or
            $followingId -ceq $boardingId -or
            $followingId -ceq $boardedId -or
            $boardingId -ceq $boardedId -or
            [int]$Carrier.m_iMixedNativeCaptiveCount -ne 3 -or
            [int]$Carrier.m_iMixedNativeCarrierCount -ne 1 -or
            [int]$Carrier.m_iMixedNativeActiveGroupCount -ne 1 -or
            [int]$Carrier.m_iMixedNativeGuardLivingCount -lt 1 -or
            [int]$Carrier.m_iMixedNativeAdapterHandleCount -lt
                [int]$Carrier.m_iMixedNativeGuardLivingCount -or
            [string]$Carrier.m_sMixedNativeShutdownFingerprint -cne
                [string]$Result.m_sMixedNativeExpectedFingerprint) {
            throw "$label mixed-native plan is not exact."
        }
        foreach ($property in @(
            "m_vMixedNativeCarrierShutdownPosition",
            "m_vMixedNativeFollowingShutdownPosition",
            "m_vMixedNativeBoardingShutdownPosition",
            "m_vMixedNativeBoardedShutdownPosition")) {
            $vector = Get-FieldVehicleVectorSignature `
                -Value $Carrier.$property `
                -ArtifactLabel "$label $property"
            if ($vector.IsZero) {
                throw "$label omitted mixed-native shutdown pose $property."
            }
        }
        if ([int]$Result.m_iMixedNativeObservedCaptiveCount -ne
                [int]$Carrier.m_iMixedNativeCaptiveCount -or
            [int]$Result.m_iMixedNativeObservedCarrierCount -ne
                [int]$Carrier.m_iMixedNativeCarrierCount -or
            [int]$Result.m_iMixedNativeObservedActiveGroupCount -ne
                [int]$Carrier.m_iMixedNativeActiveGroupCount -or
            [int]$Result.m_iMixedNativeObservedGuardLivingCount -ne
                [int]$Carrier.m_iMixedNativeGuardLivingCount -or
            [int]$Result.m_iMixedNativeObservedAdapterHandleCount -ne
                [int]$Carrier.m_iMixedNativeAdapterHandleCount) {
            throw "$label mixed-native result relationship is not exact."
        }
    }
    $fieldVehicleAId = [string]$Carrier.m_sFieldVehicleAId
    $fieldVehicleBId = [string]$Carrier.m_sFieldVehicleBId
    if ([string]$Carrier.m_sFieldVehiclePrefab -cne
            $script:FieldVehiclePrefab -or
        [string]$Carrier.m_sFieldVehicleCargoPrefab -cne
            $script:FieldVehicleCargoPrefab -or
        [string]::IsNullOrWhiteSpace($fieldVehicleAId) -or
        [string]::IsNullOrWhiteSpace($fieldVehicleBId) -or
        $fieldVehicleAId -ceq $fieldVehicleBId -or
        $fieldVehicleAId -match '^(?i:rpl_|local_)' -or
        $fieldVehicleBId -match '^(?i:rpl_|local_)' -or
        [int]$Carrier.m_iFieldVehicleACargoCount -ne 3 -or
        [int]$Carrier.m_iFieldVehicleBCargoCount -ne 7) {
        throw "$label field-vehicle fixture identity is not exact."
    }
    $fieldVehicleVectorSignatures = @()
    foreach ($property in @(
        "m_vFieldVehicleAInitialPosition",
        "m_vFieldVehicleBInitialPosition",
        "m_vFieldVehicleAMovedPosition",
        "m_vFieldVehicleAInitialAngles",
        "m_vFieldVehicleBInitialAngles",
        "m_vFieldVehicleAMovedAngles")) {
        $vector = Get-FieldVehicleVectorSignature `
            -Value $Carrier.$property `
            -ArtifactLabel "$label $property"
        if ($vector.IsZero -or
            $fieldVehicleVectorSignatures -ccontains $vector.Signature) {
            throw "$label field-vehicle vectors are zero or not distinct."
        }
        $fieldVehicleVectorSignatures += $vector.Signature
    }
    $fieldVehicleShutdownPosition = Get-FieldVehicleVectorSignature `
        -Value $Carrier.m_vFieldVehicleAShutdownPosition `
        -ArtifactLabel "$label m_vFieldVehicleAShutdownPosition"
    $fieldVehicleShutdownAngles = Get-FieldVehicleVectorSignature `
        -Value $Carrier.m_vFieldVehicleAShutdownAngles `
        -ArtifactLabel "$label m_vFieldVehicleAShutdownAngles"
    if ($generation -lt 3) {
        if (-not $fieldVehicleShutdownPosition.IsZero -or
            -not $fieldVehicleShutdownAngles.IsZero) {
            throw "$label contains a premature shutdown transform receipt."
        }
    }
    elseif ($fieldVehicleShutdownPosition.IsZero) {
        throw "$label omits its captured shutdown position receipt."
    }
    $expectedRecoveredAndMutated = $generation -ge 2
    $expectedReplayVerified = $generation -ge 3
    if (-not [bool]$Carrier.m_bFieldVehiclePrepared -or
        [bool]$Carrier.m_bFieldVehicleRecoveredAndMutated -ne
            $expectedRecoveredAndMutated -or
        [bool]$Carrier.m_bFieldVehicleReplayVerified -ne
            $expectedReplayVerified) {
        throw "$label field-vehicle generation progress is not exact."
    }
    $sentinelProperty = "m_sGeneration{0}SentinelFingerprint" -f $generation
    $fallbackProperty =
        "m_sGeneration{0}ProfileFallbackFingerprint" -f $generation
    $fallbackExactProperty =
        "m_bGeneration{0}ProfileFallbackExact" -f $generation
    if ([string]$Carrier.$sentinelProperty -cne
            [string]$Result.m_sSentinelFingerprint -or
        [string]$Carrier.$fallbackProperty -cne
            [string]$Result.m_sProfileFallbackReadBackFingerprint -or
        $Carrier.$fallbackExactProperty -isnot [bool] -or
        -not [bool]$Carrier.$fallbackExactProperty -or
        [string]$Carrier.m_sLatestProfileFallbackFingerprint -cne
            [string]$Result.m_sProfileFallbackReadBackFingerprint) {
        throw "$label does not match its stage result."
    }
    Assert-NativeSavePointId `
        ([string]$Carrier.m_sAutosaveSavePointId) `
        "$label autosave UUID"
    if ($generation -ge 2) {
        Assert-NativeSavePointId `
            ([string]$Carrier.m_sManualSavePointId) `
            "$label manual UUID"
        if ([string]$Carrier.m_sManualSavePointId -ceq
            [string]$Carrier.m_sAutosaveSavePointId) {
            throw "$label reused its autosave UUID."
        }
    }
    if ($generation -ge 3) {
        Assert-NativeSavePointId `
            ([string]$Carrier.m_sShutdownSavePointId) `
            "$label shutdown UUID"
        if ([string]$Carrier.m_sShutdownSavePointId -in @(
            [string]$Carrier.m_sAutosaveSavePointId,
            [string]$Carrier.m_sManualSavePointId)) {
            throw "$label reused an earlier checkpoint UUID."
        }
    }
    $expectedPrior = if ($generation -eq 1) { "" }
        elseif ($generation -eq 2) { [string]$Carrier.m_sAutosaveSavePointId }
        else { [string]$Carrier.m_sManualSavePointId }
    $expectedCurrent = if ($generation -eq 1) {
        [string]$Carrier.m_sAutosaveSavePointId
    }
    elseif ($generation -eq 2) {
        [string]$Carrier.m_sManualSavePointId
    }
    else { [string]$Carrier.m_sShutdownSavePointId }
    if ([string]$Carrier.m_sPriorSavePointId -cne $expectedPrior -or
        [string]$Carrier.m_sCurrentSavePointId -cne $expectedCurrent -or
        [string]$Carrier.m_sCurrentSavePointId -cne
            $ExpectedLatestSavePointId -or
        [string]$Carrier.m_sAutosaveCreatedSaveType -cne "auto" -or
        [string]$Carrier.m_sAutosaveCreatedSaveName -cne
            "Partisan autosave") {
        throw "$label checkpoint aliases are not exact."
    }
    if ([string]$Carrier.m_sAutosaveSource -cne "new_campaign" -or
        -not [string]::IsNullOrWhiteSpace(
            [string]$Carrier.m_sAutosaveSourceFingerprint) -or
        [bool]$Carrier.m_bAutosaveWasDataLoaded -or
        [bool]$Carrier.m_bAutosaveNativeRecordPresent -or
        [bool]$Carrier.m_bAutosaveNativeRecordValid) {
        throw "$label autosave source authority is not exact."
    }
    if ($generation -ge 2 -and
        ([string]$Carrier.m_sManualCreatedSaveType -cne "manual" -or
            [string]$Carrier.m_sManualCreatedSaveName -cne
                "Partisan manual checkpoint" -or
            [string]$Carrier.m_sManualSource -cne "native" -or
            [string]$Carrier.m_sManualSourceFingerprint -cne
                [string]$Carrier.m_sGeneration1ProfileFallbackFingerprint -or
            -not [bool]$Carrier.m_bManualWasDataLoaded -or
            -not [bool]$Carrier.m_bManualNativeRecordPresent -or
            -not [bool]$Carrier.m_bManualNativeRecordValid)) {
        throw "$label manual checkpoint metadata is not exact."
    }
    if ($generation -ge 3 -and
        ([string]$Carrier.m_sShutdownCreatedSaveType -cne "shutdown" -or
            [string]$Carrier.m_sShutdownCreatedSaveName -cne
                "Partisan controlled shutdown" -or
            [string]$Carrier.m_sShutdownSource -cne "native" -or
            [string]$Carrier.m_sShutdownSourceFingerprint -cne
                [string]$Carrier.m_sGeneration2ProfileFallbackFingerprint -or
            -not [bool]$Carrier.m_bShutdownWasDataLoaded -or
            -not [bool]$Carrier.m_bShutdownNativeRecordPresent -or
            -not [bool]$Carrier.m_bShutdownNativeRecordValid)) {
        throw "$label shutdown checkpoint metadata is not exact."
    }
    return $Carrier
}

function Get-StageArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$WorldResource,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$Stage,
        [AllowEmptyString()][string]$LoadSavePointId
    )

    $packedProject = Join-Path $PackedAddonsParent "Partisan\addon.gproj"
    $addonSearchPath = $RuntimeAddonPath + "," + $PackedAddonsParent
    $arguments = @(
        "-gproj", $packedProject,
        "-server", $WorldResource,
        "-MissionHeader", $script:MissionHeader,
        "-addonsDir", $addonSearchPath,
        "-addons", $script:ProjectId,
        "-profile", $ProfileRoot,
        "-logLevel", "normal",
        "-logTime", "datetime",
        "-noThrow",
        "-maxFPS", "30")
    if (-not [string]::IsNullOrWhiteSpace($LoadSavePointId)) {
        Assert-NativeSavePointId $LoadSavePointId "$Stage load save point"
        $arguments += @("-loadSessionSave", $LoadSavePointId)
    }
    elseif ($Stage -ceq "autosave_checkpoint") {
        $arguments += "-loadSessionSave"
    }
    if ($Stage -ceq "shutdown_checkpoint") {
        $arguments += "-autoshutdown"
    }
    $arguments += @(
        "-hstOrdinaryCampaignPersistenceProof", "true",
        "-hstOrdinaryCampaignPersistenceStage", $Stage,
        "-hstOrdinaryCampaignPersistenceRunId", $RunId,
        "-hstOrdinaryCampaignPersistenceSessionNonce", $SessionNonce,
        "-hstOrdinaryCampaignPersistenceStageNonce", $StageNonce)

    foreach ($forbidden in @(
        "-world", "-worldSystemsConfig", "-forceupdate", "-backendLocalStorage",
        "-backendDisableStorage", "-noBackend",
        "-rpl-validation-rdb-disable", "-rpl-validation-scr-disable",
        "-rpl-validation-version-disable", "-rpl-validation-devbin-disable",
        "-rpl-validation-addons-disable")) {
        if ($arguments -ccontains $forbidden) {
            throw "$Stage contains forbidden loose-project argument $forbidden."
        }
    }
    $serverIndex = [Array]::IndexOf($arguments, "-server")
    $missionHeaderIndex = [Array]::IndexOf($arguments, "-MissionHeader")
    $addonsIndex = [Array]::IndexOf($arguments, "-addons")
    $gprojIndex = [Array]::IndexOf($arguments, "-gproj")
    $configIndex = [Array]::IndexOf($arguments, "-config")
    $timeoutDisableCount = @($arguments | Where-Object {
        [string]$_ -ceq "-rpl-timeout-disable"
    }).Count
    if ($serverIndex -lt 0 -or
        $serverIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$serverIndex + 1] -cne $WorldResource -or
        $missionHeaderIndex -lt 0 -or
        $missionHeaderIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$missionHeaderIndex + 1] -cne
            $script:MissionHeader -or
        $addonsIndex -lt 0 -or
        $addonsIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$addonsIndex + 1] -cne $script:ProjectId -or
        $gprojIndex -lt 0 -or
        $gprojIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$gprojIndex + 1] -cne $packedProject -or
        $configIndex -ge 0 -or
        $timeoutDisableCount -ne 0) {
        throw "$Stage does not use the exact supported local-add-on server vector."
    }
    if ($arguments -icontains "-keepSessionSave") {
        throw "$Stage must not override disabled session-save retention."
    }
    $autoShutdownCount = @($arguments | Where-Object {
        [string]$_ -ceq "-autoshutdown"
    }).Count
    if (($Stage -ceq "shutdown_checkpoint" -and
            $autoShutdownCount -ne 1) -or
        ($Stage -cne "shutdown_checkpoint" -and
            $autoShutdownCount -ne 0)) {
        throw "$Stage does not have exact controlled autoshutdown authority."
    }
    if ($Stage -ceq "autosave_checkpoint" -and
        $arguments -cnotcontains "-loadSessionSave") {
        throw "$Stage requires one bare native-session bootstrap flag."
    }
    if ($Stage -ceq "profile_fallback_verify" -and
        $arguments -ccontains "-loadSessionSave") {
        throw "$Stage must start without native load authority."
    }
    if ($Stage -notin @(
        "autosave_checkpoint", "profile_fallback_verify") -and
        $arguments -cnotcontains "-loadSessionSave") {
        throw "$Stage requires one explicit native load UUID."
    }
    return $arguments
}

function Get-LoopbackClientArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$AddonTempDirectory,
        [Parameter(Mandatory = $true)][string]$ClientLogDirectory
    )

    $packedProject = Join-Path $PackedAddonsParent "Partisan\addon.gproj"
    $arguments = @(
        "-gproj", $packedProject,
        "-addonsDir", ($RuntimeAddonPath + "," + $PackedAddonsParent),
        "-addons", $script:ProjectId,
        "-addonTempDir", $AddonTempDirectory,
        "-client",
        "-profile", $ProfileRoot,
        "-logsDir", $ClientLogDirectory,
        "-logLevel", "normal",
        "-logTime", "datetime",
        "-window",
        "-noFocus",
        "-forceUpdate",
        "-noSplash",
        "-noSound",
        "-noThrow",
        "-maxFPS", "30")

    foreach ($forbidden in @(
        "-config", "-world", "-server", "-MissionHeader",
        "-worldSystemsConfig", "-loadSessionSave", "-autoshutdown",
        "-keepSessionSave",
        "-hstOrdinaryCampaignPersistenceProof", "-backendLocalStorage",
        "-backendDisableStorage", "-noBackend", "-rpl-timeout-disable",
        "-rpl-validation-rdb-disable", "-rpl-validation-scr-disable",
        "-rpl-validation-version-disable", "-rpl-validation-devbin-disable",
        "-rpl-validation-addons-disable")) {
        if ($arguments -icontains $forbidden) {
            throw "The loopback client contains forbidden authority $forbidden."
        }
    }
    $clientIndex = [Array]::IndexOf($arguments, "-client")
    $addonsIndex = [Array]::IndexOf($arguments, "-addons")
    $profileIndex = [Array]::IndexOf($arguments, "-profile")
    $addonTempIndex = [Array]::IndexOf($arguments, "-addonTempDir")
    $clientLogIndex = [Array]::IndexOf($arguments, "-logsDir")
    $gprojIndex = [Array]::IndexOf($arguments, "-gproj")
    if ($clientIndex -lt 0 -or
        $clientIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$clientIndex + 1] -cne "-profile" -or
        $addonsIndex -lt 0 -or
        $addonsIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$addonsIndex + 1] -cne $script:ProjectId -or
        $profileIndex -lt 0 -or
        $profileIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$profileIndex + 1] -cne $ProfileRoot -or
        $addonTempIndex -lt 0 -or
        $addonTempIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$addonTempIndex + 1] -cne
            $AddonTempDirectory -or
        $clientLogIndex -lt 0 -or
        $clientLogIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$clientLogIndex + 1] -cne
            $ClientLogDirectory -or
        $gprojIndex -lt 0 -or
        $gprojIndex + 1 -ge $arguments.Count -or
        [string]$arguments[$gprojIndex + 1] -cne $packedProject) {
        throw "The loopback client argument vector is not exact."
    }
    return $arguments
}

function New-EngineOwnerValue {
    param(
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)]$BuildIdentity,
        [Parameter(Mandatory = $true)][string]$World
    )

    return [ordered]@{
        m_sMagic = $script:OwnerMagic
        m_iVersion = $script:AuthorityVersion
        m_sPurpose = $script:OwnerPurpose
        m_sSessionNonce = $SessionNonce
        m_sRunId = $RunId
        m_sPayloadNonce = $PayloadNonce
        m_sBuildSha = $BuildIdentity.BuildSha
        m_sBuildUtc = $BuildIdentity.BuildUtc
        m_sBuildLabel = $BuildIdentity.BuildLabel
        m_iCampaignSchemaVersion = $BuildIdentity.CampaignSchemaVersion
        m_iSettingsSchemaVersion = $BuildIdentity.SettingsSchemaVersion
        m_sWorld = $World
        m_iExpectedStageCount = 5
        m_bDisposableProfile = $true
        m_bMixedNativeProofRequired = $true
    }
}

function Invoke-OrdinaryCampaignStage {
    param(
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$DebugDirectory,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$TempDirectory,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$PayloadNonce,
        [Parameter(Mandatory = $true)][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld,
        [AllowEmptyString()][string]$LoadSavePointId,
        [AllowEmptyString()][string]$ExpectedSourceFingerprint,
        [AllowEmptyString()][string]$ExpectedSentinelFingerprint,
        [AllowEmptyString()][string]$ExpectedLatestSavePointId,
        [string]$SteamExecutablePath = "",
        [string]$ClientExecutablePath = "",
        [string]$ClientProfileRoot = "",
        [string]$ClientWorkingDirectory = "",
        [string]$ClientTempDirectory = "",
        [string]$ClientProcessTempDirectory = "",
        [string]$ClientLogDirectory = "",
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    if ($script:Stages -cnotcontains $Stage) {
        throw "Unknown ordinary persistence stage."
    }
    $expectedServerLeaf = "ArmaReforgerServer.exe"
    if ((Split-Path -Leaf $ExecutablePath) -cne $expectedServerLeaf) {
        throw "$Stage does not use its exact server executable class."
    }
    Assert-LowerHexNonce $SessionNonce "session nonce"
    $stageNonce = [Guid]::NewGuid().ToString("N")
    $resultPath = Join-Path $DebugDirectory (
        "HST_OrdinaryCampaignPersistenceProof_{0}.{1}.json" -f
            $RunId, $Stage)
    $guardPath = Join-Path $DebugDirectory (
        "HST_OrdinaryCampaignPersistenceProof_{0}.guard.json" -f $RunId)
    $carrierPath = Join-Path $DebugDirectory (
        "HST_OrdinaryCampaignPersistenceProof_{0}.carrier.json" -f $RunId)
    $endBridgeReceiptPath = Join-Path $DebugDirectory (
        "HST_OrdinaryCampaignPersistenceProof_{0}.{1}.end_bridge.json" -f
            $RunId, $Stage)
    $mixedNativeReadyPath = Join-Path $DebugDirectory (
        "HST_OrdinaryCampaignPersistenceProof_{0}.{1}.mixed_native_ready.json" -f
            $RunId, $Stage)
    if (Test-Path -LiteralPath $resultPath) {
        throw "$Stage result path was not fresh."
    }
    if (Test-Path -LiteralPath $guardPath) {
        throw "$Stage guard path was not fresh."
    }
    if ($Stage -ceq "shutdown_checkpoint" -and
        (Test-Path -LiteralPath $endBridgeReceiptPath)) {
        throw "$Stage end bridge receipt path was not fresh."
    }
    if ($Stage -ceq "shutdown_checkpoint" -and
        (Test-Path -LiteralPath $mixedNativeReadyPath)) {
        throw "$Stage mixed-native readiness path was not fresh."
    }

    $allowCanonicalWrite = $script:StageOrdinals[$Stage] -le 2
    $guard = [ordered]@{
        m_sMagic = $script:GuardMagic
        m_iVersion = $script:AuthorityVersion
        m_sSessionNonce = $SessionNonce
        m_sStageNonce = $stageNonce
        m_sRunId = $RunId
        m_sPayloadNonce = $PayloadNonce
        m_sRequestedStage = $Stage
        m_iStageOrdinal = $script:StageOrdinals[$Stage]
        m_sExpectedSource = $script:ExpectedSources[$Stage]
        m_iExpectedSentinelGeneration =
            $script:ExpectedSentinelGenerations[$Stage]
        m_sExpectedSaveType = $script:ExpectedSaveTypes[$Stage]
        m_sExpectedSaveName = $script:ExpectedSaveNames[$Stage]
        m_sBuildSha = $ExpectedBuild.BuildSha
        m_sBuildUtc = $ExpectedBuild.BuildUtc
        m_sBuildLabel = $ExpectedBuild.BuildLabel
        m_iCampaignSchemaVersion = $ExpectedBuild.CampaignSchemaVersion
        m_iSettingsSchemaVersion = $ExpectedBuild.SettingsSchemaVersion
        m_sWorld = $ExpectedWorld
        m_bAllowCanonicalCampaignOverwrite = $allowCanonicalWrite
        m_bMixedNativeProofRequired = $true
    }
    Write-JsonUtf8NoBom -Path $guardPath -Value $guard
    Assert-EngineGuard `
        -Guard (Read-JsonArtifact -Path $guardPath) `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -PayloadNonce $PayloadNonce `
        -Stage $Stage `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld

    $canonicalPath = Join-Path $ProfileRoot (
        "profile\Partisan\HST_CampaignSaveData.json")
    $recoveryPath = Join-Path $ProfileRoot (
        "profile\Partisan\HST_CampaignSaveData.recovery.json")
    $journalBefore = $null
    if (-not $allowCanonicalWrite) {
        $journalBefore = Get-CampaignJournalFileState `
            -CanonicalPath $canonicalPath `
            -RecoveryPath $recoveryPath
        if ([int]$journalBefore.FileCount -eq 0) {
            throw "$Stage requires an existing campaign fallback journal."
        }
    }

    $arguments = Get-StageArgumentVector `
        -RuntimeAddonPath $RuntimeAddonPath `
        -PackedAddonsParent $PackedAddonsParent `
        -ProfileRoot $ProfileRoot `
        -WorldResource $ExpectedWorld `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -Stage $Stage `
        -LoadSavePointId $LoadSavePointId
    $assertMixedNativeReadyCommand =
        ${function:Assert-MixedNativeReadyReceipt}
    $readinessValidator = {
        param($candidate)
        return & $assertMixedNativeReadyCommand `
            -Receipt $candidate `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonce `
            -RunId $RunId `
            -PayloadNonce $PayloadNonce `
            -ExpectedBuild $ExpectedBuild `
            -ExpectedWorld $ExpectedWorld
    }.GetNewClosure()
    $assertStageResultCommand = ${function:Assert-StageResult}
    $assertJsonPropertyCommand = ${function:Assert-JsonProperty}
    $validator = {
        param($candidate)
        $validated = & $assertStageResultCommand `
            -Result $candidate `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonce `
            -RunId $RunId `
            -PayloadNonce $PayloadNonce `
            -Stage $Stage `
            -ExpectedBuild $ExpectedBuild `
            -ExpectedWorld $ExpectedWorld `
            -ExpectedPriorSavePointId $ExpectedLatestSavePointId `
            -ExpectedSourceFingerprint $ExpectedSourceFingerprint `
            -ExpectedSentinelFingerprint $ExpectedSentinelFingerprint
        if ($Stage -ceq "shutdown_checkpoint") {
            foreach ($property in @(
                "m_bMixedNativeProofRequired",
                "m_sMixedNativeProofPhase",
                "m_bMixedNativeClientConnected",
                "m_bMixedNativeProofExact")) {
                & $assertJsonPropertyCommand `
                    -Value $validated `
                    -PropertyName $property `
                    -ArtifactLabel "shutdown mixed-native result"
            }
            if ($validated.m_bMixedNativeProofRequired -isnot [bool] -or
                -not [bool]$validated.m_bMixedNativeProofRequired -or
                [string]$validated.m_sMixedNativeProofPhase -cne
                    "shutdown_native" -or
                $validated.m_bMixedNativeClientConnected -isnot [bool] -or
                -not [bool]$validated.m_bMixedNativeClientConnected -or
                $validated.m_bMixedNativeProofExact -isnot [bool] -or
                -not [bool]$validated.m_bMixedNativeProofExact) {
                throw "Shutdown mixed-native client proof is not exact."
            }
        }
        return $validated
    }.GetNewClosure()
    $processOutcome = $null
    if ($Stage -ceq "shutdown_checkpoint") {
        foreach ($requiredPath in @(
            $SteamExecutablePath,
            $ClientExecutablePath,
            $ClientProfileRoot,
            $ClientWorkingDirectory,
            $ClientTempDirectory,
            $ClientProcessTempDirectory,
            $ClientLogDirectory)) {
            if ([string]::IsNullOrWhiteSpace($requiredPath)) {
                throw "Shutdown requires its guarded loopback client paths."
            }
        }
        $clientArguments = Get-LoopbackClientArgumentVector `
            -RuntimeAddonPath $RuntimeAddonPath `
            -PackedAddonsParent $PackedAddonsParent `
            -ProfileRoot $ClientProfileRoot `
            -AddonTempDirectory $ClientTempDirectory `
            -ClientLogDirectory $ClientLogDirectory
        $processOutcome = Invoke-GuardedServerWithLoopbackClient `
            -Label "ordinary persistence/$Stage" `
            -ServerExecutablePath $ExecutablePath `
            -ServerArguments $arguments `
            -SteamExecutablePath $SteamExecutablePath `
            -ClientExecutablePath $ClientExecutablePath `
            -ClientArguments $clientArguments `
            -ServerWorkingDirectory $WorkingDirectory `
            -ServerTempDirectory $TempDirectory `
            -ClientWorkingDirectory $ClientWorkingDirectory `
            -ClientTempDirectory $ClientProcessTempDirectory `
            -TimeoutSeconds $StageTimeoutSeconds `
            -ReadinessPath $mixedNativeReadyPath `
            -ReadinessValidator $readinessValidator `
            -ResultPath $resultPath `
            -ResultValidator $validator `
            -DiagnosticProjectDirectory $RepositoryRoot `
            -DiagnosticAddonRoots @($RuntimeAddonPath) `
            -UnclaimedEngineProcessesObserved $UnclaimedEngineProcessesObserved
        if (-not [bool]$processOutcome.ReadinessObserved -or
            -not [bool]$processOutcome.ClientLaunched) {
            throw "Shutdown loopback client lifecycle was not exact."
        }
    }
    else {
        $processOutcome = Invoke-GuardedProcess `
            -Label "ordinary persistence/$Stage" `
            -ExecutablePath $ExecutablePath `
            -Arguments $arguments `
            -WorkingDirectory $WorkingDirectory `
            -TempDirectory $TempDirectory `
            -TimeoutSeconds $StageTimeoutSeconds `
            -ResultPath $resultPath `
            -ResultValidator $validator `
            -DiagnosticProjectDirectory $RepositoryRoot `
            -DiagnosticAddonRoots @($RuntimeAddonPath) `
            -UnclaimedEngineProcessesObserved $UnclaimedEngineProcessesObserved
    }

    if (Test-Path -LiteralPath $guardPath) {
        throw "$Stage did not consume its one-use engine guard."
    }
    $journalAfter = Get-CampaignJournalFileState `
        -CanonicalPath $canonicalPath `
        -RecoveryPath $recoveryPath
    $journalUnchanged = $null
    if (-not $allowCanonicalWrite) {
        $journalUnchanged = Test-CampaignJournalFileStateExact `
            -Expected $journalBefore `
            -Actual $journalAfter
        if (-not $journalUnchanged) {
            throw "$Stage changed its read-only campaign fallback journal."
        }
    }
    $result = $processOutcome.Result
    $endBridgeReceipt = $null
    if ($Stage -ceq "shutdown_checkpoint") {
        $endBridgeReceipt = Wait-StableJsonArtifact `
            -Path $endBridgeReceiptPath `
            -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ResultGraceSeconds))
        $stableReceiptSignature = Get-FileSignature $endBridgeReceiptPath
        $endBridgeReceipt = Read-JsonArtifact -Path $endBridgeReceiptPath
        if ((Get-FileSignature $endBridgeReceiptPath) -cne
            $stableReceiptSignature) {
            throw "$Stage end bridge receipt changed during fresh read."
        }
        $endBridgeReceipt = Assert-EndBridgeReceipt `
            -Receipt $endBridgeReceipt `
            -SessionNonce $SessionNonce `
            -StageNonce $stageNonce `
            -RunId $RunId `
            -PayloadNonce $PayloadNonce `
            -ExpectedBuild $ExpectedBuild `
            -ExpectedWorld $ExpectedWorld `
            -ExpectedShutdownSavePointId (
                [string]$result.m_sCreatedSavePointId)
    }
    $latestSavePointId = if ($script:StageOrdinals[$Stage] -le 2) {
        [string]$result.m_sCreatedSavePointId
    }
    elseif (-not [string]::IsNullOrWhiteSpace($ExpectedLatestSavePointId)) {
        [string]$ExpectedLatestSavePointId
    }
    else {
        [string]$LoadSavePointId
    }
    $carrier = Wait-StableJsonArtifact `
        -Path $carrierPath `
        -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ResultGraceSeconds))
    $carrier = Assert-Carrier `
        -Carrier $carrier `
        -SessionNonce $SessionNonce `
        -RunId $RunId `
        -PayloadNonce $PayloadNonce `
        -Stage $Stage `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $ExpectedWorld `
        -Result $result `
        -ExpectedLatestSavePointId $latestSavePointId

    return [pscustomobject]@{
        Result = $result
        Carrier = $carrier
        EndBridgeReceipt = $endBridgeReceipt
        ExitCode = $processOutcome.ExitCode
        CanonicalFallbackUnchanged = $journalUnchanged
        CampaignJournalUnchanged = $journalUnchanged
        SafeSummary = [pscustomobject]@{
            Stage = $Stage
            Success = [bool]$result.m_bSuccess
            Exit = $processOutcome.ExitCode
            Build = ([string]$result.m_sBuildSha).Substring(0, 12)
            Schema = [int]$result.m_iCampaignSchemaVersion
            Settings = [int]$result.m_iSettingsSchemaVersion
            Source = [string]$result.m_sSource
            SaveType = [string]$result.m_sExpectedSaveType
            RequestFlags = [int]$result.m_iObservedRequestFlags
            RequestFlagsExact = [bool]$result.m_bRequestFlagsExact
            Scheduler = [string]$result.m_sSchedulerOrigin
            SchedulerExact = [bool]$result.m_bSchedulerExercised
            SchedulerTickCount = [int]$result.m_iSchedulerTickCountAtAttempt
            SchedulerElapsed =
                [double]$result.m_fSchedulerAutosaveElapsedAtAttemptSeconds
            SchedulerRemarkElapsed =
                [double]$result.m_fSchedulerDebounceRemarkElapsedSeconds
            SchedulerDebounceHeld =
                [bool]$result.m_bSchedulerDebounceHeld
            FieldPhase = [string]$result.m_sFieldVehicleProofPhase
            FieldRows = ("{0}/{1}" -f
                [int]$result.m_iFieldVehicleExpectedDurableRows,
                [int]$result.m_iFieldVehicleObservedDurableRows)
            FieldRoots = ("{0}/{1}" -f
                [int]$result.m_iFieldVehicleExpectedLiveRoots,
                [int]$result.m_iFieldVehicleObservedLiveRoots)
            FieldDeleted = ("{0}/{1}" -f
                [int]$result.m_iFieldVehicleExpectedDeletedRows,
                [int]$result.m_iFieldVehicleObservedDeletedRows)
            FieldCargo = ("{0}/{1}" -f
                [int]$result.m_iFieldVehicleExpectedCargoRows,
                [int]$result.m_iFieldVehicleObservedCargoRows)
            FieldRestore = ("e{0}/i{1}/n{2}/a{3}/s{4}/t{5}/f{6}/x{7}" -f
                [int]$result.m_iFieldVehicleRestoreEligibleRows,
                [int]$result.m_iFieldVehicleRestoreInactiveRows,
                [int]$result.m_iFieldVehicleRetiredNativeTombstoneRoots,
                [int]$result.m_iFieldVehicleRestoreAdoptedRoots,
                [int]$result.m_iFieldVehicleRestoreSpawnedRoots,
                [int]$result.m_iFieldVehicleRestoreTrackedRoots,
                [int]$result.m_iFieldVehicleRestoreFailedRows,
                [int]$result.m_iFieldVehicleRestoreAmbiguousRows)
            FieldNativeTracked =
                [int]$result.m_iFieldVehicleNativeTrackedRoots
            FieldShutdownQuiesced =
                [int]$result.m_iFieldVehicleShutdownQuiescedRoots
            FieldExact = [bool]$result.m_bFieldVehicleProofExact
            ActiveSave = [string]$result.m_sActiveSavePointId
            CommittedSave = [string]$result.m_sCreatedSavePointId
            SourceDigest = (Get-FileNameSafeDigest `
                ([string]$result.m_sSourceFingerprint))
            FinalDigest = (Get-FileNameSafeDigest `
                ([string]$result.m_sProfileFallbackReadBackFingerprint))
            NoSaveVerification = -not [bool]$result.m_bSavePointRequested
            EndBridgeExact = $null -ne $endBridgeReceipt
            LoopbackClientLaunched = if ($Stage -ceq "shutdown_checkpoint") {
                [bool]$processOutcome.ClientLaunched
            }
            else { $false }
            MixedNativePhase = if ($Stage -ceq "shutdown_checkpoint") {
                [string]$result.m_sMixedNativeProofPhase
            }
            else { "not_applicable" }
            MixedNativeClientConnected =
                $Stage -ceq "shutdown_checkpoint" -and
                [bool]$result.m_bMixedNativeClientConnected
            MixedNativeExact =
                $Stage -ceq "shutdown_checkpoint" -and
                [bool]$result.m_bMixedNativeProofExact
            CanonicalFallbackUnchanged = $journalUnchanged
            CampaignJournalUnchanged = $journalUnchanged
            JournalCanonicalPresent =
                [bool]$journalAfter.CanonicalPresent
            JournalRecoveryPresent =
                [bool]$journalAfter.RecoveryPresent
            JournalFileCount = [int]$journalAfter.FileCount
            JournalGeneration = [int]$result.m_iProfileJournalGeneration
            JournalSlot = [string]$result.m_sProfileJournalSlot
            JournalValidSlotCount =
                [int]$result.m_iProfileJournalValidSlotCount
            JournalChainExact = [bool]$result.m_bProfileJournalChainExact
            SourceJournalGeneration =
                [int]$result.m_iSourceJournalGeneration
            DegradedNativeRecovery =
                [bool]$result.m_bDegradedNativeRecovery
        }
    }
}

function Get-FileNameSafeDigest {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Value)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $hash = $algorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes($Value))
        $hex = ([BitConverter]::ToString($hash)).Replace("-", "")
        return $hex.ToLowerInvariant().Substring(0, 16)
    }
    finally {
        $algorithm.Dispose()
    }
}

function Assert-FallbackProfilePrelaunchFiles {
    param(
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string[]]$ExpectedFiles
    )

    $actual = @(Get-ChildItem -LiteralPath $ProfileRoot -Recurse -File -Force |
        ForEach-Object { [IO.Path]::GetFullPath($_.FullName) } |
        Sort-Object)
    $expected = @($ExpectedFiles | ForEach-Object {
        [IO.Path]::GetFullPath($_)
    } | Sort-Object)
    if ($actual.Count -ne $expected.Count) {
        throw "The fallback profile contains unexpected prelaunch files."
    }
    for ($index = 0; $index -lt $actual.Count; $index++) {
        if (-not $actual[$index].Equals(
            $expected[$index], [StringComparison]::OrdinalIgnoreCase)) {
            throw "The fallback profile is not an isolated fallback-only source."
        }
    }
}

if ($LibraryOnly) {
    return
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$expectedProjectPath = [IO.Path]::GetFullPath(
    (Join-Path $repositoryRoot "addon.gproj"))
$workspacePackScratchPath = Join-Path `
    $repositoryRoot `
    $script:WorkspacePackScratchLeaf
$workspacePackScratchSentinelPath = Join-Path `
    $workspacePackScratchPath `
    $script:WorkspacePackScratchSentinelLeaf
$nonce = [Guid]::NewGuid().ToString("N")
$payloadNonce = [Guid]::NewGuid().ToString("N")
$runId = "ordinary_{0}_{1}" -f
    [DateTime]::UtcNow.ToString(
        "yyyyMMddHHmmss", [Globalization.CultureInfo]::InvariantCulture),
    $nonce.Substring(0, 20)
$guardBase = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) $script:GuardBaseLeaf))
$guardLeaf = $script:GuardLeafPrefix + $nonce
$guardRoot = [IO.Path]::GetFullPath((Join-Path $guardBase $guardLeaf))
$guardSentinelPath = Join-Path $guardRoot $script:GuardSentinelLeaf
$packProfileRoot = Join-Path $guardRoot "pack-profile"
$packedAddonsParent = Join-Path $guardRoot "packed-addons"
$packedAddonPath = Join-Path $packedAddonsParent "Partisan"
$primaryProfileRoot = Join-Path $guardRoot "primary-backend"
$fallbackProfileRoot = Join-Path $guardRoot "fallback-backend"
$clientProfileRoot = Join-Path $guardRoot "loopback-client-profile"
$primaryPartisanRoot = Join-Path $primaryProfileRoot "profile\Partisan"
$fallbackPartisanRoot = Join-Path $fallbackProfileRoot "profile\Partisan"
$primaryDebugRoot = Join-Path $primaryPartisanRoot "debug"
$fallbackDebugRoot = Join-Path $fallbackPartisanRoot "debug"
$workingRoot = Join-Path $guardRoot "working"
$tempRoot = Join-Path $guardRoot "temp"
$clientWorkingRoot = Join-Path $guardRoot "loopback-client-working"
$clientTempRoot = Join-Path $guardRoot "loopback-client-temp"
$clientProcessTempRoot = Join-Path $guardRoot "loopback-client-process-temp"
$clientLogRoot = Join-Path $guardRoot "loopback-client-logs"
$primaryOwnerPath = Join-Path $primaryDebugRoot (
    "HST_OrdinaryCampaignPersistenceProof_{0}.owner.json" -f $runId)
$primaryCarrierPath = Join-Path $primaryDebugRoot (
    "HST_OrdinaryCampaignPersistenceProof_{0}.carrier.json" -f $runId)
$fallbackOwnerPath = Join-Path $fallbackDebugRoot (
    "HST_OrdinaryCampaignPersistenceProof_{0}.owner.json" -f $runId)
$fallbackCarrierPath = Join-Path $fallbackDebugRoot (
    "HST_OrdinaryCampaignPersistenceProof_{0}.carrier.json" -f $runId)
$primaryCanonicalPath = Join-Path $primaryPartisanRoot (
    "HST_CampaignSaveData.json")
$primaryRecoveryPath = Join-Path $primaryPartisanRoot (
    "HST_CampaignSaveData.recovery.json")
$fallbackCanonicalPath = Join-Path $fallbackPartisanRoot (
    "HST_CampaignSaveData.json")
$fallbackRecoveryPath = Join-Path $fallbackPartisanRoot (
    "HST_CampaignSaveData.recovery.json")
$primarySettingsPath = Join-Path $primaryPartisanRoot "HST_Settings.json"

$mutex = $null
$mutexAcquired = $false
$guardOwnership = $null
$workspacePackScratchCreated = $false
$workspacePackScratchOwnership = $null
$runError = $null
$runSucceeded = $false
$cleanupErrors = New-Object Collections.Generic.List[string]
$watchSnapshots = New-Object Collections.Generic.List[object]
$spillSnapshots = New-Object Collections.Generic.List[object]
$stageOutcomes = New-Object Collections.Generic.List[object]
$unclaimedEngineProcessesObserved =
    New-Object Collections.Generic.HashSet[string]
$cleanupResult = $null
$executablePath = ""
$shutdownExecutablePath = ""
$steamBootstrapIdentity = $null
$steamExecutablePath = ""
$clientExecutablePath = ""
$workbenchPath = ""
$runtimeAddonPath = ""
$projectFile = ""
$buildIdentity = $null
$wrapperStartUtc = [DateTime]::MinValue
$packSummary = $null
$proofSummary = $null

try {
    $executablePath = Resolve-ExistingPath $Executable Leaf
    if ((Split-Path -Leaf $executablePath) -cne
        "ArmaReforgerServerDiag.exe") {
        throw "Ordinary persistence proof requires the dedicated diagnostic server."
    }
    $shutdownExecutablePath = Resolve-ExistingPath `
        (Join-Path (Split-Path -Parent $executablePath) `
            "ArmaReforgerServer.exe") `
        Leaf
    if ((Split-Path -Leaf $shutdownExecutablePath) -cne
        "ArmaReforgerServer.exe") {
        throw "Ordinary persistence proof requires the standard shutdown server."
    }
    $clientCandidate = $ClientExecutable
    if ([string]::IsNullOrWhiteSpace($clientCandidate)) {
        $serverInstallRoot = Split-Path -Parent $executablePath
        if ((Split-Path -Leaf $serverInstallRoot) -cne
            "Arma Reforger Server") {
            throw "ClientExecutable is required for a nonstandard server installation."
        }
        $commonInstallRoot = Split-Path -Parent $serverInstallRoot
        $clientCandidate = Join-Path $commonInstallRoot `
            "Arma Reforger\ArmaReforgerSteam.exe"
    }
    $clientExecutablePath = Resolve-ExistingPath $clientCandidate Leaf
    if ((Split-Path -Leaf $clientExecutablePath) -cne
        "ArmaReforgerSteam.exe") {
        throw "Ordinary persistence proof requires the standard Steam game client."
    }
    $steamBootstrapIdentity = Resolve-RunningSteamBootstrapIdentity
    $steamExecutablePath = $steamBootstrapIdentity.ExecutablePath
    $serverFileVersion = (Get-Item -LiteralPath $executablePath).VersionInfo.FileVersion
    $shutdownServerFileVersion = (Get-Item `
        -LiteralPath $shutdownExecutablePath).VersionInfo.FileVersion
    $clientFileVersion = (Get-Item -LiteralPath $clientExecutablePath).VersionInfo.FileVersion
    if ([string]::IsNullOrWhiteSpace($serverFileVersion) -or
        [string]::IsNullOrWhiteSpace($shutdownServerFileVersion) -or
        [string]::IsNullOrWhiteSpace($clientFileVersion) -or
        $serverFileVersion -cne $clientFileVersion -or
        $shutdownServerFileVersion -cne $clientFileVersion) {
        throw "The diagnostic, shutdown-server, and client builds are not exact peers."
    }
    $workbenchPath = Resolve-ExistingPath $WorkbenchExecutable Leaf
    if ((Split-Path -Leaf $workbenchPath) -cne
        "ArmaReforgerWorkbenchSteamDiag.exe") {
        throw "Ordinary persistence proof requires diagnostic Steam Workbench."
    }
    $runtimeAddonPath = Resolve-ExistingPath $RuntimeAddonRoot Container
    foreach ($marker in @("core\data.pak", "data\data.pak")) {
        $runtimeMarkerPath = Join-Path $runtimeAddonPath $marker
        if (-not (Test-Path -LiteralPath $runtimeMarkerPath -PathType Leaf)) {
            throw "RuntimeAddonRoot is not an installed packed game add-on root."
        }
    }
    $projectFile = if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
        Resolve-ExistingPath $expectedProjectPath Leaf
    }
    else {
        Resolve-ExistingPath $ProjectPath Leaf
    }
    if (-not $projectFile.Equals(
        $expectedProjectPath, [StringComparison]::OrdinalIgnoreCase)) {
        throw "The proof must pack this checkout's addon.gproj."
    }
    if ($WorldResource -cne "Worlds/HST_Everon/HST_Everon.ent") {
        throw "Ordinary persistence proof requires the canonical Everon world."
    }
    foreach ($resource in @(
        $WorldResource,
        $script:MissionHeader,
        $script:WorldSystemsConfig)) {
        $resourcePath = Join-Path $repositoryRoot (
            $resource.Replace('/', [IO.Path]::DirectorySeparatorChar))
        [void](Resolve-ExistingPath $resourcePath Leaf)
    }
    $buildIdentity = Read-CheckoutBuildIdentity $repositoryRoot
    $normalizedWatchedRoots = @($WatchedRoots | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    $normalizedSpillRoots = @($SpillRoots | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    if (-not $PreflightOnly -and
        ($normalizedWatchedRoots.Count -eq 0 -or
            $normalizedSpillRoots.Count -eq 0)) {
        throw "A real proof requires explicit watched and spill roots."
    }
    $checkoutCovered = $false
    foreach ($root in $normalizedWatchedRoots) {
        $resolvedWatchRoot = Resolve-ExistingPath $root Container
        if (Test-ContainedPath `
            -Root $resolvedWatchRoot `
            -Candidate $repositoryRoot `
            -AllowEqual) {
            $checkoutCovered = $true
            break
        }
    }
    if (-not $PreflightOnly -and -not $checkoutCovered) {
        throw "A real proof requires watched-root coverage of this checkout."
    }
    if (-not $PreflightOnly) {
        $documentsRoot = [Environment]::GetFolderPath(
            [Environment+SpecialFolder]::MyDocuments)
        if ([string]::IsNullOrWhiteSpace($documentsRoot)) {
            throw "The external game-state monitoring boundary is unavailable."
        }
        $expectedSpillBoundary = Resolve-ExistingPath `
            -Path (Join-Path $documentsRoot "My Games") `
            -Kind Container
        $spillBoundaryCovered = $false
        foreach ($root in $normalizedSpillRoots) {
            $resolvedSpillRoot = Resolve-ExistingPath $root Container
            if (Test-ContainedPath `
                -Root $resolvedSpillRoot `
                -Candidate $expectedSpillBoundary `
                -AllowEqual) {
                $spillBoundaryCovered = $true
                break
            }
        }
        if (-not $spillBoundaryCovered) {
            throw "A real proof requires spill-root coverage of the external game-state boundary."
        }
    }
    if (-not $PreflightOnly -and
        (Test-Path -LiteralPath $workspacePackScratchPath)) {
        throw "Workbench packing requires a fresh workspace scratch boundary."
    }

    $mutex = New-Object Threading.Mutex($false, $script:MutexName)
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw "Another ordinary persistence proof wrapper is active."
    }
    if (@(Get-EngineProcessRows).Count -ne 0) {
        throw "Refusing launch while an engine or Workbench process is active."
    }

	foreach ($root in $normalizedWatchedRoots) {
		$watchRoot = Resolve-ExistingPath -Path $root -Kind Container
		$watchExclusions = @(Get-CheckoutWatchExclusions `
			-RepositoryRoot $repositoryRoot `
			-WatchRoot $watchRoot)
		$watchSnapshots.Add((New-RootSnapshot `
			-Root $watchRoot `
			-ExcludedRoots $watchExclusions))
	}
    $spillExclusions = New-Object Collections.Generic.List[string]
    $spillExclusions.Add((Split-Path -Parent $projectFile))
    $spillExclusions.Add($guardBase)
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

    Assert-NoReparsePathAncestry -Path $guardBase
    if (-not (Test-Path -LiteralPath $guardBase -PathType Container)) {
        [void](New-Item -ItemType Directory -Path $guardBase)
    }
    Assert-NoReparsePathAncestry -Path $guardBase
    if (Test-Path -LiteralPath $guardRoot) {
        throw "The nonce-owned guard root was not fresh."
    }
    [void](New-Item -ItemType Directory -Path $guardRoot)
    $wrapper = Get-Process -Id $PID -ErrorAction Stop
    $wrapperStartUtc = $wrapper.StartTime.ToUniversalTime()
    Write-JsonUtf8NoBom -Path $guardSentinelPath -Value ([ordered]@{
        version = $script:SentinelVersion
        purpose = $script:OwnerPurpose
        nonce = $nonce
        guardLeaf = $guardLeaf
        ownerPid = $PID
        ownerStartUtc = $wrapperStartUtc.ToString(
            "o", [Globalization.CultureInfo]::InvariantCulture)
    })
    $guardOwnership = Read-GuardOwnership $guardRoot $guardBase
    if (-not $guardOwnership -or
        $guardOwnership.Nonce -cne $nonce -or
        $guardOwnership.OwnerPid -ne $PID -or
        $guardOwnership.OwnerStartUtc.Ticks -ne $wrapperStartUtc.Ticks) {
        throw "Guard ownership validation failed."
    }

    if (-not $PreflightOnly) {
        if (Test-Path -LiteralPath $workspacePackScratchPath) {
            throw "Workbench packing requires a fresh workspace scratch boundary."
        }
        [void](New-Item `
            -ItemType Directory `
            -Path $workspacePackScratchPath)
        $workspacePackScratchCreated = $true
        Assert-NoReparsePathAncestry -Path $workspacePackScratchPath
        Write-JsonUtf8NoBom `
            -Path $workspacePackScratchSentinelPath `
            -Value ([ordered]@{
                version = $script:SentinelVersion
                purpose = "native_workbench_pack_scratch"
                nonce = $nonce
                ownerPid = $PID
                ownerStartUtc = $wrapperStartUtc.ToString(
                    "o", [Globalization.CultureInfo]::InvariantCulture)
            })
        $workspacePackScratchOwnership =
            Read-WorkspacePackScratchOwnership `
                -Directory $workspacePackScratchPath `
                -RepositoryRoot $repositoryRoot
        if (-not $workspacePackScratchOwnership -or
            $workspacePackScratchOwnership.Nonce -cne $nonce -or
            $workspacePackScratchOwnership.OwnerPid -ne $PID -or
            $workspacePackScratchOwnership.OwnerStartUtc.Ticks -ne
                $wrapperStartUtc.Ticks) {
            throw "Workspace pack scratch ownership validation failed."
        }
    }

    foreach ($directory in @(
        $packProfileRoot,
        $packedAddonsParent,
        $primaryDebugRoot,
        $workingRoot,
        $tempRoot,
        $clientProfileRoot,
        $clientWorkingRoot,
        $clientTempRoot,
        $clientProcessTempRoot,
        $clientLogRoot)) {
        [void](New-Item -ItemType Directory -Path $directory -Force)
        Assert-NoReparsePathAncestry -Path $directory
    }
    if (Test-Path -LiteralPath $packedAddonPath) {
        throw "The packed add-on output path was not fresh."
    }

    $schedulerSettings = [ordered]@{
        schemaVersion = $buildIdentity.SettingsSchemaVersion
        persistence = [ordered]@{
            autosaveIntervalSeconds =
                $script:HSTAutosaveSchedulerIntervalSeconds
            majorChangeDebounceSeconds =
                $script:HSTAutosaveSchedulerDebounceSeconds
        }
    }
    Write-JsonUtf8NoBom -Path $primarySettingsPath -Value $schedulerSettings
    $validatedSchedulerSettings = Read-JsonArtifact -Path $primarySettingsPath
    if ([int]$validatedSchedulerSettings.schemaVersion -ne
            $buildIdentity.SettingsSchemaVersion -or
        [int]$validatedSchedulerSettings.persistence.autosaveIntervalSeconds -ne
            $script:HSTAutosaveSchedulerIntervalSeconds -or
        [int]$validatedSchedulerSettings.persistence.majorChangeDebounceSeconds -ne
            $script:HSTAutosaveSchedulerDebounceSeconds) {
        throw "The guarded HST autosave scheduler settings failed their exact gate."
    }

    $primaryOwner = New-EngineOwnerValue `
        -SessionNonce $nonce `
        -RunId $runId `
        -PayloadNonce $payloadNonce `
        -BuildIdentity $buildIdentity `
        -World $WorldResource
    Write-JsonUtf8NoBom -Path $primaryOwnerPath -Value $primaryOwner
    Assert-EngineOwner `
        -Owner (Read-JsonArtifact -Path $primaryOwnerPath) `
        -SessionNonce $nonce `
        -RunId $runId `
        -PayloadNonce $payloadNonce `
        -ExpectedBuild $buildIdentity `
        -ExpectedWorld $WorldResource

    $packArguments = Get-PackArgumentVector `
        -ProjectFile $projectFile `
        -RuntimeAddonPath $runtimeAddonPath `
        -PackProfilePath $packProfileRoot `
        -PackedAddonPath $packedAddonPath
    $placeholderSaveIds = @(
        "11111111-1111-1111-1111-111111111111",
        "22222222-2222-2222-2222-222222222222",
        "33333333-3333-3333-3333-333333333333")
    $preflightVectors = New-Object Collections.Generic.List[object]
    for ($index = 0; $index -lt $script:Stages.Count; $index++) {
        $stage = $script:Stages[$index]
        $loadId = if ($index -ge 1 -and $index -le 3) {
            $placeholderSaveIds[$index - 1]
        }
        else { "" }
        $stageNonce = [Guid]::NewGuid().ToString("N")
        $profileRoot = if ($stage -ceq "profile_fallback_verify") {
            $fallbackProfileRoot
        }
        else { $primaryProfileRoot }
        $preflightVectors.Add([pscustomobject]@{
            Stage = $stage
            Arguments = @(Get-StageArgumentVector `
                -RuntimeAddonPath $runtimeAddonPath `
                -PackedAddonsParent $packedAddonsParent `
                -ProfileRoot $profileRoot `
                -WorldResource $WorldResource `
                -SessionNonce $nonce `
                -StageNonce $stageNonce `
                -RunId $runId `
                -Stage $stage `
                -LoadSavePointId $loadId)
        })
    }
    $loopbackClientArguments = @(Get-LoopbackClientArgumentVector `
        -RuntimeAddonPath $runtimeAddonPath `
        -PackedAddonsParent $packedAddonsParent `
        -ProfileRoot $clientProfileRoot `
        -AddonTempDirectory $clientTempRoot `
        -ClientLogDirectory $clientLogRoot)
    $directClientCommandLine =
        (ConvertTo-NativeArgument $clientExecutablePath) + " " +
        (($loopbackClientArguments | ForEach-Object {
            ConvertTo-NativeArgument ([string]$_)
        }) -join " ")
    $directClientArgumentVectorExact = Test-ExactNativeArgumentVector `
        -CommandLine $directClientCommandLine `
        -ExpectedExecutable $clientExecutablePath `
        -ExpectedArguments $loopbackClientArguments
    $clientAddonTempIndex = [Array]::IndexOf(
        $loopbackClientArguments, "-addonTempDir")
    $clientAddonTempConfined =
        $clientAddonTempIndex -ge 0 -and
        $clientAddonTempIndex + 1 -lt $loopbackClientArguments.Count -and
        [string]$loopbackClientArguments[$clientAddonTempIndex + 1] -ceq
            $clientTempRoot
    $clientLogIndex = [Array]::IndexOf(
        $loopbackClientArguments, "-logsDir")
    $clientLogsConfined =
        $clientLogIndex -ge 0 -and
        $clientLogIndex + 1 -lt $loopbackClientArguments.Count -and
        [string]$loopbackClientArguments[$clientLogIndex + 1] -ceq
            $clientLogRoot
    $clientIndex = [Array]::IndexOf(
        $loopbackClientArguments, "-client")
    $clientUsesBareAutoJoin =
        $clientIndex -ge 0 -and
        $clientIndex + 1 -lt $loopbackClientArguments.Count -and
        [string]$loopbackClientArguments[$clientIndex + 1] -ceq "-profile"
    $clientWorkingConfined = Test-ContainedPath `
        -Root $guardRoot `
        -Candidate $clientWorkingRoot
    $clientProcessTempConfined = Test-ContainedPath `
        -Root $guardRoot `
        -Candidate $clientProcessTempRoot
    $clientBoundaryPathCount = @(@(
        $clientProfileRoot,
        $clientWorkingRoot,
        $clientTempRoot,
        $clientProcessTempRoot,
        $clientLogRoot) | Sort-Object -Unique).Count
    $shutdownPreflight = @($preflightVectors | Where-Object {
        $_.Stage -ceq "shutdown_checkpoint"
    })
    $shutdownUsesSupportedLocalAddonServer =
        $shutdownPreflight.Count -eq 1 -and
        $shutdownPreflight[0].Arguments -ccontains "-server" -and
        $shutdownPreflight[0].Arguments -ccontains "-MissionHeader" -and
        $shutdownPreflight[0].Arguments -cnotcontains "-worldSystemsConfig" -and
        $shutdownPreflight[0].Arguments -ccontains "-addons" -and
        $shutdownPreflight[0].Arguments -cnotcontains "-config" -and
        $shutdownPreflight[0].Arguments -cnotcontains "-rpl-timeout-disable"
    $directLocalAddonStages = @($preflightVectors | Where-Object {
        $_.Arguments -ccontains "-server" -and
        $_.Arguments -ccontains "-MissionHeader" -and
        $_.Arguments -cnotcontains "-worldSystemsConfig" -and
        $_.Arguments -ccontains "-addons" -and
        $_.Arguments -cnotcontains "-config" -and
        $_.Arguments -cnotcontains "-rpl-timeout-disable"
    })
    $allValidationArguments = @(
        @($preflightVectors | ForEach-Object { $_.Arguments }) +
        @($loopbackClientArguments)
    )
    $validationDisableCount = @($allValidationArguments | Where-Object {
        [string]$_ -clike "-rpl-validation-*-disable"
    }).Count
    if (-not $directClientArgumentVectorExact -or
        -not $clientAddonTempConfined -or
        -not $clientLogsConfined -or
        -not $clientWorkingConfined -or
        -not $clientProcessTempConfined -or
        $clientBoundaryPathCount -ne 5 -or
        -not $clientUsesBareAutoJoin -or
        -not $shutdownUsesSupportedLocalAddonServer -or
        $directLocalAddonStages.Count -ne $script:Stages.Count -or
        $validationDisableCount -ne 0 -or
        $clientLogRoot.Equals(
            $clientTempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        $clientLogRoot.Equals(
            $clientProfileRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "The direct standard client preflight vector is not exact."
    }
    $keepSessionSaveOverrideCount = @($preflightVectors | Where-Object {
        $_.Arguments -icontains "-keepSessionSave"
    }).Count
    $autoShutdownStages = @($preflightVectors | Where-Object {
        $_.Arguments -ccontains "-autoshutdown"
    } | ForEach-Object { $_.Stage })
    $nativeLoadStages = @($preflightVectors | Where-Object {
        $_.Arguments -ccontains "-loadSessionSave"
    } | ForEach-Object { $_.Stage })
    $explicitNativeLoadStages = @($preflightVectors | Where-Object {
        $loadIndex = [Array]::IndexOf($_.Arguments, "-loadSessionSave")
        $loadIndex -ge 0 -and
        $loadIndex + 1 -lt $_.Arguments.Count -and
        [string]$_.Arguments[$loadIndex + 1] -notlike "-*"
    } | ForEach-Object { $_.Stage })
    $bareNativeLoadStages = @($preflightVectors | Where-Object {
        $loadIndex = [Array]::IndexOf($_.Arguments, "-loadSessionSave")
        $loadIndex -ge 0 -and
        $loadIndex + 1 -lt $_.Arguments.Count -and
        [string]$_.Arguments[$loadIndex + 1] -like "-*"
    } | ForEach-Object { $_.Stage })
    if ($keepSessionSaveOverrideCount -ne 0 -or
        $autoShutdownStages.Count -ne 1 -or
        [string]$autoShutdownStages[0] -cne "shutdown_checkpoint" -or
        $nativeLoadStages.Count -ne 4 -or
        $explicitNativeLoadStages.Count -ne 3 -or
        $explicitNativeLoadStages -cnotcontains "manual_checkpoint" -or
        $explicitNativeLoadStages -cnotcontains "shutdown_checkpoint" -or
        $explicitNativeLoadStages -cnotcontains "native_shutdown_verify" -or
        $bareNativeLoadStages.Count -ne 1 -or
        [string]$bareNativeLoadStages[0] -cne "autosave_checkpoint") {
        throw "Preflight stage vectors failed their retention or shutdown gate."
    }

    if ($PreflightOnly) {
        $runSucceeded = $true
        Write-Output ("PREFLIGHT " + ([pscustomobject]@{
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            Settings = $buildIdentity.SettingsSchemaVersion
            World = $WorldResource
            PackArgumentTokenCount = $packArguments.Count
            StageCount = $preflightVectors.Count
            PackedLaunch = $true
            KeepSessionSaveCLIAbsent =
                $keepSessionSaveOverrideCount -eq 0
            ExplicitNativeLoadStageCount =
                $explicitNativeLoadStages.Count
            BareNativeSessionBootstrapStageCount =
                $bareNativeLoadStages.Count
            ShutdownHasAutoShutdown =
                $autoShutdownStages.Count -eq 1
            ShutdownHasLoopbackClient =
                $loopbackClientArguments -ccontains "-client"
            LocalClientUsesAutomaticAddress =
                $clientUsesBareAutoJoin
            ClientArgumentTokenCount = $loopbackClientArguments.Count
            ClientBuildMatchesServer =
                $shutdownServerFileVersion -ceq $clientFileVersion
            DiagnosticServerBuildMatchesClient =
                $serverFileVersion -ceq $clientFileVersion
            ShutdownServerIsStandard =
                (Split-Path -Leaf $shutdownExecutablePath) -ceq
                    "ArmaReforgerServer.exe"
            ShutdownUsesSupportedLocalAddonServer =
                $shutdownUsesSupportedLocalAddonServer
            DirectLocalAddonStageCount = $directLocalAddonStages.Count
            ReplicationValidationDisableCount = $validationDisableCount
            ClientIsStandardSteam =
                (Split-Path -Leaf $clientExecutablePath) -ceq
                    "ArmaReforgerSteam.exe"
            SteamBackendPrerequisiteRequired = $true
            SteamBackendPrerequisiteAvailable =
                $steamBootstrapIdentity -and
                (Test-ProcessIdentityAlive `
                    -ProcessId ([int]$steamBootstrapIdentity.ProcessId) `
                    -StartUtc $steamBootstrapIdentity.StartUtc)
            ClientLaunchMode = "direct_guarded_job"
            DirectClientArgumentVectorExact =
                $directClientArgumentVectorExact
            ClientAddonTempConfined = $clientAddonTempConfined
            ClientWorkingDirectoryConfined = $clientWorkingConfined
            ClientProcessTempConfined = $clientProcessTempConfined
            ClientLogsConfined = $clientLogsConfined
            ServerUsesDefaultBackend =
                @($preflightVectors | Where-Object {
                    $_.Arguments -icontains "-backendLocalStorage" -or
                    $_.Arguments -icontains "-backendDisableStorage" -or
                    $_.Arguments -icontains "-noBackend"
                }).Count -eq 0
            ClientUsesDefaultBackend =
                $loopbackClientArguments -cnotcontains "-backendLocalStorage" -and
                $loopbackClientArguments -cnotcontains "-backendDisableStorage" -and
                $loopbackClientArguments -cnotcontains "-noBackend"
            FallbackHasNoLoadSessionSave =
                $preflightVectors[4].Arguments -cnotcontains "-loadSessionSave"
        } | ConvertTo-Json -Compress))
    }
    else {
        $packOutcome = Invoke-GuardedProcess `
            -Label "ordinary persistence Workbench pack" `
            -ExecutablePath $workbenchPath `
            -Arguments $packArguments `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -TimeoutSeconds $PackTimeoutSeconds `
            -DiagnosticProjectDirectory $repositoryRoot `
            -DiagnosticAddonRoots @($runtimeAddonPath) `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $workspacePackScratchOwnership =
            Read-WorkspacePackScratchOwnership `
                -Directory $workspacePackScratchPath `
                -RepositoryRoot $repositoryRoot
        if (-not $workspacePackScratchOwnership -or
            $workspacePackScratchOwnership.Nonce -cne $nonce -or
            $workspacePackScratchOwnership.OwnerPid -ne $PID -or
            $workspacePackScratchOwnership.OwnerStartUtc.Ticks -ne
                $wrapperStartUtc.Ticks) {
            throw "Workbench packing changed its owned workspace scratch boundary."
        }
        $packedLayout = Assert-PackedAddonLayout `
            -GuardRoot $guardRoot `
            -PackedAddonsParent $packedAddonsParent
        $packedProjectPath = [IO.Path]::GetFullPath(
            (Join-Path $packedLayout.AddonPath "addon.gproj"))
        $expectedPackedProjectPath = [IO.Path]::GetFullPath(
            (Join-Path $packedAddonsParent "Partisan\addon.gproj"))
        if (-not $packedProjectPath.Equals(
                $expectedPackedProjectPath,
                [StringComparison]::OrdinalIgnoreCase) -or
            -not (Test-ContainedPath `
                -Root $guardRoot `
                -Candidate $packedProjectPath) -or
            -not (Test-Path -LiteralPath $packedProjectPath -PathType Leaf) -or
            (Get-Item -LiteralPath $packedProjectPath -Force).Length -le 0) {
            throw "The packed local-add-on project authority is not exact."
        }
        $packSummary = [pscustomobject]@{
            Exit = $packOutcome.ExitCode
            EngineAfter = $packOutcome.EngineAfter
            OwnedProcessesRemaining = $packOutcome.OwnedProcessesRemaining
            FileCount = $packedLayout.FileCount
            PakCount = $packedLayout.PakCount
            ProjectCount = $packedLayout.ProjectCount
            ResourceDatabaseCount = $packedLayout.ResourceDatabaseCount
        }
        Write-Output ("PACK " + ($packSummary | ConvertTo-Json -Compress))

        $autosave = Invoke-OrdinaryCampaignStage `
            -ExecutablePath $shutdownExecutablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ProfileRoot $primaryProfileRoot `
            -DebugDirectory $primaryDebugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -PayloadNonce $payloadNonce `
            -Stage "autosave_checkpoint" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -LoadSavePointId "" `
            -ExpectedSourceFingerprint "" `
            -ExpectedSentinelFingerprint "" `
            -ExpectedLatestSavePointId "" `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($autosave.SafeSummary)
        Write-Output ("STAGE " + ($autosave.SafeSummary | ConvertTo-Json -Compress))
        $u0 = [string]$autosave.Result.m_sCreatedSavePointId
        $fallbackGeneration1 =
            [string]$autosave.Result.m_sProfileFallbackReadBackFingerprint

        $manual = Invoke-OrdinaryCampaignStage `
            -ExecutablePath $shutdownExecutablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ProfileRoot $primaryProfileRoot `
            -DebugDirectory $primaryDebugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -PayloadNonce $payloadNonce `
            -Stage "manual_checkpoint" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -LoadSavePointId $u0 `
            -ExpectedSourceFingerprint $fallbackGeneration1 `
            -ExpectedSentinelFingerprint "" `
            -ExpectedLatestSavePointId $u0 `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($manual.SafeSummary)
        Write-Output ("STAGE " + ($manual.SafeSummary | ConvertTo-Json -Compress))
        $u1 = [string]$manual.Result.m_sCreatedSavePointId
        $fallbackGeneration2 =
            [string]$manual.Result.m_sProfileFallbackReadBackFingerprint

        $shutdown = Invoke-OrdinaryCampaignStage `
            -ExecutablePath $shutdownExecutablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ProfileRoot $primaryProfileRoot `
            -DebugDirectory $primaryDebugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -PayloadNonce $payloadNonce `
            -Stage "shutdown_checkpoint" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -LoadSavePointId $u1 `
            -ExpectedSourceFingerprint $fallbackGeneration2 `
            -ExpectedSentinelFingerprint "" `
            -ExpectedLatestSavePointId $u1 `
            -SteamExecutablePath $steamExecutablePath `
            -ClientExecutablePath $clientExecutablePath `
            -ClientProfileRoot $clientProfileRoot `
            -ClientWorkingDirectory $clientWorkingRoot `
            -ClientTempDirectory $clientTempRoot `
            -ClientProcessTempDirectory $clientProcessTempRoot `
            -ClientLogDirectory $clientLogRoot `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($shutdown.SafeSummary)
        Write-Output ("STAGE " + ($shutdown.SafeSummary | ConvertTo-Json -Compress))
        $u2 = [string]$shutdown.Result.m_sCreatedSavePointId
        $fallbackGeneration3 =
            [string]$shutdown.Result.m_sProfileFallbackReadBackFingerprint
        $sentinelGeneration3 =
            [string]$shutdown.Result.m_sSentinelFingerprint
        $shutdownJournalState = Get-CampaignJournalFileState `
            -CanonicalPath $primaryCanonicalPath `
            -RecoveryPath $primaryRecoveryPath
        if ([int]$shutdownJournalState.FileCount -ne 2) {
            throw "Shutdown checkpoint did not retain both fallback journal generations."
        }

        $nativeVerify = Invoke-OrdinaryCampaignStage `
            -ExecutablePath $shutdownExecutablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ProfileRoot $primaryProfileRoot `
            -DebugDirectory $primaryDebugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -PayloadNonce $payloadNonce `
            -Stage "native_shutdown_verify" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -LoadSavePointId $u2 `
            -ExpectedSourceFingerprint $fallbackGeneration3 `
            -ExpectedSentinelFingerprint $sentinelGeneration3 `
            -ExpectedLatestSavePointId $u2 `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($nativeVerify.SafeSummary)
        Write-Output ("STAGE " +
            ($nativeVerify.SafeSummary | ConvertTo-Json -Compress))
        $nativeVerifyJournalState = Get-CampaignJournalFileState `
            -CanonicalPath $primaryCanonicalPath `
            -RecoveryPath $primaryRecoveryPath
        if (-not (Test-CampaignJournalFileStateExact `
            -Expected $shutdownJournalState `
            -Actual $nativeVerifyJournalState)) {
            throw "Native verification changed the shutdown fallback journal."
        }

        [void](New-Item -ItemType Directory -Path $fallbackDebugRoot -Force)
        $fallbackOwner = New-EngineOwnerValue `
            -SessionNonce $nonce `
            -RunId $runId `
            -PayloadNonce $payloadNonce `
            -BuildIdentity $buildIdentity `
            -World $WorldResource
        Write-JsonUtf8NoBom -Path $fallbackOwnerPath -Value $fallbackOwner
        Assert-EngineOwner `
            -Owner (Read-JsonArtifact -Path $fallbackOwnerPath) `
            -SessionNonce $nonce `
            -RunId $runId `
            -PayloadNonce $payloadNonce `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource
        Copy-Item -LiteralPath $primaryCanonicalPath `
            -Destination $fallbackCanonicalPath
        Copy-Item -LiteralPath $primaryRecoveryPath `
            -Destination $fallbackRecoveryPath
        Copy-Item -LiteralPath $primaryCarrierPath `
            -Destination $fallbackCarrierPath
        $fallbackCopiedJournalState = Get-CampaignJournalFileState `
            -CanonicalPath $fallbackCanonicalPath `
            -RecoveryPath $fallbackRecoveryPath
        if (-not (Test-CampaignJournalFileStateExact `
            -Expected $shutdownJournalState `
            -Actual $fallbackCopiedJournalState)) {
            throw "Fallback profile did not receive the exact shutdown journal."
        }
        Assert-FallbackProfilePrelaunchFiles `
            -ProfileRoot $fallbackProfileRoot `
            -ExpectedFiles @(
                $fallbackCanonicalPath,
                $fallbackRecoveryPath,
                $fallbackOwnerPath,
                $fallbackCarrierPath)

        $fallbackVerify = Invoke-OrdinaryCampaignStage `
            -ExecutablePath $shutdownExecutablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ProfileRoot $fallbackProfileRoot `
            -DebugDirectory $fallbackDebugRoot `
            -WorkingDirectory $workingRoot `
            -TempDirectory $tempRoot `
            -SessionNonce $nonce `
            -RunId $runId `
            -PayloadNonce $payloadNonce `
            -Stage "profile_fallback_verify" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -LoadSavePointId "" `
            -ExpectedSourceFingerprint $fallbackGeneration3 `
            -ExpectedSentinelFingerprint $sentinelGeneration3 `
            -ExpectedLatestSavePointId $u2 `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($fallbackVerify.SafeSummary)
        Write-Output ("STAGE " +
            ($fallbackVerify.SafeSummary | ConvertTo-Json -Compress))
        $fallbackVerifyJournalState = Get-CampaignJournalFileState `
            -CanonicalPath $fallbackCanonicalPath `
            -RecoveryPath $fallbackRecoveryPath
        if (-not (Test-CampaignJournalFileStateExact `
            -Expected $fallbackCopiedJournalState `
            -Actual $fallbackVerifyJournalState)) {
            throw "Fallback verification changed its read-only journal source."
        }

        if ($stageOutcomes.Count -ne 5 -or
            $unclaimedEngineProcessesObserved.Count -ne 0) {
            throw "The five-stage proof did not complete its exact stage set."
        }
        $proofSummary = [pscustomobject]@{
            Success = $true
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            Settings = $buildIdentity.SettingsSchemaVersion
            World = $WorldResource
            StageCount = $stageOutcomes.Count
            AutosaveUuid = $u0
            ManualUuid = $u1
            ShutdownUuid = $u2
            ControlledEndBridge = $null -ne $shutdown.EndBridgeReceipt
            KeepSessionSaveCLIAbsent =
                [bool]$shutdown.EndBridgeReceipt.m_bKeepSessionSaveCLIAbsent
            PersistenceKeepSessionDataDisabled =
                [bool]$shutdown.EndBridgeReceipt.m_bPersistenceKeepSessionDataDisabled
            MixedNativePhase =
                [string]$shutdown.Result.m_sMixedNativeProofPhase
            MixedNativeClientConnected =
                [bool]$shutdown.Result.m_bMixedNativeClientConnected
            MixedNativeExact =
                [bool]$shutdown.Result.m_bMixedNativeProofExact
            SentinelDigest = Get-FileNameSafeDigest $sentinelGeneration3
            NativeVerificationNoSave =
                -not [bool]$nativeVerify.Result.m_bSavePointRequested
            FallbackVerificationNoSave =
                -not [bool]$fallbackVerify.Result.m_bSavePointRequested
            JournalFileCount = [int]$shutdownJournalState.FileCount
            NativeJournalReadOnly =
                [bool]$nativeVerify.CampaignJournalUnchanged
            FallbackJournalReadOnly =
                [bool]$fallbackVerify.CampaignJournalUnchanged
            FallbackSource =
                [string]$fallbackVerify.Result.m_sSource
            FieldVehiclePrepare =
                [bool]$autosave.Result.m_bFieldVehicleProofExact
            FieldVehicleRecover =
                [bool]$manual.Result.m_bFieldVehicleProofExact
            FieldVehicleReplay =
                [bool]$shutdown.Result.m_bFieldVehicleProofExact
            FieldVehicleNative =
                [bool]$nativeVerify.Result.m_bFieldVehicleProofExact
            FieldVehicleFallback =
                [bool]$fallbackVerify.Result.m_bFieldVehicleProofExact
        }
        $runSucceeded = $true
    }
}
catch {
    $runError = $_.Exception.Message
}
finally {
    try {
        if ($workspacePackScratchCreated) {
            if (@(Get-EngineProcessRows).Count -ne 0) {
                throw "Owned workspace scratch cannot be removed while an engine process is active."
            }
            $candidateScratchOwnership =
                Read-WorkspacePackScratchOwnership `
                    -Directory $workspacePackScratchPath `
                    -RepositoryRoot $repositoryRoot
            if ($candidateScratchOwnership -and
                $candidateScratchOwnership.Nonce -ceq $nonce -and
                $candidateScratchOwnership.OwnerPid -eq $PID -and
                $wrapperStartUtc -ne [DateTime]::MinValue -and
                $candidateScratchOwnership.OwnerStartUtc.Ticks -eq
                    $wrapperStartUtc.Ticks) {
                $workspacePackScratchOwnership = $candidateScratchOwnership
            }
            else {
                $workspacePackScratchOwnership = $null
            }
            if (-not $workspacePackScratchOwnership -or
                -not (Remove-WorkspacePackScratch `
                    -Ownership $workspacePackScratchOwnership `
                    -RepositoryRoot $repositoryRoot)) {
                throw "Owned Workbench workspace scratch removal failed."
            }
        }
    }
    catch { [void]$cleanupErrors.Add("remove-owned-workspace-pack-scratch") }
    try {
        if ($guardOwnership) {
            if (@(Get-EngineProcessRows).Count -ne 0) {
                throw "The nonce-owned guard cannot be removed while an " +
                    "engine process is active."
            }
            if (-not (Remove-ExactOwnedGuard $guardOwnership $guardBase)) {
                throw "The nonce-owned guard could not be removed."
            }
        }
    }
    catch { [void]$cleanupErrors.Add("remove-owned-guard") }
    try {
        if (Test-Path -LiteralPath $guardBase -PathType Container) {
            Assert-NoReparsePathAncestry -Path $guardBase
            if (@(Get-ChildItem -LiteralPath $guardBase -Force).Count -eq 0) {
                Remove-Item -LiteralPath $guardBase -Force -ErrorAction Stop
            }
        }
    }
    catch { [void]$cleanupErrors.Add("remove-empty-guard-base") }
    $watchNew = 0
    $watchModified = 0
    $watchDeleted = 0
    $watchMissing = 0
    $spillNew = 0
    $spillModified = 0
    $spillDeleted = 0
    $spillMissing = 0
    try {
        foreach ($snapshot in $watchSnapshots) {
            $delta = Get-RootSnapshotDelta $snapshot
            $watchNew += $delta.NewEntries
            $watchModified += $delta.ModifiedFiles
            $watchDeleted += $delta.DeletedEntries
            $watchMissing += $delta.MissingRoot
        }
    }
    catch { [void]$cleanupErrors.Add("audit-watched-roots") }
    try {
        foreach ($snapshot in $spillSnapshots) {
            $delta = Get-RootSnapshotDelta $snapshot
            $spillNew += $delta.NewEntries
            $spillModified += $delta.ModifiedFiles
            $spillDeleted += $delta.DeletedEntries
            $spillMissing += $delta.MissingRoot
        }
    }
    catch { [void]$cleanupErrors.Add("audit-spill-roots") }

    $workspacePackScratchRemaining = -1
    $guardRemaining = -1
    $guardBaseRemaining = -1
    $engineProcessesRemaining = -1
    $unclaimedEngineProcessCount = -1
    try {
        $workspacePackScratchRemaining = [int](
            $workspacePackScratchCreated -and
            (Test-Path -LiteralPath $workspacePackScratchPath))
        $guardRemaining = [int](Test-Path -LiteralPath $guardRoot)
        $guardBaseRemaining = [int](Test-Path -LiteralPath $guardBase)
        $engineProcessesRemaining = @(Get-EngineProcessRows).Count
        $unclaimedEngineProcessCount =
            $unclaimedEngineProcessesObserved.Count
    }
    catch { [void]$cleanupErrors.Add("audit-final-boundaries") }
    try {
        if ($mutexAcquired -and $mutex) {
            $mutex.ReleaseMutex()
            $mutexAcquired = $false
        }
        if ($mutex) {
            $mutex.Dispose()
            $mutex = $null
        }
    }
    catch { [void]$cleanupErrors.Add("release-mutex") }

    $cleanupResult = [pscustomobject]@{
        WorkspacePackScratchRemaining = $workspacePackScratchRemaining
        GuardRemaining = $guardRemaining
        GuardBaseRemaining = $guardBaseRemaining
        EngineProcessesRemaining = $engineProcessesRemaining
        UnclaimedEngineProcessesObserved =
            $unclaimedEngineProcessCount
        NewWatchedEntries = $watchNew
        ModifiedWatchedFiles = $watchModified
        DeletedWatchedEntries = $watchDeleted
        MissingWatchedRoots = $watchMissing
        NewSpillEntries = $spillNew
        ModifiedSpillFiles = $spillModified
        DeletedSpillEntries = $spillDeleted
        MissingSpillRoots = $spillMissing
        CleanupPhaseErrorCount = $cleanupErrors.Count
        CleanupPhaseErrors = $cleanupErrors.ToArray()
    }
    Write-Output ("CLEANUP " + ($cleanupResult | ConvertTo-Json -Compress))
}

$cleanupPassed = $cleanupResult -and
    $cleanupResult.WorkspacePackScratchRemaining -eq 0 -and
    $cleanupResult.GuardRemaining -eq 0 -and
    $cleanupResult.GuardBaseRemaining -eq 0 -and
    $cleanupResult.EngineProcessesRemaining -eq 0 -and
    $cleanupResult.UnclaimedEngineProcessesObserved -eq 0 -and
    $cleanupResult.NewWatchedEntries -eq 0 -and
    $cleanupResult.ModifiedWatchedFiles -eq 0 -and
    $cleanupResult.DeletedWatchedEntries -eq 0 -and
    $cleanupResult.MissingWatchedRoots -eq 0 -and
    $cleanupResult.NewSpillEntries -eq 0 -and
    $cleanupResult.ModifiedSpillFiles -eq 0 -and
    $cleanupResult.DeletedSpillEntries -eq 0 -and
    $cleanupResult.MissingSpillRoots -eq 0 -and
    $cleanupResult.CleanupPhaseErrorCount -eq 0

$safeRunError = ""
if (-not $runSucceeded) {
    if ([string]::IsNullOrWhiteSpace($runError)) {
        $runError = "Ordinary persistence proof did not complete."
    }
    $projectDirectory = ""
    if (-not [string]::IsNullOrWhiteSpace($projectFile)) {
        $projectDirectory = Split-Path -Parent $projectFile
    }
    $safeRunError = ConvertTo-SafeEvidenceLine `
        -Line $runError `
        -GuardRoot $guardRoot `
        -ProjectDirectory $projectDirectory `
        -ResolvedAddonRoots @($runtimeAddonPath)
    if ([string]::IsNullOrWhiteSpace($safeRunError)) {
        $safeRunError = "Ordinary persistence proof failed without safe evidence."
    }
}
if (-not $cleanupPassed) {
    if (-not [string]::IsNullOrWhiteSpace($safeRunError)) {
        [Console]::Error.WriteLine($safeRunError)
    }
    [Console]::Error.WriteLine(
        "Ordinary persistence proof cleanup did not return every boundary to zero.")
    exit 2
}
if (-not $runSucceeded) {
    [Console]::Error.WriteLine($safeRunError)
    exit 1
}
if (-not $PreflightOnly) {
    if (-not $proofSummary) {
        [Console]::Error.WriteLine(
            "Ordinary persistence proof completed without a terminal summary.")
        exit 1
    }
    Write-Output ("PROOF " + ($proofSummary | ConvertTo-Json -Compress))
}
exit 0

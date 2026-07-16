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
    [int]$StageTimeoutSeconds = 300,

    [ValidateRange(30, 900)]
    [int]$PackTimeoutSeconds = 180,

    [ValidateRange(100, 5000)]
    [int]$PollMilliseconds = 500,

    [ValidateRange(1, 60)]
    [int]$ResultGraceSeconds = 5,

    [switch]$PreflightOnly
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
    shutdown_checkpoint = 1
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
$script:MissionHeader = "Missions/HST_Everon.conf"
$script:ScenarioId = "{6985327711302100}Missions/HST_Everon.conf"
$script:ProjectId = "698532771130111D"
$script:OwnerMagic = "partisan_ordinary_campaign_persistence_owner_v1"
$script:GuardMagic = "partisan_ordinary_campaign_persistence_guard_v1"
$script:CarrierMagic = "partisan_ordinary_campaign_persistence_carrier_v1"
$script:ResultMagic = "partisan_ordinary_campaign_persistence_result_v1"
$script:EndBridgeReceiptMagic =
    "partisan_ordinary_campaign_end_bridge_receipt_v1"
$script:OwnerPurpose = "ordinary_campaign_persistence"
$script:AuthorityVersion = 1
$script:SentinelVersion = 1
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

    # Every vector emitted by this wrapper rejects embedded quote characters.
    # This bounded parser therefore needs only the quoting form produced by
    # ConvertTo-NativeArgument: one unquoted token or one double-quoted token.
    $result = New-Object Collections.Generic.List[string]
    $matches = [regex]::Matches($CommandLine, '"([^"]*)"|([^\s]+)')
    foreach ($match in $matches) {
        if ($match.Groups[1].Success) {
            $result.Add($match.Groups[1].Value)
        }
        else {
            $result.Add($match.Groups[2].Value)
        }
    }
    return $result.ToArray()
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
    $safe = [regex]::Replace($safe, '\b[0-9]{15,20}\b', '<id>')
    $safe = [regex]::Replace(
        $safe,
        '(?<![0-9])(?:[0-9]{1,3}\.){3}[0-9]{1,3}(?![0-9])',
        '<ip>')
    if ($safe.Length -gt 500) {
        $safe = $safe.Substring(0, 500)
    }
    return $safe
}

function Get-GuardedProcessDiagnosticSummary {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    $guardRoot = [IO.Path]::GetFullPath(
        (Split-Path -Parent $WorkingDirectory))
    if (-not (Test-Path -LiteralPath $guardRoot -PathType Container)) {
        return "no guarded diagnostics"
    }
    $diagnosticLines = New-Object Collections.Generic.List[string]
    $fallbackLines = New-Object Collections.Generic.List[string]
    $files = @(Get-ChildItem -LiteralPath $guardRoot -Recurse -File -Force `
        -Filter "*.log" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 40)
    foreach ($file in $files) {
        if ($file.Length -le 0 -or $file.Length -gt 33554432) {
            continue
        }
        try {
            foreach ($line in @(Get-Content -LiteralPath $file.FullName -Tail 1200 `
                -ErrorAction Stop)) {
                $text = ConvertTo-SafeEvidenceLine `
                    -Line ([string]$line) `
                    -GuardRoot $guardRoot `
                    -ProjectDirectory $ProjectDirectory `
                    -ResolvedAddonRoots $ResolvedAddonRoots
                if ([string]::IsNullOrWhiteSpace($text)) {
                    continue
                }
                if ($text -match "(?i)(\s\(E\):|fatal|can't compile|cannot be packed|failed script compilation|broken expression|invalid statement|syntax error|unexpected scope|unknown type|undefined)") {
                    [void]$diagnosticLines.Add($text)
                }
                elseif ($text -match '(?i)(SCRIPT\s*\(W\)|failed|gproj|ResourceManager)') {
                    [void]$fallbackLines.Add($text)
                }
                if ($diagnosticLines.Count -ge 12) {
                    break
                }
            }
        }
        catch {}
        if ($diagnosticLines.Count -ge 12) {
            break
        }
    }
    if ($diagnosticLines.Count -eq 0) {
        if ($fallbackLines.Count -eq 0) {
            return "no matching guarded diagnostic lines"
        }
        return (@($fallbackLines | Select-Object -First 12) -join " | ")
    }
    return ($diagnosticLines.ToArray() -join " | ")
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
            throw "$Label exceeded its guarded process deadline."
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
        "m_bDisposableProfile")) {
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
        -not [bool]$Owner.m_bDisposableProfile) {
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
        "m_bAllowCanonicalCampaignOverwrite")) {
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
        [bool]$Guard.m_bAllowCanonicalCampaignOverwrite -ne $allowWrite) {
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
        "m_sExpectedProfileFallbackFingerprint",
        "m_sProfileFallbackReadBackFingerprint",
        "m_bProfileFallbackReadBackExact",
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
        "m_bProfileFallbackReadBackExact")) {
        if ($Result.$property -isnot [bool]) {
            throw "$label contains a non-boolean invariant."
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
        [string]::IsNullOrWhiteSpace([string]$Result.m_sEvidence)) {
        throw "$label omitted a required success invariant."
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
        "m_sLatestProfileFallbackFingerprint")) {
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
        [Parameter(Mandatory = $true)][string]$ServerConfigPath,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][string]$Stage,
        [AllowEmptyString()][string]$LoadSavePointId
    )

    $baseProject = Join-Path $RuntimeAddonPath "data\ArmaReforger.gproj"
    if (-not (Test-Path -LiteralPath $baseProject -PathType Leaf)) {
        throw "The packed base game project is unavailable."
    }
    $arguments = @(
        "-addonsDir", ($RuntimeAddonPath + "," + $PackedAddonsParent),
        "-gproj", $baseProject,
        "-config", $ServerConfigPath,
        "-profile", $ProfileRoot,
        "-rpl-timeout-disable",
        "-noThrow",
        "-backendLocalStorage",
        "-maxFPS", "30")
    if (-not [string]::IsNullOrWhiteSpace($LoadSavePointId)) {
        Assert-NativeSavePointId $LoadSavePointId "$Stage load save point"
        $arguments += @("-loadSessionSave", $LoadSavePointId)
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

    foreach ($forbidden in @("-world", "-addons", "-forceupdate")) {
        if ($arguments -ccontains $forbidden) {
            throw "$Stage contains forbidden loose-project argument $forbidden."
        }
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
    if ($Stage -in @(
        "autosave_checkpoint", "profile_fallback_verify") -and
        $arguments -ccontains "-loadSessionSave") {
        throw "$Stage must start without a native load UUID."
    }
    if ($Stage -notin @(
        "autosave_checkpoint", "profile_fallback_verify") -and
        $arguments -cnotcontains "-loadSessionSave") {
        throw "$Stage requires one explicit native load UUID."
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
    }
}

function Invoke-OrdinaryCampaignStage {
    param(
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent,
        [Parameter(Mandatory = $true)][string]$ServerConfigPath,
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
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    if ($script:Stages -cnotcontains $Stage) {
        throw "Unknown ordinary persistence stage."
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
    $canonicalBefore = ""
    if (-not $allowCanonicalWrite) {
        if (-not (Test-Path -LiteralPath $canonicalPath -PathType Leaf)) {
            throw "$Stage requires an existing canonical fallback."
        }
        $canonicalBefore = Get-FileSignature -Path $canonicalPath
    }

    $arguments = Get-StageArgumentVector `
        -RuntimeAddonPath $RuntimeAddonPath `
        -PackedAddonsParent $PackedAddonsParent `
        -ServerConfigPath $ServerConfigPath `
        -ProfileRoot $ProfileRoot `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -Stage $Stage `
        -LoadSavePointId $LoadSavePointId
    $assertStageResultCommand = ${function:Assert-StageResult}
    $validator = {
        param($candidate)
        & $assertStageResultCommand `
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
    }.GetNewClosure()
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

    if (Test-Path -LiteralPath $guardPath) {
        throw "$Stage did not consume its one-use engine guard."
    }
    $canonicalUnchanged = $null
    if (-not $allowCanonicalWrite) {
        if (-not (Test-Path -LiteralPath $canonicalPath -PathType Leaf)) {
            throw "$Stage removed its canonical fallback."
        }
        $canonicalUnchanged = (Get-FileSignature -Path $canonicalPath) -ceq
            $canonicalBefore
        if (-not $canonicalUnchanged) {
            throw "$Stage rewrote its read-only canonical fallback."
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
        CanonicalFallbackUnchanged = $canonicalUnchanged
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
            ActiveSave = [string]$result.m_sActiveSavePointId
            CommittedSave = [string]$result.m_sCreatedSavePointId
            SourceDigest = (Get-FileNameSafeDigest `
                ([string]$result.m_sSourceFingerprint))
            FinalDigest = (Get-FileNameSafeDigest `
                ([string]$result.m_sProfileFallbackReadBackFingerprint))
            NoSaveVerification = -not [bool]$result.m_bSavePointRequested
            EndBridgeExact = $null -ne $endBridgeReceipt
            CanonicalFallbackUnchanged = $canonicalUnchanged
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
$primaryPartisanRoot = Join-Path $primaryProfileRoot "profile\Partisan"
$fallbackPartisanRoot = Join-Path $fallbackProfileRoot "profile\Partisan"
$primaryDebugRoot = Join-Path $primaryPartisanRoot "debug"
$fallbackDebugRoot = Join-Path $fallbackPartisanRoot "debug"
$workingRoot = Join-Path $guardRoot "working"
$tempRoot = Join-Path $guardRoot "temp"
$serverConfigPath = Join-Path $guardRoot "ordinary-server-config.json"
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
$fallbackCanonicalPath = Join-Path $fallbackPartisanRoot (
    "HST_CampaignSaveData.json")

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
    foreach ($resource in @($WorldResource, $script:MissionHeader)) {
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
        $watchSnapshots.Add((New-RootSnapshot -Root $root))
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
        $tempRoot)) {
        [void](New-Item -ItemType Directory -Path $directory -Force)
        Assert-NoReparsePathAncestry -Path $directory
    }
    if (Test-Path -LiteralPath $packedAddonPath) {
        throw "The packed add-on output path was not fresh."
    }

    $serverConfig = [ordered]@{
        game = [ordered]@{
            name = "Partisan ordinary persistence proof"
            password = ""
            passwordAdmin = ""
            scenarioId = $script:ScenarioId
            maxPlayers = 1
            visible = $false
            gameProperties = [ordered]@{
                fastValidation = $true
                battlEye = $false
                persistence = [ordered]@{
                    autoSaveInterval = 60
                    saveRetention = 10
                    loadSessionSave = $true
                    keepSessionSave = $false
                }
                missionHeader = [ordered]@{
                    m_eSaveTypes = 15
                }
            }
            mods = @(
                [ordered]@{
                    modId = $script:ProjectId
                    name = "Partisan"
                    required = $true
                })
        }
    }
    Write-JsonUtf8NoBom -Path $serverConfigPath -Value $serverConfig
    $validatedConfig = Read-JsonArtifact -Path $serverConfigPath
    $validatedMods = @($validatedConfig.game.mods)
    foreach ($property in @(
        "autoSaveInterval",
        "saveRetention",
        "loadSessionSave",
        "keepSessionSave")) {
        Assert-JsonProperty `
            -Value $validatedConfig.game.gameProperties.persistence `
            -PropertyName $property `
            -ArtifactLabel "guarded server persistence config"
    }
    if ([string]$validatedConfig.game.scenarioId -cne $script:ScenarioId -or
        [int]$validatedConfig.game.gameProperties.missionHeader.m_eSaveTypes -ne
            15 -or
        [bool]$validatedConfig.game.visible -or
        [int]$validatedConfig.game.gameProperties.persistence.autoSaveInterval -ne 60 -or
        [int]$validatedConfig.game.gameProperties.persistence.saveRetention -ne 10 -or
        $validatedConfig.game.gameProperties.persistence.loadSessionSave -isnot [bool] -or
        -not [bool]$validatedConfig.game.gameProperties.persistence.loadSessionSave -or
        $validatedConfig.game.gameProperties.persistence.keepSessionSave -isnot [bool] -or
        [bool]$validatedConfig.game.gameProperties.persistence.keepSessionSave -or
        $validatedMods.Count -ne 1 -or
        [string]$validatedMods[0].modId -cne $script:ProjectId -or
        [string]$validatedMods[0].name -cne "Partisan" -or
        $validatedMods[0].required -isnot [bool] -or
        -not [bool]$validatedMods[0].required) {
        throw "The guarded server config failed its exact gate."
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
                -ServerConfigPath $serverConfigPath `
                -ProfileRoot $profileRoot `
                -SessionNonce $nonce `
                -StageNonce $stageNonce `
                -RunId $runId `
                -Stage $stage `
                -LoadSavePointId $loadId)
        })
    }
    $keepSessionSaveOverrideCount = @($preflightVectors | Where-Object {
        $_.Arguments -icontains "-keepSessionSave"
    }).Count
    $autoShutdownStages = @($preflightVectors | Where-Object {
        $_.Arguments -ccontains "-autoshutdown"
    } | ForEach-Object { $_.Stage })
    if ($keepSessionSaveOverrideCount -ne 0 -or
        $autoShutdownStages.Count -ne 1 -or
        [string]$autoShutdownStages[0] -cne "shutdown_checkpoint") {
        throw "Preflight stage vectors failed their retention or shutdown gate."
    }

    if ($PreflightOnly) {
        $runSucceeded = $true
        Write-Output ("PREFLIGHT " + ([pscustomobject]@{
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            Settings = $buildIdentity.SettingsSchemaVersion
            World = $WorldResource
            Scenario = $script:ScenarioId
            PackArgumentTokenCount = $packArguments.Count
            StageCount = $preflightVectors.Count
            PackedLaunch = $true
            KeepSessionSave = $false
            KeepSessionSaveCLIAbsent =
                $keepSessionSaveOverrideCount -eq 0
            LoadSessionSaveConfig =
                [bool]$validatedConfig.game.gameProperties.persistence.loadSessionSave
            ShutdownHasAutoShutdown =
                $autoShutdownStages.Count -eq 1
            BackendLocalStorage = $true
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
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
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
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
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
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
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
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($shutdown.SafeSummary)
        Write-Output ("STAGE " + ($shutdown.SafeSummary | ConvertTo-Json -Compress))
        $u2 = [string]$shutdown.Result.m_sCreatedSavePointId
        $fallbackGeneration3 =
            [string]$shutdown.Result.m_sProfileFallbackReadBackFingerprint
        $sentinelGeneration3 =
            [string]$shutdown.Result.m_sSentinelFingerprint
        if (-not (Test-Path -LiteralPath $primaryCanonicalPath -PathType Leaf)) {
            throw "Shutdown checkpoint omitted its canonical fallback."
        }
        $shutdownCanonicalSignature = Get-FileSignature $primaryCanonicalPath

        $nativeVerify = Invoke-OrdinaryCampaignStage `
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
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
        if ((Get-FileSignature $primaryCanonicalPath) -cne
            $shutdownCanonicalSignature) {
            throw "Native verification changed the shutdown canonical fallback."
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
        Copy-Item -LiteralPath $primaryCarrierPath `
            -Destination $fallbackCarrierPath
        if ((Get-FileSignature $fallbackCanonicalPath) -cne
            $shutdownCanonicalSignature) {
            throw "Fallback profile did not receive the exact shutdown fallback."
        }
        Assert-FallbackProfilePrelaunchFiles `
            -ProfileRoot $fallbackProfileRoot `
            -ExpectedFiles @(
                $fallbackCanonicalPath,
                $fallbackOwnerPath,
                $fallbackCarrierPath)

        $fallbackVerify = Invoke-OrdinaryCampaignStage `
            -ExecutablePath $executablePath `
            -RepositoryRoot $repositoryRoot `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackedAddonsParent $packedAddonsParent `
            -ServerConfigPath $serverConfigPath `
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
        if ((Get-FileSignature $fallbackCanonicalPath) -cne
            $shutdownCanonicalSignature) {
            throw "Fallback verification changed its read-only canonical source."
        }

        if ($stageOutcomes.Count -ne 5 -or
            $unclaimedEngineProcessesObserved.Count -ne 0) {
            throw "The five-process proof did not complete its exact stage set."
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
            SentinelDigest = Get-FileNameSafeDigest $sentinelGeneration3
            NativeVerificationNoSave =
                -not [bool]$nativeVerify.Result.m_bSavePointRequested
            FallbackVerificationNoSave =
                -not [bool]$fallbackVerify.Result.m_bSavePointRequested
            FallbackSource =
                [string]$fallbackVerify.Result.m_sSource
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
        if ($guardOwnership -and
            -not (Remove-ExactOwnedGuard $guardOwnership $guardBase)) {
            throw "The nonce-owned guard could not be removed."
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

if (-not $cleanupPassed) {
    [Console]::Error.WriteLine(
        "Ordinary persistence proof cleanup did not return every boundary to zero.")
    exit 2
}
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

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeAddonRoot,

    [string]$ProjectPath = "",
    [string]$WorldResource = "Worlds/HST_Dev/HST_Dev.ent",
    [string[]]$WatchedRoots = @(),
    [string[]]$SpillRoots = @(),

    [ValidateRange(30, 3600)]
    [int]$StageTimeoutSeconds = 300,

    [ValidateRange(100, 5000)]
    [int]$PollMilliseconds = 500,

    [ValidateRange(1, 60)]
    [int]$ResultGraceSeconds = 5,

    [switch]$PreflightOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:CutName = "outbound_virtual"
$script:OwnerMagic = "partisan_exact_counterattack_restart_owner_v1"
$script:GuardMagic = "partisan_exact_counterattack_restart_guard_v1"
$script:CarrierMagic = "partisan_exact_counterattack_restart_carrier_v1"
$script:ResultMagic = "partisan_exact_counterattack_restart_result_v1"
$script:OwnerPurpose = "exact_counterattack_external_restart"
$script:AuthorityVersion = 1
$script:SentinelVersion = 1
$script:GuardLeafPrefix = "PartisanCounterattackRestartGuard_"
$script:GuardBaseLeaf = "PartisanCounterattackRestart"
$script:GuardSentinelLeaf = ".partisan-counterattack-restart-owner"
$script:MutexName = "Local\PartisanCounterattackRestartGuard"
$script:SnapshotHashMaximumBytes = 65536
$script:ExpectedContinuationMeters = 75.0

if (-not ("PartisanCounterattackGuardedJob" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public sealed class PartisanCounterattackGuardedJob : IDisposable
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

    public PartisanCounterattackGuardedJob()
    {
        _handle = CreateJobObject(IntPtr.Zero, null);
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException("Unable to create guarded process job.");

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
            new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
        // JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
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
            "Guarded process job membership exceeded the safety limit.");
    }

    public void Dispose()
    {
        if (_handle == IntPtr.Zero)
            return;
        CloseHandle(_handle);
        _handle = IntPtr.Zero;
    }
}

public sealed class PartisanCounterattackSuspendedProcess : IDisposable
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

    public PartisanCounterattackSuspendedProcess(
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
            throw new InvalidOperationException(
                "Suspended runtime thread is unavailable.");
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

public static class PartisanCounterattackNativeCommandLine
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
                "Unable to parse native command line.");
        try
        {
            string[] result = new string[count];
            for (int index = 0; index < count; index++)
            {
                IntPtr value = Marshal.ReadIntPtr(
                    pointer,
                    index * IntPtr.Size);
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

    $tokens = @([PartisanCounterattackNativeCommandLine]::Split($CommandLine))
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
            $stream,
            [Text.Encoding]::UTF8,
            $true,
            4096,
            $true)
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
    if ($safe.Length -gt 500) {
        $safe = $safe.Substring(0, 500)
    }
    return $safe
}

function Get-StringDigest {
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Value)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($Value)
        $hash = $algorithm.ComputeHash($bytes)
        return ([BitConverter]::ToString($hash).Replace("-", "").ToLowerInvariant())
    }
    finally {
        $algorithm.Dispose()
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

function Assert-JsonProperty {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string]$PropertyName,
        [Parameter(Mandatory = $true)][string]$ArtifactLabel
    )

    if ($null -eq $Value -or
        $Value.PSObject.Properties.Name -notcontains $PropertyName) {
        throw "$ArtifactLabel is missing a required property."
    }
}

function Read-CheckoutBuildIdentity {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)

    $buildInfoPath = Join-Path $RepositoryRoot "Scripts\Game\HST\HST_BuildInfo.c"
    $campaignStatePath = Join-Path $RepositoryRoot "Scripts\Game\HST\State\HST_CampaignState.c"
    if (-not (Test-Path -LiteralPath $buildInfoPath -PathType Leaf) -or
        -not (Test-Path -LiteralPath $campaignStatePath -PathType Leaf)) {
        throw "Checkout build identity sources are unavailable."
    }

    $buildInfoText = Read-SharedFileText -Path $buildInfoPath
    $campaignStateText = Read-SharedFileText -Path $campaignStatePath
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
    if (-not $shaMatch.Success -or -not $utcMatch.Success -or
        -not $labelMatch.Success -or -not $schemaMatch.Success) {
        throw "Checkout build identity constants could not be parsed."
    }
    if ($shaMatch.Groups[1].Value -notmatch '^[0-9a-f]{40}$' -or
        $labelMatch.Groups[1].Value.Length -gt 120) {
        throw "Checkout build identity constants failed their bounded format gate."
    }

    return [pscustomobject]@{
        BuildSha = $shaMatch.Groups[1].Value
        BuildUtc = $utcMatch.Groups[1].Value
        BuildLabel = $labelMatch.Groups[1].Value
        CampaignSchemaVersion = [int]$schemaMatch.Groups[1].Value
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
        $rootFull,
        [StringComparison]::OrdinalIgnoreCase)) {
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

function Get-SharedReadFileSha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    $stream = $null
    $algorithm = $null
    try {
        $stream = New-Object IO.FileStream(
            $Path,
            [IO.FileMode]::Open,
            [IO.FileAccess]::Read,
            ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
        $algorithm = [Security.Cryptography.SHA256]::Create()
        $bytes = $algorithm.ComputeHash($stream)
        return ([BitConverter]::ToString($bytes)).Replace("-", "").ToLowerInvariant()
    }
    catch [IO.IOException] {
        # A concurrently replaced or exclusively locked monitoring file remains
        # covered by its length/write-time record; hashing is an extra signal.
        return ""
    }
    catch [UnauthorizedAccessException] {
        return ""
    }
    finally {
        if ($algorithm) {
            $algorithm.Dispose()
        }
        if ($stream) {
            $stream.Dispose()
        }
    }
}

function Get-SafeSnapshotEntries {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$ExcludedRoots = @()
    )

    $rootFull = [IO.Path]::GetFullPath($Root)
    $result = New-Object 'Collections.Generic.Dictionary[string,object]' (
        [StringComparer]::OrdinalIgnoreCase)
    $pending = New-Object Collections.Generic.Queue[string]
    $pending.Enqueue($rootFull)
    while ($pending.Count -gt 0) {
        $directory = $pending.Dequeue()
        foreach ($item in @(
            Get-ChildItem -LiteralPath $directory -Force -ErrorAction Stop)) {
            $itemFull = [IO.Path]::GetFullPath($item.FullName)
            if (-not (Test-ContainedPath -Root $rootFull -Candidate $itemFull)) {
                throw "A snapshot entry escaped its monitoring root."
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
                throw "A monitoring tree must not contain reparse points."
            }
            $length = [long]0
            $lastWriteTicks = [long]0
            $contentHash = ""
            if (-not $item.PSIsContainer) {
                $length = [long]$item.Length
                $lastWriteTicks = $item.LastWriteTimeUtc.Ticks
                if ($length -le $script:SnapshotHashMaximumBytes) {
                    $contentHash = Get-SharedReadFileSha256 -Path $itemFull
                }
            }
            $result[$itemFull] = [pscustomobject]@{
                IsDirectory = [bool]$item.PSIsContainer
                Length = $length
                LastWriteTicks = $lastWriteTicks
                ContentHash = $contentHash
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
        if ([string]::IsNullOrWhiteSpace($excludedRoot)) {
            continue
        }
        $resolvedExclusion = [IO.Path]::GetFullPath($excludedRoot)
        if ($resolvedExclusion.Equals(
            $fullRoot,
            [StringComparison]::OrdinalIgnoreCase)) {
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
                    $before.LastWriteTicks -ne $after.LastWriteTicks -or
                    (-not [string]::IsNullOrWhiteSpace($before.ContentHash) -and
                        -not [string]::IsNullOrWhiteSpace($after.ContentHash) -and
                        $before.ContentHash -cne $after.ContentHash)))) {
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

function Write-JsonUtf8NoBom {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $encoding = New-Object Text.UTF8Encoding($false)
    [IO.File]::WriteAllText(
        $Path,
        ($Value | ConvertTo-Json -Depth 12),
        $encoding)
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
        $resolved = [IO.Path]::GetFullPath($Directory)
        if (-not (Test-ContainedPath -Root $GuardBase -Candidate $resolved)) {
            return $null
        }
        $resolvedParent = [IO.Path]::GetFullPath(
            (Split-Path -Parent $resolved)).TrimEnd('\', '/')
        if (-not $resolvedParent.Equals(
            $guardBaseFull,
            [StringComparison]::OrdinalIgnoreCase)) {
            return $null
        }
        $leaf = Split-Path -Leaf $resolved
        $pattern = '^' + [regex]::Escape($script:GuardLeafPrefix) +
            '([0-9a-f]{32})$'
        if ($leaf -notmatch $pattern) {
            return $null
        }
        $nonce = [string]$matches[1]
        if (-not (Test-Path -LiteralPath $resolved -PathType Container)) {
            return $null
        }
        $directoryItem = Get-Item -LiteralPath $resolved -Force
        if (($directoryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }

        $sentinel = Join-Path $resolved $script:GuardSentinelLeaf
        if (-not (Test-Path -LiteralPath $sentinel -PathType Leaf)) {
            return $null
        }
        $sentinelItem = Get-Item -LiteralPath $sentinel -Force
        if (($sentinelItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }
        $ownership = Read-JsonArtifact -Path $sentinel
        foreach ($property in @(
            "version",
            "nonce",
            "guardLeaf",
            "ownerPid",
            "ownerStartUtc",
            "createdUtc",
            "cut")) {
            if ($ownership.PSObject.Properties.Name -notcontains $property) {
                return $null
            }
        }
        if ([int]$ownership.version -ne $script:SentinelVersion -or
            [string]$ownership.nonce -cne $nonce -or
            [string]$ownership.guardLeaf -cne $leaf -or
            [string]$ownership.cut -cne $script:CutName -or
            [int]$ownership.ownerPid -le 0) {
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
            SentinelLastWriteUtc = $sentinelItem.LastWriteTimeUtc
            Cut = [string]$ownership.cut
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

    $candidate = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $candidate) {
        return $false
    }
    try {
        return $candidate.StartTime.ToUniversalTime().Ticks -eq
            $StartUtc.ToUniversalTime().Ticks
    }
    catch {
        # An existing process whose start time cannot be inspected is not safe
        # to classify as dead.
        return $true
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
        Assert-NoReparsePathAncestry -Path $Ownership.Directory
        $current = Read-GuardOwnership `
            -Directory $Ownership.Directory `
            -GuardBase $GuardBase
        if (-not $current -or
            $current.Nonce -cne $Ownership.Nonce -or
            $current.OwnerPid -ne $Ownership.OwnerPid -or
            $current.OwnerStartUtc.Ticks -ne $Ownership.OwnerStartUtc.Ticks) {
            return $false
        }
        $reparseDescendant = Get-ChildItem `
            -LiteralPath $current.Directory `
            -Recurse `
            -Force `
            -ErrorAction Stop |
            Where-Object {
                ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
            } |
            Select-Object -First 1
        if ($reparseDescendant) {
            return $false
        }
        try {
            Remove-Item `
                -LiteralPath $current.Directory `
                -Recurse `
                -Force `
                -ErrorAction Stop
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
        [timespan]$MinimumAge = [timespan]::Zero
    )

    # Reclamation authority comes only from the exact wrapper sentinel. The
    # engine artifact may be missing or damaged after an interrupted process.
    if (-not (Test-Path -LiteralPath $GuardBase -PathType Container)) {
        return 0
    }
    Assert-NoReparsePathAncestry -Path $GuardBase
    $baseItem = Get-Item -LiteralPath $GuardBase -Force
    if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Guard base must not be a reparse point."
    }
    $removed = 0
    foreach ($candidate in @(
        Get-ChildItem -LiteralPath $GuardBase -Directory -Force -ErrorAction Stop)) {
        $ownership = Read-GuardOwnership `
            -Directory $candidate.FullName `
            -GuardBase $GuardBase
        if (-not $ownership -or
            ([DateTime]::UtcNow - $ownership.CreatedUtc) -lt $MinimumAge -or
            ([DateTime]::UtcNow - $ownership.SentinelLastWriteUtc) -lt
                $MinimumAge -or
            (Test-ProcessIdentityAlive `
                -ProcessId $ownership.OwnerPid `
                -StartUtc $ownership.OwnerStartUtc)) {
            continue
        }
        if (-not (Remove-ExactOwnedGuard `
            -Ownership $ownership `
            -GuardBase $GuardBase)) {
            throw "An exact stale counterattack guard could not be reclaimed."
        }
        $removed++
    }
    return $removed
}

function Remove-StaleEmptyGuardDirectories {
    param(
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [timespan]$MinimumAge = [timespan]::Zero
    )

    if (-not (Test-Path -LiteralPath $GuardBase -PathType Container)) {
        return 0
    }
    Assert-NoReparsePathAncestry -Path $GuardBase
    $guardBaseFull = [IO.Path]::GetFullPath($GuardBase).TrimEnd('\', '/')
    if ((Split-Path -Leaf $guardBaseFull) -cne $script:GuardBaseLeaf) {
        throw "Guard base identity is not exact."
    }
    $baseItem = Get-Item -LiteralPath $guardBaseFull -Force -ErrorAction Stop
    if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Guard base must not be a reparse point."
    }

    $removed = 0
    $pattern = '^' + [regex]::Escape($script:GuardLeafPrefix) +
        '[0-9a-f]{32}$'
    foreach ($candidate in @(
        Get-ChildItem -LiteralPath $guardBaseFull -Directory -Force -ErrorAction Stop)) {
        if ($candidate.Name -notmatch $pattern -or
            ($candidate.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
            ([DateTime]::UtcNow - $candidate.CreationTimeUtc) -lt $MinimumAge -or
            ([DateTime]::UtcNow - $candidate.LastWriteTimeUtc) -lt $MinimumAge) {
            continue
        }
        $candidateFull = [IO.Path]::GetFullPath($candidate.FullName)
        $candidateParent = [IO.Path]::GetFullPath(
            (Split-Path -Parent $candidateFull)).TrimEnd('\', '/')
        if (-not $candidateParent.Equals(
            $guardBaseFull,
            [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        if (@(Get-ChildItem `
            -LiteralPath $candidateFull `
            -Force `
            -ErrorAction Stop).Count -ne 0) {
            continue
        }
        # No -Recurse: a concurrently populated directory fails closed.
        Remove-Item -LiteralPath $candidateFull -Force -ErrorAction Stop
        $removed++
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
        catch { }
    }
    $changed = $true
    while ($changed) {
        $changed = $false
        foreach ($row in $rows) {
            $candidateId = [int]$row.ProcessId
            $parentId = [int]$row.ParentProcessId
            if ($Owned.ContainsKey($candidateId) -or
                -not $Owned.ContainsKey($parentId)) {
                continue
            }
            if (-not (Test-ProcessIdentityAlive `
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
    if (-not $Owned.ContainsKey($RootProcessId)) {
        try {
            $rootProcess = Get-Process -Id $RootProcessId -ErrorAction Stop
            $actualRootStart = $rootProcess.StartTime.ToUniversalTime()
            if ($actualRootStart.Ticks -eq $RootStartUtc.Ticks) {
                $Owned[$RootProcessId] = $actualRootStart
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

function Get-LiveOwnedProcessCount {
    param([Parameter(Mandatory = $true)][hashtable]$Owned)

    $remaining = 0
    foreach ($processId in @($Owned.Keys)) {
        if (Test-ProcessIdentityAlive `
            -ProcessId ([int]$processId) `
            -StartUtc $Owned[[int]$processId]) {
            $remaining++
        }
    }
    return $remaining
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
    param([Parameter(Mandatory = $true)][string]$Path)

    $file = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    return "$($file.Length):$($file.LastWriteTimeUtc.Ticks):$hash"
}

function Wait-StableJsonArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][datetime]$DeadlineUtc,
        [int]$RequiredStablePolls = 2
    )

    $lastSignature = ""
    $stablePolls = 0
    while ([DateTime]::UtcNow -lt $DeadlineUtc) {
        if (Test-Path -LiteralPath $Path -PathType Leaf) {
            try {
                $signature = Get-FileSignature -Path $Path
                if ($signature -ceq $lastSignature) {
                    $stablePolls++
                }
                else {
                    $lastSignature = $signature
                    $stablePolls = 1
                }
                if ($stablePolls -ge $RequiredStablePolls) {
                    return Read-JsonArtifact -Path $Path
                }
            }
            catch {
                $stablePolls = 0
                $lastSignature = ""
            }
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    throw "A guarded JSON artifact did not become stable before timeout."
}

function Assert-BuildIdentity {
    param(
        [Parameter(Mandatory = $true)]$Artifact,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ArtifactLabel
    )

    if ([string]$Artifact.m_sBuildSha -cne $ExpectedBuild.BuildSha -or
        [string]$Artifact.m_sBuildUtc -cne $ExpectedBuild.BuildUtc -or
        [string]$Artifact.m_sBuildLabel -cne $ExpectedBuild.BuildLabel -or
        [int]$Artifact.m_iCampaignSchemaVersion -ne
            $ExpectedBuild.CampaignSchemaVersion) {
        throw "$ArtifactLabel build provenance does not match this checkout."
    }
}

function Assert-LowerHexNonce {
    param(
        [Parameter(Mandatory = $true)][string]$Nonce,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Nonce -cnotmatch '^[0-9a-f]{32}$') {
        throw "$Label must be one exact lowercase 32-character nonce."
    }
}

function Assert-PreparedCarrier {
    param(
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$ExpectedBuild
    )

    $label = "outbound_virtual carrier"
    foreach ($property in @(
        "m_sMagic",
        "m_sSessionNonce",
        "m_sRunId",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_sWorld",
        "m_sCutName",
        "m_iCut",
        "m_Expectation",
        "m_iPreparedElapsedSecond",
        "m_fPreparedRouteProgressMeters",
        "m_fPreparedRouteTotalDistanceMeters",
        "m_vPreparedStrategicPosition",
        "m_sPreparedSemanticFingerprint")) {
        Assert-JsonProperty `
            -Value $Carrier `
            -PropertyName $property `
            -ArtifactLabel $label
    }
    if ([string]$Carrier.m_sMagic -cne $script:CarrierMagic -or
        [string]$Carrier.m_sSessionNonce -cne $SessionNonce -or
        [string]$Carrier.m_sRunId -cne $RunId -or
        [string]$Carrier.m_sCutName -cne $script:CutName -or
        [int]$Carrier.m_iCut -ne 0 -or
        [string]::IsNullOrWhiteSpace([string]$Carrier.m_sWorld) -or
        [string]::IsNullOrWhiteSpace(
            [string]$Carrier.m_sPreparedSemanticFingerprint)) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity `
        -Artifact $Carrier `
        -ExpectedBuild $ExpectedBuild `
        -ArtifactLabel $label

    $expectation = $Carrier.m_Expectation
    if ($null -eq $expectation) {
        throw "$label expectation is unavailable."
    }
    foreach ($property in @(
        "m_sOrderId",
        "m_sOperationId",
        "m_sManifestId",
        "m_sManifestHash",
        "m_sBatchId",
        "m_sGroupId",
        "m_sProjectionId",
        "m_sForceId",
        "m_sFactionKey",
        "m_sSourceZoneId",
        "m_sTargetZoneId",
        "m_sDebitMutationId",
        "m_iAttackCost",
        "m_iSupportCost",
        "m_iExpectedAttackPool",
        "m_iExpectedSupportPool",
        "m_iExpectedPoolOperationalMutationCount",
        "m_iAcceptedMemberCount",
        "m_iLivingMemberCount",
        "m_sLivingSlotFingerprint")) {
        Assert-JsonProperty `
            -Value $expectation `
            -PropertyName $property `
            -ArtifactLabel "$label expectation"
    }
    $identityProperties = @(
        "m_sOrderId",
        "m_sOperationId",
        "m_sManifestId",
        "m_sManifestHash",
        "m_sBatchId",
        "m_sGroupId",
        "m_sProjectionId",
        "m_sForceId",
        "m_sFactionKey",
        "m_sSourceZoneId",
        "m_sTargetZoneId",
        "m_sDebitMutationId",
        "m_sLivingSlotFingerprint")
    foreach ($property in $identityProperties) {
        if ([string]::IsNullOrWhiteSpace([string]$expectation.$property)) {
            throw "$label expectation identity is incomplete."
        }
    }
    $attackFunded = [int]$expectation.m_iAttackCost -gt 0 -and
        [int]$expectation.m_iSupportCost -eq 0
    $supportFunded = [int]$expectation.m_iSupportCost -gt 0 -and
        [int]$expectation.m_iAttackCost -eq 0
    if ((-not $attackFunded -and -not $supportFunded) -or
        [int]$expectation.m_iAcceptedMemberCount -le 0 -or
        [int]$expectation.m_iLivingMemberCount -le 0 -or
        [int]$expectation.m_iLivingMemberCount -gt
            [int]$expectation.m_iAcceptedMemberCount -or
        [int]$expectation.m_iExpectedAttackPool -lt 0 -or
        [int]$expectation.m_iExpectedSupportPool -lt 0 -or
        [int]$expectation.m_iExpectedPoolOperationalMutationCount -ne 1) {
        throw "$label expectation counts are invalid."
    }
    $progress = [double]$Carrier.m_fPreparedRouteProgressMeters
    $total = [double]$Carrier.m_fPreparedRouteTotalDistanceMeters
    if ([double]::IsNaN($progress) -or [double]::IsInfinity($progress) -or
        [double]::IsNaN($total) -or [double]::IsInfinity($total) -or
        $progress -le 0.0 -or $total -le $progress -or
        [int]$Carrier.m_iPreparedElapsedSecond -le 0) {
        throw "$label route state is invalid."
    }
    return $Carrier
}

function Assert-StageResult {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]
        [ValidateSet("prepare", "recover", "replay")][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [string]$ExpectedWorld = ""
    )

    $label = "outbound_virtual/$Stage result"
    foreach ($property in @(
        "m_sMagic",
        "m_sSessionNonce",
        "m_sRunId",
        "m_sStage",
        "m_bSuccess",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_sWorld",
        "m_sCutName",
        "m_iCut",
        "m_bRestored",
        "m_bStartupReconcileChanged",
        "m_bSourceExact",
        "m_bContinuationExact",
        "m_bSameStateSemanticNoOp",
        "m_bRuntimeClaimantsZero",
        "m_bPersistedReadBackExact",
        "m_fProgressBeforeMeters",
        "m_fProgressAfterMeters",
        "m_sSourceSemanticFingerprint",
        "m_sFinalSemanticFingerprint",
        "m_sEvidence")) {
        Assert-JsonProperty `
            -Value $Result `
            -PropertyName $property `
            -ArtifactLabel $label
    }
    if ([string]$Result.m_sMagic -cne $script:ResultMagic -or
        [string]$Result.m_sSessionNonce -cne $SessionNonce -or
        [string]$Result.m_sRunId -cne $RunId -or
        [string]$Result.m_sStage -cne $Stage -or
        [string]$Result.m_sCutName -cne $script:CutName -or
        [int]$Result.m_iCut -ne 0 -or
        [string]::IsNullOrWhiteSpace([string]$Result.m_sWorld) -or
        (-not [string]::IsNullOrWhiteSpace($ExpectedWorld) -and
            [string]$Result.m_sWorld -cne $ExpectedWorld)) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity `
        -Artifact $Result `
        -ExpectedBuild $ExpectedBuild `
        -ArtifactLabel $label
    foreach ($property in @(
        "m_bSuccess",
        "m_bRestored",
        "m_bStartupReconcileChanged",
        "m_bSourceExact",
        "m_bContinuationExact",
        "m_bSameStateSemanticNoOp",
        "m_bRuntimeClaimantsZero",
        "m_bPersistedReadBackExact")) {
        if ($Result.$property -isnot [bool]) {
            throw "$label contains a non-boolean invariant."
        }
    }
    if (-not [bool]$Result.m_bSuccess) {
        throw "$label reported failure."
    }
    if (-not [bool]$Result.m_bSourceExact -or
        -not [bool]$Result.m_bRuntimeClaimantsZero -or
        -not [bool]$Result.m_bPersistedReadBackExact -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sSourceSemanticFingerprint) -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sFinalSemanticFingerprint) -or
        [string]::IsNullOrWhiteSpace([string]$Result.m_sEvidence)) {
        throw "$label omitted a required success invariant."
    }
    $before = [double]$Result.m_fProgressBeforeMeters
    $after = [double]$Result.m_fProgressAfterMeters
    if ([double]::IsNaN($before) -or [double]::IsInfinity($before) -or
        [double]::IsNaN($after) -or [double]::IsInfinity($after)) {
        throw "$label progress is not finite."
    }
    $sameProgress = [Math]::Abs($after - $before) -le 0.1
    $sameFingerprint = [string]$Result.m_sSourceSemanticFingerprint -ceq
        [string]$Result.m_sFinalSemanticFingerprint
    if ($Stage -eq "prepare") {
        if ([bool]$Result.m_bRestored -or
            [bool]$Result.m_bStartupReconcileChanged -or
            [bool]$Result.m_bContinuationExact -or
            [bool]$Result.m_bSameStateSemanticNoOp -or
            -not $sameProgress -or -not $sameFingerprint) {
            throw "$label violates the fresh outbound-virtual invariant."
        }
    }
    elseif ($Stage -eq "recover") {
        if (-not [bool]$Result.m_bRestored -or
            -not [bool]$Result.m_bContinuationExact -or
            [bool]$Result.m_bSameStateSemanticNoOp -or
            [Math]::Abs(
                ($after - $before) - $script:ExpectedContinuationMeters) -gt 0.1 -or
            $sameFingerprint) {
            throw "$label violates the first-start continuation invariant."
        }
    }
    else {
        if (-not [bool]$Result.m_bRestored -or
            [bool]$Result.m_bContinuationExact -or
            -not [bool]$Result.m_bSameStateSemanticNoOp -or
            -not $sameProgress -or -not $sameFingerprint) {
            throw "$label violates the second-start semantic no-op invariant."
        }
    }
    return $Result
}

function Get-SafeStageSummary {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)][int]$ExitCode
    )

    return [pscustomobject]@{
        Cut = $script:CutName
        Stage = [string]$Result.m_sStage
        Success = [bool]$Result.m_bSuccess
        Exit = $ExitCode
        Build = ([string]$Result.m_sBuildSha).Substring(0, 12)
        Schema = [int]$Result.m_iCampaignSchemaVersion
        Restored = [bool]$Result.m_bRestored
        StartupChanged = [bool]$Result.m_bStartupReconcileChanged
        SourceExact = [bool]$Result.m_bSourceExact
        ContinuationExact = [bool]$Result.m_bContinuationExact
        SemanticNoOp = [bool]$Result.m_bSameStateSemanticNoOp
        RuntimeClaimantsZero = [bool]$Result.m_bRuntimeClaimantsZero
        ReadBackExact = [bool]$Result.m_bPersistedReadBackExact
        ProgressAdvanced = [double]$Result.m_fProgressAfterMeters -gt
            ([double]$Result.m_fProgressBeforeMeters + 0.1)
        SourceDigest = (Get-StringDigest `
            -Value ([string]$Result.m_sSourceSemanticFingerprint)).Substring(0, 16)
        FinalDigest = (Get-StringDigest `
            -Value ([string]$Result.m_sFinalSemanticFingerprint)).Substring(0, 16)
    }
}

function Get-StageArgumentVector {
    param(
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$ProjectFile,
        [Parameter(Mandatory = $true)][string]$World,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]
        [ValidateSet("prepare", "recover", "replay")][string]$Stage
    )

    return @(
        "-addonsDir", $RuntimeAddonPath,
        "-gproj", $ProjectFile,
        "-world", $World,
        "-profile", $ProfileRoot,
        "-window",
        "-noFocus",
        "-forceupdate",
        "-rpl-timeout-disable",
        "-noThrow",
        "-hstExactCounterattackRestartStage", $Stage,
        "-hstExactCounterattackRestartRunId", $RunId,
        "-hstExactCounterattackRestartCut", $script:CutName,
        "-hstExactCounterattackRestartSessionNonce", $SessionNonce,
        "-hstExactCounterattackRestartStageNonce", $StageNonce)
}

function Assert-EngineOwner {
    param(
        [Parameter(Mandatory = $true)]$Owner,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sPurpose",
        "m_sSessionNonce",
        "m_sRunId",
        "m_sRequestedCut",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_sWorld",
        "m_bDisposableProfile")) {
        Assert-JsonProperty `
            -Value $Owner `
            -PropertyName $property `
            -ArtifactLabel "outbound_virtual engine-visible owner"
    }
    Assert-LowerHexNonce -Nonce $SessionNonce -Label "session nonce"
    if ([string]$Owner.m_sMagic -cne $script:OwnerMagic -or
        [int]$Owner.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Owner.m_sPurpose -cne $script:OwnerPurpose -or
        [string]$Owner.m_sSessionNonce -cne $SessionNonce -or
        [string]$Owner.m_sRunId -cne $RunId -or
        [string]$Owner.m_sRequestedCut -cne $script:CutName -or
        [string]$Owner.m_sWorld -cne $ExpectedWorld -or
        $Owner.m_bDisposableProfile -isnot [bool] -or
        -not [bool]$Owner.m_bDisposableProfile) {
        throw "The outbound_virtual engine-visible owner is not exact."
    }
    Assert-BuildIdentity `
        -Artifact $Owner `
        -ExpectedBuild $ExpectedBuild `
        -ArtifactLabel "outbound_virtual engine-visible owner"
}

function Assert-EngineGuard {
    param(
        [Parameter(Mandatory = $true)]$Guard,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$StageNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]
        [ValidateSet("prepare", "recover", "replay")][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    foreach ($property in @(
        "m_sMagic",
        "m_iVersion",
        "m_sSessionNonce",
        "m_sStageNonce",
        "m_sRunId",
        "m_sRequestedCut",
        "m_sRequestedStage",
        "m_iStageOrdinal",
        "m_sBuildSha",
        "m_sBuildUtc",
        "m_sBuildLabel",
        "m_iCampaignSchemaVersion",
        "m_sWorld",
        "m_bAllowCanonicalCampaignOverwrite")) {
        Assert-JsonProperty `
            -Value $Guard `
            -PropertyName $property `
            -ArtifactLabel "outbound_virtual engine guard"
    }
    Assert-LowerHexNonce -Nonce $SessionNonce -Label "session nonce"
    Assert-LowerHexNonce -Nonce $StageNonce -Label "$Stage stage nonce"
    $stageOrdinal = @{
        prepare = 0
        recover = 1
        replay = 2
    }[$Stage]
    if ([string]$Guard.m_sMagic -cne $script:GuardMagic -or
        [int]$Guard.m_iVersion -ne $script:AuthorityVersion -or
        [string]$Guard.m_sSessionNonce -cne $SessionNonce -or
        [string]$Guard.m_sStageNonce -cne $StageNonce -or
        [string]$Guard.m_sRunId -cne $RunId -or
        [string]$Guard.m_sRequestedCut -cne $script:CutName -or
        [string]$Guard.m_sRequestedStage -cne $Stage -or
        [int]$Guard.m_iStageOrdinal -ne [int]$stageOrdinal -or
        [string]$Guard.m_sWorld -cne $ExpectedWorld -or
        $Guard.m_bAllowCanonicalCampaignOverwrite -isnot [bool] -or
        -not [bool]$Guard.m_bAllowCanonicalCampaignOverwrite) {
        throw "The outbound_virtual one-use engine stage lease is not exact."
    }
    Assert-BuildIdentity `
        -Artifact $Guard `
        -ExpectedBuild $ExpectedBuild `
        -ArtifactLabel "outbound_virtual one-use engine stage lease"
}

function Invoke-RestartStage {
    param(
        [Parameter(Mandatory = $true)][string]$ExecutablePath,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$ProjectFile,
        [Parameter(Mandatory = $true)][string]$World,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$DebugDirectory,
        [Parameter(Mandatory = $true)][string]$GuardedTempDirectory,
        [Parameter(Mandatory = $true)][string]$GuardedWorkingDirectory,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]
        [ValidateSet("prepare", "recover", "replay")][string]$Stage,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [string]$ExpectedWorld = "",
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    $resultPath = Join-Path $DebugDirectory (
        "HST_ExactCounterattackRestart_{0}.{1}.json" -f $RunId, $Stage)
    if (Test-Path -LiteralPath $resultPath) {
        throw "outbound_virtual/$Stage result storage was not fresh."
    }

    Assert-LowerHexNonce -Nonce $SessionNonce -Label "session nonce"
    $stageNonce = [Guid]::NewGuid().ToString("N")
    Assert-LowerHexNonce -Nonce $stageNonce -Label "$Stage stage nonce"
    $guardPath = Join-Path $DebugDirectory (
        "HST_ExactCounterattackRestart_{0}.guard.json" -f $RunId)
    if (Test-Path -LiteralPath $guardPath) {
        throw "outbound_virtual/$Stage stage-lease storage was not fresh."
    }
    $stageOrdinal = @{
        prepare = 0
        recover = 1
        replay = 2
    }[$Stage]
    $engineGuard = [ordered]@{
        m_sMagic = $script:GuardMagic
        m_iVersion = $script:AuthorityVersion
        m_sSessionNonce = $SessionNonce
        m_sStageNonce = $stageNonce
        m_sRunId = $RunId
        m_sRequestedCut = $script:CutName
        m_sRequestedStage = $Stage
        m_iStageOrdinal = [int]$stageOrdinal
        m_sBuildSha = $ExpectedBuild.BuildSha
        m_sBuildUtc = $ExpectedBuild.BuildUtc
        m_sBuildLabel = $ExpectedBuild.BuildLabel
        m_iCampaignSchemaVersion = $ExpectedBuild.CampaignSchemaVersion
        m_sWorld = $World
        m_bAllowCanonicalCampaignOverwrite = $true
    }
    Write-JsonUtf8NoBom -Path $guardPath -Value $engineGuard
    $validatedEngineGuard = Read-JsonArtifact -Path $guardPath
    Assert-EngineGuard `
        -Guard $validatedEngineGuard `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -Stage $Stage `
        -ExpectedBuild $ExpectedBuild `
        -ExpectedWorld $World

    $arguments = @(Get-StageArgumentVector `
        -RuntimeAddonPath $RuntimeAddonPath `
        -ProjectFile $ProjectFile `
        -World $World `
        -ProfileRoot $ProfileRoot `
        -SessionNonce $SessionNonce `
        -StageNonce $stageNonce `
        -RunId $RunId `
        -Stage $Stage)
    $commandLine = (ConvertTo-NativeArgument $ExecutablePath) + " " +
        (($arguments | ForEach-Object {
            ConvertTo-NativeArgument ([string]$_)
        }) -join " ")
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $commandLine `
        -ExpectedExecutable $ExecutablePath `
        -ExpectedArguments $arguments)) {
        throw "outbound_virtual/$Stage native arguments did not round-trip exactly."
    }

    $engineBefore = @(Get-EngineProcessRows).Count
    if ($engineBefore -ne 0) {
        throw "An engine process appeared before outbound_virtual/$Stage."
    }

    $process = $null
    $job = $null
    $suspendedLauncher = $null
    $ownedProcesses = @{}
    $rootProcessId = 0
    $rootStartUtc = [DateTime]::MinValue
    $rootExitCode = $null
    $result = $null
    $stageError = $null
    $stageCleanupErrors = New-Object Collections.Generic.List[string]
    $ownedRemaining = -1
    $engineAfter = -1
    try {
        $job = New-Object PartisanCounterattackGuardedJob
        if (@(Get-EngineProcessRows).Count -ne 0) {
            throw "An engine process appeared during outbound_virtual/$Stage preflight."
        }

        $previousTemp = [Environment]::GetEnvironmentVariable(
            "TEMP", [EnvironmentVariableTarget]::Process)
        $previousTmp = [Environment]::GetEnvironmentVariable(
            "TMP", [EnvironmentVariableTarget]::Process)
        try {
            [Environment]::SetEnvironmentVariable(
                "TEMP",
                $GuardedTempDirectory,
                [EnvironmentVariableTarget]::Process)
            [Environment]::SetEnvironmentVariable(
                "TMP",
                $GuardedTempDirectory,
                [EnvironmentVariableTarget]::Process)
            $suspendedLauncher = New-Object `
                PartisanCounterattackSuspendedProcess(
                    $ExecutablePath,
                    $commandLine,
                    $GuardedWorkingDirectory)
        }
        finally {
            [Environment]::SetEnvironmentVariable(
                "TEMP", $previousTemp, [EnvironmentVariableTarget]::Process)
            [Environment]::SetEnvironmentVariable(
                "TMP", $previousTmp, [EnvironmentVariableTarget]::Process)
        }

        $process = $suspendedLauncher.Child
        if (-not $process) {
            throw "outbound_virtual/$Stage did not create its diagnostic process."
        }
        $rootProcessId = $process.Id
        $job.Add($process)
        $rootStartUtc = $process.StartTime.ToUniversalTime()
        $ownedProcesses[$rootProcessId] = $rootStartUtc
        $suspendedLauncher.Resume()
        $suspendedLauncher.Dispose()
        $suspendedLauncher = $null

        Start-Sleep -Milliseconds 500
        $process.Refresh()
        if ($process.HasExited) {
            throw "outbound_virtual/$Stage exited before command-line verification."
        }
        $processRow = Get-CimInstance `
            Win32_Process `
            -Filter "ProcessId=$rootProcessId" `
            -ErrorAction Stop
        if (-not $processRow -or
            -not (Test-ExactNativeArgumentVector `
                -CommandLine ([string]$processRow.CommandLine) `
                -ExpectedExecutable $ExecutablePath `
                -ExpectedArguments $arguments)) {
            throw "outbound_virtual/$Stage launched with a non-exact argument vector."
        }

        $deadlineUtc = [DateTime]::UtcNow.AddSeconds($StageTimeoutSeconds)
        $exitObservedUtc = [DateTime]::MinValue
        $lastSignature = ""
        $stablePolls = 0
        while ([DateTime]::UtcNow -lt $deadlineUtc) {
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
                    [void]$UnclaimedEngineProcessesObserved.Add(
                        "$($engineProcess.ProcessName):$($engineProcess.StartTime.ToUniversalTime().Ticks)")
                }
                catch { }
            }
            if ($UnclaimedEngineProcessesObserved.Count -ne 0) {
                throw "An unowned engine process appeared during outbound_virtual/$Stage."
            }

            if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
                try {
                    $signature = Get-FileSignature -Path $resultPath
                    if ($signature -ceq $lastSignature) {
                        $stablePolls++
                    }
                    else {
                        $lastSignature = $signature
                        $stablePolls = 1
                    }
                    if ($stablePolls -ge 2) {
                        $candidate = Read-JsonArtifact -Path $resultPath
                        $result = Assert-StageResult `
                            -Result $candidate `
                            -SessionNonce $SessionNonce `
                            -RunId $RunId `
                            -Stage $Stage `
                            -ExpectedBuild $ExpectedBuild `
                            -ExpectedWorld $ExpectedWorld
                    }
                }
                catch {
                    if ($stablePolls -ge 2) {
                        throw
                    }
                    $stablePolls = 0
                    $lastSignature = ""
                }
            }

            $process.Refresh()
            if ($process.HasExited -and
                $exitObservedUtc -eq [DateTime]::MinValue) {
                $exitObservedUtc = [DateTime]::UtcNow
                $rootExitCode = $process.ExitCode
            }

            $liveOwned = Get-LiveOwnedProcessCount -Owned $ownedProcesses
            if ($result -and $liveOwned -eq 0) {
                if ($null -eq $rootExitCode) {
                    $process.Refresh()
                    if (-not $process.HasExited) {
                        throw "outbound_virtual/$Stage retained its root process."
                    }
                    $rootExitCode = $process.ExitCode
                }
                if ([int]$rootExitCode -ne 0) {
                    throw "outbound_virtual/$Stage diagnostic process returned failure."
                }
                break
            }
            if ($exitObservedUtc -ne [DateTime]::MinValue -and
                -not $result -and
                ([DateTime]::UtcNow - $exitObservedUtc).TotalSeconds -ge
                    $ResultGraceSeconds) {
                throw "outbound_virtual/$Stage exited without a stable exact result."
            }
            Start-Sleep -Milliseconds $PollMilliseconds
        }

        if (-not $result) {
            throw "outbound_virtual/$Stage exceeded its guarded result deadline."
        }
        if ($null -eq $rootExitCode -or [int]$rootExitCode -ne 0 -or
            (Get-LiveOwnedProcessCount -Owned $ownedProcesses) -ne 0) {
            throw "outbound_virtual/$Stage did not reach a clean successful exit."
        }
    }
    catch {
        $stageError = $_.Exception.Message
    }
    finally {
        Invoke-IsolatedCleanupPhase `
            -Name "dispose-suspended-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                if ($suspendedLauncher) {
                    $suspendedLauncher.Dispose()
                    $suspendedLauncher = $null
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "discover-owned-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                if ($rootProcessId -gt 0 -and
                    $rootStartUtc -ne [DateTime]::MinValue) {
                    Update-OwnedProcesses `
                        -Owned $ownedProcesses `
                        -RootProcessId $rootProcessId `
                        -RootStartUtc $rootStartUtc `
                        -Job $job
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "request-root-stop-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                if ($process) {
                    $process.Refresh()
                    if (-not $process.HasExited) {
                        [void]$process.CloseMainWindow()
                        [void]$process.WaitForExit(3000)
                    }
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "force-owned-stop-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                Stop-OwnedProcesses -Owned $ownedProcesses
                Start-Sleep -Milliseconds 300
                Stop-OwnedProcesses -Owned $ownedProcesses
            }
        Invoke-IsolatedCleanupPhase `
            -Name "close-process-job-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                if ($job) {
                    $job.Dispose()
                    $job = $null
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "final-owned-stop-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                Start-Sleep -Milliseconds 300
                Stop-OwnedProcesses -Owned $ownedProcesses
                $ownedRemaining = Get-LiveOwnedProcessCount `
                    -Owned $ownedProcesses
            }
        Invoke-IsolatedCleanupPhase `
            -Name "dispose-root-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                if ($process) {
                    $process.Dispose()
                    $process = $null
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "audit-engine-$Stage" `
            -Errors $stageCleanupErrors `
            -Action {
                $engineAfter = @(Get-EngineProcessRows).Count
            }
    }

    if ($stageCleanupErrors.Count -ne 0 -or
        $ownedRemaining -ne 0 -or $engineAfter -ne 0) {
        throw "outbound_virtual/$Stage process containment cleanup failed."
    }
    if ($stageError) {
        throw $stageError
    }
    if (Test-Path -LiteralPath $guardPath) {
        throw "outbound_virtual/$Stage engine did not consume its one-use stage lease."
    }
    $safeSummary = Get-SafeStageSummary `
        -Result $result `
        -ExitCode ([int]$rootExitCode)
    $safeSummary | Add-Member -NotePropertyName EngineBefore -NotePropertyValue $engineBefore
    $safeSummary | Add-Member -NotePropertyName EngineAfter -NotePropertyValue $engineAfter
    return [pscustomobject]@{
        Result = $result
        ExitCode = [int]$rootExitCode
        SafeSummary = $safeSummary
    }
}

$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$expectedProjectPath = [IO.Path]::GetFullPath(
    (Join-Path $repositoryRoot "addon.gproj"))
$nonce = [Guid]::NewGuid().ToString("N")
$guardBase = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) $script:GuardBaseLeaf))
$guardLeaf = $script:GuardLeafPrefix + $nonce
$guardRoot = [IO.Path]::GetFullPath((Join-Path $guardBase $guardLeaf))
$sentinelPath = Join-Path $guardRoot $script:GuardSentinelLeaf
$runId = "counter_{0}_{1}" -f (
    [DateTime]::UtcNow.ToString(
        "yyyyMMddHHmmss",
        [Globalization.CultureInfo]::InvariantCulture)), $nonce.Substring(0, 20)

$executablePath = ""
$runtimeAddonPath = ""
$projectFile = ""
$profileDirectory = Join-Path $guardRoot "profile\Partisan"
$debugDirectory = Join-Path $profileDirectory "debug"
$guardedTempDirectory = Join-Path $guardRoot "temp"
$guardedWorkingDirectory = Join-Path $guardRoot "working"
$engineOwnerPath = Join-Path $debugDirectory (
    "HST_ExactCounterattackRestart_{0}.owner.json" -f $runId)
$carrierPath = Join-Path $debugDirectory (
    "HST_ExactCounterattackRestart_{0}.carrier.json" -f $runId)
$buildIdentity = $null
$guardOwnership = $null
$wrapperStartUtc = [DateTime]::MinValue
$guardBaseCreated = $false
$guardDirectoryCreated = $false
$mutex = $null
$mutexAcquired = $false
$runError = $null
$runSucceeded = $false
$cleanupResult = $null
$cleanupPhaseErrors = New-Object Collections.Generic.List[string]
$watchSnapshots = New-Object Collections.Generic.List[object]
$spillSnapshots = New-Object Collections.Generic.List[object]
$unclaimedEngineProcessesObserved = New-Object Collections.Generic.HashSet[string]
$stageOutcomes = New-Object Collections.Generic.List[object]
$engineProcessesBefore = -1

try {
    $executablePath = Resolve-ExistingPath -Path $Executable -Kind Leaf
    $runtimeAddonPath = Resolve-ExistingPath `
        -Path $RuntimeAddonRoot `
        -Kind Container
    $supportedExecutables = @(
        "ArmaReforgerSteamDiag.exe",
        "ArmaReforgerServerDiag.exe")
    if ($supportedExecutables -notcontains (Split-Path -Leaf $executablePath)) {
        throw "Executable must be a supported Reforger diagnostic runtime."
    }
    foreach ($packedRuntimeMarker in @("core\data.pak", "data\data.pak")) {
        if (-not (Test-Path `
            -LiteralPath (Join-Path $runtimeAddonPath $packedRuntimeMarker) `
            -PathType Leaf)) {
            throw "RuntimeAddonRoot must be the installed packed game add-on root."
        }
    }

    if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
        $projectFile = Resolve-ExistingPath `
            -Path $expectedProjectPath `
            -Kind Leaf
    }
    else {
        $projectFile = Resolve-ExistingPath -Path $ProjectPath -Kind Leaf
    }
    if (-not $projectFile.Equals(
        $expectedProjectPath,
        [StringComparison]::OrdinalIgnoreCase)) {
        throw "The exact restart proof requires this checkout's addon.gproj."
    }
    if ($WorldResource -cne "Worlds/HST_Dev/HST_Dev.ent") {
        throw "The exact restart proof requires the disposable HST_Dev world."
    }
    $worldFile = Join-Path $repositoryRoot (
        $WorldResource.Replace('/', [IO.Path]::DirectorySeparatorChar))
    [void](Resolve-ExistingPath -Path $worldFile -Kind Leaf)
    $buildIdentity = Read-CheckoutBuildIdentity `
        -RepositoryRoot $repositoryRoot

    $normalizedWatchedRoots = @($WatchedRoots | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    $normalizedSpillRoots = @($SpillRoots | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    if (-not $PreflightOnly -and
        ($normalizedWatchedRoots.Count -eq 0 -or
            $normalizedSpillRoots.Count -eq 0)) {
        throw "A real guarded proof requires explicit watched and spill roots."
    }

    $mutex = New-Object Threading.Mutex($false, $script:MutexName)
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw "Another exact counterattack restart wrapper is active."
    }

    $engineProcessesBefore = @(Get-EngineProcessRows).Count
    if ($engineProcessesBefore -ne 0) {
        throw "Refusing launch while an engine or Workbench process is active."
    }

    # This exact TEMP-owned base is the only tree the wrapper may reclaim.
    # Empty pre-sentinel nonce leaves and exact dead-owner sentinels must also
    # be old enough, so an active or just-starting wrapper fails closed.
    [void](Remove-StaleEmptyGuardDirectories -GuardBase $guardBase)
    [void](Remove-StaleOwnedGuards -GuardBase $guardBase)

    foreach ($root in $normalizedWatchedRoots) {
        $watchSnapshots.Add((New-RootSnapshot -Root $root))
    }
    $spillExclusions = New-Object Collections.Generic.List[string]
    $spillExclusions.Add((Split-Path -Parent $projectFile))
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
    $guardBaseCreated = -not (
        Test-Path -LiteralPath $guardBase -PathType Container)
    if ($guardBaseCreated) {
        [void](New-Item -ItemType Directory -Path $guardBase)
    }
    if (Test-Path -LiteralPath $guardRoot) {
        throw "The nonce-owned guard directory was not fresh."
    }
    [void](New-Item -ItemType Directory -Path $guardRoot)
    $guardDirectoryCreated = $true
    Assert-NoReparsePathAncestry -Path $guardRoot

    $wrapper = Get-Process -Id $PID -ErrorAction Stop
    $wrapperStartUtc = $wrapper.StartTime.ToUniversalTime()
    $ownershipRecord = [ordered]@{
        version = $script:SentinelVersion
        nonce = $nonce
        guardLeaf = $guardLeaf
        ownerPid = $PID
        ownerStartUtc = $wrapperStartUtc.ToString(
            "o", [Globalization.CultureInfo]::InvariantCulture)
        createdUtc = [DateTime]::UtcNow.ToString(
            "o", [Globalization.CultureInfo]::InvariantCulture)
        cut = $script:CutName
    }
    Write-JsonUtf8NoBom -Path $sentinelPath -Value $ownershipRecord
    $guardOwnership = Read-GuardOwnership `
        -Directory $guardRoot `
        -GuardBase $guardBase
    if (-not $guardOwnership -or $guardOwnership.Nonce -cne $nonce) {
        throw "Guard ownership sentinel validation failed."
    }

    foreach ($directory in @(
        $debugDirectory,
        $guardedTempDirectory,
        $guardedWorkingDirectory)) {
        [void](New-Item -ItemType Directory -Path $directory -Force)
        Assert-NoReparsePathAncestry -Path $directory
    }
    if (Test-Path -LiteralPath $engineOwnerPath) {
        throw "The engine-visible disposable owner path was not fresh."
    }
    $engineOwner = [ordered]@{
        m_sMagic = $script:OwnerMagic
        m_iVersion = $script:AuthorityVersion
        m_sPurpose = $script:OwnerPurpose
        m_sSessionNonce = $nonce
        m_sRunId = $runId
        m_sRequestedCut = $script:CutName
        m_sBuildSha = $buildIdentity.BuildSha
        m_sBuildUtc = $buildIdentity.BuildUtc
        m_sBuildLabel = $buildIdentity.BuildLabel
        m_iCampaignSchemaVersion = $buildIdentity.CampaignSchemaVersion
        m_sWorld = $WorldResource
        m_bDisposableProfile = $true
    }
    Write-JsonUtf8NoBom -Path $engineOwnerPath -Value $engineOwner
    $validatedEngineOwner = Read-JsonArtifact -Path $engineOwnerPath
    Assert-EngineOwner `
        -Owner $validatedEngineOwner `
        -SessionNonce $nonce `
        -RunId $runId `
        -ExpectedBuild $buildIdentity `
        -ExpectedWorld $WorldResource

    $safeSynthetic = ConvertTo-SafeEvidenceLine `
        -Line ("proof@example.invalid 12345678901234567 " +
            $guardRoot + " " + (Split-Path -Parent $projectFile) +
            " " + $runtimeAddonPath) `
        -GuardRoot $guardRoot `
        -ProjectDirectory (Split-Path -Parent $projectFile) `
        -ResolvedAddonRoots @($runtimeAddonPath)
    if ($safeSynthetic -notmatch '<email>' -or
        $safeSynthetic -notmatch '<id>' -or
        $safeSynthetic -notmatch '<guard>' -or
        $safeSynthetic -notmatch '<project>' -or
        $safeSynthetic -notmatch '<addon>' -or
        $safeSynthetic.Contains($guardRoot) -or
        $safeSynthetic.Contains($runtimeAddonPath)) {
        throw "Bounded-output redaction self-test failed."
    }

    $preflightStageNonce = [Guid]::NewGuid().ToString("N")
    $preflightArguments = @(Get-StageArgumentVector `
        -RuntimeAddonPath $runtimeAddonPath `
        -ProjectFile $projectFile `
        -World $WorldResource `
        -ProfileRoot $guardRoot `
        -SessionNonce $nonce `
        -StageNonce $preflightStageNonce `
        -RunId $runId `
        -Stage "prepare")
    $preflightCommandLine = (ConvertTo-NativeArgument $executablePath) +
        " " + (($preflightArguments | ForEach-Object {
            ConvertTo-NativeArgument ([string]$_)
        }) -join " ")
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $preflightCommandLine `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $preflightArguments)) {
        throw "Preflight native argument isolation failed."
    }

    if ($PreflightOnly) {
        Write-Output ("PREFLIGHT " + ([pscustomobject]@{
            Cut = $script:CutName
            Stages = 3
            EngineProcessesBefore = $engineProcessesBefore
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            EngineOwnerCreated = $true
            StageLeaseDeferred = $true
            CampaignSaveCopied = $false
            TempIsolated = $true
            KillOnWrapperClose = $true
            ArgumentTokenCount = $preflightArguments.Count
        } | ConvertTo-Json -Compress))
        $runSucceeded = $true
    }
    else {
        $prepare = Invoke-RestartStage `
            -ExecutablePath $executablePath `
            -RuntimeAddonPath $runtimeAddonPath `
            -ProjectFile $projectFile `
            -World $WorldResource `
            -ProfileRoot $guardRoot `
            -DebugDirectory $debugDirectory `
            -GuardedTempDirectory $guardedTempDirectory `
            -GuardedWorkingDirectory $guardedWorkingDirectory `
            -SessionNonce $nonce `
            -RunId $runId `
            -Stage "prepare" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld $WorldResource `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($prepare)
        Write-Output ("STAGE " + (
            $prepare.SafeSummary | ConvertTo-Json -Compress))

        $carrier = Wait-StableJsonArtifact `
            -Path $carrierPath `
            -DeadlineUtc ([DateTime]::UtcNow.AddSeconds($ResultGraceSeconds))
        $carrier = Assert-PreparedCarrier `
            -Carrier $carrier `
            -SessionNonce $nonce `
            -RunId $runId `
            -ExpectedBuild $buildIdentity
        $preparedFingerprint = [string]$carrier.m_sPreparedSemanticFingerprint
        if ([string]$prepare.Result.m_sWorld -cne [string]$carrier.m_sWorld -or
            [string]$prepare.Result.m_sSourceSemanticFingerprint -cne
                $preparedFingerprint -or
            [string]$prepare.Result.m_sFinalSemanticFingerprint -cne
                $preparedFingerprint) {
            throw "The prepare result and durable carrier fingerprint chain diverged."
        }

        $recover = Invoke-RestartStage `
            -ExecutablePath $executablePath `
            -RuntimeAddonPath $runtimeAddonPath `
            -ProjectFile $projectFile `
            -World $WorldResource `
            -ProfileRoot $guardRoot `
            -DebugDirectory $debugDirectory `
            -GuardedTempDirectory $guardedTempDirectory `
            -GuardedWorkingDirectory $guardedWorkingDirectory `
            -SessionNonce $nonce `
            -RunId $runId `
            -Stage "recover" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld ([string]$carrier.m_sWorld) `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($recover)
        Write-Output ("STAGE " + (
            $recover.SafeSummary | ConvertTo-Json -Compress))
        if ([string]$recover.Result.m_sSourceSemanticFingerprint -cne
                $preparedFingerprint -or
            [string]$recover.Result.m_sFinalSemanticFingerprint -ceq
                $preparedFingerprint) {
            throw "The recover result did not continue the prepared fingerprint exactly once."
        }

        $replay = Invoke-RestartStage `
            -ExecutablePath $executablePath `
            -RuntimeAddonPath $runtimeAddonPath `
            -ProjectFile $projectFile `
            -World $WorldResource `
            -ProfileRoot $guardRoot `
            -DebugDirectory $debugDirectory `
            -GuardedTempDirectory $guardedTempDirectory `
            -GuardedWorkingDirectory $guardedWorkingDirectory `
            -SessionNonce $nonce `
            -RunId $runId `
            -Stage "replay" `
            -ExpectedBuild $buildIdentity `
            -ExpectedWorld ([string]$carrier.m_sWorld) `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($replay)
        Write-Output ("STAGE " + (
            $replay.SafeSummary | ConvertTo-Json -Compress))
        $recoveredFingerprint = [string]$recover.Result.m_sFinalSemanticFingerprint
        if ([string]$replay.Result.m_sSourceSemanticFingerprint -cne
                $recoveredFingerprint -or
            [string]$replay.Result.m_sFinalSemanticFingerprint -cne
                $recoveredFingerprint) {
            throw "The replay result was not an exact semantic no-op."
        }

        Write-Output ("RESULT " + ([pscustomobject]@{
            Valid = $true
            Cut = $script:CutName
            StageCount = $stageOutcomes.Count
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            FingerprintChainExact = $true
            AllExitZero = @($stageOutcomes | Where-Object {
                $_.ExitCode -ne 0
            }).Count -eq 0
        } | ConvertTo-Json -Compress))
        $runSucceeded = $true
    }
}
catch {
    $runError = $_.Exception.Message
}
finally {
    $cleanupState = [ordered]@{
        GuardRemaining = -1
        GuardBaseRemaining = -1
        OwnedGuardRootsRemaining = -1
        EngineProcessesRemaining = -1
        NewWatchedEntries = -1
        ModifiedWatchedFiles = -1
        DeletedWatchedEntries = -1
        MissingWatchedRoots = -1
        NewSpillEntries = -1
        ModifiedSpillFiles = -1
        DeletedSpillEntries = -1
        MissingSpillRoots = -1
    }

    Invoke-IsolatedCleanupPhase `
        -Name "remove-exact-owned-guard" `
        -Errors $cleanupPhaseErrors `
        -Action {
            Assert-NoReparsePathAncestry -Path $guardBase
            if (-not $guardOwnership -and $guardDirectoryCreated -and
                (Test-Path -LiteralPath $guardRoot -PathType Container)) {
                $candidateOwnership = Read-GuardOwnership `
                    -Directory $guardRoot `
                    -GuardBase $guardBase
                if ($candidateOwnership -and
                    $candidateOwnership.Nonce -ceq $nonce -and
                    $candidateOwnership.OwnerPid -eq $PID -and
                    $wrapperStartUtc -ne [DateTime]::MinValue -and
                    $candidateOwnership.OwnerStartUtc.Ticks -eq
                        $wrapperStartUtc.Ticks) {
                    $guardOwnership = $candidateOwnership
                }
            }
            if ($guardOwnership) {
                $removeParameters = @{
                    Ownership = $guardOwnership
                    GuardBase = $guardBase
                }
                if (-not (Remove-ExactOwnedGuard @removeParameters)) {
                    throw "Exact nonce-owned guard removal failed."
                }
            }
            elseif ($guardDirectoryCreated -and
                (Test-Path -LiteralPath $guardRoot)) {
                throw "Nonce-owned guard could not be re-authorized for cleanup."
            }
        }
    Invoke-IsolatedCleanupPhase `
        -Name "remove-empty-guard-base" `
        -Errors $cleanupPhaseErrors `
        -Action {
            if (Test-Path -LiteralPath $guardBase -PathType Container) {
                Assert-NoReparsePathAncestry -Path $guardBase
                $baseItem = Get-Item -LiteralPath $guardBase -Force
                if (($baseItem.Attributes -band
                    [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw "Guard base became a reparse point."
                }
                if (@(Get-ChildItem `
                    -LiteralPath $guardBase `
                    -Force `
                    -ErrorAction Stop).Count -eq 0) {
                    Remove-Item `
                        -LiteralPath $guardBase `
                        -Force `
                        -ErrorAction Stop
                }
            }
        }
    Invoke-IsolatedCleanupPhase `
        -Name "audit-guard-roots" `
        -Errors $cleanupPhaseErrors `
        -Action {
            $cleanupState.GuardRemaining = [int](
                Test-Path -LiteralPath $guardRoot)
            $cleanupState.GuardBaseRemaining = [int](
                Test-Path -LiteralPath $guardBase)
            $ownedGuardRoots = 0
            if (Test-Path -LiteralPath $guardBase -PathType Container) {
                Assert-NoReparsePathAncestry -Path $guardBase
                $baseItem = Get-Item -LiteralPath $guardBase -Force
                if (($baseItem.Attributes -band
                    [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw "Guard base became a reparse point before audit."
                }
                $guardPattern = '^' +
                    [regex]::Escape($script:GuardLeafPrefix) +
                    '[0-9a-f]{32}$'
                $ownedGuardRoots = @(
                    Get-ChildItem `
                        -LiteralPath $guardBase `
                        -Directory `
                        -Force `
                        -ErrorAction Stop |
                    Where-Object { $_.Name -match $guardPattern }).Count
            }
            $cleanupState.OwnedGuardRootsRemaining = $ownedGuardRoots
        }
    Invoke-IsolatedCleanupPhase `
        -Name "audit-engine-processes" `
        -Errors $cleanupPhaseErrors `
        -Action {
            $cleanupState.EngineProcessesRemaining = @(Get-EngineProcessRows).Count
        }
    Invoke-IsolatedCleanupPhase `
        -Name "audit-external-boundaries" `
        -Errors $cleanupPhaseErrors `
        -Action {
            $watchNew = 0
            $watchModified = 0
            $watchDeleted = 0
            $watchMissing = 0
            foreach ($snapshot in $watchSnapshots) {
                $delta = Get-RootSnapshotDelta -Snapshot $snapshot
                $watchNew += $delta.NewEntries
                $watchModified += $delta.ModifiedFiles
                $watchDeleted += $delta.DeletedEntries
                $watchMissing += $delta.MissingRoot
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
            $cleanupState.NewWatchedEntries = $watchNew
            $cleanupState.ModifiedWatchedFiles = $watchModified
            $cleanupState.DeletedWatchedEntries = $watchDeleted
            $cleanupState.MissingWatchedRoots = $watchMissing
            $cleanupState.NewSpillEntries = $spillNew
            $cleanupState.ModifiedSpillFiles = $spillModified
            $cleanupState.DeletedSpillEntries = $spillDeleted
            $cleanupState.MissingSpillRoots = $spillMissing
        }
    Invoke-IsolatedCleanupPhase `
        -Name "release-wrapper-lock" `
        -Errors $cleanupPhaseErrors `
        -Action {
            if ($mutexAcquired -and $mutex) {
                $mutex.ReleaseMutex()
                $mutexAcquired = $false
            }
            if ($mutex) {
                $mutex.Dispose()
                $mutex = $null
            }
        }

    $cleanupResult = [pscustomobject]@{
        GuardRemaining = $cleanupState.GuardRemaining
        GuardBaseRemaining = $cleanupState.GuardBaseRemaining
        OwnedGuardRootsRemaining = $cleanupState.OwnedGuardRootsRemaining
        EngineProcessesRemaining = $cleanupState.EngineProcessesRemaining
        UnclaimedEngineProcessesObserved =
            $unclaimedEngineProcessesObserved.Count
        NewWatchedEntries = $cleanupState.NewWatchedEntries
        ModifiedWatchedFiles = $cleanupState.ModifiedWatchedFiles
        DeletedWatchedEntries = $cleanupState.DeletedWatchedEntries
        MissingWatchedRoots = $cleanupState.MissingWatchedRoots
        NewSpillEntries = $cleanupState.NewSpillEntries
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
    $cleanupResult.GuardBaseRemaining -eq 0 -and
    $cleanupResult.OwnedGuardRootsRemaining -eq 0 -and
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

if ($runError) {
    $projectDirectory = ""
    if (-not [string]::IsNullOrWhiteSpace($projectFile)) {
        $projectDirectory = Split-Path -Parent $projectFile
    }
    $safeRunError = ConvertTo-SafeEvidenceLine `
        -Line $runError `
        -GuardRoot $guardRoot `
        -ProjectDirectory $projectDirectory `
        -ResolvedAddonRoots @($runtimeAddonPath)
    Write-Error $safeRunError
    exit 1
}
if (-not $runSucceeded) {
    Write-Error "Exact counterattack restart proof did not reach a terminal result."
    exit 1
}
if (-not $cleanupPassed) {
    Write-Error "Exact counterattack restart cleanup did not return every tracked boundary to zero."
    exit 2
}
exit 0

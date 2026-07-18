[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$ProjectPath,

    [Parameter(Mandatory = $true)]
    [string[]]$AddonRoots,

    [ValidateSet("PC", "XBOX_ONE", "XBOX_SERIES", "PS4", "PS5", "HEADLESS")]
    [string]$Target = "PC",
    [string]$EvidenceDirectory = "",
    [string[]]$DefaultLogRoots = @(),
    [string[]]$SpillRoots = @(),
    [ValidateRange(1, 3600)]
    [int]$TimeoutSeconds = 240,
    [ValidateRange(50, 5000)]
    [int]$PollMilliseconds = 500,
    [ValidateRange(1048576, 134217728)]
    [long]$ParseBudgetBytes = 33554432,
    [switch]$PreflightOnly,
    [Alias("ParserSelfTest")]
    [switch]$SelfTest,
    [switch]$ReturnToCaller
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not ("PartisanWorkbenchGuardJob" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public sealed class PartisanWorkbenchGuardJob : IDisposable
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

    public PartisanWorkbenchGuardJob()
    {
        _handle = CreateJobObject(IntPtr.Zero, null);
        if (_handle == IntPtr.Zero)
            throw new InvalidOperationException("Unable to create Workbench guard job.");

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
            new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
        info.BasicLimitInformation.LimitFlags = 0x00002000;
        int size = Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
        IntPtr pointer = Marshal.AllocHGlobal(size);
        try
        {
            Marshal.StructureToPtr(info, pointer, false);
            if (!SetInformationJobObject(_handle, 9, pointer, (UInt32)size))
                throw new InvalidOperationException("Unable to configure Workbench guard job.");
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
            throw new InvalidOperationException("Unable to assign Workbench to its guard job.");
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
                    throw new InvalidOperationException("Unable to query Workbench guard job.");
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

        throw new InvalidOperationException("Workbench guard job membership exceeded its safety limit.");
    }

    public void Dispose()
    {
        if (_handle == IntPtr.Zero)
            return;
        CloseHandle(_handle);
        _handle = IntPtr.Zero;
    }
}

public sealed class PartisanWorkbenchSuspendedProcess : IDisposable
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

    public PartisanWorkbenchSuspendedProcess(
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
                "Unable to create suspended diagnostic Workbench process.");
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
            throw new InvalidOperationException("Suspended Workbench thread is unavailable.");
        if (ResumeThread(_threadHandle) == UInt32.MaxValue)
            throw new Win32Exception(
                Marshal.GetLastWin32Error(),
                "Unable to resume diagnostic Workbench process.");
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

public static class PartisanWorkbenchNativeCommandLine
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
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]
        [ValidateSet("Leaf", "Container")][string]$Kind
    )

    if (-not (Test-Path -LiteralPath $Path -PathType $Kind)) {
        throw "A required $Kind path is unavailable."
    }
    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "A required path boundary must not be a reparse point."
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
    if ($AllowEqual -and $candidateFull.Equals($rootFull, [StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    $prefix = $rootFull + [IO.Path]::DirectorySeparatorChar
    return $candidateFull.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)
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

    $tokens = @([PartisanWorkbenchNativeCommandLine]::Split($CommandLine))
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
        "CrashReporter",
        "CrashReportClient"
    )
    return @(Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $names -contains $_.ProcessName })
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
        # An existing process whose identity cannot be inspected is not safe to
        # classify as dead for recursive cleanup.
        return $true
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
        $guardBaseFull = [IO.Path]::GetFullPath($GuardBase).TrimEnd('\', '/')
        $resolvedParent = [IO.Path]::GetFullPath(
            (Split-Path -Parent $resolved)).TrimEnd('\', '/')
        if (-not $resolvedParent.Equals(
            $guardBaseFull,
            [StringComparison]::OrdinalIgnoreCase)) {
            return $null
        }
        $leaf = Split-Path -Leaf $resolved
        if ($leaf -notmatch '^PartisanWorkbenchGuard_([0-9a-f]{32})$') {
            return $null
        }
        if (-not (Test-Path -LiteralPath $resolved -PathType Container)) {
            return $null
        }
        $directoryItem = Get-Item -LiteralPath $resolved -Force
        if (($directoryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }

        $sentinel = Join-Path $resolved ".partisan-workbench-owner"
        if (-not (Test-Path -LiteralPath $sentinel -PathType Leaf)) {
            return $null
        }
        $sentinelItem = Get-Item -LiteralPath $sentinel -Force
        if (($sentinelItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }
        $stream = $null
        $reader = $null
        try {
            $stream = [IO.File]::Open(
                $sentinel,
                [IO.FileMode]::Open,
                [IO.FileAccess]::Read,
                ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
            $reader = New-Object IO.StreamReader($stream)
            $record = $reader.ReadToEnd() | ConvertFrom-Json
        }
        finally {
            if ($reader) { $reader.Dispose() }
            elseif ($stream) { $stream.Dispose() }
        }

        $nonce = [string]$matches[1]
        if ([int]$record.version -ne 1 -or
            [string]$record.nonce -cne $nonce -or
            [string]$record.guardLeaf -cne $leaf -or
            [int]$record.ownerPid -le 0 -or
            [string]::IsNullOrWhiteSpace([string]$record.ownerStartUtc) -or
            [string]::IsNullOrWhiteSpace([string]$record.createdUtc)) {
            return $null
        }
        $ownerStartUtc = [DateTime]::Parse(
            [string]$record.ownerStartUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        $createdUtc = [DateTime]::Parse(
            [string]$record.createdUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        return [pscustomobject]@{
            Directory = $resolved
            Sentinel = $sentinel
            Nonce = $nonce
            GuardLeaf = $leaf
            OwnerPid = [int]$record.ownerPid
            OwnerStartUtc = $ownerStartUtc
            CreatedUtc = $createdUtc
        }
    }
    catch {
        return $null
    }
}

function Remove-ExactOwnedGuard {
    param(
        [Parameter(Mandatory = $true)]$Ownership,
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [int]$Attempts = 8
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
        [timespan]$MinimumAge = [timespan]::Zero
    )

    if (-not (Test-Path -LiteralPath $GuardBase -PathType Container)) {
        return 0
    }
    $baseItem = Get-Item -LiteralPath $GuardBase -Force
    if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Workbench guard base must not be a reparse point."
    }
    $removed = 0
    foreach ($candidate in @(Get-ChildItem -LiteralPath $GuardBase -Directory -Force -ErrorAction Stop)) {
        $ownership = Read-GuardOwnership -Directory $candidate.FullName -GuardBase $GuardBase
        if (-not $ownership -or ([DateTime]::UtcNow - $ownership.CreatedUtc) -lt $MinimumAge) {
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
    if ((Split-Path -Leaf $guardBaseFull) -cne 'PartisanWorkbenchValidation') {
        throw "Workbench guard base identity is not exact."
    }
    $baseItem = Get-Item -LiteralPath $guardBaseFull -Force -ErrorAction Stop
    if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Workbench guard base must not be a reparse point."
    }
    $pattern = '^PartisanWorkbenchGuard_[0-9a-f]{32}$'
    $removed = 0
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
            if ($item.PSIsContainer -and
                ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -eq 0) {
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
        if ($resolvedExclusion.Equals($resolved, [StringComparison]::OrdinalIgnoreCase)) {
            throw "A monitoring root must not be fully excluded."
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

function Update-OwnedProcesses {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Owned,
        [Parameter(Mandatory = $true)][int]$RootProcessId,
        [Parameter(Mandatory = $true)][datetime]$RootStartUtc,
        $Job
    )

    if ($RootProcessId -le 0) {
        return
    }
    $rows = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $jobIds = @()
    if ($Job) {
        try { $jobIds = @($Job.GetProcessIds()) }
        catch { $jobIds = @() }
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
}

function Stop-ExactOwnedProcesses {
    param([Parameter(Mandatory = $true)][hashtable]$Owned)

    $rows = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $depths = @{}
    foreach ($ownedId in @($Owned.Keys)) {
        $depth = 0
        $cursor = [int]$ownedId
        while ($depth -lt 32) {
            $row = $rows | Where-Object { [int]$_.ProcessId -eq $cursor } | Select-Object -First 1
            if (-not $row -or -not $Owned.ContainsKey([int]$row.ParentProcessId)) {
                break
            }
            $cursor = [int]$row.ParentProcessId
            $depth++
        }
        $depths[[int]$ownedId] = $depth
    }
    foreach ($ownedId in @($Owned.Keys | Sort-Object { $depths[[int]$_] } -Descending)) {
        $expectedStart = $Owned[[int]$ownedId]
        try {
            $candidate = Get-Process -Id ([int]$ownedId) -ErrorAction Stop
            if ($candidate.StartTime.ToUniversalTime().Ticks -eq
                $expectedStart.ToUniversalTime().Ticks) {
                $candidate.Kill()
            }
        }
        catch { }
    }
    for ($attempt = 0; $attempt -lt 20; $attempt++) {
        $remaining = @($Owned.Keys | Where-Object {
            Test-ProcessIdentityAlive -ProcessId ([int]$_) -StartUtc $Owned[[int]$_]
        })
        if ($remaining.Count -eq 0) {
            return
        }
        Start-Sleep -Milliseconds 100
    }
}

function Get-ExactOwnedProcessCount {
    param([Parameter(Mandatory = $true)][hashtable]$Owned)

    return @($Owned.Keys | Where-Object {
        Test-ProcessIdentityAlive -ProcessId ([int]$_) -StartUtc $Owned[[int]$_]
    }).Count
}

function Complete-WorkbenchProcessQuiescence {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Owned,
        [Parameter(Mandatory = $true)][string]$ExpectedExecutable,
        [Parameter(Mandatory = $true)][string[]]$ExpectedArguments,
        [Parameter(Mandatory = $true)][datetime]$EarliestStartUtc,
        [Parameter(Mandatory = $true)]$UnclaimedProcessesObserved,
        [ValidateRange(1000, 30000)][int]$ObservationMilliseconds = 5000,
        [ValidateRange(50, 1000)][int]$PollMilliseconds = 250
    )

    $claimed = 0
    $deadline = [DateTime]::UtcNow.AddMilliseconds($ObservationMilliseconds)
    do {
        $claimedThisPoll = 0
        foreach ($candidate in @(Get-EngineProcessRows)) {
            $candidateId = [int]$candidate.Id
            if ($Owned.ContainsKey($candidateId)) {
                continue
            }

            $candidateStartUtc = [DateTime]::MinValue
            $candidatePath = ""
            try {
                $candidateStartUtc = $candidate.StartTime.ToUniversalTime()
                $candidatePath = [IO.Path]::GetFullPath($candidate.Path)
            }
            catch {
                # A just-created process can briefly lack inspectable metadata.
                # Leave it unclassified until a later poll or the final audit.
                continue
            }

            $identity = "$($candidate.ProcessName):$($candidateStartUtc.Ticks)"
            if ($candidateStartUtc -lt $EarliestStartUtc -or
                -not $candidatePath.Equals(
                    [IO.Path]::GetFullPath($ExpectedExecutable),
                    [StringComparison]::OrdinalIgnoreCase)) {
                [void]$UnclaimedProcessesObserved.Add($identity)
                continue
            }

            $processRow = Get-CimInstance `
                Win32_Process `
                -Filter "ProcessId=$candidateId" `
                -ErrorAction SilentlyContinue
            if (-not $processRow -or
                [string]::IsNullOrWhiteSpace([string]$processRow.CommandLine)) {
                continue
            }
            if (-not (Test-ExactNativeArgumentVector `
                    -CommandLine ([string]$processRow.CommandLine) `
                    -ExpectedExecutable $ExpectedExecutable `
                    -ExpectedArguments $ExpectedArguments)) {
                [void]$UnclaimedProcessesObserved.Add($identity)
                continue
            }

            $Owned[$candidateId] = $candidateStartUtc
            $claimed++
            $claimedThisPoll++
        }

        if ($claimedThisPoll -gt 0) {
            Stop-ExactOwnedProcesses -Owned $Owned
            $settleDeadline = [DateTime]::UtcNow.AddMilliseconds(1000)
            if ($settleDeadline -gt $deadline) {
                $deadline = $settleDeadline
            }
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    while ([DateTime]::UtcNow -lt $deadline)

    Stop-ExactOwnedProcesses -Owned $Owned
    return $claimed
}

function Get-SafeGuardLogFiles {
    param([Parameter(Mandatory = $true)][string]$GuardRoot)

    $extensions = @(".log", ".txt", ".rpt")
    $files = New-Object Collections.Generic.List[object]
    $pending = New-Object Collections.Generic.Queue[string]
    $pending.Enqueue([IO.Path]::GetFullPath($GuardRoot))
    while ($pending.Count -gt 0) {
        $directory = $pending.Dequeue()
        foreach ($item in @(Get-ChildItem -LiteralPath $directory -Force -ErrorAction SilentlyContinue)) {
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                continue
            }
            if (-not (Test-ContainedPath -Root $GuardRoot -Candidate $item.FullName)) {
                continue
            }
            if ($item.PSIsContainer) {
                $pending.Enqueue($item.FullName)
                continue
            }
            if ($extensions -contains $item.Extension.ToLowerInvariant()) {
                $files.Add($item)
            }
        }
    }
    return @($files.ToArray() | Sort-Object LastWriteTimeUtc -Descending)
}

function Read-BoundedSharedText {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][long]$MaximumBytes
    )

    $stream = $null
    try {
        $stream = [IO.File]::Open(
            $Path,
            [IO.FileMode]::Open,
            [IO.FileAccess]::Read,
            ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
        $length = $stream.Length
        $accepted = [int][Math]::Min($length, $MaximumBytes)
        if ($accepted -le 0) {
            return ""
        }
        if ($length -le $MaximumBytes) {
            $bytes = New-Object byte[] $accepted
            $read = $stream.Read($bytes, 0, $accepted)
            return [Text.Encoding]::UTF8.GetString($bytes, 0, $read)
        }

        $half = [int][Math]::Floor($MaximumBytes / 2)
        $head = New-Object byte[] $half
        $headRead = $stream.Read($head, 0, $half)
        [void]$stream.Seek(-$half, [IO.SeekOrigin]::End)
        $tail = New-Object byte[] $half
        $tailRead = $stream.Read($tail, 0, $half)
        return [Text.Encoding]::UTF8.GetString($head, 0, $headRead) +
            "`r`n<bounded-log-gap>`r`n" +
            [Text.Encoding]::UTF8.GetString($tail, 0, $tailRead)
    }
    finally {
        if ($stream) { $stream.Dispose() }
    }
}

function Read-GuardLogCorpus {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][long]$MaximumBytes
    )

    $builder = New-Object Text.StringBuilder
    $remaining = $MaximumBytes
    foreach ($file in @(Get-SafeGuardLogFiles -GuardRoot $GuardRoot)) {
        if ($remaining -le 0) { break }
        $budget = [Math]::Min([long]8388608, $remaining)
        $text = Read-BoundedSharedText -Path $file.FullName -MaximumBytes $budget
        [void]$builder.AppendLine($text)
        $remaining -= [Math]::Min([long]$file.Length, $budget)
    }
    return $builder.ToString()
}

function ConvertTo-ComparablePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    try {
        return [IO.Path]::GetFullPath($Path.Replace('/', '\')).TrimEnd('\', '/')
    }
    catch {
        return ""
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
    $safe = [regex]::Replace($safe, '(?i)(["''])[A-Z]:[\\/][^"'']+\1', '$1<path>$1')
    $safe = [regex]::Replace($safe, '(?i)\b[A-Z]:[\\/][^\s;,)]+', '<path>')
    $safe = [regex]::Replace($safe, '(?i)\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b', '<email>')
    $safe = [regex]::Replace($safe, '\b[0-9]{15,20}\b', '<id>')
    if ($safe.Length -gt 600) {
        $safe = $safe.Substring(0, 600)
    }
    return $safe
}

function Get-FirstMatchingLine {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][scriptblock]$Predicate
    )

    foreach ($line in ($Text -split "`r?`n")) {
        if (& $Predicate $line) {
            return $line
        }
    }
    return $null
}

function Read-WorkbenchValidationEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$ExpectedProjectPath,
        [Parameter(Mandatory = $true)][string]$ExpectedProjectId,
        [Parameter(Mandatory = $true)][string]$ExpectedProjectGuid,
        [string]$GuardRoot = "",
        [string[]]$ResolvedAddonRoots = @()
    )

    $expectedPath = ConvertTo-ComparablePath -Path $ExpectedProjectPath
    $expectedDirectory = ConvertTo-ComparablePath -Path (Split-Path -Parent $ExpectedProjectPath)
    $pathMatched = $false
    $guidMatched = $false
    $pathGuidMatched = $false
    $gprojPattern = "(?im)ENGINE\s*:\s*gproj:\s*'(?<path>[^'\r\n]+)'\s+guid:\s*'(?<guid>[0-9A-Fa-f]+)'"
    foreach ($match in [regex]::Matches($Text, $gprojPattern)) {
        $samePath = (ConvertTo-ComparablePath -Path $match.Groups["path"].Value).Equals(
            $expectedPath,
            [StringComparison]::OrdinalIgnoreCase)
        $sameGuid = $match.Groups["guid"].Value.Equals(
            $ExpectedProjectGuid,
            [StringComparison]::OrdinalIgnoreCase)
        if ($samePath) { $pathMatched = $true }
        if ($sameGuid) { $guidMatched = $true }
        if ($samePath -and $sameGuid) { $pathGuidMatched = $true }
    }

    $projectIdMatched = $false
    $mountPattern = "(?im)ENGINE\s*:\s*FileSystem:\s*Adding relative directory\s+'(?<root>[^'\r\n]+)'\s+to filesystem under name\s+(?<id>[^\s\r\n]+)"
    foreach ($match in [regex]::Matches($Text, $mountPattern)) {
        $mountRoot = ConvertTo-ComparablePath -Path $match.Groups["root"].Value
        $mountId = $match.Groups["id"].Value.Trim("'", '"')
        if ($mountRoot.Equals($expectedDirectory, [StringComparison]::OrdinalIgnoreCase) -and
            $mountId.Equals($ExpectedProjectId, [StringComparison]::Ordinal)) {
            $projectIdMatched = $true
            break
        }
    }

    $moduleGame = $false
    $files = 0
    $classes = 0
    $crc = ""
    $statsPattern = "(?im)SCRIPT\s*:\s*Module:\s*Game;\s*loaded\s+(?<files>[0-9]+)[xX]?\s+files;\s*(?<classes>[0-9]+)[xX]?\s+classes;[^\r\n]*?CRC32:\s*(?<crc>[0-9A-Fa-f]{8})"
    $statsMatches = [regex]::Matches($Text, $statsPattern)
    if ($statsMatches.Count -gt 0) {
        $stats = $statsMatches[$statsMatches.Count - 1]
        $moduleGame = $true
        $files = [int]$stats.Groups["files"].Value
        $classes = [int]$stats.Groups["classes"].Value
        $crc = $stats.Groups["crc"].Value.ToLowerInvariant()
    }
    elseif ($Text -match '(?im)SCRIPT\s*:\s*Module:\s*Game(?:;|\b)') {
        $moduleGame = $true
    }

    $gameCreated = [regex]::IsMatch($Text, '(?im)\bGame created\b')
    $scriptValidation = [regex]::IsMatch($Text, '(?im)\bScript validation successful\b')
    $firstHstError = Get-FirstMatchingLine -Text $Text -Predicate {
        param($line)
        $line -match '(?i)\bHST(?:_|\b)' -and
            $line -match '(?i)(?:\bERROR\b|\(E\)|\bfatal\b|\bfailed\b|\bexception\b)'
    }
    $firstScriptError = Get-FirstMatchingLine -Text $Text -Predicate {
        param($line)
        $line -match '(?i)(?:SCRIPT\s*\(E\)|SCRIPT[^\r\n]*\b(?:error|failed|failure)\b|\bscript compiler error\b)'
    }
    $firstHardError = Get-FirstMatchingLine -Text $Text -Predicate {
        param($line)
        $line -match '(?i)(?:ACCESS_VIOLATION|0xC000[0-9A-F]+|heap corruption|unhandled exception|fatal error|application crash|engine initialization failed)'
    }
    $diagnosticTail = New-Object Collections.Generic.List[string]
    foreach ($line in ($Text -split "`r?`n")) {
        if ($line -notmatch '(?i)\b(?:SCRIPT|ENGINE)\b') {
            continue
        }
        $safeLine = ConvertTo-SafeEvidenceLine `
            -Line $line `
            -GuardRoot $GuardRoot `
            -ProjectDirectory $expectedDirectory `
            -ResolvedAddonRoots $ResolvedAddonRoots
        if ([string]::IsNullOrWhiteSpace($safeLine)) {
            continue
        }
        [void]$diagnosticTail.Add($safeLine)
        if ($diagnosticTail.Count -gt 12) {
            $diagnosticTail.RemoveAt(0)
        }
    }

    return [pscustomobject]@{
        ProjectPath = $pathMatched
        ProjectGuid = $guidMatched
        ProjectPathGuidPair = $pathGuidMatched
        ProjectId = $projectIdMatched
        ModuleGame = $moduleGame
        Files = $files
        Classes = $classes
        Crc = $crc
        GameCreated = $gameCreated
        ScriptValidation = $scriptValidation
        FirstHstError = ConvertTo-SafeEvidenceLine `
            -Line $firstHstError `
            -GuardRoot $GuardRoot `
            -ProjectDirectory $expectedDirectory `
            -ResolvedAddonRoots $ResolvedAddonRoots
        FirstScriptError = ConvertTo-SafeEvidenceLine `
            -Line $firstScriptError `
            -GuardRoot $GuardRoot `
            -ProjectDirectory $expectedDirectory `
            -ResolvedAddonRoots $ResolvedAddonRoots
        FirstHardError = ConvertTo-SafeEvidenceLine `
            -Line $firstHardError `
            -GuardRoot $GuardRoot `
            -ProjectDirectory $expectedDirectory `
            -ResolvedAddonRoots $ResolvedAddonRoots
        DiagnosticTail = @($diagnosticTail.ToArray())
    }
}

function Invoke-ParserSelfTest {
    param(
        [Parameter(Mandatory = $true)][string]$GuardRoot,
        [Parameter(Mandatory = $true)][string]$ExpectedProjectPath,
        [Parameter(Mandatory = $true)][string]$ExpectedProjectId,
        [Parameter(Mandatory = $true)][string]$ExpectedProjectGuid,
        [Parameter(Mandatory = $true)][string[]]$ResolvedAddonRoots
    )

    $projectLogPath = $ExpectedProjectPath.Replace('\', '/')
    $projectRoot = (Split-Path -Parent $ExpectedProjectPath).Replace('\', '/')
    $validText = @"
ENGINE : gproj: '$projectLogPath' guid: '$ExpectedProjectGuid'
ENGINE : FileSystem: Adding relative directory '$projectRoot' to filesystem under name $ExpectedProjectId
SCRIPT : Module: Game; loaded 5830x files; 11822x classes; used 47077K of static memory; CRC32: dc565606
SCRIPT : Script validation successful
"@
    $validPath = Join-Path $GuardRoot "selftest-valid.log"
    [IO.File]::WriteAllText($validPath, $validText, (New-Object Text.UTF8Encoding($false)))
    $valid = Read-WorkbenchValidationEvidence `
        -Text (Read-GuardLogCorpus -GuardRoot $GuardRoot -MaximumBytes 1048576) `
        -ExpectedProjectPath $ExpectedProjectPath `
        -ExpectedProjectId $ExpectedProjectId `
        -ExpectedProjectGuid $ExpectedProjectGuid `
        -GuardRoot $GuardRoot `
        -ResolvedAddonRoots $ResolvedAddonRoots
    if (-not $valid.ProjectPathGuidPair -or -not $valid.ProjectId -or
        -not $valid.ModuleGame -or $valid.Files -ne 5830 -or
        $valid.Classes -ne 11822 -or $valid.Crc -cne "dc565606" -or
        $valid.GameCreated -or -not $valid.ScriptValidation -or
        $valid.FirstHstError -or $valid.FirstScriptError -or $valid.FirstHardError) {
        throw "Synthetic valid Workbench evidence did not satisfy the exact parser contract."
    }

    $invalidDirectory = Join-Path $GuardRoot "negative"
    New-Item -ItemType Directory -Path $invalidDirectory -Force | Out-Null
    Move-Item -LiteralPath $validPath -Destination (Join-Path $invalidDirectory "ignored.bin")
    $wrongPath = Join-Path $GuardRoot "wrong\addon.gproj"
    $invalidText = @"
ENGINE : gproj: '$wrongPath' guid: '0000000000000000'
ENGINE : FileSystem: Adding relative directory '$GuardRoot' to filesystem under name wrong_project
SCRIPT (E) : HST synthetic compiler error
ENGINE : fatal error synthetic self-test
"@
    [IO.File]::WriteAllText(
        (Join-Path $GuardRoot "selftest-invalid.log"),
        $invalidText,
        (New-Object Text.UTF8Encoding($false)))
    $invalid = Read-WorkbenchValidationEvidence `
        -Text (Read-GuardLogCorpus -GuardRoot $GuardRoot -MaximumBytes 1048576) `
        -ExpectedProjectPath $ExpectedProjectPath `
        -ExpectedProjectId $ExpectedProjectId `
        -ExpectedProjectGuid $ExpectedProjectGuid `
        -GuardRoot $GuardRoot `
        -ResolvedAddonRoots $ResolvedAddonRoots
    if ($invalid.ProjectPath -or $invalid.ProjectGuid -or $invalid.ProjectId -or
        $invalid.ModuleGame -or $invalid.GameCreated -or $invalid.ScriptValidation -or
        -not $invalid.FirstHstError -or -not $invalid.FirstScriptError -or
        -not $invalid.FirstHardError) {
        throw "Synthetic invalid Workbench evidence did not fail closed."
    }
    return $valid
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

$executablePath = Resolve-ExistingPath -Path $Executable -Kind Leaf
$projectFile = Resolve-ExistingPath -Path $ProjectPath -Kind Leaf
$projectDirectory = Split-Path -Parent $projectFile
$supportedExecutables = @(
    "ArmaReforgerWorkbenchDiag.exe",
    "ArmaReforgerWorkbenchSteamDiag.exe"
)
if ($supportedExecutables -notcontains (Split-Path -Leaf $executablePath)) {
    throw "Executable must be a supported diagnostic Workbench executable."
}
if ([IO.Path]::GetExtension($projectFile) -cne ".gproj") {
    throw "Project path must identify a .gproj file."
}

$projectText = Get-Content -LiteralPath $projectFile -Raw
$projectIdMatch = [regex]::Match($projectText, '(?m)^\s*ID\s+"(?<id>[A-Za-z0-9_.-]+)"')
$projectGuidMatch = [regex]::Match($projectText, '(?m)^\s*GUID\s+"(?<guid>[0-9A-Fa-f]{16})"')
if (-not $projectIdMatch.Success -or -not $projectGuidMatch.Success) {
    throw "Project ID or GUID is missing from the requested project."
}
$expectedProjectId = $projectIdMatch.Groups["id"].Value
$expectedProjectGuid = $projectGuidMatch.Groups["guid"].Value.ToUpperInvariant()

$resolvedAddonRoots = New-Object Collections.Generic.List[string]
foreach ($addonRoot in $AddonRoots) {
    if ([string]::IsNullOrWhiteSpace([string]$addonRoot)) {
        continue
    }
    $resolvedRoot = Resolve-ExistingPath -Path $addonRoot -Kind Container
    if (Test-PathOverlap -First $resolvedRoot -Second $projectDirectory) {
        throw "Explicit addon roots must not include or contain the user project tree."
    }
    if (-not $resolvedAddonRoots.Contains($resolvedRoot)) {
        $resolvedAddonRoots.Add($resolvedRoot)
    }
}
if ($resolvedAddonRoots.Count -lt 1) {
    throw "At least one explicit packed dependency addon root is required."
}

$normalizedDefaultLogRoots = @($DefaultLogRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$normalizedSpillRoots = @($SpillRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$resolvedEvidenceDirectory = ""
if (-not [string]::IsNullOrWhiteSpace($EvidenceDirectory)) {
    $resolvedEvidenceDirectory = [IO.Path]::GetFullPath($EvidenceDirectory)
    if (-not (Test-Path -LiteralPath $resolvedEvidenceDirectory -PathType Container)) {
        throw "The Workbench evidence directory must be an existing fresh directory."
    }
    Assert-NoReparsePathAncestry -Path $resolvedEvidenceDirectory
    if (@(Get-ChildItem -LiteralPath $resolvedEvidenceDirectory -Force -ErrorAction Stop).Count -ne 0) {
        throw "The Workbench evidence directory must be empty before validation."
    }
}
if (-not $PreflightOnly -and -not $SelfTest -and
    ($normalizedDefaultLogRoots.Count -eq 0 -or $normalizedSpillRoots.Count -eq 0)) {
    throw "A real Workbench validation requires explicit default-log and spill monitoring roots."
}

$nonce = [Guid]::NewGuid().ToString("N")
$guardBase = [IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetTempPath()) "PartisanWorkbenchValidation"))
$guardLeaf = "PartisanWorkbenchGuard_$nonce"
$guardRoot = [IO.Path]::GetFullPath((Join-Path $guardBase $guardLeaf))
Assert-NoReparsePathAncestry -Path $guardBase
if ($resolvedEvidenceDirectory -and
    (Test-ContainedPath -Root $guardBase -Candidate $resolvedEvidenceDirectory -AllowEqual)) {
    throw "The retained evidence directory must be outside the disposable Workbench guard."
}
if (-not (Test-ContainedPath -Root $guardBase -Candidate $guardRoot)) {
    throw "Workbench guard containment failed."
}
$sentinelPath = Join-Path $guardRoot ".partisan-workbench-owner"

$mutex = $null
$mutexAcquired = $false
$guardOwnership = $null
$wrapperStartUtc = [DateTime]::MinValue
$process = $null
$job = $null
$suspendedLauncher = $null
$rootProcessId = 0
$rootStartUtc = [DateTime]::MinValue
$ownedProcesses = @{}
$unclaimedProcessesObserved = New-Object Collections.Generic.HashSet[string]
$defaultSnapshots = New-Object Collections.Generic.List[object]
$spillSnapshots = New-Object Collections.Generic.List[object]
$cleanupErrors = New-Object Collections.Generic.List[string]
$cleanupResult = $null
$runError = $null
$evidence = $null
$exitCode = -1
$stopwatch = $null

try {
    $mutex = New-Object Threading.Mutex($false, "Local\PartisanWorkbenchValidationGuard")
    try {
        $mutexAcquired = $mutex.WaitOne(0)
    }
    catch [Threading.AbandonedMutexException] {
        $mutexAcquired = $true
    }
    if (-not $mutexAcquired) {
        throw "Another guarded Workbench validation is already active."
    }

    $engineProcessesBefore = @(Get-EngineProcessRows)
    if ($engineProcessesBefore.Count -ne 0) {
        throw "Refusing guarded Workbench validation while an engine, Workbench, server, or crash-report process is already running."
    }
    foreach ($root in $normalizedDefaultLogRoots) {
        $defaultSnapshots.Add((New-RootSnapshot -Root $root))
    }
    $spillExclusions = New-Object Collections.Generic.List[string]
    $spillExclusions.Add($projectDirectory)
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

    [void](Remove-StaleEmptyGuardDirectories -GuardBase $guardBase)
    [void](Remove-StaleOwnedGuards -GuardBase $guardBase)
    if (Test-Path -LiteralPath $guardRoot) {
        throw "The nonce-owned Workbench guard directory was not fresh."
    }
    New-Item -ItemType Directory -Path $guardRoot | Out-Null
    Assert-NoReparsePathAncestry -Path $guardRoot
    $guardItem = Get-Item -LiteralPath $guardRoot -Force
    if (($guardItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Workbench guard root must not be a reparse point."
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
    $guardOwnership = Read-GuardOwnership -Directory $guardRoot -GuardBase $guardBase
    if (-not $guardOwnership -or $guardOwnership.Nonce -cne $nonce) {
        throw "Workbench guard ownership validation failed."
    }

    $workingDirectory = Join-Path $guardRoot "working"
    $guardedTempDirectory = Join-Path $guardRoot "temp"
    New-Item -ItemType Directory -Path $workingDirectory -Force | Out-Null
    New-Item -ItemType Directory -Path $guardedTempDirectory -Force | Out-Null

    $addonRootArgument = $resolvedAddonRoots.ToArray() -join ","
    $arguments = @(
        "-addonsDir", $addonRootArgument,
        "-gproj", $projectFile,
        "-wbModule=ScriptEditor",
        "-run",
        "-validate", $Target,
        "-wbsilent",
        "-exitAfterInit",
        "-profile", $guardRoot
    )
    $commandLine = ($arguments | ForEach-Object {
        ConvertTo-NativeArgument -Value ([string]$_)
    }) -join " "
    $roundTripLine = (ConvertTo-NativeArgument -Value $executablePath) + " " + $commandLine
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $roundTripLine `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $arguments)) {
        throw "Workbench native argument construction did not round-trip exactly."
    }

    if ($SelfTest) {
        $evidence = Invoke-ParserSelfTest `
            -GuardRoot $guardRoot `
            -ExpectedProjectPath $projectFile `
            -ExpectedProjectId $expectedProjectId `
            -ExpectedProjectGuid $expectedProjectGuid `
            -ResolvedAddonRoots $resolvedAddonRoots.ToArray()
        Write-Output ("SELFTEST " + ([pscustomobject]@{
            Target = $Target
            ProjectPathGuidPair = $evidence.ProjectPathGuidPair
            ProjectId = $evidence.ProjectId
            ModuleGame = $evidence.ModuleGame
            Files = $evidence.Files
            Classes = $evidence.Classes
            Crc = $evidence.Crc
            GameCreated = $evidence.GameCreated
            ScriptValidation = $evidence.ScriptValidation
            HardErrors = [int]([bool]$evidence.FirstHstError) + [int]([bool]$evidence.FirstScriptError) + [int]([bool]$evidence.FirstHardError)
        } | ConvertTo-Json -Compress))
        throw (New-Object OperationCanceledException("guarded-preflight-complete"))
    }
    if ($PreflightOnly) {
        Write-Output ("PREFLIGHT " + ([pscustomobject]@{
            EngineProcessesBefore = $engineProcessesBefore.Count
            GuardCreated = $true
            ProjectIdentityParsed = $true
            AddonRootCount = $resolvedAddonRoots.Count
            ArgumentTokenCount = $arguments.Count
            KillOnWrapperClose = $true
        } | ConvertTo-Json -Compress))
        throw (New-Object OperationCanceledException("guarded-preflight-complete"))
    }

    $job = New-Object PartisanWorkbenchGuardJob
    if (@(Get-EngineProcessRows).Count -ne 0) {
        throw "An engine process appeared during Workbench preflight; launch was cancelled."
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
        $suspendedLauncher = New-Object PartisanWorkbenchSuspendedProcess(
            $executablePath,
            $roundTripLine,
            $workingDirectory)
    }
    finally {
        [Environment]::SetEnvironmentVariable(
            "TEMP", $previousTemp, [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable(
            "TMP", $previousTmp, [EnvironmentVariableTarget]::Process)
    }
    $process = $suspendedLauncher.Child
    if (-not $process) {
        throw "Diagnostic Workbench did not start."
    }
    $rootProcessId = $process.Id
    $job.Add($process)
    $rootStartUtc = $process.StartTime.ToUniversalTime()
    $ownedProcesses[$rootProcessId] = $rootStartUtc
    $suspendedLauncher.Resume()
    $suspendedLauncher.Dispose()
    $suspendedLauncher = $null

    Start-Sleep -Milliseconds 500
    if ($process.HasExited) {
        throw "Diagnostic Workbench exited before its exact launch vector could be verified."
    }
    $processRow = Get-CimInstance Win32_Process -Filter "ProcessId=$rootProcessId" -ErrorAction Stop
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine ([string]$processRow.CommandLine) `
        -ExpectedExecutable $executablePath `
        -ExpectedArguments $arguments)) {
        throw "Launched Workbench command-line token verification failed."
    }
    $stopwatch = [Diagnostics.Stopwatch]::StartNew()

    while ($stopwatch.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
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
                [void]$unclaimedProcessesObserved.Add(
                    "$($engineProcess.ProcessName):$($engineProcess.StartTime.ToUniversalTime().Ticks)")
            }
            catch { }
        }
        if ($unclaimedProcessesObserved.Count -gt 0) {
            throw "An unowned engine process appeared; it was left untouched and validation was cancelled."
        }
        $process.Refresh()
        if ($process.HasExited) {
            break
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    $process.Refresh()
    if (-not $process.HasExited) {
        throw "Workbench validation exceeded the guarded timeout."
    }
    $exitCode = $process.ExitCode
    $corpus = Read-GuardLogCorpus -GuardRoot $guardRoot -MaximumBytes $ParseBudgetBytes
    $evidence = Read-WorkbenchValidationEvidence `
        -Text $corpus `
        -ExpectedProjectPath $projectFile `
        -ExpectedProjectId $expectedProjectId `
        -ExpectedProjectGuid $expectedProjectGuid `
        -GuardRoot $guardRoot `
        -ResolvedAddonRoots $resolvedAddonRoots.ToArray()
    $validationPassed = $exitCode -eq 0 -and
        $evidence.ProjectPathGuidPair -and
        $evidence.ProjectId -and
        $evidence.ModuleGame -and
        $evidence.Files -gt 0 -and
        $evidence.Classes -gt 0 -and
        -not [string]::IsNullOrWhiteSpace($evidence.Crc) -and
        $evidence.ScriptValidation -and
        -not $evidence.FirstHstError -and
        -not $evidence.FirstScriptError -and
        -not $evidence.FirstHardError
    Write-Output ("RESULT " + ([pscustomobject]@{
        Target = $Target
        Success = [bool]$validationPassed
        ExitCode = $exitCode
        TimedOut = $false
        ProjectPathGuidPair = $evidence.ProjectPathGuidPair
        ProjectId = $evidence.ProjectId
        ModuleGame = $evidence.ModuleGame
        Files = $evidence.Files
        Classes = $evidence.Classes
        Crc = $evidence.Crc
        GameCreated = $evidence.GameCreated
        ScriptValidation = $evidence.ScriptValidation
        HardErrorCount = [int]([bool]$evidence.FirstHstError) + [int]([bool]$evidence.FirstScriptError) + [int]([bool]$evidence.FirstHardError)
        FirstHstError = $evidence.FirstHstError
        FirstScriptError = $evidence.FirstScriptError
        FirstHardError = $evidence.FirstHardError
        DiagnosticTail = @($evidence.DiagnosticTail)
    } | ConvertTo-Json -Compress))
    if (-not $validationPassed) {
        throw "Workbench completed without satisfying the exact validation contract."
    }
}
catch {
    if ($_.Exception -is [OperationCanceledException] -and
        $_.Exception.Message -eq "guarded-preflight-complete") {
        $runError = $null
    }
    else {
        $runError = $_.Exception.Message
    }
}
finally {
    $cleanupState = [ordered]@{
        GuardRemaining = -1
        GuardBaseRemaining = -1
        OwnedGuardRootsRemaining = -1
        OwnedProcessesRemaining = -1
        NewEngineProcessesRemaining = -1
        LateOwnedProcessesClaimed = 0
        NewDefaultEntriesRemaining = -1
        ModifiedDefaultFiles = -1
        DeletedDefaultEntries = -1
        MissingDefaultRoots = -1
        ExternalSpillEntriesRemaining = -1
        ModifiedSpillFiles = -1
        DeletedSpillEntries = -1
        MissingSpillRoots = -1
    }
    Invoke-IsolatedCleanupPhase -Name "discover-owned-processes" -Errors $cleanupErrors -Action {
        if ($rootProcessId -gt 0) {
            Update-OwnedProcesses `
                -Owned $ownedProcesses `
                -RootProcessId $rootProcessId `
                -RootStartUtc $rootStartUtc `
                -Job $job
        }
    }
    Invoke-IsolatedCleanupPhase -Name "dispose-suspended-launcher" -Errors $cleanupErrors -Action {
        if ($suspendedLauncher) {
            $suspendedLauncher.Dispose()
            $suspendedLauncher = $null
        }
    }
    Invoke-IsolatedCleanupPhase -Name "stop-owned-processes" -Errors $cleanupErrors -Action {
        Stop-ExactOwnedProcesses -Owned $ownedProcesses
    }
    Invoke-IsolatedCleanupPhase -Name "close-process-job" -Errors $cleanupErrors -Action {
        if ($job) { $job.Dispose() }
    }
    Invoke-IsolatedCleanupPhase -Name "dispose-root-process" -Errors $cleanupErrors -Action {
        if ($process) { $process.Dispose() }
    }
    Invoke-IsolatedCleanupPhase -Name "settle-late-owned-processes" -Errors $cleanupErrors -Action {
        if ($rootProcessId -gt 0 -and $rootStartUtc -ne [DateTime]::MinValue) {
            $cleanupState.LateOwnedProcessesClaimed = Complete-WorkbenchProcessQuiescence `
                -Owned $ownedProcesses `
                -ExpectedExecutable $executablePath `
                -ExpectedArguments $arguments `
                -EarliestStartUtc $rootStartUtc `
                -UnclaimedProcessesObserved $unclaimedProcessesObserved
        }
    }
    Invoke-IsolatedCleanupPhase -Name "retain-raw-evidence" -Errors $cleanupErrors -Action {
        if (-not $resolvedEvidenceDirectory -or
            -not (Test-Path -LiteralPath $guardRoot -PathType Container)) {
            return
        }
        Assert-NoReparsePathAncestry -Path $guardRoot
        Assert-NoReparsePathAncestry -Path $resolvedEvidenceDirectory
        $reparseDescendant = Get-ChildItem `
            -LiteralPath $guardRoot `
            -Recurse `
            -Force `
            -ErrorAction Stop | Where-Object {
                ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
            } | Select-Object -First 1
        if ($reparseDescendant) {
            throw "Workbench evidence retention refuses a reparse descendant."
        }
        foreach ($sourceFile in @(Get-ChildItem `
            -LiteralPath $guardRoot `
            -Recurse `
            -File `
            -Force `
            -ErrorAction Stop)) {
            if ($sourceFile.FullName.Equals(
                $sentinelPath,
                [StringComparison]::OrdinalIgnoreCase)) {
                continue
            }
            $relativePath = $sourceFile.FullName.Substring($guardRoot.Length).TrimStart('\', '/')
            if ([string]::IsNullOrWhiteSpace($relativePath)) {
                throw "Workbench evidence retention produced an empty relative path."
            }
            $destinationPath = [IO.Path]::GetFullPath(
                (Join-Path $resolvedEvidenceDirectory $relativePath))
            if (-not (Test-ContainedPath `
                -Root $resolvedEvidenceDirectory `
                -Candidate $destinationPath)) {
                throw "Workbench evidence retention escaped its destination."
            }
            $destinationParent = Split-Path -Parent $destinationPath
            if (-not (Test-Path -LiteralPath $destinationParent -PathType Container)) {
                New-Item -ItemType Directory -Path $destinationParent -Force | Out-Null
            }
            Copy-Item -LiteralPath $sourceFile.FullName -Destination $destinationPath -Force
        }
    }
    Invoke-IsolatedCleanupPhase -Name "remove-owned-guard" -Errors $cleanupErrors -Action {
        Assert-NoReparsePathAncestry -Path $guardBase
        if (-not $guardOwnership -and (Test-Path -LiteralPath $guardRoot -PathType Container)) {
            $candidateOwnership = Read-GuardOwnership -Directory $guardRoot -GuardBase $guardBase
            if ($candidateOwnership -and $candidateOwnership.Nonce -ceq $nonce -and
                $candidateOwnership.OwnerPid -eq $PID -and
                $wrapperStartUtc -ne [DateTime]::MinValue -and
                $candidateOwnership.OwnerStartUtc.Ticks -eq $wrapperStartUtc.Ticks) {
                $guardOwnership = $candidateOwnership
            }
        }
        if ($guardOwnership -and
            -not (Remove-ExactOwnedGuard -Ownership $guardOwnership -GuardBase $guardBase)) {
            throw "Exact Workbench guard removal failed."
        }
    }
    Invoke-IsolatedCleanupPhase -Name "remove-empty-guard-base" -Errors $cleanupErrors -Action {
        Assert-NoReparsePathAncestry -Path $guardBase
        if (Test-Path -LiteralPath $guardBase -PathType Container) {
            $baseItem = Get-Item -LiteralPath $guardBase -Force
            if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Workbench guard base became a reparse point."
            }
            if (@(Get-ChildItem -LiteralPath $guardBase -Force -ErrorAction Stop).Count -eq 0) {
                Remove-Item -LiteralPath $guardBase -Force -ErrorAction Stop
            }
        }
    }
    Invoke-IsolatedCleanupPhase -Name "audit-owned-processes" -Errors $cleanupErrors -Action {
        $cleanupState.OwnedProcessesRemaining = Get-ExactOwnedProcessCount -Owned $ownedProcesses
    }
    Invoke-IsolatedCleanupPhase -Name "audit-engine-processes" -Errors $cleanupErrors -Action {
        $cleanupState.NewEngineProcessesRemaining = @(Get-EngineProcessRows).Count
    }
    Invoke-IsolatedCleanupPhase -Name "audit-external-boundaries" -Errors $cleanupErrors -Action {
        $defaultNew = 0
        $defaultModified = 0
        $defaultDeleted = 0
        $defaultMissing = 0
        foreach ($snapshot in $defaultSnapshots) {
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
    Invoke-IsolatedCleanupPhase -Name "audit-guard-roots" -Errors $cleanupErrors -Action {
        $cleanupState.GuardRemaining = [int](Test-Path -LiteralPath $guardRoot)
        $cleanupState.GuardBaseRemaining = [int](Test-Path -LiteralPath $guardBase)
        $ownedGuardRoots = 0
        if (Test-Path -LiteralPath $guardBase -PathType Container) {
            Assert-NoReparsePathAncestry -Path $guardBase
            $baseItem = Get-Item -LiteralPath $guardBase -Force
            if (($baseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Workbench guard base became a reparse point before audit."
            }
            $ownedGuardRoots = @(
                Get-ChildItem `
                    -LiteralPath $guardBase `
                    -Directory `
                    -Force `
                    -ErrorAction Stop |
                Where-Object {
                    $_.Name -match '^PartisanWorkbenchGuard_[0-9a-f]{32}$'
                }).Count
        }
        $cleanupState.OwnedGuardRootsRemaining = $ownedGuardRoots
    }
    Invoke-IsolatedCleanupPhase -Name "release-wrapper-lock" -Errors $cleanupErrors -Action {
        if ($mutexAcquired -and $mutex) {
            $mutex.ReleaseMutex()
            $mutexAcquired = $false
        }
        if ($mutex) { $mutex.Dispose() }
    }
    $cleanupResult = [pscustomobject]@{
        GuardRemaining = $cleanupState.GuardRemaining
        GuardBaseRemaining = $cleanupState.GuardBaseRemaining
        OwnedGuardRootsRemaining = $cleanupState.OwnedGuardRootsRemaining
        OwnedProcessesRemaining = $cleanupState.OwnedProcessesRemaining
        NewEngineProcessesRemaining = $cleanupState.NewEngineProcessesRemaining
        LateOwnedProcessesClaimed = $cleanupState.LateOwnedProcessesClaimed
        UnclaimedEngineProcessesObserved = $unclaimedProcessesObserved.Count
        NewDefaultEntriesRemaining = $cleanupState.NewDefaultEntriesRemaining
        ModifiedDefaultFiles = $cleanupState.ModifiedDefaultFiles
        DeletedDefaultEntries = $cleanupState.DeletedDefaultEntries
        MissingDefaultRoots = $cleanupState.MissingDefaultRoots
        ExternalSpillEntriesRemaining = $cleanupState.ExternalSpillEntriesRemaining
        ModifiedSpillFiles = $cleanupState.ModifiedSpillFiles
        DeletedSpillEntries = $cleanupState.DeletedSpillEntries
        MissingSpillRoots = $cleanupState.MissingSpillRoots
        CleanupPhaseErrorCount = $cleanupErrors.Count
        CleanupPhaseErrors = $cleanupErrors.ToArray()
        MonitoringRootsAreDetectionOnly = $true
    }
    Write-Output ("CLEANUP " + ($cleanupResult | ConvertTo-Json -Compress))
}

$cleanupPassed = $cleanupResult -and
    $cleanupResult.GuardRemaining -eq 0 -and
    $cleanupResult.GuardBaseRemaining -eq 0 -and
    $cleanupResult.OwnedGuardRootsRemaining -eq 0 -and
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
if ($runError) {
    $safeRunError = ConvertTo-SafeEvidenceLine `
        -Line $runError `
        -GuardRoot $guardRoot `
        -ProjectDirectory $projectDirectory `
        -ResolvedAddonRoots $resolvedAddonRoots.ToArray()
    if ($ReturnToCaller) {
        throw $safeRunError
    }
    Write-Error $safeRunError
    exit 1
}
if (-not $cleanupPassed) {
    if ($ReturnToCaller) {
        throw "Guarded Workbench cleanup did not return every tracked boundary to zero."
    }
    Write-Error "Guarded Workbench cleanup did not return every tracked boundary to zero."
    exit 2
}
if ($ReturnToCaller) {
    return
}
exit 0

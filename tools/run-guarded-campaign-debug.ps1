[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeAddonRoot,

    [Parameter(Mandatory = $true)]
    [string]$SettingsSource,

    [Parameter(Mandatory = $true)]
    [string]$ExpectedBuildSha,

    [Parameter(Mandatory = $true)]
    [string]$ExpectedBuildUtc,

    [Parameter(Mandatory = $true)]
    [string]$ExpectedBuildLabel,

    [ValidateSet("full_certification", "force_authority")]
    [string]$Profile = "full_certification",

    [string]$ProjectPath = "",
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

if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $ProjectPath = Join-Path (Split-Path -Parent $PSScriptRoot) "addon.gproj"
}

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
    $safe = [regex]::Replace($safe, '(?i)(["''])[A-Z]:[\\/][^"'']+\1', '$1<path>$1')
    $safe = [regex]::Replace($safe, '(?i)\b[A-Z]:[\\/][^\s;,)]+', '<path>')
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
        Trigger = ConvertTo-SafeArtifactScalar -Value $Validation.Trigger -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        ArtifactCount = [int]$Validation.ArtifactCount
        StateDiffRows = [int]$Validation.StateDiffRows
        NonzeroStateDiffRows = [int]$Validation.NonzeroStateDiffRows
        Phase17 = $safePhase17
        Phase17Metrics = ConvertTo-SafeArtifactPropertyBag -Bag $Validation.Phase17Metrics -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        Phase24 = $safePhase24
        Phase24Metrics = ConvertTo-SafeArtifactPropertyBag -Bag $Validation.Phase24Metrics -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        StagedCleanup = $safeStagedCleanup
        FocusedCaseId = ConvertTo-SafeArtifactScalar -Value $Validation.FocusedCaseId -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        FocusedCaseStatus = ConvertTo-SafeArtifactScalar -Value $Validation.FocusedCaseStatus -GuardRoot $GuardRoot -ProjectDirectory $ProjectDirectory -ResolvedAddonRoots $ResolvedAddonRoots
        FocusedAssertions = $safeFocusedAssertions
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

function Test-CampaignDebugArtifacts {
    param(
        [Parameter(Mandatory = $true)][string]$JsonPath,
        [Parameter(Mandatory = $true)][string]$SummaryPath,
        [Parameter(Mandatory = $true)][string]$StateDiffPath,
        [Parameter(Mandatory = $true)][string]$ExpectedSha,
        [Parameter(Mandatory = $true)][string]$ExpectedUtc,
        [Parameter(Mandatory = $true)][string]$ExpectedLabel,
        [ValidateSet("full_certification", "force_authority")]
        [string]$ExpectedProfile = "full_certification"
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
    if ([int]$run.m_iEndedAtSecond -le 0) {
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
        $cleanupOrphans = $null
        $cleanupPass = $false
        if ($cleanupCases.Count -eq 1) {
            $cleanupOrphans = Get-MetricValue -Metrics $cleanupCases[0].m_aMetrics -MetricId "cleanup.orphan_active_groups"
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
            $summaryText -notmatch "(?m)^profile force_authority\s*$" -or
            $summaryText.IndexOf($ExpectedSha, [StringComparison]::Ordinal) -lt 0 -or
            $summaryText.IndexOf($ExpectedLabel, [StringComparison]::Ordinal) -lt 0) {
            $problems.Add("summary-identity")
        }

        $deltaMatches = [Regex]::Matches($stateDiffText, '(?m)\|\s+delta\s+(-?\d+)\s*$')
        $nonzeroDeltas = @($deltaMatches | Where-Object { [int]$_.Groups[1].Value -ne 0 }).Count
        if ($stateDiffText -notmatch "(?m)^Partisan campaign debug state diff\s*$" -or
            $stateDiffText -notmatch "(?m)^run $escapedRunId\s*$" -or
            $deltaMatches.Count -ne 18 -or
            $nonzeroDeltas -ne 0 -or
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
            Trigger = $trigger
            ArtifactCount = $artifactNames.Count
            StateDiffRows = $deltaMatches.Count
            NonzeroStateDiffRows = $nonzeroDeltas
            Phase17 = @()
            Phase17Metrics = [pscustomobject][ordered]@{}
            Phase24 = @()
            Phase24Metrics = [pscustomobject][ordered]@{}
            StagedCleanup = @()
            FocusedCaseId = if ($focusedCase) { [string]$focusedCase.m_sCaseId } else { $null }
            FocusedCaseStatus = if ($focusedCase) { [string]$focusedCase.m_sStatus } else { $null }
            FocusedAssertions = $focusedAssertionStatus.ToArray()
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
    $cleanupOrphans = $null
    $cleanupPass = $false
    if ($cleanupCases.Count -eq 1) {
        $cleanupOrphans = Get-MetricValue -Metrics $cleanupCases[0].m_aMetrics -MetricId "cleanup.orphan_active_groups"
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

    $deltaMatches = [Regex]::Matches($stateDiffText, '(?m)\|\s+delta\s+(-?\d+)\s*$')
    $nonzeroDeltas = @($deltaMatches | Where-Object { [int]$_.Groups[1].Value -ne 0 }).Count
    if ($stateDiffText -notmatch "(?m)^Partisan campaign debug state diff\s*$" -or
        $stateDiffText -notmatch "(?m)^run $escapedRunId\s*$" -or
        $deltaMatches.Count -ne 18 -or
        $nonzeroDeltas -ne 0 -or
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
        Trigger = $trigger
        ArtifactCount = $artifactNames.Count
        StateDiffRows = $deltaMatches.Count
        NonzeroStateDiffRows = $nonzeroDeltas
        Phase17 = $phase17Status.ToArray()
        Phase17Metrics = [pscustomobject]$phase17Metrics
        Phase24 = $phase24Status.ToArray()
        Phase24Metrics = [pscustomobject]$phase24Metrics
        StagedCleanup = $stagedStatus.ToArray()
        FocusedCaseId = $null
        FocusedCaseStatus = $null
        FocusedAssertions = @()
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
        param([string]$Id, [string]$Status = "PASS", [string]$Actual = "")
        [pscustomobject]@{
            m_sAssertionId = $Id
            m_sStatus = $Status
            m_sActual = $Actual
            m_bCountsTowardCertification = $false
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
            [object[]]$Metrics = @()
        )
        [pscustomobject]@{
            m_sCaseId = $Id
            m_sStatus = $Status
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
    for ($index = 0; $index -lt 18; $index++) {
        $stateDiffLines.Add("synthetic $index -> $index | delta 0")
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

    $focusedAssertions = @(Get-FocusedForceAuthorityAssertionIds | ForEach-Object {
        & $newAssertion $_
    })
    $focusedCases = @(
        (& $newCase "early_mechanics.force_authority" "PASS" $focusedAssertions),
        (& $newCase "cleanup.run_leak_snapshot" "PASS" @((& $newAssertion "cleanup.orphan_active_groups")) @((& $newMetric "cleanup.orphan_active_groups" "0"))),
        (& $newCase "cleanup.state_isolation_restore" "PASS" @(
            (& $newAssertion "isolation.snapshot"),
            (& $newAssertion "isolation.state_restore"),
            (& $newAssertion "isolation.persistence_restore")
        ))
    )
    $focusedRun = [pscustomobject]@{
        m_sRunId = $runId
        m_sProfile = "force_authority"
        m_sBuildSha = $ExpectedSha
        m_sBuildUtc = $ExpectedUtc
        m_sBuildLabel = $ExpectedLabel
        m_iStartedAtSecond = 1
        m_iEndedAtSecond = 2
        m_iPassCount = @($focusedCases | Where-Object { $_.m_sStatus -eq "PASS" }).Count
        m_iWarnCount = 0
        m_iFailCount = 0
        m_iBlockedCount = 0
        m_iSkippedCount = 0
        m_iCertificationRequiredCount = 0
        m_iCertificationProvenCount = 0
        m_iCertificationFailCount = 0
        m_iCertificationBlockedCount = 0
        m_iCertificationWarnCount = 0
        m_bCertificationPassed = $false
        m_aCases = $focusedCases
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
    }
    $focusedResult = Test-CampaignDebugArtifacts @focusedParameters
    if (-not $focusedResult.Valid -or
        $focusedResult.FullCertification -or
        $focusedResult.ProofScope -ne "focused_force_authority" -or
        $focusedResult.FocusedCaseId -ne "early_mechanics.force_authority" -or
        @($focusedResult.FocusedAssertions).Count -ne @(Get-FocusedForceAuthorityAssertionIds).Count) {
        throw "Synthetic focused force-authority validator self-test failed."
    }

    $focusedNegativeChecks = New-Object Collections.Generic.List[string]
    $failedCaseRun = $focusedJson | ConvertFrom-Json
    $failedCaseRun.m_aCases[0].m_sStatus = "FAIL"
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
    (@($failedAssertionRun.m_aCases[0].m_aAssertions | Where-Object {
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
    $unexpectedAssertionRun.m_aCases[0].m_aAssertions += & $newAssertion "town_influence.unexpected_contract"
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

function Get-GuardErrorCensus {
    param([Parameter(Mandatory = $true)][string]$GuardRoot)

    $scriptErrors = 0
    $partisanErrors = 0
    $crashMarkers = 0
    $logRoot = Join-Path $GuardRoot "logs"
    $logs = @()
    if (Test-Path -LiteralPath $logRoot -PathType Container) {
        $logs = @(Get-ChildItem -LiteralPath $logRoot -Recurse -File -Force -ErrorAction SilentlyContinue |
            Where-Object { $_.Extension -in @(".log", ".rpt") })
    }
    foreach ($log in $logs) {
        try {
            $text = Read-SharedFileText -Path $log.FullName
            $scriptErrors += [Regex]::Matches(
                $text,
                '(?im)^\s*SCRIPT\s+\(E\):').Count
            $partisanErrors += [Regex]::Matches(
                $text,
                '(?im)^.*(?:Partisan|HST).*(?:\s\(E\):|\bERROR\s*:|\bFATAL\s*:).*$').Count
            $crashMarkers += [Regex]::Matches(
                $text,
                '(?im)\b(?:ACCESS_VIOLATION|unhandled exception|fatal error|application crash)\b').Count
        }
        catch {
            continue
        }
    }

    return [pscustomobject]@{
        ScriptErrors = $scriptErrors
        PartisanErrors = $partisanErrors
        CrashMarkers = $crashMarkers
    }
}

$executablePath = Resolve-ExistingPath -Path $Executable -Kind Leaf
$runtimeAddonPath = Resolve-ExistingPath -Path $RuntimeAddonRoot -Kind Container
$settingsPath = Resolve-ExistingPath -Path $SettingsSource -Kind Leaf
$projectFile = Resolve-ExistingPath -Path $ProjectPath -Kind Leaf
foreach ($packedRuntimeMarker in @("core\data.pak", "data\data.pak")) {
    if (-not (Test-Path -LiteralPath (Join-Path $runtimeAddonPath $packedRuntimeMarker) -PathType Leaf)) {
        throw "RuntimeAddonRoot must be the installed packed game add-on root."
    }
}
$supportedDiagnosticExecutables = @(
    "ArmaReforgerSteamDiag.exe",
    "ArmaReforgerServerDiag.exe"
)
if ($supportedDiagnosticExecutables -notcontains (Split-Path -Leaf $executablePath)) {
    throw "Executable must be a supported Reforger diagnostic runtime."
}

try {
    $settings = Read-SharedFileText -Path $settingsPath | ConvertFrom-Json
}
catch {
    throw "Runtime settings file could not be read and parsed."
}
if ([int]$settings.schemaVersion -ne 24) {
    throw "Runtime settings must use Schema 24."
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
if (-not $PreflightOnly -and -not $ArtifactValidatorSelfTest -and
    ($normalizedWatchedRoots.Count -eq 0 -or $normalizedSpillRoots.Count -eq 0)) {
    throw "A real guarded run requires explicit nonempty watched and spill roots."
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

    $profileDirectory = Join-Path $guardRoot "profile\Partisan"
    New-Item -ItemType Directory -Path $profileDirectory -Force | Out-Null
    Copy-Item -LiteralPath $settingsPath -Destination (Join-Path $profileDirectory "HST_Settings.json") -Force
    $guardedWorkingDirectory = Join-Path $guardRoot "working"
    New-Item -ItemType Directory -Path $guardedWorkingDirectory -Force | Out-Null
    $guardedTempDirectory = Join-Path $guardRoot "temp"
    New-Item -ItemType Directory -Path $guardedTempDirectory -Force | Out-Null

    $arguments = @(
        "-addonsDir", $runtimeAddonPath,
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
        "-hstCampaignDebugProfile", $Profile
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
            -ResolvedAddonRoots @($runtimeAddonPath)
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
            EngineProcessesBefore = $existingProcesses.Count
            SettingsSchema = [int]$settings.schemaVersion
            TrustedAdminCount = @($settings.membership.adminIdentityIds).Count
            GuardCreated = $true
            CampaignSaveCopied = $false
            KillOnWrapperClose = $true
            ArgumentTokenCount = $arguments.Count
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
        EngineProcessesBefore = $existingProcesses.Count
        SettingsSchema = [int]$settings.schemaVersion
        TrustedAdminCount = @($settings.membership.adminIdentityIds).Count
        GuardCreated = $true
        CampaignSaveCopied = $false
        KillOnWrapperClose = $true
        ArgumentTokensVerified = $true
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
                -ResolvedAddonRoots @($runtimeAddonPath)
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
                    $artifactValidation = Test-CampaignDebugArtifacts @artifactParameters
                    $safeArtifactValidation = Get-SafeArtifactValidationSummary `
                        -Validation $artifactValidation `
                        -ExpectedBuildSha $ExpectedBuildSha `
                        -ExpectedBuildUtc $ExpectedBuildUtc `
                        -ExpectedBuildLabel $ExpectedBuildLabel `
                        -GuardRoot $guardRoot `
                        -ProjectDirectory (Split-Path -Parent $projectFile) `
                        -ResolvedAddonRoots @($runtimeAddonPath)
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
    $errorCensus = Get-GuardErrorCensus -GuardRoot $guardRoot
    if (-not $artifactValidation.Valid) {
        throw "Campaign Debug artifacts completed but failed the exact validation contract."
    }
    if ($errorCensus.ScriptErrors -ne 0 -or
        $errorCensus.PartisanErrors -ne 0 -or
        $errorCensus.CrashMarkers -ne 0) {
        throw "Campaign Debug runtime completed with forbidden error signals."
    }

    Write-Output ("RESULT " + ([pscustomobject]@{
        RuntimeSeconds = [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
        Armed = $armed
        Started = $started
        ShutdownCensus = $true
        ArtifactsStable = $artifactSignaturePolls -ge 2
        ArtifactBytes = @($artifactPaths | ForEach-Object { (Get-Item -LiteralPath $_).Length })
        Validation = $safeArtifactValidation
        ErrorCensus = $errorCensus
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
            RuntimeSeconds = [Math]::Floor($stopwatch.Elapsed.TotalSeconds)
            Armed = $armed
            Started = $started
            ShutdownCensus = [bool]$errorCensus
            ArtifactsStable = $artifactSignaturePolls -ge 2
            Validation = $safeArtifactValidation
            ErrorCensus = $errorCensus
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

if ($runError) {
    $safeRunError = ConvertTo-SafeEvidenceLine `
        -Line $runError `
        -GuardRoot $guardRoot `
        -ProjectDirectory (Split-Path -Parent $projectFile) `
        -ResolvedAddonRoots @($runtimeAddonPath)
    Write-Error $safeRunError
    exit 1
}
if (-not $cleanupPassed) {
    Write-Error "Guarded Campaign Debug cleanup did not return every tracked boundary to zero."
    exit 2
}
exit 0

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeAddonRoot,

    [string]$ProjectPath = "",
    [string]$WorldResource = "Worlds/HST_Dev/HST_Dev.ent",
    [ValidateSet(
        "outbound_virtual",
        "dematerializing_before_hold",
        "materializing_checkpoint_deferred",
        "physical_live_position",
        "prepared_before_refund",
        "prepared_after_refund",
        "prepared_after_receipt",
        "owner_applied_pending")]
    [string]$CutName = "outbound_virtual",
    [string[]]$WatchedRoots = @(),
    [string[]]$SpillRoots = @(),

    [ValidateRange(30, 3600)]
    [int]$StageTimeoutSeconds = 300,

    [ValidateRange(100, 5000)]
    [int]$PollMilliseconds = 500,

    [ValidateRange(1, 60)]
    [int]$ResultGraceSeconds = 5,

    [switch]$PreflightOnly,

    [switch]$NativeSourceSelection,

    [string]$WorkbenchExecutable = "",

    [ValidateRange(30, 900)]
    [int]$NativePackTimeoutSeconds = 180
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:CutOrdinals = @{
    outbound_virtual = 0
    dematerializing_before_hold = 1
    materializing_checkpoint_deferred = 2
    physical_live_position = 3
    prepared_before_refund = 4
    prepared_after_refund = 5
    prepared_after_receipt = 6
    owner_applied_pending = 7
}
$script:SupportedCutNames = @(
    "outbound_virtual",
    "dematerializing_before_hold",
    "materializing_checkpoint_deferred",
    "physical_live_position",
    "prepared_before_refund",
    "prepared_after_refund",
    "prepared_after_receipt",
    "owner_applied_pending")
$script:CutName = $CutName.ToLowerInvariant()
$script:CutOrdinal = [int]$script:CutOrdinals[$script:CutName]
$script:IsPreparedSettlementCut = $script:CutName -in @(
    "prepared_before_refund",
    "prepared_after_refund",
    "prepared_after_receipt")
$script:IsOwnerAppliedPendingCut =
    $script:CutName -ceq "owner_applied_pending"
$script:NativeSourceSelection = [bool]$NativeSourceSelection
$script:NativeMissionHeader = "Missions/HST_Dev.conf"
$script:NativeScenarioId = "{6985327711302110}Missions/HST_Dev.conf"
$script:NativeProjectId = "698532771130111D"
$script:NativeWorldSystemsConfig =
    "Configs/HST/Persistence/HST_CampaignSystems.conf"
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
$script:WorkspacePackScratchLeaf = ".tmp-native-pack"
$script:WorkspacePackScratchSentinelLeaf = ".partisan-native-pack-owner"
$script:MutexName = "Local\PartisanCounterattackRestartGuard"
$script:SnapshotHashMaximumBytes = 65536
$script:ExpectedContinuationMeters = 75.0

if ($script:NativeSourceSelection -and
    -not $script:IsOwnerAppliedPendingCut) {
    throw "Native source-selection proof is restricted to owner_applied_pending."
}

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
	$safe = [regex]::Replace(
		$safe,
		'(?<![0-9])(?:[0-9]{1,3}\.){3}[0-9]{1,3}(?![0-9])',
		'<ip>')
    if ($safe.Length -gt 500) {
        $safe = $safe.Substring(0, 500)
    }
    return $safe
}

function Get-SafeGuardedEngineDiagnostic {
    param(
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$ProjectDirectory,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath
    )

    $logRoot = Join-Path $ProfileRoot "logs"
    if (-not (Test-Path -LiteralPath $logRoot -PathType Container)) {
        return ""
    }
    $diagnosticLines = New-Object Collections.Generic.List[string]
	$tailLines = New-Object Collections.Generic.List[string]
    foreach ($file in @(Get-ChildItem `
        -LiteralPath $logRoot `
        -Recurse `
        -File `
        -Filter "*.log" `
        -ErrorAction SilentlyContinue | Sort-Object LastWriteTimeUtc)) {
        if ($file.Length -le 0 -or $file.Length -gt 33554432) {
            continue
        }
        foreach ($line in @(Get-Content -LiteralPath $file.FullName -Tail 200)) {
            $safe = ConvertTo-SafeEvidenceLine `
                -Line ([string]$line) `
                -GuardRoot $ProfileRoot `
                -ProjectDirectory $ProjectDirectory `
                -ResolvedAddonRoots @($RuntimeAddonPath)
            if (-not [string]::IsNullOrWhiteSpace($safe)) {
				$tailLines.Add($safe)
				if ([string]$line -match '(?i)(error|fail|invalid|config|scenario|mission|resource)') {
					$diagnosticLines.Add($safe)
				}
            }
        }
    }
	if ($diagnosticLines.Count -eq 0 -and $tailLines.Count -eq 0) {
        return ""
    }
	if ($diagnosticLines.Count -eq 0) {
		return (@($tailLines.ToArray() | Select-Object -Last 8) -join " | ")
	}
	$specificConfigErrors = @($diagnosticLines.ToArray() | Where-Object {
		$_ -match '(?i)(server config|json|property|required|unknown|unsupported|expected)' -and
		$_ -notmatch '(?i)(CLI Params|JSON is invalid|There are errors in server config|Error while initializing game|OnError|OnTimeout)'
	} | Select-Object -Unique)
	$hardErrors = @($diagnosticLines.ToArray() | Where-Object {
		$_ -match '\(E\):' -and
		$_ -notmatch '(?i)(resource leaks|\.edds\s+\d+$)'
	} | Select-Object -Unique)
	$priority = @($specificConfigErrors + $hardErrors)
	$useTail = $false
	if ($priority.Count -eq 0) {
		$priority = @($tailLines.ToArray() | Where-Object {
			$_ -notmatch '(?i)(resource leaks|\.edds\s+\d+$)'
		} | Select-Object -Unique)
		$useTail = $true
	}
	if ($useTail) {
		return (@($priority | Select-Object -Last 8) -join " | ")
	}
    return (@($priority | Select-Object -First 8) -join " | ")
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
            -not ($script:SupportedCutNames -ccontains [string]$ownership.cut) -or
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
            $current.OwnerStartUtc.Ticks -ne $Ownership.OwnerStartUtc.Ticks -or
            $current.Cut -cne $Ownership.Cut) {
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

function Read-NativeWorkspacePackScratchOwnership {
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
        $ownership = Read-JsonArtifact -Path $sentinel
        foreach ($property in @(
            "version",
            "purpose",
            "nonce",
            "ownerPid",
            "ownerStartUtc")) {
            if ($ownership.PSObject.Properties.Name -notcontains $property) {
                return $null
            }
        }
        if ([int]$ownership.version -ne $script:SentinelVersion -or
            [string]$ownership.purpose -cne "native_workbench_pack_scratch" -or
            -not ([string]$ownership.nonce -match '^[0-9a-f]{32}$') -or
            [int]$ownership.ownerPid -le 0) {
            return $null
        }
        $ownerStartUtc = [DateTime]::Parse(
            [string]$ownership.ownerStartUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        return [pscustomobject]@{
            Directory = $resolved
            Sentinel = $sentinel
            Nonce = [string]$ownership.nonce
            OwnerPid = [int]$ownership.ownerPid
            OwnerStartUtc = $ownerStartUtc
        }
    }
    catch {
        return $null
    }
}

function Remove-NativeWorkspacePackScratch {
    param(
        [Parameter(Mandatory = $true)]$Ownership,
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [int]$Attempts = 4
    )

    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        if (-not (Test-Path -LiteralPath $Ownership.Directory)) {
            return $true
        }
        $current = Read-NativeWorkspacePackScratchOwnership `
            -Directory $Ownership.Directory `
            -RepositoryRoot $RepositoryRoot
        if (-not $current -or
            $current.Nonce -cne $Ownership.Nonce -or
            $current.OwnerPid -ne $Ownership.OwnerPid -or
            $current.OwnerStartUtc.Ticks -ne
                $Ownership.OwnerStartUtc.Ticks) {
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

function Assert-NativeSavePointId {
    param(
        [Parameter(Mandatory = $true)][string]$SavePointId,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($SavePointId -cnotmatch
        '^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$') {
        throw "$Label must be one exact lowercase native save UUID."
    }
    $parsed = [Guid]::Empty
    if (-not [Guid]::TryParseExact($SavePointId, "D", [ref]$parsed) -or
        $parsed -eq [Guid]::Empty) {
        throw "$Label is not a valid non-empty native save UUID."
    }
}

function Get-AvailableLoopbackUdpPort {
    $socket = New-Object Net.Sockets.UdpClient(0)
    try {
        $endpoint = [Net.IPEndPoint]$socket.Client.LocalEndPoint
        if (-not $endpoint -or $endpoint.Port -lt 1024 -or
            $endpoint.Port -gt 65535) {
            throw "Unable to reserve a bounded loopback UDP port."
        }
        return [int]$endpoint.Port
    }
    finally {
        $socket.Dispose()
    }
}

function Test-ProofVectorNonZero {
    param([Parameter(Mandatory = $true)]$Value)

    $serialized = if ($Value -is [string]) {
        [string]$Value
    }
    else {
        $Value | ConvertTo-Json -Compress -Depth 4
    }
    $numberPattern = '[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?'
    $matches = [regex]::Matches($serialized, $numberPattern)
    if ($matches.Count -ne 3) {
        return $false
    }
    foreach ($match in $matches) {
        $coordinate = 0.0
        if (-not [double]::TryParse(
            $match.Value,
            [Globalization.NumberStyles]::Float,
            [Globalization.CultureInfo]::InvariantCulture,
            [ref]$coordinate)) {
            return $false
        }
        if ([Math]::Abs($coordinate) -gt 0.0001) {
            return $true
        }
    }
    return $false
}

function Assert-OwnershipStartupScope {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)][string]$Label,
        [bool]$OwnerAppliedPendingCut = $script:IsOwnerAppliedPendingCut
    )

    if ([bool]$Result.m_bSuccess -and -not $OwnerAppliedPendingCut -and
        [bool]$Result.m_bOwnershipStartupReconcileChanged) {
        throw "$Label reported owner-cut startup mutation for a different cut."
    }
}

function Assert-PreparedSettlementCarrier {
    param(
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$Label,
        [string]$CutName = $script:CutName
    )

    Assert-JsonProperty `
        -Value $Carrier `
        -PropertyName "m_Expectation" `
        -ArtifactLabel $Label
    if ($null -ne $Carrier.m_Expectation) {
        throw "$Label mixes movement and settlement carrier families."
    }
    foreach ($property in @(
        "m_iAccepted",
        "m_iCasualties",
        "m_iSurvivors",
        "m_iAttackRefund",
        "m_iSupportRefund",
        "m_iAttackBeforeRefund",
        "m_iSupportBeforeRefund",
        "m_iPreparedAtSecond",
        "m_iPrefixRevision",
        "m_iExpectedPrefixMutationCount",
        "m_iExpectedPrefixAttackDelta",
        "m_iExpectedPrefixSupportDelta",
        "m_bExpectedPrefixReceiptApplied",
        "m_sSettlementKind",
        "m_sSettlementId",
        "m_sRefundMutationId",
        "m_sReason",
        "m_SettlementExpectation",
        "m_sPreparedSettlementFingerprint")) {
        Assert-JsonProperty `
            -Value $Carrier `
            -PropertyName $property `
            -ArtifactLabel $Label
    }
    if ($Carrier.m_bExpectedPrefixReceiptApplied -isnot [bool]) {
        throw "$Label prefix receipt policy is not a JSON boolean."
    }

    $expectation = $Carrier.m_SettlementExpectation
    if ($null -eq $expectation) {
        throw "$Label settlement expectation is unavailable."
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
		"m_sExpectedSourceOwnerFactionKey",
		"m_iExpectedSourceOwnershipRevision",
		"m_sExpectedTargetOwnerFactionKey",
		"m_iExpectedTargetOwnershipRevision",
        "m_sDebitMutationId",
        "m_sSettlementKind",
        "m_sSettlementId",
        "m_sRefundMutationId",
        "m_sReason",
        "m_iAttackCost",
        "m_iSupportCost",
        "m_iAccepted",
        "m_iSurvivors",
        "m_iAttackRefund",
        "m_iSupportRefund",
        "m_iExpectedAttackPool",
        "m_iExpectedSupportPool",
        "m_iExpectedPoolRevision",
        "m_iExpectedPoolOperationalMutationCount",
        "m_sExpectedLastStrategicMutationId",
        "m_iPreparedAtSecond",
        "m_iExpectedTerminalRevision")) {
        Assert-JsonProperty `
            -Value $expectation `
            -PropertyName $property `
            -ArtifactLabel "$Label settlement expectation"
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
		"m_sExpectedSourceOwnerFactionKey",
		"m_sExpectedTargetOwnerFactionKey",
        "m_sDebitMutationId")) {
        if ([string]::IsNullOrWhiteSpace([string]$expectation.$property)) {
            throw "$Label settlement aggregate identity is incomplete."
        }
    }
    if ([string]$expectation.m_sSourceZoneId -ceq
        [string]$expectation.m_sTargetZoneId) {
        throw "$Label settlement source and target identities conflict."
    }
	if ([int]$expectation.m_iExpectedSourceOwnershipRevision -le 0 -or
		[int]$expectation.m_iExpectedTargetOwnershipRevision -le 0) {
		throw "$Label settlement endpoint ownership revisions are invalid."
	}

    $attackCost = [int]$expectation.m_iAttackCost
    $supportCost = [int]$expectation.m_iSupportCost
    $attackFunded = $attackCost -gt 0 -and $supportCost -eq 0
    $supportFunded = $supportCost -gt 0 -and $attackCost -eq 0
    $accepted = [int]$Carrier.m_iAccepted
    $casualties = [int]$Carrier.m_iCasualties
    $survivors = [int]$Carrier.m_iSurvivors
    $attackRefund = [int]$Carrier.m_iAttackRefund
    $supportRefund = [int]$Carrier.m_iSupportRefund
    if ((-not $attackFunded -and -not $supportFunded) -or
        $accepted -le 1 -or $casualties -le 0 -or $survivors -le 0 -or
        $survivors -ne ($accepted - $casualties) -or
        [int]$expectation.m_iAccepted -ne $accepted -or
        [int]$expectation.m_iSurvivors -ne $survivors -or
        [int]$expectation.m_iAttackRefund -ne $attackRefund -or
        [int]$expectation.m_iSupportRefund -ne $supportRefund -or
        $attackRefund -ne [int][Math]::Floor(
            ($attackCost * $survivors) / [double]$accepted) -or
        $supportRefund -ne [int][Math]::Floor(
            ($supportCost * $survivors) / [double]$accepted) -or
        ($attackRefund + $supportRefund) -le 0) {
        throw "$Label settlement funding, survivors, or proportional refund is not exact."
    }

    $settlementKind = [string]$Carrier.m_sSettlementKind
    $settlementId = [string]$Carrier.m_sSettlementId
    $refundId = [string]$Carrier.m_sRefundMutationId
    $expectedSettlementId = "settlement_{0}_{1}" -f `
        [string]$expectation.m_sOperationId, $settlementKind
    if ($settlementKind -cne "route_failed_survivors" -or
        $settlementId -cne $expectedSettlementId -or
        $refundId -cne ("enemy_resource_refund_" + $settlementId) -or
        [string]$expectation.m_sSettlementKind -cne $settlementKind -or
        [string]$expectation.m_sSettlementId -cne $settlementId -or
        [string]$expectation.m_sRefundMutationId -cne $refundId -or
        [string]::IsNullOrWhiteSpace([string]$Carrier.m_sReason) -or
        [string]$expectation.m_sReason -cne [string]$Carrier.m_sReason) {
        throw "$Label settlement identity is not deterministic."
    }

    $expectedMutationCount = 0
    $expectedAttackDelta = 0
    $expectedSupportDelta = 0
    $expectedReceiptApplied = $false
    if ($CutName -ceq "prepared_after_refund" -or
        $CutName -ceq "prepared_after_receipt") {
        $expectedMutationCount = 1
        $expectedAttackDelta = $attackRefund
        $expectedSupportDelta = $supportRefund
    }
    if ($CutName -ceq "prepared_after_receipt") {
        $expectedReceiptApplied = $true
    }
    if ([int]$Carrier.m_iExpectedPrefixMutationCount -ne
            $expectedMutationCount -or
        [int]$Carrier.m_iExpectedPrefixAttackDelta -ne
            $expectedAttackDelta -or
        [int]$Carrier.m_iExpectedPrefixSupportDelta -ne
            $expectedSupportDelta -or
        [bool]$Carrier.m_bExpectedPrefixReceiptApplied -ne
            $expectedReceiptApplied) {
        throw "$Label prepared prefix policy is not exact."
    }

    $terminalRevision = [int]$Carrier.m_iPrefixRevision + 2
    if ($expectedReceiptApplied) {
        $terminalRevision = [int]$Carrier.m_iPrefixRevision + 1
    }
    if ([int]$Carrier.m_iPreparedAtSecond -le 0 -or
        [int]$Carrier.m_iPrefixRevision -le 0 -or
        [int]$expectation.m_iPreparedAtSecond -ne
            [int]$Carrier.m_iPreparedAtSecond -or
        [int]$expectation.m_iExpectedTerminalRevision -ne
            $terminalRevision -or
        [int]$Carrier.m_iAttackBeforeRefund -lt 0 -or
        [int]$Carrier.m_iSupportBeforeRefund -lt 0 -or
        [int]$expectation.m_iExpectedAttackPool -ne
            ([int]$Carrier.m_iAttackBeforeRefund + $attackRefund) -or
        [int]$expectation.m_iExpectedSupportPool -ne
            ([int]$Carrier.m_iSupportBeforeRefund + $supportRefund) -or
        [int]$expectation.m_iExpectedPoolRevision -le 0 -or
        [int]$expectation.m_iExpectedPoolOperationalMutationCount -ne 2 -or
        [string]$expectation.m_sExpectedLastStrategicMutationId -cne
            $refundId) {
        throw "$Label terminal settlement expectation is not exact."
    }

    $preparedFingerprint = [string]$Carrier.m_sPreparedSemanticFingerprint
    if ([string]$Carrier.m_sPreparedSettlementFingerprint -cne
            $preparedFingerprint -or
        [string]$Carrier.m_sRawPreparedCutSemanticFingerprint -cne
            $preparedFingerprint -or
        [int]$Carrier.m_iExpectedPhysicalAdapterHandleCount -ne 0 -or
        [int]$Carrier.m_iExpectedPhysicalRuntimeMemberCount -ne 0) {
        throw "$Label prepared fingerprint or zero-physical policy is not exact."
    }
}

function Assert-OwnerAppliedPendingCarrier {
    param(
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-JsonProperty `
        -Value $Carrier `
        -PropertyName "m_SettlementExpectation" `
        -ArtifactLabel $Label
    Assert-JsonProperty `
        -Value $Carrier `
        -PropertyName "m_Expectation" `
        -ArtifactLabel $Label
    if ($null -ne $Carrier.m_SettlementExpectation -or
        $null -eq $Carrier.m_Expectation) {
        throw "$Label mixes owner-applied pending and settlement carrier families."
    }

    $expectation = $Carrier.m_Expectation
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
        "m_sExpectedSourceOwnerFactionKey",
        "m_iExpectedSourceOwnershipRevision",
        "m_sExpectedTargetOwnerFactionKey",
        "m_iExpectedTargetOwnershipRevision",
        "m_sDebitMutationId",
        "m_iAttackCost",
        "m_iSupportCost",
        "m_iExpectedAttackPool",
        "m_iExpectedSupportPool",
        "m_iExpectedPoolOperationalMutationCount",
        "m_iAcceptedMemberCount",
        "m_iLivingMemberCount",
        "m_sLivingSlotFingerprint",
        "m_bExpectedLivingSlotsEverAlive",
        "m_iExpectedNormalizedSlotAttemptCount",
        "m_sConfirmedCasualtySlotId",
        "m_sCasualtyTombstoneFingerprint",
        "m_iExpectedNormalizedReprojectionCount")) {
        Assert-JsonProperty `
            -Value $expectation `
            -PropertyName $property `
            -ArtifactLabel "$Label expectation"
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
        "m_sExpectedSourceOwnerFactionKey",
        "m_sExpectedTargetOwnerFactionKey",
        "m_sDebitMutationId",
        "m_sLivingSlotFingerprint")) {
        if ([string]::IsNullOrWhiteSpace([string]$expectation.$property)) {
            throw "$Label owner-applied pending identity is incomplete."
        }
    }
    if ([string]$expectation.m_sSourceZoneId -ceq
            [string]$expectation.m_sTargetZoneId -or
        [int]$expectation.m_iExpectedSourceOwnershipRevision -le 0 -or
        [int]$expectation.m_iExpectedTargetOwnershipRevision -le 0 -or
        [string]$expectation.m_sExpectedTargetOwnerFactionKey -cne
            [string]$expectation.m_sFactionKey) {
        throw "$Label owner-applied pending endpoint authority is invalid."
    }

    $attackFunded = [int]$expectation.m_iAttackCost -gt 0 -and
        [int]$expectation.m_iSupportCost -eq 0
    $supportFunded = [int]$expectation.m_iSupportCost -gt 0 -and
        [int]$expectation.m_iAttackCost -eq 0
    $accepted = [int]$expectation.m_iAcceptedMemberCount
    $living = [int]$expectation.m_iLivingMemberCount
    $livingSlotIds = @(
        ([string]$expectation.m_sLivingSlotFingerprint -split ',') |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ((-not $attackFunded -and -not $supportFunded) -or
        [int]$expectation.m_iExpectedAttackPool -lt 0 -or
        [int]$expectation.m_iExpectedSupportPool -lt 0 -or
        [int]$expectation.m_iExpectedPoolOperationalMutationCount -ne 1 -or
        $accepted -le 0 -or $living -ne $accepted -or
        $livingSlotIds.Count -ne $living -or
        @($livingSlotIds | Select-Object -Unique).Count -ne $living -or
        [bool]$expectation.m_bExpectedLivingSlotsEverAlive -or
        [int]$expectation.m_iExpectedNormalizedSlotAttemptCount -ne 0 -or
        -not [string]::IsNullOrWhiteSpace(
            [string]$expectation.m_sConfirmedCasualtySlotId) -or
        -not [string]::IsNullOrWhiteSpace(
            [string]$expectation.m_sCasualtyTombstoneFingerprint) -or
        [int]$expectation.m_iExpectedNormalizedReprojectionCount -ne 0) {
        throw "$Label owner-applied pending resource or roster authority is invalid."
    }

    $progress = [double]$Carrier.m_fPreparedRouteProgressMeters
    $total = [double]$Carrier.m_fPreparedRouteTotalDistanceMeters
    if ([double]::IsNaN($progress) -or [double]::IsInfinity($progress) -or
        [double]::IsNaN($total) -or [double]::IsInfinity($total) -or
        [int]$Carrier.m_iPreparedElapsedSecond -le 0 -or $total -le 0.0 -or
        [Math]::Abs($progress - $total) -gt 0.1 -or
        -not (Test-ProofVectorNonZero $Carrier.m_vPreparedStrategicPosition) -or
        [string]$Carrier.m_sRawPreparedCutSemanticFingerprint -ceq
            [string]$Carrier.m_sPreparedSemanticFingerprint -or
        [int]$Carrier.m_iExpectedPhysicalAdapterHandleCount -ne 0 -or
        [int]$Carrier.m_iExpectedPhysicalRuntimeMemberCount -ne 0) {
        throw "$Label owner-applied pending route, fingerprint, or physical authority is invalid."
    }
}

function Assert-NativeSourceCarrierEvidence {
    param(
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($property in @(
        "m_bNativeSourceSelectionProof",
        "m_sNativeSavePointId",
        "m_sFallbackConflictSemanticFingerprint")) {
        Assert-JsonProperty `
            -Value $Carrier `
            -PropertyName $property `
            -ArtifactLabel $Label
    }
    if ($Carrier.m_bNativeSourceSelectionProof -isnot [bool] -or
        -not [bool]$Carrier.m_bNativeSourceSelectionProof) {
        throw "$Label omitted native source-selection authority."
    }
    Assert-NativeSavePointId `
        -SavePointId ([string]$Carrier.m_sNativeSavePointId) `
        -Label "$Label save point id"
    $fallbackFingerprint =
        [string]$Carrier.m_sFallbackConflictSemanticFingerprint
    if ([string]::IsNullOrWhiteSpace($fallbackFingerprint) -or
        $fallbackFingerprint -ceq
            [string]$Carrier.m_sPreparedSemanticFingerprint) {
        throw "$Label fallback-conflict fingerprint is not independent."
    }
}

function Assert-PreparedCarrier {
    param(
        [Parameter(Mandatory = $true)]$Carrier,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$ExpectedBuild
    )

    $label = "$($script:CutName) carrier"
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
        "m_iExpectedPhysicalAdapterHandleCount",
        "m_iExpectedPhysicalRuntimeMemberCount",
        "m_sPreparedSemanticFingerprint",
        "m_sRawPreparedCutSemanticFingerprint")) {
        Assert-JsonProperty `
            -Value $Carrier `
            -PropertyName $property `
            -ArtifactLabel $label
    }
    if ([string]$Carrier.m_sMagic -cne $script:CarrierMagic -or
        [string]$Carrier.m_sSessionNonce -cne $SessionNonce -or
        [string]$Carrier.m_sRunId -cne $RunId -or
        [string]$Carrier.m_sCutName -cne $script:CutName -or
        [int]$Carrier.m_iCut -ne $script:CutOrdinal -or
        [string]::IsNullOrWhiteSpace([string]$Carrier.m_sWorld) -or
        [string]::IsNullOrWhiteSpace(
            [string]$Carrier.m_sPreparedSemanticFingerprint) -or
        [string]::IsNullOrWhiteSpace(
            [string]$Carrier.m_sRawPreparedCutSemanticFingerprint)) {
        throw "$label identity is not exact."
    }
    Assert-BuildIdentity `
        -Artifact $Carrier `
        -ExpectedBuild $ExpectedBuild `
        -ArtifactLabel $label

    if ($script:NativeSourceSelection) {
        Assert-NativeSourceCarrierEvidence -Carrier $Carrier -Label $label
    }
    elseif ($Carrier.PSObject.Properties.Name -contains
            "m_bNativeSourceSelectionProof" -and
        [bool]$Carrier.m_bNativeSourceSelectionProof) {
        throw "$label unexpectedly claims native source-selection authority."
    }

    if ($script:IsPreparedSettlementCut) {
        Assert-PreparedSettlementCarrier -Carrier $Carrier -Label $label
        return $Carrier
    }
    if ($script:IsOwnerAppliedPendingCut) {
        Assert-OwnerAppliedPendingCarrier -Carrier $Carrier -Label $label
        return $Carrier
    }

    Assert-JsonProperty `
        -Value $Carrier `
        -PropertyName "m_SettlementExpectation" `
        -ArtifactLabel $label
    if ($null -ne $Carrier.m_SettlementExpectation) {
        throw "$label mixes movement and settlement carrier families."
    }

    foreach ($property in @(
        "m_iPreparedElapsedSecond",
        "m_fPreparedRouteProgressMeters",
        "m_fPreparedRouteTotalDistanceMeters",
        "m_vPreparedStrategicPosition",
        "m_vInjectedStalePosition",
        "m_vPreparedLivePosition")) {
        Assert-JsonProperty `
            -Value $Carrier `
            -PropertyName $property `
            -ArtifactLabel $label
    }
    Assert-JsonProperty `
        -Value $Carrier `
        -PropertyName "m_Expectation" `
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
		"m_sExpectedSourceOwnerFactionKey",
		"m_iExpectedSourceOwnershipRevision",
		"m_sExpectedTargetOwnerFactionKey",
		"m_iExpectedTargetOwnershipRevision",
        "m_sDebitMutationId",
        "m_iAttackCost",
        "m_iSupportCost",
        "m_iExpectedAttackPool",
        "m_iExpectedSupportPool",
        "m_iExpectedPoolOperationalMutationCount",
        "m_iAcceptedMemberCount",
        "m_iLivingMemberCount",
        "m_sLivingSlotFingerprint",
        "m_bExpectedLivingSlotsEverAlive",
        "m_iExpectedNormalizedSlotAttemptCount",
        "m_sConfirmedCasualtySlotId",
        "m_sCasualtyTombstoneFingerprint",
        "m_iExpectedNormalizedReprojectionCount")) {
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
		"m_sExpectedSourceOwnerFactionKey",
		"m_sExpectedTargetOwnerFactionKey",
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
		[int]$expectation.m_iExpectedPoolOperationalMutationCount -ne 1 -or
		[int]$expectation.m_iExpectedSourceOwnershipRevision -le 0 -or
		[int]$expectation.m_iExpectedTargetOwnershipRevision -le 0) {
        throw "$label expectation counts are invalid."
    }
    $normalizedFingerprint = [string]$Carrier.m_sPreparedSemanticFingerprint
    $rawCutFingerprint = [string]$Carrier.m_sRawPreparedCutSemanticFingerprint
    $livingSlotIds = @(
        ([string]$expectation.m_sLivingSlotFingerprint -split ',') |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $uniqueLivingSlotIds = @($livingSlotIds | Select-Object -Unique)
    if ($livingSlotIds.Count -ne [int]$expectation.m_iLivingMemberCount -or
        $uniqueLivingSlotIds.Count -ne $livingSlotIds.Count) {
        throw "$label living-slot fingerprint is not exact."
    }
    if ($script:CutName -ceq "outbound_virtual") {
        if ([bool]$expectation.m_bExpectedLivingSlotsEverAlive -or
            [int]$expectation.m_iExpectedNormalizedSlotAttemptCount -ne 0 -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$expectation.m_sConfirmedCasualtySlotId) -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$expectation.m_sCasualtyTombstoneFingerprint) -or
            [int]$expectation.m_iExpectedNormalizedReprojectionCount -ne 0 -or
            [int]$expectation.m_iLivingMemberCount -ne
                [int]$expectation.m_iAcceptedMemberCount -or
            $rawCutFingerprint -cne $normalizedFingerprint) {
            throw "$label outbound expectation is not exact."
        }
    }
    elseif ($script:CutName -ceq "dematerializing_before_hold") {
        $casualtySlotId = [string]$expectation.m_sConfirmedCasualtySlotId
        $casualtyFingerprint = [string]$expectation.m_sCasualtyTombstoneFingerprint
        if ([string]::IsNullOrWhiteSpace($casualtySlotId) -or
            [string]::IsNullOrWhiteSpace($casualtyFingerprint) -or
            -not $casualtyFingerprint.StartsWith($casualtySlotId + "|") -or
            [int]$expectation.m_iExpectedNormalizedReprojectionCount -ne 1 -or
            [int]$expectation.m_iLivingMemberCount -ne
                ([int]$expectation.m_iAcceptedMemberCount - 1) -or
            $livingSlotIds -ccontains $casualtySlotId -or
            $rawCutFingerprint -ceq $normalizedFingerprint) {
            throw "$label dematerializing expectation is not exact."
        }
    }
    elseif ($script:CutName -ceq "materializing_checkpoint_deferred") {
        if ([bool]$expectation.m_bExpectedLivingSlotsEverAlive -or
            [int]$expectation.m_iExpectedNormalizedSlotAttemptCount -ne 0 -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$expectation.m_sConfirmedCasualtySlotId) -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$expectation.m_sCasualtyTombstoneFingerprint) -or
            [int]$expectation.m_iExpectedNormalizedReprojectionCount -ne 0 -or
            [int]$expectation.m_iLivingMemberCount -ne
                [int]$expectation.m_iAcceptedMemberCount -or
            $rawCutFingerprint -ceq $normalizedFingerprint) {
            throw "$label materializing expectation is not exact."
        }
    }
    elseif ($script:CutName -ceq "physical_live_position") {
        $stalePosition = $Carrier.m_vInjectedStalePosition |
            ConvertTo-Json -Compress -Depth 4
        $livePosition = $Carrier.m_vPreparedLivePosition |
            ConvertTo-Json -Compress -Depth 4
        if (-not [bool]$expectation.m_bExpectedLivingSlotsEverAlive -or
            [int]$expectation.m_iExpectedNormalizedSlotAttemptCount -ne 1 -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$expectation.m_sConfirmedCasualtySlotId) -or
            -not [string]::IsNullOrWhiteSpace(
                [string]$expectation.m_sCasualtyTombstoneFingerprint) -or
            [int]$expectation.m_iExpectedNormalizedReprojectionCount -ne 1 -or
            [int]$expectation.m_iLivingMemberCount -ne
                [int]$expectation.m_iAcceptedMemberCount -or
            [int]$Carrier.m_iExpectedPhysicalAdapterHandleCount -ne
                ([int]$expectation.m_iLivingMemberCount + 1) -or
            [int]$Carrier.m_iExpectedPhysicalRuntimeMemberCount -ne
                [int]$expectation.m_iLivingMemberCount -or
            [string]::IsNullOrWhiteSpace($stalePosition) -or
            [string]::IsNullOrWhiteSpace($livePosition) -or
            $stalePosition -ceq $livePosition -or
            $rawCutFingerprint -ceq $normalizedFingerprint) {
            throw "$label physical live-position expectation is not exact."
        }
    }
    else {
        throw "$label uses an unsupported cut."
    }
    $progress = [double]$Carrier.m_fPreparedRouteProgressMeters
    $total = [double]$Carrier.m_fPreparedRouteTotalDistanceMeters
    $zeroProgressCut = $script:CutName -ceq "dematerializing_before_hold" -or
        $script:CutName -ceq "physical_live_position"
    $progressInvalid = $progress -lt 0.0 -or
        (-not $zeroProgressCut -and $progress -le 0.0)
    if ([double]::IsNaN($progress) -or [double]::IsInfinity($progress) -or
        [double]::IsNaN($total) -or [double]::IsInfinity($total) -or
        $progressInvalid -or $total -le $progress -or
        [int]$Carrier.m_iPreparedElapsedSecond -le 0) {
        throw "$label route state is invalid."
    }
    return $Carrier
}

function Assert-PreparedSettlementStageSemantics {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)]
        [ValidateSet("prepare", "recover", "replay")][string]$Stage,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $sourceFingerprint = [string]$Result.m_sSourceSemanticFingerprint
    $finalFingerprint = [string]$Result.m_sFinalSemanticFingerprint
    $preparedFingerprint = [string]$Result.m_sRawPreparedCutSemanticFingerprint
    if (-not [bool]$Result.m_bPreparedCutExact -or
        -not [bool]$Result.m_bCasualtyContinuityExact -or
        [int]$Result.m_iPhysicalAdapterHandleCount -ne 0 -or
        [int]$Result.m_iPhysicalRuntimeMemberCount -ne 0 -or
        [bool]$Result.m_bPhysicalBindingsExact -or
        [bool]$Result.m_bLivePositionRefreshExact -or
        [bool]$Result.m_bPhysicalCaptureNormalizedExact) {
        throw "$Label omitted exact settlement authority or invented physical authority."
    }
    if ($Stage -eq "prepare") {
        if ([bool]$Result.m_bRestored -or
            [bool]$Result.m_bStartupReconcileChanged -or
            [bool]$Result.m_bContinuationExact -or
            [bool]$Result.m_bSameStateSemanticNoOp -or
            $sourceFingerprint -cne $finalFingerprint -or
            $preparedFingerprint -cne $sourceFingerprint) {
            throw "$Label violates the fresh PREPARED-settlement invariant."
        }
    }
    elseif ($Stage -eq "recover") {
        if (-not [bool]$Result.m_bRestored -or
            -not [bool]$Result.m_bStartupReconcileChanged -or
            -not [bool]$Result.m_bContinuationExact -or
            -not [bool]$Result.m_bSameStateSemanticNoOp -or
            $sourceFingerprint -ceq $finalFingerprint -or
            $preparedFingerprint -cne $sourceFingerprint) {
            throw "$Label violates the first-start PREPARED-settlement recovery invariant."
        }
    }
    else {
        if (-not [bool]$Result.m_bRestored -or
            [bool]$Result.m_bStartupReconcileChanged -or
            [bool]$Result.m_bContinuationExact -or
            -not [bool]$Result.m_bSameStateSemanticNoOp -or
            $sourceFingerprint -cne $finalFingerprint -or
            $preparedFingerprint -ceq $sourceFingerprint) {
            throw "$Label violates the second-start settlement semantic no-op invariant."
        }
    }
}

function Assert-OwnerAppliedPendingStageSemantics {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)]
        [ValidateSet("prepare", "recover", "replay")][string]$Stage,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $sourceFingerprint = [string]$Result.m_sSourceSemanticFingerprint
    $finalFingerprint = [string]$Result.m_sFinalSemanticFingerprint
    $preparedFingerprint = [string]$Result.m_sRawPreparedCutSemanticFingerprint
    if (-not [bool]$Result.m_bPreparedCutExact -or
        -not [bool]$Result.m_bCasualtyContinuityExact -or
        [int]$Result.m_iPhysicalAdapterHandleCount -ne 0 -or
        [int]$Result.m_iPhysicalRuntimeMemberCount -ne 0 -or
        [bool]$Result.m_bPhysicalBindingsExact -or
        [bool]$Result.m_bLivePositionRefreshExact -or
        [bool]$Result.m_bPhysicalCaptureNormalizedExact) {
        throw "$Label omitted owner-applied pending authority or invented physical authority."
    }
    if ($Stage -eq "prepare") {
        if ([bool]$Result.m_bRestored -or
            [bool]$Result.m_bStartupReconcileChanged -or
            [bool]$Result.m_bOwnershipStartupReconcileChanged -or
            [bool]$Result.m_bContinuationExact -or
            [bool]$Result.m_bSameStateSemanticNoOp -or
            $sourceFingerprint -cne $finalFingerprint -or
            $preparedFingerprint -ceq $sourceFingerprint) {
            throw "$Label violates the fresh owner-applied pending invariant."
        }
    }
    elseif ($Stage -eq "recover") {
        if (-not [bool]$Result.m_bRestored -or
            -not [bool]$Result.m_bOwnershipStartupReconcileChanged -or
            -not [bool]$Result.m_bContinuationExact -or
            -not [bool]$Result.m_bSameStateSemanticNoOp -or
            $sourceFingerprint -ceq $finalFingerprint -or
            $preparedFingerprint -ceq $sourceFingerprint) {
            throw "$Label violates the first-start owner-applied completion invariant."
        }
    }
    else {
        if (-not [bool]$Result.m_bRestored -or
            [bool]$Result.m_bOwnershipStartupReconcileChanged -or
            [bool]$Result.m_bContinuationExact -or
            -not [bool]$Result.m_bSameStateSemanticNoOp -or
            $sourceFingerprint -cne $finalFingerprint -or
            $preparedFingerprint -ceq $sourceFingerprint) {
            throw "$Label violates the second-start owner-applied semantic no-op invariant."
        }
    }
}

function Assert-NativeSourceStageEvidence {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [Parameter(Mandatory = $true)]
        [ValidateSet("prepare", "recover", "replay")][string]$Stage,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($property in @(
        "m_bNativeSourceSelectionProof",
        "m_bNativePersistenceLoaded",
        "m_bFallbackConflictRejected",
        "m_bNativeSavePointCommitted",
        "m_sNativeSavePointId",
        "m_sRestoreSource",
        "m_sFallbackConflictSemanticFingerprint")) {
        Assert-JsonProperty `
            -Value $Result `
            -PropertyName $property `
            -ArtifactLabel $Label
    }
    foreach ($property in @(
        "m_bNativeSourceSelectionProof",
        "m_bNativePersistenceLoaded",
        "m_bFallbackConflictRejected",
        "m_bNativeSavePointCommitted")) {
        if ($Result.$property -isnot [bool]) {
            throw "$Label contains a non-boolean native invariant."
        }
    }

    $restoredStage = $Stage -cne "prepare"
    $savePointStage = $Stage -cne "replay"
    $expectedSource = if ($Stage -ceq "prepare") {
        "new_campaign"
    }
    else {
        "native"
    }
    if (-not [bool]$Result.m_bNativeSourceSelectionProof -or
        [bool]$Result.m_bNativePersistenceLoaded -ne $restoredStage -or
        [bool]$Result.m_bFallbackConflictRejected -ne $restoredStage -or
        [bool]$Result.m_bNativeSavePointCommitted -ne $savePointStage -or
        [string]$Result.m_sRestoreSource -cne $expectedSource) {
        throw "$Label native source-selection stage evidence is not exact."
    }
    Assert-NativeSavePointId `
        -SavePointId ([string]$Result.m_sNativeSavePointId) `
        -Label "$Label save point id"
    $fallbackFingerprint =
        [string]$Result.m_sFallbackConflictSemanticFingerprint
    if ([string]::IsNullOrWhiteSpace($fallbackFingerprint) -or
        $fallbackFingerprint -ceq
            [string]$Result.m_sRawPreparedCutSemanticFingerprint) {
        throw "$Label fallback-conflict evidence is not independent."
    }
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

    $label = "$($script:CutName)/$Stage result"
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
        "m_bOwnershipStartupReconcileChanged",
        "m_bSourceExact",
        "m_bContinuationExact",
        "m_bSameStateSemanticNoOp",
        "m_bRuntimeClaimantsZero",
        "m_bPersistedReadBackExact",
        "m_bPreparedCutExact",
        "m_bCasualtyContinuityExact",
        "m_bPhysicalBindingsExact",
        "m_bLivePositionRefreshExact",
        "m_bPhysicalCaptureNormalizedExact",
        "m_iPhysicalAdapterHandleCount",
        "m_iPhysicalRuntimeMemberCount",
        "m_vInjectedStalePosition",
        "m_vPreparedLivePosition",
        "m_fProgressBeforeMeters",
        "m_fProgressAfterMeters",
        "m_sSourceSemanticFingerprint",
        "m_sFinalSemanticFingerprint",
        "m_sRawPreparedCutSemanticFingerprint",
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
        [int]$Result.m_iCut -ne $script:CutOrdinal -or
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
        "m_bOwnershipStartupReconcileChanged",
        "m_bSourceExact",
        "m_bContinuationExact",
        "m_bSameStateSemanticNoOp",
        "m_bRuntimeClaimantsZero",
        "m_bPersistedReadBackExact",
        "m_bPreparedCutExact",
        "m_bCasualtyContinuityExact",
        "m_bPhysicalBindingsExact",
        "m_bLivePositionRefreshExact",
        "m_bPhysicalCaptureNormalizedExact")) {
        if ($Result.$property -isnot [bool]) {
            throw "$label contains a non-boolean invariant."
        }
    }
    if (-not [bool]$Result.m_bSuccess) {
        $failureFlags = "source=$([bool]$Result.m_bSourceExact)," +
            "runtime=$([bool]$Result.m_bRuntimeClaimantsZero)," +
            "readback=$([bool]$Result.m_bPersistedReadBackExact)," +
            "prepared=$([bool]$Result.m_bPreparedCutExact)," +
            "casualty=$([bool]$Result.m_bCasualtyContinuityExact)," +
            "restored=$([bool]$Result.m_bRestored)," +
            "ownership=$([bool]$Result.m_bOwnershipStartupReconcileChanged)," +
            "continuation=$([bool]$Result.m_bContinuationExact)," +
            "noop=$([bool]$Result.m_bSameStateSemanticNoOp)"
		if ($script:NativeSourceSelection) {
			$failureFlags += ",nativeLoaded=$([bool]$Result.m_bNativePersistenceLoaded)," +
				"fallbackRejected=$([bool]$Result.m_bFallbackConflictRejected)," +
				"saveCommitted=$([bool]$Result.m_bNativeSavePointCommitted)," +
				"restoreSource=$([string]$Result.m_sRestoreSource)"
		}
		$rawFailureEvidence = [string]$Result.m_sEvidence
        $safeFailureEvidence = ConvertTo-SafeEvidenceLine `
			-Line $rawFailureEvidence
        if ([string]::IsNullOrWhiteSpace($safeFailureEvidence)) {
            $safeFailureEvidence = "no bounded engine evidence"
        }
		if ($rawFailureEvidence.Length -gt 220) {
			$tailStart = [Math]::Max(0, $rawFailureEvidence.Length - 220)
			$safeFailureTail = ConvertTo-SafeEvidenceLine `
				-Line $rawFailureEvidence.Substring($tailStart)
			if (-not [string]::IsNullOrWhiteSpace($safeFailureTail)) {
				$safeFailureEvidence = $safeFailureTail +
					" | head " + $safeFailureEvidence
			}
		}
        throw "$label reported failure ($failureFlags): $safeFailureEvidence"
    }
    Assert-OwnershipStartupScope -Result $Result -Label $label
    if ($script:NativeSourceSelection) {
        Assert-NativeSourceStageEvidence `
            -Result $Result `
            -Stage $Stage `
            -Label $label
    }
    elseif ($Result.PSObject.Properties.Name -contains
            "m_bNativeSourceSelectionProof" -and
        [bool]$Result.m_bNativeSourceSelectionProof) {
        throw "$label unexpectedly claims native source-selection authority."
    }
    if (-not [bool]$Result.m_bSourceExact -or
        -not [bool]$Result.m_bRuntimeClaimantsZero -or
        -not [bool]$Result.m_bPersistedReadBackExact -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sSourceSemanticFingerprint) -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sFinalSemanticFingerprint) -or
        [string]::IsNullOrWhiteSpace(
            [string]$Result.m_sRawPreparedCutSemanticFingerprint) -or
        [string]::IsNullOrWhiteSpace([string]$Result.m_sEvidence)) {
        throw "$label omitted a required success invariant."
    }
    if ($script:CutName -cne "outbound_virtual" -and
        (-not [bool]$Result.m_bPreparedCutExact -or
            -not [bool]$Result.m_bCasualtyContinuityExact)) {
        throw "$label omitted exact prepared-cut or casualty continuity proof."
    }
    if ($script:IsPreparedSettlementCut) {
        Assert-PreparedSettlementStageSemantics `
            -Result $Result `
            -Stage $Stage `
            -Label $label
        return $Result
    }
    if ($script:IsOwnerAppliedPendingCut) {
        Assert-OwnerAppliedPendingStageSemantics `
            -Result $Result `
            -Stage $Stage `
            -Label $label
        return $Result
    }
    if ($script:CutName -cne "outbound_virtual" -and
        ([string]$Result.m_sRawPreparedCutSemanticFingerprint -ceq
            [string]$Result.m_sSourceSemanticFingerprint -or
            [string]$Result.m_sRawPreparedCutSemanticFingerprint -ceq
                [string]$Result.m_sFinalSemanticFingerprint)) {
        throw "$label collapsed a checkpoint-deferred cut into normalized virtual state."
    }
    if ($script:CutName -ceq "physical_live_position") {
        $stalePosition = $Result.m_vInjectedStalePosition |
            ConvertTo-Json -Compress -Depth 4
        $livePosition = $Result.m_vPreparedLivePosition |
            ConvertTo-Json -Compress -Depth 4
        if ([int]$Result.m_iPhysicalRuntimeMemberCount -le 0 -or
            [int]$Result.m_iPhysicalAdapterHandleCount -ne
                ([int]$Result.m_iPhysicalRuntimeMemberCount + 1) -or
            $stalePosition -ceq $livePosition) {
            throw "$label omitted exact physical root/member or live-position evidence."
        }
        if ($Stage -eq "prepare" -and
            (-not [bool]$Result.m_bPhysicalBindingsExact -or
                -not [bool]$Result.m_bLivePositionRefreshExact -or
                -not [bool]$Result.m_bPhysicalCaptureNormalizedExact)) {
            throw "$label omitted a physical capture gate."
        }
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
            throw "$label violates the fresh normalized-source invariant."
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

    $summary = [pscustomobject]@{
        Cut = $script:CutName
        Stage = [string]$Result.m_sStage
        Success = [bool]$Result.m_bSuccess
        Exit = $ExitCode
        Build = ([string]$Result.m_sBuildSha).Substring(0, 12)
        Schema = [int]$Result.m_iCampaignSchemaVersion
        Restored = [bool]$Result.m_bRestored
        StartupChanged = [bool]$Result.m_bStartupReconcileChanged
        OwnershipStartupChanged =
            [bool]$Result.m_bOwnershipStartupReconcileChanged
        SourceExact = [bool]$Result.m_bSourceExact
        ContinuationExact = [bool]$Result.m_bContinuationExact
        SemanticNoOp = [bool]$Result.m_bSameStateSemanticNoOp
        RuntimeClaimantsZero = [bool]$Result.m_bRuntimeClaimantsZero
        ReadBackExact = [bool]$Result.m_bPersistedReadBackExact
        PreparedCutExact = [bool]$Result.m_bPreparedCutExact
        CasualtyContinuityExact = [bool]$Result.m_bCasualtyContinuityExact
        PhysicalBindingsExact = [bool]$Result.m_bPhysicalBindingsExact
        LivePositionRefreshExact = [bool]$Result.m_bLivePositionRefreshExact
        PhysicalCaptureNormalizedExact =
            [bool]$Result.m_bPhysicalCaptureNormalizedExact
        ProgressAdvanced = [double]$Result.m_fProgressAfterMeters -gt
            ([double]$Result.m_fProgressBeforeMeters + 0.1)
        SourceDigest = (Get-StringDigest `
            -Value ([string]$Result.m_sSourceSemanticFingerprint)).Substring(0, 16)
        FinalDigest = (Get-StringDigest `
            -Value ([string]$Result.m_sFinalSemanticFingerprint)).Substring(0, 16)
    }
    if ($script:NativeSourceSelection) {
        $summary | Add-Member `
            -NotePropertyName NativePersistenceLoaded `
            -NotePropertyValue ([bool]$Result.m_bNativePersistenceLoaded)
        $summary | Add-Member `
            -NotePropertyName FallbackConflictRejected `
            -NotePropertyValue ([bool]$Result.m_bFallbackConflictRejected)
        $summary | Add-Member `
            -NotePropertyName NativeSavePointCommitted `
            -NotePropertyValue ([bool]$Result.m_bNativeSavePointCommitted)
        $summary | Add-Member `
            -NotePropertyName RestoreSource `
            -NotePropertyValue ([string]$Result.m_sRestoreSource)
    }
    return $summary
}

function Get-NativePackArgumentVector {
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

function Assert-NativeAddonSearchRoot {
    param(
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent
    )

    $expectedParent = [IO.Path]::GetFullPath(
        (Join-Path $ProfileRoot "packed-addons"))
    $resolvedParent = Resolve-ExistingPath `
        -Path $PackedAddonsParent `
        -Kind Container
    if (-not $resolvedParent.Equals(
        $expectedParent,
        [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-ContainedPath `
            -Root $ProfileRoot `
            -Candidate $resolvedParent)) {
        throw "Native packed add-on search root escaped its nonce-owned profile."
    }
    Assert-NoReparsePathAncestry -Path $resolvedParent
    $entries = @(Get-ChildItem `
        -LiteralPath $resolvedParent `
        -Force `
        -ErrorAction Stop)
    $directories = @($entries | Where-Object { $_.PSIsContainer })
    $files = @($entries | Where-Object { -not $_.PSIsContainer })
    if ($directories.Count -ne 1 -or $files.Count -ne 0 -or
        [string]$directories[0].Name -cne "Partisan") {
        throw "Native packed add-on search root must contain one exact add-on folder."
    }
    $packedAddonPath = [IO.Path]::GetFullPath($directories[0].FullName)
    if (-not (Test-ContainedPath `
        -Root $resolvedParent `
        -Candidate $packedAddonPath)) {
        throw "Native packed add-on folder escaped its guarded search root."
    }
    Assert-NoReparsePathAncestry -Path $packedAddonPath
    return $packedAddonPath
}

function Assert-NativePackedAddonLayout {
    param(
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent
    )

    $packedAddonPath = Assert-NativeAddonSearchRoot `
        -ProfileRoot $ProfileRoot `
        -PackedAddonsParent $PackedAddonsParent
    foreach ($requiredFile in @(
        "addon.gproj",
        "data.pak",
        "resourceDatabase.rdb")) {
        $requiredPath = Join-Path $packedAddonPath $requiredFile
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf) -or
            (Get-Item -LiteralPath $requiredPath -Force).Length -le 0) {
            throw "Native Workbench pack omitted a required non-empty artifact."
        }
    }
    $packedFiles = @(Get-ChildItem `
        -LiteralPath $packedAddonPath `
        -File `
        -Force `
        -ErrorAction Stop)
    $packedDirectories = @(Get-ChildItem `
        -LiteralPath $packedAddonPath `
        -Directory `
        -Force `
        -ErrorAction Stop)
    if ($packedDirectories.Count -ne 0 -or
        @($packedFiles | Where-Object { $_.Extension -ceq ".pak" }).Count -ne 1 -or
        @($packedFiles | Where-Object { $_.Extension -ceq ".gproj" }).Count -ne 1 -or
        @($packedFiles | Where-Object { $_.Extension -ceq ".rdb" }).Count -ne 1) {
        throw "Native Workbench pack artifact family is not exact."
    }
    return [pscustomobject]@{
        PackedAddonPath = $packedAddonPath
        FileCount = $packedFiles.Count
        PakCount = @($packedFiles | Where-Object {
            $_.Extension -ceq ".pak"
        }).Count
        ProjectCount = @($packedFiles | Where-Object {
            $_.Extension -ceq ".gproj"
        }).Count
        ResourceDatabaseCount = @($packedFiles | Where-Object {
            $_.Extension -ceq ".rdb"
        }).Count
    }
}

function Invoke-NativeAddonPack {
    param(
        [Parameter(Mandatory = $true)][string]$WorkbenchExecutablePath,
        [Parameter(Mandatory = $true)][string]$RuntimeAddonPath,
        [Parameter(Mandatory = $true)][string]$ProjectFile,
        [Parameter(Mandatory = $true)][string]$ProfileRoot,
        [Parameter(Mandatory = $true)][string]$PackProfilePath,
        [Parameter(Mandatory = $true)][string]$GuardedTempDirectory,
        [Parameter(Mandatory = $true)][string]$PackedAddonsParent,
        [Parameter(Mandatory = $true)][string]$PackedAddonPath,
        [Parameter(Mandatory = $true)][int]$TimeoutSeconds,
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    $packLabel = "native Workbench add-on pack"
    $resolvedParent = Resolve-ExistingPath `
        -Path $PackedAddonsParent `
        -Kind Container
    $expectedAddonPath = [IO.Path]::GetFullPath(
        (Join-Path $resolvedParent "Partisan"))
    $resolvedAddonPath = [IO.Path]::GetFullPath($PackedAddonPath)
    if (-not $resolvedAddonPath.Equals(
        $expectedAddonPath,
        [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-ContainedPath `
            -Root $ProfileRoot `
            -Candidate $resolvedAddonPath) -or
        (Test-Path -LiteralPath $resolvedAddonPath)) {
        throw "$packLabel output path is not fresh and nonce-owned."
    }
    foreach ($guardedDirectory in @(
        $PackProfilePath,
        $GuardedTempDirectory,
        $resolvedParent)) {
        [void](Resolve-ExistingPath -Path $guardedDirectory -Kind Container)
        Assert-NoReparsePathAncestry -Path $guardedDirectory
    }

    $arguments = @(Get-NativePackArgumentVector `
        -ProjectFile $ProjectFile `
        -RuntimeAddonPath $RuntimeAddonPath `
        -PackProfilePath $PackProfilePath `
        -PackedAddonPath $resolvedAddonPath)
    $commandLine = (ConvertTo-NativeArgument $WorkbenchExecutablePath) + " " +
        (($arguments | ForEach-Object {
            ConvertTo-NativeArgument ([string]$_)
        }) -join " ")
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $commandLine `
        -ExpectedExecutable $WorkbenchExecutablePath `
        -ExpectedArguments $arguments)) {
        throw "$packLabel arguments did not round-trip exactly."
    }

    $engineBefore = @(Get-EngineProcessRows).Count
    if ($engineBefore -ne 0) {
        throw "An engine process appeared before $packLabel."
    }

    $process = $null
    $job = $null
    $suspendedLauncher = $null
    $ownedProcesses = @{}
    $rootProcessId = 0
    $rootStartUtc = [DateTime]::MinValue
    $rootExitCode = $null
    $packError = $null
    $packStartedUtc = [DateTime]::UtcNow
    $packCleanupErrors = New-Object Collections.Generic.List[string]
    $packCleanupState = [ordered]@{
        OwnedRemaining = -1
        EngineAfter = -1
    }
    try {
        $job = New-Object PartisanCounterattackGuardedJob
        if (@(Get-EngineProcessRows).Count -ne 0) {
            throw "An engine process appeared during $packLabel preflight."
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
                    $WorkbenchExecutablePath,
                    $commandLine,
                    $GuardedTempDirectory)
        }
        finally {
            [Environment]::SetEnvironmentVariable(
                "TEMP", $previousTemp, [EnvironmentVariableTarget]::Process)
            [Environment]::SetEnvironmentVariable(
                "TMP", $previousTmp, [EnvironmentVariableTarget]::Process)
        }

        $process = $suspendedLauncher.Child
        if (-not $process) {
            throw "$packLabel did not create its Workbench process."
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
            throw "$packLabel exited before command-line verification."
        }
        $processRow = Get-CimInstance `
            Win32_Process `
            -Filter "ProcessId=$rootProcessId" `
            -ErrorAction Stop
        if (-not $processRow -or
            -not (Test-ExactNativeArgumentVector `
                -CommandLine ([string]$processRow.CommandLine) `
                -ExpectedExecutable $WorkbenchExecutablePath `
                -ExpectedArguments $arguments)) {
            throw "$packLabel launched with a non-exact argument vector."
        }

        $deadlineUtc = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
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
                throw "An unowned engine process appeared during $packLabel."
            }

            $process.Refresh()
            if ($process.HasExited -and $null -eq $rootExitCode) {
                $rootExitCode = $process.ExitCode
            }
            if ($null -ne $rootExitCode -and
                (Get-LiveOwnedProcessCount -Owned $ownedProcesses) -eq 0) {
                break
            }
            Start-Sleep -Milliseconds 250
        }
        if ($null -eq $rootExitCode -or
            (Get-LiveOwnedProcessCount -Owned $ownedProcesses) -ne 0) {
            throw "$packLabel exceeded its guarded deadline."
        }
        if ([int]$rootExitCode -ne 0) {
            throw "$packLabel returned a nonzero exit code."
        }
    }
    catch {
        $packError = $_.Exception.Message
    }
    finally {
        Invoke-IsolatedCleanupPhase `
            -Name "dispose-suspended-native-pack" `
            -Errors $packCleanupErrors `
            -Action {
                if ($suspendedLauncher) {
                    $suspendedLauncher.Dispose()
                    $suspendedLauncher = $null
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "discover-owned-native-pack" `
            -Errors $packCleanupErrors `
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
            -Name "request-root-stop-native-pack" `
            -Errors $packCleanupErrors `
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
            -Name "force-owned-stop-native-pack" `
            -Errors $packCleanupErrors `
            -Action {
                Stop-OwnedProcesses -Owned $ownedProcesses
                Start-Sleep -Milliseconds 300
                Stop-OwnedProcesses -Owned $ownedProcesses
            }
        Invoke-IsolatedCleanupPhase `
            -Name "close-process-job-native-pack" `
            -Errors $packCleanupErrors `
            -Action {
                if ($job) {
                    $job.Dispose()
                    $job = $null
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "final-owned-stop-native-pack" `
            -Errors $packCleanupErrors `
            -Action {
                Start-Sleep -Milliseconds 300
                Stop-OwnedProcesses -Owned $ownedProcesses
                $packCleanupState.OwnedRemaining = Get-LiveOwnedProcessCount `
                    -Owned $ownedProcesses
            }
        Invoke-IsolatedCleanupPhase `
            -Name "dispose-root-native-pack" `
            -Errors $packCleanupErrors `
            -Action {
                if ($process) {
                    $process.Dispose()
                    $process = $null
                }
            }
        Invoke-IsolatedCleanupPhase `
            -Name "audit-engine-native-pack" `
            -Errors $packCleanupErrors `
            -Action {
                $packCleanupState.EngineAfter = @(Get-EngineProcessRows).Count
            }
    }

    $ownedRemaining = [int]$packCleanupState.OwnedRemaining
    $engineAfter = [int]$packCleanupState.EngineAfter
    if ($packCleanupErrors.Count -ne 0 -or
        $ownedRemaining -ne 0 -or $engineAfter -ne 0) {
        $failedCleanupPhases = if ($packCleanupErrors.Count -eq 0) {
            "none"
        }
        else {
            $packCleanupErrors -join ","
        }
        throw "$packLabel containment cleanup failed " +
            "(phases=$failedCleanupPhases; owned=$ownedRemaining; " +
            "engine=$engineAfter)."
    }
    if ($packError) {
        $safeDiagnostic = Get-SafeGuardedEngineDiagnostic `
            -ProfileRoot $PackProfilePath `
            -ProjectDirectory (Split-Path -Parent $ProjectFile) `
            -RuntimeAddonPath $RuntimeAddonPath
        if (-not [string]::IsNullOrWhiteSpace($safeDiagnostic)) {
            throw "$packError | diagnostic $safeDiagnostic"
        }
        throw $packError
    }

    $layout = Assert-NativePackedAddonLayout `
        -ProfileRoot $ProfileRoot `
        -PackedAddonsParent $resolvedParent
    return [pscustomobject]@{
        ExitCode = [int]$rootExitCode
        EngineBefore = $engineBefore
        EngineAfter = $engineAfter
        OwnedProcessesRemaining = $ownedRemaining
        ElapsedSeconds = [Math]::Round(
            ([DateTime]::UtcNow - $packStartedUtc).TotalSeconds,
            3)
        FileCount = $layout.FileCount
        PakCount = $layout.PakCount
        ProjectCount = $layout.ProjectCount
        ResourceDatabaseCount = $layout.ResourceDatabaseCount
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
        [ValidateSet("prepare", "recover", "replay")][string]$Stage,
        [string]$NativeSavePointId = "",
        [string]$NativeServerConfigPath = "",
        [string]$NativePackedAddonsParent = ""
    )

    if (-not $script:NativeSourceSelection) {
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

    if ($Stage -ceq "prepare") {
        if (-not [string]::IsNullOrWhiteSpace($NativeSavePointId)) {
            throw "Native prepare must not load a prior session save."
        }
    }
    else {
        Assert-NativeSavePointId `
            -SavePointId $NativeSavePointId `
            -Label "$Stage load-session save point id"
    }
	if ([string]::IsNullOrWhiteSpace($NativeServerConfigPath) -or
		-not (Test-Path -LiteralPath $NativeServerConfigPath -PathType Leaf)) {
		throw "Native source selection requires an exact guarded server config."
	}

	[void](Assert-NativeAddonSearchRoot `
		-ProfileRoot $ProfileRoot `
		-PackedAddonsParent $NativePackedAddonsParent)
	$nativeAddonRoot = $RuntimeAddonPath + "," +
		[IO.Path]::GetFullPath($NativePackedAddonsParent)
	$nativeBaseProject = Join-Path $RuntimeAddonPath "data\ArmaReforger.gproj"
	if (-not (Test-Path -LiteralPath $nativeBaseProject -PathType Leaf)) {
		throw "Native source selection could not resolve the packed base project."
	}
	$nativeArguments = @(
		"-addonsDir", $nativeAddonRoot,
		"-gproj", $nativeBaseProject,
		"-config", $NativeServerConfigPath,
        "-profile", $ProfileRoot,
        "-rpl-timeout-disable",
        "-noThrow",
        "-backendLocalStorage",
		"-keepSessionSave",
		"-maxFPS", "30")
    if ($Stage -cne "prepare") {
        $nativeArguments += @("-loadSessionSave", $NativeSavePointId)
    }
    $nativeArguments += @(
        "-hstExactCounterattackNativeSourceSelection", "true",
        "-hstExactCounterattackRestartStage", $Stage,
        "-hstExactCounterattackRestartRunId", $RunId,
        "-hstExactCounterattackRestartCut", $script:CutName,
        "-hstExactCounterattackRestartSessionNonce", $SessionNonce,
        "-hstExactCounterattackRestartStageNonce", $StageNonce)
    return $nativeArguments

}

function Assert-EngineOwner {
    param(
        [Parameter(Mandatory = $true)]$Owner,
        [Parameter(Mandatory = $true)][string]$SessionNonce,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)]$ExpectedBuild,
        [Parameter(Mandatory = $true)][string]$ExpectedWorld
    )

    $label = "$($script:CutName) engine-visible owner"
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
            -ArtifactLabel $label
    }
    if ($script:NativeSourceSelection) {
        Assert-JsonProperty `
            -Value $Owner `
            -PropertyName "m_bNativeSourceSelectionProof" `
            -ArtifactLabel $label
        if ($Owner.m_bNativeSourceSelectionProof -isnot [bool] -or
            -not [bool]$Owner.m_bNativeSourceSelectionProof) {
            throw "The $label omitted native source-selection authority."
        }
    }
    elseif ($Owner.PSObject.Properties.Name -contains
            "m_bNativeSourceSelectionProof" -and
        [bool]$Owner.m_bNativeSourceSelectionProof) {
        throw "The $label unexpectedly claims native source-selection authority."
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
        throw "The $label is not exact."
    }
    Assert-BuildIdentity `
        -Artifact $Owner `
        -ExpectedBuild $ExpectedBuild `
        -ArtifactLabel $label
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

    $label = "$($script:CutName) one-use engine stage lease"
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
            -ArtifactLabel $label
    }
    if ($script:NativeSourceSelection) {
        Assert-JsonProperty `
            -Value $Guard `
            -PropertyName "m_bNativeSourceSelectionProof" `
            -ArtifactLabel $label
        if ($Guard.m_bNativeSourceSelectionProof -isnot [bool] -or
            -not [bool]$Guard.m_bNativeSourceSelectionProof) {
            throw "The $label omitted native source-selection authority."
        }
    }
    elseif ($Guard.PSObject.Properties.Name -contains
            "m_bNativeSourceSelectionProof" -and
        [bool]$Guard.m_bNativeSourceSelectionProof) {
        throw "The $label unexpectedly claims native source-selection authority."
    }
    Assert-LowerHexNonce -Nonce $SessionNonce -Label "session nonce"
    Assert-LowerHexNonce -Nonce $StageNonce -Label "$Stage stage nonce"
    $stageOrdinal = @{
        prepare = 0
        recover = 1
        replay = 2
    }[$Stage]
	$ownerReplayReadOnly = $script:IsOwnerAppliedPendingCut -and
		$Stage -ceq "replay"
	$allowCanonicalCampaignOverwrite = -not $ownerReplayReadOnly
	if ($script:NativeSourceSelection) {
		$allowCanonicalCampaignOverwrite = $Stage -ceq "prepare"
	}
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
		[bool]$Guard.m_bAllowCanonicalCampaignOverwrite -ne
			$allowCanonicalCampaignOverwrite) {
        throw "The $label is not exact."
    }
    Assert-BuildIdentity `
        -Artifact $Guard `
        -ExpectedBuild $ExpectedBuild `
        -ArtifactLabel $label
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
        [string]$NativeSavePointId = "",
		[string]$NativeServerConfigPath = "",
        [string]$NativePackedAddonsParent = "",
        [Parameter(Mandatory = $true)]$UnclaimedEngineProcessesObserved
    )

    $stageLabel = "$($script:CutName)/$Stage"
    $ownerReplayReadOnly = $script:IsOwnerAppliedPendingCut -and
        $Stage -ceq "replay"
    $allowCanonicalCampaignOverwrite = -not $ownerReplayReadOnly
    $campaignJournalReadOnly = $ownerReplayReadOnly
    if ($script:NativeSourceSelection) {
        $campaignJournalReadOnly = $Stage -cne "prepare"
        $allowCanonicalCampaignOverwrite = -not $campaignJournalReadOnly
    }
    $canonicalCampaignPath = [IO.Path]::GetFullPath(
        (Join-Path $ProfileRoot "profile\Partisan\HST_CampaignSaveData.json"))
    $recoveryCampaignPath = [IO.Path]::GetFullPath(
        (Join-Path `
            $ProfileRoot `
            "profile\Partisan\HST_CampaignSaveData.recovery.json"))
    $campaignDirectory = Split-Path -Parent $DebugDirectory
    $expectedCanonicalCampaignPath = [IO.Path]::GetFullPath(
        (Join-Path $campaignDirectory "HST_CampaignSaveData.json"))
    $expectedRecoveryCampaignPath = [IO.Path]::GetFullPath(
        (Join-Path `
            $campaignDirectory `
            "HST_CampaignSaveData.recovery.json"))
    if (-not $canonicalCampaignPath.Equals(
            $expectedCanonicalCampaignPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not $recoveryCampaignPath.Equals(
            $expectedRecoveryCampaignPath,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "$stageLabel campaign journal escaped its disposable profile."
    }
    $campaignJournalStateBefore = $null
    $campaignJournalUnchanged = $null
    if ($campaignJournalReadOnly) {
        $campaignJournalStateBefore = Get-CampaignJournalFileState `
            -CanonicalPath $canonicalCampaignPath `
            -RecoveryPath $recoveryCampaignPath
        if ([int]$campaignJournalStateBefore.FileCount -eq 0) {
            throw "$stageLabel campaign journal was unavailable before its read-only stage."
        }
    }
    $resultPath = Join-Path $DebugDirectory (
        "HST_ExactCounterattackRestart_{0}.{1}.json" -f $RunId, $Stage)
    if (Test-Path -LiteralPath $resultPath) {
        throw "$stageLabel result storage was not fresh."
    }

    Assert-LowerHexNonce -Nonce $SessionNonce -Label "session nonce"
    $stageNonce = [Guid]::NewGuid().ToString("N")
    Assert-LowerHexNonce -Nonce $stageNonce -Label "$Stage stage nonce"
    $guardPath = Join-Path $DebugDirectory (
        "HST_ExactCounterattackRestart_{0}.guard.json" -f $RunId)
    if (Test-Path -LiteralPath $guardPath) {
        throw "$stageLabel stage-lease storage was not fresh."
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
		m_bAllowCanonicalCampaignOverwrite = $allowCanonicalCampaignOverwrite
    }
    if ($script:NativeSourceSelection) {
        $engineGuard["m_bNativeSourceSelectionProof"] = $true
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
        -Stage $Stage `
        -NativeSavePointId $NativeSavePointId `
		-NativeServerConfigPath $NativeServerConfigPath `
        -NativePackedAddonsParent $NativePackedAddonsParent)
    $commandLine = (ConvertTo-NativeArgument $ExecutablePath) + " " +
        (($arguments | ForEach-Object {
            ConvertTo-NativeArgument ([string]$_)
        }) -join " ")
    if (-not (Test-ExactNativeArgumentVector `
        -CommandLine $commandLine `
        -ExpectedExecutable $ExecutablePath `
        -ExpectedArguments $arguments)) {
        throw "$stageLabel native arguments did not round-trip exactly."
    }

    $engineBefore = @(Get-EngineProcessRows).Count
    if ($engineBefore -ne 0) {
        throw "An engine process appeared before $stageLabel."
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
    $stageCleanupState = [ordered]@{
        OwnedRemaining = -1
        EngineAfter = -1
    }
    try {
        $job = New-Object PartisanCounterattackGuardedJob
        if (@(Get-EngineProcessRows).Count -ne 0) {
            throw "An engine process appeared during $stageLabel preflight."
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
            throw "$stageLabel did not create its diagnostic process."
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
            throw "$stageLabel exited before command-line verification."
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
            throw "$stageLabel launched with a non-exact argument vector."
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
                throw "An unowned engine process appeared during $stageLabel."
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
                        throw "$stageLabel retained its root process."
                    }
                    $rootExitCode = $process.ExitCode
                }
                if ([int]$rootExitCode -ne 0) {
                    throw "$stageLabel diagnostic process returned failure."
                }
                break
            }
            if ($exitObservedUtc -ne [DateTime]::MinValue -and
                -not $result -and
                ([DateTime]::UtcNow - $exitObservedUtc).TotalSeconds -ge
                    $ResultGraceSeconds) {
                throw "$stageLabel exited without a stable exact result."
            }
            Start-Sleep -Milliseconds $PollMilliseconds
        }

        if (-not $result) {
            throw "$stageLabel exceeded its guarded result deadline."
        }
        if ($null -eq $rootExitCode -or [int]$rootExitCode -ne 0 -or
            (Get-LiveOwnedProcessCount -Owned $ownedProcesses) -ne 0) {
            throw "$stageLabel did not reach a clean successful exit."
        }
        if ($campaignJournalReadOnly) {
            $campaignJournalStateAfter = Get-CampaignJournalFileState `
                -CanonicalPath $canonicalCampaignPath `
                -RecoveryPath $recoveryCampaignPath
            $campaignJournalUnchanged = Test-CampaignJournalFileStateExact `
                -Expected $campaignJournalStateBefore `
                -Actual $campaignJournalStateAfter
            if (-not $campaignJournalUnchanged) {
                throw "$stageLabel changed its read-only campaign journal."
            }
        }
    }
    catch {
        $stageError = $_.Exception.Message
		$stageError += (" | exit {0} | artifacts guard/result {1}/{2}" -f
			$rootExitCode,
			$(Test-Path -LiteralPath $guardPath -PathType Leaf),
			$(Test-Path -LiteralPath $resultPath -PathType Leaf))
		$safeDiagnostic = Get-SafeGuardedEngineDiagnostic `
			-ProfileRoot $ProfileRoot `
			-ProjectDirectory (Split-Path -Parent $ProjectFile) `
			-RuntimeAddonPath $RuntimeAddonPath
		if (-not [string]::IsNullOrWhiteSpace($safeDiagnostic)) {
			$stageError += " | engine " + $safeDiagnostic
		}
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
                $stageCleanupState.OwnedRemaining = Get-LiveOwnedProcessCount `
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
                $stageCleanupState.EngineAfter = @(Get-EngineProcessRows).Count
            }
    }

    $ownedRemaining = [int]$stageCleanupState.OwnedRemaining
    $engineAfter = [int]$stageCleanupState.EngineAfter
    if ($stageCleanupErrors.Count -ne 0 -or
        $ownedRemaining -ne 0 -or $engineAfter -ne 0) {
        $failedCleanupPhases = if ($stageCleanupErrors.Count -eq 0) {
            "none"
        }
        else {
            $stageCleanupErrors -join ","
        }
        throw "$stageLabel process containment cleanup failed " +
            "(phases=$failedCleanupPhases; owned=$ownedRemaining; " +
            "engine=$engineAfter)."
    }
    if ($stageError) {
        throw $stageError
    }
    if (Test-Path -LiteralPath $guardPath) {
        throw "$stageLabel engine did not consume its one-use stage lease."
    }
    $safeSummary = Get-SafeStageSummary `
        -Result $result `
        -ExitCode ([int]$rootExitCode)
    $safeSummary | Add-Member -NotePropertyName EngineBefore -NotePropertyValue $engineBefore
    $safeSummary | Add-Member -NotePropertyName EngineAfter -NotePropertyValue $engineAfter
    $safeSummary | Add-Member `
        -NotePropertyName CanonicalCampaignOverwriteAllowed `
        -NotePropertyValue $allowCanonicalCampaignOverwrite
    $safeSummary | Add-Member `
        -NotePropertyName CanonicalCampaignUnchanged `
        -NotePropertyValue $campaignJournalUnchanged
    $safeSummary | Add-Member `
        -NotePropertyName CampaignJournalOverwriteAllowed `
        -NotePropertyValue $allowCanonicalCampaignOverwrite
    $safeSummary | Add-Member `
        -NotePropertyName CampaignJournalUnchanged `
        -NotePropertyValue $campaignJournalUnchanged
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
$workbenchExecutablePath = ""
$runtimeAddonPath = ""
$projectFile = ""
$profileDirectory = Join-Path $guardRoot "profile\Partisan"
$nativePackProfilePath = Join-Path $guardRoot "profile"
$nativePackedAddonsParent = Join-Path $guardRoot "packed-addons"
$nativePackedAddonPath = Join-Path $nativePackedAddonsParent "Partisan"
$nativeWorkspacePackScratchPath = Join-Path `
    $repositoryRoot `
    $script:WorkspacePackScratchLeaf
$nativeWorkspacePackScratchSentinelPath = Join-Path `
    $nativeWorkspacePackScratchPath `
    $script:WorkspacePackScratchSentinelLeaf
$debugDirectory = Join-Path $profileDirectory "debug"
$canonicalCampaignPath = Join-Path $profileDirectory "HST_CampaignSaveData.json"
$recoveryCampaignPath = Join-Path `
    $profileDirectory `
    "HST_CampaignSaveData.recovery.json"
$guardedTempDirectory = Join-Path $guardRoot "temp"
$guardedWorkingDirectory = Join-Path $guardRoot "working"
$nativeServerConfigPath = Join-Path $guardRoot "native-server-config.json"
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
$nativeSavePointId = ""
$nativeFallbackConflictFingerprint = ""
$nativeCampaignJournalState = $null
$nativePackSummary = $null
$nativeWorkspacePackScratchCreated = $false
$nativeWorkspacePackScratchOwnership = $null

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
	if ($script:NativeSourceSelection -and
		(Split-Path -Leaf $executablePath) -cne
			"ArmaReforgerServerDiag.exe") {
		throw "Native source-selection proof requires the dedicated diagnostic runtime."
	}
    if ($script:NativeSourceSelection) {
        if ([string]::IsNullOrWhiteSpace($WorkbenchExecutable)) {
            throw "Native source-selection proof requires WorkbenchExecutable."
        }
        $workbenchExecutablePath = Resolve-ExistingPath `
            -Path $WorkbenchExecutable `
            -Kind Leaf
        if ((Split-Path -Leaf $workbenchExecutablePath) -cne
            "ArmaReforgerWorkbenchSteamDiag.exe") {
            throw "Native packing requires the diagnostic Steam Workbench executable."
        }
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
    if ($script:NativeSourceSelection) {
        foreach ($nativeResource in @(
            $script:NativeMissionHeader,
            $script:NativeWorldSystemsConfig)) {
            $nativeResourceFile = Join-Path $repositoryRoot (
                $nativeResource.Replace(
                    '/',
                    [IO.Path]::DirectorySeparatorChar))
            [void](Resolve-ExistingPath -Path $nativeResourceFile -Kind Leaf)
        }
    }
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
    if (-not $guardOwnership -or
        $guardOwnership.Nonce -cne $nonce -or
        $guardOwnership.Cut -cne $script:CutName -or
        $guardOwnership.OwnerPid -ne $PID -or
        $wrapperStartUtc -eq [DateTime]::MinValue -or
        $guardOwnership.OwnerStartUtc.Ticks -ne $wrapperStartUtc.Ticks) {
        throw "Guard ownership sentinel validation failed."
    }

    foreach ($directory in @(
        $debugDirectory,
        $guardedTempDirectory,
        $guardedWorkingDirectory)) {
        [void](New-Item -ItemType Directory -Path $directory -Force)
        Assert-NoReparsePathAncestry -Path $directory
    }
	if ($script:NativeSourceSelection) {
		if (-not $PreflightOnly) {
			if (Test-Path -LiteralPath $nativeWorkspacePackScratchPath) {
				throw "Native Workbench pack requires a fresh workspace scratch boundary."
			}
			[void](New-Item `
				-ItemType Directory `
				-Path $nativeWorkspacePackScratchPath)
			$nativeWorkspacePackScratchCreated = $true
			Assert-NoReparsePathAncestry `
				-Path $nativeWorkspacePackScratchPath
			$workspaceScratchRecord = [ordered]@{
				version = $script:SentinelVersion
				purpose = "native_workbench_pack_scratch"
				nonce = $nonce
				ownerPid = $PID
				ownerStartUtc = $wrapperStartUtc.ToString(
					"o",
					[Globalization.CultureInfo]::InvariantCulture)
			}
			Write-JsonUtf8NoBom `
				-Path $nativeWorkspacePackScratchSentinelPath `
				-Value $workspaceScratchRecord
			$nativeWorkspacePackScratchOwnership =
				Read-NativeWorkspacePackScratchOwnership `
					-Directory $nativeWorkspacePackScratchPath `
					-RepositoryRoot $repositoryRoot
			if (-not $nativeWorkspacePackScratchOwnership -or
				$nativeWorkspacePackScratchOwnership.Nonce -cne $nonce -or
				$nativeWorkspacePackScratchOwnership.OwnerPid -ne $PID -or
				$nativeWorkspacePackScratchOwnership.OwnerStartUtc.Ticks -ne
					$wrapperStartUtc.Ticks) {
				throw "Native Workbench pack scratch ownership validation failed."
			}
		}
        foreach ($directory in @(
            $nativePackProfilePath,
            $nativePackedAddonsParent)) {
            [void](New-Item -ItemType Directory -Path $directory -Force)
            Assert-NoReparsePathAncestry -Path $directory
        }
        if (Test-Path -LiteralPath $nativePackedAddonPath) {
            throw "Native packed add-on output was not fresh."
        }
        if ($PreflightOnly) {
            [void](New-Item `
                -ItemType Directory `
                -Path $nativePackedAddonPath)
            Assert-NoReparsePathAncestry -Path $nativePackedAddonPath
        }
        else {
            $nativePackSummary = Invoke-NativeAddonPack `
                -WorkbenchExecutablePath $workbenchExecutablePath `
                -RuntimeAddonPath $runtimeAddonPath `
                -ProjectFile $projectFile `
                -ProfileRoot $guardRoot `
                -PackProfilePath $nativePackProfilePath `
                -GuardedTempDirectory $guardedTempDirectory `
                -PackedAddonsParent $nativePackedAddonsParent `
                -PackedAddonPath $nativePackedAddonPath `
                -TimeoutSeconds $NativePackTimeoutSeconds `
                -UnclaimedEngineProcessesObserved `
                    $unclaimedEngineProcessesObserved
			$nativeWorkspacePackScratchOwnership =
				Read-NativeWorkspacePackScratchOwnership `
					-Directory $nativeWorkspacePackScratchPath `
					-RepositoryRoot $repositoryRoot
			if (-not $nativeWorkspacePackScratchOwnership -or
				$nativeWorkspacePackScratchOwnership.Nonce -cne $nonce -or
				$nativeWorkspacePackScratchOwnership.OwnerPid -ne $PID -or
				$nativeWorkspacePackScratchOwnership.OwnerStartUtc.Ticks -ne
					$wrapperStartUtc.Ticks) {
				throw "Native Workbench pack changed its owned workspace scratch boundary."
			}
            Write-Output ("PACK " + (
                $nativePackSummary | ConvertTo-Json -Compress))
        }

		$nativeServerConfig = [ordered]@{
			game = [ordered]@{
				name = "Partisan native persistence proof"
				password = ""
				passwordAdmin = ""
				scenarioId = $script:NativeScenarioId
				maxPlayers = 1
				visible = $false
				gameProperties = [ordered]@{
					fastValidation = $true
					battlEye = $false
					missionHeader = [ordered]@{
						m_eSaveTypes = 15
					}
				}
				mods = @(
					[ordered]@{
						modId = $script:NativeProjectId
						name = "Partisan"
						required = $true
					}
				)
			}
		}
		Write-JsonUtf8NoBom `
			-Path $nativeServerConfigPath `
			-Value $nativeServerConfig
		$validatedNativeServerConfig = Read-JsonArtifact `
			-Path $nativeServerConfigPath
		$validatedNativeMods = @($validatedNativeServerConfig.game.mods)
		if ([string]$validatedNativeServerConfig.game.scenarioId -cne
				$script:NativeScenarioId -or
			[int]$validatedNativeServerConfig.game.gameProperties.missionHeader.m_eSaveTypes -ne 15 -or
			[bool]$validatedNativeServerConfig.game.visible -or
			$validatedNativeMods.Count -ne 1 -or
			[string]$validatedNativeMods[0].modId -cne
				$script:NativeProjectId -or
			[string]$validatedNativeMods[0].name -cne "Partisan" -or
			$validatedNativeMods[0].required -isnot [bool] -or
			-not [bool]$validatedNativeMods[0].required) {
			throw "Guarded native server config failed its exact persistence gate."
		}
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
    if ($script:NativeSourceSelection) {
        $engineOwner["m_bNativeSourceSelectionProof"] = $true
    }
    Write-JsonUtf8NoBom -Path $engineOwnerPath -Value $engineOwner
    $validatedEngineOwner = Read-JsonArtifact -Path $engineOwnerPath
    Assert-EngineOwner `
        -Owner $validatedEngineOwner `
        -SessionNonce $nonce `
        -RunId $runId `
        -ExpectedBuild $buildIdentity `
        -ExpectedWorld $WorldResource

    $syntheticEmail = "proof" + [char]64 + "example.invalid"
    $syntheticIdentifier = "1" * 17
    $safeSynthetic = ConvertTo-SafeEvidenceLine `
        -Line ($syntheticEmail + " " + $syntheticIdentifier + " " +
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

    $selfTestSettlementId =
        "settlement_operation_self_test_route_failed_survivors"
    $selfTestRefundId = "enemy_resource_refund_" + $selfTestSettlementId
    $selfTestExpectation = [pscustomobject]@{
        m_sOrderId = "order_self_test"
        m_sOperationId = "operation_self_test"
        m_sManifestId = "manifest_self_test"
        m_sManifestHash = "manifest_hash_self_test"
        m_sBatchId = "batch_self_test"
        m_sGroupId = "group_self_test"
        m_sProjectionId = "projection_self_test"
        m_sForceId = "force_self_test"
        m_sFactionKey = "faction_self_test"
        m_sSourceZoneId = "source_self_test"
        m_sTargetZoneId = "target_self_test"
		m_sExpectedSourceOwnerFactionKey = "source_owner_self_test"
		m_iExpectedSourceOwnershipRevision = 3
		m_sExpectedTargetOwnerFactionKey = "target_owner_self_test"
		m_iExpectedTargetOwnershipRevision = 7
        m_sDebitMutationId = "debit_self_test"
        m_sSettlementKind = "route_failed_survivors"
        m_sSettlementId = $selfTestSettlementId
        m_sRefundMutationId = $selfTestRefundId
        m_sReason = "prepared settlement validation self-test"
        m_iAttackCost = 24
        m_iSupportCost = 0
        m_iAccepted = 4
        m_iSurvivors = 3
        m_iAttackRefund = 18
        m_iSupportRefund = 0
        m_iExpectedAttackPool = 494
        m_iExpectedSupportPool = 500
        m_iExpectedPoolRevision = 3
        m_iExpectedPoolOperationalMutationCount = 2
        m_sExpectedLastStrategicMutationId = $selfTestRefundId
        m_iPreparedAtSecond = 10
        m_iExpectedTerminalRevision = 7
    }
    $selfTestCarrier = [pscustomobject]@{
		m_Expectation = $null
        m_iAccepted = 4
        m_iCasualties = 1
        m_iSurvivors = 3
        m_iAttackRefund = 18
        m_iSupportRefund = 0
        m_iAttackBeforeRefund = 476
        m_iSupportBeforeRefund = 500
        m_iPreparedAtSecond = 10
        m_iPrefixRevision = 5
        m_iExpectedPrefixMutationCount = 0
        m_iExpectedPrefixAttackDelta = 0
        m_iExpectedPrefixSupportDelta = 0
        m_bExpectedPrefixReceiptApplied = $false
        m_sSettlementKind = "route_failed_survivors"
        m_sSettlementId = $selfTestSettlementId
        m_sRefundMutationId = $selfTestRefundId
        m_sReason = "prepared settlement validation self-test"
        m_SettlementExpectation = $selfTestExpectation
        m_sPreparedSettlementFingerprint = "prepared_self_test"
        m_sPreparedSemanticFingerprint = "prepared_self_test"
        m_sRawPreparedCutSemanticFingerprint = "prepared_self_test"
        m_iExpectedPhysicalAdapterHandleCount = 0
        m_iExpectedPhysicalRuntimeMemberCount = 0
    }
    Assert-PreparedSettlementCarrier `
        -Carrier $selfTestCarrier `
        -Label "prepared settlement carrier self-test" `
        -CutName "prepared_before_refund"
    $tamperedCarrier = $selfTestCarrier |
        ConvertTo-Json -Compress -Depth 8 | ConvertFrom-Json
    $tamperedCarrier.m_bExpectedPrefixReceiptApplied = $true
    $carrierRejected = $false
    try {
        Assert-PreparedSettlementCarrier `
            -Carrier $tamperedCarrier `
            -Label "tampered prepared settlement carrier self-test" `
            -CutName "prepared_before_refund"
    }
    catch {
        $carrierRejected = $true
    }
    if (-not $carrierRejected) {
        throw "Prepared-settlement carrier negative self-test failed."
    }

	$endpointTamperedCarrier = $selfTestCarrier |
		ConvertTo-Json -Compress -Depth 8 | ConvertFrom-Json
	$endpointTamperedCarrier.m_SettlementExpectation.m_iExpectedSourceOwnershipRevision = 0
	$endpointCarrierRejected = $false
	try {
		Assert-PreparedSettlementCarrier `
			-Carrier $endpointTamperedCarrier `
			-Label "endpoint-tampered prepared settlement carrier self-test" `
			-CutName "prepared_before_refund"
	}
	catch {
		$endpointCarrierRejected = $true
	}
	if (-not $endpointCarrierRejected) {
		throw "Prepared-settlement endpoint carrier negative self-test failed."
	}

    $mixedCarrier = $selfTestCarrier |
        ConvertTo-Json -Compress -Depth 8 | ConvertFrom-Json
    $mixedCarrier.m_Expectation = [pscustomobject]@{
        m_sOrderId = "forged_movement_order"
    }
    $mixedCarrierRejected = $false
    try {
        Assert-PreparedSettlementCarrier `
            -Carrier $mixedCarrier `
            -Label "mixed-family prepared settlement carrier self-test" `
            -CutName "prepared_before_refund"
    }
    catch {
        $mixedCarrierRejected = $true
    }
    if (-not $mixedCarrierRejected) {
        throw "Mixed-family prepared-settlement carrier negative self-test failed."
    }

    $selfTestRecovery = [pscustomobject]@{
        m_bPreparedCutExact = $true
        m_bCasualtyContinuityExact = $true
        m_iPhysicalAdapterHandleCount = 0
        m_iPhysicalRuntimeMemberCount = 0
        m_bPhysicalBindingsExact = $false
        m_bLivePositionRefreshExact = $false
        m_bPhysicalCaptureNormalizedExact = $false
        m_bRestored = $true
        m_bStartupReconcileChanged = $true
        m_bContinuationExact = $true
        m_bSameStateSemanticNoOp = $true
        m_sSourceSemanticFingerprint = "prepared_self_test"
        m_sFinalSemanticFingerprint = "terminal_self_test"
        m_sRawPreparedCutSemanticFingerprint = "prepared_self_test"
    }
    Assert-PreparedSettlementStageSemantics `
        -Result $selfTestRecovery `
        -Stage "recover" `
        -Label "prepared settlement recovery self-test"
    $tamperedRecovery = $selfTestRecovery |
        ConvertTo-Json -Compress -Depth 4 | ConvertFrom-Json
    $tamperedRecovery.m_bStartupReconcileChanged = $false
    $recoveryRejected = $false
    try {
        Assert-PreparedSettlementStageSemantics `
            -Result $tamperedRecovery `
            -Stage "recover" `
            -Label "tampered prepared settlement recovery self-test"
    }
    catch {
        $recoveryRejected = $true
    }
    if (-not $recoveryRejected) {
        throw "Prepared-settlement recovery negative self-test failed."
    }

    $selfTestOwnerExpectation = [pscustomobject]@{
        m_sOrderId = "owner_order_self_test"
        m_sOperationId = "owner_operation_self_test"
        m_sManifestId = "owner_manifest_self_test"
        m_sManifestHash = "owner_manifest_hash_self_test"
        m_sBatchId = "owner_batch_self_test"
        m_sGroupId = "owner_group_self_test"
        m_sProjectionId = "owner_projection_self_test"
        m_sForceId = "owner_force_self_test"
        m_sFactionKey = "owner_faction_self_test"
        m_sSourceZoneId = "owner_source_self_test"
        m_sTargetZoneId = "owner_target_self_test"
        m_sExpectedSourceOwnerFactionKey = "owner_source_faction_self_test"
        m_iExpectedSourceOwnershipRevision = 3
        m_sExpectedTargetOwnerFactionKey = "owner_faction_self_test"
        m_iExpectedTargetOwnershipRevision = 8
        m_sDebitMutationId = "owner_debit_self_test"
        m_iAttackCost = 24
        m_iSupportCost = 0
        m_iExpectedAttackPool = 476
        m_iExpectedSupportPool = 500
        m_iExpectedPoolOperationalMutationCount = 1
        m_iAcceptedMemberCount = 4
        m_iLivingMemberCount = 4
        m_sLivingSlotFingerprint = "owner_slot_0,owner_slot_1,owner_slot_2,owner_slot_3"
        m_bExpectedLivingSlotsEverAlive = $false
        m_iExpectedNormalizedSlotAttemptCount = 0
        m_sConfirmedCasualtySlotId = ""
        m_sCasualtyTombstoneFingerprint = ""
        m_iExpectedNormalizedReprojectionCount = 0
    }
    $selfTestOwnerCarrier = [pscustomobject]@{
        m_Expectation = $selfTestOwnerExpectation
        m_SettlementExpectation = $null
        m_iPreparedElapsedSecond = 12
        m_fPreparedRouteProgressMeters = 900.0
        m_fPreparedRouteTotalDistanceMeters = 900.0
        m_vPreparedStrategicPosition = "900 0 450"
        m_iExpectedPhysicalAdapterHandleCount = 0
        m_iExpectedPhysicalRuntimeMemberCount = 0
        m_sPreparedSemanticFingerprint = "owner_pending_self_test"
        m_sRawPreparedCutSemanticFingerprint = "owner_raw_pending_self_test"
    }
    Assert-OwnerAppliedPendingCarrier `
        -Carrier $selfTestOwnerCarrier `
        -Label "owner-applied pending carrier self-test"
    $collapsedOwnerCarrier = $selfTestOwnerCarrier |
        ConvertTo-Json -Compress -Depth 8 | ConvertFrom-Json
    $collapsedOwnerCarrier.m_sRawPreparedCutSemanticFingerprint =
        $collapsedOwnerCarrier.m_sPreparedSemanticFingerprint
    $collapsedOwnerCarrierRejected = $false
    try {
        Assert-OwnerAppliedPendingCarrier `
            -Carrier $collapsedOwnerCarrier `
            -Label "collapsed owner-applied pending carrier self-test"
    }
    catch {
        $collapsedOwnerCarrierRejected = $true
    }
    if (-not $collapsedOwnerCarrierRejected) {
        throw "Owner-applied pending equal-fingerprint carrier negative self-test failed."
    }
    $tamperedOwnerCarrier = $selfTestOwnerCarrier |
        ConvertTo-Json -Compress -Depth 8 | ConvertFrom-Json
    $tamperedOwnerCarrier.m_fPreparedRouteProgressMeters = 899.0
    $ownerCarrierRejected = $false
    try {
        Assert-OwnerAppliedPendingCarrier `
            -Carrier $tamperedOwnerCarrier `
            -Label "tampered owner-applied pending carrier self-test"
    }
    catch {
        $ownerCarrierRejected = $true
    }
    if (-not $ownerCarrierRejected) {
        throw "Owner-applied pending carrier negative self-test failed."
    }

    $mixedOwnerCarrier = $selfTestOwnerCarrier |
        ConvertTo-Json -Compress -Depth 8 | ConvertFrom-Json
    $mixedOwnerCarrier.m_SettlementExpectation = [pscustomobject]@{
        m_sSettlementId = "forged_owner_settlement"
    }
    $mixedOwnerCarrierRejected = $false
    try {
        Assert-OwnerAppliedPendingCarrier `
            -Carrier $mixedOwnerCarrier `
            -Label "mixed-family owner-applied pending carrier self-test"
    }
    catch {
        $mixedOwnerCarrierRejected = $true
    }
    if (-not $mixedOwnerCarrierRejected) {
        throw "Mixed-family owner-applied pending carrier negative self-test failed."
    }

    $selfTestOwnerPrepare = [pscustomobject]@{
        m_bPreparedCutExact = $true
        m_bCasualtyContinuityExact = $true
        m_iPhysicalAdapterHandleCount = 0
        m_iPhysicalRuntimeMemberCount = 0
        m_bPhysicalBindingsExact = $false
        m_bLivePositionRefreshExact = $false
        m_bPhysicalCaptureNormalizedExact = $false
        m_bRestored = $false
        m_bStartupReconcileChanged = $false
        m_bOwnershipStartupReconcileChanged = $false
        m_bContinuationExact = $false
        m_bSameStateSemanticNoOp = $false
        m_sSourceSemanticFingerprint = "owner_pending_self_test"
        m_sFinalSemanticFingerprint = "owner_pending_self_test"
        m_sRawPreparedCutSemanticFingerprint = "owner_raw_pending_self_test"
    }
    Assert-OwnerAppliedPendingStageSemantics `
        -Result $selfTestOwnerPrepare `
        -Stage "prepare" `
        -Label "owner-applied pending prepare self-test"
    $collapsedOwnerPrepare = $selfTestOwnerPrepare |
        ConvertTo-Json -Compress -Depth 4 | ConvertFrom-Json
    $collapsedOwnerPrepare.m_sRawPreparedCutSemanticFingerprint =
        $collapsedOwnerPrepare.m_sSourceSemanticFingerprint
    $collapsedOwnerPrepareRejected = $false
    try {
        Assert-OwnerAppliedPendingStageSemantics `
            -Result $collapsedOwnerPrepare `
            -Stage "prepare" `
            -Label "collapsed owner-applied pending prepare self-test"
    }
    catch {
        $collapsedOwnerPrepareRejected = $true
    }
    if (-not $collapsedOwnerPrepareRejected) {
        throw "Owner-applied pending equal-fingerprint result negative self-test failed."
    }
    $tamperedOwnerPrepare = $selfTestOwnerPrepare |
        ConvertTo-Json -Compress -Depth 4 | ConvertFrom-Json
    $tamperedOwnerPrepare.m_bStartupReconcileChanged = $true
    $ownerPrepareRejected = $false
    try {
        Assert-OwnerAppliedPendingStageSemantics `
            -Result $tamperedOwnerPrepare `
            -Stage "prepare" `
            -Label "tampered owner-applied pending prepare self-test"
    }
    catch {
        $ownerPrepareRejected = $true
    }
    if (-not $ownerPrepareRejected) {
        throw "Owner-applied pending prepare negative self-test failed."
    }

    $selfTestOwnerRecovery = $selfTestOwnerPrepare |
        ConvertTo-Json -Compress -Depth 4 | ConvertFrom-Json
    $selfTestOwnerRecovery.m_bRestored = $true
    $selfTestOwnerRecovery.m_bStartupReconcileChanged = $true
    $selfTestOwnerRecovery.m_bOwnershipStartupReconcileChanged = $true
    $selfTestOwnerRecovery.m_bContinuationExact = $true
    $selfTestOwnerRecovery.m_bSameStateSemanticNoOp = $true
    $selfTestOwnerRecovery.m_sFinalSemanticFingerprint = "owner_returning_self_test"
    Assert-OwnerAppliedPendingStageSemantics `
        -Result $selfTestOwnerRecovery `
        -Stage "recover" `
        -Label "owner-applied pending recovery self-test"
    $tamperedOwnerRecovery = $selfTestOwnerRecovery |
        ConvertTo-Json -Compress -Depth 4 | ConvertFrom-Json
    $tamperedOwnerRecovery.m_bOwnershipStartupReconcileChanged = $false
    $ownerRecoveryRejected = $false
    try {
        Assert-OwnerAppliedPendingStageSemantics `
            -Result $tamperedOwnerRecovery `
            -Stage "recover" `
            -Label "tampered owner-applied pending recovery self-test"
    }
    catch {
        $ownerRecoveryRejected = $true
    }
    if (-not $ownerRecoveryRejected) {
        throw "Owner-applied pending recovery negative self-test failed."
    }

    $selfTestOwnerReplay = $selfTestOwnerRecovery |
        ConvertTo-Json -Compress -Depth 4 | ConvertFrom-Json
    $selfTestOwnerReplay.m_bOwnershipStartupReconcileChanged = $false
    $selfTestOwnerReplay.m_bContinuationExact = $false
    $selfTestOwnerReplay.m_sSourceSemanticFingerprint = "owner_returning_self_test"
    $selfTestOwnerReplay.m_sFinalSemanticFingerprint = "owner_returning_self_test"
    Assert-OwnerAppliedPendingStageSemantics `
        -Result $selfTestOwnerReplay `
        -Stage "replay" `
        -Label "owner-applied pending replay self-test"
    $tamperedOwnerReplay = $selfTestOwnerReplay |
        ConvertTo-Json -Compress -Depth 4 | ConvertFrom-Json
    $tamperedOwnerReplay.m_bOwnershipStartupReconcileChanged = $true
    $ownerReplayRejected = $false
    try {
        Assert-OwnerAppliedPendingStageSemantics `
            -Result $tamperedOwnerReplay `
            -Stage "replay" `
            -Label "tampered owner-applied pending replay self-test"
    }
    catch {
        $ownerReplayRejected = $true
    }
    if (-not $ownerReplayRejected) {
        throw "Owner-applied pending replay negative self-test failed."
    }

    $foreignOwnershipStartup = [pscustomobject]@{
        m_bSuccess = $true
        m_bOwnershipStartupReconcileChanged = $true
    }
    $foreignOwnershipStartupRejected = $false
    try {
        Assert-OwnershipStartupScope `
            -Result $foreignOwnershipStartup `
            -Label "non-owner ownership-startup self-test" `
            -OwnerAppliedPendingCut $false
    }
    catch {
        $foreignOwnershipStartupRejected = $true
    }
    if (-not $foreignOwnershipStartupRejected) {
        throw "Non-owner ownership-startup scope negative self-test failed."
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
        -Stage "prepare" `
		-NativeServerConfigPath $nativeServerConfigPath `
        -NativePackedAddonsParent $nativePackedAddonsParent)
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
    if ($script:NativeSourceSelection) {
        $packPreflightArguments = @(Get-NativePackArgumentVector `
            -ProjectFile $projectFile `
            -RuntimeAddonPath $runtimeAddonPath `
            -PackProfilePath $nativePackProfilePath `
            -PackedAddonPath $nativePackedAddonPath)
        $packPreflightCommandLine =
            (ConvertTo-NativeArgument $workbenchExecutablePath) + " " +
            (($packPreflightArguments | ForEach-Object {
                ConvertTo-NativeArgument ([string]$_)
            }) -join " ")
        if (-not (Test-ExactNativeArgumentVector `
            -CommandLine $packPreflightCommandLine `
            -ExpectedExecutable $workbenchExecutablePath `
            -ExpectedArguments $packPreflightArguments)) {
            throw "Native Workbench pack argument isolation failed."
        }
        foreach ($requiredPackToken in @(
            "-wbModule=ResourceManager",
            "-packAddon",
            "-packAddonDir")) {
            if (@($packPreflightArguments | Where-Object {
                [string]$_ -ceq $requiredPackToken
            }).Count -ne 1) {
                throw "Native Workbench pack argument contract is incomplete."
            }
        }

        foreach ($requiredToken in @(
			"-config",
            "-backendLocalStorage",
            "-keepSessionSave",
            "-hstExactCounterattackNativeSourceSelection")) {
            if (@($preflightArguments | Where-Object {
                [string]$_ -ceq $requiredToken
            }).Count -ne 1) {
                throw "Native prepare argument contract omitted an exact token."
            }
        }
        foreach ($forbiddenToken in @(
            "-loadSessionSave",
            "-addons",
            "-forceupdate")) {
            if (@($preflightArguments | Where-Object {
                [string]$_ -ceq $forbiddenToken
            }).Count -ne 0) {
                throw "Native prepare argument contract included a forbidden token."
            }
        }
        $nativeAddonRoot = $runtimeAddonPath + "," +
            [IO.Path]::GetFullPath($nativePackedAddonsParent)
        $nativeBaseProject = Join-Path `
            $runtimeAddonPath `
            "data\ArmaReforger.gproj"
        $addonsDirIndex = [Array]::IndexOf(
            [object[]]$preflightArguments,
            "-addonsDir")
        $projectIndex = [Array]::IndexOf(
            [object[]]$preflightArguments,
            "-gproj")
        $configIndex = [Array]::IndexOf(
            [object[]]$preflightArguments,
            "-config")
        if ($addonsDirIndex -lt 0 -or $projectIndex -lt 0 -or
            $configIndex -lt 0 -or
            [string]$preflightArguments[$addonsDirIndex + 1] -cne
                $nativeAddonRoot -or
            [string]$preflightArguments[$projectIndex + 1] -cne
                $nativeBaseProject -or
            [string]$preflightArguments[$configIndex + 1] -cne
                $nativeServerConfigPath) {
            throw "Native server argument contract lost its exact packed add-on inputs."
        }

        $selfTestSavePointId = "5c146b14-3c52-8afd-938a-375d0df1fbf6"
        Assert-NativeSavePointId `
            -SavePointId $selfTestSavePointId `
            -Label "native argument self-test save point id"
        $nativeRecoveryArguments = @(Get-StageArgumentVector `
            -RuntimeAddonPath $runtimeAddonPath `
            -ProjectFile $projectFile `
            -World $WorldResource `
            -ProfileRoot $guardRoot `
            -SessionNonce $nonce `
            -StageNonce $preflightStageNonce `
            -RunId $runId `
            -Stage "recover" `
            -NativeSavePointId $selfTestSavePointId `
			-NativeServerConfigPath $nativeServerConfigPath `
            -NativePackedAddonsParent $nativePackedAddonsParent)
        $nativeRecoveryCommandLine =
            (ConvertTo-NativeArgument $executablePath) + " " +
            (($nativeRecoveryArguments | ForEach-Object {
                ConvertTo-NativeArgument ([string]$_)
            }) -join " ")
        if (-not (Test-ExactNativeArgumentVector `
            -CommandLine $nativeRecoveryCommandLine `
            -ExpectedExecutable $executablePath `
            -ExpectedArguments $nativeRecoveryArguments)) {
            throw "Native recovery argument isolation failed."
        }
        $loadTokenIndex = [Array]::IndexOf(
            [object[]]$nativeRecoveryArguments,
            "-loadSessionSave")
        if ($loadTokenIndex -lt 0 -or
            $loadTokenIndex + 1 -ge $nativeRecoveryArguments.Count -or
            [string]$nativeRecoveryArguments[$loadTokenIndex + 1] -cne
                $selfTestSavePointId) {
            throw "Native recovery argument contract lost its exact save UUID."
        }
    }

    if ($PreflightOnly) {
        $preflightSummary = [pscustomobject]@{
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
        }
        if ($script:NativeSourceSelection) {
            $preflightSummary | Add-Member `
                -NotePropertyName NativeSourceSelection `
                -NotePropertyValue $true
            $preflightSummary | Add-Member `
                -NotePropertyName WorkbenchPackExecuted `
                -NotePropertyValue $false
            $preflightSummary | Add-Member `
                -NotePropertyName PackedAddonPlaceholder `
                -NotePropertyValue $true
            $preflightSummary | Add-Member `
                -NotePropertyName PackArgumentTokenCount `
                -NotePropertyValue $packPreflightArguments.Count
        }
        Write-Output ("PREFLIGHT " + (
            $preflightSummary | ConvertTo-Json -Compress))
        $runSucceeded = $true
    }
    else {
        if ($script:NativeSourceSelection -and
            (-not $nativePackSummary -or
                [int]$nativePackSummary.ExitCode -ne 0 -or
                [int]$nativePackSummary.EngineAfter -ne 0 -or
                [int]$nativePackSummary.OwnedProcessesRemaining -ne 0 -or
                [int]$nativePackSummary.PakCount -ne 1 -or
                [int]$nativePackSummary.ProjectCount -ne 1 -or
                [int]$nativePackSummary.ResourceDatabaseCount -ne 1)) {
            throw "Native proof did not retain an exact guarded Workbench pack."
        }
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
			-NativeServerConfigPath $nativeServerConfigPath `
            -NativePackedAddonsParent $nativePackedAddonsParent `
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
                $preparedFingerprint -or
            [string]$prepare.Result.m_sRawPreparedCutSemanticFingerprint -cne
                [string]$carrier.m_sRawPreparedCutSemanticFingerprint) {
            throw "The prepare result and durable carrier fingerprint chain diverged."
        }
        if ($script:NativeSourceSelection) {
            $nativeSavePointId = [string]$carrier.m_sNativeSavePointId
            $nativeFallbackConflictFingerprint =
                [string]$carrier.m_sFallbackConflictSemanticFingerprint
            if ([string]$prepare.Result.m_sNativeSavePointId -cne
                    $nativeSavePointId -or
                [string]$prepare.Result.m_sFallbackConflictSemanticFingerprint -cne
                    $nativeFallbackConflictFingerprint -or
                -not [bool]$prepare.SafeSummary.CanonicalCampaignOverwriteAllowed) {
                throw "Native prepare result and carrier evidence diverged."
            }
            $nativeCampaignJournalState = Get-CampaignJournalFileState `
                -CanonicalPath $canonicalCampaignPath `
                -RecoveryPath $recoveryCampaignPath
            if ([int]$nativeCampaignJournalState.FileCount -ne 2 -or
                -not [bool]$nativeCampaignJournalState.CanonicalPresent -or
                -not [bool]$nativeCampaignJournalState.RecoveryPresent) {
                throw "Native prepare did not create its complete two-generation fallback journal."
            }
        }
        if ($script:CutName -ceq "physical_live_position") {
            $carrierStale = $carrier.m_vInjectedStalePosition |
                ConvertTo-Json -Compress -Depth 4
            $carrierLive = $carrier.m_vPreparedLivePosition |
                ConvertTo-Json -Compress -Depth 4
            $resultStale = $prepare.Result.m_vInjectedStalePosition |
                ConvertTo-Json -Compress -Depth 4
            $resultLive = $prepare.Result.m_vPreparedLivePosition |
                ConvertTo-Json -Compress -Depth 4
            if ([int]$prepare.Result.m_iPhysicalAdapterHandleCount -ne
                    [int]$carrier.m_iExpectedPhysicalAdapterHandleCount -or
                [int]$prepare.Result.m_iPhysicalRuntimeMemberCount -ne
                    [int]$carrier.m_iExpectedPhysicalRuntimeMemberCount -or
                $resultStale -cne $carrierStale -or
                $resultLive -cne $carrierLive) {
                throw "The physical prepare result and carrier authority evidence diverged."
            }
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
            -NativeSavePointId $nativeSavePointId `
			-NativeServerConfigPath $nativeServerConfigPath `
            -NativePackedAddonsParent $nativePackedAddonsParent `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($recover)
        Write-Output ("STAGE " + (
            $recover.SafeSummary | ConvertTo-Json -Compress))
        if ([string]$recover.Result.m_sSourceSemanticFingerprint -cne
                $preparedFingerprint -or
            [string]$recover.Result.m_sFinalSemanticFingerprint -ceq
                $preparedFingerprint -or
            [string]$recover.Result.m_sRawPreparedCutSemanticFingerprint -cne
                [string]$carrier.m_sRawPreparedCutSemanticFingerprint) {
            throw "The recover result did not continue the prepared fingerprint exactly once."
        }
        if ($script:NativeSourceSelection) {
            $nativeRecoveredJournalState = Get-CampaignJournalFileState `
                -CanonicalPath $canonicalCampaignPath `
                -RecoveryPath $recoveryCampaignPath
            if ([bool]$recover.SafeSummary.CanonicalCampaignOverwriteAllowed -or
                -not [bool]$recover.SafeSummary.CanonicalCampaignUnchanged -or
                -not [bool]$recover.SafeSummary.CampaignJournalUnchanged -or
                -not (Test-CampaignJournalFileStateExact `
                    -Expected $nativeCampaignJournalState `
                    -Actual $nativeRecoveredJournalState)) {
                throw "Native recovery did not preserve the complete conflicting fallback journal."
            }
            $recoveredCarrier = Wait-StableJsonArtifact `
                -Path $carrierPath `
                -DeadlineUtc ([DateTime]::UtcNow.AddSeconds(
                    $ResultGraceSeconds))
            $recoveredCarrier = Assert-PreparedCarrier `
                -Carrier $recoveredCarrier `
                -SessionNonce $nonce `
                -RunId $runId `
                -ExpectedBuild $buildIdentity
            if ([string]$recoveredCarrier.m_sPreparedSemanticFingerprint -cne
                    $preparedFingerprint -or
                [string]$recoveredCarrier.m_sRawPreparedCutSemanticFingerprint -cne
                    [string]$carrier.m_sRawPreparedCutSemanticFingerprint -or
                [string]$recoveredCarrier.m_sFallbackConflictSemanticFingerprint -cne
                    $nativeFallbackConflictFingerprint -or
                [string]$recover.Result.m_sFallbackConflictSemanticFingerprint -cne
                    $nativeFallbackConflictFingerprint -or
                [string]$recover.Result.m_sNativeSavePointId -cne
                    [string]$recoveredCarrier.m_sNativeSavePointId) {
                throw "Native recovery result and refreshed carrier evidence diverged."
            }
            $nativeSavePointId =
                [string]$recoveredCarrier.m_sNativeSavePointId
            Assert-NativeSavePointId `
                -SavePointId $nativeSavePointId `
                -Label "native recovery save point id"
            $carrier = $recoveredCarrier
            $nativeCarrierSignatureAfterRecover = Get-FileSignature `
                -Path $carrierPath
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
            -NativeSavePointId $nativeSavePointId `
			-NativeServerConfigPath $nativeServerConfigPath `
            -NativePackedAddonsParent $nativePackedAddonsParent `
            -UnclaimedEngineProcessesObserved $unclaimedEngineProcessesObserved
        $stageOutcomes.Add($replay)
        Write-Output ("STAGE " + (
            $replay.SafeSummary | ConvertTo-Json -Compress))
		if ($script:IsOwnerAppliedPendingCut -and
			([bool]$replay.SafeSummary.CampaignJournalOverwriteAllowed -or
				-not [bool]$replay.SafeSummary.CampaignJournalUnchanged)) {
            throw "Owner-applied pending replay did not preserve its read-only campaign journal."
        }
        $nativeReplayJournalExact = $true
        if ($script:NativeSourceSelection) {
            $nativeReplayJournalState = Get-CampaignJournalFileState `
                -CanonicalPath $canonicalCampaignPath `
                -RecoveryPath $recoveryCampaignPath
            $nativeReplayJournalExact = Test-CampaignJournalFileStateExact `
                -Expected $nativeCampaignJournalState `
                -Actual $nativeReplayJournalState
        }
        if ($script:NativeSourceSelection -and
            ([string]$replay.Result.m_sNativeSavePointId -cne
                    $nativeSavePointId -or
                [string]$replay.Result.m_sFallbackConflictSemanticFingerprint -cne
                    $nativeFallbackConflictFingerprint -or
                -not $nativeReplayJournalExact -or
                (Get-FileSignature -Path $carrierPath) -cne
                    $nativeCarrierSignatureAfterRecover)) {
            throw "Native replay did not preserve its exact save, fallback journal, and carrier evidence."
        }
        $recoveredFingerprint = [string]$recover.Result.m_sFinalSemanticFingerprint
        if ([string]$replay.Result.m_sSourceSemanticFingerprint -cne
                $recoveredFingerprint -or
            [string]$replay.Result.m_sFinalSemanticFingerprint -cne
                $recoveredFingerprint -or
            [string]$replay.Result.m_sRawPreparedCutSemanticFingerprint -cne
                [string]$carrier.m_sRawPreparedCutSemanticFingerprint) {
            throw "The replay result was not an exact semantic no-op."
        }

        $resultSummary = [pscustomobject]@{
            Valid = $true
            Cut = $script:CutName
            StageCount = $stageOutcomes.Count
            Build = $buildIdentity.BuildSha.Substring(0, 12)
            Schema = $buildIdentity.CampaignSchemaVersion
            FingerprintChainExact = $true
            AllExitZero = @($stageOutcomes | Where-Object {
                $_.ExitCode -ne 0
            }).Count -eq 0
        }
        if ($script:NativeSourceSelection) {
            $resultSummary | Add-Member `
                -NotePropertyName NativeSourceSelectionExact `
                -NotePropertyValue $true
        }
        Write-Output ("RESULT " + (
            $resultSummary | ConvertTo-Json -Compress))
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
        WorkspacePackScratchRemaining = -1
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
        -Name "remove-owned-workspace-pack-scratch" `
        -Errors $cleanupPhaseErrors `
        -Action {
            if ($nativeWorkspacePackScratchCreated) {
                $candidateScratchOwnership =
                    Read-NativeWorkspacePackScratchOwnership `
                        -Directory $nativeWorkspacePackScratchPath `
                        -RepositoryRoot $repositoryRoot
                if ($candidateScratchOwnership -and
                    $candidateScratchOwnership.Nonce -ceq $nonce -and
                    $candidateScratchOwnership.OwnerPid -eq $PID -and
                    $wrapperStartUtc -ne [DateTime]::MinValue -and
                    $candidateScratchOwnership.OwnerStartUtc.Ticks -eq
                        $wrapperStartUtc.Ticks) {
                    $nativeWorkspacePackScratchOwnership =
                        $candidateScratchOwnership
                }
                else {
                    $nativeWorkspacePackScratchOwnership = $null
                }
                if (-not $nativeWorkspacePackScratchOwnership -or
                    -not (Remove-NativeWorkspacePackScratch `
                        -Ownership $nativeWorkspacePackScratchOwnership `
                        -RepositoryRoot $repositoryRoot)) {
                    throw "Owned native Workbench workspace scratch removal failed."
                }
            }
        }
    Invoke-IsolatedCleanupPhase `
        -Name "remove-exact-owned-guard" `
        -Errors $cleanupPhaseErrors `
        -Action {
            Assert-NoReparsePathAncestry -Path $guardBase
            if ($guardDirectoryCreated -and
                (Test-Path -LiteralPath $guardRoot -PathType Container)) {
                $candidateOwnership = Read-GuardOwnership `
                    -Directory $guardRoot `
                    -GuardBase $guardBase
                if ($candidateOwnership -and
                    $candidateOwnership.Nonce -ceq $nonce -and
                    $candidateOwnership.Cut -ceq $script:CutName -and
                    $candidateOwnership.OwnerPid -eq $PID -and
                    $wrapperStartUtc -ne [DateTime]::MinValue -and
                    $candidateOwnership.OwnerStartUtc.Ticks -eq
                        $wrapperStartUtc.Ticks) {
                    $guardOwnership = $candidateOwnership
                }
                else {
                    $guardOwnership = $null
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
            $cleanupState.WorkspacePackScratchRemaining = [int](
                $nativeWorkspacePackScratchCreated -and
                (Test-Path -LiteralPath $nativeWorkspacePackScratchPath))
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
        WorkspacePackScratchRemaining =
            $cleanupState.WorkspacePackScratchRemaining
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
    $cleanupResult.WorkspacePackScratchRemaining -eq 0 -and
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

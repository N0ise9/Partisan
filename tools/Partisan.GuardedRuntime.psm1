Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:GuardMagic = 'partisan_guarded_runtime_owner_v1'
$script:ContextMagic = 'partisan_guarded_runtime_context_v1'
$script:StageMagic = 'partisan_guarded_runtime_stage_v1'
$script:GuardLeafPrefix = 'PartisanGuardedRuntime_'
$script:GuardSentinelLeaf = '.partisan-guarded-runtime-owner.json'
$script:CandidateBindingLeaf = '.partisan-sealed-candidate.json'
$script:PermanentFailureLeaf = '.partisan-permanent-no-go.json'
$script:RuntimeV2Magic = 'partisan_guarded_runtime_authoritative_v2'
$script:RuntimeReceiptMagic = 'partisan_guarded_runtime_clean_receipt_v2'
$script:RuntimeJournalMagic = 'partisan_guarded_runtime_external_journal_v2'
$script:RuntimeAttestationMagic = 'partisan_guarded_runtime_completion_attestation_v1'
$script:MaximumPermanentFailures = 64
$script:MaximumPermanentFailureMessageChars = 512
$script:ModuleFilePath = [IO.Path]::GetFullPath($PSCommandPath)
$script:EngineProcessNames = @(
    'ArmaReforger',
    'ArmaReforger_BE',
    'ArmaReforgerSteam',
    'ArmaReforgerSteamDiag',
    'ArmaReforgerServer',
    'ArmaReforgerServerDiag',
    'ArmaReforgerWorkbench',
    'ArmaReforgerWorkbenchDiag',
    'ArmaReforgerWorkbenchSteamDiag',
    'CrashReporter',
    'CrashReportClient'
)

if (-not ('Partisan.GuardedRuntime.RegistryHub' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;

namespace Partisan.GuardedRuntime
{
    public static class RegistryHub
    {
        public static readonly Dictionary<string, object> RuntimeRegistry =
            new Dictionary<string, object>(StringComparer.Ordinal);
        public static readonly Dictionary<string, object> RegisteredGuards =
            new Dictionary<string, object>(StringComparer.OrdinalIgnoreCase);
    }
}
'@
}
$script:RuntimeRegistry =
    [Partisan.GuardedRuntime.RegistryHub]::RuntimeRegistry
$script:RegisteredGuards =
    [Partisan.GuardedRuntime.RegistryHub]::RegisteredGuards

if (-not ('Partisan.GuardedRuntime.NativeJob' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace Partisan.GuardedRuntime
{
    public sealed class NativeJob : IDisposable
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

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr CreateJobObject(IntPtr attributes, string name);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool SetInformationJobObject(
            IntPtr job,
            Int32 informationClass,
            IntPtr information,
            UInt32 informationLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool QueryInformationJobObject(
            IntPtr job,
            Int32 informationClass,
            IntPtr information,
            UInt32 informationLength,
            out UInt32 returnLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr handle);

        public NativeJob()
        {
            _handle = CreateJobObject(IntPtr.Zero, null);
            if (_handle == IntPtr.Zero)
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to create a guarded process job.");

            try
            {
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
                    new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
                info.BasicLimitInformation.LimitFlags = 0x00002000;
                Int32 size = Marshal.SizeOf(
                    typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
                IntPtr pointer = Marshal.AllocHGlobal(size);
                try
                {
                    Marshal.StructureToPtr(info, pointer, false);
                    if (!SetInformationJobObject(
                            _handle, 9, pointer, (UInt32)size))
                        throw new Win32Exception(
                            Marshal.GetLastWin32Error(),
                            "Unable to configure kill-on-close process ownership.");
                }
                finally
                {
                    Marshal.FreeHGlobal(pointer);
                }
            }
            catch
            {
                CloseHandle(_handle);
                _handle = IntPtr.Zero;
                throw;
            }
        }

        public void Add(Process process)
        {
            if (process == null)
                throw new ArgumentNullException("process");
            if (_handle == IntPtr.Zero)
                throw new ObjectDisposedException("NativeJob");
            if (!AssignProcessToJobObject(_handle, process.Handle))
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to assign a process to its guarded job.");
        }

        public Int32[] GetProcessIds()
        {
            if (_handle == IntPtr.Zero)
                return new Int32[0];
            Int32 capacity = 16;
            while (capacity <= 4096)
            {
                Int32 size = 8 + (capacity * IntPtr.Size);
                IntPtr pointer = Marshal.AllocHGlobal(size);
                try
                {
                    for (Int32 offset = 0; offset < size; offset += 4)
                        Marshal.WriteInt32(pointer, offset, 0);
                    UInt32 returned;
                    bool ok = QueryInformationJobObject(
                        _handle, 3, pointer, (UInt32)size, out returned);
                    Int32 assigned = Marshal.ReadInt32(pointer, 0);
                    Int32 listed = Marshal.ReadInt32(pointer, 4);
                    if (!ok && assigned <= capacity)
                        throw new Win32Exception(
                            Marshal.GetLastWin32Error(),
                            "Unable to query guarded job membership.");
                    if (assigned > capacity || listed > capacity)
                    {
                        capacity *= 2;
                        continue;
                    }
                    List<Int32> result = new List<Int32>();
                    for (Int32 index = 0; index < listed; index++)
                    {
                        Int64 value = Marshal.ReadIntPtr(
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
                "Guarded job membership exceeded its safety limit.");
        }

        public void Dispose()
        {
            if (_handle == IntPtr.Zero)
                return;
            if (!CloseHandle(_handle))
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to close the guarded process job.");
            _handle = IntPtr.Zero;
        }
    }

    public sealed class SuspendedProcess : IDisposable
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

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr handle);

        private IntPtr _threadHandle;
        private bool _resumed;
        public Process Child { get; private set; }

        public SuspendedProcess(
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
                    "Unable to create a suspended guarded process.");
            }

            try
            {
                Child = Process.GetProcessById((Int32)information.dwProcessId);
                _threadHandle = information.hThread;
                information.hThread = IntPtr.Zero;
            }
            catch
            {
                if (information.hProcess != IntPtr.Zero)
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
                    "The suspended process thread is unavailable.");
            if (ResumeThread(_threadHandle) == UInt32.MaxValue)
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to resume a guarded process.");
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

    public static class NativeInspection
    {
        [DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr CommandLineToArgvW(
            string commandLine,
            out Int32 argumentCount);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LocalFree(IntPtr memory);

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

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr handle);

        public static string[] SplitCommandLine(string commandLine)
        {
            if (String.IsNullOrWhiteSpace(commandLine))
                return new string[0];
            Int32 count;
            IntPtr pointer = CommandLineToArgvW(commandLine, out count);
            if (pointer == IntPtr.Zero)
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to parse a native Windows command line.");
            try
            {
                string[] result = new string[count];
                for (Int32 index = 0; index < count; index++)
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

        public static string QueryImagePath(Int32 processId)
        {
            const UInt32 PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;
            IntPtr process = OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION,
                false,
                processId);
            if (process == IntPtr.Zero)
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "Unable to open a process for image inspection.");
            try
            {
                UInt32 capacity = 32768;
                StringBuilder path = new StringBuilder((Int32)capacity);
                if (!QueryFullProcessImageName(process, 0, path, ref capacity))
                    throw new Win32Exception(
                        Marshal.GetLastWin32Error(),
                        "Unable to query a process image path.");
                return path.ToString();
            }
            finally
            {
                CloseHandle(process);
            }
        }
    }
}
'@
}

function Assert-PartisanProperties {
    param(
        [Parameter(Mandatory = $true)]$Value,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($null -eq $Value) {
        throw "$Label is missing."
    }
    foreach ($name in $Names) {
        if ($Value.PSObject.Properties.Name -cnotcontains $name) {
            throw "$Label is missing required property $name."
        }
    }
}

function Get-PartisanFullPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw 'A guarded path must not be empty.'
    }
    return [IO.Path]::GetFullPath($Path)
}

function Resolve-PartisanExistingPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]
        [ValidateSet('Leaf', 'Container')][string]$Kind
    )

    $full = Get-PartisanFullPath -Path $Path
    if (-not (Test-Path -LiteralPath $full -PathType $Kind)) {
        throw "A required guarded $Kind path does not exist."
    }
    return [IO.Path]::GetFullPath(
        (Get-Item -LiteralPath $full -Force -ErrorAction Stop).FullName)
}

function Test-PartisanContainedPath {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Candidate,
        [switch]$AllowEqual
    )

    $rootFull = (Get-PartisanFullPath -Path $Root).TrimEnd('\', '/')
    $candidateFull = (Get-PartisanFullPath -Path $Candidate).TrimEnd('\', '/')
    if ($candidateFull.Equals(
            $rootFull,
            [StringComparison]::OrdinalIgnoreCase)) {
        return [bool]$AllowEqual
    }
    $prefix = $rootFull + [IO.Path]::DirectorySeparatorChar
    return $candidateFull.StartsWith(
        $prefix,
        [StringComparison]::OrdinalIgnoreCase)
}

function Test-PartisanPathOverlap {
    param(
        [Parameter(Mandatory = $true)][string]$First,
        [Parameter(Mandatory = $true)][string]$Second
    )

    return (Test-PartisanContainedPath -Root $First -Candidate $Second -AllowEqual) -or
        (Test-PartisanContainedPath -Root $Second -Candidate $First -AllowEqual)
}

function Assert-PartisanNoReparseAncestry {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $cursor = (Get-PartisanFullPath -Path $Path).TrimEnd('\', '/')
    while (-not (Test-Path -LiteralPath $cursor)) {
        $parent = Split-Path -Parent $cursor
        if ([string]::IsNullOrWhiteSpace($parent) -or
            $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'A safe existing guarded path ancestor could not be resolved.'
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

function Assert-PartisanNoReparseTree {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Root)

    $rootFull = Resolve-PartisanExistingPath -Path $Root -Kind Container
    Assert-PartisanNoReparseAncestry -Path $rootFull
    foreach ($item in @(Get-ChildItem `
            -LiteralPath $rootFull `
            -Recurse `
            -Force `
            -ErrorAction Stop)) {
        $itemFull = Get-PartisanFullPath -Path $item.FullName
        if (-not (Test-PartisanContainedPath `
                -Root $rootFull `
                -Candidate $itemFull)) {
            throw 'A guarded tree entry escaped its exact root.'
        }
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'A guarded tree must not contain a reparse point.'
        }
    }
}

function ConvertTo-PartisanNativeArgument {
    [CmdletBinding()]
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Value)

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

function ConvertTo-PartisanNativeCommandLine {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments
    )

    $tokens = New-Object Collections.Generic.List[string]
    [void]$tokens.Add((ConvertTo-PartisanNativeArgument -Value $Executable))
    foreach ($argument in $Arguments) {
        [void]$tokens.Add((ConvertTo-PartisanNativeArgument -Value ([string]$argument)))
    }
    return $tokens.ToArray() -join ' '
}

function Test-PartisanExactNativeArgumentVector {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$CommandLine,
        [Parameter(Mandatory = $true)][string]$ExpectedExecutable,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$ExpectedArguments
    )

    try {
        $tokens = @(
            [Partisan.GuardedRuntime.NativeInspection]::SplitCommandLine($CommandLine))
        if ($tokens.Count -ne $ExpectedArguments.Count + 1) {
            return $false
        }
        $actualExecutable = Get-PartisanFullPath -Path ([string]$tokens[0])
        $expectedExecutable = Get-PartisanFullPath -Path $ExpectedExecutable
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
    catch {
        return $false
    }
}

function Get-PartisanSha256 {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $leaf = Resolve-PartisanExistingPath -Path $Path -Kind Leaf
    Assert-PartisanNoReparseAncestry -Path $leaf
    return (Get-FileHash `
        -LiteralPath $leaf `
        -Algorithm SHA256 `
        -ErrorAction Stop).Hash.ToLowerInvariant()
}

function Get-PartisanSha256Text {
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string]$Text)

    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($Text)
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $algorithm.Dispose()
    }
}

function Get-PartisanFileSignature {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $leaf = Resolve-PartisanExistingPath -Path $Path -Kind Leaf
    $file = Get-Item -LiteralPath $leaf -Force -ErrorAction Stop
    if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'A guarded file signature cannot follow a reparse point.'
    }
    return [pscustomobject][ordered]@{
        length = [long]$file.Length
        sha256 = Get-PartisanSha256 -Path $leaf
    }
}

function Assert-PartisanGitWorktreeFilesMatchCommit {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$Commit,
        [Parameter(Mandatory = $true)][string[]]$PortablePaths
    )

    $root = Resolve-PartisanExistingPath -Path $RepositoryRoot -Kind Container
    if ($Commit -cnotmatch '^[0-9a-f]{40,64}$') {
        throw 'A Git-bound worktree assertion has an invalid commit identity.'
    }
    if (@($PortablePaths).Count -lt 1) {
        throw 'A Git-bound worktree assertion requires at least one file.'
    }
    $seen = New-Object 'Collections.Generic.HashSet[string]' (
        [StringComparer]::Ordinal)
    foreach ($portablePath in $PortablePaths) {
        if ([string]::IsNullOrWhiteSpace($portablePath) -or
            $portablePath.Contains('\') -or
            $portablePath.Contains(':') -or
            $portablePath.StartsWith('/') -or
            $portablePath.Split('/') -contains '..' -or
            -not $seen.Add($portablePath)) {
            throw 'A Git-bound worktree path is invalid or duplicated.'
        }
        $worktreePath = [IO.Path]::GetFullPath(
            (Join-Path $root $portablePath.Replace('/', '\')))
        if (-not (Test-PartisanContainedPath `
                -Root $root -Candidate $worktreePath) -or
            -not (Test-Path -LiteralPath $worktreePath -PathType Leaf)) {
            throw "A Git-bound worktree file is missing: $portablePath"
        }
        Assert-PartisanNoReparseAncestry -Path $worktreePath

        $commitOutput = @(& git -C $root rev-parse `
            ($Commit + ':' + $portablePath) 2>$null)
        $commitExit = $LASTEXITCODE
        $commitBlob = ($commitOutput -join '').Trim()
        if ($commitExit -ne 0 -or
            $commitBlob -cnotmatch '^[0-9a-f]{40,64}$') {
            throw "A Git-bound commit blob is missing: $portablePath"
        }
        $worktreeOutput = @(& git -C $root hash-object `
            --no-filters -- $portablePath 2>$null)
        $worktreeExit = $LASTEXITCODE
        $worktreeBlob = ($worktreeOutput -join '').Trim()
        if ($worktreeExit -ne 0 -or
            $worktreeBlob -cnotmatch '^[0-9a-f]{40,64}$') {
            throw "A Git-bound worktree blob is invalid: $portablePath"
        }
        if ($worktreeBlob -cne $commitBlob) {
            throw "A Git-bound worktree file differs from its commit blob: $portablePath"
        }
    }
}

function Test-PartisanFileSignatureExact {
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    try {
        foreach ($value in @($Expected, $Actual)) {
            Assert-PartisanProperties `
                -Value $value `
                -Names @('length', 'sha256') `
                -Label 'Portable file signature'
        }
        return [long]$Expected.length -eq [long]$Actual.length -and
            [string]$Expected.sha256 -ceq [string]$Actual.sha256 -and
            [string]$Expected.sha256 -cmatch '^[0-9a-f]{64}$'
    }
    catch {
        return $false
    }
}

function ConvertTo-PartisanCanonicalValue {
    param($Value)

    if ($null -eq $Value) {
        return $null
    }
    if ($Value -is [datetime]) {
        return $Value.ToUniversalTime().ToString(
            'o',
            [Globalization.CultureInfo]::InvariantCulture)
    }
    if ($Value -is [Collections.IDictionary]) {
        $result = [ordered]@{}
        foreach ($key in @($Value.Keys | ForEach-Object { [string]$_ } | Sort-Object)) {
            $result[$key] = ConvertTo-PartisanCanonicalValue -Value $Value[$key]
        }
        return $result
    }
    if ($Value -is [Collections.IEnumerable] -and -not ($Value -is [string])) {
        return ,@($Value | ForEach-Object {
            ConvertTo-PartisanCanonicalValue -Value $_
        })
    }
    if ($Value -is [psobject] -and
        @($Value.PSObject.Properties).Count -gt 0 -and
        -not ($Value -is [ValueType]) -and
        -not ($Value -is [string])) {
        $result = [ordered]@{}
        foreach ($name in @($Value.PSObject.Properties.Name | Sort-Object)) {
            $result[$name] = ConvertTo-PartisanCanonicalValue -Value $Value.$name
        }
        return $result
    }
    return $Value
}

function Write-PartisanPortableJson {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $full = Get-PartisanFullPath -Path $Path
    $parent = Split-Path -Parent $full
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        throw 'The guarded JSON parent directory does not exist.'
    }
    Assert-PartisanNoReparseAncestry -Path $full
    $canonical = ConvertTo-PartisanCanonicalValue -Value $Value
    $text = (($canonical | ConvertTo-Json -Depth 32).Replace("`r`n", "`n") + "`n")
    [IO.File]::WriteAllText(
        $full,
        $text,
        (New-Object Text.UTF8Encoding($false)))
    return Get-PartisanFileSignature -Path $full
}

function Write-PartisanPortableJsonAtomic {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $full = Get-PartisanFullPath -Path $Path
    $parent = Resolve-PartisanExistingPath `
        -Path (Split-Path -Parent $full) `
        -Kind Container
    $temporary = Get-PartisanFullPath -Path (
        Join-Path $parent ('.partisan-write-' + [Guid]::NewGuid().ToString('N')))
    if (-not (Test-PartisanContainedPath -Root $parent -Candidate $temporary) -or
        (Test-Path -LiteralPath $temporary)) {
        throw 'A guarded atomic JSON temporary path is not fresh and contained.'
    }
    try {
        [void](Write-PartisanPortableJson -Path $temporary -Value $Value)
        Move-Item `
            -LiteralPath $temporary `
            -Destination $full `
            -Force `
            -ErrorAction Stop
        return Get-PartisanFileSignature -Path $full
    }
    finally {
        if (Test-Path -LiteralPath $temporary -PathType Leaf) {
            Remove-Item -LiteralPath $temporary -Force -ErrorAction Stop
        }
    }
}

function Read-PartisanPortableJson {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [ValidateRange(1, 67108864)][int]$MaximumBytes = 16777216
    )

    $leaf = Resolve-PartisanExistingPath -Path $Path -Kind Leaf
    Assert-PartisanNoReparseAncestry -Path $leaf
    $file = Get-Item -LiteralPath $leaf -Force -ErrorAction Stop
    if ($file.Length -gt $MaximumBytes) {
        throw 'A guarded JSON artifact exceeded its size limit.'
    }
    $bytes = [IO.File]::ReadAllBytes($leaf)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw 'A guarded JSON artifact must be UTF-8 without a BOM.'
    }
    try {
        return ((New-Object Text.UTF8Encoding($false, $true)).GetString($bytes) |
            ConvertFrom-Json -ErrorAction Stop)
    }
    catch {
        throw 'A guarded JSON artifact is not strict UTF-8 JSON.'
    }
}

function Get-PartisanGuardOwnership {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$GuardBase
    )

    try {
        $baseFull = Resolve-PartisanExistingPath -Path $GuardBase -Kind Container
        $directoryFull = Resolve-PartisanExistingPath `
            -Path $Directory `
            -Kind Container
        if (-not (Test-PartisanContainedPath `
                -Root $baseFull `
                -Candidate $directoryFull)) {
            return $null
        }
        $parentFull = Get-PartisanFullPath -Path (Split-Path -Parent $directoryFull)
        if (-not $parentFull.TrimEnd('\', '/').Equals(
                $baseFull.TrimEnd('\', '/'),
                [StringComparison]::OrdinalIgnoreCase)) {
            return $null
        }
        $leaf = Split-Path -Leaf $directoryFull
        $pattern = '^' + [regex]::Escape($script:GuardLeafPrefix) +
            '([0-9a-f]{32})$'
        if ($leaf -cnotmatch $pattern) {
            return $null
        }
        $leafNonce = [string]$matches[1]
        Assert-PartisanNoReparseAncestry -Path $directoryFull
        $directoryItem = Get-Item -LiteralPath $directoryFull -Force
        if (($directoryItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $null
        }
        $sentinel = Join-Path $directoryFull $script:GuardSentinelLeaf
        if (-not (Test-Path -LiteralPath $sentinel -PathType Leaf)) {
            return $null
        }
        Assert-PartisanNoReparseAncestry -Path $sentinel
        $value = Read-PartisanPortableJson -Path $sentinel
        $required = @(
            'createdUtc',
            'guardLeaf',
            'magic',
            'nonce',
            'ownerPid',
            'ownerStartUtc',
            'purpose',
            'version') | Sort-Object
        $actual = @($value.PSObject.Properties.Name | Sort-Object)
        if (@(Compare-Object `
                -ReferenceObject $required `
                -DifferenceObject $actual `
                -CaseSensitive).Count -ne 0) {
            return $null
        }
        if ([int]$value.version -ne 1 -or
            [string]$value.magic -cne $script:GuardMagic -or
            [string]$value.nonce -cne $leafNonce -or
            [string]$value.guardLeaf -cne $leaf -or
            [string]$value.purpose -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,95}$' -or
            [int]$value.ownerPid -le 0) {
            return $null
        }
        $ownerStartUtc = [DateTime]::Parse(
            [string]$value.ownerStartUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        $createdUtc = [DateTime]::Parse(
            [string]$value.createdUtc,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::RoundtripKind).ToUniversalTime()
        return [pscustomobject][ordered]@{
            Directory = $directoryFull
            GuardBase = $baseFull
            Sentinel = Get-PartisanFullPath -Path $sentinel
            Nonce = $leafNonce
            Purpose = [string]$value.purpose
            OwnerPid = [int]$value.ownerPid
            OwnerStartUtc = $ownerStartUtc
            CreatedUtc = $createdUtc
            SentinelSignature = Get-PartisanFileSignature -Path $sentinel
        }
    }
    catch {
        return $null
    }
}

function New-PartisanGuardDirectory {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [Parameter(Mandatory = $true)][string]$Purpose
    )

    if ($Purpose -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,95}$') {
        throw 'A guarded runtime purpose must be portable and nonempty.'
    }
    $baseFull = Resolve-PartisanExistingPath -Path $GuardBase -Kind Container
    Assert-PartisanNoReparseTree -Root $baseFull
    $nonce = [Guid]::NewGuid().ToString('N')
    $leaf = $script:GuardLeafPrefix + $nonce
    $directory = Get-PartisanFullPath -Path (Join-Path $baseFull $leaf)
    if (-not (Test-PartisanContainedPath `
            -Root $baseFull `
            -Candidate $directory) -or
        (Test-Path -LiteralPath $directory)) {
        throw 'The nonce-owned guard directory must be fresh and exactly contained.'
    }
    $owner = Get-Process -Id $PID -ErrorAction Stop
    $ownerStartUtc = $owner.StartTime.ToUniversalTime()
    $createdUtc = [DateTime]::UtcNow
    New-Item -ItemType Directory -Path $directory -ErrorAction Stop | Out-Null
    try {
        Assert-PartisanNoReparseAncestry -Path $directory
        $sentinel = Join-Path $directory $script:GuardSentinelLeaf
        [void](Write-PartisanPortableJson `
            -Path $sentinel `
            -Value ([ordered]@{
                version = 1
                magic = $script:GuardMagic
                purpose = $Purpose
                nonce = $nonce
                guardLeaf = $leaf
                ownerPid = $PID
                ownerStartUtc = $ownerStartUtc.ToString(
                    'o',
                    [Globalization.CultureInfo]::InvariantCulture)
                createdUtc = $createdUtc.ToString(
                    'o',
                    [Globalization.CultureInfo]::InvariantCulture)
            }))
        $ownership = Get-PartisanGuardOwnership `
            -Directory $directory `
            -GuardBase $baseFull
        if ($null -eq $ownership -or
            $ownership.Nonce -cne $nonce -or
            $ownership.OwnerPid -ne $PID -or
            $ownership.OwnerStartUtc.Ticks -ne $ownerStartUtc.Ticks) {
            throw 'Guarded runtime ownership could not be established exactly.'
        }
        return $ownership
    }
    catch {
        # Ownership was not proven, so no child of this directory is trusted
        # for deletion.  Preserve the entire partial guard for inspection.
        throw
    }
}

function Remove-PartisanGuardDirectoryCore {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Ownership,
        [AllowNull()][object[]]$ExpectedInventory,
        [switch]$RequirePresent
    )

    Assert-PartisanProperties `
        -Value $Ownership `
        -Names @(
            'Directory',
            'GuardBase',
            'Nonce',
            'Purpose',
            'OwnerPid',
            'OwnerStartUtc',
            'SentinelSignature') `
        -Label 'Guard ownership'
    if ([string]$Ownership.Nonce -cnotmatch '^[0-9a-f]{32}$') {
        throw 'Guard cleanup received an invalid ownership nonce.'
    }
    if (-not (Test-Path -LiteralPath $Ownership.Directory)) {
        if ($RequirePresent) {
            throw 'The exact nonce-owned guard disappeared before final removal.'
        }
        return $true
    }
    $currentOwner = Get-Process -Id $PID -ErrorAction Stop
    $currentOwnerStartUtc = $currentOwner.StartTime.ToUniversalTime()
    if ([int]$Ownership.OwnerPid -ne $PID -or
        ([datetime]$Ownership.OwnerStartUtc).ToUniversalTime().Ticks -ne
            $currentOwnerStartUtc.Ticks) {
        throw 'Guard cleanup is not running as the exact recorded PowerShell owner.'
    }
    $current = Get-PartisanGuardOwnership `
        -Directory ([string]$Ownership.Directory) `
        -GuardBase ([string]$Ownership.GuardBase)
    if ($null -eq $current -or
        $current.Nonce -cne [string]$Ownership.Nonce -or
        $current.Purpose -cne [string]$Ownership.Purpose -or
        $current.OwnerPid -ne [int]$Ownership.OwnerPid -or
        $current.OwnerStartUtc.Ticks -ne
            ([datetime]$Ownership.OwnerStartUtc).ToUniversalTime().Ticks -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $Ownership.SentinelSignature `
            -Actual $current.SentinelSignature)) {
        throw 'Guard cleanup ownership does not match the exact current sentinel.'
    }
    if ($null -eq $ExpectedInventory) {
        $ExpectedInventory = [object[]]@(
            [pscustomobject][ordered]@{
                relativePath = $script:GuardSentinelLeaf
                kind = 'file'
                signature = Copy-PartisanV2PlainValue `
                    -Value $Ownership.SentinelSignature
            })
    }
    [void](Assert-PartisanV2GuardInventoryExact `
        -GuardDirectory $current.Directory `
        -ExpectedInventory $ExpectedInventory)
    $ownedChildren = @($ExpectedInventory | Where-Object {
        [string]$_.relativePath -cne $script:GuardSentinelLeaf
    } | Sort-Object { ([string]$_.relativePath).Length } -Descending)
    foreach ($entry in $ownedChildren) {
        $relative = ([string]$entry.relativePath).Replace(
            '/',
            [IO.Path]::DirectorySeparatorChar)
        $target = Get-PartisanFullPath -Path (Join-Path `
            $current.Directory `
            $relative)
        if (-not (Test-PartisanContainedPath `
                -Root $current.Directory `
                -Candidate $target)) {
            throw 'A sealed guard-inventory cleanup target escaped its guard.'
        }
        $item = Get-Item -LiteralPath $target -Force -ErrorAction Stop
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'A sealed guard-inventory cleanup target became a reparse point.'
        }
        if ([string]$entry.kind -ceq 'file') {
            if ($item.PSIsContainer -or
                -not (Test-PartisanFileSignatureExact `
                    -Expected $entry.signature `
                    -Actual (Get-PartisanFileSignature -Path $target))) {
                throw 'A sealed guard-inventory file changed before cleanup.'
            }
        }
        elseif ([string]$entry.kind -ceq 'directory') {
            if (-not $item.PSIsContainer) {
                throw 'A sealed guard-inventory directory changed before cleanup.'
            }
        }
        else {
            throw 'A sealed guard-inventory entry has an invalid kind.'
        }
        Remove-Item -LiteralPath $target -Force -ErrorAction Stop
    }
    $final = Get-PartisanGuardOwnership `
        -Directory $current.Directory `
        -GuardBase $current.GuardBase
    if ($null -eq $final -or
        $final.Nonce -cne $current.Nonce -or
        $final.OwnerPid -ne $current.OwnerPid -or
        $final.OwnerStartUtc.Ticks -ne $current.OwnerStartUtc.Ticks -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $Ownership.SentinelSignature `
            -Actual $final.SentinelSignature)) {
        throw 'Guard ownership changed during exact cleanup.'
    }
    [void](Assert-PartisanV2GuardInventoryExact `
        -GuardDirectory $final.Directory `
        -ExpectedInventory ([object[]]@(
            [pscustomobject][ordered]@{
                relativePath = $script:GuardSentinelLeaf
                kind = 'file'
                signature = Copy-PartisanV2PlainValue `
                    -Value $Ownership.SentinelSignature
            })))
    Remove-Item -LiteralPath $final.Sentinel -Force -ErrorAction Stop
    # Exact guard-directory removal is the final filesystem operation here.
    Remove-Item -LiteralPath $final.Directory -Force -ErrorAction Stop
    return $true
}

function Get-PartisanSnapshotEntries {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$ExcludedRoots = @()
    )

    $rootFull = Resolve-PartisanExistingPath -Path $Root -Kind Container
    Assert-PartisanNoReparseTree -Root $rootFull
    $prefix = $rootFull.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $entries = @{}
    foreach ($item in @(Get-ChildItem `
            -LiteralPath $rootFull `
            -Recurse `
            -Force `
            -ErrorAction Stop)) {
        $full = Get-PartisanFullPath -Path $item.FullName
        if (-not (Test-PartisanContainedPath -Root $rootFull -Candidate $full)) {
            throw 'A snapshot entry escaped its exact root.'
        }
        $excluded = $false
        foreach ($excludedRoot in $ExcludedRoots) {
            if (Test-PartisanContainedPath `
                    -Root $excludedRoot `
                    -Candidate $full `
                    -AllowEqual) {
                $excluded = $true
                break
            }
        }
        if ($excluded) {
            continue
        }
        $relative = $full.Substring($prefix.Length).Replace('\', '/')
        if ($relative.Contains(':') -or
            $relative.Split('/') -contains '..') {
            throw 'A snapshot path is not portable.'
        }
        $signature = if ($item.PSIsContainer) {
            'D'
        }
        else {
            $fileSignature = Get-PartisanFileSignature -Path $full
            'F:{0}:{1}' -f $fileSignature.length, $fileSignature.sha256
        }
        $entries[$relative] = $signature
    }
    return $entries
}

function New-PartisanRootSnapshot {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [ValidateSet('watched', 'spill')][string]$Kind = 'watched',
        [string[]]$ExcludedRoots = @()
    )

    $rootFull = Resolve-PartisanExistingPath -Path $Root -Kind Container
    $resolvedExclusions = New-Object Collections.Generic.List[string]
    foreach ($excludedRoot in $ExcludedRoots) {
        $excludedFull = Get-PartisanFullPath -Path $excludedRoot
        if (-not (Test-PartisanContainedPath `
                -Root $rootFull `
                -Candidate $excludedFull `
                -AllowEqual)) {
            throw 'A snapshot exclusion must be exactly contained by its root.'
        }
        [void]$resolvedExclusions.Add($excludedFull)
    }
    return [pscustomobject][ordered]@{
        Root = $rootFull
        Kind = $Kind
        ExcludedRoots = $resolvedExclusions.ToArray()
        Entries = Get-PartisanSnapshotEntries `
            -Root $rootFull `
            -ExcludedRoots $resolvedExclusions.ToArray()
    }
}

function Compare-PartisanRootSnapshot {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)]$Snapshot)

    Assert-PartisanProperties `
        -Value $Snapshot `
        -Names @('Root', 'Kind', 'ExcludedRoots', 'Entries') `
        -Label 'Root snapshot'
    if (-not (Test-Path -LiteralPath $Snapshot.Root -PathType Container)) {
        return [pscustomobject][ordered]@{
            Root = [string]$Snapshot.Root
            Kind = [string]$Snapshot.Kind
            NewEntries = 0
            ModifiedEntries = 0
            DeletedEntries = 0
            MissingRoot = 1
            Changes = @()
            Clean = $false
        }
    }
    $current = Get-PartisanSnapshotEntries `
        -Root ([string]$Snapshot.Root) `
        -ExcludedRoots @($Snapshot.ExcludedRoots)
    $changes = New-Object Collections.Generic.List[object]
    foreach ($path in @($current.Keys | Sort-Object)) {
        if (-not $Snapshot.Entries.ContainsKey($path)) {
            [void]$changes.Add([pscustomobject][ordered]@{
                path = $path
                change = 'new'
            })
        }
        elseif ([string]$current[$path] -cne
            [string]$Snapshot.Entries[$path]) {
            [void]$changes.Add([pscustomobject][ordered]@{
                path = $path
                change = 'modified'
            })
        }
    }
    foreach ($path in @($Snapshot.Entries.Keys | Sort-Object)) {
        if (-not $current.ContainsKey($path)) {
            [void]$changes.Add([pscustomobject][ordered]@{
                path = $path
                change = 'deleted'
            })
        }
    }
    $rows = $changes.ToArray()
    $newCount = @($rows | Where-Object { $_.change -ceq 'new' }).Count
    $modifiedCount = @($rows | Where-Object { $_.change -ceq 'modified' }).Count
    $deletedCount = @($rows | Where-Object { $_.change -ceq 'deleted' }).Count
    return [pscustomobject][ordered]@{
        Root = [string]$Snapshot.Root
        Kind = [string]$Snapshot.Kind
        NewEntries = $newCount
        ModifiedEntries = $modifiedCount
        DeletedEntries = $deletedCount
        MissingRoot = 0
        Changes = $rows
        Clean = $rows.Count -eq 0
    }
}

function New-PartisanBoundarySnapshotSet {
    [CmdletBinding()]
    param(
        [string[]]$WatchedRoots = @(),
        [string[]]$SpillRoots = @()
    )

    $seen = New-Object Collections.Generic.HashSet[string](
        [StringComparer]::OrdinalIgnoreCase)
    $snapshots = New-Object Collections.Generic.List[object]
    foreach ($entry in @(
            @($WatchedRoots | ForEach-Object {
                [pscustomobject]@{ Root = $_; Kind = 'watched' }
            }) +
            @($SpillRoots | ForEach-Object {
                [pscustomobject]@{ Root = $_; Kind = 'spill' }
            }))) {
        $full = Resolve-PartisanExistingPath -Path $entry.Root -Kind Container
        if (-not $seen.Add($full)) {
            throw 'A boundary root was supplied more than once.'
        }
        [void]$snapshots.Add((New-PartisanRootSnapshot `
            -Root $full `
            -Kind $entry.Kind))
    }
    return $snapshots.ToArray()
}

function Compare-PartisanBoundarySnapshotSet {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]]$Snapshots)

    $deltas = @($Snapshots | ForEach-Object {
        Compare-PartisanRootSnapshot -Snapshot $_
    })
    return [pscustomobject][ordered]@{
        Deltas = $deltas
        NewEntries = [int](($deltas | Measure-Object NewEntries -Sum).Sum)
        ModifiedEntries = [int](($deltas | Measure-Object ModifiedEntries -Sum).Sum)
        DeletedEntries = [int](($deltas | Measure-Object DeletedEntries -Sum).Sum)
        MissingRoots = [int](($deltas | Measure-Object MissingRoot -Sum).Sum)
        Clean = @($deltas | Where-Object { -not $_.Clean }).Count -eq 0
    }
}

function Test-PartisanLoopbackPortAvailable {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateRange(1, 65535)][int]$Port,
        [ValidateSet('Tcp', 'Udp')][string]$Protocol = 'Udp'
    )

    $socket = $null
    try {
        $socketType = if ($Protocol -ceq 'Tcp') {
            [Net.Sockets.SocketType]::Stream
        }
        else {
            [Net.Sockets.SocketType]::Dgram
        }
        $protocolType = if ($Protocol -ceq 'Tcp') {
            [Net.Sockets.ProtocolType]::Tcp
        }
        else {
            [Net.Sockets.ProtocolType]::Udp
        }
        # This is deliberately only an IPv4 loopback bind-availability probe.
        # It does not attribute a listener to a PID and does not reserve the port.
        $socket = New-Object Net.Sockets.Socket(
            [Net.Sockets.AddressFamily]::InterNetwork,
            $socketType,
            $protocolType)
        $socket.ExclusiveAddressUse = $true
        $socket.Bind((New-Object Net.IPEndPoint(
            [Net.IPAddress]::Loopback,
            $Port)))
        return $true
    }
    catch {
        return $false
    }
    finally {
        if ($socket) {
            $socket.Dispose()
        }
    }
}

function Assert-PartisanLoopbackPortAvailable {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateRange(1, 65535)][int]$Port,
        [ValidateSet('Tcp', 'Udp')][string]$Protocol = 'Udp'
    )

    if (-not (Test-PartisanLoopbackPortAvailable `
            -Port $Port `
            -Protocol $Protocol)) {
        throw "The IPv4 loopback $Protocol port is not bind-available."
    }
    return [pscustomobject][ordered]@{
        Port = $Port
        Protocol = $Protocol
        AddressFamily = 'IPv4'
        Scope = 'loopback'
        Semantics = 'bind-availability-only'
    }
}

function Wait-PartisanLoopbackPortReleased {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][ValidateRange(1, 65535)][int]$Port,
        [ValidateSet('Tcp', 'Udp')][string]$Protocol = 'Udp',
        [ValidateRange(1, 120)][int]$TimeoutSeconds = 15,
        [ValidateRange(20, 2000)][int]$PollMilliseconds = 100
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $stablePolls = 0
    while ([DateTime]::UtcNow -lt $deadline) {
        if (Test-PartisanLoopbackPortAvailable -Port $Port -Protocol $Protocol) {
            $stablePolls++
            if ($stablePolls -ge 2) {
                return $true
            }
        }
        else {
            $stablePolls = 0
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    throw "The IPv4 loopback $Protocol port did not become bind-available."
}

function Get-PartisanExecutableProvenance {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $leaf = Resolve-PartisanExistingPath -Path $Path -Kind Leaf
    Assert-PartisanNoReparseAncestry -Path $leaf
    $file = Get-Item -LiteralPath $leaf -Force -ErrorAction Stop
    if ($file.Length -le 0) {
        throw 'A guarded runtime executable must not be empty.'
    }
    return [pscustomobject][ordered]@{
        fileName = $file.Name
        fileVersion = [string]$file.VersionInfo.FileVersion
        productVersion = [string]$file.VersionInfo.ProductVersion
        length = [long]$file.Length
        sha256 = Get-PartisanSha256 -Path $leaf
    }
}

function Assert-PartisanExecutableProvenance {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)][string]$Path
    )

    Assert-PartisanProperties `
        -Value $Expected `
        -Names @('fileName', 'fileVersion', 'productVersion', 'length', 'sha256') `
        -Label 'Sealed executable provenance'
    $actual = Get-PartisanExecutableProvenance -Path $Path
    if ([string]$Expected.fileName -cne [string]$actual.fileName -or
        [string]$Expected.fileVersion -cne [string]$actual.fileVersion -or
        [string]$Expected.productVersion -cne [string]$actual.productVersion -or
        [long]$Expected.length -ne [long]$actual.length -or
        [string]$Expected.sha256 -cne [string]$actual.sha256) {
        throw 'The runtime executable differs from its sealed provenance.'
    }
    return $actual
}

function Assert-PartisanCandidateBindingShape {
    param([Parameter(Mandatory = $true)]$Candidate)

    Assert-PartisanProperties `
        -Value $Candidate `
        -Names @(
            'CandidateId',
            'PackageSha256',
            'PackageFiles',
            'PackedAddonPath',
            'RuntimeAddonRootPath') `
        -Label 'Release-candidate binding'
    if ([string]$Candidate.CandidateId -cnotmatch
            '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$' -or
        [string]$Candidate.PackageSha256 -cnotmatch '^[0-9a-f]{64}$' -or
        @($Candidate.PackageFiles).Count -eq 0 -or
        [string]$Candidate.PackedAddonPath -match ',' -or
        [string]$Candidate.RuntimeAddonRootPath -match ',') {
        throw 'The release-candidate binding is not suitable for guarded staging.'
    }
}

function ConvertTo-PartisanSealedCandidateBinding {
    param([Parameter(Mandatory = $true)]$Candidate)

    Assert-PartisanCandidateBindingShape -Candidate $Candidate
    $sourceAddon = Resolve-PartisanExistingPath `
        -Path ([string]$Candidate.PackedAddonPath) `
        -Kind Container
    $runtimeAddon = Resolve-PartisanExistingPath `
        -Path ([string]$Candidate.RuntimeAddonRootPath) `
        -Kind Container
    foreach ($path in @($sourceAddon, $runtimeAddon)) {
        if ($path.Contains(',')) {
            throw 'A candidate path must not contain the add-on search delimiter.'
        }
        Assert-PartisanNoReparseTree -Root $path
    }
    $rows = New-Object Collections.Generic.List[object]
    $seenIndexPaths = New-Object Collections.Generic.HashSet[string](
        [StringComparer]::Ordinal)
    foreach ($record in @($Candidate.PackageFiles | Sort-Object indexPath)) {
        Assert-PartisanProperties `
            -Value $record `
            -Names @('indexPath', 'length', 'sha256') `
            -Label 'Candidate package record'
        $indexPath = [string]$record.indexPath
        $sha256 = [string]$record.sha256
        if ([string]::IsNullOrWhiteSpace($indexPath) -or
            $indexPath -match '[,\r\n\t]' -or
            $indexPath.Contains('\') -or
            $indexPath.Contains(':') -or
            $indexPath.StartsWith('/', [StringComparison]::Ordinal) -or
            $indexPath.Split('/') -contains '..' -or
            -not $seenIndexPaths.Add($indexPath) -or
            [long]$record.length -le 0 -or
            $sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'A candidate package record is not canonical and exact.'
        }
        [void]$rows.Add([pscustomobject][ordered]@{
            indexPath = $indexPath
            length = [long]$record.length
            sha256 = $sha256
        })
    }
    return [pscustomobject][ordered]@{
        CandidateId = [string]$Candidate.CandidateId
        PackageSha256 = [string]$Candidate.PackageSha256
        PackageFiles = [object[]]$rows.ToArray()
        PackedAddonPath = $sourceAddon
        RuntimeAddonRootPath = $runtimeAddon
    }
}

function Get-PartisanCandidateBindingSha256 {
    param([Parameter(Mandatory = $true)]$Candidate)

    $canonical = ConvertTo-PartisanCanonicalValue -Value $Candidate
    $text = ($canonical | ConvertTo-Json -Compress -Depth 32) + "`n"
    return Get-PartisanSha256Text -Text $text
}

function Get-PartisanContextCandidateBinding {
    param([Parameter(Mandatory = $true)]$Context)

    if ([string]::IsNullOrWhiteSpace([string]$Context.CandidateBindingPath) -or
        $null -eq $Context.CandidateBindingSignature -or
        [string]$Context.CandidateBindingSha256 -cnotmatch '^[0-9a-f]{64}$') {
        throw 'The guarded context has no sealed candidate binding.'
    }
    $bindingPath = Resolve-PartisanExistingPath `
        -Path ([string]$Context.CandidateBindingPath) `
        -Kind Leaf
    $expectedPath = Get-PartisanFullPath -Path (
        Join-Path $Context.Guard.Directory $script:CandidateBindingLeaf)
    if (-not $bindingPath.Equals(
            $expectedPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-PartisanContainedPath `
            -Root $Context.Guard.Directory `
            -Candidate $bindingPath)) {
        throw 'The sealed candidate binding escaped its exact guard.'
    }
    $signature = Get-PartisanFileSignature -Path $bindingPath
    if (-not (Test-PartisanFileSignatureExact `
            -Expected $Context.CandidateBindingSignature `
            -Actual $signature)) {
        throw 'The sealed candidate binding bytes changed.'
    }
    $binding = Read-PartisanPortableJson -Path $bindingPath
    $sealed = ConvertTo-PartisanSealedCandidateBinding -Candidate $binding
    if ((Get-PartisanCandidateBindingSha256 -Candidate $sealed) -cne
        [string]$Context.CandidateBindingSha256) {
        throw 'The sealed candidate binding digest changed.'
    }
    return $sealed
}

function Get-PartisanCandidateStageDigest {
    param(
        [Parameter(Mandatory = $true)]$Candidate,
        [Parameter(Mandatory = $true)][string]$StagedAddonPath
    )

    $rows = New-Object Collections.Generic.List[string]
    foreach ($record in @($Candidate.PackageFiles | Sort-Object indexPath)) {
        Assert-PartisanProperties `
            -Value $record `
            -Names @('indexPath', 'length', 'sha256') `
            -Label 'Candidate package record'
        $indexPath = [string]$record.indexPath
        if ([string]::IsNullOrWhiteSpace($indexPath) -or
            $indexPath.Contains('\') -or
            $indexPath.Contains(':') -or
            $indexPath.StartsWith('/', [StringComparison]::Ordinal) -or
            $indexPath.Split('/') -contains '..' -or
            [long]$record.length -le 0 -or
            [string]$record.sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'A candidate stage record is not portable and exact.'
        }
        $name = Split-Path -Leaf $indexPath
        if ($name -cne $indexPath.Split('/')[-1]) {
            throw 'A candidate stage record has an invalid file name.'
        }
        $path = Resolve-PartisanExistingPath `
            -Path (Join-Path $StagedAddonPath $name) `
            -Kind Leaf
        if (-not (Test-PartisanContainedPath `
                -Root $StagedAddonPath `
                -Candidate $path)) {
            throw 'A candidate stage file escaped its exact add-on root.'
        }
        $signature = Get-PartisanFileSignature -Path $path
        if ([long]$signature.length -ne [long]$record.length -or
            [string]$signature.sha256 -cne [string]$record.sha256) {
            throw 'A candidate stage file differs from its sealed record.'
        }
        [void]$rows.Add(("{0}`t{1}`t{2}" -f
            $signature.sha256,
            [long]$signature.length,
            $indexPath))
    }
    return Get-PartisanSha256Text -Text (($rows.ToArray() -join "`n") + "`n")
}

function Assert-PartisanCandidateStage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Stage
    )

    Assert-PartisanRuntimeContext `
        -Context $Context `
        -AllowedStates @('active', 'teardown-failed')
    if ($null -eq $Context.Stage -or
        -not [object]::ReferenceEquals($Stage, $Context.Stage)) {
        throw 'Only the exact in-context candidate stage object is accepted.'
    }
    $Candidate = Get-PartisanContextCandidateBinding -Context $Context
    Assert-PartisanProperties `
        -Value $Stage `
        -Names @(
            'Magic',
            'CandidateId',
            'GuardNonce',
            'StageRootPath',
            'PackedAddonPath',
            'PackedProjectPath',
            'AddonSearchPath',
            'PackageSha256',
            'CandidateBindingSha256') `
        -Label 'Guarded candidate stage'
    if ([string]$Stage.Magic -cne $script:StageMagic -or
        [string]$Stage.GuardNonce -cne [string]$Context.ContextId -or
        [string]$Stage.CandidateId -cne [string]$Candidate.CandidateId -or
        [string]$Stage.PackageSha256 -cne [string]$Candidate.PackageSha256 -or
        [string]$Stage.CandidateBindingSha256 -cne
            [string]$Context.CandidateBindingSha256) {
        throw 'The guarded candidate stage identity is not exact.'
    }
    $stageRoot = Resolve-PartisanExistingPath `
        -Path ([string]$Stage.StageRootPath) `
        -Kind Container
    $expectedStageRoot = Get-PartisanFullPath -Path (
        Join-Path $Context.Guard.Directory 'candidate-addons')
    if (-not $stageRoot.Equals(
            $expectedStageRoot,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-PartisanContainedPath `
            -Root $Context.Guard.Directory `
            -Candidate $stageRoot)) {
        throw 'The candidate stage is not the exact guard-contained stage root.'
    }
    Assert-PartisanNoReparseTree -Root $stageRoot
    $topNames = @(Get-ChildItem `
        -LiteralPath $stageRoot `
        -Force `
        -ErrorAction Stop | ForEach-Object { $_.Name })
    if ($topNames.Count -ne 1 -or $topNames[0] -cne 'Partisan') {
        throw 'The guarded candidate stage has an unexpected top-level layout.'
    }
    $addonPath = Resolve-PartisanExistingPath `
        -Path (Join-Path $stageRoot 'Partisan') `
        -Kind Container
    $expectedNames = @($Candidate.PackageFiles | ForEach-Object {
        Split-Path -Leaf ([string]$_.indexPath)
    } | Sort-Object)
    if (@($expectedNames | Select-Object -Unique).Count -ne $expectedNames.Count) {
        throw 'The candidate package maps more than one record to a staged file.'
    }
    $actualNames = @(Get-ChildItem `
        -LiteralPath $addonPath `
        -Force `
        -ErrorAction Stop | ForEach-Object { $_.Name } | Sort-Object)
    if (@(Compare-Object `
            -ReferenceObject $expectedNames `
            -DifferenceObject $actualNames `
            -CaseSensitive).Count -ne 0) {
        throw 'The guarded candidate stage does not have its exact sealed inventory.'
    }
    $digest = Get-PartisanCandidateStageDigest `
        -Candidate $Candidate `
        -StagedAddonPath $addonPath
    if ($digest -cne [string]$Candidate.PackageSha256) {
        throw 'The guarded candidate stage digest differs from the sealed package.'
    }
    $projectPath = Resolve-PartisanExistingPath `
        -Path ([string]$Stage.PackedProjectPath) `
        -Kind Leaf
    if (-not $projectPath.Equals(
            (Get-PartisanFullPath -Path (Join-Path $addonPath 'addon.gproj')),
            [StringComparison]::OrdinalIgnoreCase) -or
        -not ([string]$Stage.PackedAddonPath).Equals(
            $addonPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        [string]$Stage.AddonSearchPath -cne
            ([string]$Candidate.RuntimeAddonRootPath + ',' + $stageRoot)) {
        throw 'The guarded candidate stage paths are not exact.'
    }
    return $Stage
}

function New-PartisanCandidateStage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Candidate
    )

    Assert-PartisanRuntimeContext -Context $Context -AllowedStates @('active')
    $sealedCandidate = ConvertTo-PartisanSealedCandidateBinding `
        -Candidate $Candidate
    if ($null -ne $Context.Stage -or
        $null -ne $Context.CandidateBindingSignature -or
        -not [string]::IsNullOrWhiteSpace(
            [string]$Context.CandidateBindingPath)) {
        throw 'The guarded runtime context already has a candidate stage.'
    }
    $currentOwnership = Get-PartisanGuardOwnership `
        -Directory $Context.Guard.Directory `
        -GuardBase $Context.Guard.GuardBase
    if ($null -eq $currentOwnership -or
        $currentOwnership.Nonce -cne $Context.Guard.Nonce) {
        throw 'Candidate staging requires the exact live guard ownership.'
    }
    $sourceAddon = [string]$sealedCandidate.PackedAddonPath
    $runtimeAddon = [string]$sealedCandidate.RuntimeAddonRootPath
    $stageRoot = Get-PartisanFullPath -Path (
        Join-Path $Context.Guard.Directory 'candidate-addons')
    if ($stageRoot.Contains(',') -or
        -not (Test-PartisanContainedPath `
            -Root $Context.Guard.Directory `
            -Candidate $stageRoot) -or
        (Test-Path -LiteralPath $stageRoot)) {
        throw 'The guarded candidate stage must be fresh and exactly contained.'
    }
    $stagedAddon = Join-Path $stageRoot 'Partisan'
    New-Item -ItemType Directory -Path $stagedAddon -ErrorAction Stop | Out-Null
    try {
        $seenNames = New-Object Collections.Generic.HashSet[string](
            [StringComparer]::Ordinal)
        foreach ($record in @($sealedCandidate.PackageFiles)) {
            Assert-PartisanProperties `
                -Value $record `
                -Names @('indexPath', 'length', 'sha256') `
                -Label 'Candidate package record'
            $name = Split-Path -Leaf ([string]$record.indexPath)
            if ([string]::IsNullOrWhiteSpace($name) -or
                -not $seenNames.Add($name)) {
                throw 'The candidate stage file mapping is ambiguous.'
            }
            $source = Resolve-PartisanExistingPath `
                -Path (Join-Path $sourceAddon $name) `
                -Kind Leaf
            if (-not (Test-PartisanContainedPath `
                    -Root $sourceAddon `
                    -Candidate $source)) {
                throw 'A candidate source file escaped its sealed package root.'
            }
            Copy-Item `
                -LiteralPath $source `
                -Destination (Join-Path $stagedAddon $name) `
                -ErrorAction Stop
        }
        $stage = [pscustomobject][ordered]@{
            Magic = $script:StageMagic
            CandidateId = [string]$sealedCandidate.CandidateId
            GuardNonce = [string]$Context.Guard.Nonce
            StageRootPath = Get-PartisanFullPath -Path $stageRoot
            PackedAddonPath = Get-PartisanFullPath -Path $stagedAddon
            PackedProjectPath = Get-PartisanFullPath -Path (
                Join-Path $stagedAddon 'addon.gproj')
            AddonSearchPath = $runtimeAddon + ',' +
                (Get-PartisanFullPath -Path $stageRoot)
            PackageSha256 = [string]$sealedCandidate.PackageSha256
            CandidateBindingSha256 = Get-PartisanCandidateBindingSha256 `
                -Candidate $sealedCandidate
        }
        $bindingPath = Get-PartisanFullPath -Path (
            Join-Path $Context.Guard.Directory $script:CandidateBindingLeaf)
        if (Test-Path -LiteralPath $bindingPath) {
            throw 'The sealed candidate binding path is not fresh.'
        }
        $Context.CandidateBindingPath = $bindingPath
        $Context.CandidateBindingSha256 = [string]$stage.CandidateBindingSha256
        $Context.CandidateBindingSignature = Write-PartisanPortableJson `
            -Path $bindingPath `
            -Value $sealedCandidate
        $Context.Stage = $stage
        [void](Assert-PartisanCandidateStage `
            -Context $Context `
            -Stage $stage)
        return $stage
    }
    catch {
        $Context.Stage = $null
        # A failed legacy staging attempt has no sealed exact inventory.  Its
        # partial children are therefore evidence and are preserved intact.
        throw
    }
}

function Get-PartisanProcessIdentity {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][ValidateRange(1, 2147483647)][int]$ProcessId)

    $process = Get-Process -Id $ProcessId -ErrorAction Stop
    $startUtc = $process.StartTime.ToUniversalTime()
    $rows = @(Get-CimInstance `
        Win32_Process `
        -Filter "ProcessId=$ProcessId" `
        -ErrorAction Stop)
    if ($rows.Count -ne 1) {
        throw 'A guarded process identity could not be resolved exactly.'
    }
    $row = $rows[0]
    $executablePath = [string]$row.ExecutablePath
    if ([string]::IsNullOrWhiteSpace($executablePath)) {
        $executablePath =
            [Partisan.GuardedRuntime.NativeInspection]::QueryImagePath($ProcessId)
    }
    $commandLine = [string]$row.CommandLine
    if ([string]::IsNullOrWhiteSpace($executablePath) -or
        [string]::IsNullOrWhiteSpace($commandLine)) {
        throw 'A guarded process lacks an inspectable executable or argument vector.'
    }
    $tokens = @(
        [Partisan.GuardedRuntime.NativeInspection]::SplitCommandLine($commandLine))
    if ($tokens.Count -eq 0) {
        throw 'A guarded process argument vector is empty.'
    }
    $resolvedExecutable = Get-PartisanFullPath -Path $executablePath
    $tokenValue = [string]$tokens[0]
    if ($tokenValue.StartsWith('\??\', [StringComparison]::Ordinal)) {
        $tokenValue = $tokenValue.Substring(4)
    }
    elseif ($tokenValue.StartsWith('\\?\', [StringComparison]::Ordinal)) {
        $tokenValue = $tokenValue.Substring(4)
    }
    $tokenExecutable = if ([IO.Path]::IsPathRooted($tokenValue)) {
        Get-PartisanFullPath -Path $tokenValue
    }
    elseif ($tokenValue.Equals(
            [IO.Path]::GetFileName($resolvedExecutable),
            [StringComparison]::OrdinalIgnoreCase)) {
        $resolvedExecutable
    }
    else {
        Get-PartisanFullPath -Path $tokenValue
    }
    if (-not $tokenExecutable.Equals(
            $resolvedExecutable,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw 'A guarded process executable differs from argv[0].'
    }
    $arguments = if ($tokens.Count -gt 1) {
        @($tokens[1..($tokens.Count - 1)] | ForEach-Object { [string]$_ })
    }
    else {
        @()
    }
    return [pscustomobject][ordered]@{
        ProcessId = $ProcessId
        ParentProcessId = [int]$row.ParentProcessId
        StartUtc = $startUtc
        ExecutablePath = $resolvedExecutable
        Arguments = [string[]]$arguments
        CommandLine = $commandLine
    }
}

function Test-PartisanProcessIdentity {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Expected,
        [Parameter(Mandatory = $true)]$Actual
    )

    try {
        foreach ($value in @($Expected, $Actual)) {
            Assert-PartisanProperties `
                -Value $value `
                -Names @('ProcessId', 'StartUtc', 'ExecutablePath', 'Arguments') `
                -Label 'Process identity'
        }
        if ([int]$Expected.ProcessId -ne [int]$Actual.ProcessId -or
            ([datetime]$Expected.StartUtc).ToUniversalTime().Ticks -ne
                ([datetime]$Actual.StartUtc).ToUniversalTime().Ticks -or
            -not (Get-PartisanFullPath -Path ([string]$Expected.ExecutablePath)).Equals(
                (Get-PartisanFullPath -Path ([string]$Actual.ExecutablePath)),
                [StringComparison]::OrdinalIgnoreCase)) {
            return $false
        }
        $expectedArguments = @($Expected.Arguments)
        $actualArguments = @($Actual.Arguments)
        if ($expectedArguments.Count -ne $actualArguments.Count) {
            return $false
        }
        for ($index = 0; $index -lt $expectedArguments.Count; $index++) {
            if (-not ([string]$expectedArguments[$index]).Equals(
                    [string]$actualArguments[$index],
                    [StringComparison]::Ordinal)) {
                return $false
            }
        }
        return $true
    }
    catch {
        return $false
    }
}

function Get-PartisanProcessObjectState {
    param([Parameter(Mandatory = $true)]$Process)

    try {
        $Process.Refresh()
        if ($Process.HasExited) {
            return 'dead'
        }
        return 'alive'
    }
    catch {
        return 'unknown'
    }
}

function Get-PartisanProcessIdentityStatusCore {
    param(
        [Parameter(Mandatory = $true)]$Identity,
        [Parameter(Mandatory = $true)]$Process,
        [Parameter(Mandatory = $true)][scriptblock]$IdentityInspector
    )

    Assert-PartisanProperties `
        -Value $Identity `
        -Names @('ProcessId', 'StartUtc', 'ExecutablePath', 'CommandLine') `
        -Label 'Guarded process identity status input'
    $state = Get-PartisanProcessObjectState -Process $Process
    if ($state -ceq 'dead') {
        return [pscustomobject][ordered]@{
            Status = 'dead'
            Reason = 'process-has-exited'
            Actual = $null
        }
    }
    if ($state -ceq 'unknown') {
        return [pscustomobject][ordered]@{
            Status = 'unknown'
            Reason = 'process-state-inspection-failed'
            Actual = $null
        }
    }

    try {
        $actual = & $IdentityInspector ([int]$Identity.ProcessId)
        if (-not (Test-PartisanProcessIdentity `
                -Expected $Identity `
                -Actual $actual)) {
            return [pscustomobject][ordered]@{
                Status = 'unknown'
                Reason = 'identity-mismatch'
                Actual = $actual
            }
        }
        return [pscustomobject][ordered]@{
            Status = 'alive'
            Reason = 'exact-identity'
            Actual = $actual
        }
    }
    catch {
        if ((Get-PartisanProcessObjectState -Process $Process) -ceq 'dead') {
            return [pscustomobject][ordered]@{
                Status = 'dead'
                Reason = 'process-exited-during-identity-inspection'
                Actual = $null
            }
        }
        return [pscustomobject][ordered]@{
            Status = 'unknown'
            Reason = 'identity-inspection-failed'
            Actual = $null
        }
    }
}

function Get-PartisanProcessIdentityStatus {
    param([Parameter(Mandatory = $true)]$Identity)

    Assert-PartisanProperties `
        -Value $Identity `
        -Names @('ProcessId', 'StartUtc', 'ExecutablePath', 'CommandLine') `
        -Label 'Guarded process identity status input'
    $process = Get-Process `
        -Id ([int]$Identity.ProcessId) `
        -ErrorAction SilentlyContinue
    if ($null -eq $process) {
        return [pscustomobject][ordered]@{
            Status = 'dead'
            Reason = 'pid-absent'
            Actual = $null
        }
    }
    return Get-PartisanProcessIdentityStatusCore `
        -Identity $Identity `
        -Process $process `
        -IdentityInspector {
            param([int]$TargetProcessId)
            Get-PartisanProcessIdentity -ProcessId $TargetProcessId
        }
}

function Add-PartisanPermanentFailure {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$Phase,
        [string]$Evidence = 'guarded safety invariant failed'
    )

    if ($Phase -cnotmatch '^[a-z0-9][a-z0-9:._-]{0,127}$') {
        throw 'A guarded permanent-failure phase is invalid.'
    }
    if ([string]::IsNullOrWhiteSpace($Evidence) -or $Evidence.Length -gt 1024) {
        throw 'Guarded permanent-failure evidence is invalid.'
    }
    $Context.CertificationInvalidated = $true
    $Context.State = 'teardown-failed'
    if (-not $Context.TeardownFailures.Contains($Phase)) {
        [void]$Context.TeardownFailures.Add($Phase)
    }
    $liveGuard = Get-PartisanGuardOwnership `
        -Directory ([string]$Context.Guard.Directory) `
        -GuardBase ([string]$Context.Guard.GuardBase)
    if ($null -eq $liveGuard -or
        $liveGuard.Nonce -cne [string]$Context.ContextId -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $Context.Guard.SentinelSignature `
            -Actual $liveGuard.SentinelSignature)) {
        $Context.PermanentFailureLedgerFault = 'guard-ownership-unavailable'
        throw 'Permanent NO-GO evidence could not be written without exact guard ownership.'
    }
    $ledgerPath = Get-PartisanFullPath -Path (
        Join-Path $Context.Guard.Directory $script:PermanentFailureLeaf)
    if (-not (Test-PartisanContainedPath `
            -Root $Context.Guard.Directory `
            -Candidate $ledgerPath)) {
        $Context.PermanentFailureLedgerFault = 'ledger-path-escaped'
        throw 'Permanent NO-GO evidence path escaped the exact guard.'
    }
    $entries = New-Object Collections.Generic.List[object]
    if (Test-Path -LiteralPath $ledgerPath -PathType Leaf) {
        if ($null -eq $Context.PermanentFailureLedgerSignature) {
            $Context.PermanentFailureLedgerFault = 'unbound-existing-ledger'
            throw 'An unbound permanent NO-GO ledger already exists.'
        }
        $actualSignature = Get-PartisanFileSignature -Path $ledgerPath
        if (-not (Test-PartisanFileSignatureExact `
                -Expected $Context.PermanentFailureLedgerSignature `
                -Actual $actualSignature)) {
            $Context.PermanentFailureLedgerFault = 'ledger-bytes-changed'
            throw 'The permanent NO-GO ledger bytes changed.'
        }
        $existing = Read-PartisanPortableJson -Path $ledgerPath
        Assert-PartisanProperties `
            -Value $existing `
            -Names @('version', 'contextId', 'certification', 'failures') `
            -Label 'Permanent NO-GO ledger'
        if ([int]$existing.version -ne 1 -or
            [string]$existing.contextId -cne [string]$Context.ContextId -or
            [string]$existing.certification -cne 'permanent-no-go') {
            $Context.PermanentFailureLedgerFault = 'ledger-identity-changed'
            throw 'The permanent NO-GO ledger identity changed.'
        }
        foreach ($entry in @($existing.failures)) {
            [void]$entries.Add($entry)
        }
    }
    elseif ($null -ne $Context.PermanentFailureLedgerSignature) {
        $Context.PermanentFailureLedgerFault = 'ledger-missing'
        throw 'The permanent NO-GO ledger disappeared.'
    }
    $record = [pscustomobject][ordered]@{
        sequence = $entries.Count + 1
        phase = $Phase
        evidence = $Evidence
        recordedUtc = [DateTime]::UtcNow.ToString(
            'o',
            [Globalization.CultureInfo]::InvariantCulture)
    }
    [void]$entries.Add($record)
    $Context.PermanentFailureLedgerSignature =
        Write-PartisanPortableJsonAtomic `
            -Path $ledgerPath `
            -Value ([ordered]@{
                version = 1
                contextId = [string]$Context.ContextId
                certification = 'permanent-no-go'
                failures = [object[]]$entries.ToArray()
            })
    $Context.PermanentFailureLedgerPath = $ledgerPath
    [void]$Context.PermanentFailures.Add($record)
    return $record
}

function Get-PartisanEngineProcesses {
    return @(Get-Process -ErrorAction Stop | Where-Object {
        $script:EngineProcessNames -contains $_.ProcessName
    })
}

function Get-PartisanLedgerEntry {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][int]$ProcessId
    )

    return @($Context.Ledger | Where-Object {
        [int]$_.Identity.ProcessId -eq $ProcessId
    } | Select-Object -First 1)
}

function Add-PartisanLedgerEntry {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Identity,
        [Parameter(Mandatory = $true)][string]$RootRole
    )

    $existing = @(Get-PartisanLedgerEntry `
        -Context $Context `
        -ProcessId ([int]$Identity.ProcessId))
    if ($existing.Count -ne 0) {
        if (-not (Test-PartisanProcessIdentity `
                -Expected $existing[0].Identity `
                -Actual $Identity)) {
            throw 'A guarded ledger PID was reused or changed identity.'
        }
        return $existing[0]
    }
    $entry = [pscustomobject][ordered]@{
        RootRole = $RootRole
        Identity = $Identity
        RecordedUtc = [DateTime]::UtcNow
    }
    [void]$Context.Ledger.Add($entry)
    return $entry
}

function Sync-PartisanProcessLedger {
    param([Parameter(Mandatory = $true)]$Context)

    foreach ($entry in $Context.Ledger.ToArray()) {
        $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
        if ([string]$status.Status -ceq 'unknown') {
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase 'process-identity-unknown' `
                -Evidence ([string]$status.Reason))
            throw 'A recorded guarded process identity became unknown.'
        }
    }
    if ($null -eq $Context.Job -or $Context.JobClosed) {
        return
    }
    try {
        $jobProcessIds = @($Context.Job.GetProcessIds())
    }
    catch {
        [void](Add-PartisanPermanentFailure `
            -Context $Context `
            -Phase 'job-membership-inspection' `
            -Evidence $_.Exception.Message)
        throw
    }
    $pending = New-Object Collections.Generic.List[object]
    foreach ($processId in $jobProcessIds) {
        $identity = $null
        try {
            $identity = Get-PartisanProcessIdentity -ProcessId ([int]$processId)
        }
        catch {
            if ($null -eq (Get-Process `
                    -Id ([int]$processId) `
                    -ErrorAction SilentlyContinue)) {
                continue
            }
            $inspectionEvidence = 'pid=' + [int]$processId + ';error=' +
                $_.Exception.Message
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase 'job-member-identity-unknown' `
                -Evidence $inspectionEvidence)
            throw ('A live guarded job member could not be inspected exactly (' +
                $inspectionEvidence + ').')
        }
        $existing = @(Get-PartisanLedgerEntry `
            -Context $Context `
            -ProcessId ([int]$processId))
        if ($existing.Count -ne 0) {
            if (-not (Test-PartisanProcessIdentity `
                    -Expected $existing[0].Identity `
                    -Actual $identity)) {
                [void](Add-PartisanPermanentFailure `
                    -Context $Context `
                    -Phase 'job-member-identity-mismatch' `
                    -Evidence ('pid=' + [int]$processId))
                throw 'A live guarded job member changed its bound identity.'
            }
            continue
        }
        [void]$pending.Add($identity)
    }
    $changed = $true
    while ($pending.Count -gt 0 -and $changed) {
        $changed = $false
        foreach ($identity in @($pending.ToArray())) {
            $parentEntry = @(Get-PartisanLedgerEntry `
                -Context $Context `
                -ProcessId ([int]$identity.ParentProcessId))
            if ($parentEntry.Count -eq 0) {
                continue
            }
            [void](Add-PartisanLedgerEntry `
                -Context $Context `
                -Identity $identity `
                -RootRole ([string]$parentEntry[0].RootRole))
            [void]$pending.Remove($identity)
            $changed = $true
        }
    }
    foreach ($identity in @($pending.ToArray())) {
        [void](Add-PartisanLedgerEntry `
            -Context $Context `
            -Identity $identity `
            -RootRole 'job-owned')
    }
}

function Assert-PartisanEngineOwnership {
    param([Parameter(Mandatory = $true)]$Context)

    Sync-PartisanProcessLedger -Context $Context
    foreach ($process in @(Get-PartisanEngineProcesses)) {
        try {
            $actual = Get-PartisanProcessIdentity -ProcessId ([int]$process.Id)
        }
        catch {
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase 'engine-identity-unknown' `
                -Evidence ('pid=' + [int]$process.Id))
            throw 'An engine process identity could not be inspected exactly.'
        }
        $entry = @(Get-PartisanLedgerEntry `
            -Context $Context `
            -ProcessId ([int]$process.Id))
        if ($entry.Count -ne 1 -or
            -not (Test-PartisanProcessIdentity `
                -Expected $entry[0].Identity `
                -Actual $actual)) {
            $description = $process.ProcessName + ':' + $process.Id
            [void]$Context.UnclaimedEngines.Add($description)
        }
    }
    if ($Context.UnclaimedEngines.Count -ne 0) {
        [void](Add-PartisanPermanentFailure `
            -Context $Context `
            -Phase 'unclaimed-engine-process' `
            -Evidence (($Context.UnclaimedEngines | Sort-Object) -join ';'))
        throw 'An engine process exists outside the exact guarded job ledger.'
    }
}

function Assert-PartisanRuntimeContext {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string[]]$AllowedStates
    )

    Assert-PartisanProperties `
        -Value $Context `
        -Names @(
            'Magic',
            'ContextId',
            'State',
            'Guard',
            'Job',
            'JobClosed',
            'Ledger',
            'Server',
            'Clients',
            'Stage',
            'CandidateBindingPath',
            'CandidateBindingSignature',
            'CandidateBindingSha256',
            'BoundarySnapshots',
            'PortAvailabilityChecks',
            'UnclaimedEngines',
            'CertificationInvalidated',
            'TeardownFailures',
            'PermanentFailures',
            'PermanentFailureLedgerPath',
            'PermanentFailureLedgerSignature',
            'PermanentFailureLedgerFault',
            'OwnershipAuditCount') `
        -Label 'Guarded runtime context'
    if ([string]$Context.Magic -cne $script:ContextMagic -or
        [string]$Context.ContextId -cnotmatch '^[0-9a-f]{32}$' -or
        $AllowedStates -cnotcontains [string]$Context.State -or
        ([bool]$Context.CertificationInvalidated -and
            [string]$Context.State -ceq 'active')) {
        throw 'The guarded runtime context identity or state is invalid.'
    }
    Assert-PartisanProperties `
        -Value $Context.Guard `
        -Names @(
            'Directory',
            'GuardBase',
            'Nonce',
            'Purpose',
            'OwnerPid',
            'OwnerStartUtc',
            'SentinelSignature') `
        -Label 'Guarded runtime context ownership'
    if ([string]$Context.ContextId -cne [string]$Context.Guard.Nonce) {
        throw 'The guarded runtime context is not bound to its ownership nonce.'
    }
    $currentOwner = Get-Process -Id $PID -ErrorAction Stop
    $currentOwnerStartUtc = $currentOwner.StartTime.ToUniversalTime()
    if ([int]$Context.Guard.OwnerPid -ne $PID -or
        ([datetime]$Context.Guard.OwnerStartUtc).ToUniversalTime().Ticks -ne
            $currentOwnerStartUtc.Ticks) {
        throw 'The guarded runtime context is not owned by this exact PowerShell process.'
    }
    $liveGuard = Get-PartisanGuardOwnership `
        -Directory ([string]$Context.Guard.Directory) `
        -GuardBase ([string]$Context.Guard.GuardBase)
    if ($null -eq $liveGuard -or
        $liveGuard.Nonce -cne [string]$Context.Guard.Nonce -or
        $liveGuard.Purpose -cne [string]$Context.Guard.Purpose -or
        $liveGuard.OwnerPid -ne [int]$Context.Guard.OwnerPid -or
        $liveGuard.OwnerStartUtc.Ticks -ne
            ([datetime]$Context.Guard.OwnerStartUtc).ToUniversalTime().Ticks -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $Context.Guard.SentinelSignature `
            -Actual $liveGuard.SentinelSignature)) {
        throw 'The live guard sentinel differs from the context ownership binding.'
    }
    $candidateBindingParts = @(
        (-not [string]::IsNullOrWhiteSpace(
            [string]$Context.CandidateBindingPath))
        ($null -ne $Context.CandidateBindingSignature)
        (-not [string]::IsNullOrWhiteSpace(
            [string]$Context.CandidateBindingSha256)))
    $candidateBindingPartCount = @($candidateBindingParts | Where-Object { $_ }).Count
    if (($null -eq $Context.Stage -and $candidateBindingPartCount -ne 0) -or
        ($null -ne $Context.Stage -and $candidateBindingPartCount -ne 3)) {
        throw 'The guarded runtime context has an incomplete candidate binding.'
    }
    if ($null -ne $Context.PermanentFailureLedgerSignature) {
        $ledgerPath = Resolve-PartisanExistingPath `
            -Path ([string]$Context.PermanentFailureLedgerPath) `
            -Kind Leaf
        $expectedLedgerPath = Get-PartisanFullPath -Path (
            Join-Path $Context.Guard.Directory $script:PermanentFailureLeaf)
        if (-not $ledgerPath.Equals(
                $expectedLedgerPath,
                [StringComparison]::OrdinalIgnoreCase) -or
            -not (Test-PartisanFileSignatureExact `
                -Expected $Context.PermanentFailureLedgerSignature `
                -Actual (Get-PartisanFileSignature -Path $ledgerPath))) {
            throw 'The permanent NO-GO ledger is not byte-exact and guard-bound.'
        }
    }
    foreach ($availability in @($Context.PortAvailabilityChecks)) {
        Assert-PartisanProperties `
            -Value $availability `
            -Names @('Port', 'Protocol', 'AddressFamily', 'Scope', 'Semantics') `
            -Label 'IPv4 loopback availability check'
        if ([string]$availability.AddressFamily -cne 'IPv4' -or
            [string]$availability.Scope -cne 'loopback' -or
            [string]$availability.Semantics -cne 'bind-availability-only') {
            throw 'A loopback port record overclaims its availability-only semantics.'
        }
    }
}

function Assert-PartisanRuntimeOwnershipAudit {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)]$Context)

    Assert-PartisanRuntimeContext `
        -Context $Context `
        -AllowedStates @('active', 'teardown-failed')
    if ($null -ne $Context.Stage) {
        try {
            [void](Assert-PartisanCandidateStage `
                -Context $Context `
                -Stage $Context.Stage)
        }
        catch {
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase 'candidate-stage-audit' `
                -Evidence $_.Exception.Message)
            throw
        }
    }
    Sync-PartisanProcessLedger -Context $Context
    Assert-PartisanEngineOwnership -Context $Context
    $Context.OwnershipAuditCount = [int]$Context.OwnershipAuditCount + 1
    return [pscustomobject][ordered]@{
        ContextId = [string]$Context.ContextId
        State = [string]$Context.State
        Certification = if ($Context.CertificationInvalidated) {
            'permanent-no-go'
        }
        else {
            'not-invalidated'
        }
        AuditCount = [int]$Context.OwnershipAuditCount
        LedgerEntries = [int]$Context.Ledger.Count
        PortSemantics = 'ipv4-loopback-bind-availability-only'
    }
}

function New-PartisanGuardedRuntimeContext {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [Parameter(Mandatory = $true)][string]$Purpose,
        [Parameter(Mandatory = $true)][string[]]$WatchedRoots,
        [Parameter(Mandatory = $true)][string[]]$SpillRoots,
        [Parameter(Mandatory = $true)][int[]]$LoopbackPorts,
        [ValidateSet('Tcp', 'Udp')][string[]]$LoopbackProtocols = @('Udp')
    )

    if ($env:OS -cne 'Windows_NT') {
        throw 'Guarded runtime process ownership requires Windows.'
    }
    if ($WatchedRoots.Count -eq 0 -or
        $SpillRoots.Count -eq 0 -or
        $LoopbackPorts.Count -eq 0 -or
        $LoopbackProtocols.Count -eq 0) {
        throw 'A guarded runtime context requires watched, spill, and loopback boundaries.'
    }
    if (@(Get-PartisanEngineProcesses).Count -ne 0) {
        throw 'A guarded runtime context requires an engine-free preflight.'
    }
    $guardBaseFull = Resolve-PartisanExistingPath `
        -Path $GuardBase `
        -Kind Container
    if ($guardBaseFull.Contains(',')) {
        throw 'The guarded runtime base must not contain a comma.'
    }
    Assert-PartisanNoReparseTree -Root $guardBaseFull
    $allRoots = @($WatchedRoots) + @($SpillRoots)
    $resolvedRoots = @($allRoots | ForEach-Object {
        Resolve-PartisanExistingPath -Path $_ -Kind Container
    })
    for ($left = 0; $left -lt $resolvedRoots.Count; $left++) {
        if (Test-PartisanPathOverlap `
                -First $guardBaseFull `
                -Second $resolvedRoots[$left]) {
            throw 'The guard base must not overlap a watched or spill root.'
        }
        for ($right = $left + 1; $right -lt $resolvedRoots.Count; $right++) {
            if (Test-PartisanPathOverlap `
                    -First $resolvedRoots[$left] `
                    -Second $resolvedRoots[$right]) {
                throw 'Watched and spill roots must be pairwise non-overlapping.'
            }
        }
    }
    $availabilityChecks = New-Object Collections.Generic.List[object]
    $seenAvailabilityChecks = New-Object Collections.Generic.HashSet[string](
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($protocol in $LoopbackProtocols) {
        foreach ($port in $LoopbackPorts) {
            if ($port -lt 1 -or $port -gt 65535) {
                throw 'A guarded loopback port is outside its valid range.'
            }
            $key = $protocol + ':' + $port
            if (-not $seenAvailabilityChecks.Add($key)) {
                throw 'An IPv4 loopback availability check was supplied more than once.'
            }
            [void]$availabilityChecks.Add((Assert-PartisanLoopbackPortAvailable `
                -Port $port `
                -Protocol $protocol))
        }
    }
    $snapshots = New-PartisanBoundarySnapshotSet `
        -WatchedRoots $WatchedRoots `
        -SpillRoots $SpillRoots
    $guard = $null
    $job = $null
    try {
        $guard = New-PartisanGuardDirectory `
            -GuardBase $guardBaseFull `
            -Purpose $Purpose
        $job = New-Object Partisan.GuardedRuntime.NativeJob
        if (@(Get-PartisanEngineProcesses).Count -ne 0) {
            throw 'An engine process appeared during guarded context creation.'
        }
        return [pscustomobject][ordered]@{
            Magic = $script:ContextMagic
            ContextId = [string]$guard.Nonce
            State = 'active'
            Guard = $guard
            GuardInventory = $null
            GuardRemoved = $false
            Job = $job
            JobClosed = $false
            Ledger = New-Object Collections.Generic.List[object]
            Server = $null
            Clients = New-Object Collections.Generic.List[object]
            Stage = $null
            CandidateBindingPath = $null
            CandidateBindingSignature = $null
            CandidateBindingSha256 = $null
            BoundarySnapshots = [object[]]$snapshots
            LastBoundaryDelta = $null
            PortAvailabilityChecks = $availabilityChecks.ToArray()
            UnclaimedEngines = New-Object Collections.Generic.HashSet[string](
                [StringComparer]::Ordinal)
            CertificationInvalidated = $false
            TeardownFailures = New-Object Collections.Generic.HashSet[string](
                [StringComparer]::Ordinal)
            PermanentFailures = New-Object Collections.Generic.List[object]
            PermanentFailureLedgerPath = $null
            PermanentFailureLedgerSignature = $null
            PermanentFailureLedgerFault = $null
            OwnershipAuditCount = 0
        }
    }
    catch {
        if ($job) {
            try { $job.Dispose() } catch { }
        }
        if ($guard) {
            try { [void](Remove-PartisanGuardDirectory -Ownership $guard) } catch { }
        }
        throw
    }
}

function Test-PartisanEngineExecutable {
    param([Parameter(Mandatory = $true)][string]$Path)

    $baseName = [IO.Path]::GetFileNameWithoutExtension($Path)
    return $script:EngineProcessNames -contains $baseName
}

function Assert-PartisanCandidateConsumption {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$CandidateConsumption
    )

    if ($null -eq $Context.Stage -or
        -not [object]::ReferenceEquals(
            $CandidateConsumption,
            $Context.Stage)) {
        throw 'An engine launch requires the exact in-context candidate stage binding.'
    }
    [void](Assert-PartisanCandidateStage `
        -Context $Context `
        -Stage $CandidateConsumption)
}

function Start-PartisanGuardedRole {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$RootRole,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$ExecutableProvenance,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [AllowNull()]$CandidateConsumption,
        [switch]$NonEngineSelfTestOnly,
        [switch]$NonEngineSelfTestForcePostResumeFailure
    )

    Assert-PartisanRuntimeContext -Context $Context -AllowedStates @('active')
    $executablePath = Resolve-PartisanExistingPath -Path $Executable -Kind Leaf
    $workingPath = Resolve-PartisanExistingPath `
        -Path $WorkingDirectory `
        -Kind Container
    Assert-PartisanNoReparseAncestry -Path $executablePath
    Assert-PartisanNoReparseAncestry -Path $workingPath
    $isEngine = Test-PartisanEngineExecutable -Path $executablePath
    if ($isEngine) {
        if ($NonEngineSelfTestOnly -or $NonEngineSelfTestForcePostResumeFailure) {
            throw 'Engine launches cannot use a non-engine self-test bypass.'
        }
        Assert-PartisanCandidateConsumption `
            -Context $Context `
            -CandidateConsumption $CandidateConsumption
    }
    else {
        if (-not $NonEngineSelfTestOnly -or
            [string]$Context.Guard.Purpose -cnotmatch '^self_test_') {
            throw 'A non-engine guarded launch is permitted only as an explicit self-test.'
        }
        if ($NonEngineSelfTestForcePostResumeFailure -and
            -not $NonEngineSelfTestOnly) {
            throw 'A post-resume failure injection is restricted to non-engine self-tests.'
        }
    }
    if ($null -eq $Context.Stage) {
        throw 'Every guarded launch requires a sealed candidate stage.'
    }
    [void](Assert-PartisanCandidateStage `
        -Context $Context `
        -Stage $Context.Stage)
    [void](Assert-PartisanExecutableProvenance `
        -Expected $ExecutableProvenance `
        -Path $executablePath)
    [void](Assert-PartisanRuntimeOwnershipAudit -Context $Context)
    $commandLine = ConvertTo-PartisanNativeCommandLine `
        -Executable $executablePath `
        -Arguments $Arguments
    if (-not (Test-PartisanExactNativeArgumentVector `
            -CommandLine $commandLine `
            -ExpectedExecutable $executablePath `
            -ExpectedArguments $Arguments)) {
        throw 'The guarded native argument vector did not round-trip exactly.'
    }
    $suspended = $null
    $process = $null
    $identity = $null
    $resumed = $false
    try {
        $suspended = New-Object Partisan.GuardedRuntime.SuspendedProcess(
            $executablePath,
            $commandLine,
            $workingPath)
        $process = $suspended.Child
        if ($null -eq $process) {
            throw 'The guarded suspended process was not created.'
        }
        $Context.Job.Add($process)
        $identity = Get-PartisanProcessIdentity -ProcessId ([int]$process.Id)
        if (-not (Test-PartisanExactNativeArgumentVector `
                -CommandLine ([string]$identity.CommandLine) `
                -ExpectedExecutable $executablePath `
                -ExpectedArguments $Arguments)) {
            throw 'The suspended guarded process has a non-exact argument vector.'
        }
        [void](Add-PartisanLedgerEntry `
            -Context $Context `
            -Identity $identity `
            -RootRole $RootRole)
        $suspended.Resume()
        $resumed = $true
        $suspended.Dispose()
        $suspended = $null
        Start-Sleep -Milliseconds 150
        if ($NonEngineSelfTestForcePostResumeFailure) {
            Start-Sleep -Milliseconds 500
        }
        Sync-PartisanProcessLedger -Context $Context
        if ($NonEngineSelfTestForcePostResumeFailure) {
            throw 'Injected non-engine self-test failure after process resume.'
        }
        $process.Refresh()
        if ($process.HasExited) {
            throw 'The guarded process exited before post-resume identity validation.'
        }
        $postIdentity = Get-PartisanProcessIdentity -ProcessId ([int]$process.Id)
        if (-not (Test-PartisanProcessIdentity `
                -Expected $identity `
                -Actual $postIdentity)) {
            throw 'The guarded process identity changed during launch.'
        }
        [void](Assert-PartisanExecutableProvenance `
            -Expected $ExecutableProvenance `
            -Path $executablePath)
        [void](Assert-PartisanRuntimeOwnershipAudit -Context $Context)
        return [pscustomobject][ordered]@{
            ContextId = [string]$Context.ContextId
            RootRole = $RootRole
            Process = $process
            RootIdentity = $identity
            ExecutableProvenance = $ExecutableProvenance
            CandidateBindingSha256 = [string]$Context.CandidateBindingSha256
            CandidateConsumption = if ($isEngine) {
                'exact-stage-binding-required'
            }
            else {
                'non-engine-self-test-only'
            }
            StartedUtc = [DateTime]::UtcNow
            ExitCode = $null
            Stopped = $false
        }
    }
    catch {
        $launchError = $_
        if ($resumed) {
            try {
                [void](Add-PartisanPermanentFailure `
                    -Context $Context `
                    -Phase ('post-resume-launch:' + $RootRole) `
                    -Evidence $launchError.Exception.Message)
            }
            catch { }
            if ($identity) {
                try {
                    Sync-PartisanProcessLedger -Context $Context
                    [void](Stop-PartisanLedgerRole `
                        -Context $Context `
                        -RootRole $RootRole `
                        -TimeoutSeconds 15)
                }
                catch {
                    try {
                        [void](Add-PartisanPermanentFailure `
                            -Context $Context `
                            -Phase ('post-resume-cleanup:' + $RootRole) `
                            -Evidence $_.Exception.Message)
                    }
                    catch { }
                }
            }
        }
        throw $launchError
    }
    finally {
        if ($suspended) {
            $suspended.Dispose()
        }
    }
}

function Start-PartisanGuardedServer {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$ExecutableProvenance,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [AllowNull()]$CandidateConsumption,
        [switch]$NonEngineSelfTestOnly,
        [switch]$NonEngineSelfTestForcePostResumeFailure
    )

    Assert-PartisanRuntimeContext -Context $Context -AllowedStates @('active')
    if ($null -ne $Context.Server) {
        throw 'The guarded runtime context already has a server launch.'
    }
    foreach ($availability in @($Context.PortAvailabilityChecks)) {
        [void](Assert-PartisanLoopbackPortAvailable `
            -Port ([int]$availability.Port) `
            -Protocol ([string]$availability.Protocol))
    }
    $launch = Start-PartisanGuardedRole `
        -Context $Context `
        -RootRole 'server' `
        -Executable $Executable `
        -ExecutableProvenance $ExecutableProvenance `
        -Arguments $Arguments `
        -WorkingDirectory $WorkingDirectory `
        -CandidateConsumption $CandidateConsumption `
        -NonEngineSelfTestOnly:$NonEngineSelfTestOnly `
        -NonEngineSelfTestForcePostResumeFailure:$NonEngineSelfTestForcePostResumeFailure
    $Context.Server = $launch
    return $launch
}

function Start-PartisanGuardedClient {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$ExecutableProvenance,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [AllowNull()]$CandidateConsumption,
        [switch]$NonEngineSelfTestOnly,
        [switch]$NonEngineSelfTestForcePostResumeFailure
    )

    Assert-PartisanRuntimeContext -Context $Context -AllowedStates @('active')
    if ($null -eq $Context.Server) {
        throw 'A guarded client requires its exact live server identity.'
    }
    $serverStatus = Get-PartisanProcessIdentityStatus `
        -Identity $Context.Server.RootIdentity
    if ([string]$serverStatus.Status -ceq 'unknown') {
        [void](Add-PartisanPermanentFailure `
            -Context $Context `
            -Phase 'server-identity-unknown-before-client' `
            -Evidence ([string]$serverStatus.Reason))
        throw 'The guarded server identity is unknown before client launch.'
    }
    if ([string]$serverStatus.Status -cne 'alive') {
        throw 'A guarded client requires its exact live server identity.'
    }
    $role = 'client-' + ($Context.Clients.Count + 1)
    $launch = Start-PartisanGuardedRole `
        -Context $Context `
        -RootRole $role `
        -Executable $Executable `
        -ExecutableProvenance $ExecutableProvenance `
        -Arguments $Arguments `
        -WorkingDirectory $WorkingDirectory `
        -CandidateConsumption $CandidateConsumption `
        -NonEngineSelfTestOnly:$NonEngineSelfTestOnly `
        -NonEngineSelfTestForcePostResumeFailure:$NonEngineSelfTestForcePostResumeFailure
    [void]$Context.Clients.Add($launch)
    return $launch
}

function Assert-PartisanLaunchBinding {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Launch
    )

    Assert-PartisanProperties `
        -Value $Launch `
        -Names @(
            'ContextId',
            'RootRole',
            'Process',
            'RootIdentity',
            'Stopped',
            'CandidateBindingSha256') `
        -Label 'Guarded process launch'
    if ([string]$Launch.ContextId -cne [string]$Context.ContextId -or
        [string]$Launch.RootRole -cnotmatch '^(server|client-[1-9][0-9]*)$' -or
        [string]$Launch.CandidateBindingSha256 -cne
            [string]$Context.CandidateBindingSha256) {
        throw 'A guarded process launch is not bound to this context.'
    }
    $bound = if ([string]$Launch.RootRole -ceq 'server') {
        $null -ne $Context.Server -and
            [object]::ReferenceEquals($Launch, $Context.Server)
    }
    else {
        @($Context.Clients | Where-Object {
            [object]::ReferenceEquals($_, $Launch)
        }).Count -eq 1
    }
    if (-not $bound) {
        throw 'Only the exact in-context launch object is accepted.'
    }
}

function Wait-PartisanGuardedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Launch,
        [ValidateRange(1, 86400)][int]$TimeoutSeconds = 600,
        [ValidateRange(20, 5000)][int]$PollMilliseconds = 250,
        [switch]$RequireZeroExit
    )

    Assert-PartisanRuntimeContext -Context $Context -AllowedStates @('active')
    Assert-PartisanLaunchBinding -Context $Context -Launch $Launch
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        [void](Assert-PartisanRuntimeOwnershipAudit -Context $Context)
        $identityStatus = Get-PartisanProcessIdentityStatus `
            -Identity $Launch.RootIdentity
        if ([string]$identityStatus.Status -ceq 'unknown') {
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase ('wait-identity-unknown:' + $Launch.RootRole) `
                -Evidence ([string]$identityStatus.Reason))
            throw 'A guarded process identity became unknown while waiting.'
        }
        if ([string]$identityStatus.Status -ceq 'dead') {
            $Launch.Process.WaitForExit()
            $Launch.ExitCode = [int]$Launch.Process.ExitCode
            if ($RequireZeroExit -and $Launch.ExitCode -ne 0) {
                throw 'A guarded process returned a nonzero exit code.'
            }
            return [pscustomobject][ordered]@{
                RootRole = [string]$Launch.RootRole
                ExitCode = [int]$Launch.ExitCode
                ElapsedSeconds = [Math]::Round(
                    ([DateTime]::UtcNow - $Launch.StartedUtc).TotalSeconds,
                    3)
            }
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
    throw 'A guarded process exceeded its exact wait deadline.'
}

function Stop-PartisanLedgerRole {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$RootRole,
        [ValidateRange(1, 120)][int]$TimeoutSeconds = 15
    )

    if ($RootRole -cnotmatch '^(server|client-[1-9][0-9]*)$') {
        throw 'A guarded ledger role is invalid.'
    }
    Sync-PartisanProcessLedger -Context $Context
    $roleEntries = @($Context.Ledger | Where-Object {
        [string]$_.RootRole -ceq $RootRole
    } | Sort-Object { [int]$_.Identity.ProcessId } -Descending)
    foreach ($entry in $roleEntries) {
        $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
        if ([string]$status.Status -ceq 'unknown') {
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase ('stop-identity-unknown:' + $RootRole) `
                -Evidence ([string]$status.Reason))
            throw 'A guarded process identity was unknown during exact termination.'
        }
        if ([string]$status.Status -ceq 'dead') {
            continue
        }
        try {
            (Get-Process `
                -Id ([int]$entry.Identity.ProcessId) `
                -ErrorAction Stop).Kill()
        }
        catch {
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase ('stop-kill-failed:' + $RootRole) `
                -Evidence $_.Exception.Message)
            throw
        }
    }
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $lastUnknownReason = $null
    while ([DateTime]::UtcNow -lt $deadline) {
        $liveCount = 0
        $unknownCount = 0
        foreach ($entry in $roleEntries) {
            $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
            if ([string]$status.Status -ceq 'unknown') {
                $unknownCount++
                $lastUnknownReason = [string]$status.Reason
            }
            elseif ([string]$status.Status -ceq 'alive') {
                $liveCount++
            }
        }
        if ($liveCount -eq 0 -and $unknownCount -eq 0) {
            return $true
        }
        Start-Sleep -Milliseconds 100
    }
    if (-not [string]::IsNullOrWhiteSpace($lastUnknownReason)) {
        [void](Add-PartisanPermanentFailure `
            -Context $Context `
            -Phase ('stop-poll-identity-unknown:' + $RootRole) `
            -Evidence $lastUnknownReason)
        throw 'A guarded process identity remained unknown through its stop deadline.'
    }
    [void](Add-PartisanPermanentFailure `
        -Context $Context `
        -Phase ('stop-timeout:' + $RootRole) `
        -Evidence 'exact role remained alive past the stop deadline')
    throw 'A guarded process role did not stop within its deadline.'
}

function Stop-PartisanGuardedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Launch,
        [ValidateRange(1, 120)][int]$TimeoutSeconds = 15
    )

    Assert-PartisanRuntimeContext `
        -Context $Context `
        -AllowedStates @('active', 'teardown-failed')
    Assert-PartisanLaunchBinding -Context $Context -Launch $Launch
    Sync-PartisanProcessLedger -Context $Context
    if ([string]$Launch.RootRole -ceq 'server') {
        foreach ($entry in @($Context.Ledger | Where-Object {
            ([string]$_.RootRole -like 'client-*' -or
                [string]$_.RootRole -ceq 'job-owned')
        })) {
            $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
            if ([string]$status.Status -ceq 'unknown') {
                [void](Add-PartisanPermanentFailure `
                    -Context $Context `
                    -Phase 'server-stop-dependent-identity-unknown' `
                    -Evidence ([string]$status.Reason))
                throw 'A client or unclassified job member identity is unknown.'
            }
            if ([string]$status.Status -ceq 'alive') {
                if ([string]$entry.RootRole -ceq 'job-owned') {
                    [void](Add-PartisanPermanentFailure `
                        -Context $Context `
                        -Phase 'server-stop-unclassified-job-member' `
                        -Evidence ('pid=' + [int]$entry.Identity.ProcessId))
                }
                throw 'All guarded clients and unclassified job members must stop before the server.'
            }
        }
    }
    [void](Stop-PartisanLedgerRole `
        -Context $Context `
        -RootRole ([string]$Launch.RootRole) `
        -TimeoutSeconds $TimeoutSeconds)
    $Launch.Stopped = $true
    try {
        $Launch.Process.Refresh()
        if ($Launch.Process.HasExited) {
            $Launch.ExitCode = [int]$Launch.Process.ExitCode
        }
    }
    catch { }
    [void](Assert-PartisanRuntimeOwnershipAudit -Context $Context)
    return $true
}

function Invoke-PartisanV2TeardownCertificationCheck {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)][string]$Identifier,
        [Parameter(Mandatory = $true)][scriptblock]$Action
    )

    try {
        & $Action
        return $true
    }
    catch {
        if (-not $Record.PermanentNoGo) { throw }
        Set-PartisanV2PermanentNoGo `
            -Record $Record `
            -Identifier $Identifier `
            -Message $_.Exception.Message
        return $false
    }
}

function Invoke-PartisanGuardedTeardown {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [ValidateRange(1, 120)][int]$StopTimeoutSeconds = 15,
        [ValidateRange(1, 120)][int]$PortReleaseTimeoutSeconds = 15
    )

    Assert-PartisanRuntimeContext `
        -Context $Context `
        -AllowedStates @('active', 'teardown-failed')
    $Context.State = 'teardown-failed'
    $errors = New-Object Collections.Generic.List[string]
    $clientsStoppedBeforeServer = $false
    $processesVerifiedStopped = $false
    $portsBindAvailable = $false

    # Client-role shutdown is a hard ordering barrier. If any client or
    # unclassified job member is live or unknown, neither the server nor the
    # kill-on-close job handle is touched.
    $clientsToStop = $Context.Clients.ToArray()
    [array]::Reverse($clientsToStop)
    foreach ($client in $clientsToStop) {
        try {
            [void](Stop-PartisanGuardedProcess `
                -Context $Context `
                -Launch $client `
                -TimeoutSeconds $StopTimeoutSeconds)
        }
        catch {
            [void]$errors.Add('stop-client')
        }
    }
    try {
        Sync-PartisanProcessLedger -Context $Context
        foreach ($entry in @($Context.Ledger | Where-Object {
            [string]$_.RootRole -like 'client-*' -or
                [string]$_.RootRole -ceq 'job-owned'
        })) {
            $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
            if ([string]$status.Status -ceq 'unknown') {
                [void]$errors.Add('client-or-job-member-identity-unknown')
            }
            elseif ([string]$status.Status -ceq 'alive') {
                [void]$errors.Add('client-or-job-member-remains')
            }
        }
    }
    catch {
        [void]$errors.Add('sync-after-client-stop')
    }
    $clientBarrierClear = @($errors | Where-Object {
        $_ -like '*client*' -or $_ -like '*job-member*' -or
            $_ -ceq 'sync-after-client-stop'
    }).Count -eq 0
    if ($clientBarrierClear) {
        $clientsStoppedBeforeServer = $true
    }
    if ($clientBarrierClear -and $null -ne $Context.Server) {
        try {
            [void](Stop-PartisanGuardedProcess `
                -Context $Context `
                -Launch $Context.Server `
                -TimeoutSeconds $StopTimeoutSeconds)
        }
        catch {
            [void]$errors.Add('stop-server')
        }
    }
    elseif (-not $clientBarrierClear -and $null -ne $Context.Server) {
        [void]$errors.Add('server-retained-by-client-ordering-barrier')
    }

    if ($clientBarrierClear) {
        try {
            Sync-PartisanProcessLedger -Context $Context
            foreach ($entry in $Context.Ledger.ToArray()) {
                $status = Get-PartisanProcessIdentityStatus `
                    -Identity $entry.Identity
                if ([string]$status.Status -ceq 'unknown') {
                    [void]$errors.Add('owned-process-identity-unknown')
                }
                elseif ([string]$status.Status -ceq 'alive') {
                    [void]$errors.Add('owned-process-remains')
                }
            }
        }
        catch {
            [void]$errors.Add('verify-owned-processes')
        }
    }

    $processErrors = @($errors | Where-Object {
        $_ -like '*process*' -or $_ -like '*client*' -or
            $_ -like '*server*' -or $_ -like '*job-member*'
    })
    if ($processErrors.Count -eq 0) {
        $processesVerifiedStopped = $true
        if (-not $Context.JobClosed -and $null -ne $Context.Job) {
            try {
                $jobMembers = @($Context.Job.GetProcessIds())
                if ($jobMembers.Count -ne 0) {
                    [void]$errors.Add('job-not-empty-before-close')
                    $processesVerifiedStopped = $false
                }
            }
            catch {
                [void]$errors.Add('job-membership-unknown-before-close')
                $processesVerifiedStopped = $false
            }
            if ($processesVerifiedStopped) {
                try {
                    $Context.Job.Dispose()
                    $Context.JobClosed = $true
                    $Context.Job = $null
                }
                catch {
                    [void]$errors.Add('close-empty-job')
                    $processesVerifiedStopped = $false
                }
            }
        }
    }

    try {
        if (@(Get-PartisanEngineProcesses).Count -ne 0 -or
            $Context.UnclaimedEngines.Count -ne 0) {
            [void]$errors.Add('unclaimed-engine-processes')
        }
    }
    catch {
        [void]$errors.Add('verify-engine-processes')
    }

    if ($processesVerifiedStopped) {
        $portsBindAvailable = $true
        foreach ($availability in @($Context.PortAvailabilityChecks)) {
            try {
                [void](Wait-PartisanLoopbackPortReleased `
                    -Port ([int]$availability.Port) `
                    -Protocol ([string]$availability.Protocol) `
                    -TimeoutSeconds $PortReleaseTimeoutSeconds)
            }
            catch {
                $portsBindAvailable = $false
                [void]$errors.Add('ipv4-loopback-not-bind-available')
            }
        }
    }

    if (($null -eq $Context.Stage) -xor
        [string]::IsNullOrWhiteSpace([string]$Context.CandidateBindingPath)) {
        [void]$errors.Add('candidate-stage-integrity')
    }
    elseif ($null -ne $Context.Stage) {
        if (-not (Test-Path `
                -LiteralPath $Context.Stage.StageRootPath `
                -PathType Container)) {
            [void]$errors.Add('candidate-stage-integrity')
        }
        else {
            try {
                [void](Assert-PartisanCandidateStage `
                    -Context $Context `
                    -Stage $Context.Stage)
            }
            catch {
                [void]$errors.Add('candidate-stage-integrity')
            }
        }
    }
    try {
        $boundaryDelta = Compare-PartisanBoundarySnapshotSet `
            -Snapshots @($Context.BoundarySnapshots)
        $Context.LastBoundaryDelta = $boundaryDelta
        if (-not $boundaryDelta.Clean) {
            [void]$errors.Add(('boundary-delta:{0}:{1}:{2}:{3}' -f
                $boundaryDelta.NewEntries,
                $boundaryDelta.ModifiedEntries,
                $boundaryDelta.DeletedEntries,
                $boundaryDelta.MissingRoots))
        }
    }
    catch {
        [void]$errors.Add('verify-boundaries')
    }

    $uniqueErrors = @($errors.ToArray() | Sort-Object -Unique)
    foreach ($phase in $uniqueErrors) {
        try {
            [void](Add-PartisanPermanentFailure `
                -Context $Context `
                -Phase $phase `
                -Evidence 'guarded teardown failed closed')
        }
        catch {
            if ([string]::IsNullOrWhiteSpace(
                    [string]$Context.PermanentFailureLedgerFault)) {
                $Context.PermanentFailureLedgerFault = 'ledger-write-failed'
            }
        }
    }
    $destructiveCleanupAuthorized = $errors.Count -eq 0 -and
        -not [bool]$Context.CertificationInvalidated
    if ($destructiveCleanupAuthorized) {
        try {
            [void](Remove-PartisanGuardDirectory -Ownership $Context.Guard)
            $Context.GuardRemoved = $true
        }
        catch {
            [void]$errors.Add('remove-guard')
            try {
                [void](Add-PartisanPermanentFailure `
                    -Context $Context `
                    -Phase 'remove-guard' `
                    -Evidence $_.Exception.Message)
            }
            catch {
                $Context.CertificationInvalidated = $true
                $Context.State = 'teardown-failed'
                [void]$Context.TeardownFailures.Add('remove-guard')
            }
        }
        if ($null -ne $Context.Stage -and
            (Test-Path -LiteralPath $Context.Stage.StageRootPath)) {
            [void]$errors.Add('candidate-stage-remains')
        }
        if (Test-Path -LiteralPath $Context.Guard.Directory) {
            [void]$errors.Add('guard-remains')
        }
    }
    else {
        if (-not (Test-Path `
                -LiteralPath $Context.Guard.Directory `
                -PathType Container)) {
            [void]$errors.Add('failed-guard-retention')
        }
        if ($null -ne $Context.Stage -and
            -not (Test-Path `
                -LiteralPath $Context.Stage.StageRootPath `
                -PathType Container)) {
            [void]$errors.Add('failed-stage-retention')
        }
    }
    if ($processesVerifiedStopped) {
        $launchesToDispose = New-Object Collections.Generic.List[object]
        foreach ($client in $Context.Clients.ToArray()) {
            [void]$launchesToDispose.Add($client)
        }
        if ($null -ne $Context.Server) {
            [void]$launchesToDispose.Add($Context.Server)
        }
        foreach ($launch in $launchesToDispose.ToArray()) {
            try {
                if ($launch.Process) {
                    $launch.Process.Dispose()
                }
            }
            catch {
                [void]$errors.Add('dispose-process-handle')
            }
        }
    }
    if ($errors.Count -ne 0 -or $Context.CertificationInvalidated) {
        $reported = @($Context.TeardownFailures | Sort-Object -Unique)
        if ($reported.Count -eq 0) {
            $reported = @('historical-permanent-no-go')
        }
        throw ('Guarded runtime teardown failed closed (phases=' +
            ($reported -join ',') + ').')
    }
    $Context.State = 'closed'
    return [pscustomobject][ordered]@{
        ContextId = [string]$Context.ContextId
        ClientsStoppedBeforeServer = [bool]$clientsStoppedBeforeServer
        JobClosed = [bool]$Context.JobClosed
        OwnedProcessesRemaining = 0
        UnclaimedEnginesRemaining = 0
        GuardRemoved = [bool]$Context.GuardRemoved
        StageRemoved = $null -eq $Context.Stage -or
            -not (Test-Path -LiteralPath $Context.Stage.StageRootPath)
        IPv4LoopbackPortsBindAvailable = [bool]$portsBindAvailable
        PortSemantics = 'availability-only-no-pid-attribution'
        BoundariesClean = [bool]$Context.LastBoundaryDelta.Clean
    }
}

# Authoritative v2 runtime. Public objects below are projections only. Every
# process handle, job, baseline, identity, stage, and lifecycle decision lives
# in the module-private nonce registry.

function Get-PartisanV2Digest {
    param(
        [AllowNull()]
        [AllowEmptyCollection()]
        [Parameter(Mandatory = $true)]$Value
    )

    $canonical = ConvertTo-PartisanCanonicalValue -Value $Value
    return Get-PartisanSha256Text `
        -Text (($canonical | ConvertTo-Json -Compress -Depth 64) + "`n")
}

function New-PartisanV2PortableJsonArtifact {
    param([Parameter(Mandatory = $true)]$Value)

    $canonical = ConvertTo-PartisanCanonicalValue -Value $Value
    $text = (($canonical | ConvertTo-Json -Depth 32).Replace(
            "`r`n",
            "`n") + "`n")
    $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($text)
    return [pscustomobject][ordered]@{
        Text = $text
        Bytes = [byte[]]$bytes
        Signature = [pscustomobject][ordered]@{
            length = [long]$bytes.Length
            sha256 = Get-PartisanSha256Text -Text $text
        }
    }
}

function Write-PartisanV2CreationFaultJournal {
    param(
        [Parameter(Mandatory = $true)]$Guard,
        [Parameter(Mandatory = $true)][string]$CreationError,
        [Parameter(Mandatory = $true)][string[]]$CleanupErrors
    )

    $prefix = '.PartisanGuardedRuntime_' + [string]$Guard.Nonce
    $primary = Join-Path $Guard.GuardBase ($prefix + '.journal.json')
    $fault = Join-Path $Guard.GuardBase ($prefix + '.journal-fault.json')
    $target = $null
    foreach ($candidate in @($fault, $primary)) {
        if (-not (Test-Path -LiteralPath $candidate)) {
            $target = Get-PartisanFullPath -Path $candidate
            break
        }
    }
    if ([string]::IsNullOrWhiteSpace([string]$target)) {
        for ($attempt = 1; $attempt -le 32; $attempt++) {
            $candidate = Join-Path `
                $Guard.GuardBase `
                ($prefix + '.journal-overflow-' +
                    [Guid]::NewGuid().ToString('N') + '.json')
            if (-not (Test-Path -LiteralPath $candidate)) {
                $target = Get-PartisanFullPath -Path $candidate
                break
            }
        }
    }
    if ([string]::IsNullOrWhiteSpace([string]$target)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_CREATE_FAULT_JOURNAL_COLLISION' `
            -Message 'Creation cleanup failed and no fresh evidence path was available.'
    }
    $signature = Write-PartisanPortableJsonAtomic `
        -Path $target `
        -Value ([ordered]@{
            version = 2
            magic = $script:RuntimeJournalMagic
            nonce = [string]$Guard.Nonce
            mode = 'permanent-no-go'
            state = 'creating'
            failure = 'PGR_CREATE_CLEANUP_FAILED'
            creationError = $CreationError
            cleanupErrors = [string[]]$CleanupErrors
            recordedUtc = [DateTime]::UtcNow.ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
        })
    return [pscustomobject][ordered]@{
        Path = $target
        Signature = $signature
    }
}

function Copy-PartisanV2PlainValue {
    param($Value)

    if ($null -eq $Value) {
        return $null
    }
    $canonical = ConvertTo-PartisanCanonicalValue -Value $Value
    return (($canonical | ConvertTo-Json -Compress -Depth 64) |
        ConvertFrom-Json)
}

function Get-PartisanV2GuardRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$GuardDirectory,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $guardFull = Get-PartisanFullPath -Path $GuardDirectory
    $pathFull = Get-PartisanFullPath -Path $Path
    if (-not (Test-PartisanContainedPath `
            -Root $guardFull `
            -Candidate $pathFull) -or
        $pathFull.Equals($guardFull, [StringComparison]::OrdinalIgnoreCase)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_GUARD_INVENTORY_ESCAPE' `
            -Message 'Guard inventory path escaped its exact guard.'
    }
    return $pathFull.Substring(
        $guardFull.TrimEnd('\', '/').Length + 1).Replace('\', '/')
}

function Assert-PartisanV2GuardInventoryExact {
    param(
        [Parameter(Mandatory = $true)][string]$GuardDirectory,
        [Parameter(Mandatory = $true)][object[]]$ExpectedInventory
    )

    $guardFull = Resolve-PartisanExistingPath `
        -Path $GuardDirectory `
        -Kind Container
    Assert-PartisanNoReparseTree -Root $guardFull
    $expected = New-Object 'Collections.Generic.Dictionary[string,object]' (
        [StringComparer]::Ordinal)
    foreach ($entry in $ExpectedInventory) {
        Assert-PartisanProperties `
            -Value $entry `
            -Names @('relativePath', 'kind', 'signature') `
            -Label 'Sealed guard inventory entry'
        $relative = [string]$entry.relativePath
        if ([string]::IsNullOrWhiteSpace($relative) -or
            $relative.Contains('\') -or
            $relative.StartsWith('/', [StringComparison]::Ordinal) -or
            $relative.Split('/') -contains '..' -or
            $expected.ContainsKey($relative)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_GUARD_INVENTORY_SCHEMA' `
                -Message 'Sealed guard inventory is ambiguous or noncanonical.'
        }
        $expected.Add($relative, $entry)
    }
    $items = @(Get-ChildItem `
        -LiteralPath $guardFull `
        -Recurse `
        -Force `
        -ErrorAction Stop)
    if ($items.Count -ne $expected.Count) {
        Throw-PartisanV2 `
            -Identifier 'PGR_GUARD_INVENTORY_MISMATCH' `
            -Message 'Guard contains missing or unknown children; all children are preserved.'
    }
    foreach ($item in $items) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            Throw-PartisanV2 `
                -Identifier 'PGR_GUARD_INVENTORY_REPARSE' `
                -Message 'Guard inventory contains a reparse point.'
        }
        $relative = Get-PartisanV2GuardRelativePath `
            -GuardDirectory $guardFull `
            -Path $item.FullName
        if (-not $expected.ContainsKey($relative)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_GUARD_INVENTORY_UNKNOWN' `
                -Message 'Unknown guard child is preserved and rejects cleanup.'
        }
        $sealed = $expected[$relative]
        if ([string]$sealed.kind -ceq 'directory') {
            if (-not $item.PSIsContainer) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_GUARD_INVENTORY_KIND' `
                    -Message 'A sealed guard directory changed kind.'
            }
        }
        elseif ([string]$sealed.kind -ceq 'file') {
            if ($item.PSIsContainer -or $null -eq $sealed.signature -or
                -not (Test-PartisanFileSignatureExact `
                    -Expected $sealed.signature `
                    -Actual (Get-PartisanFileSignature -Path $item.FullName))) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_GUARD_INVENTORY_SIGNATURE' `
                    -Message 'A sealed guard file changed bytes or kind.'
            }
        }
        else {
            Throw-PartisanV2 `
                -Identifier 'PGR_GUARD_INVENTORY_KIND' `
                -Message 'A sealed guard inventory kind is invalid.'
        }
    }
    return $true
}

function New-PartisanV2GuardInventory {
    param([Parameter(Mandatory = $true)]$Record)

    $inventory = New-Object Collections.Generic.List[object]
    [void]$inventory.Add([pscustomobject][ordered]@{
        relativePath = $script:GuardSentinelLeaf
        kind = 'file'
        signature = Copy-PartisanV2PlainValue `
            -Value $Record.Guard.SentinelSignature
    })
    if ($Record.Stage) {
        [void]$inventory.Add([pscustomobject][ordered]@{
            relativePath = $script:CandidateBindingLeaf
            kind = 'file'
            signature = Copy-PartisanV2PlainValue `
                -Value $Record.CandidateBindingSignature
        })
        [void]$inventory.Add([pscustomobject][ordered]@{
            relativePath = 'candidate-addons'
            kind = 'directory'
            signature = $null
        })
        [void]$inventory.Add([pscustomobject][ordered]@{
            relativePath = 'candidate-addons/Partisan'
            kind = 'directory'
            signature = $null
        })
        foreach ($row in @($Record.Candidate.PackageFiles)) {
            $name = Split-Path -Leaf ([string]$row.indexPath)
            $path = Join-Path $Record.Stage.PackedAddonPath $name
            [void]$inventory.Add([pscustomobject][ordered]@{
                relativePath = 'candidate-addons/Partisan/' + $name
                kind = 'file'
                signature = Copy-PartisanV2PlainValue `
                    -Value (Get-PartisanFileSignature -Path $path)
            })
        }
    }
    return [object[]]$inventory.ToArray()
}

function Throw-PartisanV2 {
    param(
        [Parameter(Mandatory = $true)][string]$Identifier,
        [Parameter(Mandatory = $true)][string]$Message
    )

    throw ('[' + $Identifier + '] ' + $Message)
}

function New-PartisanV2PublicLaunch {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)]$Launch
    )

    $values = [ordered]@{
        ContextId = [string]$Record.Nonce
        RootRole = [string]$Launch.Role
        Process = [pscustomobject][ordered]@{
            ProcessId = [int]$Launch.Identity.ProcessId
        }
        RootIdentity = Copy-PartisanV2PlainValue -Value $Launch.Identity
        ExecutablePath = [string]$Launch.ExecutablePath
        ExecutableProvenance = Copy-PartisanV2PlainValue `
            -Value $Launch.ExecutableProvenance
        Arguments = [string[]]$Launch.Arguments
        IsEngine = [bool]$Launch.IsEngine
        CandidateConsumptionEvidence = Copy-PartisanV2PlainValue `
            -Value $Launch.CandidateConsumptionEvidence
        CandidateBindingSha256 = [string]$Record.CandidateBindingSha256
        StartedUtc = ([datetime]$Launch.StartedUtc).ToString(
            'o',
            [Globalization.CultureInfo]::InvariantCulture)
        ExitCode = $Launch.ExitCode
        Stopped = [bool]$Launch.Stopped
    }
    if ($null -eq $Launch.PublicLaunch) {
        return [pscustomobject]$values
    }
    foreach ($name in $values.Keys) {
        $Launch.PublicLaunch.$name = $values[$name]
    }
    return $Launch.PublicLaunch
}

function Update-PartisanV2PublicProjection {
    param([Parameter(Mandatory = $true)]$Record)

    $context = $Record.PublicContext
    $guardProjection = [pscustomobject][ordered]@{
        Directory = [string]$Record.Guard.Directory
        GuardBase = [string]$Record.Guard.GuardBase
        Sentinel = [string]$Record.Guard.Sentinel
        Nonce = [string]$Record.Guard.Nonce
        Purpose = [string]$Record.Guard.Purpose
        OwnerPid = [int]$Record.Guard.OwnerPid
        OwnerStartUtc = ([datetime]$Record.Guard.OwnerStartUtc).ToString(
            'o',
            [Globalization.CultureInfo]::InvariantCulture)
        SentinelSignature = Copy-PartisanV2PlainValue `
            -Value $Record.Guard.SentinelSignature
    }
    $ledgerProjection = @($Record.Ledger.ToArray() | ForEach-Object {
        [pscustomobject][ordered]@{
            RootRole = [string]$_.Role
            Identity = Copy-PartisanV2PlainValue -Value $_.Identity
            RecordedUtc = ([datetime]$_.RecordedUtc).ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
        }
    })
    $boundaryProjection = @($Record.Boundaries | ForEach-Object {
        [pscustomobject][ordered]@{
            Root = [string]$_.Root
            Kind = [string]$_.Kind
            BaselineDigest = Get-PartisanV2Digest -Value $_.Entries
        }
    })
    $portProjection = @($Record.Ports | ForEach-Object {
        Copy-PartisanV2PlainValue -Value $_
    })
    $clientProjection = @($Record.Clients.ToArray() | ForEach-Object {
        $_.PublicLaunch
    })
    $values = [ordered]@{
        Magic = $script:RuntimeV2Magic
        ContextId = [string]$Record.Nonce
        State = [string]$Record.State
        CertificationInvalidated = [bool]$Record.PermanentNoGo
        Guard = $guardProjection
        GuardRemoved = [bool]$Record.GuardRemoved
        Job = [pscustomobject][ordered]@{
            Nonce = [string]$Record.Nonce
            Closed = [bool]$Record.JobClosed
        }
        JobClosed = [bool]$Record.JobClosed
        Ledger = [object[]]$ledgerProjection
        Server = if ($Record.Server) { $Record.Server.PublicLaunch } else { $null }
        Clients = [object[]]$clientProjection
        Stage = if ($Record.Stage) { $Record.Stage.PublicStage } else { $null }
        BoundarySnapshots = [object[]]$boundaryProjection
        PortAvailabilityChecks = [object[]]$portProjection
        CandidateBindingSha256 = $Record.CandidateBindingSha256
        FailureJournalExpectedPath = [string]$Record.JournalPath
        FailureJournalFaultExpectedPath = [string]$Record.JournalFaultPath
        FailureJournalPath = [string]$Record.JournalActivePath
        FailureJournalSignature = Copy-PartisanV2PlainValue `
            -Value $Record.JournalSignature
        CleanReceiptPath = [string]$Record.ReceiptPath
        CleanReceiptSignature = Copy-PartisanV2PlainValue `
            -Value $Record.ReceiptSignature
        CompletionAttestationPath = [string]$Record.CompletionAttestationPath
        CompletionAttestationSignature = Copy-PartisanV2PlainValue `
            -Value $Record.CompletionAttestationSignature
        PermanentFailures = [object[]]@($Record.Failures.ToArray() |
            ForEach-Object { Copy-PartisanV2PlainValue -Value $_ })
        OmittedPermanentFailureCount = [long]$Record.OmittedFailureCount
        PermanentFailureSequence = [long]$Record.FailureSequence
        OwnershipAuditCount = [int]$Record.AuditCount
    }
    foreach ($name in $values.Keys) {
        if (@($context.PSObject.Properties | ForEach-Object { $_.Name }) `
                -contains $name) {
            $context.$name = $values[$name]
        }
        else {
            Add-Member `
                -InputObject $context `
                -MemberType NoteProperty `
                -Name $name `
                -Value $values[$name]
        }
    }
    $launchReferences = New-Object Collections.Generic.List[object]
    $privateLaunches = New-Object Collections.Generic.List[object]
    if ($Record.Server) { [void]$privateLaunches.Add($Record.Server) }
    foreach ($client in $Record.Clients.ToArray()) {
        [void]$privateLaunches.Add($client)
    }
    foreach ($launch in $privateLaunches.ToArray()) {
        [void]$launchReferences.Add([pscustomobject][ordered]@{
            PublicLaunch = $launch.PublicLaunch
            Process = $launch.PublicLaunch.Process
            RootIdentity = $launch.PublicLaunch.RootIdentity
            ExecutableProvenance = $launch.PublicLaunch.ExecutableProvenance
            Arguments = $launch.PublicLaunch.Arguments
            CandidateConsumptionEvidence =
                $launch.PublicLaunch.CandidateConsumptionEvidence
        })
    }
    $Record.ProjectionReferences = [pscustomobject][ordered]@{
        Guard = $context.Guard
        GuardSentinelSignature = $context.Guard.SentinelSignature
        Job = $context.Job
        Ledger = $context.Ledger
        Server = $context.Server
        Clients = $context.Clients
        Stage = $context.Stage
        BoundarySnapshots = $context.BoundarySnapshots
        PortAvailabilityChecks = $context.PortAvailabilityChecks
        FailureJournalSignature = $context.FailureJournalSignature
        CleanReceiptSignature = $context.CleanReceiptSignature
        CompletionAttestationSignature =
            $context.CompletionAttestationSignature
        PermanentFailures = $context.PermanentFailures
        Launches = [object[]]$launchReferences.ToArray()
    }
    $Record.PublicDigest = Get-PartisanV2Digest -Value $context
}

function Test-PartisanV2PublicProjectionExact {
    param([Parameter(Mandatory = $true)]$Record)

    try {
        $context = $Record.PublicContext
        $references = $Record.ProjectionReferences
        if ($null -eq $references -or
            -not [object]::ReferenceEquals($context.Guard, $references.Guard) -or
            -not [object]::ReferenceEquals(
                $context.Guard.SentinelSignature,
                $references.GuardSentinelSignature) -or
            -not [object]::ReferenceEquals($context.Job, $references.Job) -or
            -not [object]::ReferenceEquals($context.Ledger, $references.Ledger) -or
            -not [object]::ReferenceEquals($context.Server, $references.Server) -or
            -not [object]::ReferenceEquals($context.Clients, $references.Clients) -or
            -not [object]::ReferenceEquals($context.Stage, $references.Stage) -or
            -not [object]::ReferenceEquals(
                $context.BoundarySnapshots,
                $references.BoundarySnapshots) -or
            -not [object]::ReferenceEquals(
                $context.PortAvailabilityChecks,
                $references.PortAvailabilityChecks) -or
            -not [object]::ReferenceEquals(
                $context.FailureJournalSignature,
                $references.FailureJournalSignature) -or
            -not [object]::ReferenceEquals(
                $context.CleanReceiptSignature,
                $references.CleanReceiptSignature) -or
            -not [object]::ReferenceEquals(
                $context.CompletionAttestationSignature,
                $references.CompletionAttestationSignature) -or
            -not [object]::ReferenceEquals(
                $context.PermanentFailures,
                $references.PermanentFailures)) {
            return $false
        }
        $privateLaunches = New-Object Collections.Generic.List[object]
        if ($Record.Server) { [void]$privateLaunches.Add($Record.Server) }
        foreach ($client in $Record.Clients.ToArray()) {
            [void]$privateLaunches.Add($client)
        }
        if ($privateLaunches.Count -ne @($references.Launches).Count) {
            return $false
        }
        for ($index = 0; $index -lt $privateLaunches.Count; $index++) {
            $launch = $privateLaunches[$index]
            $expected = @($references.Launches)[$index]
            if (-not [object]::ReferenceEquals(
                    $launch.PublicLaunch,
                    $expected.PublicLaunch) -or
                -not [object]::ReferenceEquals(
                    $launch.PublicLaunch.Process,
                    $expected.Process) -or
                -not [object]::ReferenceEquals(
                    $launch.PublicLaunch.RootIdentity,
                    $expected.RootIdentity) -or
                -not [object]::ReferenceEquals(
                    $launch.PublicLaunch.ExecutableProvenance,
                    $expected.ExecutableProvenance) -or
                -not [object]::ReferenceEquals(
                    $launch.PublicLaunch.Arguments,
                    $expected.Arguments) -or
                -not [object]::ReferenceEquals(
                    $launch.PublicLaunch.CandidateConsumptionEvidence,
                    $expected.CandidateConsumptionEvidence)) {
                return $false
            }
        }
        return (Get-PartisanV2Digest -Value $context) -ceq
            [string]$Record.PublicDigest
    }
    catch {
        return $false
    }
}

function Write-PartisanV2Journal {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)]
        [ValidateSet(
            'teardown-running',
            'guard-removal-authorized',
            'permanent-no-go')][string]$Mode
    )

    $preferred = if ($Mode -ceq 'permanent-no-go' -and
        [string]$Record.JournalMode -ceq 'guard-removal-authorized') {
        [string]$Record.JournalFaultPath
    }
    else { [string]$Record.JournalPath }
    $candidates = @()
    if ($Mode -ceq [string]$Record.JournalMode -and
        -not [string]::IsNullOrWhiteSpace(
            [string]$Record.JournalActivePath)) {
        $candidates += [string]$Record.JournalActivePath
    }
    if ($candidates -cnotcontains $preferred) { $candidates += $preferred }
    if ($preferred -cne [string]$Record.JournalFaultPath) {
        $candidates += [string]$Record.JournalFaultPath
    }
    $target = $null
    foreach ($candidate in $candidates) {
        if (-not (Test-Path -LiteralPath $candidate)) {
            $target = $candidate
            break
        }
        if ([string]$Record.JournalActivePath -ceq $candidate -and
            $null -ne $Record.JournalSignature -and
            (Test-Path -LiteralPath $candidate -PathType Leaf) -and
            (Test-PartisanFileSignatureExact `
                -Expected $Record.JournalSignature `
                -Actual (Get-PartisanFileSignature -Path $candidate))) {
            $target = $candidate
            break
        }
    }
    if ([string]::IsNullOrWhiteSpace([string]$target)) {
        for ($attempt = 1; $attempt -le 32; $attempt++) {
            $overflow = Join-Path `
                $Record.Guard.GuardBase `
                ('.PartisanGuardedRuntime_' + $Record.Nonce +
                    '.journal-overflow-' + [Guid]::NewGuid().ToString('N') +
                    '.json')
            if (-not (Test-Path -LiteralPath $overflow)) {
                $target = Get-PartisanFullPath -Path $overflow
                [void]$Record.JournalOverflowPaths.Add($target)
                break
            }
        }
    }
    if ([string]::IsNullOrWhiteSpace([string]$target)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_JOURNAL_COLLISION_EXHAUSTED' `
            -Message 'No fresh collision-safe external journal path was available.'
    }
    $value = [ordered]@{
        version = 2
        magic = $script:RuntimeJournalMagic
        nonce = [string]$Record.Nonce
        guardBindingSha256 = [string]$Record.GuardBindingSha256
        mode = $Mode
        state = [string]$Record.State
        recordedUtc = [DateTime]::UtcNow.ToString(
            'o',
            [Globalization.CultureInfo]::InvariantCulture)
        failures = [object[]]$Record.Failures.ToArray()
        failureSequence = [long]$Record.FailureSequence
        omittedFailureCount = [long]$Record.OmittedFailureCount
        lastOmittedFailure = Copy-PartisanV2PlainValue `
            -Value $Record.LastOmittedFailure
    }
    $Record.JournalSignature = Write-PartisanPortableJsonAtomic `
        -Path $target `
        -Value $value
    $Record.JournalActivePath = $target
    $Record.JournalMode = $Mode
}

function Set-PartisanV2PermanentNoGo {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)][string]$Identifier,
        [Parameter(Mandatory = $true)][string]$Message,
        [switch]$SkipJournal
    )

    if ($Record.State -ceq 'closed') {
        return
    }
    $Record.State = 'permanent-no-go'
    $Record.PermanentNoGo = $true
    $Record.FailureSequence = [long]$Record.FailureSequence + 1
    $boundedMessage = $Message.Replace("`r", ' ').Replace("`n", ' ')
    if ($boundedMessage.Length -gt $script:MaximumPermanentFailureMessageChars) {
        $boundedMessage = $boundedMessage.Substring(
            0,
            $script:MaximumPermanentFailureMessageChars)
    }
    $failureKey = $Identifier + ':' +
        (Get-PartisanSha256Text -Text ($boundedMessage + "`n"))
    $recordedUtc = [DateTime]::UtcNow.ToString(
        'o',
        [Globalization.CultureInfo]::InvariantCulture)
    if ($Record.FailureIndex.ContainsKey($failureKey)) {
        $existing = $Record.FailureIndex[$failureKey]
        $existing.repeatCount = [long]$existing.repeatCount + 1
        $existing.lastRecordedUtc = $recordedUtc
    }
    elseif ($Record.Failures.Count -lt $script:MaximumPermanentFailures) {
        $failure = [pscustomobject][ordered]@{
            sequence = [long]$Record.FailureSequence
            identifier = $Identifier
            message = $boundedMessage
            repeatCount = [long]1
            firstRecordedUtc = $recordedUtc
            lastRecordedUtc = $recordedUtc
        }
        [void]$Record.Failures.Add($failure)
        $Record.FailureIndex.Add($failureKey, $failure)
    }
    else {
        $Record.OmittedFailureCount = [long]$Record.OmittedFailureCount + 1
        $Record.LastOmittedFailure = [pscustomobject][ordered]@{
            sequence = [long]$Record.FailureSequence
            identifier = $Identifier
            messageSha256 = Get-PartisanSha256Text -Text ($boundedMessage + "`n")
            recordedUtc = $recordedUtc
        }
    }
    if (-not $SkipJournal) {
        try {
            Write-PartisanV2Journal -Record $Record -Mode 'permanent-no-go'
        }
        catch {
            $Record.JournalWriteFault = $_.Exception.Message
        }
    }
    try { Update-PartisanV2PublicProjection -Record $Record } catch { }
}

function Assert-PartisanV2ExternalEvidence {
    param([Parameter(Mandatory = $true)]$Record)

    $primaryExists = Test-Path -LiteralPath $Record.JournalPath -PathType Leaf
    $faultExists = Test-Path -LiteralPath $Record.JournalFaultPath -PathType Leaf
    $receiptExists = Test-Path -LiteralPath $Record.ReceiptPath -PathType Leaf
    $attestationExists = Test-Path `
        -LiteralPath $Record.CompletionAttestationPath `
        -PathType Leaf
    $overflowEvidence = @(Get-ChildItem `
        -LiteralPath $Record.Guard.GuardBase `
        -File `
        -Filter ('.PartisanGuardedRuntime_' + $Record.Nonce +
            '.journal-overflow-*.json') `
        -ErrorAction Stop)
    if ([string]::IsNullOrWhiteSpace([string]$Record.JournalMode)) {
        if ($primaryExists -or $faultExists -or $receiptExists -or
            $attestationExists -or $overflowEvidence.Count -ne 0) {
            Set-PartisanV2PermanentNoGo `
                -Record $Record `
                -Identifier 'PGR_UNEXPECTED_EXTERNAL_EVIDENCE' `
                -Message 'Unexpected nonce-bound external journal or receipt discovered.' `
                -SkipJournal:$Record.FinalTransactionStarted
            Throw-PartisanV2 `
                -Identifier 'PGR_UNEXPECTED_EXTERNAL_EVIDENCE' `
                -Message 'Unexpected nonce-bound external journal discovered.'
        }
        return
    }
    if ($attestationExists -and -not $Record.FinalTransactionStarted) {
        Set-PartisanV2PermanentNoGo `
            -Record $Record `
            -Identifier 'PGR_UNEXPECTED_COMPLETION_ATTESTATION' `
            -Message 'Completion attestation appeared before final guard removal.' `
            -SkipJournal:$Record.FinalTransactionStarted
        Throw-PartisanV2 `
            -Identifier 'PGR_UNEXPECTED_COMPLETION_ATTESTATION' `
            -Message 'Completion attestation appeared before final guard removal.'
    }
    $path = [string]$Record.JournalActivePath
    if ([string]::IsNullOrWhiteSpace($path) -or
        -not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Set-PartisanV2PermanentNoGo `
            -Record $Record `
            -Identifier 'PGR_EXPECTED_JOURNAL_MISSING' `
            -Message 'Expected nonce-bound external journal is missing.' `
            -SkipJournal:$Record.FinalTransactionStarted
        Throw-PartisanV2 `
            -Identifier 'PGR_EXPECTED_JOURNAL_MISSING' `
            -Message 'Expected nonce-bound external journal is missing.'
    }
    $signature = Get-PartisanFileSignature -Path $path
    if ($null -eq $Record.JournalSignature -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $Record.JournalSignature `
            -Actual $signature)) {
        Set-PartisanV2PermanentNoGo `
            -Record $Record `
            -Identifier 'PGR_EXTERNAL_JOURNAL_MISMATCH' `
            -Message 'Nonce-bound external journal bytes changed.' `
            -SkipJournal:$Record.FinalTransactionStarted
        Throw-PartisanV2 `
            -Identifier 'PGR_EXTERNAL_JOURNAL_MISMATCH' `
            -Message 'Nonce-bound external journal bytes changed.'
    }
    $journal = Read-PartisanPortableJson -Path $path
    if ([string]$journal.magic -cne $script:RuntimeJournalMagic -or
        [string]$journal.nonce -cne [string]$Record.Nonce -or
        [string]$journal.guardBindingSha256 -cne
            [string]$Record.GuardBindingSha256 -or
        [string]$journal.mode -cne [string]$Record.JournalMode) {
        Set-PartisanV2PermanentNoGo `
            -Record $Record `
            -Identifier 'PGR_EXTERNAL_JOURNAL_IDENTITY' `
            -Message 'Nonce-bound external journal identity changed.' `
            -SkipJournal:$Record.FinalTransactionStarted
        Throw-PartisanV2 `
            -Identifier 'PGR_EXTERNAL_JOURNAL_IDENTITY' `
            -Message 'Nonce-bound external journal identity changed.'
    }
}

function Assert-PartisanV2GuardExact {
    param([Parameter(Mandatory = $true)]$Record)

    $legacyNoGoPath = Join-Path `
        $Record.Guard.Directory `
        $script:PermanentFailureLeaf
    if (Test-Path -LiteralPath $legacyNoGoPath) {
        Throw-PartisanV2 `
            -Identifier 'PGR_LEGACY_LEDGER_UNEXPECTED' `
            -Message 'Unexpected legacy in-guard permanent NO-GO ledger discovered.'
    }
    $live = Get-PartisanGuardOwnership `
        -Directory $Record.Guard.Directory `
        -GuardBase $Record.Guard.GuardBase
    if ($null -eq $live -or
        [string]$live.Nonce -cne [string]$Record.Guard.Nonce -or
        [string]$live.Purpose -cne [string]$Record.Guard.Purpose -or
        [int]$live.OwnerPid -ne [int]$Record.Guard.OwnerPid -or
        ([datetime]$live.OwnerStartUtc).Ticks -ne
            ([datetime]$Record.Guard.OwnerStartUtc).Ticks -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $Record.Guard.SentinelSignature `
            -Actual $live.SentinelSignature)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_GUARD_IDENTITY_MISMATCH' `
            -Message 'Expected exact guard or sentinel is missing or partial.'
    }
    [void](Assert-PartisanV2GuardInventoryExact `
        -GuardDirectory $Record.Guard.Directory `
        -ExpectedInventory ([object[]]$Record.GuardInventory))
}

function Find-PartisanV2Record {
    param($Context)

    foreach ($record in $script:RuntimeRegistry.Values) {
        if ([object]::ReferenceEquals($Context, $record.PublicContext)) {
            return $record
        }
    }
    if ($null -ne $Context -and
        @($Context.PSObject.Properties | ForEach-Object { $_.Name }) `
            -contains 'ContextId') {
        $nonce = [string]$Context.ContextId
        if ($script:RuntimeRegistry.ContainsKey($nonce)) {
            return $script:RuntimeRegistry[$nonce]
        }
    }
    return $null
}

function Resolve-PartisanV2Record {
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$Operation,
        [Parameter(Mandatory = $true)][string[]]$AllowedStates
    )

    $record = Find-PartisanV2Record -Context $Context
    if ($null -eq $record) {
        Throw-PartisanV2 `
            -Identifier 'PGR_CONTEXT_UNREGISTERED' `
            -Message 'Runtime context is not registered in this module instance.'
    }
    try { Assert-PartisanV2ExternalEvidence -Record $record } catch { throw }
    if (-not [object]::ReferenceEquals($Context, $record.PublicContext)) {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_CONTEXT_REFERENCE_MISMATCH' `
            -Message ('Context clone rejected at API entry: ' + $Operation + '.')
        Throw-PartisanV2 `
            -Identifier 'PGR_CONTEXT_REFERENCE_MISMATCH' `
            -Message 'Only the exact registered context reference is accepted.'
    }
    if (-not (Test-PartisanV2PublicProjectionExact -Record $record)) {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_PUBLIC_PROJECTION_MUTATED' `
            -Message ('Public context projection mutated before: ' + $Operation + '.')
        Throw-PartisanV2 `
            -Identifier 'PGR_PUBLIC_PROJECTION_MUTATED' `
            -Message 'Public context fields or nested projections were mutated.'
    }
    if ($record.State -cne 'closed') {
        try { Assert-PartisanV2GuardExact -Record $record }
        catch {
            Set-PartisanV2PermanentNoGo `
                -Record $record `
                -Identifier 'PGR_GUARD_IDENTITY_MISMATCH' `
                -Message $_.Exception.Message `
                -SkipJournal:$record.FinalTransactionStarted
            if (-not ($record.PermanentNoGo -and
                    $Operation -ceq 'Invoke-PartisanGuardedTeardown')) {
                throw
            }
        }
    }
    if ($AllowedStates -cnotcontains [string]$record.State) {
        Throw-PartisanV2 `
            -Identifier 'PGR_STATE_REJECTED' `
            -Message ('Operation ' + $Operation + ' is invalid in state ' +
                $record.State + '.')
    }
    return $record
}

function Remove-PartisanGuardDirectory {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)]$Ownership)

    $directory = if ($null -ne $Ownership -and
        @($Ownership.PSObject.Properties | ForEach-Object { $_.Name }) `
            -contains 'Directory') {
        Get-PartisanFullPath -Path ([string]$Ownership.Directory)
    }
    else {
        $null
    }
    if ($directory -and $script:RegisteredGuards.ContainsKey($directory)) {
        $record = $script:RegisteredGuards[$directory]
        if ([string]$record.State -cne 'closed') {
            Throw-PartisanV2 `
                -Identifier 'PGR_REGISTERED_GUARD_PROTECTED' `
                -Message 'Registered active or permanent-NO-GO guards cannot be removed directly.'
        }
    }
    return Remove-PartisanGuardDirectoryCore -Ownership $Ownership
}

function Assert-PartisanV2StageExact {
    param([Parameter(Mandatory = $true)]$Record)

    if ($null -eq $Record.Stage -or $null -eq $Record.Candidate) {
        Throw-PartisanV2 `
            -Identifier 'PGR_STAGE_MISSING' `
            -Message 'A sealed candidate stage is required.'
    }
    $bindingPath = Resolve-PartisanExistingPath `
        -Path $Record.CandidateBindingPath `
        -Kind Leaf
    $expectedBindingPath = Get-PartisanFullPath -Path (
        Join-Path $Record.Guard.Directory $script:CandidateBindingLeaf)
    if (-not $bindingPath.Equals(
            $expectedBindingPath,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $Record.CandidateBindingSignature `
            -Actual (Get-PartisanFileSignature -Path $bindingPath))) {
        Throw-PartisanV2 `
            -Identifier 'PGR_CANDIDATE_BINDING_MISMATCH' `
            -Message 'Private sealed candidate binding is missing or changed.'
    }
    $binding = ConvertTo-PartisanSealedCandidateBinding `
        -Candidate (Read-PartisanPortableJson -Path $bindingPath)
    if ((Get-PartisanCandidateBindingSha256 -Candidate $binding) -cne
        [string]$Record.CandidateBindingSha256) {
        Throw-PartisanV2 `
            -Identifier 'PGR_CANDIDATE_BINDING_DIGEST' `
            -Message 'Private sealed candidate binding digest changed.'
    }
    $stageRoot = Resolve-PartisanExistingPath `
        -Path $Record.Stage.StageRootPath `
        -Kind Container
    $expectedStageRoot = Get-PartisanFullPath -Path (
        Join-Path $Record.Guard.Directory 'candidate-addons')
    if (-not $stageRoot.Equals(
            $expectedStageRoot,
            [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-PartisanContainedPath `
            -Root $Record.Guard.Directory `
            -Candidate $stageRoot)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_STAGE_ROOT_MISMATCH' `
            -Message 'Private stage root escaped its exact registered guard.'
    }
    Assert-PartisanNoReparseTree -Root $stageRoot
    $top = @(Get-ChildItem -LiteralPath $stageRoot -Force -ErrorAction Stop)
    if ($top.Count -ne 1 -or -not $top[0].PSIsContainer -or
        $top[0].Name -cne 'Partisan') {
        Throw-PartisanV2 `
            -Identifier 'PGR_STAGE_LAYOUT_MISMATCH' `
            -Message 'Private candidate stage layout changed.'
    }
    $addonPath = Resolve-PartisanExistingPath `
        -Path $Record.Stage.PackedAddonPath `
        -Kind Container
    $expectedNames = @($Record.Candidate.PackageFiles | ForEach-Object {
        Split-Path -Leaf ([string]$_.indexPath)
    } | Sort-Object)
    $actualNames = @(Get-ChildItem `
        -LiteralPath $addonPath `
        -Force `
        -ErrorAction Stop | ForEach-Object { $_.Name } | Sort-Object)
    if (@(Compare-Object `
            -ReferenceObject $expectedNames `
            -DifferenceObject $actualNames `
            -CaseSensitive).Count -ne 0 -or
        (Get-PartisanCandidateStageDigest `
            -Candidate $Record.Candidate `
            -StagedAddonPath $addonPath) -cne
                [string]$Record.Candidate.PackageSha256) {
        Throw-PartisanV2 `
            -Identifier 'PGR_STAGE_CONTENT_MISMATCH' `
            -Message 'Private staged package differs from its sealed inventory.'
    }
    if ([string]$Record.Stage.GuardNonce -cne [string]$Record.Nonce -or
        [string]$Record.Stage.CandidateBindingSha256 -cne
            [string]$Record.CandidateBindingSha256 -or
        [string]$Record.Stage.AddonSearchPath -cne
            ([string]$Record.Candidate.RuntimeAddonRootPath + ',' + $stageRoot)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_STAGE_PRIVATE_BINDING' `
            -Message 'Private candidate stage identity changed.'
    }
}

function Get-PartisanV2LedgerEntry {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)][int]$ProcessId
    )

    return @($Record.Ledger.ToArray() | Where-Object {
        [int]$_.Identity.ProcessId -eq $ProcessId
    } | Select-Object -First 1)
}

function Add-PartisanV2LedgerEntry {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)]$Identity,
        [Parameter(Mandatory = $true)][string]$Role
    )

    $existing = @(Get-PartisanV2LedgerEntry `
        -Record $Record `
        -ProcessId ([int]$Identity.ProcessId))
    if ($existing.Count -ne 0) {
        if (-not (Test-PartisanProcessIdentity `
                -Expected $existing[0].Identity `
                -Actual $Identity) -or
            [string]$existing[0].Role -cne $Role) {
            Throw-PartisanV2 `
                -Identifier 'PGR_LEDGER_IDENTITY_REUSE' `
                -Message 'Private process ledger identity or role changed.'
        }
        return $existing[0]
    }
    $entry = [pscustomobject][ordered]@{
        Role = $Role
        Identity = $Identity
        RecordedUtc = [DateTime]::UtcNow
    }
    [void]$Record.Ledger.Add($entry)
    return $entry
}

function Sync-PartisanV2Ledger {
    param([Parameter(Mandatory = $true)]$Record)

    foreach ($entry in $Record.Ledger.ToArray()) {
        $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
        if ([string]$status.Status -ceq 'unknown') {
            for ($attempt = 1; $attempt -le 3; $attempt++) {
                Start-Sleep -Milliseconds 25
                $status = Get-PartisanProcessIdentityStatus `
                    -Identity $entry.Identity
                if ([string]$status.Status -cne 'unknown') { break }
            }
        }
        if ([string]$status.Status -ceq 'unknown') {
            Throw-PartisanV2 `
                -Identifier 'PGR_LEDGER_IDENTITY_UNKNOWN' `
                -Message ('Private ledger identity is unknown: pid=' +
                    [int]$entry.Identity.ProcessId + '.')
        }
    }
    if ($Record.JobClosed -or $null -eq $Record.Job) {
        return
    }
    $jobPids = @($Record.Job.GetProcessIds())
    $pending = New-Object Collections.Generic.List[object]
    foreach ($processId in $jobPids) {
        $identity = $null
        for ($attempt = 1; $attempt -le 4; $attempt++) {
            try {
                $identity = Get-PartisanProcessIdentity `
                    -ProcessId ([int]$processId)
                break
            }
            catch {
                if ($null -eq (Get-Process `
                        -Id ([int]$processId) `
                        -ErrorAction SilentlyContinue)) {
                    break
                }
                if ($attempt -lt 4) { Start-Sleep -Milliseconds 25 }
            }
        }
        if ($null -eq $identity) {
            if ($null -eq (Get-Process `
                    -Id ([int]$processId) `
                    -ErrorAction SilentlyContinue)) {
                continue
            }
            Throw-PartisanV2 `
                -Identifier 'PGR_JOB_MEMBER_UNKNOWN' `
                -Message ('Live job member cannot be inspected: pid=' +
                    [int]$processId + '.')
        }
        $existing = @(Get-PartisanV2LedgerEntry `
            -Record $Record `
            -ProcessId ([int]$processId))
        if ($existing.Count -ne 0) {
            if (-not (Test-PartisanProcessIdentity `
                    -Expected $existing[0].Identity `
                    -Actual $identity)) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_JOB_MEMBER_IDENTITY_CHANGED' `
                    -Message 'Live job member differs from its private identity.'
            }
            continue
        }
        [void]$pending.Add($identity)
    }
    $progress = $true
    while ($pending.Count -gt 0 -and $progress) {
        $progress = $false
        foreach ($identity in @($pending.ToArray())) {
            $parent = @(Get-PartisanV2LedgerEntry `
                -Record $Record `
                -ProcessId ([int]$identity.ParentProcessId))
            if ($parent.Count -eq 0) {
                continue
            }
            $parentRole = [string]$parent[0].Role
            $descendantRole = if ($parentRole -ceq 'server' -or
                $parentRole -ceq 'server-descendant') {
                'server-descendant'
            }
            else {
                $parentRole
            }
            [void](Add-PartisanV2LedgerEntry `
                -Record $Record `
                -Identity $identity `
                -Role $descendantRole)
            [void]$pending.Remove($identity)
            $progress = $true
        }
    }
    foreach ($identity in $pending.ToArray()) {
        [void](Add-PartisanV2LedgerEntry `
            -Record $Record `
            -Identity $identity `
            -Role 'job-owned')
    }
}

function Assert-PartisanV2ExecutableBindings {
    param([Parameter(Mandatory = $true)]$Record)

    $launches = New-Object Collections.Generic.List[object]
    if ($Record.Server) { [void]$launches.Add($Record.Server) }
    foreach ($client in $Record.Clients.ToArray()) {
        [void]$launches.Add($client)
    }
    foreach ($launch in $launches.ToArray()) {
        [void](Assert-PartisanExecutableProvenance `
            -Expected $launch.ExecutableProvenance `
            -Path $launch.ExecutablePath)
        $expectedCommandLine = ConvertTo-PartisanNativeCommandLine `
            -Executable $launch.ExecutablePath `
            -Arguments $launch.Arguments
        if (-not (Test-PartisanExactNativeArgumentVector `
                -CommandLine $expectedCommandLine `
                -ExpectedExecutable $launch.ExecutablePath `
                -ExpectedArguments $launch.Arguments)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_PRIVATE_ARGV_BINDING' `
                -Message 'Private executable argument binding changed.'
        }
    }
}

function Assert-PartisanV2EngineOwnership {
    param([Parameter(Mandatory = $true)]$Record)

    Sync-PartisanV2Ledger -Record $Record
    foreach ($process in @(Get-PartisanEngineProcesses)) {
        $identity = Get-PartisanProcessIdentity -ProcessId ([int]$process.Id)
        $entry = @(Get-PartisanV2LedgerEntry `
            -Record $Record `
            -ProcessId ([int]$process.Id))
        if ($entry.Count -ne 1 -or
            -not (Test-PartisanProcessIdentity `
                -Expected $entry[0].Identity `
                -Actual $identity)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_UNCLAIMED_ENGINE' `
                -Message 'Engine process exists outside the private job ledger.'
        }
    }
}

function Invoke-PartisanV2PrivateAudit {
    param([Parameter(Mandatory = $true)]$Record)

    Assert-PartisanV2GuardExact -Record $Record
    if (-not (Test-PartisanFileSignatureExact `
            -Expected $Record.ModuleToolSignature `
            -Actual (Get-PartisanFileSignature -Path $Record.ModuleToolPath))) {
        Throw-PartisanV2 `
            -Identifier 'PGR_GUARDED_TOOL_CHANGED' `
            -Message 'Guarded runtime module bytes changed during the context.'
    }
    if ($Record.Stage) {
        Assert-PartisanV2StageExact -Record $Record
    }
    Assert-PartisanV2ExecutableBindings -Record $Record
    Assert-PartisanV2EngineOwnership -Record $Record
    $delta = Compare-PartisanBoundarySnapshotSet `
        -Snapshots ([object[]]$Record.Boundaries)
    if (-not $delta.Clean) {
        Throw-PartisanV2 `
            -Identifier 'PGR_BOUNDARY_DELTA' `
            -Message 'Original private watched or spill baseline changed.'
    }
}

function Resolve-PartisanV2Launch {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)]$Launch,
        [Parameter(Mandatory = $true)][string]$Operation
    )

    $all = New-Object Collections.Generic.List[object]
    if ($Record.Server) { [void]$all.Add($Record.Server) }
    foreach ($client in $Record.Clients.ToArray()) { [void]$all.Add($client) }
    foreach ($privateLaunch in $all.ToArray()) {
        if ([object]::ReferenceEquals($Launch, $privateLaunch.PublicLaunch)) {
            return $privateLaunch
        }
    }
    Set-PartisanV2PermanentNoGo `
        -Record $Record `
        -Identifier 'PGR_LAUNCH_REFERENCE_MISMATCH' `
        -Message ('Launch clone or replacement rejected at: ' + $Operation + '.')
    Throw-PartisanV2 `
        -Identifier 'PGR_LAUNCH_REFERENCE_MISMATCH' `
        -Message 'Only the exact registered launch projection is accepted.'
}

function Stop-PartisanV2RoleFixedPoint {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)][string]$Role,
        [ValidateRange(1, 120)][int]$TimeoutSeconds = 15
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $stable = 0
    while ([DateTime]::UtcNow -lt $deadline) {
        Sync-PartisanV2Ledger -Record $Record
        $entries = @($Record.Ledger.ToArray() | Where-Object {
            [string]$_.Role -ceq $Role
        })
        $alive = New-Object Collections.Generic.List[object]
        foreach ($entry in $entries) {
            $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
            if ([string]$status.Status -ceq 'unknown') {
                Throw-PartisanV2 `
                    -Identifier 'PGR_STOP_IDENTITY_UNKNOWN' `
                    -Message ('Role identity remained unknown: ' + $Role + '.')
            }
            if ([string]$status.Status -ceq 'alive') {
                [void]$alive.Add($entry)
            }
        }
        if ($alive.Count -eq 0) {
            $stable++
            if ($stable -ge 2) { return $true }
        }
        else {
            $stable = 0
            foreach ($entry in $alive.ToArray()) {
                try {
                    (Get-Process `
                        -Id ([int]$entry.Identity.ProcessId) `
                        -ErrorAction Stop).Kill()
                }
                catch {
                    $after = Get-PartisanProcessIdentityStatus `
                        -Identity $entry.Identity
                    if ([string]$after.Status -cne 'dead') {
                        throw
                    }
                }
            }
        }
        Start-Sleep -Milliseconds 100
    }
    Throw-PartisanV2 `
        -Identifier 'PGR_STOP_FIXED_POINT_TIMEOUT' `
        -Message ('Role did not reach a dead fixed point: ' + $Role + '.')
}

function New-PartisanGuardedRuntimeContext {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$GuardBase,
        [Parameter(Mandatory = $true)][string]$Purpose,
        [Parameter(Mandatory = $true)][string[]]$WatchedRoots,
        [Parameter(Mandatory = $true)][string[]]$SpillRoots,
        [Parameter(Mandatory = $true)][int[]]$LoopbackPorts,
        [ValidateSet('Tcp', 'Udp')][string[]]$LoopbackProtocols = @('Udp'),
        [ValidateSet(
            'create-cleanup-failure',
            'handle-dispose-failure',
            'client-unknown-barrier',
            'job-owned-member',
            'missing-guard-final',
            'crash-after-receipt')]
        [string[]]$NonEngineSelfTestFaults = @()
    )

    if ($env:OS -cne 'Windows_NT') {
        Throw-PartisanV2 `
            -Identifier 'PGR_WINDOWS_REQUIRED' `
            -Message 'Guarded runtime ownership requires Windows.'
    }
    if ($WatchedRoots.Count -eq 0 -or $SpillRoots.Count -eq 0 -or
        $LoopbackPorts.Count -eq 0 -or $LoopbackProtocols.Count -eq 0) {
        Throw-PartisanV2 `
            -Identifier 'PGR_BOUNDARIES_REQUIRED' `
            -Message 'Watched, spill, and IPv4 loopback boundaries are required.'
    }
    if ($NonEngineSelfTestFaults.Count -ne 0 -and
        $Purpose -cnotmatch '^self_test_') {
        Throw-PartisanV2 `
            -Identifier 'PGR_SELFTEST_FAULT_SCOPE' `
            -Message 'Fault injection is restricted to explicit self-test contexts.'
    }
    if (@(Get-PartisanEngineProcesses).Count -ne 0) {
        Throw-PartisanV2 `
            -Identifier 'PGR_ENGINE_PREFLIGHT' `
            -Message 'An engine-free preflight is required.'
    }
    $guardBaseFull = Resolve-PartisanExistingPath `
        -Path $GuardBase `
        -Kind Container
    if ($guardBaseFull.Contains(',')) {
        Throw-PartisanV2 `
            -Identifier 'PGR_COMMA_GUARD_BASE' `
            -Message 'Guard base must not contain a comma.'
    }
    Assert-PartisanNoReparseTree -Root $guardBaseFull
    $allRoots = @($WatchedRoots) + @($SpillRoots)
    $resolvedRoots = @($allRoots | ForEach-Object {
        Resolve-PartisanExistingPath -Path $_ -Kind Container
    })
    for ($left = 0; $left -lt $resolvedRoots.Count; $left++) {
        if (Test-PartisanPathOverlap `
                -First $guardBaseFull `
                -Second $resolvedRoots[$left]) {
            Throw-PartisanV2 `
                -Identifier 'PGR_GUARD_BOUNDARY_OVERLAP' `
                -Message 'Guard base overlaps a watched or spill root.'
        }
        for ($right = $left + 1; $right -lt $resolvedRoots.Count; $right++) {
            if (Test-PartisanPathOverlap `
                    -First $resolvedRoots[$left] `
                    -Second $resolvedRoots[$right]) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_BOUNDARY_OVERLAP' `
                    -Message 'Watched and spill roots overlap.'
            }
        }
    }
    $ports = New-Object Collections.Generic.List[object]
    $portKeys = New-Object Collections.Generic.HashSet[string](
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($protocol in $LoopbackProtocols) {
        foreach ($port in $LoopbackPorts) {
            $key = $protocol + ':' + $port
            if (-not $portKeys.Add($key)) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_DUPLICATE_PORT_CHECK' `
                    -Message 'IPv4 loopback availability check is duplicated.'
            }
            [void]$ports.Add((Assert-PartisanLoopbackPortAvailable `
                -Port $port `
                -Protocol $protocol))
        }
    }
    $boundaries = New-PartisanBoundarySnapshotSet `
        -WatchedRoots $WatchedRoots `
        -SpillRoots $SpillRoots
    $guard = $null
    $job = $null
    $record = $null
    $guardKey = $null
    $protectionRecord = $null
    try {
        $guard = New-PartisanGuardDirectory `
            -GuardBase $guardBaseFull `
            -Purpose $Purpose
        $guardKey = Get-PartisanFullPath -Path $guard.Directory
        $protectionRecord = [pscustomobject][ordered]@{
            Nonce = [string]$guard.Nonce
            State = 'creating'
            PermanentNoGo = $false
            Guard = $guard
        }
        $script:RegisteredGuards.Add($guardKey, $protectionRecord)
        $journalPrefix = '.PartisanGuardedRuntime_' + $guard.Nonce
        $journalPath = Join-Path $guardBaseFull ($journalPrefix + '.journal.json')
        $journalFaultPath = Join-Path `
            $guardBaseFull `
            ($journalPrefix + '.journal-fault.json')
        $receiptPath = Join-Path $guardBaseFull ($journalPrefix + '.receipt.json')
        $attestationPath = Join-Path `
            $guardBaseFull `
            ($journalPrefix + '.completion.json')
        foreach ($path in @(
                $journalPath,
                $journalFaultPath,
                $receiptPath,
                $attestationPath)) {
            if (Test-Path -LiteralPath $path) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_EXTERNAL_EVIDENCE_COLLISION' `
                    -Message 'Fresh nonce-bound external evidence path already exists.'
            }
        }
        $job = New-Object Partisan.GuardedRuntime.NativeJob
        $public = [pscustomobject][ordered]@{}
        $record = [pscustomobject][ordered]@{
            Nonce = [string]$guard.Nonce
            State = 'creating'
            PermanentNoGo = $false
            PublicContext = $public
            PublicDigest = $null
            ProjectionReferences = $null
            Guard = $guard
            GuardInventory = $null
            GuardBindingSha256 = Get-PartisanV2Digest -Value ([ordered]@{
                directory = [string]$guard.Directory
                base = [string]$guard.GuardBase
                nonce = [string]$guard.Nonce
                purpose = [string]$guard.Purpose
                ownerPid = [int]$guard.OwnerPid
                ownerStartUtc = ([datetime]$guard.OwnerStartUtc).ToString('o')
                sentinel = Copy-PartisanV2PlainValue `
                    -Value $guard.SentinelSignature
            })
            GuardRemoved = $false
            Job = $job
            JobClosed = $false
            Ledger = New-Object Collections.Generic.List[object]
            Server = $null
            Clients = New-Object Collections.Generic.List[object]
            Stage = $null
            Candidate = $null
            CandidateBindingPath = $null
            CandidateBindingSignature = $null
            CandidateBindingSha256 = $null
            Boundaries = [object[]]$boundaries
            Ports = [object[]]$ports.ToArray()
            Failures = New-Object Collections.Generic.List[object]
            FailureIndex = New-Object `
                'Collections.Generic.Dictionary[string,object]' `
                ([StringComparer]::Ordinal)
            FailureSequence = [long]0
            OmittedFailureCount = [long]0
            LastOmittedFailure = $null
            JournalPath = Get-PartisanFullPath -Path $journalPath
            JournalFaultPath = Get-PartisanFullPath -Path $journalFaultPath
            JournalActivePath = $null
            JournalSignature = $null
            JournalMode = $null
            JournalWriteFault = $null
            JournalOverflowPaths = New-Object Collections.Generic.List[string]
            ReceiptPath = Get-PartisanFullPath -Path $receiptPath
            ReceiptSignature = $null
            CompletionAttestationPath = Get-PartisanFullPath `
                -Path $attestationPath
            CompletionAttestationSignature = $null
            CompletionToken = $null
            FinalTransactionStarted = $false
            FinalTransactionCompleted = $false
            AuditCount = 0
            Purpose = $Purpose
            SelfTestFaults = [string[]]$NonEngineSelfTestFaults
            SelfTestProcessHandles = New-Object Collections.Generic.List[object]
            TeardownStopOrder = New-Object Collections.Generic.List[string]
            ModuleToolPath = [string]$script:ModuleFilePath
            ModuleToolSignature = Get-PartisanFileSignature `
                -Path $script:ModuleFilePath
        }
        $record.GuardInventory = New-PartisanV2GuardInventory -Record $record
        $script:RegisteredGuards[$guardKey] = $record
        $script:RuntimeRegistry.Add($record.Nonce, $record)
        if ($record.SelfTestFaults -contains 'create-cleanup-failure') {
            Remove-Item `
                -LiteralPath $record.Guard.Sentinel `
                -Force `
                -ErrorAction Stop
            Throw-PartisanV2 `
                -Identifier 'PGR_SELFTEST_CREATE_FAILURE' `
                -Message 'Injected context-creation failure with fallible guard cleanup.'
        }
        $record.State = 'active'
        Update-PartisanV2PublicProjection -Record $record
        return $public
    }
    catch {
        $creationError = $_
        $cleanupErrors = New-Object Collections.Generic.List[string]
        if ($job) {
            try {
                $job.Dispose()
                if ($record) {
                    $record.JobClosed = $true
                    $record.Job = $null
                }
            }
            catch { [void]$cleanupErrors.Add($_.Exception.Message) }
        }
        if ($guard -and $cleanupErrors.Count -eq 0) {
            try {
                [void](Remove-PartisanGuardDirectoryCore `
                    -Ownership $guard `
                    -ExpectedInventory $(if ($record) {
                        [object[]]$record.GuardInventory
                    }
                    else { $null }) `
                    -RequirePresent)
            }
            catch { [void]$cleanupErrors.Add($_.Exception.Message) }
        }
        if ($guard -and $cleanupErrors.Count -eq 0) {
            if ($record -and
                $script:RuntimeRegistry.ContainsKey($record.Nonce)) {
                [void]$script:RuntimeRegistry.Remove($record.Nonce)
            }
            if ($guardKey -and $script:RegisteredGuards.ContainsKey($guardKey)) {
                [void]$script:RegisteredGuards.Remove($guardKey)
            }
        }
        elseif ($guard) {
            if ($record) {
                Set-PartisanV2PermanentNoGo `
                    -Record $record `
                    -Identifier 'PGR_CREATE_CLEANUP_FAILED' `
                    -Message (($cleanupErrors.ToArray()) -join '; ') `
                    -SkipJournal
            }
            elseif ($protectionRecord) {
                $protectionRecord.State = 'permanent-no-go'
                $protectionRecord.PermanentNoGo = $true
            }
            $fault = Write-PartisanV2CreationFaultJournal `
                -Guard $guard `
                -CreationError $creationError.Exception.Message `
                -CleanupErrors ([string[]]$cleanupErrors.ToArray())
            if ($record) {
                $record.JournalActivePath = [string]$fault.Path
                $record.JournalSignature = $fault.Signature
                $record.JournalMode = 'permanent-no-go'
            }
        }
        throw $creationError
    }
}

function New-PartisanCandidateStage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Candidate
    )

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'New-PartisanCandidateStage' `
        -AllowedStates @('active')
    if ($record.Stage) {
        Throw-PartisanV2 `
            -Identifier 'PGR_STAGE_ALREADY_EXISTS' `
            -Message 'Registered context already owns a candidate stage.'
    }
    try {
        $sealed = ConvertTo-PartisanSealedCandidateBinding -Candidate $Candidate
        $stageRoot = Get-PartisanFullPath -Path (
            Join-Path $record.Guard.Directory 'candidate-addons')
        if ($stageRoot.Contains(',') -or
            -not (Test-PartisanContainedPath `
                -Root $record.Guard.Directory `
                -Candidate $stageRoot) -or
            (Test-Path -LiteralPath $stageRoot)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_STAGE_ROOT_UNSAFE' `
                -Message 'Candidate stage root is not fresh and guard-contained.'
        }
        $stagedAddon = Join-Path $stageRoot 'Partisan'
        New-Item -ItemType Directory -Path $stagedAddon -ErrorAction Stop |
            Out-Null
        $seen = New-Object Collections.Generic.HashSet[string](
            [StringComparer]::Ordinal)
        foreach ($row in @($sealed.PackageFiles)) {
            $name = Split-Path -Leaf ([string]$row.indexPath)
            if (-not $seen.Add($name)) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_STAGE_FILE_COLLISION' `
                    -Message 'Candidate stage file mapping is ambiguous.'
            }
            $source = Resolve-PartisanExistingPath `
                -Path (Join-Path $sealed.PackedAddonPath $name) `
                -Kind Leaf
            if (-not (Test-PartisanContainedPath `
                    -Root $sealed.PackedAddonPath `
                    -Candidate $source)) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_STAGE_SOURCE_ESCAPE' `
                    -Message 'Candidate source file escaped sealed package root.'
            }
            Copy-Item `
                -LiteralPath $source `
                -Destination (Join-Path $stagedAddon $name) `
                -ErrorAction Stop
        }
        $bindingPath = Get-PartisanFullPath -Path (
            Join-Path $record.Guard.Directory $script:CandidateBindingLeaf)
        $bindingSha = Get-PartisanCandidateBindingSha256 -Candidate $sealed
        $bindingSignature = Write-PartisanPortableJson `
            -Path $bindingPath `
            -Value $sealed
        $privateStage = [pscustomobject][ordered]@{
            GuardNonce = [string]$record.Nonce
            CandidateId = [string]$sealed.CandidateId
            StageRootPath = $stageRoot
            PackedAddonPath = Get-PartisanFullPath -Path $stagedAddon
            PackedProjectPath = Get-PartisanFullPath -Path (
                Join-Path $stagedAddon 'addon.gproj')
            AddonSearchPath = [string]$sealed.RuntimeAddonRootPath + ',' +
                $stageRoot
            PackageSha256 = [string]$sealed.PackageSha256
            CandidateBindingSha256 = $bindingSha
            PublicStage = $null
        }
        $publicStage = [pscustomobject][ordered]@{
            GuardNonce = [string]$privateStage.GuardNonce
            CandidateId = [string]$privateStage.CandidateId
            StageRootPath = [string]$privateStage.StageRootPath
            PackedAddonPath = [string]$privateStage.PackedAddonPath
            PackedProjectPath = [string]$privateStage.PackedProjectPath
            AddonSearchPath = [string]$privateStage.AddonSearchPath
            PackageSha256 = [string]$privateStage.PackageSha256
            CandidateBindingSha256 = [string]$bindingSha
        }
        $privateStage.PublicStage = $publicStage
        $record.Candidate = $sealed
        $record.CandidateBindingPath = $bindingPath
        $record.CandidateBindingSignature = $bindingSignature
        $record.CandidateBindingSha256 = $bindingSha
        $record.Stage = $privateStage
        $record.GuardInventory = New-PartisanV2GuardInventory -Record $record
        Assert-PartisanV2StageExact -Record $record
        [void](Assert-PartisanV2GuardInventoryExact `
            -GuardDirectory $record.Guard.Directory `
            -ExpectedInventory ([object[]]$record.GuardInventory))
        Update-PartisanV2PublicProjection -Record $record
        return $publicStage
    }
    catch {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_STAGE_CREATION_FAILED' `
            -Message $_.Exception.Message
        throw
    }
}

function Assert-PartisanCandidateStage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Stage
    )

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'Assert-PartisanCandidateStage' `
        -AllowedStates @('active', 'teardown-running', 'permanent-no-go')
    if ($null -eq $record.Stage -or
        -not [object]::ReferenceEquals($Stage, $record.Stage.PublicStage)) {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_STAGE_REFERENCE_MISMATCH' `
            -Message 'Stage clone or replacement rejected.'
        Throw-PartisanV2 `
            -Identifier 'PGR_STAGE_REFERENCE_MISMATCH' `
            -Message 'Only the exact registered stage projection is accepted.'
    }
    try { Assert-PartisanV2StageExact -Record $record }
    catch {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_STAGE_AUDIT_FAILED' `
            -Message $_.Exception.Message
        throw
    }
    return $Stage
}

function Assert-PartisanRuntimeOwnershipAudit {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)]$Context)

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'Assert-PartisanRuntimeOwnershipAudit' `
        -AllowedStates @('active', 'teardown-running', 'permanent-no-go')
    try {
        Invoke-PartisanV2PrivateAudit -Record $record
        $record.AuditCount++
        Update-PartisanV2PublicProjection -Record $record
        return [pscustomobject][ordered]@{
            ContextId = [string]$record.Nonce
            State = [string]$record.State
            Certification = if ($record.PermanentNoGo) {
                'permanent-no-go'
            }
            else { 'not-invalidated' }
            AuditCount = [int]$record.AuditCount
            LedgerEntries = [int]$record.Ledger.Count
            PortSemantics = 'ipv4-loopback-bind-availability-only'
        }
    }
    catch {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_OWNERSHIP_AUDIT_FAILED' `
            -Message $_.Exception.Message
        throw
    }
}

function Get-PartisanV2CandidateConsumptionEvidence {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments,
        [Parameter(Mandatory = $true)][bool]$IsEngine
    )

    if (-not $IsEngine) {
        return [pscustomobject][ordered]@{
            mode = 'non-engine-self-test-only'
            candidateBindingSha256 = [string]$Record.CandidateBindingSha256
            exactArgumentPairs = [object[]]@()
        }
    }
    $requiredPairs = [ordered]@{
        '-addonsDir' = [string]$Record.Stage.AddonSearchPath
        '-gproj' = [string]$Record.Stage.PackedProjectPath
        '-hstReleaseCandidateId' = [string]$Record.Candidate.CandidateId
        '-hstReleasePackageSha256' = [string]$Record.Candidate.PackageSha256
    }
    foreach ($argument in $Arguments) {
        foreach ($canonicalFlag in $requiredPairs.Keys) {
            if ([string]$argument -ieq [string]$canonicalFlag -and
                [string]$argument -cne [string]$canonicalFlag) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_ENGINE_CANDIDATE_ARGV_FORM' `
                    -Message ('Engine candidate option casing is not canonical: ' +
                        [string]$argument + '.')
            }
            if ([string]$argument -imatch
                ('^' + [regex]::Escape([string]$canonicalFlag) + '[:=]')) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_ENGINE_CANDIDATE_ARGV_FORM' `
                    -Message ('Engine candidate options require separate exact argv tokens: ' +
                        [string]$argument + '.')
            }
            $longFlag = '-' + [string]$canonicalFlag
            if ([string]$argument -ieq $longFlag -or
                [string]$argument -imatch
                    ('^' + [regex]::Escape($longFlag) + '[:=]')) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_ENGINE_CANDIDATE_ARGV_FORM' `
                    -Message ('Conflicting engine candidate option form rejected: ' +
                        [string]$argument + '.')
            }
        }
    }
    $evidencePairs = New-Object Collections.Generic.List[object]
    foreach ($flag in $requiredPairs.Keys) {
        $positions = New-Object Collections.Generic.List[int]
        for ($index = 0; $index -lt $Arguments.Count; $index++) {
            if ([string]$Arguments[$index] -ceq [string]$flag) {
                [void]$positions.Add($index)
            }
        }
        if ($positions.Count -ne 1 -or
            $positions[0] + 1 -ge $Arguments.Count -or
            [string]$Arguments[$positions[0] + 1] -cne
                [string]$requiredPairs[$flag]) {
            Throw-PartisanV2 `
                -Identifier 'PGR_ENGINE_CANDIDATE_ARGV' `
                -Message ('Engine candidate argument pair is missing, duplicated, ' +
                    'or changed: ' + $flag + '.')
        }
        [void]$evidencePairs.Add([pscustomobject][ordered]@{
            flag = [string]$flag
            value = [string]$requiredPairs[$flag]
            index = [int]$positions[0]
        })
    }
    return [pscustomobject][ordered]@{
        mode = 'exact-stage-and-candidate-argv-enforced'
        candidateBindingSha256 = [string]$Record.CandidateBindingSha256
        exactArgumentPairs = [object[]]$evidencePairs.ToArray()
    }
}

function Start-PartisanV2Role {
    param(
        [Parameter(Mandatory = $true)]$Record,
        [Parameter(Mandatory = $true)][string]$Role,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$ExecutableProvenance,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [AllowNull()]$CandidateConsumption,
        [switch]$NonEngineSelfTestOnly,
        [switch]$NonEngineSelfTestForcePostResumeFailure
    )

    try {
    $executablePath = Resolve-PartisanExistingPath -Path $Executable -Kind Leaf
    $workingPath = Resolve-PartisanExistingPath `
        -Path $WorkingDirectory `
        -Kind Container
    Assert-PartisanNoReparseAncestry -Path $executablePath
    Assert-PartisanNoReparseAncestry -Path $workingPath
    $isEngine = Test-PartisanEngineExecutable -Path $executablePath
    if ($isEngine) {
        if ($NonEngineSelfTestOnly -or $NonEngineSelfTestForcePostResumeFailure -or
            $null -eq $Record.Stage -or
            -not [object]::ReferenceEquals(
                $CandidateConsumption,
                $Record.Stage.PublicStage)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_ENGINE_CANDIDATE_CONSUMPTION' `
                -Message 'Engine launch requires the exact registered stage and no self-test bypass.'
        }
    }
    elseif (-not $NonEngineSelfTestOnly -or
        [string]$Record.Purpose -cnotmatch '^self_test_') {
        Throw-PartisanV2 `
            -Identifier 'PGR_NONENGINE_SELFTEST_ONLY' `
            -Message 'Non-engine launch requires an explicit self-test-only context and switch.'
    }
    if ($null -eq $Record.Stage) {
        Throw-PartisanV2 `
            -Identifier 'PGR_LAUNCH_STAGE_REQUIRED' `
            -Message 'Every guarded launch requires the private sealed stage.'
    }
    Assert-PartisanV2StageExact -Record $Record
    $candidateConsumptionEvidence = Get-PartisanV2CandidateConsumptionEvidence `
        -Record $Record `
        -Arguments $Arguments `
        -IsEngine $isEngine
    $sealedProvenance = Copy-PartisanV2PlainValue -Value $ExecutableProvenance
    [void](Assert-PartisanExecutableProvenance `
        -Expected $sealedProvenance `
        -Path $executablePath)
    Invoke-PartisanV2PrivateAudit -Record $Record
    $commandLine = ConvertTo-PartisanNativeCommandLine `
        -Executable $executablePath `
        -Arguments $Arguments
    if (-not (Test-PartisanExactNativeArgumentVector `
            -CommandLine $commandLine `
            -ExpectedExecutable $executablePath `
            -ExpectedArguments $Arguments)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_NATIVE_ARGV_ROUNDTRIP' `
            -Message 'Native argument vector did not round-trip exactly.'
    }
    $suspended = $null
    $process = $null
    $identity = $null
    $resumed = $false
    try {
        $suspended = New-Object Partisan.GuardedRuntime.SuspendedProcess(
            $executablePath,
            $commandLine,
            $workingPath)
        $process = $suspended.Child
        $Record.Job.Add($process)
        $identity = Get-PartisanProcessIdentity -ProcessId ([int]$process.Id)
        if (-not (Test-PartisanExactNativeArgumentVector `
                -CommandLine ([string]$identity.CommandLine) `
                -ExpectedExecutable $executablePath `
                -ExpectedArguments $Arguments)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_SUSPENDED_ARGV_MISMATCH' `
                -Message 'Suspended process argument vector changed.'
        }
        [void](Add-PartisanV2LedgerEntry `
            -Record $Record `
            -Identity $identity `
            -Role $Role)
        $privateLaunch = [pscustomobject][ordered]@{
            Role = $Role
            Process = $process
            Identity = $identity
            ExecutablePath = $executablePath
            ExecutableProvenance = $sealedProvenance
            Arguments = [string[]]$Arguments.Clone()
            WorkingDirectory = $workingPath
            IsEngine = [bool]$isEngine
            CandidateConsumptionEvidence = $candidateConsumptionEvidence
            StartedUtc = [DateTime]::UtcNow
            ExitCode = $null
            Stopped = $false
            PublicLaunch = $null
        }
        $suspended.Resume()
        $resumed = $true
        $suspended.Dispose()
        $suspended = $null
        Start-Sleep -Milliseconds 150
        if ($NonEngineSelfTestForcePostResumeFailure) {
            Start-Sleep -Milliseconds 500
        }
        Sync-PartisanV2Ledger -Record $Record
        if ($NonEngineSelfTestForcePostResumeFailure) {
            Throw-PartisanV2 `
                -Identifier 'PGR_SELFTEST_POST_RESUME_INJECTED' `
                -Message 'Injected post-resume self-test failure.'
        }
        $status = Get-PartisanProcessIdentityStatus -Identity $identity
        if ([string]$status.Status -cne 'alive') {
            Throw-PartisanV2 `
                -Identifier 'PGR_POST_RESUME_IDENTITY' `
                -Message 'Post-resume root identity is not exactly alive.'
        }
        [void](Assert-PartisanExecutableProvenance `
            -Expected $sealedProvenance `
            -Path $executablePath)
        $privateLaunch.PublicLaunch = New-PartisanV2PublicLaunch `
            -Record $Record `
            -Launch $privateLaunch
        return $privateLaunch
    }
    catch {
        $failure = $_
        if ($resumed) {
            Set-PartisanV2PermanentNoGo `
                -Record $Record `
                -Identifier 'PGR_POST_RESUME_LAUNCH_FAILED' `
                -Message $failure.Exception.Message
            try {
                Sync-PartisanV2Ledger -Record $Record
                [void](Stop-PartisanV2RoleFixedPoint `
                    -Record $Record `
                    -Role $Role `
                    -TimeoutSeconds 15)
            }
            catch {
                Set-PartisanV2PermanentNoGo `
                    -Record $Record `
                    -Identifier 'PGR_POST_RESUME_CLEANUP_FAILED' `
                    -Message $_.Exception.Message
            }
            try { Update-PartisanV2PublicProjection -Record $Record } catch { }
        }
        else {
            Set-PartisanV2PermanentNoGo `
                -Record $Record `
                -Identifier 'PGR_LAUNCH_VALIDATION_FAILED' `
                -Message $failure.Exception.Message
        }
        throw $failure
    }
    finally {
        if ($suspended) { $suspended.Dispose() }
    }
    }
    catch {
        if (-not $Record.PermanentNoGo) {
            Set-PartisanV2PermanentNoGo `
                -Record $Record `
                -Identifier 'PGR_LAUNCH_VALIDATION_FAILED' `
                -Message $_.Exception.Message
        }
        throw
    }
}

function Start-PartisanGuardedServer {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$ExecutableProvenance,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [AllowNull()]$CandidateConsumption,
        [switch]$NonEngineSelfTestOnly,
        [switch]$NonEngineSelfTestForcePostResumeFailure
    )

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'Start-PartisanGuardedServer' `
        -AllowedStates @('active')
    try {
    if ($record.Server) {
        Throw-PartisanV2 `
            -Identifier 'PGR_SERVER_ALREADY_REGISTERED' `
            -Message 'Private context already owns a server launch.'
    }
    foreach ($port in $record.Ports) {
        [void](Assert-PartisanLoopbackPortAvailable `
            -Port ([int]$port.Port) `
            -Protocol ([string]$port.Protocol))
    }
        $launch = Start-PartisanV2Role `
            -Record $record `
            -Role 'server' `
            -Executable $Executable `
            -ExecutableProvenance $ExecutableProvenance `
            -Arguments $Arguments `
            -WorkingDirectory $WorkingDirectory `
            -CandidateConsumption $CandidateConsumption `
            -NonEngineSelfTestOnly:$NonEngineSelfTestOnly `
            -NonEngineSelfTestForcePostResumeFailure:$NonEngineSelfTestForcePostResumeFailure
        $record.Server = $launch
        Update-PartisanV2PublicProjection -Record $record
        return $launch.PublicLaunch
    }
    catch {
        if (-not $record.PermanentNoGo) {
            Set-PartisanV2PermanentNoGo `
                -Record $record `
                -Identifier 'PGR_SERVER_LAUNCH_FAILED' `
                -Message $_.Exception.Message
        }
        throw
    }
}

function Start-PartisanGuardedClient {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)]$ExecutableProvenance,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()]
        [string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [AllowNull()]$CandidateConsumption,
        [switch]$NonEngineSelfTestOnly,
        [switch]$NonEngineSelfTestForcePostResumeFailure
    )

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'Start-PartisanGuardedClient' `
        -AllowedStates @('active')
    try {
    if ($null -eq $record.Server) {
        Throw-PartisanV2 `
            -Identifier 'PGR_CLIENT_SERVER_REQUIRED' `
            -Message 'Private exact server launch is required before a client.'
    }
    $serverStatus = Get-PartisanProcessIdentityStatus `
        -Identity $record.Server.Identity
    if ([string]$serverStatus.Status -cne 'alive') {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_SERVER_NOT_EXACTLY_ALIVE' `
            -Message 'Server identity is dead or unknown before client launch.'
        Throw-PartisanV2 `
            -Identifier 'PGR_SERVER_NOT_EXACTLY_ALIVE' `
            -Message 'Server identity is dead or unknown before client launch.'
    }
    $role = 'client-' + ([int]$record.Clients.Count + 1)
    $launch = Start-PartisanV2Role `
        -Record $record `
        -Role $role `
        -Executable $Executable `
        -ExecutableProvenance $ExecutableProvenance `
        -Arguments $Arguments `
        -WorkingDirectory $WorkingDirectory `
        -CandidateConsumption $CandidateConsumption `
        -NonEngineSelfTestOnly:$NonEngineSelfTestOnly `
        -NonEngineSelfTestForcePostResumeFailure:$NonEngineSelfTestForcePostResumeFailure
    [void]$record.Clients.Add($launch)
    Update-PartisanV2PublicProjection -Record $record
    return $launch.PublicLaunch
    }
    catch {
        if (-not $record.PermanentNoGo) {
            Set-PartisanV2PermanentNoGo `
                -Record $record `
                -Identifier 'PGR_CLIENT_LAUNCH_FAILED' `
                -Message $_.Exception.Message
        }
        throw
    }
}

function Wait-PartisanGuardedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Launch,
        [ValidateRange(1, 86400)][int]$TimeoutSeconds = 600,
        [ValidateRange(20, 5000)][int]$PollMilliseconds = 250,
        [switch]$RequireZeroExit
    )

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'Wait-PartisanGuardedProcess' `
        -AllowedStates @('active')
    $privateLaunch = Resolve-PartisanV2Launch `
        -Record $record `
        -Launch $Launch `
        -Operation 'Wait-PartisanGuardedProcess'
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    try {
        while ([DateTime]::UtcNow -lt $deadline) {
            Invoke-PartisanV2PrivateAudit -Record $record
            $status = Get-PartisanProcessIdentityStatus `
                -Identity $privateLaunch.Identity
            if ([string]$status.Status -ceq 'unknown') {
                $expectedStartUtc = ([datetime]$privateLaunch.Identity.StartUtc).
                    ToUniversalTime().ToString('o')
                Throw-PartisanV2 `
                    -Identifier 'PGR_WAIT_IDENTITY_UNKNOWN' `
                    -Message ('Private launch identity became unknown while waiting;' +
                        'role=' + [string]$privateLaunch.Role + ';pid=' +
                        [int]$privateLaunch.Identity.ProcessId +
                        ';expectedStartUtc=' + $expectedStartUtc +
                        ';reason=' + [string]$status.Reason + '.')
            }
            if ([string]$status.Status -ceq 'dead') {
                $privateLaunch.Process.WaitForExit()
                $privateLaunch.ExitCode = [int]$privateLaunch.Process.ExitCode
                $privateLaunch.Stopped = $true
                $privateLaunch.PublicLaunch = New-PartisanV2PublicLaunch `
                    -Record $record `
                    -Launch $privateLaunch
                if ($record.Server -and
                    [object]::ReferenceEquals($privateLaunch, $record.Server)) {
                    $record.Server.PublicLaunch = $privateLaunch.PublicLaunch
                }
                Update-PartisanV2PublicProjection -Record $record
                if ($RequireZeroExit -and $privateLaunch.ExitCode -ne 0) {
                    Throw-PartisanV2 `
                        -Identifier 'PGR_PROCESS_NONZERO_EXIT' `
                        -Message 'Guarded process returned a nonzero exit code.'
                }
                return [pscustomobject][ordered]@{
                    RootRole = [string]$privateLaunch.Role
                    ExitCode = [int]$privateLaunch.ExitCode
                    ElapsedSeconds = [Math]::Round(
                        ([DateTime]::UtcNow -
                            [datetime]$privateLaunch.StartedUtc).TotalSeconds,
                        3)
                }
            }
            Start-Sleep -Milliseconds $PollMilliseconds
        }
        Throw-PartisanV2 `
            -Identifier 'PGR_WAIT_TIMEOUT' `
            -Message 'Guarded process exceeded its exact wait deadline.'
    }
    catch {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_WAIT_FAILED' `
            -Message $_.Exception.Message
        throw
    }
}

function Stop-PartisanGuardedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [Parameter(Mandatory = $true)]$Launch,
        [ValidateRange(1, 120)][int]$TimeoutSeconds = 15
    )

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'Stop-PartisanGuardedProcess' `
        -AllowedStates @('active', 'teardown-running', 'permanent-no-go')
    $privateLaunch = Resolve-PartisanV2Launch `
        -Record $record `
        -Launch $Launch `
        -Operation 'Stop-PartisanGuardedProcess'
    try {
    Sync-PartisanV2Ledger -Record $record
    if ([string]$privateLaunch.Role -ceq 'server') {
        foreach ($entry in $record.Ledger.ToArray()) {
            if ([string]$entry.Role -notlike 'client-*' -and
                [string]$entry.Role -cne 'job-owned' -and
                [string]$entry.Role -cne 'server-descendant') {
                continue
            }
            if ([string]$entry.Role -ceq 'server-descendant') {
                continue
            }
            $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
            if ([string]$status.Status -cne 'dead') {
                Throw-PartisanV2 `
                    -Identifier 'PGR_CLIENTS_BEFORE_SERVER' `
                    -Message 'All client and unclassified job roles must stop before server.'
            }
        }
        if (@($record.Ledger.ToArray() | Where-Object {
                [string]$_.Role -ceq 'server-descendant'
            }).Count -ne 0) {
            [void](Stop-PartisanV2RoleFixedPoint `
                -Record $record `
                -Role 'server-descendant' `
                -TimeoutSeconds $TimeoutSeconds)
        }
    }
        [void](Stop-PartisanV2RoleFixedPoint `
            -Record $record `
            -Role $privateLaunch.Role `
            -TimeoutSeconds $TimeoutSeconds)
        $privateLaunch.Stopped = $true
        $privateLaunch.Process.Refresh()
        if ($privateLaunch.Process.HasExited) {
            $privateLaunch.ExitCode = [int]$privateLaunch.Process.ExitCode
        }
        $privateLaunch.PublicLaunch = New-PartisanV2PublicLaunch `
            -Record $record `
            -Launch $privateLaunch
        Update-PartisanV2PublicProjection -Record $record
        return $true
    }
    catch {
        Set-PartisanV2PermanentNoGo `
            -Record $record `
            -Identifier 'PGR_STOP_FAILED' `
            -Message $_.Exception.Message
        throw
    }
}

function Invoke-PartisanGuardedTeardown {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]$Context,
        [ValidateRange(1, 120)][int]$StopTimeoutSeconds = 15,
        [ValidateRange(1, 120)][int]$PortReleaseTimeoutSeconds = 15
    )

    $record = Resolve-PartisanV2Record `
        -Context $Context `
        -Operation 'Invoke-PartisanGuardedTeardown' `
        -AllowedStates @('active', 'permanent-no-go')
    $enteredNoGo = [bool]$record.PermanentNoGo
    if (-not $enteredNoGo) {
        $record.State = 'teardown-running'
        try {
            Write-PartisanV2Journal -Record $record -Mode 'teardown-running'
            Update-PartisanV2PublicProjection -Record $record
        }
        catch {
            Set-PartisanV2PermanentNoGo `
                -Record $record `
                -Identifier 'PGR_TEARDOWN_ENTRY_JOURNAL' `
                -Message $_.Exception.Message
            throw
        }
    }
    try {
        if ($record.SelfTestFaults -contains 'job-owned-member') {
            $record.SelfTestFaults = [string[]]@($record.SelfTestFaults |
                Where-Object { $_ -cne 'job-owned-member' })
            if ($null -eq $record.Server) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_SELFTEST_JOB_MEMBER_SERVER' `
                    -Message 'Job-owned member injection requires a registered server.'
            }
            $injectedArguments = [string[]]@(
                '-NoLogo',
                '-NoProfile',
                '-NonInteractive',
                '-Command',
                ('$child = Start-Process -FilePath ' +
                    '([Diagnostics.Process]::GetCurrentProcess().MainModule.FileName) ' +
                    '-ArgumentList @(''-NoLogo'',''-NoProfile'',''-NonInteractive'',' +
                    '''-Command'',''Start-Sleep -Seconds 30'') -PassThru; ' +
                    'Start-Sleep -Seconds 30'))
            $injectedCommandLine = ConvertTo-PartisanNativeCommandLine `
                -Executable $record.Server.ExecutablePath `
                -Arguments $injectedArguments
            $injectedSuspended = $null
            try {
                $injectedSuspended = New-Object `
                    Partisan.GuardedRuntime.SuspendedProcess(
                        $record.Server.ExecutablePath,
                        $injectedCommandLine,
                        $record.Server.WorkingDirectory)
                $injectedProcess = $injectedSuspended.Child
                $record.Job.Add($injectedProcess)
                $injectedIdentity = Get-PartisanProcessIdentity `
                    -ProcessId ([int]$injectedProcess.Id)
                [void](Add-PartisanV2LedgerEntry `
                    -Record $record `
                    -Identity $injectedIdentity `
                    -Role 'job-owned')
                [void]$record.SelfTestProcessHandles.Add($injectedProcess)
                $injectedSuspended.Resume()
                Start-Sleep -Milliseconds 350
            }
            finally {
                if ($injectedSuspended) { $injectedSuspended.Dispose() }
            }
        }
        if ($record.SelfTestFaults -contains 'client-unknown-barrier' -and
            $record.Clients.Count -ne 0) {
            $record.SelfTestFaults = [string[]]@($record.SelfTestFaults |
                Where-Object { $_ -cne 'client-unknown-barrier' })
            Throw-PartisanV2 `
                -Identifier 'PGR_SELFTEST_CLIENT_UNKNOWN_BARRIER' `
                -Message 'Injected client unknown barrier retained server and job.'
        }
        $clients = $record.Clients.ToArray()
        [array]::Reverse($clients)
        foreach ($client in $clients) {
            [void](Stop-PartisanV2RoleFixedPoint `
                -Record $record `
                -Role $client.Role `
                -TimeoutSeconds $StopTimeoutSeconds)
            [void]$record.TeardownStopOrder.Add([string]$client.Role)
            $client.Stopped = $true
        }
        # A second fixed-point sweep catches descendants that appeared during
        # the reverse ordered pass.
        foreach ($role in @($record.Ledger.ToArray() | Where-Object {
            [string]$_.Role -like 'client-*'
        } | ForEach-Object { [string]$_.Role } | Sort-Object -Unique -Descending)) {
            [void](Stop-PartisanV2RoleFixedPoint `
                -Record $record `
                -Role $role `
                -TimeoutSeconds $StopTimeoutSeconds)
        }
        if (@($record.Ledger.ToArray() | Where-Object {
                [string]$_.Role -ceq 'job-owned'
            }).Count -ne 0) {
            [void](Stop-PartisanV2RoleFixedPoint `
                -Record $record `
                -Role 'job-owned' `
                -TimeoutSeconds $StopTimeoutSeconds)
            [void]$record.TeardownStopOrder.Add('job-owned')
        }
        Sync-PartisanV2Ledger -Record $record
        if (@($record.Ledger.ToArray() | Where-Object {
                [string]$_.Role -ceq 'server-descendant'
            }).Count -ne 0) {
            [void](Stop-PartisanV2RoleFixedPoint `
                -Record $record `
                -Role 'server-descendant' `
                -TimeoutSeconds $StopTimeoutSeconds)
            [void]$record.TeardownStopOrder.Add('server-descendant')
        }
        Sync-PartisanV2Ledger -Record $record
        foreach ($entry in $record.Ledger.ToArray()) {
            if ([string]$entry.Role -notlike 'client-*' -and
                [string]$entry.Role -cne 'job-owned' -and
                [string]$entry.Role -cne 'server-descendant') {
                continue
            }
            $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
            if ([string]$status.Status -cne 'dead') {
                Throw-PartisanV2 `
                    -Identifier 'PGR_TEARDOWN_CLIENT_BARRIER' `
                    -Message 'Client, server descendant, or unclassified job member blocks server teardown.'
            }
        }
        if ($record.Server) {
            [void](Stop-PartisanV2RoleFixedPoint `
                -Record $record `
                -Role 'server' `
                -TimeoutSeconds $StopTimeoutSeconds)
            [void]$record.TeardownStopOrder.Add('server')
            $record.Server.Stopped = $true
        }
        Sync-PartisanV2Ledger -Record $record
        foreach ($entry in $record.Ledger.ToArray()) {
            $status = Get-PartisanProcessIdentityStatus -Identity $entry.Identity
            if ([string]$status.Status -cne 'dead') {
                Throw-PartisanV2 `
                    -Identifier 'PGR_TEARDOWN_PROCESS_REMAINS' `
                    -Message 'Private owned process is live or unknown after ordered stop.'
            }
        }
        if (-not $record.JobClosed -and
            @($record.Job.GetProcessIds()).Count -ne 0) {
            Throw-PartisanV2 `
                -Identifier 'PGR_TEARDOWN_JOB_NOT_EMPTY' `
                -Message 'Private job is not empty after fixed-point stop.'
        }
        if (@(Get-PartisanEngineProcesses).Count -ne 0) {
            Throw-PartisanV2 `
                -Identifier 'PGR_TEARDOWN_ENGINE_REMAINS' `
                -Message 'Engine process exists after private ordered stop.'
        }
        foreach ($port in $record.Ports) {
            [void](Wait-PartisanLoopbackPortReleased `
                -Port ([int]$port.Port) `
                -Protocol ([string]$port.Protocol) `
                -TimeoutSeconds $PortReleaseTimeoutSeconds)
        }
        [void](Invoke-PartisanV2TeardownCertificationCheck `
            -Record $record `
            -Identifier 'PGR_TEARDOWN_STAGE_INVALID' `
            -Action { Assert-PartisanV2StageExact -Record $record })
        [void](Invoke-PartisanV2TeardownCertificationCheck `
            -Record $record `
            -Identifier 'PGR_TEARDOWN_EXECUTABLE_INVALID' `
            -Action { Assert-PartisanV2ExecutableBindings -Record $record })
        [void](Invoke-PartisanV2TeardownCertificationCheck `
            -Record $record `
            -Identifier 'PGR_TEARDOWN_TOOL_INVALID' `
            -Action {
                if (-not (Test-PartisanFileSignatureExact `
                        -Expected $record.ModuleToolSignature `
                        -Actual (Get-PartisanFileSignature `
                            -Path $record.ModuleToolPath))) {
                    Throw-PartisanV2 `
                        -Identifier 'PGR_GUARDED_TOOL_CHANGED' `
                        -Message 'Guarded runtime module bytes changed during teardown.'
                }
            })
        [void](Invoke-PartisanV2TeardownCertificationCheck `
            -Record $record `
            -Identifier 'PGR_TEARDOWN_BOUNDARY_INVALID' `
            -Action {
                $boundaryDelta = Compare-PartisanBoundarySnapshotSet `
                    -Snapshots ([object[]]$record.Boundaries)
                if (-not $boundaryDelta.Clean) {
                    Throw-PartisanV2 `
                        -Identifier 'PGR_TEARDOWN_BOUNDARY_DELTA' `
                        -Message 'Original private boundary baseline changed.'
                }
            })
        [void](Invoke-PartisanV2TeardownCertificationCheck `
            -Record $record `
            -Identifier 'PGR_TEARDOWN_GUARD_INVALID' `
            -Action { Assert-PartisanV2GuardExact -Record $record })
        if ($record.SelfTestFaults -contains 'handle-dispose-failure') {
            $record.SelfTestFaults = [string[]]@($record.SelfTestFaults |
                Where-Object { $_ -cne 'handle-dispose-failure' })
            Throw-PartisanV2 `
                -Identifier 'PGR_SELFTEST_HANDLE_DISPOSE' `
                -Message 'Injected fallible handle-dispose failure.'
        }
        $launches = New-Object Collections.Generic.List[object]
        foreach ($client in $record.Clients.ToArray()) {
            [void]$launches.Add($client)
        }
        if ($record.Server) { [void]$launches.Add($record.Server) }
        foreach ($launch in $launches.ToArray()) {
            $launch.Process.Dispose()
        }
        foreach ($processHandle in $record.SelfTestProcessHandles.ToArray()) {
            $processHandle.Dispose()
        }
        if (-not $record.JobClosed -and $record.Job) {
            $record.Job.Dispose()
            $record.JobClosed = $true
            $record.Job = $null
        }
        if ($record.PermanentNoGo) {
            Update-PartisanV2PublicProjection -Record $record
            Throw-PartisanV2 `
                -Identifier 'PGR_TEARDOWN_PERMANENT_NO_GO' `
                -Message 'Processes and handles stopped; evidence and guard retained permanently.'
        }
        if (Test-Path -LiteralPath $record.ReceiptPath) {
            Throw-PartisanV2 `
                -Identifier 'PGR_PREEXISTING_CLEAN_RECEIPT' `
                -Message 'Pre-existing clean receipt is untrusted and preserved.'
        }
        if (Test-Path -LiteralPath $record.CompletionAttestationPath) {
            Throw-PartisanV2 `
                -Identifier 'PGR_PREEXISTING_COMPLETION_ATTESTATION' `
                -Message 'Pre-existing completion attestation is untrusted and preserved.'
        }
        Write-PartisanV2Journal `
            -Record $record `
            -Mode 'guard-removal-authorized'
        $launchBindings = @($launches.ToArray() | ForEach-Object {
            [ordered]@{
                role = [string]$_.Role
                identity = Copy-PartisanV2PlainValue -Value $_.Identity
                path = [string]$_.ExecutablePath
                provenance = Copy-PartisanV2PlainValue `
                    -Value $_.ExecutableProvenance
                arguments = [string[]]$_.Arguments
                workingDirectory = [string]$_.WorkingDirectory
                isEngine = [bool]$_.IsEngine
                candidateConsumptionEvidence = Copy-PartisanV2PlainValue `
                    -Value $_.CandidateConsumptionEvidence
            }
        })
        $ledgerBindings = @($record.Ledger.ToArray() | ForEach-Object {
            [ordered]@{
                role = [string]$_.Role
                identity = Copy-PartisanV2PlainValue -Value $_.Identity
                recordedUtc = ([datetime]$_.RecordedUtc).ToString('o')
            }
        })
        $completionRecordedUtc = [DateTime]::UtcNow.ToString(
            'o',
            [Globalization.CultureInfo]::InvariantCulture)
        $record.CompletionToken = [Guid]::NewGuid().ToString('N')
        $completionTokenSha256 = Get-PartisanSha256Text `
            -Text ([string]$record.CompletionToken + "`n")
        $completionValue = [ordered]@{
            version = 1
            magic = $script:RuntimeAttestationMagic
            nonce = [string]$record.Nonce
            guardBindingSha256 = [string]$record.GuardBindingSha256
            guardDirectory = [string]$record.Guard.Directory
            candidateBindingSha256 = [string]$record.CandidateBindingSha256
            completionToken = [string]$record.CompletionToken
            receiptPath = [string]$record.ReceiptPath
            status = 'guard-removed-by-exact-owner'
            recordedUtc = $completionRecordedUtc
        }
        $completionArtifact = New-PartisanV2PortableJsonArtifact `
            -Value $completionValue
        $record.CompletionAttestationSignature =
            Copy-PartisanV2PlainValue -Value $completionArtifact.Signature
        $pendingValue = [ordered]@{
            version = 1
            magic = $script:RuntimeAttestationMagic
            nonce = [string]$record.Nonce
            guardBindingSha256 = [string]$record.GuardBindingSha256
            guardDirectory = [string]$record.Guard.Directory
            candidateBindingSha256 = [string]$record.CandidateBindingSha256
            completionTokenSha256 = $completionTokenSha256
            receiptPath = [string]$record.ReceiptPath
            status = 'pending-guard-removal'
            recordedUtc = $completionRecordedUtc
        }
        $pendingArtifact = New-PartisanV2PortableJsonArtifact `
            -Value $pendingValue
        $receipt = [ordered]@{
            version = 2
            magic = $script:RuntimeReceiptMagic
            nonce = [string]$record.Nonce
            guardBindingSha256 = [string]$record.GuardBindingSha256
            guardDirectory = [string]$record.Guard.Directory
            journalPath = [string]$record.JournalActivePath
            journalSignature = Copy-PartisanV2PlainValue `
                -Value $record.JournalSignature
            completionAttestationPath =
                [string]$record.CompletionAttestationPath
            completionAttestationSignature = Copy-PartisanV2PlainValue `
                -Value $record.CompletionAttestationSignature
            completionTokenSha256 = $completionTokenSha256
            candidateBindingSha256 = [string]$record.CandidateBindingSha256
            guardInventorySha256 = Get-PartisanV2Digest `
                -Value ([object[]]$record.GuardInventory)
            boundaryBaselineSha256 = Get-PartisanV2Digest `
                -Value $record.Boundaries
            executableBindingsSha256 = Get-PartisanV2Digest `
                -Value $launchBindings
            processLedgerSha256 = Get-PartisanV2Digest -Value $ledgerBindings
            launchBindings = [object[]]$launchBindings
            processLedger = [object[]]$ledgerBindings
            guardedToolHashes = [object[]]@(
                [ordered]@{
                    role = 'guarded-runtime-module'
                    path = [string]$record.ModuleToolPath
                    signature = Copy-PartisanV2PlainValue `
                        -Value $record.ModuleToolSignature
                })
            portSemantics = 'ipv4-loopback-bind-availability-only-no-pid-attribution'
            candidateConsumptionBridge =
                'exact-stage-and-candidate-argv-enforced-for-engine-launches'
            teardownStopOrder = [string[]]$record.TeardownStopOrder.ToArray()
            guardRemovalAuthorized = $true
            completionSemantics =
                'complete-only-with-exact-post-removal-attestation'
            recordedUtc = [DateTime]::UtcNow.ToString(
                'o',
                [Globalization.CultureInfo]::InvariantCulture)
        }
        $record.ReceiptSignature = Write-PartisanPortableJsonAtomic `
            -Path $record.ReceiptPath `
            -Value $receipt
        Update-PartisanV2PublicProjection -Record $record
        $result = [pscustomobject][ordered]@{
            ContextId = [string]$record.Nonce
            State = 'closed'
            ClientsStoppedBeforeServer = $true
            TeardownStopOrder = [string[]]$record.TeardownStopOrder.ToArray()
            JobClosed = $true
            GuardRemovalWasFinalAction = $true
            CleanReceiptPath = [string]$record.ReceiptPath
            CleanReceiptSignature = Copy-PartisanV2PlainValue `
                -Value $record.ReceiptSignature
            CompletionAttestationPath =
                [string]$record.CompletionAttestationPath
            CompletionAttestationSignature = Copy-PartisanV2PlainValue `
                -Value $record.CompletionAttestationSignature
            IPv4LoopbackPortsBindAvailable = $true
            PortSemantics = 'availability-only-no-pid-attribution'
            BoundariesClean = $true
        }
        $guardRegistryKey = [string]$record.Guard.Directory
        $attestationStream = $null
        try {
            $attestationStream = New-Object IO.FileStream(
                $record.CompletionAttestationPath,
                [IO.FileMode]::CreateNew,
                [IO.FileAccess]::ReadWrite,
                [IO.FileShare]::Read)
            $attestationStream.Write(
                [byte[]]$pendingArtifact.Bytes,
                0,
                [int]$pendingArtifact.Bytes.Length)
            $attestationStream.Flush($true)
            $record.FinalTransactionStarted = $true
            if ($record.SelfTestFaults -contains 'crash-after-receipt') {
                $record.SelfTestFaults = [string[]]@($record.SelfTestFaults |
                    Where-Object { $_ -cne 'crash-after-receipt' })
                Throw-PartisanV2 `
                    -Identifier 'PGR_SELFTEST_CRASH_AFTER_RECEIPT' `
                    -Message 'Injected crash point retained a pending completion attestation.'
            }
            if ($record.SelfTestFaults -contains 'missing-guard-final') {
                $record.SelfTestFaults = [string[]]@($record.SelfTestFaults |
                    Where-Object { $_ -cne 'missing-guard-final' })
                [void](Remove-PartisanGuardDirectoryCore `
                    -Ownership $record.Guard `
                    -ExpectedInventory ([object[]]$record.GuardInventory) `
                    -RequirePresent)
                $record.GuardRemoved = $true
                Throw-PartisanV2 `
                    -Identifier 'PGR_SELFTEST_GUARD_DISAPPEARED' `
                    -Message 'Injected guard disappearance left completion pending.'
            }
            [void](Remove-PartisanGuardDirectoryCore `
                -Ownership $record.Guard `
                -ExpectedInventory ([object[]]$record.GuardInventory) `
                -RequirePresent)
            $record.GuardRemoved = $true
            # The stream and both byte sequences were prepared before guard
            # removal.  This durable overwrite is the sole post-removal
            # filesystem commit; no absence probe is used as certification.
            $attestationStream.Position = 0
            $attestationStream.SetLength(0)
            $attestationStream.Write(
                [byte[]]$completionArtifact.Bytes,
                0,
                [int]$completionArtifact.Bytes.Length)
            $attestationStream.Flush($true)
            $attestationStream.Dispose()
            $attestationStream = $null
            $record.FinalTransactionCompleted = $true
        }
        finally {
            if ($null -ne $attestationStream) {
                $attestationStream.Dispose()
            }
        }
        $record.State = 'closed'
        $record.PublicContext.State = 'closed'
        $record.PublicContext.GuardRemoved = $true
        [void]$script:RegisteredGuards.Remove($guardRegistryKey)
        [void]$script:RuntimeRegistry.Remove([string]$record.Nonce)
        return $result
    }
    catch {
        if (-not $record.PermanentNoGo) {
            Set-PartisanV2PermanentNoGo `
                -Record $record `
                -Identifier 'PGR_TEARDOWN_FAILED' `
                -Message $_.Exception.Message `
                -SkipJournal:$record.FinalTransactionStarted
        }
        throw
    }
}

function Test-PartisanGuardedRuntimeReceipt {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][AllowNull()]$ExpectedSignature
    )

    if ($null -eq $ExpectedSignature -or
        -not (Test-PartisanFileSignatureExact `
            -Expected $ExpectedSignature `
            -Actual $ExpectedSignature) -or
        [long]$ExpectedSignature.length -lt 1) {
        Throw-PartisanV2 `
            -Identifier 'PGR_EXPECTED_RECEIPT_SIGNATURE_REQUIRED' `
            -Message 'A trusted, exact expected receipt signature is mandatory.'
    }
    $receiptPath = Resolve-PartisanExistingPath -Path $Path -Kind Leaf
    Assert-PartisanNoReparseAncestry -Path $receiptPath
    $actualReceiptSignature = Get-PartisanFileSignature -Path $receiptPath
    if (-not (Test-PartisanFileSignatureExact `
            -Expected $ExpectedSignature `
            -Actual $actualReceiptSignature)) {
        Throw-PartisanV2 `
            -Identifier 'PGR_RECEIPT_SIGNATURE_MISMATCH' `
            -Message 'Clean receipt bytes differ from the expected signature.'
    }
    $receipt = Read-PartisanPortableJson -Path $receiptPath
    try {
        Assert-PartisanProperties `
            -Value $receipt `
            -Names @(
                'version',
                'magic',
                'nonce',
                'guardBindingSha256',
                'guardDirectory',
                'journalPath',
                'journalSignature',
                'completionAttestationPath',
                'completionAttestationSignature',
                'completionTokenSha256',
                'candidateBindingSha256',
                'guardInventorySha256',
                'boundaryBaselineSha256',
                'executableBindingsSha256',
                'processLedgerSha256',
                'launchBindings',
                'processLedger',
                'guardedToolHashes',
                'portSemantics',
                'candidateConsumptionBridge',
                'teardownStopOrder',
                'guardRemovalAuthorized',
                'completionSemantics',
                'recordedUtc') `
            -Label 'Guarded runtime clean receipt'
        if ([int]$receipt.version -ne 2 -or
            [string]$receipt.magic -cne $script:RuntimeReceiptMagic -or
            [string]$receipt.nonce -cnotmatch '^[0-9a-f]{32}$' -or
            [string]$receipt.guardBindingSha256 -cnotmatch '^[0-9a-f]{64}$' -or
            [string]$receipt.candidateBindingSha256 -cnotmatch '^[0-9a-f]{64}$' -or
            [string]$receipt.guardInventorySha256 -cnotmatch '^[0-9a-f]{64}$' -or
            [string]$receipt.boundaryBaselineSha256 -cnotmatch '^[0-9a-f]{64}$' -or
            [string]$receipt.executableBindingsSha256 -cnotmatch '^[0-9a-f]{64}$' -or
            [string]$receipt.processLedgerSha256 -cnotmatch '^[0-9a-f]{64}$' -or
            [string]$receipt.completionTokenSha256 -cnotmatch '^[0-9a-f]{64}$' -or
            -not (Test-PartisanFileSignatureExact `
                -Expected $receipt.completionAttestationSignature `
                -Actual $receipt.completionAttestationSignature) -or
            -not [bool]$receipt.guardRemovalAuthorized -or
            [string]$receipt.completionSemantics -cne
                'complete-only-with-exact-post-removal-attestation' -or
            [string]$receipt.portSemantics -cne
                'ipv4-loopback-bind-availability-only-no-pid-attribution' -or
            [string]$receipt.candidateConsumptionBridge -cne
                'exact-stage-and-candidate-argv-enforced-for-engine-launches') {
            Throw-PartisanV2 `
                -Identifier 'PGR_RECEIPT_SCHEMA' `
                -Message 'Clean receipt schema or fixed semantics are invalid.'
        }
        $receiptLaunchBindings = @($receipt.launchBindings | Where-Object {
            $null -ne $_
        })
        $receiptProcessLedger = @($receipt.processLedger | Where-Object {
            $null -ne $_
        })
        if ((Get-PartisanV2Digest -Value $receiptLaunchBindings) -cne
                [string]$receipt.executableBindingsSha256 -or
            (Get-PartisanV2Digest -Value $receiptProcessLedger) -cne
                [string]$receipt.processLedgerSha256) {
            Throw-PartisanV2 `
                -Identifier 'PGR_RECEIPT_BINDING_DIGEST' `
                -Message 'Clean receipt launch or process-ledger digest is invalid.'
        }
        foreach ($binding in $receiptLaunchBindings) {
            Assert-PartisanProperties `
                -Value $binding `
                -Names @(
                    'role',
                    'identity',
                    'path',
                    'provenance',
                    'arguments',
                    'workingDirectory',
                    'isEngine',
                    'candidateConsumptionEvidence') `
                -Label 'Clean receipt launch binding'
            Assert-PartisanProperties `
                -Value $binding.candidateConsumptionEvidence `
                -Names @(
                    'mode',
                    'candidateBindingSha256',
                    'exactArgumentPairs') `
                -Label 'Clean receipt candidate-consumption evidence'
            if ([string]$binding.candidateConsumptionEvidence.candidateBindingSha256 `
                    -cne [string]$receipt.candidateBindingSha256) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_RECEIPT_CANDIDATE_CONSUMPTION' `
                    -Message 'Launch candidate-consumption evidence is not receipt-bound.'
            }
            $expectedConsumptionMode = if ([bool]$binding.isEngine) {
                'exact-stage-and-candidate-argv-enforced'
            }
            else { 'non-engine-self-test-only' }
            if ([string]$binding.candidateConsumptionEvidence.mode -cne
                    $expectedConsumptionMode -or
                ([bool]$binding.isEngine -and
                    @($binding.candidateConsumptionEvidence.exactArgumentPairs).Count `
                        -ne 4)) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_RECEIPT_CANDIDATE_CONSUMPTION' `
                    -Message 'Launch candidate-consumption mode or exact argument pairs are invalid.'
            }
        }
        foreach ($entry in $receiptProcessLedger) {
            Assert-PartisanProperties `
                -Value $entry `
                -Names @('role', 'identity', 'recordedUtc') `
                -Label 'Clean receipt process ledger entry'
        }
        foreach ($tool in @($receipt.guardedToolHashes)) {
            Assert-PartisanProperties `
                -Value $tool `
                -Names @('role', 'path', 'signature') `
                -Label 'Clean receipt guarded tool binding'
            if ([string]$tool.signature.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
                [long]$tool.signature.length -lt 1) {
                Throw-PartisanV2 `
                    -Identifier 'PGR_RECEIPT_TOOL_HASH' `
                    -Message 'Clean receipt guarded tool hash is invalid.'
            }
        }
        $evidenceBase = Split-Path -Parent $receiptPath
        $expectedReceiptPath = Get-PartisanFullPath -Path (Join-Path `
            $evidenceBase `
            ('.PartisanGuardedRuntime_' + [string]$receipt.nonce +
                '.receipt.json'))
        $expectedJournalPath = Get-PartisanFullPath -Path (Join-Path `
            $evidenceBase `
            ('.PartisanGuardedRuntime_' + [string]$receipt.nonce +
                '.journal.json'))
        $expectedFaultJournalPath = Get-PartisanFullPath -Path (Join-Path `
            $evidenceBase `
            ('.PartisanGuardedRuntime_' + [string]$receipt.nonce +
                '.journal-fault.json'))
        $expectedAttestationPath = Get-PartisanFullPath -Path (Join-Path `
            $evidenceBase `
            ('.PartisanGuardedRuntime_' + [string]$receipt.nonce +
                '.completion.json'))
        $journalPath = Get-PartisanFullPath -Path ([string]$receipt.journalPath)
        $attestationPath = Get-PartisanFullPath -Path (
            [string]$receipt.completionAttestationPath)
        $guardDirectory = Get-PartisanFullPath -Path (
            [string]$receipt.guardDirectory)
        $expectedGuardDirectory = Get-PartisanFullPath -Path (Join-Path `
            $evidenceBase `
            ($script:GuardLeafPrefix + [string]$receipt.nonce))
        if (-not $receiptPath.Equals(
                $expectedReceiptPath,
                [StringComparison]::OrdinalIgnoreCase) -or
            -not $journalPath.Equals(
                $expectedJournalPath,
                [StringComparison]::OrdinalIgnoreCase) -or
            -not $attestationPath.Equals(
                $expectedAttestationPath,
                [StringComparison]::OrdinalIgnoreCase) -or
            -not $guardDirectory.Equals(
                $expectedGuardDirectory,
                [StringComparison]::OrdinalIgnoreCase)) {
            Throw-PartisanV2 `
                -Identifier 'PGR_RECEIPT_PATH_BINDING' `
                -Message 'Clean receipt evidence paths do not match its nonce.'
        }
        if (-not (Test-Path -LiteralPath $journalPath -PathType Leaf) -or
            -not (Test-PartisanFileSignatureExact `
                -Expected $receipt.journalSignature `
                -Actual (Get-PartisanFileSignature -Path $journalPath))) {
            Throw-PartisanV2 `
                -Identifier 'PGR_RECEIPT_JOURNAL_SIGNATURE' `
                -Message 'Authorized guard-removal journal is missing or changed.'
        }
        $journal = Read-PartisanPortableJson -Path $journalPath
        if ([int]$journal.version -ne 2 -or
            [string]$journal.magic -cne $script:RuntimeJournalMagic -or
            [string]$journal.nonce -cne [string]$receipt.nonce -or
            [string]$journal.guardBindingSha256 -cne
                [string]$receipt.guardBindingSha256 -or
            [string]$journal.mode -cne 'guard-removal-authorized') {
            Throw-PartisanV2 `
                -Identifier 'PGR_RECEIPT_JOURNAL_IDENTITY' `
                -Message 'Authorized guard-removal journal identity is invalid.'
        }
        $hasFinalRemovalFault = Test-Path `
            -LiteralPath $expectedFaultJournalPath `
            -PathType Leaf
        $attestationExact = $false
        $attestationStatus = 'missing'
        $actualAttestationSignature = $null
        if (Test-Path -LiteralPath $attestationPath -PathType Leaf) {
            try {
                $actualAttestationSignature = Get-PartisanFileSignature `
                    -Path $attestationPath
                if (Test-PartisanFileSignatureExact `
                        -Expected $receipt.completionAttestationSignature `
                        -Actual $actualAttestationSignature) {
                    $attestation = Read-PartisanPortableJson `
                        -Path $attestationPath
                    Assert-PartisanProperties `
                        -Value $attestation `
                        -Names @(
                            'version',
                            'magic',
                            'nonce',
                            'guardBindingSha256',
                            'guardDirectory',
                            'candidateBindingSha256',
                            'completionToken',
                            'receiptPath',
                            'status',
                            'recordedUtc') `
                        -Label 'Completion attestation'
                    $attestationStatus = [string]$attestation.status
                    $attestationExact = [int]$attestation.version -eq 1 -and
                        [string]$attestation.magic -ceq
                            $script:RuntimeAttestationMagic -and
                        [string]$attestation.nonce -ceq
                            [string]$receipt.nonce -and
                        [string]$attestation.guardBindingSha256 -ceq
                            [string]$receipt.guardBindingSha256 -and
                        (Get-PartisanFullPath `
                            -Path ([string]$attestation.guardDirectory)).Equals(
                                $guardDirectory,
                                [StringComparison]::OrdinalIgnoreCase) -and
                        [string]$attestation.candidateBindingSha256 -ceq
                            [string]$receipt.candidateBindingSha256 -and
                        (Get-PartisanSha256Text -Text (
                            [string]$attestation.completionToken + "`n")) -ceq
                            [string]$receipt.completionTokenSha256 -and
                        (Get-PartisanFullPath `
                            -Path ([string]$attestation.receiptPath)).Equals(
                                $receiptPath,
                                [StringComparison]::OrdinalIgnoreCase) -and
                        $attestationStatus -ceq 'guard-removed-by-exact-owner'
                }
                else {
                    $attestationStatus = 'signature-mismatch'
                }
            }
            catch {
                $attestationStatus = 'invalid'
                $attestationExact = $false
            }
        }
        $guardAbsent = -not (Test-Path -LiteralPath $guardDirectory)
        $complete = $guardAbsent -and $attestationExact
        return [pscustomobject][ordered]@{
            Status = if ($complete) { 'complete' }
                else { 'incomplete-permanent-no-go' }
            Certification = if ($complete) { 'clean' }
                else { 'permanent-no-go' }
            Complete = [bool]$complete
            GuardDirectoryAbsent = [bool]$guardAbsent
            FinalRemovalFaultPresent = [bool]$hasFinalRemovalFault
            CompletionAttestationExact = [bool]$attestationExact
            CompletionAttestationStatus = $attestationStatus
            CompletionAttestationPath = $attestationPath
            CompletionAttestationSignature = $actualAttestationSignature
            ContextId = [string]$receipt.nonce
            ReceiptPath = $receiptPath
            ReceiptSignature = $actualReceiptSignature
            JournalPath = $journalPath
            JournalSignature = Get-PartisanFileSignature -Path $journalPath
            FaultJournalPath = if ($hasFinalRemovalFault) {
                $expectedFaultJournalPath
            }
            else { $null }
            CandidateBindingSha256 = [string]$receipt.candidateBindingSha256
            TeardownStopOrder = [string[]]@($receipt.teardownStopOrder)
        }
    }
    catch {
        if ($_.Exception.Message -match '^\[PGR_') { throw }
        Throw-PartisanV2 `
            -Identifier 'PGR_RECEIPT_INVALID' `
            -Message $_.Exception.Message
    }
}

Export-ModuleMember -Function `
    Assert-PartisanGitWorktreeFilesMatchCommit, `
    Assert-PartisanCandidateStage, `
    Assert-PartisanExecutableProvenance, `
    Assert-PartisanLoopbackPortAvailable, `
    Assert-PartisanNoReparseAncestry, `
    Assert-PartisanNoReparseTree, `
    Assert-PartisanRuntimeOwnershipAudit, `
    Compare-PartisanBoundarySnapshotSet, `
    Compare-PartisanRootSnapshot, `
    ConvertTo-PartisanNativeArgument, `
    ConvertTo-PartisanNativeCommandLine, `
    Get-PartisanExecutableProvenance, `
    Get-PartisanFileSignature, `
    Get-PartisanGuardOwnership, `
    Get-PartisanProcessIdentity, `
    Get-PartisanProcessIdentityStatus, `
    Get-PartisanSha256, `
    Invoke-PartisanGuardedTeardown, `
    New-PartisanBoundarySnapshotSet, `
    New-PartisanCandidateStage, `
    New-PartisanGuardDirectory, `
    New-PartisanGuardedRuntimeContext, `
    New-PartisanRootSnapshot, `
    Read-PartisanPortableJson, `
    Remove-PartisanGuardDirectory, `
    Start-PartisanGuardedClient, `
    Start-PartisanGuardedServer, `
    Stop-PartisanGuardedProcess, `
    Test-PartisanContainedPath, `
    Test-PartisanExactNativeArgumentVector, `
    Test-PartisanLoopbackPortAvailable, `
    Test-PartisanProcessIdentity, `
    Test-PartisanGuardedRuntimeReceipt, `
    Wait-PartisanGuardedProcess, `
    Wait-PartisanLoopbackPortReleased, `
    Write-PartisanPortableJson

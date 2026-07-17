[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [string]$ProjectPath,

    [Parameter(Mandatory = $true)]
    [string]$AddonRoot,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z_][A-Za-z0-9_]*$')]
    [string]$TestCase,

    [string[]]$RequiredLogPatterns = @(),
    [string[]]$DefaultRoots = @(),
    [string[]]$SpillRoots = @(),

    [ValidateRange(1, 3600)]
    [int]$TimeoutSeconds = 240,

    [ValidateRange(50, 5000)]
    [int]$PollMilliseconds = 500
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

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
    $rootTests = [string]$root.GetAttribute('tests')
    $rootFailures = [string]$root.GetAttribute('failures')
    $rootErrors = [string]$root.GetAttribute('errors')
    if (-not [string]::IsNullOrEmpty($rootTests)) {
        $tests = [int]$rootTests
        if (-not [string]::IsNullOrEmpty($rootFailures)) {
            $failures = [int]$rootFailures
        }
        if (-not [string]::IsNullOrEmpty($rootErrors)) {
            $errors = [int]$rootErrors
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
            if (-not [string]::IsNullOrEmpty($suiteTests)) {
                $tests += [int]$suiteTests
            }
            if (-not [string]::IsNullOrEmpty($suiteFailures)) {
                $failures += [int]$suiteFailures
            }
            if (-not [string]::IsNullOrEmpty($suiteErrors)) {
                $errors += [int]$suiteErrors
            }
        }
    }
    $testCases = @($document.SelectNodes('//testcase'))
    $caseName = ''
    $caseClassName = ''
    $caseFailures = 0
    $caseErrors = 0
    $caseSkipped = 0
    $caseFailureText = ''
    $caseErrorText = ''
    if ($testCases.Count -eq 1) {
        $testCase = $testCases[0]
        $caseName = [string]$testCase.GetAttribute('name')
        $caseClassName = [string]$testCase.GetAttribute('classname')
        if ([string]::IsNullOrEmpty($caseClassName)) {
            $caseClassName = [string]$testCase.GetAttribute('class')
        }
        $failureNodes = @($testCase.SelectNodes('./failure'))
        $errorNodes = @($testCase.SelectNodes('./error'))
        $caseFailures = $failureNodes.Count
        $caseErrors = $errorNodes.Count
        $caseSkipped = @($testCase.SelectNodes('./skipped')).Count
        if ($failureNodes.Count -gt 0) {
            $caseFailureText = [string]$failureNodes[0].GetAttribute('message')
            if ([string]::IsNullOrWhiteSpace($caseFailureText)) {
                $caseFailureText = [string]$failureNodes[0].InnerText
            }
        }
        if ($errorNodes.Count -gt 0) {
            $caseErrorText = [string]$errorNodes[0].GetAttribute('message')
            if ([string]::IsNullOrWhiteSpace($caseErrorText)) {
                $caseErrorText = [string]$errorNodes[0].InnerText
            }
        }
    }
    return [pscustomobject]@{
        Tests = $tests
        Failures = $failures
        Errors = $errors
        TestCaseCount = $testCases.Count
        CaseName = $caseName
        CaseClassName = $caseClassName
        CaseIdentityExact = $testCases.Count -eq 1 -and
            ($caseName -ceq $ExpectedTestCase -or
                $caseClassName -ceq $ExpectedTestCase)
        CaseFailures = $caseFailures
        CaseErrors = $caseErrors
        CaseSkipped = $caseSkipped
        CaseFailureText = $caseFailureText
        CaseErrorText = $caseErrorText
    }
}

$executablePath = Resolve-ExistingPath -Path $Executable -Kind Leaf
$projectFile = Resolve-ExistingPath -Path $ProjectPath -Kind Leaf
$projectDirectory = Split-Path -Parent $projectFile
$addonDirectory = Resolve-ExistingPath -Path $AddonRoot -Kind Container
if ((Split-Path -Leaf $executablePath) -ine 'ArmaReforgerSteamDiag.exe') {
    throw 'Focused autotest requires ArmaReforgerSteamDiag.exe.'
}
if ([IO.Path]::GetExtension($projectFile) -cne '.gproj') {
    throw 'Focused autotest project must be a .gproj file.'
}

$normalizedDefaultRoots = @($DefaultRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
$normalizedSpillRoots = @($SpillRoots | Where-Object {
    -not [string]::IsNullOrWhiteSpace([string]$_)
})
if ($normalizedDefaultRoots.Count -eq 0 -or $normalizedSpillRoots.Count -eq 0) {
    throw 'Focused autotest requires explicit default-root and spill-root monitoring.'
}
$diagnosticRedactedPaths = @(
    $executablePath,
    $projectFile,
    $projectDirectory,
    $addonDirectory
) + $normalizedDefaultRoots + $normalizedSpillRoots

$guardBase = [IO.Path]::GetFullPath(
    (Join-Path ([IO.Path]::GetTempPath()) 'PartisanFocusedAutotest'))
$guardRoot = [IO.Path]::GetFullPath(
    (Join-Path $guardBase ([Guid]::NewGuid().ToString('N'))))
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

    foreach ($root in $normalizedDefaultRoots) {
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
    [IO.File]::WriteAllText($sentinelPath, 'owned')
    $workingDirectory = Join-Path $guardRoot 'work'
    $tempDirectory = Join-Path $guardRoot 'temp'
    $profileDirectory = Join-Path $guardRoot 'profile'
    New-Item -ItemType Directory -Path $workingDirectory, $tempDirectory, $profileDirectory -Force | Out-Null
    $profileProofSentinelPath = Join-Path $profileDirectory '.partisan-focused-owner'
    [IO.File]::WriteAllText($profileProofSentinelPath, 'owned')

    $arguments = @(
        '-addonsDir', $addonDirectory,
        '-gproj', $projectFile,
        '-profile', $guardRoot,
        '-window',
        '-noFocus',
        '-forceupdate',
        '-rpl-timeout-disable',
        '-noThrow',
        '-autotest', $TestCase
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

    $consoleText = ''
    foreach ($consoleFile in $consoleFiles) {
        $consoleText += [IO.File]::ReadAllText($consoleFile.FullName)
    }
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
    foreach ($pattern in $RequiredLogPatterns) {
        if ([string]::IsNullOrEmpty($pattern)) {
            continue
        }
        if ($consoleText.IndexOf($pattern, [StringComparison]::Ordinal) -lt 0) {
            $requiredPatternsSeen = $false
        }
    }

    $result = [ordered]@{
        Success = $exitCode -eq 0 -and
            $junit.Tests -eq 1 -and
            $junit.Failures -eq 0 -and
            $junit.Errors -eq 0 -and
            $junit.TestCaseCount -eq 1 -and
            $junit.CaseIdentityExact -and
            $junit.CaseFailures -eq 0 -and
            $junit.CaseErrors -eq 0 -and
            $junit.CaseSkipped -eq 0 -and
            $failedFiles.Count -eq 1 -and
            $failedListBytes -eq 0 -and
            $requiredPatternsSeen
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
        ConsoleTestCaseSeen = $consoleText.IndexOf(
            $TestCase,
            [StringComparison]::Ordinal) -ge 0
    }
    if (-not $result.Success) {
        throw 'Focused autotest evidence did not pass.'
    }
}
catch {
    $runError = ConvertTo-SafeDiagnosticText `
        -Text $_.Exception.Message `
        -RedactedPaths ($diagnosticRedactedPaths + @($guardRoot))
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
            if (Test-Path -LiteralPath $guardRoot) {
                if (-not (Test-Path -LiteralPath $sentinelPath -PathType Leaf)) {
                    throw 'Focused-autotest cleanup ownership sentinel is missing.'
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
if ($cleanupFailed) {
    throw 'Focused-autotest cleanup did not converge to zero.'
}
if ($runError) {
    throw $runError
}
if (-not $result -or -not $result.Success) {
    throw 'Focused autotest did not pass.'
}

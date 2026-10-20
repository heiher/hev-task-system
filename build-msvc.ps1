# Native MSVC x64 / experimental x86 build; independent from the MSYS2 Makefile.
# PowerShell 5.1+: powershell -NoProfile -ExecutionPolicy Bypass -File .\build-msvc.ps1 -Target all
# CMD shortcut: build-msvc.bat -Target all -Arch x86 -VSVersion 2022
# Targets: lib (default), tests, apps, all, run-tests, verify, clean.
# run-tests builds all supported tests, then runs them with a per-test timeout.
# verify re-runs the static binary checks on already-built outputs.
param(
    [ValidateSet('lib', 'tests', 'apps', 'all', 'run-tests', 'verify', 'clean')]
    [string]$Target = 'lib',
    [ValidateSet('latest', '2026', '2022', '2019', '2017')]
    [string]$VSVersion = 'latest',
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Arch = 'x64',
    [ValidateRange(1, 3600)]
    [int]$TestTimeoutSeconds = 30,
    [string[]]$ExtraCflags = @(),
    [ValidateSet('MD', 'MDd', 'MT', 'MTd')]
    [string]$Runtime = 'MD'
)

$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
# Preserve the established x64 output paths. Use separate folders for x86 and
# native ARM64 so no architecture ever overwrites another's artifacts.
$BuildDir = if ($Arch -eq 'x64') {
    Join-Path $Root 'build-msvc'
} elseif ($Arch -eq 'x86') {
    Join-Path $Root 'build-msvc\x86'
} else {
    Join-Path $Root 'build-msvc\arm64'
}
$ObjDir = Join-Path $BuildDir 'obj'
$BinDir = Join-Path $BuildDir 'bin'
$LibPath = Join-Path $BuildDir 'hev-task-system.lib'
$MachineType = if ($Arch -eq 'x64') { '8664' } elseif ($Arch -eq 'x86') { '14C' } else { 'AA64' }
$AsmObjectPath = Join-Path $ObjDir 'src_kern_task_hev-task-execute.S.obj'

function Invoke-Native([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Executable failed with exit code $LASTEXITCODE"
    }
}

function Get-PeMachineTypes([string]$Path) {
    # Every COFF member of a library and every image declares its machine
    # type in its file header. Cross-compilation mistakes (ARM64EC sneaking
    # in, stale x64 objects reused) show up here.
    $out = & dumpbin.exe /nologo /headers $Path
    @($out | Where-Object { $_ -match '^\s*([0-9A-Fa-f]+) machine' } | ForEach-Object { $Matches[1].ToUpper() })
}

function Assert-PeMachine([string]$Path, [string]$Expected) {
    $machines = Get-PeMachineTypes $Path
    if ($machines.Count -eq 0) { throw "No machine type found in $Path" }
    $bad = @($machines | Where-Object { $_ -ne $Expected })
    if ($bad.Count -gt 0) {
        throw "$Path contains unexpected machine types ($($bad -join ', ')); expected $Expected only."
    }
}

function Assert-Arm64AsmUnwindSections([string]$ObjPath) {
    # clang-cl's integrated assembler emits unwind data only if the source
    # carries .seh_* directives. The context switch object must keep its
    # .pdata record describing the frame-chained hev_task_execute prologue
    # (save_fplr_x(16) + set_fp, function length 32 bytes) and its
    # epilogue. When the epilogue exactly mirrors the prologue, LLVM packs
    # the record into an 8-byte .pdata (second DWORD 0x00E00021, the same
    # encoding MSVC emits for such functions) and omits .xdata; otherwise a
    # full .xdata is emitted and must match the explicit-scope fingerprint.
    $out = & dumpbin.exe /nologo /headers $ObjPath
    if (-not ($out | Where-Object { $_ -match '^\s*\.pdata name' })) {
        throw "$ObjPath is missing the .pdata unwind section."
    }
    if ($out | Where-Object { $_ -match '^\s*\.xdata name' }) {
        $sectionOut = @(& dumpbin.exe /nologo /section:.xdata $ObjPath)
        $offset = 0
        foreach ($line in $sectionOut) {
            if ($line -match 'file pointer to raw data \((\w+) to') {
                $offset = [Convert]::ToInt64($Matches[1], 16)
                break
            }
        }
        if ($offset -gt 0) {
            $bytes = [IO.File]::ReadAllBytes($ObjPath)
            $xdata = ($bytes[$offset..($offset + 11)] | ForEach-Object { $_.ToString('X2') }) -join ' '
            if ($xdata -ne '08 00 E0 10 E1 81 E4 E1 42 E4 E3 E3') {
                throw "Unexpected .xdata encoding in $ObjPath ($xdata); the unwind data does not describe the frame-chained hev_task_execute."
            }
            return
        }
    }
    $sectionOut = @(& dumpbin.exe /nologo /section:.pdata $ObjPath)
    $offset = 0
    foreach ($line in $sectionOut) {
        if ($line -match 'file pointer to raw data \((\w+) to') {
            $offset = [Convert]::ToInt64($Matches[1], 16)
            break
        }
    }
    if ($offset -le 0) {
        throw "Could not locate the .pdata raw data of $ObjPath."
    }
    $bytes = [IO.File]::ReadAllBytes($ObjPath)
    $packed = ($bytes[($offset + 4)..($offset + 7)] | ForEach-Object { $_.ToString('X2') }) -join ' '
    if ($packed -ne '21 00 E0 00') {
        throw "Unexpected packed .pdata encoding in $ObjPath ($packed); the record does not describe the frame-chained hev_task_execute."
    }
}

function Assert-Arm64UnwindInfo([string]$ExePath) {
    # The frame-chained hev_task_execute record must survive linking and must
    # decode against the real instructions without dumpbin mismatches. Two
    # encodings are accepted:
    #   * full .xdata record (E=1 with an explicit epilogue scope), identified
    #     structurally: frame-chained prologue plus RawXdata, and the decoded
    #     block must contain the mov sp,fp epilogue without decoder mismatches;
    #   * packed .pdata record (Flag=1, CR=3, FrameSize=0x10, second DWORD
    #     0x00E00021): prologue stp fp,lr,[sp,#-0x10]! + mov fp,sp and the
    #     shared epilogue (ldp fp,lr,[sp],#0x10; ret) decoded by dumpbin.
    # Boundaries between table entries are the 'Begin Packed' lines.
    $lines = @(& dumpbin.exe /nologo /unwindinfo $ExePath)
    $starts = @()
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '\+0000 stp\s+fp,lr,\[sp,#-0x10\]!') { $starts += $i }
        elseif ($lines[$i] -match '01: 81\.{6} \+0000 stp   fp,lr,\[sp,#-0x10\]!') { $starts += $i }
    }
    if ($starts.Count -eq 0) {
        throw "No frame-chained hev_task_execute unwind entry found in $ExePath"
    }
    $found = $false
    foreach ($start in $starts) {
        $end = $start + 1
        while ($end -lt $lines.Count -and $lines[$end] -notmatch '^(Function Table|\s+[0-9A-Fa-f]{8} [0-9A-Fa-f]{8}\s)') { $end++ }
        $block = $lines[$start..($end - 1)]
        if ($block | Where-Object { $_ -match 'RawXdata=[0-9A-Fa-f]+' }) {
            $found = $true
            if (-not ($block | Where-Object { $_ -match 'mov   sp,fp' })) {
                throw "hev_task_execute unwind entry in $ExePath lost its epilogue."
            }
            if ($block | Where-Object { $_ -match '^\*\*\*\*' }) {
                throw "hev_task_execute unwind entry in $ExePath does not match the encoded instructions."
            }
            return
        }
        if ($block | Where-Object { $_ -match 'RawPdata=\w+ 00E00021' }) {
            $found = $true
            if (-not ($block | Where-Object { $_ -match 'mov\s+fp,sp' })) {
                throw "hev_task_execute packed unwind entry in $ExePath lost its frame-chained prologue."
            }
            if (-not ($block | Where-Object { $_ -match 'ldp\s+fp,lr,\[sp\],#0x10' }) -or
                -not ($block | Where-Object { $_ -match 'Epilog' })) {
                throw "hev_task_execute packed unwind entry in $ExePath lost its shared epilogue."
            }
            return
        }
    }
    if (-not $found) {
        throw "hev_task_execute unwind record (frame-chained prologue/epilogue) not found in $ExePath"
    }
}

function Invoke-StaticChecks([switch]$RequireExes) {
    if (-not (Test-Path -LiteralPath $LibPath)) { throw 'Missing static library for verification.' }
    $exes = @(Get-ChildItem -LiteralPath $BinDir -Filter '*.exe' -File -ErrorAction SilentlyContinue |
        ForEach-Object { $_.FullName })
    Assert-PeMachine $LibPath $MachineType
    Write-Host "  CHECK machine types in hev-task-system.lib ($MachineType)"
    foreach ($exe in $exes) {
        Assert-PeMachine $exe $MachineType
    }
    if ($exes.Count -gt 0) {
        Write-Host "  CHECK machine types in $($exes.Count) executables ($MachineType)"
    }
    if ($Arch -eq 'arm64') {
        Assert-Arm64AsmUnwindSections $AsmObjectPath
        Write-Host '  CHECK unwind sections and encoding in the assembled hev-task-execute.S object'
        if ($exes.Count -gt 0) {
            $probe = $exes | Where-Object { (Split-Path -Leaf $_) -eq 'tests-task.exe' } | Select-Object -First 1
            if (-not $probe) { $probe = $exes[0] }
            Assert-Arm64UnwindInfo $probe
            Write-Host "  CHECK unwind info for hev_task_execute in $(Split-Path -Leaf $probe)"
        } elseif ($RequireExes) {
            Write-Host ('  SKIP no ARM64 executables available (ARM64 MSVC CRT not installed); ' +
                'the unwind record is certified at object level instead.')
        }
    }
}

function Initialize-MSVC {
    # A pre-configured matching-architecture Native Tools prompt is supported.
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    $clang = Get-Command clang-cl.exe -ErrorAction SilentlyContinue
    $lib = Get-Command lib.exe -ErrorAction SilentlyContinue
    if ($VSVersion -eq 'latest' -and $cl -and $clang -and $lib -and
        $env:VSCMD_ARG_TGT_ARCH -eq $Arch) { return }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw 'vswhere.exe not found. Install Visual Studio / Build Tools with Desktop development with C++.'
    }
    $versionRanges = @{
        '2017' = '[15.0,16.0)'
        '2019' = '[16.0,17.0)'
        '2022' = '[17.0,18.0)'
        '2026' = '[18.0,19.0)'
    }
    $args = @('-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath')
    if ($VSVersion -ne 'latest') { $args += @('-version', $versionRanges[$VSVersion]) }
    $install = & $vswhere @args
    if ($LASTEXITCODE -ne 0 -or -not $install) {
        throw "Visual Studio $VSVersion with MSVC $Arch tools not found."
    }
    $vcvars = Join-Path $install 'VC\Auxiliary\Build\vcvarsall.bat'
    if (-not (Test-Path -LiteralPath $vcvars)) { throw "Missing vcvarsall.bat: $vcvars" }

    # vcvarsall argument for the requested target. ARM64 is native when the
    # host CPU is ARM64, otherwise x64_arm64 performs genuine (non-ARM64EC)
    # cross compilation of native ARM64 objects.
    $vcvarsArg = if ($Arch -eq 'arm64') {
        if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') { 'arm64' } else { 'x64_arm64' }
    } else {
        $Arch
    }

    Write-Host "Loading MSVC $Arch environment: $vcvars $vcvarsArg"
    # Avoid cmd.exe /s /c quoting ambiguities with paths such as "Program Files".
    # PowerShell invokes the generated .cmd directly, then imports its environment.
    $tempBase = Join-Path ([IO.Path]::GetTempPath()) ("hev-msvc-env-{0}" -f [guid]::NewGuid().ToString('N'))
    $tempBatch = "$tempBase.cmd"
    $tempLog = "$tempBase.log"
    $batchLines = @(
        '@echo off',
        ('call "' + $vcvars + '" ' + $vcvarsArg + ' > "' + $tempLog + '" 2>&1'),
        'if errorlevel 1 (',
        ('    type "' + $tempLog + '"'),
        '    exit /b 1',
        ')',
        'set'
    )
    try {
        Set-Content -LiteralPath $tempBatch -Value $batchLines -Encoding Default
        $lines = & $tempBatch
        if ($LASTEXITCODE -ne 0) { throw "vcvarsall.bat failed (exit code $LASTEXITCODE)." }
    } finally {
        foreach ($temporary in @($tempBatch, $tempLog)) {
            if (Test-Path -LiteralPath $temporary) {
                Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
            }
        }
    }
    foreach ($line in $lines) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }
    if ($env:VSCMD_ARG_TGT_ARCH -ne $Arch) { throw "Expected MSVC $Arch target environment." }
    foreach ($name in @('cl.exe', 'lib.exe', 'link.exe', 'clang-cl.exe')) {
        if (-not (Get-Command $name -ErrorAction SilentlyContinue)) { throw "$name not found in MSVC environment." }
    }
}

function Assert-C11AtomicsSupport {
}

$CommonDefines = @(
    '/D_CRT_SECURE_NO_WARNINGS',
    '/D_CRT_NONSTDC_NO_WARNINGS',
    '/D_WINSOCK_DEPRECATED_NO_WARNINGS',
    '/DCONFIG_STACK_BACKEND=STACK_HEAP',
    '/DCONFIG_STACK_OVERFLOW_DETECTION=1',
    '/DENABLE_STACK_OVERFLOW_DETECTION',
    '/DENABLE_MEMALLOC_SLICE',
    '/DCONFIG_MEMALLOC_SLICE_ALIGN=16',
    '/DCONFIG_MEMALLOC_SLICE_MAX_SIZE=4096',
    '/DCONFIG_MEMALLOC_SLICE_MAX_COUNT=1000',
    '/DCONFIG_SCHED_CLOCK=CLOCK_NONE'
)
$Includes = @(
    "/I$Root\include", "/I$Root\src",
    "/I$Root\win32-compat", "/I$Root\win32-compat\include"
)
$Cflags = @(
    '/nologo', '/std:c11', '/O2', '/W3', "/$Runtime",
    "/FI$Root\win32-compat\hev-win-compat.h"
) + $Includes + $CommonDefines
# clang-cl defaults to the host architecture and ignores the MSVC INCLUDE/LIB
# environment when selecting the target; pin the target explicitly per arch.
$ClangTarget = if ($Arch -eq 'x64') {
    'x86_64-pc-windows-msvc'
} elseif ($Arch -eq 'x86') {
    'i686-pc-windows-msvc'
} else {
    'aarch64-pc-windows-msvc'
}
# The library is built with clang-cl only: it compiles the C sources and its
# integrated assembler assembles the GAS hev-task-execute.S directly (the
# MASM/armasm64 .asm sources were removed). cl.exe cannot assemble GAS syntax,
# so it is no longer a supported build path.
# clang-cl understands most MSVC options but rejects /experimental:c11atomics
# (it ships a complete <stdatomic.h> itself) and has no /Zi-style PDB
# database; /Z7 keeps debug info in the objects. The MSVC INCLUDE/LIB
# environment selected by -Arch is reused unchanged.
$CompilerCmd = 'clang-cl.exe'
$Cflags += @('/Z7', "--target=$ClangTarget", '-Wno-microsoft-enum-forward-reference')
# Later flags win, so /Od here overrides the /O2 above.
$Cflags += $ExtraCflags
# Coroutine switching restores a saved stack directly and is incompatible
# with CET hardware shadow-stack instrumentation. lib.exe cannot set this;
# each final executable must explicitly opt out at link time. Consumers that
# link the .lib themselves MUST also use /CETCOMPAT:NO. ARM64 has no CET
# shadow stack, so the flag is applied only to the x64/x86 targets.
$ExeLinkFlags = @("--target=$ClangTarget", '/link')
if ($Arch -in @('x64', 'x86')) { $ExeLinkFlags += '/CETCOMPAT:NO' }
$ExeLinkFlags += @('Ws2_32.lib', 'Kernel32.lib')

function Build-Library {
    # A fresh object directory prevents an accidentally partial archive.
    if (Test-Path -LiteralPath $ObjDir) { Remove-Item -Recurse -Force -LiteralPath $ObjDir }
    if (Test-Path -LiteralPath $LibPath) { Remove-Item -Force -LiteralPath $LibPath }
    New-Item -ItemType Directory -Force -Path $ObjDir | Out-Null

    # POSIX-only stack implementations are excluded from native Windows builds.
    $sources = @(Get-ChildItem -LiteralPath (Join-Path $Root 'src') -Recurse -File -Filter '*.c' |
        Where-Object { $_.Name -notin @('hev-task-stack-mmap.c', 'hev-task-stack-detector.c') } |
        Sort-Object FullName)
    $compat = @(Get-ChildItem -LiteralPath (Join-Path $Root 'win32-compat') -File -Filter '*.c' | Sort-Object FullName)
    $objects = @()
    foreach ($src in ($sources + $compat)) {
        $rel = $src.FullName.Substring($Root.Length + 1) -replace '[\\/]', '_'
        $obj = Join-Path $ObjDir ($rel + '.obj')
        Write-Host "  CC  $rel"
        Invoke-Native $CompilerCmd ($Cflags + @('/c', "/Fo$obj", $src.FullName))
        $objects += $obj
    }

    $asmSrc = Join-Path $Root 'src\kern\task\hev-task-execute.S'
    $asmObj = Join-Path $ObjDir 'src_kern_task_hev-task-execute.S.obj'
    Write-Host '  ASM src_kern_task_hev-task-execute.S'
    # Assembly is preprocessed like C but must not receive C-only flags such
    # as the forced-include header or runtime selection; only the include
    # paths (for asm.h) and the pinned target are needed.
    $asmCflags = @('/nologo', '/c', "--target=$ClangTarget") + $Includes
    Invoke-Native $CompilerCmd ($asmCflags + @("/Fo$asmObj", $asmSrc))
    if ($Arch -eq 'arm64') {
        Assert-Arm64AsmUnwindSections $asmObj
    }
    $objects += $asmObj

    Write-Host '  LIB hev-task-system.lib'
    Invoke-Native 'lib.exe' (@('/nologo', "/OUT:$LibPath") + $objects)
}

function Test-Arm64CrtAvailable {
    # Linking ARM64 executables needs the MSVC ARM64 CRT (msvcrt.lib/
    # libcmt.lib under <VCToolsInstallDir>\lib\arm64). Some VS installations
    # ship without the ARM64 build tools component; the library and test
    # objects still cross-compile with clang-cl, only the exe link is skipped.
    $dir = $env:VCToolsInstallDir
    if (-not $dir) { return $false }
    return (Test-Path -LiteralPath (Join-Path $dir 'lib\arm64\msvcrt.lib'))
}

function Build-Programs([string]$Kind) {
    if (-not (Test-Path -LiteralPath $LibPath)) { throw 'Missing static library.' }
    New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
    $dir = Join-Path $Root $Kind
    $names = if ($Kind -eq 'apps') { @('simple.c', 'wakeup.c', 'channel.c', 'timeout.c', 'echo-server.c', 'call.c') } else { @() }
    # The following tests depend on genuine POSIX /dev, openat or UDP socketpair behaviour.
    $skip = @('io-open.c', 'io-openat.c', 'io-creat.c', 'io-socket.c')
    $failed = @()
    # Remove stale binaries for this kind, including tests no longer selected.
    Get-ChildItem -LiteralPath $BinDir -Filter "$Kind-*.exe" -File |
        Remove-Item -Force
    foreach ($src in (Get-ChildItem -LiteralPath $dir -File -Filter '*.c' | Sort-Object Name)) {
        if ($Kind -eq 'apps' -and $src.Name -notin $names) { continue }
        if ($Kind -eq 'tests' -and $src.Name -in $skip) {
            Write-Host "  SKIP (POSIX-only) $($src.Name)"
            continue
        }
        $name = [IO.Path]::GetFileNameWithoutExtension($src.Name)
        $exe = Join-Path $BinDir "$Kind-$name.exe"
        $obj = Join-Path $ObjDir "$Kind-$name.obj"
        # Do not reuse stale binaries from a previous successful build.
        foreach ($output in @($exe, $obj)) {
            if (Test-Path -LiteralPath $output) { Remove-Item -Force -LiteralPath $output }
        }
        if ($Arch -eq 'arm64' -and -not (Test-Arm64CrtAvailable)) {
            Write-Host "  SKIP (ARM64 CRT unavailable) $Kind-$name.exe"
            continue
        }
        Write-Host "  LINK $Kind-$name.exe"
        # clang-cl cannot mix a .lib and a source in one compile+link
        # invocation; produce the object first, then link it separately.
        Invoke-Native $CompilerCmd ($Cflags + @('/c', "/Fo$obj", $src.FullName))
        Invoke-Native $CompilerCmd (@("/Fe$exe", $obj, $LibPath) + $ExeLinkFlags)
    }
    if ($failed.Count -gt 0) {
        throw "Failed to build $($failed.Count) ${Kind}: $($failed -join ', ')"
    }
    Write-Host "All supported $Kind built successfully."
}

function Run-Tests {
    $tests = @(Get-ChildItem -LiteralPath $BinDir -Filter 'tests-*.exe' -File | Sort-Object Name)
    if ($tests.Count -eq 0) { throw 'No MSVC test executables found.' }
    $failed = @()
    foreach ($test in $tests) {
        Write-Host "  TEST $($test.Name)"
        # System.Diagnostics.Process is used because Start-Process -PassThru
        # (without -Wait) does not reliably expose ExitCode on Windows
        # PowerShell 5.1 and would report every passing test as failed.
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $test.FullName
        $psi.WorkingDirectory = $Root
        $psi.UseShellExecute = $false
        $proc = [System.Diagnostics.Process]::Start($psi)
        try {
            if (-not $proc.WaitForExit($TestTimeoutSeconds * 1000)) {
                $failed += "$($test.Name) (timeout)"
                $proc.Kill()
                $proc.WaitForExit()
            } elseif ($proc.ExitCode -ne 0) {
                $failed += "$($test.Name) (exit $($proc.ExitCode))"
            }
        } finally {
            $proc.Dispose()
        }
    }
    if ($failed.Count -gt 0) { throw "Test failures: $($failed -join ', ')" }
    Write-Host "Passed $($tests.Count) MSVC tests."
}

try {
    Push-Location $Root
    try {
        if ($Target -eq 'clean') {
            foreach ($path in @($ObjDir, $BinDir, $LibPath)) {
                if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Recurse -Force }
            }
            Write-Host "MSVC $Arch build outputs cleaned."
        } else {
            Initialize-MSVC
            if ($Target -eq 'verify') {
                Invoke-StaticChecks -RequireExes
            } else {
                Build-Library
                if ($Target -in @('tests', 'all', 'run-tests')) { Build-Programs 'tests' }
                if ($Target -in @('apps', 'all')) { Build-Programs 'apps' }
                if ($Target -eq 'run-tests') { Run-Tests }
                Invoke-StaticChecks
            }
        }
    } finally { Pop-Location }
} catch {
    [Console]::Error.WriteLine("MSVC build failed: $($_.Exception.Message)")
    exit 1
}

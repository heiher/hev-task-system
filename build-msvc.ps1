# Native MSVC x64 / experimental x86 build; independent from the MSYS2 Makefile.
# PowerShell 5.1+: powershell -NoProfile -ExecutionPolicy Bypass -File .\build-msvc.ps1 -Target all
# CMD shortcut: build-msvc.bat -Target all -Arch x86 -VSVersion 2022
# Targets: lib (default), tests, apps, all, run-tests, verify, clean.
# run-tests builds all supported tests, then runs them with a per-test timeout.
# verify re-runs the static binary checks on already-built outputs; for ARM64
# it validates them against a build provenance manifest and fails when the
# required artifacts are missing or stale relative to the assembly source.
param(
    [ValidateSet('lib', 'tests', 'apps', 'all', 'run-tests', 'verify', 'clean')]
    [string]$Target = 'lib',
    [ValidateSet('latest', '2026', '2022', '2019', '2017')]
    [string]$VSVersion = 'latest',
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Arch = 'x64',
    [ValidateRange(1, 3600)]
    [int]$TestTimeoutSeconds = 30
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
$MasmName = if ($Arch -eq 'x64') { 'ml64.exe' } elseif ($Arch -eq 'x86') { 'ml.exe' } else { 'armasm64.exe' }
$MachineType = if ($Arch -eq 'x64') { '8664' } elseif ($Arch -eq 'x86') { '14C' } else { 'AA64' }
# ARM64-only provenance tracking: the static checks must only certify
# artifacts that were produced from the current assembly source, so the build
# records SHA256 hashes of the source and every output it produced.
$AsmSourcePath = if ($Arch -eq 'arm64') { Join-Path $Root 'src\arch\arm\hev-task-execute-arm64-msvc.asm' } else { $null }
$AsmObjectPath = if ($Arch -eq 'arm64') { Join-Path $ObjDir 'hev-task-execute-arm64-msvc.asm.obj' } else { $null }
$ProvenancePath = Join-Path $BuildDir 'build-provenance.json'

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
    # armasm64 emits unwind data only if the source carries it. The context
    # switch object must keep its .pdata/.xdata records.
    $out = & dumpbin.exe /nologo /headers $ObjPath
    foreach ($section in @('.pdata', '.xdata')) {
        if (-not ($out | Where-Object { $_ -match "^\s*$section name" })) {
            throw "$ObjPath is missing the $section unwind section."
        }
    }
}

function Assert-Arm64UnwindInfo([string]$ExePath) {
    # The frame-chained hev_task_execute record must survive linking and must
    # decode against the real instructions without dumpbin mismatches. The
    # record is identified by its exact raw .xdata words
    # (header 08400008, epilog scope 00000005, codes 00E481E1); update this
    # fingerprint if the assembler source changes the unwind encoding.
    # Boundaries between table entries are the 'Begin Packed' lines.
    $lines = @(& dumpbin.exe /nologo /unwindinfo $ExePath)
    $starts = @()
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match 'stp   fp,lr,\[sp,#-0x10\]!') { $starts += $i }
    }
    if ($starts.Count -eq 0) {
        throw "No frame-chained hev_task_execute unwind entry found in $ExePath"
    }
    $found = $false
    foreach ($start in $starts) {
        $end = $start + 1
        while ($end -lt $lines.Count -and $lines[$end] -notmatch '^(Function Table|\s+[0-9A-Fa-f]{8} [0-9A-Fa-f]{8}\s)') { $end++ }
        $block = $lines[$start..($end - 1)]
        if ($block | Where-Object { $_ -match 'RawXdata=08400008\s+00000005\s+00E481E1' }) {
            $found = $true
            if (-not ($block | Where-Object { $_ -match 'mov   sp,fp' })) {
                throw "hev_task_execute unwind entry in $ExePath lost its epilogue."
            }
            if ($block | Where-Object { $_ -match '^\*\*\*\*' }) {
                throw "hev_task_execute unwind entry in $ExePath does not match the encoded instructions."
            }
        }
    }
    if (-not $found) {
        throw "hev_task_execute unwind record (RawXdata 08400008/00000005/00E481E1) not found in $ExePath"
    }
}

function Get-FileSha256([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-ProvenanceRelativePath([string]$Path) {
    $Path.Substring($Root.Length + 1)
}

function Write-Arm64Provenance {
    # Recorded after a successful ARM64 build. `verify` re-hashes the assembly
    # source and every listed artifact; any mismatch means the outputs on disk
    # were not produced from the current source and must not be certified.
    if (-not (Test-Path -LiteralPath $AsmObjectPath)) {
        throw 'The freshly assembled ARM64 context-switch object is missing.'
    }
    $entries = [ordered]@{}
    $entries[(Get-ProvenanceRelativePath $AsmSourcePath)] = Get-FileSha256 $AsmSourcePath
    $entries[(Get-ProvenanceRelativePath $AsmObjectPath)] = Get-FileSha256 $AsmObjectPath
    $entries[(Get-ProvenanceRelativePath $LibPath)] = Get-FileSha256 $LibPath
    foreach ($exe in @(Get-ChildItem -LiteralPath $BinDir -Filter '*.exe' -File -ErrorAction SilentlyContinue)) {
        $entries[(Get-ProvenanceRelativePath $exe.FullName)] = Get-FileSha256 $exe.FullName
    }
    [IO.File]::WriteAllText($ProvenancePath, ($entries | ConvertTo-Json))
}

function Assert-Arm64Provenance {
    if (-not (Test-Path -LiteralPath $ProvenancePath)) {
        throw "Missing ARM64 build provenance manifest ($ProvenancePath). " +
            'Run a build (-Target lib/tests/all/run-tests -Arch arm64) before verify.'
    }
    $manifest = [IO.File]::ReadAllText($ProvenancePath) | ConvertFrom-Json
    $asmRel = Get-ProvenanceRelativePath $AsmSourcePath
    foreach ($required in @($asmRel, (Get-ProvenanceRelativePath $AsmObjectPath), (Get-ProvenanceRelativePath $LibPath))) {
        if (-not $manifest.PSObject.Properties[$required]) {
            throw "The ARM64 provenance manifest does not record $required; rebuild with -Arch arm64."
        }
    }
    $recordedSource = $manifest.PSObject.Properties[$asmRel].Value
    if ((Get-FileSha256 $AsmSourcePath) -ne $recordedSource) {
        throw 'The ARM64 assembly source changed since the recorded build; ' +
            'rebuild with -Target lib/tests/all/run-tests -Arch arm64 before verify.'
    }
    foreach ($prop in $manifest.PSObject.Properties) {
        if ($prop.Name -eq $asmRel) { continue }
        $artifact = Join-Path $Root $prop.Name
        if (-not (Test-Path -LiteralPath $artifact)) {
            throw "Missing ARM64 build artifact recorded in the provenance manifest: $($prop.Name). Rebuild with -Arch arm64."
        }
        if ((Get-FileSha256 $artifact) -ne $prop.Value) {
            throw "ARM64 build artifact does not match the provenance manifest: $($prop.Name). Rebuild with -Arch arm64."
        }
    }
    Write-Host '  CHECK ARM64 build provenance (assembly source and artifacts match the manifest)'
}

function Invoke-StaticChecks([switch]$RequireExes) {
    $exes = @()
    if ($Arch -eq 'arm64') {
        # Only artifacts listed in, and hash-matched against, the provenance
        # manifest are certified; everything else is treated as stale.
        Assert-Arm64Provenance
        $manifest = [IO.File]::ReadAllText($ProvenancePath) | ConvertFrom-Json
        $exes = @($manifest.PSObject.Properties |
            Where-Object { $_.Name -like 'build-msvc*\bin\*.exe' } |
            ForEach-Object { Join-Path $Root $_.Name })
    } else {
        if (-not (Test-Path -LiteralPath $LibPath)) { throw 'Missing static library for verification.' }
        $exes = @(Get-ChildItem -LiteralPath $BinDir -Filter '*.exe' -File -ErrorAction SilentlyContinue |
            ForEach-Object { $_.FullName })
    }
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
        Write-Host '  CHECK unwind sections in the assembled hev-task-execute-arm64-msvc.asm.obj'
        if ($exes.Count -gt 0) {
            $probe = $exes | Where-Object { (Split-Path -Leaf $_) -eq 'tests-task.exe' } | Select-Object -First 1
            if (-not $probe) { $probe = $exes[0] }
            Assert-Arm64UnwindInfo $probe
            Write-Host "  CHECK unwind info for hev_task_execute in $(Split-Path -Leaf $probe)"
        } elseif ($RequireExes) {
            throw 'ARM64 verification requires at least one linked executable carrying the ' +
                'hev_task_execute unwind record; build tests or apps with -Arch arm64 first.'
        }
    }
}

function Initialize-MSVC {
    # A pre-configured matching-architecture Native Tools prompt is supported.
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    $masm = Get-Command $MasmName -ErrorAction SilentlyContinue
    $lib = Get-Command lib.exe -ErrorAction SilentlyContinue
    if ($VSVersion -eq 'latest' -and $cl -and $masm -and $lib -and
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
    foreach ($name in @('cl.exe', $MasmName, 'lib.exe', 'link.exe')) {
        if (-not (Get-Command $name -ErrorAction SilentlyContinue)) { throw "$name not found in MSVC environment." }
    }
}

function Assert-C11AtomicsSupport {
    # A toolset feature probe, not a Visual Studio product-version guess.
    # /std:c11 arrived in VS 2019 16.8, but /experimental:c11atomics
    # arrived in VS 2022 17.5. Both are required by the existing sources.
    $tempBase = Join-Path ([IO.Path]::GetTempPath()) ('hev-msvc-c11-' + [guid]::NewGuid().ToString('N'))
    $source = "$tempBase.c"
    $obj = "$tempBase.obj"
    try {
        Set-Content -LiteralPath $source -Encoding Ascii -Value @(
            '#include <stdatomic.h>',
            'int hev_probe(void) {',
            '    atomic_int value;',
            '    atomic_init(&value, 0);',
            '    return atomic_fetch_add_explicit(&value, 1, memory_order_relaxed);',
            '}'
        )
        & cl.exe /nologo /TC /std:c11 /experimental:c11atomics /c "/Fo$obj" $source
        if ($LASTEXITCODE -ne 0) {
            throw ('Selected MSVC toolset does not support required C11 atomics. ' +
                'Use Visual Studio 2022 17.5+ (or a newer compiler). ' +
                'VS 2017 and VS 2019 are NOT supported by this codebase without a separate atomic-compatibility port.')
        }
    } finally {
        foreach ($path in @($source, $obj)) {
            if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue }
        }
    }
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
    '/nologo', '/std:c11', '/experimental:c11atomics', '/O2', '/W3', '/MD',
    "/FI$Root\win32-compat\hev-win-compat.h"
) + $Includes + $CommonDefines
# Coroutine switching restores a saved stack directly and is incompatible
# with CET hardware shadow-stack instrumentation. lib.exe cannot set this;
# each final executable must explicitly opt out at link time. Consumers that
# link the .lib themselves MUST also use /CETCOMPAT:NO. ARM64 has no CET
# shadow stack, so the flag is applied only to the x64/x86 targets.
$ExeLinkFlags = @('/link')
if ($Arch -in @('x64', 'x86')) { $ExeLinkFlags += '/CETCOMPAT:NO' }
$ExeLinkFlags += @('Ws2_32.lib', 'Kernel32.lib')

function Build-Library {
    # A fresh object directory prevents an accidentally partial archive.
    if (Test-Path -LiteralPath $ObjDir) { Remove-Item -Recurse -Force -LiteralPath $ObjDir }
    if (Test-Path -LiteralPath $LibPath) { Remove-Item -Force -LiteralPath $LibPath }
    # Rebuilding the library invalidates every executable linked against the
    # previous one. For ARM64 they must be removed so the provenance manifest
    # never certifies stale executables that merely still exist on disk; the
    # manifest itself is dropped too, so verify fails until a fresh build
    # records artifacts produced from the current sources.
    if ($Arch -eq 'arm64') {
        Get-ChildItem -LiteralPath $BinDir -Filter '*.exe' -File -ErrorAction SilentlyContinue |
            Remove-Item -Force
        if (Test-Path -LiteralPath $ProvenancePath) {
            Remove-Item -Force -LiteralPath $ProvenancePath
        }
    }
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
        Invoke-Native 'cl.exe' ($Cflags + @('/c', "/Fo$obj", $src.FullName))
        $objects += $obj
    }

    if ($Arch -eq 'arm64') {
        $asmObj = $AsmObjectPath
        Write-Host '  ASM hev-task-execute-arm64-msvc.asm (armasm64 -machine ARM64)'
        # -machine ARM64 guarantees genuine native ARM64 COFF objects and
        # prevents armasm64 from emitting ARM64EC code.
        Invoke-Native 'armasm64.exe' (@('-machine', 'ARM64', '-nologo', $AsmSourcePath, $AsmObjectPath))
        Assert-Arm64AsmUnwindSections $AsmObjectPath
    } else {
        $asmName = if ($Arch -eq 'x64') { 'hev-task-execute-x86_64.asm' } else { 'hev-task-execute-x86.asm' }
        $asm = Join-Path $Root ("src\arch\x86\" + $asmName)
        $asmObj = Join-Path $ObjDir ($asmName + '.obj')
        Write-Host "  ASM $asmName"
        $asmFlags = if ($Arch -eq 'x86') { @('/nologo', '/c', '/coff', '/safeseh') } else { @('/nologo', '/c') }
        Invoke-Native $MasmName ($asmFlags + @("/Fo$asmObj", $asm))
    }
    $objects += $asmObj

    Write-Host '  LIB hev-task-system.lib'
    Invoke-Native 'lib.exe' (@('/nologo', "/OUT:$LibPath") + $objects)
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
        Write-Host "  LINK $Kind-$name.exe"
        & cl.exe @Cflags "/Fo$obj" "/Fe$exe" $src.FullName $LibPath @ExeLinkFlags
        if ($LASTEXITCODE -ne 0) { $failed += $src.Name }
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
            foreach ($path in @($ObjDir, $BinDir, $LibPath, $ProvenancePath, (Join-Path $BuildDir 'hev-task-execute-x86_64.obj'), (Join-Path $BuildDir 'hev-task-execute-x86.obj'))) {
                if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Recurse -Force }
            }
            Write-Host "MSVC $Arch build outputs cleaned."
        } else {
            Initialize-MSVC
            Assert-C11AtomicsSupport
            if ($Target -eq 'verify') {
                Invoke-StaticChecks -RequireExes
            } else {
                Build-Library
                if ($Target -in @('tests', 'all', 'run-tests')) { Build-Programs 'tests' }
                if ($Target -in @('apps', 'all')) { Build-Programs 'apps' }
                if ($Target -eq 'run-tests') { Run-Tests }
                if ($Arch -eq 'arm64') { Write-Arm64Provenance }
                Invoke-StaticChecks
            }
        }
    } finally { Pop-Location }
} catch {
    [Console]::Error.WriteLine("MSVC build failed: $($_.Exception.Message)")
    exit 1
}

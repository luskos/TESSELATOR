# MORT TESSELATOR - AUTOMATED EXECUTION SCRIPT
Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║           MORT TESSELATOR - LAUNCH SEQUENCE               ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Set paths
$MORT_ROOT = "C:\MORT"
$BUILD_DIR = "$MORT_ROOT\build"
$DATA_DIR = "$MORT_ROOT\data"
$LOG_DIR = "$MORT_ROOT\logs"
$TIMESTAMP = Get-Date -Format "yyyyMMdd_HHmmss"
$LOG_FILE = "$LOG_DIR\mort_run_$TIMESTAMP.log"

# Function to log output
function Log {
    param([string]$Message, [string]$Color = "White")
    $timestamp = Get-Date -Format "HH:mm:ss.fff"
    $logMessage = "[$timestamp] $Message"
    Write-Host $logMessage -ForegroundColor $Color
    Add-Content -Path $LOG_FILE -Value $logMessage
}

Log "MORT TESSELATOR v1.0 - Launch Sequence Initiated" "Green"
Log "Log file: $LOG_FILE"

# Step 1: Check for Visual Studio
Log "Step 1: Checking build environment..." "Yellow"
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
if ($vsPath) {
    Log "✓ Visual Studio found at: $vsPath" "Green"
    # Load Visual Studio environment
    cmd /c "call `"$vsPath\VC\Auxiliary\Build\vcvars64.bat`" && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2])
        }
    }
} else {
    Log "✗ Visual Studio not found! Please install Build Tools for Visual Studio." "Red"
    exit 1
}

# Step 2: Create data space if not exists
Log "Step 2: Checking MORT space..." "Yellow"
if (-not (Test-Path "$DATA_DIR\mort_space.bin")) {
    Log "MORT space not found. Generating 50GB space..." "Yellow"
    Set-Location $MORT_ROOT\src
    cl /EHsc mort_space_generator.cpp /Fe:mort_space_generator.exe
    if ($LASTEXITCODE -eq 0) {
        .\mort_space_generator.exe
        Log "✓ MORT space generated successfully" "Green"
    } else {
        Log "✗ Failed to generate MORT space" "Red"
    }
} else {
    $fileInfo = Get-Item "$DATA_DIR\mort_space.bin"
    Log "✓ MORT space exists: $($fileInfo.Length / 1GB) GB" "Green"
}

# Step 3: Compile the Tesselator Core
Log "Step 3: Compiling MORT Tesselator Core..." "Yellow"
Set-Location $BUILD_DIR

# Compile with maximum optimization
$compileCmd = @"
cl /EHsc /O2 /openmp /arch:AVX2 /fp:fast /GL /MT `
    /I"$MORT_ROOT\include" `
    $MORT_ROOT\src\mort_tesselator_core.cpp `
    /Fe:mort_tesselator.exe `
    /link /LTCG /OPT:REF /OPT:ICF
"@

Log "Compiling with: $compileCmd" "Gray"
$compileResult = Invoke-Expression $compileCmd

if ($LASTEXITCODE -eq 0) {
    Log "✓ Tesselator Core compiled successfully" "Green"
} else {
    Log "✗ Compilation failed. Error code: $LASTEXITCODE" "Red"
    Log "$compileResult" "Red"
    exit 1
}

# Step 4: Set process priority
Log "Step 4: Configuring system for maximum performance..." "Yellow"
# Set power scheme to high performance
powercfg /setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c
Log "✓ Power scheme set to High Performance" "Green"

# Step 5: Launch the Tesselator
Log "Step 5: Launching MORT Tesselator..." "Cyan"
Log "════════════════════════════════════════════════════════════" "Cyan"

# Start with high priority
Start-Process -FilePath "$BUILD_DIR\mort_tesselator.exe" `
    -WorkingDirectory $BUILD_DIR `
    -WindowStyle Normal `
    -RedirectStandardOutput "$LOG_DIR\output_$TIMESTAMP.log" `
    -RedirectStandardError "$LOG_DIR\error_$TIMESTAMP.log"

Log "✓ Tesselator launched. Monitoring output..." "Green"
Log "Output log: $LOG_DIR\output_$TIMESTAMP.log"
Log "Error log: $LOG_DIR\error_$TIMESTAMP.log"

# Step 6: Monitor and keep alive
Log "Step 6: Running keep-alive monitor..." "Yellow"

$monitor = {
    param($LOG_DIR, $BUILD_DIR)
    
    $running = $true
    $lastEnergy = 0
    $checkCount = 0
    
    while ($running) {
        $process = Get-Process -Name "mort_tesselator" -ErrorAction SilentlyContinue
        
        if ($process) {
            $cpu = $process.CPU
            $mem = $process.WorkingSet64 / 1MB
            $timestamp = Get-Date -Format "HH:mm:ss"
            
            # Read last few lines of output
            $outputFile = Get-ChildItem "$LOG_DIR\output_*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
            if ($outputFile) {
                $lastLine = Get-Content $outputFile.FullName -Tail 1
                
                # Try to extract energy reading
                if ($lastLine -match "Net: ([-+]?[0-9]*\.?[0-9]+)") {
                    $energy = [double]$matches[1]
                    Write-Host "[$timestamp] CPU: $($cpu.ToString('F1'))% | MEM: $($mem.ToString('F0'))MB | Net Energy: $energy J" -ForegroundColor Yellow
                    
                    # Check for net positive
                    if ($energy -gt 0.1 -and $lastEnergy -le 0.1) {
                        Write-Host "⚡⚡⚡ NET POSITIVE ENERGY DETECTED! ⚡⚡⚡" -ForegroundColor Magenta
                        Write-Host "System is now self-sustaining!" -ForegroundColor Green
                        
                        # Play notification
                        [System.Console]::Beep(1000, 200)
                        [System.Console]::Beep(1500, 200)
                        [System.Console]::Beep(2000, 400)
                    }
                    $lastEnergy = $energy
                }
            }
            
            $checkCount++
        } else {
            Write-Host "[$(Get-Date -Format 'HH:mm:ss')] Process terminated." -ForegroundColor Red
            $running = $false
        }
        
        Start-Sleep -Seconds 1
    }
}

# Start monitor in background
Start-Job -Name "MORTMonitor" -ScriptBlock $monitor -ArgumentList $LOG_DIR, $BUILD_DIR

Write-Host ""
Write-Host "Press any key to stop the Tesselator and cleanup..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

# Cleanup
Log "Step 7: Cleanup sequence initiated..." "Yellow"
Get-Job -Name "MORTMonitor" | Stop-Job | Remove-Job
Stop-Process -Name "mort_tesselator" -Force -ErrorAction SilentlyContinue
Log "✓ Cleanup complete" "Green"

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║           MORT TESSELATOR - SHUTDOWN COMPLETE              ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
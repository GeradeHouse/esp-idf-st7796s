# ---------------------------------------------------------------------------
# Script Name: CleanBuildEnvironment.ps1
# Description: Automates the cleanup of build directories for ESP-IDF projects.
# Author: GeradeHouse
# Date: 15-11-2024
# ---------------------------------------------------------------------------

# ------------------------------ Configuration -----------------------------

# Define the paths to the build and managed_components directories
$buildPath = "C:\Users\imede.IME-DEKKER\esp\esp-idf-st7796s\build"
$managedComponentsPath = "C:\Users\imede.IME-DEKKER\esp\esp-idf-st7796s\managed_components"



# Path to idf.py (ensure it's in the PATH or provide the full path)
$idfPyPath = "idf.py"  # Replace with full path if necessary, e.g., "C:\Path\To\idf.py"

# ---------------------------------------------------------------------------

# Global variable to track script success
$global:ScriptCompletedSuccessfully = $false

# Function to log messages with timestamp and color
function Write-Log {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true)]
        [string]$Message,

        [switch]$NoNewline,

        [System.ConsoleColor]$ForegroundColor = $null
    )
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    if ($NoNewline) {
        Write-Host "[$timestamp] $Message" -NoNewline -ForegroundColor $ForegroundColor
    }
    else {
        Write-Host "[$timestamp] $Message" -ForegroundColor $ForegroundColor
    }
}

# Function to take ownership and grant permissions to the current user
function Set-Permissions {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true)]
        [string]$Path
    )
    if (Test-Path $Path) {
        try {
            $CurrentUser = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
            Write-Log "Setting permissions for '$Path'..." -NoNewline -ForegroundColor Cyan
            takeown /F $Path /R /D Y | Out-Null
            icacls $Path /reset /T /C | Out-Null
            icacls $Path /grant "${CurrentUser}:(OI)(CI)F" /T /C | Out-Null
            Get-ChildItem -Path $Path -Recurse -Force | ForEach-Object {
                $_.Attributes = 'Normal'
            }
            Write-Host " ...permissions set successfully." -ForegroundColor Green
        }
        catch {
            Write-Log "ERROR: Could not set permissions for '$Path'." -ForegroundColor Red
            exit 1
        }
    }
    else {
        Write-Log "Path '$Path' does not exist. Skipping permission setting." -ForegroundColor Yellow
    }
}

# Function to run idf.py fullclean and handle specific output
function Invoke-IdfFullClean {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true)]
        [string]$IdfPyPath
    )
    $attempts = 0
    $maxAttempts = 2
    $success = $false
    do {
        try {
            $attempts++
            Write-Log "Running 'idf.py fullclean' (Attempt $attempts)..." -ForegroundColor Cyan
            $output = & $IdfPyPath fullclean 2>&1

            if ($output -match "PermissionError") {
                Write-Log "Permission issue detected. Adjusting permissions and retrying..." -ForegroundColor Yellow
                Set-Permissions -Path $buildPath
                Set-Permissions -Path $managedComponentsPath

                # Attempt to remove managed_components manually
                Write-Log "Removing 'managed_components' manually..." -ForegroundColor Cyan
                Remove-Item -Recurse -Force $managedComponentsPath -ErrorAction SilentlyContinue
            }
            elseif ($output -match "doesn't seem to be a CMake build directory") {
                Write-Log "Non-CMake build directory detected. Removing '$buildPath'..." -ForegroundColor Yellow
                Remove-Item -Recurse -Force $buildPath
            }
            else {
                # No errors, mark success and break the loop
                $success = $true
                break
            }
        }
        catch {
            Write-Log "ERROR: Failed to run 'idf.py fullclean'." -ForegroundColor Red
            exit 1
        }
    } while ($attempts -lt $maxAttempts)

    if ($success) {
        Write-Log "'idf.py fullclean' completed successfully." -ForegroundColor Green
    }
    else {
        Write-Log "ERROR: 'idf.py fullclean' failed after $maxAttempts attempts." -ForegroundColor Red
        exit 1
    }
}

# Function to perform final cleanup
function Remove-Directories {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$PathName
    )
    if (Test-Path $Path) {
        try {
            Remove-Item -Recurse -Force $Path -ErrorAction Stop
            Write-Log "'$PathName' directory '$Path' removed successfully." -ForegroundColor Green
        }
        catch {
            Write-Log "ERROR: Could not remove '$PathName' directory '$Path'." -ForegroundColor Red
            exit 1
        }
    }
    else {
        Write-Log "'$PathName' directory '$Path' does not exist. Skipping removal." -ForegroundColor Yellow
    }
}

# Function to display a countdown timer on the same line with keypress detection
function Show-Countdown {
    param(
        [int]$Seconds
    )

    try {
        # Hide the cursor for cleaner display
        [Console]::CursorVisible = $false

        for ($i = $Seconds; $i -gt 0; $i--) {
            # Overwrite the same line with the countdown
            Write-Host -NoNewline "`rClearing console in $i seconds... " -ForegroundColor Yellow

            $startTime = Get-Date

            while ((Get-Date) - $startTime -lt [TimeSpan]::FromSeconds(1)) {
                # Check for key press
                if ([Console]::KeyAvailable) {
                    $key = [Console]::ReadKey($true)
                    if ($key.KeyChar -eq 'k' -or $key.KeyChar -eq 'K') {
                        Write-Host "`nKeeping the output as per user request." -ForegroundColor Yellow
                        return $false  # Do not clear the console
                    } elseif ($key.Key -eq 'Enter' -or $key.Key -eq 'Spacebar') {
                        # Enter or Space pressed
                        return $true  # Clear the console immediately
                    }
                }
                Start-Sleep -Milliseconds 100  # Sleep in short intervals to stay responsive
            }
        }
    }
    finally {
        # Restore the cursor visibility
        [Console]::CursorVisible = $true
    }
    return $true  # Indicate that console should be cleared
}

# Function to wait or clear the console
function Wait-OrClear {
    param (
        [int]$Seconds = 10
    )

    Write-Host "`nThe console output will be cleared in $Seconds seconds. Press 'Enter' or 'Space' to clear immediately, or 'k' to keep the output." -ForegroundColor Yellow

    $shouldClear = Show-Countdown -Seconds $Seconds

    if ($shouldClear) {
        Clear-Host
        # Optionally, write a final message after clearing
        # Write-Host "Console cleared."
    }
    else {
        # Do not clear console
        return
    }
}

# ------------------------------ Main Script -------------------------------

# Check for Administrator privileges and prompt if not elevated
if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "This script requires administrative privileges. Attempting to restart as Administrator..." -ForegroundColor Yellow
    try {
        Start-Process powershell "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
        exit
    }
    catch {
        Write-Log "ERROR: Could not restart the script with administrative privileges." -ForegroundColor Red
        exit 1
    }
}

try {
    # Process the 'build' directory
    Set-Permissions -Path $buildPath

    # Process the 'managed_components' directory
    Set-Permissions -Path $managedComponentsPath

    # Run idf.py fullclean and handle specific output
    Invoke-IdfFullClean -IdfPyPath $idfPyPath

    # Final cleanup: Check if directories still exist and remove if necessary
    Remove-Directories -Path $buildPath -PathName "Build"
    Remove-Directories -Path $managedComponentsPath -PathName "Managed Components"

    # Mark script as completed successfully
    $global:ScriptCompletedSuccessfully = $true

    Write-Log "'idf.py fullclean' completed successfully. Build environment cleanup completed." -ForegroundColor Green
}
catch {
    # In case any unexpected error occurs
    Write-Log "ERROR: An unexpected error occurred: $_" -ForegroundColor Red
    exit 1
}

# If script completed successfully, prompt and clear console
if ($global:ScriptCompletedSuccessfully) {
    Wait-OrClear -Seconds 10
}

# ---------------------------------------------------------------------------

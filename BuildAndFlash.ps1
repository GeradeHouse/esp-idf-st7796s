
# ---------------------------------------------------------------------------
# Script Name: BuildAndFlash.ps1
# Description: This script builds, flashes, and monitors the ESP-IDF project without reconfiguring, with retry options.
# Author: GeradeHouse
# Date: 15-11-2024
# ---------------------------------------------------------------------------

# Function to display animated messages
function Show-AnimatedMessage {
    param(
        [string]$Message
    )

    $Cycles = 2
    $DelayMilliseconds = 125

    try {
        # Hide the cursor for a cleaner animation
        [Console]::CursorVisible = $false

        for ($j = 0; $j -lt $Cycles; $j++) {  # Number of animation cycles
            for ($dots = 1; $dots -le 3; $dots++) {  # Cycle through 1 to 3 dots
                $currentMessage = "$Message" + ("." * $dots)
                # Overwrite the same line using carriage return
                [Console]::Write("`r$currentMessage   ")
                Start-Sleep -Milliseconds $DelayMilliseconds
            }
        }
        # Clear the line after animation
        [Console]::Write("`r" + (" " * ($Message.Length + 3)))
        [Console]::Write("`r")  # Return cursor to the start of the line
    }
    finally {
        # Restore the cursor visibility
        [Console]::CursorVisible = $true
    }
}

# Function to execute a command and handle errors with Retry, Continue, or Quit options
function Execute-Command {
    param(
        [string]$Command,
        [string[]]$Arguments,
        [string]$ErrorMessage
    )

    $retry = $true
    while ($retry) {
        Clear-Host  # Clear the console
        Write-Host "===============================================================================" -ForegroundColor DarkMagenta
        Write-Host "                            Executing: $Command $($Arguments -join ' ')" -ForegroundColor DarkMagenta
        Write-Host "===============================================================================" -ForegroundColor DarkMagenta
        Write-Host  # This creates an empty line

        & $Command @Arguments
        $exitCode = $LASTEXITCODE

        if ($exitCode -eq 0) {
            Clear-Host
            break  # Command succeeded, exit the loop
        } else {
            Write-Host $ErrorMessage -ForegroundColor Red
            $validInput = $false

            while (-not $validInput) {
                $choice = Read-Host "Do you want to Retry (R), Continue (C), or Quit (Q)? [R/C/Q]"
                $choice = $choice.Trim().ToUpper()

                if ($choice -eq 'R') {
                    $validInput = $true
                    # Show "Retrying..." animation
                    Show-AnimatedMessage -Message "Retrying"
                    # Loop will continue, retrying the command
                } elseif ($choice -eq 'C') {
                    $validInput = $true
                    # Show "Continuing to the next step..." animation
                    Show-AnimatedMessage -Message "Continuing to the next step"
                    $retry = $false  # Do not retry, proceed to next command
                } elseif ($choice -eq 'Q' -or $choice -eq 'CLEAR') {
                    $validInput = $true
                    # Show "Exiting script..." animation
                    Show-AnimatedMessage -Message "Exiting script"
                    Clear-Host
                    exit $exitCode
                } else {
                    Write-Host "Invalid choice. Please enter R, C, or Q." -ForegroundColor Yellow
                }
            }
        }
    }
}

# Step 1: Build
Execute-Command -Command "idf.py" -Arguments @("build") -ErrorMessage "Error: 'idf.py build' failed."

# Step 2: Flash
Execute-Command -Command "idf.py" -Arguments @("-p", "COM5", "flash") -ErrorMessage "Error: 'idf.py flash' failed."

# Step 3: Monitor
Execute-Command -Command "idf.py" -Arguments @("-p", "COM5", "monitor") -ErrorMessage "Error: 'idf.py monitor' failed."
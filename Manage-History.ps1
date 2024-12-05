<#
.SYNOPSIS
   A script to manage PowerShell command history with a user-friendly menu.

.DESCRIPTION
   Provides options to view, edit, and delete command history, set history size, manage keybindings, and more.

.AUTHOR
   Assistant

#>

function Show-Menu {
    Clear-Host
    Write-Host "================= PowerShell History Manager =================" -ForegroundColor Magenta
    Write-Host "1. View and manage persistent history items"
    Write-Host "2. Clear the command history in the current session"
    Write-Host "3. Edit or delete the persistent command history"
    Write-Host "4. Limit the history size"
    Write-Host "5. Temporarily disable or enable history navigation"
    Write-Host "6. Manage custom keybindings for specific commands"
    Write-Host "7. Exit"
    Write-Host "=================================================================" -ForegroundColor Magenta
    Write-Host "Press 'Escape' at any time to go back or exit."
}

function Show-HistoryItems {
    $historyPath = "$env:APPDATA\Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt"
    if (-not (Test-Path $historyPath)) {
        Write-Host "Persistent history file not found."
        Read-Host "Press Enter to return to main menu..."
        return
    }

    # Read the history lines and reverse them to display the most recent items first
    $historyLines = [System.Collections.ArrayList](Get-Content $historyPath | Where-Object { $_ -ne '' })
    [array]::Reverse($historyLines)

    if ($historyLines.Count -eq 0) {
        Write-Host "No history items found."
        Read-Host "Press Enter to return to main menu..."
        return
    }

    $index = 0
    $pageSize = 30
    $totalItems = $historyLines.Count

    # Store initial window size
    $initialWindowSize = $Host.UI.RawUI.WindowSize

    while ($true) {
        # Display header and instructions once
        Clear-Host
        Write-Host "Viewing persistent history items ($($index + 1)-$([Math]::Min($index + $pageSize, $totalItems)) of $totalItems)" -ForegroundColor Cyan
        Write-Host "Use Up/Down arrows to navigate, 'Delete' to remove an item, 'N' for next page, 'P' for previous page, 'Escape' to return to main menu." -ForegroundColor White

        $pagedItems = $historyLines[$index..($([Math]::Min($index + $pageSize - 1, $totalItems - 1)))]
        $selectedIndex = 0
        $initialCursorTop = [System.Console]::CursorTop

        # Display items
        for ($i = 0; $i -lt $pagedItems.Count; $i++) {
            if ($i -eq $selectedIndex) {
                Write-Host "> $($index + $i + 1): $($pagedItems[$i])" -ForegroundColor Yellow
            } else {
                Write-Host "  $($index + $i + 1): $($pagedItems[$i])"
            }
        }

        $prevSelectedIndex = $selectedIndex

        while ($true) {
            # Check for window size change
            $currentWindowSize = $Host.UI.RawUI.WindowSize
            if ($currentWindowSize.Height -ne $initialWindowSize.Height -or $currentWindowSize.Width -ne $initialWindowSize.Width) {
                # Window size has changed, redraw the screen
                $initialWindowSize = $currentWindowSize
                break
            }

            $key = [System.Console]::ReadKey($true)
            $shouldBreak = $false
            $prevSelectedIndex = $selectedIndex

            switch ($key.Key) {
                'UpArrow' {
                    if ($selectedIndex -gt 0) {
                        $selectedIndex--
                    } else {
                        # At the top of the list, move to previous page if possible
                        if ($index - $pageSize -ge 0) {
                            $index -= $pageSize
                            $selectedIndex = $pageSize - 1
                            $shouldBreak = $true
                            break
                        }
                    }
                }
                'DownArrow' {
                    if ($selectedIndex -lt ($pagedItems.Count - 1)) {
                        $selectedIndex++
                    } else {
                        # At the bottom of the list, move to next page if possible
                        if ($index + $pageSize -lt $totalItems) {
                            $index += $pageSize
                            $selectedIndex = 0
                            $shouldBreak = $true
                            break
                        }
                    }
                }
                'Delete' {
                    $lineNumber = $index + $selectedIndex
                    $itemToRemove = $historyLines[$lineNumber]
                    $historyLines.RemoveAt($lineNumber) | Out-Null
                    # Save the updated history in reverse order to maintain most recent first
                    $historyToSave = @($historyLines)
                    [array]::Reverse($historyToSave)
                    Set-Content -Path $historyPath -Value $historyToSave
                    Write-Host "`nRemoved item $($lineNumber + 1): $itemToRemove" -ForegroundColor Green
                    # Immediate deletion without confirmation

                    # Update total items and adjust indices
                    $totalItems = $historyLines.Count
                    if ($totalItems -eq 0) {
                        Write-Host "No more history items."
                        Read-Host "Press Enter to return to main menu..."
                        return
                    }

                    # Adjust selectedIndex to stay on the same line if possible
                    if ($selectedIndex -ge $historyLines.Count - $index) {
                        $selectedIndex = [Math]::Max(0, $selectedIndex - 1)
                    }

                    # Refresh pagedItems
                    $pagedItems = $historyLines[$index..($([Math]::Min($index + $pageSize - 1, $totalItems - 1)))]

                    # Redraw the list without moving the cursor back to the first item
                    for ($i = 0; $i -lt $pagedItems.Count; $i++) {
                        [System.Console]::SetCursorPosition(0, $initialCursorTop + $i)
                        if ($i -eq $selectedIndex) {
                            Write-Host "> $($index + $i + 1): $($pagedItems[$i])" -ForegroundColor Yellow -NoNewline
                        } else {
                            Write-Host "  $($index + $i + 1): $($pagedItems[$i])" -NoNewline
                        }
                        # Clear to end of line
                        [System.Console]::Write(' ' * ([System.Console]::BufferWidth - [System.Console]::CursorLeft))
                    }
                    # Clear any remaining lines if the list has shrunk
                    if ($pagedItems.Count -lt $pageSize) {
                        for ($i = $pagedItems.Count; $i -lt $pageSize; $i++) {
                            [System.Console]::SetCursorPosition(0, $initialCursorTop + $i)
                            [System.Console]::Write(' ' * [System.Console]::BufferWidth)
                        }
                    }
                    continue
                }
                'N' {
                    if ($index + $pageSize -lt $totalItems) {
                        $index += $pageSize
                        $selectedIndex = 0
                        $shouldBreak = $true
                        break
                    } else {
                        Write-Host "`nNo more pages." -ForegroundColor Red
                        Read-Host "Press Enter to continue..."
                    }
                }
                'P' {
                    if ($index -ge $pageSize) {
                        $index -= $pageSize
                        $selectedIndex = 0
                        $shouldBreak = $true
                        break
                    } else {
                        Write-Host "`nAlready at the first page." -ForegroundColor Red
                        Read-Host "Press Enter to continue..."
                    }
                }
                'Escape' {
                    return
                }
                default { }
            }

            if ($shouldBreak) {
                break
            }

            if ($selectedIndex -ne $prevSelectedIndex) {
                # Un-highlight previous selection
                [System.Console]::SetCursorPosition(0, $initialCursorTop + $prevSelectedIndex)
                Write-Host "  $($index + $prevSelectedIndex + 1): $($pagedItems[$prevSelectedIndex])" -NoNewline
                # Clear to end of line
                [System.Console]::Write(' ' * ([System.Console]::BufferWidth - [System.Console]::CursorLeft))

                # Highlight current selection
                [System.Console]::SetCursorPosition(0, $initialCursorTop + $selectedIndex)
                Write-Host "> $($index + $selectedIndex + 1): $($pagedItems[$selectedIndex])" -ForegroundColor Yellow -NoNewline
                [System.Console]::Write(' ' * ([System.Console]::BufferWidth - [System.Console]::CursorLeft))
            }
        }
    }
}

function Clear-Current-Session-History {
    Clear-History
    Write-Host "Command history in the current session has been cleared." -ForegroundColor Green
    Read-Host "Press Enter to continue..."
}

function Invoke-PersistentHistoryEdit {
    $historyPath = "$env:APPDATA\Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt"
    if (-not (Test-Path $historyPath)) {
        Write-Host "Persistent history file not found." -ForegroundColor Red
    } else {
        Write-Host "Opening persistent history file in Notepad..." -ForegroundColor Green
        Start-Process notepad.exe $historyPath
    }
    Read-Host "Press Enter to continue..."
}

function Remove-Persistent-History {
    $historyPath = "$env:APPDATA\Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt"
    if (-not (Test-Path $historyPath)) {
        Write-Host "Persistent history file not found." -ForegroundColor Red
    } else {
        Remove-Item $historyPath
        Write-Host "Persistent history file has been deleted." -ForegroundColor Green
    }
    Read-Host "Press Enter to continue..."
}

function Set-HistorySize {
    $currentSize = (Get-PSReadLineOption).MaximumHistoryCount
    Write-Host "Current maximum history size: $currentSize" -ForegroundColor Cyan
    Write-Host "Enter new maximum history size:"
    $newSize = Read-Host
    if ($newSize -match '^\d+$') {
        Set-PSReadLineOption -MaximumHistoryCount [int]$newSize
        Write-Host "Maximum history size has been set to $newSize." -ForegroundColor Green
    } else {
        Write-Host "Invalid input. Please enter a numeric value." -ForegroundColor Red
    }
    Read-Host "Press Enter to continue..."
}

function Disable-History-Navigation {
    Remove-PSReadLineKeyHandler -Key UpArrow -ErrorAction SilentlyContinue
    Remove-PSReadLineKeyHandler -Key DownArrow -ErrorAction SilentlyContinue
    Write-Host "History navigation has been disabled for this session." -ForegroundColor Green
    Read-Host "Press Enter to continue..."
}

function Enable-History-Navigation {
    Set-PSReadLineKeyHandler -Key UpArrow -Function HistorySearchBackward
    Set-PSReadLineKeyHandler -Key DownArrow -Function HistorySearchForward
    Write-Host "History navigation has been enabled." -ForegroundColor Green
    Read-Host "Press Enter to continue..."
}

function Set-Keybindings {
    while ($true) {
        Clear-Host
        Write-Host "=========== Manage Custom Keybindings ===========" -ForegroundColor Magenta
        Write-Host "1. Create a new keybinding"
        Write-Host "2. View existing keybindings"
        Write-Host "3. Remove a keybinding"
        Write-Host "4. Save keybindings to profile (make persistent)"
        Write-Host "5. Return to main menu"
        Write-Host "=================================================" -ForegroundColor Magenta
        Write-Host "Press 'Escape' to return to main menu."
        Write-Host "Please select an option (1-5): "
        $choiceKey = [System.Console]::ReadKey($true)
        if ($choiceKey.Key -eq 'Escape') {
            return
        } else {
            $choice = $choiceKey.KeyChar
        }

        switch ($choice) {
            '1' {
                Write-Host "`nEnter the key chord (e.g., Ctrl+Shift+1):"
                $chord = Read-Host
                Write-Host "Enter the command to execute:"
                $command = Read-Host

                $scriptBlock = [ScriptBlock]::Create($command)
                try {
                    Set-PSReadLineKeyHandler -Chord $chord -ScriptBlock $scriptBlock
                    Write-Host "Keybinding for '$chord' has been set to execute: $command" -ForegroundColor Green
                } catch {
                    Write-Host "Failed to set keybinding. Please ensure the chord is valid." -ForegroundColor Red
                }
                Read-Host "Press Enter to continue..."
            }
            '2' {
                $keyHandlers = (Get-PSReadLineKeyHandler).Where({ $_.Key -ne '' })
                if ($keyHandlers.Count -eq 0) {
                    Write-Host "`nNo custom keybindings found." -ForegroundColor Yellow
                } else {
                    Write-Host "`nCustom keybindings:" -ForegroundColor Cyan
                    $keyHandlers | ForEach-Object {
                        Write-Host "Chord: $($_.Key), Function: $($_.BriefDescription)"
                    }
                }
                Read-Host "Press Enter to continue..."
            }
            '3' {
                Write-Host "`nEnter the key chord to remove (e.g., Ctrl+Shift+1):"
                $chord = Read-Host
                try {
                    Remove-PSReadLineKeyHandler -Chord $chord
                    Write-Host "Keybinding for '$chord' has been removed." -ForegroundColor Green
                } catch {
                    Write-Host "Failed to remove keybinding. Please ensure the chord is valid." -ForegroundColor Red
                }
                Read-Host "Press Enter to continue..."
            }
            '4' {
                Export-KeybindingsToProfile
            }
            '5' {
                return
            }
            default {
                Write-Host "`nInvalid selection. Please try again." -ForegroundColor Red
                Read-Host "Press Enter to continue..."
            }
        }
    }
}

function Export-KeybindingsToProfile {
    $profilePath = $PROFILE
    if (-not (Test-Path -Path $profilePath)) {
        # Create the profile file if it doesn't exist
        New-Item -ItemType File -Path $profilePath -Force | Out-Null
    }

    # Read existing profile content
    $profileContent = Get-Content -Path $profilePath -Raw

    # Remove existing custom keybindings from profile (if any)
    $profileContent = $profileContent -replace '(# Begin Custom Keybindings.*# End Custom Keybindings)', ''

    # Get current custom keybindings
    $keyHandlers = (Get-PSReadLineKeyHandler).Where({ $_.Key -ne '' })
    if ($keyHandlers.Count -eq 0) {
        Write-Host "No custom keybindings to save." -ForegroundColor Yellow
        Read-Host "Press Enter to continue..."
        return
    }

    # Build the keybindings script block
    $keybindingsScript = "`n# Begin Custom Keybindings`n"
    foreach ($handler in $keyHandlers) {
        $chord = $handler.Key
        $function = $handler.ScriptBlock.ToString()
        $keybindingsScript += "Set-PSReadLineKeyHandler -Chord '$chord' -ScriptBlock $function`n"
    }
    $keybindingsScript += "# End Custom Keybindings`n"

    # Save the updated profile content
    Set-Content -Path $profilePath -Value $profileContent
    Add-Content -Path $profilePath -Value $keybindingsScript

    Write-Host "Custom keybindings have been saved to your PowerShell profile." -ForegroundColor Green
    Read-Host "Press Enter to continue..."
}

function Main {
    while ($true) {
        Show-Menu
        Write-Host "Please select an option (1-7): " -ForegroundColor White
        $choiceKey = [System.Console]::ReadKey($true)
        if ($choiceKey.Key -eq 'Escape') {
            break
        } else {
            $choice = $choiceKey.KeyChar
        }

        switch ($choice) {
            '1' {
                Clear-Host
                Show-HistoryItems
            }
            '2' {
                Clear-Host
                Clear-Current-Session-History
            }
            '3' {
                Clear-Host
                Write-Host "1. Edit the persistent command history"
                Write-Host "2. Remove the persistent command history"
                Write-Host "Press 'Escape' to return to main menu."
                $subChoiceKey = [System.Console]::ReadKey($true)
                if ($subChoiceKey.Key -eq 'Escape') {
                    continue
                } else {
                    $subChoice = $subChoiceKey.KeyChar
                }

                switch ($subChoice) {
                    '1' { Invoke-PersistentHistoryEdit }
                    '2' { Remove-Persistent-History }
                    default {
                        Write-Host "`nInvalid selection. Returning to main menu." -ForegroundColor Red
                        Read-Host "Press Enter to continue..."
                    }
                }
            }
            '4' {
                Clear-Host
                Set-HistorySize
            }
            '5' {
                Clear-Host
                Write-Host "1. Disable history navigation"
                Write-Host "2. Enable history navigation"
                Write-Host "Press 'Escape' to return to main menu."
                $subChoiceKey = [System.Console]::ReadKey($true)
                if ($subChoiceKey.Key -eq 'Escape') {
                    continue
                } else {
                    $subChoice = $subChoiceKey.KeyChar
                }

                switch ($subChoice) {
                    '1' { Disable-History-Navigation }
                    '2' { Enable-History-Navigation }
                    default {
                        Write-Host "`nInvalid selection. Returning to main menu." -ForegroundColor Red
                        Read-Host "Press Enter to continue..."
                    }
                }
            }
            '6' {
                Set-Keybindings
            }
            '7' {
                break
            }
            default {
                Write-Host "`nInvalid selection. Please try again." -ForegroundColor Red
                Read-Host "Press Enter to continue..."
            }
        }
    }
    # Clear the terminal when exiting
    Clear-Host
}

# Run the main function
Main

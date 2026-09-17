<#
Send commands to the robot state controller over the USB serial port.

    .\tools\robot.ps1 PING
    .\tools\robot.ps1 GET_STATE
    .\tools\robot.ps1 SET_STATE,SCAN_QR
    .\tools\robot.ps1 SET_BRIGHTNESS,40
    .\tools\robot.ps1 -Port COM5 SET_STATE,FAULT

Without a command it just listens, printing whatever the board sends
(button events, log lines) until you press Ctrl+C:

    .\tools\robot.ps1 -Listen

Nothing else may hold the port at the same time - close Tera Term and
idf.py monitor first.
#>
[CmdletBinding()]
param(
    # PowerShell splits an unquoted SET_STATE,FAULT on the comma, so take an
    # array and put it back together. Quoting the command also works.
    [Parameter(Position = 0)]
    [string[]]$Command,

    [string]$Port = "COM8",

    [switch]$Listen,

    # Milliseconds to wait for a reply before giving up.
    [int]$Wait = 900
)

$ErrorActionPreference = "Stop"

$sp = New-Object System.IO.Ports.SerialPort($Port, 115200, 'None', 8, 'One')
$sp.ReadTimeout = 1000
$sp.WriteTimeout = 2000
# Leave RTS clear: asserting both RTS and DTR resets the C3 into the
# ROM bootloader.
$sp.DtrEnable = $true
$sp.RtsEnable = $false

try {
    $sp.Open()
}
catch {
    Write-Host "Cannot open $Port : $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Ports available: $([System.IO.Ports.SerialPort]::GetPortNames() -join ', ')"
    Write-Host "If it says 'access denied', close Tera Term or idf.py monitor."
    exit 1
}

try {
    Start-Sleep -Milliseconds 250

    if ($Listen) {
        Write-Host "Listening on $Port, Ctrl+C to stop." -ForegroundColor Cyan
        while ($true) {
            $data = $sp.ReadExisting()
            if ($data) { Write-Host -NoNewline $data }
            Start-Sleep -Milliseconds 100
        }
    }

    $line = $Command -join ','

    if (-not $line) {
        Write-Host "Nothing to send. Pass a command, or -Listen." -ForegroundColor Yellow
        exit 1
    }

    $sp.DiscardInBuffer()
    # The firmware accepts CR, LF or CRLF as end of line.
    $sp.Write("$line`r")

    Start-Sleep -Milliseconds $Wait
    $reply = $sp.ReadExisting()

    if ($reply) {
        Write-Host "> $line" -ForegroundColor DarkGray
        Write-Host $reply.TrimEnd()
    }
    else {
        Write-Host "> $line" -ForegroundColor DarkGray
        Write-Host "(no reply)" -ForegroundColor Yellow
    }
}
finally {
    if ($sp.IsOpen) { $sp.Close() }
}

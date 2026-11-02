<#
Walk every state and every brightness level over the serial link and check
what the firmware answers.

    .\tools\state_test.ps1
    .\tools\state_test.ps1 -Port COM5
    .\tools\state_test.ps1 -Hold 2000      # linger on each state to look at it

For every state it sends SET_STATE and then GET_STATE, and fails the state if
the reply is not ACK,SET_STATE followed by STATE,<the same name>. It then
sweeps brightness 20 / 80 / 200 and finally checks that a made-up state name
is still rejected with NACK,BAD_STATE.

This only proves the protocol side. Whether the pictures are readable, and
whether LOAD_BEER and UNLOAD_BEER really cannot be confused, has to be judged
by eye - use -Hold and watch the panel.

Nothing else may hold the port at the same time - close Tera Term, the GUI
and idf.py monitor first.
#>
[CmdletBinding()]
param(
    [string]$Port = "COM8",

    # Milliseconds to wait for a reply.
    [int]$Wait = 400,

    # Milliseconds to leave each state on the panel before moving on.
    [int]$Hold = 0
)

$ErrorActionPreference = "Stop"

# Mission order, so watching the run looks like watching a delivery.
$States = @(
    "WAITING"
    "SCAN_QR"
    "QR_CONFIRM"
    "ACCEPTED"
    "REJECTED"
    "AUTONOMOUS_DRIVING"
    "LOAD_BEER"
    "UNLOAD_BEER"
    "RETURNING"
    "MISSION_COMPLETE"
    "NO_SIGNAL"
    "FAULT"
)

$Brightness = @(20, 80, 200)

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
    Write-Host "If it says 'access denied', close Tera Term, the GUI or idf.py monitor."
    exit 1
}

$failures = 0

function Send-Line([string]$line) {
    $sp.DiscardInBuffer()
    # The firmware accepts CR, LF or CRLF as end of line.
    $sp.Write("$line`r")
    Start-Sleep -Milliseconds $Wait
    return $sp.ReadExisting()
}

# The board also logs to this port, so pick the protocol line out of the
# noise rather than assuming the reply is alone.
function Get-Reply([string]$text, [string]$prefix) {
    foreach ($l in ($text -split "`n")) {
        $l = $l.Trim()
        if ($l.StartsWith($prefix)) { return $l }
    }
    return ""
}

function Check([string]$what, [string]$got, [string]$expected) {
    if ($got -eq $expected) {
        Write-Host ("  ok   {0,-22} {1}" -f $what, $got) -ForegroundColor DarkGray
        return $true
    }
    Write-Host ("  FAIL {0,-22} expected '{1}', got '{2}'" -f $what, $expected, $got) -ForegroundColor Red
    $script:failures++
    return $false
}

try {
    Start-Sleep -Milliseconds 250

    $reply = Send-Line "PING"
    Check "PING" (Get-Reply $reply "PONG") "PONG" | Out-Null

    Write-Host "`nStates" -ForegroundColor Cyan
    foreach ($state in $States) {
        Write-Host $state -ForegroundColor White

        $reply = Send-Line "SET_STATE,$state"
        Check "SET_STATE" (Get-Reply $reply "ACK") "ACK,SET_STATE" | Out-Null

        $reply = Send-Line "GET_STATE"
        Check "GET_STATE" (Get-Reply $reply "STATE,") "STATE,$state" | Out-Null

        if ($Hold -gt 0) { Start-Sleep -Milliseconds $Hold }
    }

    Write-Host "`nBrightness" -ForegroundColor Cyan
    # Left on the last state above, which is FAULT; step through the levels so
    # a pattern can be checked at each one.
    foreach ($b in $Brightness) {
        $reply = Send-Line "SET_BRIGHTNESS,$b"
        Check "SET_BRIGHTNESS,$b" (Get-Reply $reply "ACK") "ACK,SET_BRIGHTNESS" | Out-Null
        if ($Hold -gt 0) { Start-Sleep -Milliseconds $Hold }
    }

    Write-Host "`nRejections" -ForegroundColor Cyan
    $reply = Send-Line "SET_STATE,NOT_A_STATE"
    Check "unknown state" (Get-Reply $reply "NACK") "NACK,BAD_STATE" | Out-Null

    $reply = Send-Line "SET_BRIGHTNESS,999"
    Check "brightness 999" (Get-Reply $reply "NACK") "NACK,BAD_ARGUMENT" | Out-Null

    $reply = Send-Line "NOT_A_COMMAND"
    Check "unknown command" (Get-Reply $reply "NACK") "NACK,UNKNOWN_COMMAND" | Out-Null

    # Back to something harmless, at the normal level.
    Send-Line "SET_BRIGHTNESS,80" | Out-Null
    Send-Line "SET_STATE,WAITING" | Out-Null

    Write-Host ""
    if ($failures -eq 0) {
        Write-Host "All checks passed." -ForegroundColor Green
    }
    else {
        Write-Host "$failures check(s) failed." -ForegroundColor Red
    }
}
finally {
    if ($sp.IsOpen) { $sp.Close() }
}

exit ([int]($failures -gt 0))

#
# Write-CvarDocJson
#
# Generates the cvar documentation JSON that ships alongside the binaries.
#
function Write-CvarDocJson
{
    param(
        # Path to odamex.exe or odasrv.exe.
        [Parameter(Mandatory = $true)][string]$ExePath
    )

    $Exe = Get-Item -Path $ExePath
    $JsonPath = Join-Path $Exe.DirectoryName "$($Exe.BaseName)_cvardoc.json"

    # Remove any document left over from an earlier run, so a silent failure
    # below cannot pass a stale file off as a fresh one.
    if (Test-Path $JsonPath)
    {
        Remove-Item -Force -Path $JsonPath
    }

    try
    {
        $Proc = Start-Process -FilePath $Exe.FullName -ArgumentList "-cvardocjson" `
            -NoNewWindow -PassThru -ErrorAction Stop

        $Proc | Wait-Process -Timeout 30 -ErrorAction Stop
    }
    catch [System.Management.Automation.TimeoutException]
    {
        Write-Error "$($Exe.Name) timed out! Force closing it now..."
        Stop-Process -Id $Proc.Id -Force
    }
    catch
    {
        Write-Error "An unexpected error occurred: $_"
    }


    if (!$Proc.HasExited)
    {
        throw "$($Exe.Name) has not exited yet. No cvardocjson file will be written."
    }

    if ($Proc.ExitCode -ne 0)
    {
        throw "$($Exe.Name) -cvardocjson exited with $($Proc.ExitCode)"
    }

    if (-not (Test-Path $JsonPath))
    {
        throw "$($Exe.Name) -cvardocjson did not write $JsonPath"
    }

    return $JsonPath
}

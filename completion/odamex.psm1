#-----------------------------------------------------------------------------
#
# Copyright (C) 2006-2026 by The Odamex Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#-----------------------------------------------------------------------------

function Get-OdamexInstallDirFromRegistry {
    $keys = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{2E517BBB-916F-4AB6-80E0-D4A292513F7A}_is1',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{2E517BBB-916F-4AB6-80E0-D4A292513F7A}_is1'
    )

    foreach ($key in $keys) {
        if (Test-Path $key) {
            $props = Get-ItemProperty $key
            return $props.InstallLocation
        }
    }

    return $null
}

$script:InstallDir = Get-OdamexInstallDirFromRegistry
$script:OdamexExe = Join-Path $script:InstallDir 'odamex.exe'
$script:OdasrvExe = Join-Path $script:InstallDir 'odasrv.exe'

New-Alias odamex $script:OdamexExe
New-Alias odasrv $script:OdasrvExe

Export-ModuleMember -Alias odamex, odasrv

enum OdamexOptionScope {
    Client
    Server
    ClientAndServer
}

enum OdamexOptionGroup {
    None
    Timer
    Demo
    Map
}

class OdamexOption {
    [string]$Name
    [OdamexOptionScope]$Scope
    [OdamexOptionGroup]$Group
    [string]$FileType
    [bool]$NoComplete
    [bool]$Repeatable
    [bool]$Exclusive

    OdamexOption([string]$name, [OdamexOptionScope]$scope) {
        $this.Name = $name
        $this.Scope = $scope
        $this.Group = [OdamexOptionGroup]::None
        $this.FileType = $null
        $this.NoComplete = $false
        $this.Repeatable = $false
        $this.Exclusive = $false
    }
}

function New-OdamexOption {
    param(
        [OdamexOptionScope]$Scope,
        [string]$Name,
        [OdamexOptionGroup]$Group = [OdamexOptionGroup]::None,
        [string]$FileType = $null,
        [switch]$NoComplete,
        [switch]$Repeatable,
        [switch]$Exclusive
    )

    $opt = [OdamexOption]::new($Name, $Scope)
    $opt.Group = $Group
    $opt.FileType = $FileType
    $opt.NoComplete = $NoComplete.IsPresent
    $opt.Repeatable = $Repeatable.IsPresent
    $opt.Exclusive = $Exclusive.IsPresent

    return $opt
}

$script:Options = @(
    # exclusive options, i.e. can't be used with any others
    New-OdamexOption ClientAndServer "--version" -Exclusive
    New-OdamexOption ClientAndServer "--cvardoc" -Exclusive
    New-OdamexOption ClientAndServer "--cvardocjson" -Exclusive
    # simple flags
    New-OdamexOption Client "-nomouse"
    New-OdamexOption Client "-nosound"
    New-OdamexOption Client "-nomusic"
    New-OdamexOption Client "-novideo"
    New-OdamexOption Client "-nodraw"
    New-OdamexOption Client "-noblit"
    New-OdamexOption Client "-pistolstart"
    New-OdamexOption Client "-coop-things"
    New-OdamexOption Client "-shorttics"
    New-OdamexOption Server "-fork"
    New-OdamexOption ClientAndServer "-devparm"
    New-OdamexOption ClientAndServer "-stepmode"
    New-OdamexOption ClientAndServer "-blockmap"
    New-OdamexOption ClientAndServer "-noflathack"
    New-OdamexOption ClientAndServer "-nomonsters"
    New-OdamexOption ClientAndServer "-respawn"
    New-OdamexOption ClientAndServer "-fast"
    # timer related options
    New-OdamexOption Server "-avg"   -Group Timer
    New-OdamexOption Server "-timer" -Group Timer -NoComplete
    # map changing options
    New-OdamexOption ClientAndServer "-map"  -Group Map -NoComplete
    New-OdamexOption ClientAndServer "+map"  -Group Map -NoComplete
    New-OdamexOption ClientAndServer "-warp" -Group Map -NoComplete
    # demo related options
    New-OdamexOption Client "-netrecord" -Group Demo
    New-OdamexOption Client "-netplay"   -Group Demo -FileType ".odd"
    New-OdamexOption Client "-playdemo"  -Group Demo -FileType ".lmp"
    New-OdamexOption Client "+playdemo"  -Group Demo -FileType ".lmp"
    New-OdamexOption Client "+demotest"  -Group Demo -FileType ".lmp"
    New-OdamexOption Client "-timedemo"  -Group Demo -FileType ".lmp"
    # other options that take a filename parameter
    New-OdamexOption ClientAndServer "-wad" -FileType ".wad"
    New-OdamexOption ClientAndServer "-iwad" -FileType ".wad"
    New-OdamexOption ClientAndServer "-file" -FileType ".wad" -Repeatable
    New-OdamexOption ClientAndServer "-deh" -FileType ".deh" -Repeatable
    New-OdamexOption ClientAndServer "-bex" -FileType ".bex" -Repeatable
    New-OdamexOption ClientAndServer "-config" -FileType ".cfg"
    New-OdamexOption ClientAndServer "+logfile" -FileType ".log"
    # TODO: add support for ; separated directory list
    # New-OdamexOption ClientAndServer "-waddir" -FileType "directorylist"
    New-OdamexOption ClientAndServer "-waddir" -FileType "directory"
    New-OdamexOption ClientAndServer "-cfgdir" -FileType "directory"
    New-OdamexOption ClientAndServer "-crashdir" -FileType "directory"
    New-OdamexOption ClientAndServer "-confile" -FileType "any"
    # everything else
    New-OdamexOption ClientAndServer "-skill" -NoComplete
    New-OdamexOption Server "-port" -NoComplete
    New-OdamexOption Server "-maxclients" -NoComplete
    New-OdamexOption Client "-connect" -NoComplete
    New-OdamexOption Client "-numparticles" -NoComplete
    New-OdamexOption Client "-fltk" -NoComplete
)

$script:OptionsMap = @{}

$script:Options | ForEach-Object {
    $script:OptionsMap[$_.Name] = $_
}

function Get-OdamexActiveState {
    param([string[]]$Words, [string]$CurrentWord)

    $used = @{}
    $usedGroups = @{}
    $exclusiveSeen = $false

    foreach ($w in $Words) {
        if ($w -notmatch '^-|\+') { continue }

        $opt = $script:OptionsMap[$w]
        if (-not $opt) { continue }

        if ($opt.Exclusive) {
            $exclusiveSeen = $true
        }

        $used[$w] = $true

        if ($opt.Group -ne [OdamexOptionGroup]::None) {
            $usedGroups[$opt.Group] = $true
        }
    }

    return [pscustomobject]@{
        Used = $used
        Groups = $usedGroups
        ExclusiveSeen = $exclusiveSeen
    }
}

$script:OdamexCompleter = {
    param($wordToComplete, $commandAst, $cursorPosition)

    $words = $commandAst.CommandElements.ForEach({ $_.Extent.Text })

    if (-not $words -or $words.Count -lt 1) { return "" }

    $state = Get-OdamexActiveState -Words $words

    $prev = if ($words.Count -gt 1 -and $wordToComplete -and $wordToComplete -eq $words[-1]) { $words[-2] } else { $words[-1] }

    if ($state.ExclusiveSeen) {
        return ""
    }

    $prevOpt = $script:OptionsMap[$prev]
    if ($prevOpt) {
        if ($prevOpt.FileType) {
            switch ($prevOpt.FileType) {
                "directory" {
                    return Get-ChildItem -Directory -ErrorAction SilentlyContinue |
                        Where-Object Name -like "$wordToComplete*" |
                        ForEach-Object {
                            $completionText = $_.Name
                            if (-not ($completionText -match '^[a-zA-Z]:\\|^/|^\\.\\')) {
                                $completionText = ".\$completionText"
                            }
                            [System.Management.Automation.CompletionResult]::new(
                                $completionText,
                                $_.Name,
                                "ProviderContainer",
                                $_.FullName
                            )
                        }
                }
                "any" {
                    return [System.Management.Automation.CompletionCompleters]::CompleteFilename($wordToComplete)
                }
                default {
                    $results = Get-ChildItem -File -Filter "*$($prevOpt.FileType)" -ErrorAction SilentlyContinue

                    if (-not $results)
                    {
                        $results = Get-ChildItem -Directory -ErrorAction SilentlyContinue
                    }

                    return $results | Where-Object Name -like "$wordToComplete*" |
                        ForEach-Object {
                            $type = if ($_.PSIsContainer) {
                                    "ProviderContainer"
                                } else {
                                    "ProviderItem"
                                }
                            $completionText = $_.Name
                            if (-not ($completionText -match '^[a-zA-Z]:\\|^/|^\\.\\')) {
                                $completionText = ".\$completionText"
                            }
                            [System.Management.Automation.CompletionResult]::new(
                                $completionText,
                                $_.Name,
                                $type,
                                $_.FullName
                            )
                        }
                }
            }
        }
        if ($prevOpt.NoComplete) {
            return ""
        }
    }

    $scope = switch ($commandAst.GetCommandName()) {
        "odamex" { @([OdamexOptionScope]::Client, [OdamexOptionScope]::ClientAndServer) }
        "odasrv" { @([OdamexOptionScope]::Server, [OdamexOptionScope]::ClientAndServer) }
        default { return "" }
    }

    $candidates = foreach ($opt in $script:Options) {
        if ($opt.Scope -notin $scope) { continue }

        if ($opt.Group -ne [OdamexOptionGroup]::None -and
            $state.Groups.ContainsKey($opt.Group)) {
            continue
        }

        if ($opt.Repeatable -eq $false -and $state.Used.ContainsKey($opt.Name)) {
            continue
        }

        $opt.Name
    }

    $matches = $candidates | Where-Object { $_ -like "$wordToComplete*" }

    if (-not $matches) {
        return ""
    }

    return $matches | Sort-Object
}

Register-ArgumentCompleter -CommandName odamex -Native -ScriptBlock $script:OdamexCompleter
Register-ArgumentCompleter -CommandName odasrv -Native -ScriptBlock $script:OdamexCompleter

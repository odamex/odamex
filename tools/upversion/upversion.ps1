### Upversion.ps1
### Uses regexes to intelligently update code files to the new version and year.
### Designed to be run in the base git path

$majminpatchRegex = "(\d+)[.](\d+)[.](\d+)"
$majminpatchCommaRegex = "(\d+)[, ]+(\d+)[, ]+(\d+)"
$majminpatchbuildCommaRegex = "(\d+)[,](\d+)[,](\d+)[,](\d+)"
$configverRegex = "(\d{6})"
$yearRegex = "2006-(\d{4})"

$maj = "$env:MAJORVERSION"
$min = "$env:MINORVERSION"
$patch = "$env:PATCHVERSION"

if ([String]::IsNullOrWhiteSpace($maj)) {
    $maj = "10"
}

if ([String]::IsNullOrWhiteSpace($min)) {
    $min = "6"
}

if ([String]::IsNullOrWhiteSpace($patch)) {
    $patch = "0"
}

$curYear = (Get-Date).year

$majminpatch = "$maj.$min.$patch"
$majminpatchcomma = "$maj, $min, $patch"
$majminpatchbuildComma = "$maj,$min,$patch,0"

if (([int]$maj -le 9) -and ([int]$min -le 9) -and ([int]$patch -le 9))
{
  $configver = "$maj" + "$min" + "$patch"  
}
else
{
  $configver = $maj.PadLeft(3, '0') + $min.PadLeft(2, '0') + $patch.PadLeft(1, '0')
}

$year = "2006-$curYear"

$majminpatchFiles = @(
'.\README',
'.\CMakeLists.txt',
'.\Xbox\README.Xbox',
'.\ag-odalaunch\res\Info.plist',
'.\common\version.h',
'.\odalaunch\res\Info.plist',
'.\switch.cmake',
'.\tools\upversion\upversion.ini'
)

$majminpatchCfgs = Get-ChildItem '.\config-samples' -Filter "*.cfg"

$yearFiles = (Get-ChildItem . -Recurse -File) | ? {($_.FullName -notmatch "build") -and ($_.FullName -notmatch "out") -and ($_.FullName -notmatch "libraries")}

$majminpatchCommaFiles = @(
'.\odalpapi\net_packet.h',
'.\common\version.h'
)

$majminpatchCommaSpaceFiles = @(
'.\CMakeLists.txt'
)

$configverFiles = @(
'.\common\version.h'
)

# Update simple versions in individual files (e.g. 10.4.0)
foreach ($file in $majminpatchFiles) {
   $content = Get-Content -Raw $file

   if ($content -match $majminpatchRegex) {
        Write-Output "MajMinPatch Match found in $file"

        $newcontent = $content -replace $majminpatchRegex, $majminpatch
        Set-Content -Path $file -Value $newcontent -NoNewline
        Write-Output "Replaced MajMinPatch in $file"

   }
}



# Update simple versions in configs (e.g. 10.4.0)
foreach ($file in $majminpatchCfgs) {
   $filename = $file.FullName

   $content = Get-Content -Raw $filename

   if ($content -match $majminpatchRegex) {
        Write-Output "MajMinPatch Match found in $filename"

        $newcontent = $content -replace $majminpatchRegex, $majminpatch
        Set-Content -Path $filename -Value $newcontent -NoNewline
        Write-Output "Replaced MajMinPatch in $filename"
   }
}

# Update simple versions with commas (e.g. 10, 4, 0)
foreach ($file in $majminpatchCommaFiles) {
   $content = Get-Content -Raw $file

   if ($content -match $majminpatchCommaRegex) {
        Write-Output "MajMinPatchComma Match found in $file"

        $newcontent = $content -replace $majminpatchCommaRegex, $majminpatchcomma
        Set-Content -Path $file -Value $newcontent -NoNewline
        Write-Output "Replaced MajMinPatchComma in $file"
   }
}

# Update min.maj.patch.build versions with commas (e.g. 10, 4, 0, 0)
foreach ($file in $majminpatchCommaSpaceFiles) {
   $content = Get-Content -Raw $file

   if ($content -match $majminpatchbuildCommaRegex) {
        Write-Output "MajMinPatchBuildComma Match found in $file"

        $newcontent = $content -replace $majminpatchbuildCommaRegex, $majminpatchbuildComma
        Set-Content -Path $file -Value $newcontent -NoNewline
        Write-Output "Replaced MajMinPatchBuildComma in $file"
   }
}

# Update configversionstr (e.g. 010030)
foreach ($file in $configverFiles) {
   $content = Get-Content -Raw $file

   if ($content -match $configverRegex) {
        Write-Output "ConfigVersionStr Match found in $file"

        $newcontent = $content -replace $configverRegex, $configver
        Set-Content -Path $file -Value $newcontent -NoNewline
        Write-Output "Replaced ConfigVersionStr in $file"
   }
}

# Update years (e.g. 2006-2024)
foreach ($file in $yearFiles) {
   $filename = $file.FullName
   $content = Get-Content -Raw $filename

   if ($content -match $yearRegex) {
        Write-Output "Year Match found in $filename"

        $newcontent = $content -replace $yearRegex, $year
        Set-Content -Path $filename -Value $newcontent -NoNewline
        Write-Output "Replaced Year in $filename"
   }
}
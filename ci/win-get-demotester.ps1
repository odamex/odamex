Set-PSDebug -Trace 1

$DemoTesterPath = "https://github.com/bcahue/odatests/releases/download/1.0.0/odatests-v1.0.0.zip"
$DemoResourcePath = "https://github.com/bcahue/odatests-resources/releases/download/1.0.0/odatests-resources-v1.0.0.zip"

Set-Location "build"
New-Item -Name "demotester" -ItemType "directory" | Out-Null

Invoke-WebRequest -Uri $DemoTesterPath -OutFile .\odatests.zip
Invoke-WebRequest -Uri $DemoResourcePath -OutFile .\odatests-resources.zip

7z.exe e odatests.zip -o./demotester
7z.exe e odatests-resources.zip -o./demotester

Set-Location ..

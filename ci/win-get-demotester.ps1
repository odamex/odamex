Set-PSDebug -Trace 1

$DemoTesterPath = "https://github.com/bcahue/odatests/releases/download/1.0.0/odatests-v1.0.0.zip"
$DemoResourcePath = "https://github.com/bcahue/odatests-resources/releases/download/1.0.0/odatests-resources-v1.0.0.zip"

Set-Location "build"
New-Item -Name "demotester" -ItemType "directory" | Out-Null

Invoke-WebRequest -Uri $DemoTesterPath -OutFile .\odatests.zip
Invoke-WebRequest -Uri $DemoResourcePath -OutFile .\odatests-resources.zip

Expand-Archive .\odatests.zip -Destination .\demotester
Expand-Archive .\odatests-resources.zip -Destination .\demotester

Set-Location ..

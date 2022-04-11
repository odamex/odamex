Set-PSDebug -Trace 1

$DemoTesterPath = "https://github.com/bcahue/odatests/releases/download/1.0.0/odatests-v1.0.0.zip"
$DemoResourcePath = "https://github.com/bcahue/odatests-resources/releases/download/1.0.0/odatests-resources-v1.0.0.zip"

Set-Location "build"
New-Item -Name "demotester" -ItemType "directory" | Out-Null

Invoke-WebRequest -Uri $DemoTesterPath -OutFile .
Invoke-WebRequest -Uri $DemoResourcePath -OutFile .

Expand-Archive $DemoTesterPath -Destination .demotester/
Expand-Archive $DemoResourcePath -Destination .demotester/

Set-Location ..

Set-PSDebug -Trace 1

if (($env:DEMOTESTER_URL -ne null) -and ($env:DEMORESOURCES_URL -ne null))
{
		Write-Output "OdaTests Download URL: $env:DEMOTESTER_URL"
		Write-Output "OdaTests WAD Resources Download URL: $env:DEMORESOURCES_URL"

		$DemoTesterPath = $env:DEMOTESTER_URL
		$DemoResourcePath = $env:DEMORESOURCES_URL

		Set-Location "build"
		New-Item -Name "demotester" -ItemType "directory" | Out-Null

		Invoke-WebRequest -Uri $DemoTesterPath -OutFile .\odatests.zip
		Invoke-WebRequest -Uri $DemoResourcePath -OutFile .\odatests-resources.zip

		7z.exe x odatests.zip -odemotester -y
		7z.exe x odatests-resources.zip -odemotester -y

		Set-Location ..
}
else
{
	Write-Output "OdaTests URL or OdaTests Resources URL missing, skipping..."
}

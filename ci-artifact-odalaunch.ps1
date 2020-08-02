Set-PSDebug -Trace 1

Set-Location build-gcc
mkdir artifact | Out-Null

# Copy all built files into artifact directory
Copy-Item `
    -Path ".\odalaunch\odalaunch.exe", `
    ".\odalaunch\libgcc_s_seh-1.dll", `
    ".\odalaunch\libstdc++-6.dll", `
    ".\odalaunch\libwinpthread-1.dll", `
    ".\wxMSW\lib\gcc_dll\wxbase313u_gcc810_x64.dll", `
    ".\wxMSW\lib\gcc_dll\wxbase313u_net_gcc810_x64.dll", `
    ".\wxMSW\lib\gcc_dll\wxbase313u_xml_gcc810_x64.dll", `
    ".\wxMSW\lib\gcc_dll\wxmsw313u_core_gcc810_x64.dll", `
    ".\wxMSW\lib\gcc_dll\wxmsw313u_html_gcc810_x64.dll", `
    ".\wxMSW\lib\gcc_dll\wxmsw313u_xrc_gcc810_x64.dll" `
    -Destination "artifact"

Set-Location ..

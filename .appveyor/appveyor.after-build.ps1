# Get build path
$path = $env:APPVEYOR_BUILD_FOLDER;
# Create deploy folder
mkdir $path\deploy;
# Copy *.sys files from Release folder preserving directory structure
robocopy $path\lowcdc\Release $path\deploy *.sys /s ;
# Copy particle.inf into deploy folder
Copy-Item $path\particle.inf $path\deploy\particle.inf ;

cmd.exe /c "`"${env:INF2CAT}`" /v /driver:$path\deploy /os:XP_X86,Vista_X86,Vista_X64,7_X86,7_X64,8_X86,8_X64,6_3_X86,6_3_X64,6_3_ARM";

$sign = "`"${env:SIGNTOOL}`"  sign /v /ac AddTrust_External_CA_Root.cer /f windows_key.p12 /p %key_secret% /tr http://tsa.starfieldtech.com";

cmd.exe /c "$sign $path\deploy\x86\lowcdc.sys" ;
cmd.exe /c "$sign $path\deploy\amd64\lowcdc.sys" ;
cmd.exe /c "$sign $path\deploy\particle.cat" ;

# Create a zip
7z a windows-device-drivers.zip $path\deploy\* ;

exit

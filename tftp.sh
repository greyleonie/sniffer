#!/bin/bash

# Save current directory
tftpHost=$1
FileDir=$2
tftpCmd=$3
tftpFile=$4
CurrDir=`pwd`

echo "$CurrDir $tftpHost $FileDir $tftpCmd $tftpFile"

# Go to upload file directory
cd $FileDir

# tftp process
tftp $tftpHost -c $tftpCmd $tftpFile
if [ $? != "0" ]; then
	echo "ERROR:tftp failed."
	cd $CurrDir
	exit 1
fi

# Go back to current directory
cd $CurrDir

echo "tftp done."



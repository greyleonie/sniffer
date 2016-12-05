#!/bin/bash

#Get parameter from command line
MoteType=$1
MoteID=$2
MotePort=$3
TargetFile=$4	#target file path name
TosVersion=$5
TempFile="$TargetFile.out-$MoteID"

echo "$MoteType $MoteID $MotePort $TargetFile $TempFile $TosVersion"

#Select tinyos version.
if [ $TosVersion = "tos1" ]; then
	# The "set-mote-id" in directory "$TOSROOT/tools/make/msp" must be in $PATH
	SetMoteId="set-mote-id"
	SetMoteIdCfg="$MoteID"
	MicaFuse="0xd8"
elif [ $TosVersion = "tos2" ]; then
	# The `tos-set-symbols` must be in $PATH
	SetMoteId="tos-set-symbols"
	SetMoteIdCfg="TOS_NODE_ID=$MoteID ActiveMessageAddressC\$addr=$MoteID"
	MicaFuse="0xd9"
else
	echo "ERROR:Invalid Argument \"$TosVersion\"."
	exit 1
fi #end if [ $TosVersion = "tos1" ]; then


#Set mote ID and Upload mote code.
if [ $MoteType = "telosb" ]; then
	#Make temp hex file with Mote ID.
	$SetMoteId --objcopy msp430-objcopy --objdump msp430-objdump --target ihex $TargetFile $TempFile $SetMoteIdCfg
	if [ $? != "0" ]; then 
		echo "ERROR:Set Mote ID failed."
		exit 1
	fi

	#Upload target code into the mote.
	tos-bsl --telosb -c $MotePort -reIp $TempFile 
	if [ $? != "0" ]; then
		echo "ERROR:upload mote failed."
		exit 1
	fi

elif [ $MoteType = "micaz" ] || [ $MoteType = "mib520" ]; then
	#Make temp hex file with Mote ID.
	$SetMoteId $TargetFile $TempFile $SetMoteIdCfg
	if [ $? != "0" ]; then 
		echo "ERROR:Set Mote ID failed."
		exit 1
	fi
	
	#Upload target code into the mote.
	uisp -dprog=mib510 -dserial=$MotePort --wr_fuse_h=$MicaFuse -dpart=ATmega128 --wr_fuse_e=ff --erase --upload if=$TempFile
	if [ $? != "0" ]; then
		echo "ERROR:upload mote failed."
		exit 1
	fi

else
	echo "ERROR:Invalid Argument \"$MoteType\"."
	exit 1

fi #end if [ $MoteType = "telosb" ]; then

#Remove the temp file
rm -f $TempFile

echo "upload done."



#!/bin/bash

###################################################
# set sniffer working directory
MOTE_MAP=$1
###################################################

####################################################
# check mote-map file existed
if [ -f $MOTE_MAP ]; then 
	OldDevID=$(awk -F: '{print $1}' $MOTE_MAP)
	$(cp /dev/null $MOTE_MAP)	#clear mote-map
else 
	OldDevID=
	$(touch $MOTE_MAP)	#create mote-map file	
fi
####################################################

####################################################
# search usb device, directory name like "1-1","1-2"...
# and save device information to mote-map file
DriverDir=/sys/bus/usb/drivers/usb
for dev in $(ls $DriverDir | grep -E '^[1-9]-[1-9]$')
do
#	echo "dev=$dev"
	# get device directory
	DevDir=$DriverDir/$dev
	
	# get device id
	DevNum=$(cat $DevDir/devnum)

	# get device information
	Pid=$(cat $DevDir/idProduct)
	Vid=$(cat $DevDir/idVendor)
	IntfaceNum=$(cat $DevDir/bNumInterfaces)
	Product=$(cat $DevDir/product)
	Manu=$(cat $DevDir/manufacturer)

	# check information
	if [ $Pid = "6001" ] && [ $Vid = "0403" ] && [ $Manu = "FTDI" ]; then
		DevType="telosb"
		interface=$(ls $DevDir/ | grep -E '^[1-9]')
		msgport=$(ls $DevDir/$interface | grep -E '^tty')
		updport=$msgport
	elif [ $Pid = "6010" ] && [ $Vid = "0403" ] && [ $Manu = "Crossbow" ]; then
		DevType="mib520"
		for interface in $(ls $DevDir/ | grep -E '^[1-9]')	# interface directory like "1-1:1.0"
		do
			if [ $(expr index "$interface" "0") != 0 ] ; then
				updport=$(ls $DevDir/$interface | grep -E '^tty')
			elif [ $(expr index "$interface" "1") != 0 ] ; then
				msgport=$(ls $DevDir/$interface | grep -E '^tty')
			fi
		done # end for interface
	else
		continue
	fi # end if 

	# save mote-map file
	log="$DevNum:$DevType:$msgport:$updport"
	echo $log >> $MOTE_MAP

done #end for dev
####################################################


####################################################
# check device information change
NewDevID=$(awk -F: '{print $1}' $MOTE_MAP)	# get new device IDs
IsIDMatch="TRUE"

# check old device pull out
for old in $OldDevID
do
	IsIDMatch="FALSE"
	for new in $NewDevID
	do
		if [ $old = $new ]; then
			IsIDMatch="TRUE"
			break
		fi
	done	

	if [ $IsIDMatch = "FALSE" ]; then
		break
	fi
done

if [ $IsIDMatch = "FALSE" ]; then
	echo "device pull out."
	exit 1
fi
####################################################

####################################################
# check new device insert
for new in $NewDevID
do
	IsIDMatch="FALSE"
	for old in $OldDevID
	do
		if [ $old = $new ]; then
			IsIDMatch="TRUE"
			break
		fi	
	done
	if [ $IsIDMatch = "FALSE" ]; then
		break
	fi
done

if [ $IsIDMatch = "FALSE" ]; then
	echo "new device insert."
	exit 1
fi
####################################################

echo "device not change."



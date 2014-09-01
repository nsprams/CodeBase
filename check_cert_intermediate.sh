#!/usr/bin/bash


VERSION="Version 0.1"
AUTHOR="pramod@elance-odesk.com"
PROGNAME=`/usr/bin/basename $0`

#### EXIT CODES ####
STATE_OK=0
STATE_WARNING=1
STATE_CRITICAL=2
STATE_UNKNOWN=3

#### Helper functions ####
function print_revision {
	# print the revision number 
	echo "$PROGNAME - $VERSION"
}


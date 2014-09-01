#!/usr/bin/bash

VERSION="Version 0.1"
AUTHOR="pramod@elance-odesk.com"
PROGNAME=`/usr/bin/basename $0`

#### Exit codes ####
STATE_OK=0
STATE_WARNING=1
STATE_CRITICAL=2
STATE_UNKNOWN=3

#### Helper functions ####
function print_revision {
   # Print the revision number
   echo "$PROGNAME - $VERSION"
}

function print_usage {
   # Print a short usage statement
   echo "Usage: $PROGNAME [-v] -w <limit> -c <limit>"
}

function print_help {
   # Print detailed help information
   print_revision
   print_usage

   /usr/bin/cat <<__EOT

Options:
-h
   Print detailed help screen
-V
   Print version information
-d domainname
   Enter the domain name for which you need to check the certificate
-p portnumber
   port number to connect to, defaults to 443
-w INTEGER
   Exit with WARNING status if less than INTEGER days to certificate expiry
-c INTEGER
   Exit with CRITICAL status if less than INTEGER days to certificate expiry
-v
   Verbose output
__EOT
}


#### MAIN ####
#Expiry Date 
function compute_days_to_expire {
expiry_date=`echo |\
  openssl s_client -connect ${check_domain}:${check_port} 2>/dev/null |\
  sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' |\
  openssl x509 -noout -enddate|cut -d= -f2-`
epoch_expiry_date=`date -d "$expiry_date" +'%s'`
epoch_current_date=`date +'%s'`
let days_to_expiry=($epoch_expiry_date - $epoch_current_date)/86400

}



#### Check conditions, alert ####
# Comparing the results of the computation against thresholds and exiting.
function determine_status {
if [[ "$days_to_expiry" -lt "$critical_threshold" ]]; then
#	echo "Comparing $days_to_expiry with  $critical_threshold"
	echo "CERT CRITICAL: $CERT expires in $days_to_expiry days."
	exit $STATE_CRITICAL
elif [[ "$days_to_expiry" -lt "$warning_threshold" ]]; then
#	echo "Comparing $days_to_expiry with  $critical_threshold"
	echo "CERT WARNING: $CERT expires in $days_to_expiry days."
	exit $STATE_WARNING
else 
#	echo "Comparing $days_to_expiry with  $critical_threshold"
	echo "CERT OK: $days_to_expiry days to expire."
	exit $STATE_OK
fi
}








# Parse command line options
while [ "$1" ]; do
#echo "--------"
#echo "ONE: $1 .. $2"
   case "$1" in
       -h | --help)
           print_help
           exit $STATE_OK
           ;;
       -V | --version)
           print_revision
           exit $STATE_OK
           ;;
       -v | --verbose)
           : $(( verbosity++ ))
           shift
           ;;
       -w | --warning | -c | --critical| -d | --domain | -p | --port )
	   if [[ "$1" = -w || $1 = --warning ]]; then
		if [[ -n "$2" ]]; then
			warning_threshold=$2
		fi
	        shift 2
	   elif [[ "$1" = -c || $1 = --critical ]]; then
		if [[ -n "$2" ]]; then
			critical_threshold=$2
		fi
	        shift 2
	   elif [[ "$1" = -d || $1 = --domain ]]; then
		if [[ -n "$2" ]]; then
			check_domain=$2
		fi
	        shift 2
	   elif [[ "$1" = -p || $1 = --port ]]; then
		if [[ -n "$2" ]]; then
			check_port=$2
		fi
	        shift 2
           else
               # Threshold is neither a number nor a percentage
               echo "$PROGNAME: Threshold must be integer or percentage"
               print_usage
               exit $STATE_UNKNOWN
           fi
	   # Validate the all required values are present
           ;;
	
       -?)
           print_usage
           exit $STATE_OK
           ;;
       *)
           echo "$PROGNAME: Invalid option '$1'"
           print_usage
           exit $STATE_UNKNOWN
           ;;
   esac
done



#### Ensuring the values have been collected in variables ####

if [[ -z "$warning_threshold" || -z "$critical_threshold" || -z "$check_domain" || -z "$check_port" ]]; then
   # One or both thresholds were not specified
#echo "W: $warning_threshold C:$critical_threshold D:$check_domain P:$check_port"
   echo "$PROGNAME: Threshold not set or domain:port not provided"
   print_usage
   exit $STATE_UNKNOWN
elif [[ "$critical_threshold" -gt "$warning_threshold" ]]; then
   # The warning threshold must be greater than the critical threshold
   echo "$PROGNAME: Warning days to expire should be higher than  critical days to expire"
   print_usage
   exit $STATE_UNKNOWN
fi

#If all validation passes, proceed to computation, determine status and return

compute_days_to_expire
determine_status




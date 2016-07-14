#!/bin/bash

year=`date -u +%Y`
day=`date -u +%j`

brdc_file="brdc${day}0.16n"
brdc_file_year="brdc${year}_${day}0.16n"

if [ ! -e "$brdc_file_year" ]; then
	wget ftp://cddis.gsfc.nasa.gov/gnss/data/daily/${year}/brdc/${brdc_file}.Z -O ${brdc_file_year}.Z
	
	if [ ! -e "${brdc_file_year}.Z" ]; then
		echo "Can't download BRDC file"
		exit
	fi
	uncompress ${brdc_file_year}.Z
fi

./bladegps -e $brdc_file_year -t `date -u +'%Y/%m/%d,%H:%M:%S'` $*

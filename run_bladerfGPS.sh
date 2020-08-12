#!/bin/bash

export BLADERF_SEARCH_DIR="./external/prebuilt/fpga_images" 
year=`date -u +%Y`
day=`date -u +%j`

brdc_file="brdc${day}0.${year: -2}n"
brdc_file_year="brdc${year}_${day}0.${year: -2}n"

if [ ! -e ".wgetrc" ]; then
  echo -e "http_user=<INSERT_USERNAME>\nhttp_password=<INSERT_PASSWORD>" > .wgetrc
  echo "Enter your CDDIS portal credentials into .wgetrc and then re-run"
  exit
fi

export WGETRC=./.wgetrc

if [ ! -e "$brdc_file_year" ]; then
	wget --auth-no-challenge "https://cddis.nasa.gov/archive/gnss/data/daily/${year}/brdc/${brdc_file}.Z" -O ${brdc_file_year}.Z
	if [ ! -e "${brdc_file_year}.Z" ]; then
		echo "Can't download BRDC file"
		exit
	fi
	uncompress ${brdc_file_year}.Z
fi

if [ ! -e "external/prebuilt/fpga_images/hostedx115.rbf" ]; then
  echo "downloading hostedx115.rbf to external/prebuilt/fpga_images"
  wget "https://www.nuand.com/fpga/v0.11.0/hostedx115.rbf" -P external/prebuilt/fpga_images
fi

if [ ! -e "external/prebuilt/fpga_images/hostedx40.rbf" ]; then
  echo "downloading hostedx40.rbf to external/prebuilt/fpga_images"
  wget "https://www.nuand.com/fpga/v0.11.0/hostedx40.rbf" -P external/prebuilt/fpga_images
fi

if [ ! -e "external/prebuilt/fpga_images/hostedxA4.rbf" ]; then
  echo "downloading hostedxA4.rbf to external/prebuilt/fpga_images"
  wget "https://www.nuand.com/fpga/v0.11.0/hostedxA4.rbf" -P external/prebuilt/fpga_images
fi

if [ ! -e "external/prebuilt/fpga_images/hostedxA9.rbf" ]; then
  echo "downloading hostedxA9.rbf to external/prebuilt/fpga_images"
  wget "https://www.nuand.com/fpga/v0.11.0/hostedxA9.rbf" -P external/prebuilt/fpga_images
fi

./bladegps -e $brdc_file_year -t `date -u +'%Y/%m/%d,%H:%M:%S'` $*

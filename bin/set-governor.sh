#!/bin/bash

###############################################################################
# Sets the cpufreq governor of a Linux system to $1. If governor is set to 
# 'userspace' all CPU are set to minimum frequency.
#
# Usage: $ sudo set-governor.sh <governor> 
#           where <governor> is one of 
#          'userspace', 'ondemand', 'performance', 'powersave', or 
#          'conservative'"
#
#
# Author: Tom Henretty <henretty@reservoir.com>
#
###############################################################################
#
# Copyright 2013-2014 Reservoir Labs, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################


USAGE="Usage: $0 <governor> where <governor> is one of 'userspace', 'ondemand', 'performance', 'powersave', or 'conservative'"

if [ "x$1" == "x" ] 
then
	echo $USAGE
	exit 1
fi

if [ $1 != "ondemand" ] && [ $1 != "userspace" ] && [ $1 != "performance" ] && [ $1 != "powersave" ] && [ $1 != "conservative" ] 
then
	echo $USAGE
	exit 2
fi

GOVERNOR=$1
CPUS=`ls -1 -d /sys/devices/system/cpu/cpu* | grep -v freq | grep -v idle`
for CPU in $CPUS ; do
	echo $1 > $CPU/cpufreq/scaling_governor
	echo -n "`basename $CPU`: " && cat $CPU/cpufreq/scaling_governor
done

if [ $GOVERNOR == "userspace" ]
then
    echo "Setting minimum frequencies..."
    for CPU in $CPUS ; do
        MINFREQ=`cat $CPU/cpufreq/scaling_min_freq`
        echo $MINFREQ > $CPU/cpufreq/scaling_setspeed
        echo `cat $CPU/cpufreq/scaling_cur_freq`
    done
fi

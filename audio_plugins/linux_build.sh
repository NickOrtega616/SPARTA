#!/bin/bash

# script to "projuce" and build JUCER plugins on Linux

# parameters:
# without parameters the script will show info about the plugin(s)
# - a directory: the script use it as a starting point (only one)
# - "build": to build the plugin(s)
# "projucer": to open Projucer for plugins(s)
# "projuce": to regenerate Projucer files
# remaining params are passed to "make"
# ex: "_SPARTA_ambiBIN_ Projuce" open Projucer for _SPARTA_ambiBIN_ plugin
# ex: "projuce build clean" generates all Projucer files and cleans all builds
# ex: "_SPARTA_ambiBIN_ build -j6" builds _SPARTA_ambiBIN_ using 6 cores

from="."
build=0
projucer=0
projuce=0

# location of plugin binaries
binaries="../lib"

i=1
while [ $i -le $# ]
  do
    var="$1"
    if [ -d ${var} ]; then
      from=${var}
      i=$(($i-1))
    elif [ ${var} == "build" ]; then
      build=1
      i=$(($i-1))
    elif [ ${var} == "projucer" ]; then
      projucer=1
      i=$(($i-1))
    elif [ ${var} == "projuce" ]; then
      projuce=1
      i=$(($i-1))
    else
      set -- "$@" "$var"
    fi
    shift
    i=$(($i+1))
done

let "info = ${build} + ${projucer} + ${projuce}"
if [ ${info} -eq 0 ]; then
    if [ ${from} = "." ]; then
      ls -Rl --color ${binaries}
    else
      jucer=$(find "${from}" -type f -name "*.jucer")
      status="$(Projucer --status ${jucer} 2>&1)"
      echo "${status}"
      name=$(echo "${status}" | grep Name | cut -c 7-)
      ls -Rl --color ${binaries}/${name}
    fi
exit
fi

# opening Projucer editor
[ ${projucer} -gt 0 ] && find ${from} -type f -name "*.jucer" \
    -exec Projucer "{}" \;

# projucing (resaving Projucer files)
[ ${projuce} -gt 0 ] && find ${from} -type f -name "*.jucer" \
    -exec Projucer --resave "{}" \;

# building (or cleaning)
[ ${build} -gt 0 ] && find "${from}" -type d -name "LinuxMakefile" \
    -exec bash -c "cd \"{}\" && make CONFIG=Release $@" \;

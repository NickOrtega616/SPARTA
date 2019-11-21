#!/bin/bash

help_message="
script to \"projuce\" and build JUCER plugins on Linux

parameters:
without parameters the script will show info about the plugin(s)
- \"help\": to help this message
- a directory: the script use it as a starting point (only one)
- \"projucer\": to open Projucer for plugins(s)
- \"projuce\": to regenerate Projucer files
- \"clean\": to clean the plugin(s)
- \"build\": to build the plugin(s)
remaining params are passed to \"make\"

ex: \"_SPARTA_ambiBIN_ projucer\" opens Projucer for the \"_SPARTA_ambiBIN_\" plugin
ex: \"_SPARTA_ambiBIN_ build -j6\" builds _SPARTA_ambiBIN_ using 6 cores
ex: \"projuce clean build\" generates all Projucer files, clean and build all plugins
"

help=0
info=0
build=0
clean=0
projucer=0
projuce=0

# location of plugin binaries
binaries="${SCRIPT_PATH}/../lib"
# create it
mkdir -p "${binaries}"

SCRIPT_PATH=$(dirname `which $0`)
from="${SCRIPT_PATH}"
SDKs="${SCRIPT_PATH}/../SDKs"


i=$#
while [ $i -gt 0 ]; do
  var=$1
  if [ -d ${var} ]; then
    from="${var}"
  elif [ ${var} == "help" ]; then
    help=1
  elif [ ${var} == "info" ]; then
    info=1
  elif [ ${var} == "projucer" ]; then
    projucer=1
  elif [ ${var} == "projuce" ]; then
    projuce=1
  elif [ ${var} == "clean" ]; then
    clean=1
  elif [ ${var} == "build" ]; then
    build=1
  else
    set -- "$@" "$var"
  fi
  i=$(($i-1))
  shift
done

if [ $(( info + projucer + projuce + clean + build )) -eq 0 ]; then help=1; fi
if [ ${help} -gt 0 ]; then
  echo "${help_message}"
  exit
fi

# location of Projucer app: priority to the one built from source
projucer_from_source="${SDKs}/JUCE/extras/Projucer/Builds/LinuxMakefile/build/Projucer"
projucer_app="${projucer_from_source}"
[ ! -f ${projucer_app} ] && projucer_app=$(which Projucer)
if [ ! ${projucer_app} ] || [ ! -f ${projucer_app} ]; then
  echo "Projucer is not installed"
  echo "On Debian (and Ubuntu), install the \"juce-tools\" package"
  echo "or compile a GPL enabled version using this script:"
  echo "${SDKs}/linux-build-projucer.sh"
  echo "or install Projucer and set the \"projucer_app\" variable in this script"
  exit
fi

if [ ${info} -gt 0 ]; then
  if [ ${from} = "." ]; then
    ls -Rl --color ${binaries}
  else
    jucer=$(find "${from}" -type f -name "*.jucer")
    status="$(${projucer_app} --status ${jucer} 2>&1)"
    name=$(echo "${status}" | grep Name | cut -c 7-)
    echo "${status}"
    name=$(echo "${status}" | grep Name | cut -c 7-)
    ls -Rl --color ${binaries}/${name}
  fi
  exit
fi

# opening Projucer editor
[ ${projucer} -gt 0 ] && find "${from}" -type f -name "*.jucer" \
  -exec ${projucer_app} "{}" \;

# projucing (resaving Projucer files)
[ ${projuce} -gt 0 ] && find "${from}" -type f -name "*.jucer" \
  -exec ${projucer_app} --resave "{}" \;

# cleaning)
[ ${clean} -gt 0 ] && find "${from}" -type d -name "LinuxMakefile" \
  -exec bash -c "cd \"{}\" && make CONFIG=Release clean" \;

# building
[ ${build} -gt 0 ] && find "${from}" -type d -name "LinuxMakefile" \
  -exec bash -c "cd \"{}\" && make CONFIG=Release $@" \;

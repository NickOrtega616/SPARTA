#!/bin/bash

JUCE_VERSION=5.4.5

SCRIPT_PATH=$(dirname `which $0`)
cd "${SCRIPT_PATH}/JUCE"
git checkout ${JUCE_VERSION}

# enable GPL mode
cd extras/Projucer
sed -i 's/JUCER_ENABLE_GPL_MODE 0/JUCER_ENABLE_GPL_MODE 1/g' JuceLibraryCode/AppConfig.h

# build Projucer
cd Builds/LinuxMakefile
make -j



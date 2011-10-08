#!/bin/bash
#set -x
DESTDIR=~/music/OOMidi-release/
OOMBASE="oom-"
SRCDIR=~/workspace/git-projects/oom/
OOMEXCLUDES=${SRCDIR}oomfiles/excludes
VERSION=${1}
CMD=${DESTDIR}${OOMBASE}${VERSION}/
if [ -z "$VERSION" ]; then
	echo "Usage: $0 <version>"
	echo "Example: $0 2011.3"
else
	mkdir -p "${DESTDIR}"
	echo "Syncing distro"
	rsync -av --progress --delete --delete-excluded --exclude-from=${OOMEXCLUDES} ${SRCDIR} ${CMD}
fi

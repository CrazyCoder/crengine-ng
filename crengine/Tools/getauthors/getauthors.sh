#!/bin/sh

# The first item in the git log is Wed Mar 14 11:31:43 2007 +0000,
# but the project started around 2000, so the statistics
# we can get with this script are by definition incomplete.

srctree="../../../"

die()
{
    echo $*
    exit 1
}

git shortlog -s -e -n --group=author --no-merges \
    -- ${srctree}

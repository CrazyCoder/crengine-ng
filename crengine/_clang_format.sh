#!/bin/sh

dirs="include src Tools tests"
regex_pat=".*\.(h|c|cpp)"

excludes="^src\/locale_data\/files\/.*$ ^src/xxhash.h$ ^src/xxhash.c$ ^Tools\/Fb2Test\/.*$ ^Tools\/FontConv\/.*$"

die()
{
    echo $*
    exit 1
}

do_file()
{
    local f=$1
    echo -n "Processing file \"${f}\"... "
    clang-format -i "${f}" > /dev/null 2>&1
    echo "done"
    return 0
}

do_file_with_exc()
{
    local f=$1
    local p=
    for p in ${excludes}
    do
        if echo "${f}" | grep -i -e "${p}" > /dev/null
        then
            return 0
        fi
    done
    echo -n "Processing file \"${f}\"... "
    clang-format -i "${f}" > /dev/null 2>&1
    echo "done"
    return 0
}

export excludes
export -f die
export -f do_file
export -f do_file_with_exc

if [ ! -f .clang-format ]
then
    die "You can only run this script in <top_srcdir>/crengine!"
fi

uname_s=`uname -s`
is_darwin=no
test "x${uname_s}" = "xDarwin" && is_darwin="yes"

if [ "x${is_darwin}" = "xyes" ]
then
    for dir in ${dirs}
    do
        find -E ${dir} -depth -maxdepth 5 -type f -iregex ${regex_pat} -exec bash -c 'do_file_with_exc "$0"' '{}' \;
    done
else
    for dir in ${dirs}
    do
        find ${dir} -depth -maxdepth 5 -type f -regextype posix-egrep -iregex ${regex_pat} -exec bash -c 'do_file_with_exc "$0"' '{}' \;
    done
fi

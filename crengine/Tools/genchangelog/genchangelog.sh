#!/bin/sh

prev_tag=$1

if [ "x${prev_tag}" = "x" ]
then
    echo "Please specify the previous tag!"
    exit 1
fi

git log --format=format:"%as  %an  <%ae>%n%n	%s%n" ${prev_tag}..main

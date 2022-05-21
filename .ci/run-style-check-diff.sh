#!/bin/bash

set -e

# For now, clang-format is only used for the "crengine" directory, when
# another directory is formatted with clang-format, add it to this variable.
ALLOWED_CLANG_FORMAT_DIRS="crengine"

# We need to add a new remote for the upstream main, since this script could
# be running in a personal fork of the repository which has out of date branches.
if [ "x${CI_PROJECT_NAMESPACE}" != "x" -a "${CI_PROJECT_NAMESPACE}" != "coolreader-ng" ]; then
    echo "Retrieving the current upstream repository from ${CI_PROJECT_NAMESPACE}/${CI_PROJECT_NAME}..."
    git remote add upstream https://gitlab.com/coolreader-ng/crengine-ng.git
    ORIGIN="upstream"
else
    echo "Reusing the existing repository on ${CI_PROJECT_NAMESPACE}/${CI_PROJECT_NAME}"
    ORIGIN="origin"
fi

# Fetch full history to retrieve newest common ancestor commit
git fetch ${ORIGIN}

# Some debug noise...
echo "CI_MERGE_REQUEST_TARGET_BRANCH_NAME=${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}"
echo "CI_DEFAULT_BRANCH=${CI_DEFAULT_BRANCH}"
echo "CI_COMMIT_SHA=${CI_COMMIT_SHA}"
echo "ORIGIN=${ORIGIN}"
echo "ALLOWED_CLANG_FORMAT_DIRS=${ALLOWED_CLANG_FORMAT_DIRS}"

# Work out the newest common ancestor between the detached HEAD that this CI job
# has checked out, and the upstream target branch (which will typically be
# `upstream/main` or `upstream/some-feature-branch`).
#
# `${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}` is only defined if we’re running in
# a merge request pipeline; fall back to `${CI_DEFAULT_BRANCH}` otherwise.
newest_common_ancestor_sha=$(diff --old-line-format='' --new-line-format='' <(git rev-list --first-parent "${ORIGIN}/${CI_MERGE_REQUEST_TARGET_BRANCH_NAME:-${CI_DEFAULT_BRANCH}}") <(git rev-list --first-parent HEAD) | head -1)

if [ "x${newest_common_ancestor_sha}" = "x" ]
then
    echo "Can't get newest common ancestor commit"
    exit 1
fi
if [ "x${CI_COMMIT_SHA}" = "x" ]
then
    echo "Is this script running not in GitLab CI?"
    exit 1
fi
if [ "x${newest_common_ancestor_sha}" = "x${CI_COMMIT_SHA}" ]
then
    # Pushing to default branch?
    if [ "x${CI_COMMIT_BEFORE_SHA}" != "x" -a "${CI_COMMIT_BEFORE_SHA}" != "0000000000000000000000000000000000000000" ]
    then
        newest_common_ancestor_sha=${CI_COMMIT_BEFORE_SHA}
    else
        echo "Can't get difference commits. Assuming all OK."
        exit 0
    fi
fi
echo "newest_common_ancestor_sha=${newest_common_ancestor_sha}"

echo "The lines below should be interpreted as a patch, but if this inscription is the last one, there is no patch."
echo ""
git diff -U0 --no-color ${newest_common_ancestor_sha}..${CI_COMMIT_SHA} -- ${ALLOWED_CLANG_FORMAT_DIRS} | .ci/clang-format-diff.py -binary "clang-format" -p1
exit_status=$?

# The style check is not infallible.
# Hopefully we can eventually improve clang-format to
# be configurable enough for our coding style. That’s why this CI check is OK
# to fail: the idea is that people can look through the output and ignore it if
# it’s wrong. (That situation can also happen if someone touches pre-existing
# badly formatted code and it doesn’t make sense to tidy up the wider coding
# style with the changes they’re making.)

exit ${exit_status}

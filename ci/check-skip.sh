#!/usr/bin/env bash

# http://redsymbol.net/articles/unofficial-bash-strict-mode/

set -euo pipefail
IFS=$'\n\t'

echo "==> Check to see if the build should be skipped"

printf "GITHUB_REPOSITORY=%s\n" "${GITHUB_REPOSITORY}"
printf "GITHUB_EVENT_NAME=%s\n" "${GITHUB_EVENT_NAME}"
printf "GITHUB_EVENT_PATH=%s\n" "${GITHUB_EVENT_PATH}"

if [[ $GITHUB_EVENT_NAME != "pull_request" ]]; then
    echo "==> Event is not a pull request, build will proceed."
    echo "should_skip=false" >> $GITHUB_OUTPUT
    exit 0
fi

GITHUB_PR_REPOSITORY="$(cat "$GITHUB_EVENT_PATH" | jq --raw-output ".pull_request.head.repo.full_name")"
printf "GITHUB_PR_REPOSITORY=%s\n" "${GITHUB_PR_REPOSITORY}"

if [[ $GITHUB_REPOSITORY != $GITHUB_PR_REPOSITORY ]]; then
    echo "==> Pull request came from an outside repository, build will proceed."
    echo "should_skip=false" >> $GITHUB_OUTPUT
    exit 0
fi

echo "==> Internal pull request detected, skipping build."
echo "should_skip=true" >> $GITHUB_OUTPUT
exit 0

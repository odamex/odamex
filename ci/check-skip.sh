#!/usr/bin/env bash

echo "==> Check to see if the build should be skipped"

printf "GITHUB_EVENT_NAME=%s" "${GITHUB_EVENT_NAME}"
printf "GITHUB_ACTOR=%s" "${GITHUB_ACTOR}"

if [[ $GITHUB_EVENT_NAME != "pull_request" ]]; then
    echo "==> Event is not a pull request, build will proceed."
    echo "::set-output name=should_skip::false"
    exit 0
fi

if [[ $GITHUB_ACTOR != "odamex" ]]; then
    echo "==> Pull request came from an outside repository, build will proceed."
    echo "::set-output name=should_skip::false"
    exit 0
fi

echo "==> Internal pull request detected, skipping build."
echo "::set-output name=should_skip::true"
exit 0
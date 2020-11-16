#!/usr/bin/env bash

echo "==> Check to see if the build should be skipped"

printf "GITHUB_WORKFLOW=%s\n" "${GITHUB_WORKFLOW}"
printf "GITHUB_RUN_ID=%s\n" "${GITHUB_RUN_ID}"
printf "GITHUB_RUN_NUMBER=%s\n" "${GITHUB_RUN_NUMBER}"
printf "GITHUB_ACTION=%s\n" "${GITHUB_ACTION}"
printf "GITHUB_ACTIONS=%s\n" "${GITHUB_ACTIONS}"
printf "GITHUB_ACTOR=%s\n" "${GITHUB_ACTOR}"
printf "GITHUB_REPOSITORY=%s\n" "${GITHUB_REPOSITORY}"
printf "GITHUB_EVENT_NAME=%s\n" "${GITHUB_EVENT_NAME}"
printf "GITHUB_EVENT_PATH=%s\n" "${GITHUB_EVENT_PATH}"
printf "GITHUB_WORKSPACE=%s\n" "${GITHUB_WORKSPACE}"
printf "GITHUB_SHA=%s\n" "${GITHUB_SHA}"
printf "GITHUB_REF=%s\n" "${GITHUB_REF}"
printf "GITHUB_HEAD_REF=%s\n" "${GITHUB_HEAD_REF}"
printf "GITHUB_BASE_REF=%s\n" "${GITHUB_BASE_REF}"
printf "GITHUB_SERVER_URL=%s\n" "${GITHUB_SERVER_URL}"
printf "GITHUB_API_URL=%s\n" "${GITHUB_API_URL}"
printf "GITHUB_GRAPHQL_URL=%s\n" "${GITHUB_GRAPHQL_URL}"

echo "::set-output name=should_skip::true"
exit 0

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

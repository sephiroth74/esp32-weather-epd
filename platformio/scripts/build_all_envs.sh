#!/usr/bin/env bash
# Builds every firmware env in platformio.ini, each in its own `pio run`:
# mixing espressif32 and pioarduino envs in one run breaks PlatformIO's tooling.
#
#   scripts/build_all_envs.sh          build all envs and print a summary
#   scripts/build_all_envs.sh --list   print the env names as a JSON array
#
# Test envs (those with test_build_src) are skipped. An env whose location file
# is missing fails, unless built with the committed template:
#   PLATFORMIO_BUILD_FLAGS=-DALLOW_EXAMPLE_LOCATION scripts/build_all_envs.sh

set -u
cd "$(dirname "$0")/.."

list_envs() {
  pio project config --json-output | python3 -I -c '
import json, sys
print(json.dumps([section[4:] for section, options in json.load(sys.stdin)
                  if section.startswith("env:")
                  and "test_build_src" not in dict(options)]))'
}

if [ "${1:-}" = "--list" ]; then
  list_envs
  exit
fi

envs=$(list_envs | python3 -I -c 'import json, sys; print(" ".join(json.load(sys.stdin)))')
summary=""
failed=0
for env in $envs; do
  echo "=== $env"
  # After an espressif32 env, a pioarduino env reinstalls its tools: doing that
  # in the same process as the build fails with a TypeError, so do it first.
  if pio pkg install -e "$env" > /dev/null && pio run -e "$env"; then
    status="OK"
  else
    status="FAILED"
    failed=1
  fi
  summary="$summary$(printf '%-32s %s' "$env" "$status")\n"
done

printf '\n%b' "$summary"
exit $failed

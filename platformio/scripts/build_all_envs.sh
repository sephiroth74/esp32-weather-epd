#!/usr/bin/env zsh
# Builds every firmware env in platformio.ini, one `pio run` per env, and
# prints a summary.
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

summary=""
failed=0
# a command substitution is word-split by both bash and zsh (unlike a variable
# in zsh)
for env in $(list_envs | python3 -I -c 'import json, sys; print(" ".join(json.load(sys.stdin)))'); do
  echo "=== $env"
  if pio run -e "$env"; then
    result="OK"
  else
    result="FAILED"
    failed=1
  fi
  summary="$summary$(printf '%-32s %s' "$env" "$result")\n"
done

printf '\n%b' "$summary"
exit $failed

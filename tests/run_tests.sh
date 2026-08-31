#!/bin/sh
# Runs every program in tests/ and compares the result with the recorded one.
#
#   tests/testN.txt          must succeed; its stdout is compared with
#                            tests/expected/testN.out
#   tests/errors/NAME.txt    must fail; its stderr is compared with
#                            tests/expected/NAME.err
#
# Run with UPDATE=1 to rewrite the recorded files after a deliberate change.

BIN=${BIN:-bin/interpreter}
root=$(dirname "$0")/..
cd "$root" || exit 1

if [ ! -x "$BIN" ]; then
	echo "no $BIN, run make first" >&2
	exit 1
fi

passed=0
failed=0

# compares one recorded stream with what the run produced
check_against() { # name, label, golden, actual
	if printf '%s\n' "$4" | diff -q - "$3" > /dev/null; then
		printf 'ok   %s\n' "$1"
		passed=$((passed + 1))
		return
	fi
	printf 'FAIL %s (%s)\n' "$1" "$2"
	printf '%s\n' "$4" | diff -u "$3" - | sed 's/^/     /'
	failed=$((failed + 1))
}

for src in tests/test*.txt; do
	name=$(basename "$src" .txt)
	golden=tests/expected/$name.out
	actual=$("$BIN" < "$src" 2>/dev/null)
	code=$?
	if [ -n "$UPDATE" ]; then
		printf '%s\n' "$actual" > "$golden"
		continue
	fi
	if [ "$code" -ne 0 ]; then
		printf 'FAIL %s (exit %s, expected 0)\n' "$name" "$code"
		failed=$((failed + 1))
		continue
	fi
	if [ ! -f "$golden" ]; then
		printf 'FAIL %s (no %s)\n' "$name" "$golden"
		failed=$((failed + 1))
		continue
	fi
	check_against "$name" stdout "$golden" "$actual"
done

for src in tests/errors/*.txt; do
	name=$(basename "$src" .txt)
	golden=tests/expected/$name.err
	actual=$("$BIN" < "$src" 2>&1 >/dev/null)
	code=$?
	if [ -n "$UPDATE" ]; then
		printf '%s\n' "$actual" > "$golden"
		continue
	fi
	if [ "$code" -eq 0 ]; then
		printf 'FAIL %s (exit 0, expected a failure)\n' "$name"
		failed=$((failed + 1))
		continue
	fi
	if [ ! -f "$golden" ]; then
		printf 'FAIL %s (no %s)\n' "$name" "$golden"
		failed=$((failed + 1))
		continue
	fi
	check_against "$name" stderr "$golden" "$actual"
done

# the command line itself: a program may come from a file or from stdin
if [ -z "$UPDATE" ]; then
	from_stdin=$("$BIN" < tests/test6.txt 2>/dev/null)
	from_file=$("$BIN" tests/test6.txt 2>/dev/null)
	if [ "$from_stdin" = "$from_file" ]; then
		printf 'ok   cli_file_argument\n'
		passed=$((passed + 1))
	else
		printf 'FAIL cli_file_argument (file and stdin disagree)\n'
		failed=$((failed + 1))
	fi

	for bad in "no_such_file.txt" "-x" "tests/test1.txt tests/test2.txt"; do
		# shellcheck disable=SC2086
		"$BIN" $bad > /dev/null 2>&1
		if [ $? -eq 2 ]; then
			passed=$((passed + 1))
		else
			printf 'FAIL cli_usage (%s did not exit 2)\n' "$bad"
			failed=$((failed + 1))
		fi
	done
	printf 'ok   cli_usage\n'
fi

printf '\n%s passed, %s failed\n' "$passed" "$failed"
[ "$failed" -eq 0 ]

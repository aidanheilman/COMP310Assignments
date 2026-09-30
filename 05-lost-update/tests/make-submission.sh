#!/bin/sh
# Builds the zip to upload. Always builds; unfinished work is a warning.
#
# The label comes from submission.json.
# The zip is written with python3 when `zip` is absent.
set -u
name=$(awk -F'"' '/"student"/ {print $4}' submission.json 2>/dev/null)
[ -z "$name" ] && name=unknown
label=$(awk -F'"' '/"label"/ {print $4}' submission.json 2>/dev/null)
[ -z "$label" ] && label=A5
ZIP="comp310-$label-$name.zip"

sh tests/run-tests.sh >/dev/null 2>&1 || \
  echo "warning: make test is not clean - submitting anyway"

rm -rf submission "$ZIP"
mkdir -p submission
for p in README.md REPORT.md Makefile src tests bench submission.json; do
  [ -e "$p" ] && cp -R "$p" submission/
done
rm -f submission/counter submission/counter-* submission/src/config.h

if command -v zip >/dev/null 2>&1; then
  zip -qr "$ZIP" submission
elif command -v python3 >/dev/null 2>&1; then
  echo "  ('zip' is not installed - using python3 instead, same result)"
  python3 - "$ZIP" <<'PYZIP'
import os, sys, zipfile
with zipfile.ZipFile(sys.argv[1], "w", zipfile.ZIP_DEFLATED) as z:
    for root, _dirs, files in os.walk("submission"):
        for name in sorted(files):
            p = os.path.join(root, name)
            z.write(p, p)
PYZIP
else
  echo "error: neither 'zip' nor 'python3' is available - cannot build the archive" >&2
  rm -rf submission
  exit 1
fi

# Only claim success if the archive is really there.
if [ -s "$ZIP" ]; then
  rm -rf submission
  echo "wrote $ZIP"
else
  echo "error: $ZIP was not created - your work is still in submission/" >&2
  exit 1
fi

#!/bin/bash

TMPFILE=$(mktemp)
ranlib "$@" 2> "$TMPFILE"
cat "$TMPFILE" | grep -v 'ranlib: warning for library: .* the table of contents is empty' | grep -v 'ranlib: file: .* has no symbols' 1>&2
rm "$TMPFILE"



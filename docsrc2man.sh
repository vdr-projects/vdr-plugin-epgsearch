#!/bin/sh
#
# Creates the man pages
#
# Needs: pod2man and nroff
#
# Mike Constabel
#
# Version 0.1 - 31.07.2006
#

DOCSRC="doc-src"

if [ ! -s "epgsearch.c" ]; then
	echo "Cannot find epgsearch.c. Call this script from epgsearch source directory."
	exit
fi

VERSION="$(awk -F\" '/VERSION/ {print $2; exit;}' epgsearch.c)"

for LANGUAGE in $(ls "$DOCSRC"/); do

	[ ! -d ""$DOCSRC"/$LANGUAGE" ] && continue
	mkdir -p man/$LANGUAGE
	rm man/$LANGUAGE/* 2>/dev/null

	for i in "$DOCSRC"/$LANGUAGE/*.txt; do
		echo -ne "create man page: ($LANGUAGE) $(basename "$i" ".txt")..."
		name=$(echo "$(basename "$i")" | sed -e 's/\.[0-9]\..*$//')
		sect=$(echo "$i" | sed -e 's/.*\.\([0-9]\)\.txt/\1/')
		pod2man -c "Epgsearch Version $VERSION" -n "$name" --section="$sect" "$i" >"man/$LANGUAGE/$(basename "$i" ".txt")"
		if [ $? -eq 0 ]; then
			echo " done."
		else
			echo " failed."
		fi
	done

	rm "$DOCSRC"/$LANGUAGE/*~ 2>/dev/null
	gzip -f man/$LANGUAGE/*.[0-9]

done

echo

for LANGUAGE in $(ls "$DOCSRC"/); do

	[ ! -d "$DOCSRC/$LANGUAGE" ] && continue
	mkdir -p doc/$LANGUAGE
	rm doc/$LANGUAGE/* 2>/dev/null

	for i in man/$LANGUAGE/*.gz; do
		echo -ne "create doc file from man page: ($LANGUAGE) $(basename "$i")..."
		zcat "$i" | nroff -man - | col -xbp > "doc/$LANGUAGE/$(basename "$i" ".gz").txt"
		echo " done"
	done

done

echo

#EOF

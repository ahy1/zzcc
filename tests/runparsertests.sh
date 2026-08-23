#!/bin/sh

mkdir -p out

for fname in test_*.c; do
	outfname="out/$fname.stdout"
	errfname="out/$fname.stderr"

	echo "Testing parser on $fname"

	echo "" >"$outfname"
	echo "" >"$errfname"

	../build/zzparser <"$fname" 2>>"$errfname" >>"$outfname"
	if test $? != 0; then
		echo " >>> Failed"
		exit
	fi

	echo "" >>"$outfname"
	echo "" >>"$errfname"
done

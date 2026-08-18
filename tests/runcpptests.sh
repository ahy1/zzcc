#!/bin/sh

mkdir -p out

for fname in testcpp_*.c; do
	outfname="out/$fname.stdout"
	errfname="out/$fname.stderr"

	echo "Testing preprocessor on $fname"

	echo "" >"$outfname"
	echo "" >"$errfname"

	../build/zzcpp <"$fname" 2>>"$errfname" >>"$outfname"
	if test $? != 0; then
		echo " >>> Failed"
	fi

	echo "" >>"$outfname"
	echo "" >>"$errfname"
done

#!/bin/sh

mkdir -p out

for fname in testcc_*.c; do
	outfname="out/$fname.stdout"
	errfname="out/$fname.stderr"
	asfname="a.s"

	echo "Testing compiler on $fname"

	echo "" >"$outfname"
	echo "" >"$errfname"

	../build/zzcc <"$fname" 2>>"$errfname" >>"$outfname"
	if test $? != 0; then
		echo " >>> Failed"
	fi

	gcc -o out/a.exe a.s

	out/a.exe
	if test $? != 0; then
		echo " >>> Failed"
	fi

	echo "" >>"$outfname"
	echo "" >>"$errfname"
done

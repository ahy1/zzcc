#!/bin/sh

# Testing C parser

mkdir -p out

for fname in test_*.c; do
	outfname="out/$fname.stdout"
	errfname="out/$fname.stderr"

	echo "Testing parser on $fname"

	echo "" >"$outfname"
	echo "" >"$errfname"

	../zzparser <"$fname" 2>>"$errfname" >>"$outfname"
	if test $? != 0; then
		echo " >>> Failed"
#		break
	fi

	echo "" >>"$outfname"
	echo "" >>"$errfname"
done

# Testing C preprocessor

for fname in testcpp_*.c; do
	outfname="out/$fname.stdout"
	errfname="out/$fname.stderr"

	echo "Testing preprocessor on $fname"

	echo "" >"$outfname"
	echo "" >"$errfname"

	../zzcpp <"$fname" 2>>"$errfname" >>"$outfname"
	if test $? != 0; then
		echo " >>> Failed"
	fi

	echo "" >>"$outfname"
	echo "" >>"$errfname"
done



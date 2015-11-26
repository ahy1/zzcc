#!/bin/sh

# Testing C parser

for fname in test_*.c; do
	echo "Testing parser on $fname"

	echo "" >"$fname.stdout"
	echo "" >"$fname.stderr"

	../zzparser <"$fname" 2>>"$fname.stderr" >>"$fname.stdout"
	if test $? != 0; then
		echo " >>> Failed"
#		break
	fi

	echo "" >>"$fname.stdout"
	echo "" >>"$fname.stderr"
done

# Testing C preprocessor

for fname in testcpp_*.c; do
	echo "Testing preprocessor on $fname"

	echo "" >"$fname.stdout"
	echo "" >"$fname.stderr"

	../zzcpp <"$fname" 2>>"$fname.stderr" >>"$fname.stdout"
	if test $? != 0; then
		echo " >>> Failed"
	fi

	echo "" >>"$fname.stdout"
	echo "" >>"$fname.stderr"
done



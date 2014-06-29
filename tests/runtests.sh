#!/bin/sh

for fname in *.c; do
	echo "Testing $fname"

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


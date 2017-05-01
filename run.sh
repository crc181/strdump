#!/bin/bash

# this is one way to get a list of unknown words from your source files
# this will generate list.txt which contains all unknown words
# set spell to your stdin spell checker
# temp/ will contain unknown words for each source file
# if you aren't sure which source file produced an unknown word, grep in temp/
# once you know the file, run strdump again on that file to see the line number
# you can use list.txt as a baseline to find future issues as they appear

if [ ! "$1" ] ; then
    echo "fatal: need source tree path"
    exit -1
fi

dir=$1
spell="hunspell -l"
out=list.txt

src=`find $dir -name \*.cc -o -name \*.h -o -name \*.c | sort`
pfx=`echo $dir`
len=${#pfx}

rm -rf temp/
mkdir -p temp/

for f in $src ; do
    g=${f:$len}
    g=${g//\//.}

    if [ "${g:0:1}" = "." ] ; then
        g=${g:1}
    fi

    strdump $f | $spell > temp/$g
    [ -s temp/$g ] || rm -f temp/$g
done

cat temp/* | sort -u > $out

#rm -rf temp/
wc -l $out


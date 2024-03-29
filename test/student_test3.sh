#Tests size of filename and shows that filenames over 16 characters are illegal


#!/bin/sh

# make virtual disk
./fs_make.x disk.fs 4096

# create file with filename that exceeds 16 characters
echo test > file123456789101112

./fs_ref.x add disk.fs filefilefilefile >ref.stdout 2>ref.stderr
./test_fs.x add disk.fs filefilefilefile >lib.stdout 2>lib.stderr


REF_STDOUT=$(cat ref.stdout)
REF_STDERR=$(cat ref.stderr)

LIB_STDOUT=$(cat lib.stdout)
LIB_STDERR=$(cat lib.stderr)

#stdout
if [ "$REF_STDOUT" != "$LIB_STDOUT" ]; then
    echo "Stdout outputs do not match."
    diff -u ref.stdout lib.stdout
else
    echo "Stdout outputs match. Good job."
fi

#stderr
if [ "$REF_STDERR" != "$LIB_STDERR" ]; then
    echo "Stderr outputs do not match."
    diff -u ref.stderr lib.stderr
else
    echo "Stderr outputs match. Good job"
fi

# clean eveything
rm disk.fs
rm ref.stdout ref.stderr
rm lib.stdout lib.stderr
rm file123456789101112

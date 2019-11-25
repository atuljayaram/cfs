#Tests removal of a file that doesn't actually exist. Error

#!/bin/sh

# virtual disk
./fs_make.x disk.fs 4096
./fs_ref.x rm disk.fs file1 >ref.stdout 2>ref.stderr
./test_fs.x add disk.fs file1 >lib.stdout 2>lib.stderr


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
    echo "Stderr outputs match. Good job."
fi

# clean everything
rm disk.fs
rm ref.stdout ref.stderr
rm lib.stdout lib.stderr
rm file1

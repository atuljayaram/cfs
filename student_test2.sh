#This tests if everything is ok

#!/bin/sh

#make virutal disk
./fs_make.x disk.fs 4096

#create 30 files
for i in $(seq -w 1 30); do echo this_is_test > file${i}; done

#add in 30 files
for i in $(seq -w 1 30); do ./fs_ref.x add disk.fs file${i}; done

# get the ls of fs_ref.x
./fs_rex.x ls disk.fs >ref.stdout 2>ref.stderr

# get the cat of fs_ref.x
./fs_ref.x cat disk.fs file6 >ref.stdout 2>ref.stderr

# add a 31st file
./test_fs.x add disk.fs file31

# add a 32nd file
./test_fs.x add disk.fs file32

# get the ls of fs_ref.x
./fs_rex.x ls disk.fs >ref.stdout 2>ref.stderr

# get the cat of fs_ref.x
./fs_ref.x cat disk.fs file6 >ref.stdout 2>ref.stderr

# remove every file
for i in $(seq -w 1 32); do ./test_fs.x rm disk.fs file${i}; done

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
for i in $(seq -w 1 6); do rm file${i}; done

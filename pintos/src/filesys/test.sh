#!/bin/bash
cd build
pth="./tests/userprog/$1.result"
if [[ $1 == "multi-oom" ]]; then
    pth="./tests/userprog/no-vm/$1.result"
fi
rm "$pth"
make "$pth"
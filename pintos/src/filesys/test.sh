#!/bin/bash
cd build
pth="./tests/filesys/extended/$1.result"
rm "$pth"
make "$pth"
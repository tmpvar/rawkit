#!/bin/bash
make -j8 install

if [ $? -eq 0 ]; then
  notify-send "success"
else
  notify-send "build failed"
fi

#!/bin/bash

./server 12345 $1 5000 2>&1 | tee run.log

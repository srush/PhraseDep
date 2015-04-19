#!/bin/bash

for f in $(find .); 
do [ -f $f ] && echo $f; 
don
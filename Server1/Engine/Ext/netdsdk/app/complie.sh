#!/bin/sh

g++ -DNDK_DEBUG $1 -g -o ${1%.cpp} -I../include -I../src -L../bin -lnetdkit -lpthread -lrt

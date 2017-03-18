#!/bin/sh
#export CC=/opt/centos/devtoolset-1.1/root/usr/bin/gcc  
#export CPP=/opt/centos/devtoolset-1.1/root/usr/bin/cpp
#export CXX=/opt/centos/devtoolset-1.1/root/usr/bin/c++
export CXX=c++
export CC=gcc

killall autorestart.py
killall test_server.exe
rm -rf gziped_files
rm test_server.exe
rm FullLog.txt
rm nohup.out
$CXX  -std=c++11 -c main.cpp ./proxy/AbuseError.cpp ./proxy/GuestBook.cpp ./proxy/Template.cpp ./proxy/TopSites.cpp ./proxy/MakeUrl.cpp ./proxy/proxy_index.cpp ./proxy/ClientForProxy.cpp StartupInfo.cpp ./proxy/Proxy.cpp ./utils/orm.cpp
$CC -c ./md5/md5.c ./sqlite/sqlite3.c 
$CXX -g -Wall  -ansi -pedantic -o test_server.exe AbuseError.o GuestBook.o Template.o TopSites.o MakeUrl.o proxy_index.o ClientForProxy.o StartupInfo.o Proxy.o  md5.o main.o sqlite3.o orm.o  -ldl -lpthread -lcurl -lcares -lssl -lcrypto -lz
rm *.o
#nohup ./autorestart.py &
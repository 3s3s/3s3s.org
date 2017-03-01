#!/bin/sh
export CC=/opt/centos/devtoolset-1.1/root/usr/bin/gcc  
export CPP=/opt/centos/devtoolset-1.1/root/usr/bin/cpp
export CXX=/opt/centos/devtoolset-1.1/root/usr/bin/c++
killall autorestart.py
killall test_server.exe
rm -rf gziped_files
rm test_server.exe
rm FullLog.txt
rm nohup.out
$CXX -std=c++0x -c  main.cpp ./proxy/AbuseError.cpp ./proxy/GuestBook.cpp ./proxy/Template.cpp ./proxy/TopSites.cpp ./proxy/MakeUrl.cpp ./proxy/proxy_index.cpp ./proxy/ClientForProxy.cpp StartupInfo.cpp ./proxy/Proxy.cpp ./english/index.cpp ./english/RegisterSSP.cpp ./english/CTranslatorSSP.cpp  ./english/Dictionary.cpp ./english/VoiceFile.cpp ./english/ClientForEnglish.cpp ./english/GameEventsSSP.cpp ./admin/AdminSSP.cpp ./admin/orm.cpp
$CC -c md5.c ./english/sqlite/sqlite3.c 
$CXX -g -Wall  -ansi -pedantic -ldl -lpthread -lcurl -lcares -lssl -lcrypto -lz -o test_server.exe AbuseError.o GuestBook.o Template.o TopSites.o MakeUrl.o proxy_index.o ClientForProxy.o StartupInfo.o Proxy.o ClientForEnglish.o RegisterSSP.o CTranslatorSSP.o md5.o main.o Dictionary.o sqlite3.o VoiceFile.o index.o orm.o AdminSSP.o GameEventsSSP.o
rm *.o *.gch ./english/*.gch ./english/sqlite/*.gch 
nohup ./autorestart.py &
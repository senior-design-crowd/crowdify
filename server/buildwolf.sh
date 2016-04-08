#!/bin/sh
g++ -o wolf UDP_Receiver_with_wolfssl.cpp -I/usr/include/mysql -I/usr/include/mysql++ -lm -L /usr/local/lib -lwolfssl -lmysqlpp

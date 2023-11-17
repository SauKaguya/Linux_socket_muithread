#pragma once

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

struct sockaddr_in *createIPv4Address(char *ip, int port);

int createIPv4Socket();

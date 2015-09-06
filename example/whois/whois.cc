// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: whois.cc
// Description: 简单的域名查询,暂时只支持域名的形式，不支持ip形式
//              参考linux whois的实现，相当于它的简化版本
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-09-03 12:03:05



#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <stdbool.h>

#define REFERTO_FORMAT "%% referto: whois -h %255s -p %15s %1021[^\n\r]"


char* whoisinfo = NULL;
size_t whois_size = 0;
const char *tld_serv[] = {
  #include "tld_serv.h"
  NULL, NULL
};

const char*new_gtlds[] = {
  #include "new_gtlds.h"
  NULL
};

int connect_server(const char* ip, int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("new socket");
    exit(0);
  }
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(ip);
  if (connect(fd, (const struct sockaddr*)&server, sizeof(server)) == -1) {
    perror("connect");
    exit(0);
  }
  return fd;
}

int connect_host(const char* host, char* server) {
  struct addrinfo hints, *res, *res0;
  int error;
  int s;
  const char *cause = NULL;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  printf("connect host:%s\n", host);
  error = getaddrinfo(host, server ? server:"nicname", &hints, &res0);
  if (error) {
    errx(1, "getaddrinfo %s", gai_strerror(error));
    /*NOTREACHED*/
  }
  printf("get addrinfo ok\n");
  s = -1;
  for (res = res0; res; res = res->ai_next) {
    s = socket(res->ai_family, res->ai_socktype,
               res->ai_protocol);
    if (s < 0) {
      cause = "socket";
      continue;
    }
    if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
      cause = "connect";
      close(s);
      s = -1;
      continue;
    }
    break;  /* okay we got one */
  }
  if (s < 0) {
    err(1, "connect_host:%s", cause);
    /*NOTREACHED*/
  }
  freeaddrinfo(res0);
  return s;
}


// 查检domain是不是tld的子域名
bool isSubDomain(const char* domain, const char* tld) {
  size_t dom_len = strlen(domain);
  size_t tld_len = strlen(tld);
  if (dom_len == 0 || tld_len == 0 || dom_len < tld_len) {
    return false;
  }
  const char *p = domain + dom_len - tld_len -1;
  if ((*p) != '.') {
    return false;
  }
  return (strcmp(p+1, tld) == 0);
}

const char *is_new_gtld(const char *s) {
  for (int i = 0; new_gtlds[i]; i++)
    if (isSubDomain(s, new_gtlds[i]))
      return new_gtlds[i];
  return 0;
}

const char* is_tld(const char* s) {
  for (int i = 0; tld_serv[i] != NULL; i = i+2) {
    if (isSubDomain(s, tld_serv[i])) {
      return tld_serv[i+1];
    }
  }
  return 0;
}

char* get_query_server(const char* host) {
  const char* server = NULL;
  char* s = reinterpret_cast<char*>(malloc(100));
  if ((server = is_tld(host)) != NULL) {
    snprintf(s, 100, "%s", server);
    return s;
  }
  if ((server = is_new_gtld(host)) != NULL) {
    snprintf(s, 100, "whois.nic.%s", server);
    return s;
  }
  free(s);
  return NULL;
}


char *query_crsnic(int sock, const char *query) {
    char *p, buf[2000];
    FILE *fi;
    char *referral_server = NULL;
    int state = 0;

    char temp[1000] = {'\0'};
    temp[0] = '=';
    snprintf(temp, sizeof(temp), "%s\r\n", query);

    fi = fdopen(sock, "r");
    if (write(sock, temp, strlen(temp)) < 0) {
      perror("query_crsnic write");
    }
    memset(buf, '\0', sizeof(buf));
    while (fgets(buf, sizeof(buf), fi)) {
	/* If there are multiple matches only the server of the first record
	   is queried */
      if (state == 0 && strncmp(buf, "   Domain Name:", 15) == 0)
          state = 1;
      if (state == 1 && (strncmp(buf, "   Whois Server:", 16) == 0
                           || strncmp(buf, "   WHOIS Server:", 16) == 0)) {
          for (p = buf; *p != ':'; p++) {}  /* skip until the colon */
          for (p++; *p == ' '; p++) {}  /* skip the spaces */
          referral_server = strdup(p);
          if ((p = strpbrk(referral_server, "\r\n ")))
            *p = '\0';
          state = 2;
        }
        snprintf(whoisinfo+strlen(whoisinfo),
                 whois_size-strlen(whoisinfo), "%s", buf);
        // fputs(buf, stdout);
        if ((p = strpbrk(buf, "\r\n"))) {
          *p = '\0';
        }
        memset(buf, '\0', sizeof(buf));
    }

    if (ferror(fi))
      perror("query_crsnic fgets");
    fclose(fi);

    return referral_server;
}

// 它和query_crsnic的区别在发送的查询字符串和接收到的格式不同
char *query_afilias(const int sock, const char *query) {
    char *p, buf[2000];
    FILE *fi;
    char *referral_server = NULL;
    int state = 0;
    char temp[1000] = {'\0'};
    snprintf(temp, sizeof(temp), "%s\r\n", query);

    fi = fdopen(sock, "r");
    if (write(sock, temp, strlen(temp)) < 0) {
      perror("query_afilias write");
    }

    memset(buf, '\0', sizeof(buf));
    while (fgets(buf, sizeof(buf), fi)) {
	/* If multiple attributes are returned then use the first result.
	   This is not supposed to happen. */
      if (state == 0 && strncmp(buf, "Domain Name:", 12) == 0)
        state = 1;
      if (state == 1 && strncmp(buf, "Whois Server:", 13) == 0) {
        for (p = buf; *p != ':'; p++) {}  /* skip until colon */
        for (p++; *p == ' '; p++) {}  /* skip colon and spaces */
        referral_server = strdup(p);
        if ((p = strpbrk(referral_server, "\r\n ")))
          *p = '\0';
        state = 2;
      }
      snprintf(whoisinfo+strlen(whoisinfo),
               whois_size-strlen(whoisinfo), "%s", buf);
      // fputs(buf, stdout);
      if ((p = strpbrk(buf, "\r\n")))
        *p = '\0';

      memset(buf, '\0', sizeof(buf));
    }

    if (ferror(fi)) {
      perror("query_afilias fgets");
    }
    fclose(fi);

    return referral_server;
}

char *do_query(const int sock, const char *query) {
  char *p;
  char buf[2000] = {'\0'};
  FILE *fi;
  char *referral_server = NULL;

  char temp[200] = {'\0'};
  snprintf(temp, sizeof(temp), "%s\r\n", query);

  fi = fdopen(sock, "r");
  if (write(sock, temp, strlen(temp)) < 0) {
    perror("do_query write");
  }

  while (fgets(buf, sizeof(buf), fi)) {
    /* 6bone-style referral:
     * % referto: whois -h whois.arin.net -p 43 as 1
       */
      if (!referral_server && strncmp(buf, "% referto:", 10) == 0) {
        char nh[256], np[16], nq[1024];
        if (sscanf(buf, REFERTO_FORMAT, nh, np, nq) == 3) {
          /* XXX we are ignoring the new query string */
          int referral_server_size = strlen(nh) + 1 + strlen(np) + 1;
          referral_server = reinterpret_cast<char*>(
              malloc(referral_server_size));
          snprintf(referral_server, referral_server_size, "%s:%s", nh, np);
        }
      }

      /* ARIN referrals:
       * ReferralServer: rwhois://rwhois.fuse.net:4321/
       * ReferralServer: whois://whois.ripe.net
       */
      if (!referral_server && strncmp(buf, "ReferralServer:", 15) == 0) {
        if ((p = strstr(buf, "rwhois://")))
          referral_server = strdup(p + 9);
        else if ((p = strstr(buf, "whois://")))
          referral_server = strdup(p + 8);
        if (referral_server && (p = strpbrk(referral_server, "/\r\n")))
          *p = '\0';
      }
      snprintf(whoisinfo+strlen(whoisinfo),
               whois_size-strlen(whoisinfo), "%s", buf);
      // fputs(buf, stdout);
      if ((p = strpbrk(buf, "\r\n")))
        *p = '\0';

      memset(buf, '\0', sizeof(buf));
  }

    if (ferror(fi)) {
      perror("do_query fgets");
    }

    fclose(fi);
    return referral_server;
}


void handle_query(const char* server, const char* query) {
  printf("server :%s\n", server);
  char* referral_server = NULL;
  switch (server[0]) {
    case 0:
    case 1:
    case 3:
    case 5:
    case 6:
      printf("no whois server\n");
      return;
    case 4: {  // 返回是包含了Whois Server:信息，要从这里取出真正的server
      printf("first char 4\n");
      int sockfd = connect_host(server+1, NULL);
      referral_server = query_crsnic(sockfd, query);
      server = server + 1;
      printf("get next server:%s\n", referral_server);
      break;
    }
    case 8: {
      int sockfd = connect_host("whois.afilias-grs.info", NULL);
      referral_server = query_afilias(sockfd, query);
      break;
    }
    default:
      break;
  }

  int sockfd = 0;
  if (referral_server != NULL) {
    sockfd = connect_host(referral_server, NULL);
    free(referral_server);
    referral_server = NULL;
  } else {
    sockfd = connect_host(server, NULL);
  }

  printf("do query\n");
  referral_server = do_query(sockfd, query);
  if (referral_server != NULL && !strchr(query, ' ')) {
    printf("\n\nFound a referral to %s.\n\n", referral_server);
    handle_query(referral_server, query);
    free(referral_server);
    referral_server = NULL;
  }
}

int whois(const char* host, char* info, size_t info_size) {
  whoisinfo = NULL;
  if (host== NULL || strlen(host) == 0) {
    printf("input query host\n");
    return -1;
  }
  if (info_size < 100) {
    printf("input info_size too small\n");
    return -1;
  }
  char* server = get_query_server(host);
  if (server != NULL) {
    printf("get whois server: %s\n", server);
    whoisinfo = info;
    whois_size = info_size;
    handle_query(server, host);
    free(server);
    return 0;
  } else {
    printf("can't found whois server for %s\n", host);
  }
  return -1;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("input query host\n");
    return 0;
  }
  char* query = argv[1];
  char info[10000] = {'\0'};
  if (whois(query, info, 10000) == 0) {
    printf("info :%s\n", info);
  }
  return 0;
}









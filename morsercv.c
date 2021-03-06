/* ====================================================================
 * The Morse-over-IP License, Version 1.0
 *
 * Copyright (c) 2002,2017 Hans Liss (Hans@Liss.pp.se).  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by Hans Liss
 *       (Hans@Liss.pp.se)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL HANS LISS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifndef PROTO_NAME
#define PROTO_NAME "morse"
#endif

#define PSIZE 1500
#define PLEN_DASH 300
#define PLEN_DOT 100
#define PLEN_CSPACE 10
#define PLEN_WSPACE 15
#define PLEN_TERM 400

struct xtab_struct {
  char c;
  char *x;
} xtab[]=
  {
    {'"', ".-..-."},
    {'\'', ".----."},
    {'(', "-.--.-"},
    {')', "-.--.-"},
    {',', "--..--"},
    {'-', "-....-"},
    {'.', ".-.-.-"},
    {'/', "-..-."},
    {'&', ".-..."},
    {'=', "-...-"},
    {':', "---..."},
    {';', "-.-.-"},
    {'+', ".-.-."},
    {'_', "..--.-"},
    {'$', "...-..-"},
    {'@', ".--.-."},
    {'0', "-----"},
    {'1', ".----"},
    {'2', "..---"},
    {'3', "...--"},
    {'4', "....-"},
    {'5', "....."},
    {'6', "-...."},
    {'7', "--..."},
    {'8', "---.."},
    {'9', "----."},
    {':', "---..."},
    {'?', "..--.."},
    {'A', ".-"},
    {'B', "-..."},
    {'C', "-.-."},
    {'D', "-.."},
    {'E', "."},
    {'F', "..-."},
    {'G', "--."},
    {'H', "...."},
    {'I', ".."},
    {'J', ".---"},
    {'K', "-.-"},
    {'L', ".-.."},
    {'M', "--"},
    {'N', "-."},
    {'O', "---"},
    {'P', ".--."},
    {'Q', "--.-"},
    {'R', ".-."},
    {'S', "..."},
    {'T', "-"},
    {'U', "..-"},
    {'V', "...-"},
    {'W', ".--"},
    {'X', "-..-"},
    {'Y', "-.--"},
    {'Z', "--.."},
    {'�', ".--.-"},
    {'�', ".-.-"},
    {'�', ".--.-"},
    {'�', "..-.."},
    {'�', "--.--"},
    {'�', "---."},
    {'�', "..--"}
  };

#define NCODES (sizeof(xtab)/sizeof(struct xtab_struct))

struct morse_packet {
  unsigned long seq;
};

/* Translate an ASCII hostname or ip address to a struct in_addr - return 0
   if unable */
int makeaddress(char *name_or_ip, struct in_addr *res) {
  struct hostent *he;
  if (!inet_aton(name_or_ip,res)) {
    if (!(he=gethostbyname(name_or_ip))) {
      return 0;
    } else {
      memcpy(res, he->h_addr_list[0], sizeof(*res));
      return 1;
    }
  } else {
    return 1;
  }
}

// Print a character, given a morse sequence as dots and dashes
void printword(char *mword) {
  int i, printed=0;
  for (i=0; i<NCODES; i++) {
    if (!strcmp(xtab[i].x,mword)) {
      putchar(xtab[i].c);
      printed=1;
      break;
    }
  }
  if (!printed) {
    printf("[%s]", mword);
  }
  mword[0]='\0';
}

int main(int argc, char *argv[])
{
  int sock;
  struct protoent *p;
  char mypacket[PSIZE];
  struct morse_packet *mpack=(struct morse_packet *)(mypacket+20);
  unsigned long seqno=0;
  struct sockaddr_in sender, from;
  fd_set myfdset;
  struct timeval select_timeout;
  int ready;
  unsigned int alen;
  unsigned int len;
  char mword[512];

  // Check parameters and extract sender address
  if (argc<2) {
    fprintf(stderr, "Usage: %s <sender host/ip>\n", argv[0]);
    return -1;
  }
  memset(&sender, 0, sizeof(sender));
  sender.sin_family=AF_INET;
  if (!makeaddress(argv[1], &sender.sin_addr))
    {
      perror(argv[1]);
      return -2;
    }

  // Look up the protocol
  if (!(p=getprotobyname(PROTO_NAME)))
    {
      fprintf(stderr, "Couldn't find the \"%s\" protocol.\n", PROTO_NAME);
      return -1;
    }

  // Open a socket
  if ((sock=socket(AF_INET, SOCK_RAW, p->p_proto))<0)
    {
      perror("socket()");
      return -2;
    }

  // Now just loop and receive packets
  ready=0;
  mword[0]='\0';
  while (!ready) {
    select_timeout.tv_sec=1;
    select_timeout.tv_usec=0;
    FD_ZERO(&myfdset);
    FD_SET(sock,&myfdset);
    if (select(sock+1, &myfdset, NULL, NULL, &select_timeout)) {
      alen=sizeof(from);
      len=recvfrom(sock, mypacket, sizeof(mypacket), 0, (struct sockaddr *)&from, &alen);
      // Verify that the sender is the expected one
      if (!memcmp(&(from.sin_addr), &(sender.sin_addr), sizeof(from.sin_addr))) {
	// Check the sequence number
	if (ntohl(mpack->seq)==seqno++) {
	  // An interesting improvement: Remove the hard-coded packet lengths on the
	  // receiver side, and instead collect enough lengths until you can determine
	  // how long a dot and a dash are
	  switch (len - 20) {
	  case PLEN_DASH:
	    strcat(mword, "-");
	    break;
	  case PLEN_DOT:
	    strcat(mword, ".");
	    break;
	  case PLEN_WSPACE:
	    printword(mword);
	    putchar(' ');
	    break;
	  case PLEN_CSPACE:
	    printword(mword);
	    break;
	  case PLEN_TERM:
	    printword(mword);
	    putchar('\n');
	    seqno=0;
	    // if you want the receiver to terminate after one message, uncomment this
	    // ready=1;
	    break;
	  default:
	    printf("len=%d ", len);
	    break;
	  }
	  fflush(stdout);
	} else {
	  if (seqno!=0) {
	    printf("Packet out of order: %ld != %ld\n", (unsigned long)ntohl(mpack->seq), seqno-1);
	  }
	  seqno=0;
	}
      } else {
	printf("Wrong sender %s != %s\n", inet_ntoa(from.sin_addr), 
	       inet_ntoa(sender.sin_addr));
      }
    }
  }
  close(sock);
  return 0;
}

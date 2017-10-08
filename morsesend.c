/* ====================================================================
 * The Morse-over-IP License, Version 1.0
 *
 * Copyright (c) 2002 Hans Liss (http://Hans.Liss.pp.se).  All rights
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
 *       "This product includes software developed by the
 *        Hans Liss (http://hans.liss.pp.se)."
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

#define PSIZE 32
#define PLEN_DASH 300
#define PLEN_DOT 100
#define PLEN_CSPACE 10
#define PLEN_WSPACE 15
#define PLEN_TERM 400

struct xtab_struct
{
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
    {'Á', ".--.-"},
    {'Ä', ".-.-"},
    {'Å', ".--.-"},
    {'É', "..-.."},
    {'Ñ', "--.--"},
    {'Ö', "---."},
    {'Ü', "..--"}
  };

#define NCODES (sizeof(xtab)/sizeof(struct xtab_struct))

struct morse_packet
{
  unsigned long seq;
};

char mytoupper(char c)
{
  char *p;
  if ((p=strchr("áäåéñöüÁÄÅÉÑÖÜ", c))!=NULL)
    return *(p+7);
  else
    return toupper(c);
}

/* Translate an ASCII hostname or ip address to a struct in_addr - return 0
   if unable */
int makeaddress(char *name_or_ip, struct in_addr *res)
{
  struct hostent *he;
  if (!inet_aton(name_or_ip,res))
    {
      if (!(he=gethostbyname(name_or_ip)))
	return 0;
      else
	{
	  memcpy(res, he->h_addr_list[0], sizeof(*res));
	  return 1;
	}
    }
  else
    return 1;
}

int main(int argc, char *argv[])
{
  int sock;
  struct protoent *p;
  char mypacket[PSIZE];
  struct morse_packet *mpack=(struct morse_packet *)mypacket;
  unsigned long seqno=0;
  int i, j, k;
  char *mcode;
  struct sockaddr_in recipient;

  if (argc<2)
    {
      fprintf(stderr, "Usage: %s <rcpt> <text> ...\n", argv[0]);
      return -1;
    }
  memset(&recipient, 0, sizeof(recipient));
  recipient.sin_family=AF_INET;
  if (!makeaddress(argv[1], &recipient.sin_addr))
    {
      perror(argv[1]);
      return -2;
    }
  if (!(p=getprotobyname("morse")))
    {
      perror("morse");
      return -1;
    }
  if ((sock=socket(AF_INET, SOCK_RAW, p->p_proto))<0)
    {
      perror("socket()");
      return -2;
    }
  for (i=2; i<argc; i++)
    {
      for (j=0; j<strlen(argv[i]); j++)
	{
	  mcode=NULL;
	  for (k=0; k<NCODES; k++)
	    if (xtab[k].c==mytoupper(argv[i][j]))
	      mcode=xtab[k].x;
	  if (!mcode)
	    printf("Illegal character %c\n", argv[i][j]);
	  else
	    {
	      for (k=0; k<strlen(mcode); k++)
		{
		  memset(mypacket, 0, sizeof(mypacket));
		  mpack->seq=htonl(seqno++);
		  if (sendto(sock, mypacket, ((mcode[k]=='-')?PLEN_DASH:PLEN_DOT), 0,
			     (struct sockaddr *)&recipient, sizeof(recipient))<0)
		    perror("sendto()");
		}
	    }
	  if (j<(strlen(argv[i])-1))
	    {
	      memset(mypacket, 0, sizeof(mypacket));
	      mpack->seq=htonl(seqno++);
	      if (sendto(sock, mypacket, PLEN_CSPACE, 0,
			 (struct sockaddr *)&recipient, sizeof(recipient))<0)
		perror("sendto()");
	    }
	}
      if (i < (argc-1))
	{
	  memset(mypacket, 0, sizeof(mypacket));
	  mpack->seq=htonl(seqno++);
	  if (sendto(sock, mypacket, PLEN_WSPACE, 0,
		     (struct sockaddr *)&recipient, sizeof(recipient))<0)
	    perror("sendto()");
	}
    }
  memset(mypacket, 0, sizeof(mypacket));
  mpack->seq=htonl(seqno++);
  if (sendto(sock, mypacket, PLEN_TERM, 0,
	     (struct sockaddr *)&recipient, sizeof(recipient))<0)
    perror("sendto()");
  close(sock);
  return 0;
}

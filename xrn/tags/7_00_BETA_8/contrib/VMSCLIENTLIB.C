/*
 * grabbed this file from TGV.COM [ricks]
 */

/*
Message 145 -- ************************
15-Jan-91 12:14:58-PST,18867;000000000000
Return-Path: <MRL@ALCVAX.PFC.MIT.EDU>
Received: from ALCVAX.PFC.MIT.EDU by TGV.COM with INTERNET ;
          Tue, 15 Jan 91 12:14:36 PST
Message-id: <D3FBF1CDCFCBE00977@ALCVAX.PFC.MIT.EDU>
Date: Tue, 15 Jan 91 15:14 EST
From: MRL@ALCVAX.PFC.MIT.EDU
Subject: Re: Clientlib
To: MCMAHON@TGV.COM
X-Envelope-to: MCMAHON@TGV.COM
X-VMS-To: IN%"MCMAHON@TGV.COM"

I think the following is a correctly modified version.  I'm compiling now,
and I'll let you know if I have problems.
----------------------------------------------------------------------------
*/

#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/contrib/VMSCLIENTLIB.C,v 1.1 1994-10-10 08:38:36 jik Exp $";
#endif

/*
 * nntp client interface
 *
 * Copyright (c) 1988, 1989, 1990, The Regents of the University of California.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of California not
 * be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The University
 * of California makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF CALIFORNIA DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef VMS
#include <X11/Xos.h>
#else
#define index strchr
/*
 * Define UCX if you want UCX TCP/IP transport for VMS.
 * Define CMU_TCP for CMU TCP/IP transport. (Code supplied by Mike Iglesias,
				             iglesias@draco.acs.uci.edu)
 * Define FUSION for Fusion TCP/IP. (Code Supplied by Ken Robinson,
				     robinson@cp0201.bellcore.com)
 * Define WOLLONGONG for WIN/TCP.
 * Define MULTINET for MultiNet TCP/IP. (Code Supplied by John McMahon,
					 mcmahon@tgv.com)
 */


/* JJM (1 Line) */
#define MULTINET
/* JJM (1 Line) */
#if defined(UCX) || defined(CMU_TCP) || defined(FUSION) || defined (WOLLONGONG) || defined (MULTINET)
#define VMS_TCPIP
#else
#define VMS_DECNET
#endif 

/* JJM (Block) */
#ifdef MULTINET
#define BSD_BFUNCS
#include "MultiNet_Common_Root:[MultiNet.Include.Sys]types.h"
#include "MultiNet_Common_Root:[MultiNet.Include.Sys]socket.h"
#include "MultiNet_Common_Root:[MultiNet.Include.NetInet]in.h"
#include "MultiNet_Common_Root:[MultiNet.Include]netdb.h"
/* the following defines cure conflicts with files included
   with decw$include/xos.h */
#define CADDR_T
#define __TYPES_LOADED 1
#endif /* MULTINET */

#include <decw$include/Xos.h>

#define NNTP_PORT_NUMBER 119
#define index strchr

#ifdef CMU_TCP
#include <descrip.h>
#include <ssdef.h>
#include <iodef.h>
#define TCP$OPEN IO$_CREATE
#define TCP$READ IO$_READVBLK
#define TCP$SEND IO$_WRITEVBLK
#define TCP$CLISE IO$_DELETE
#endif /* CMU_TCP */

#if defined(VMS_UCX) || defined(WOLLONGONG)
#include <socket.h>
#include <in.h>
#include <netdb.h>
#endif /* VMS_UCX or WOLLONGONG */

#ifdef FUSION
#include <fns_base:[h]flip.h>
#include <fns_base:[h]socket.h>
#include <fns_base:[h]in.h>
#include <fns_base:[h]netdb.h>
#endif /* FUSION */

#ifdef VMS_DECNET
#include <unixio.h>
#include <file.h>
#define NNTPobject "::\"0=NNTP\""
#endif /* VMS_DECNET */

#endif /* VMS */
#include <stdio.h>

#ifndef VMS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif /* VMS */

#ifdef DECNET
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif

#include "codes.h"
#include "config.h"
#include "utils.h"

void close_server();
#ifdef BSD_BFUNCS
#define memset(_Str_, _Chr_, _Len_) bzero(_Str_, _Len_)
#define memcpy(_To_, _From_, _Len_) bcopy(_From_, _To_, _Len_)
#endif

static FILE	*ser_rd_fp = NULL;
static FILE	*ser_wr_fp = NULL;
static Boolean	ServerOpen = False;
static int	sockt_rd, sockt_wr;

#ifdef CMU_TCP
static unsigned short cmu_chan;
$DESCRIPTOR (cmu_ip, "IP0");
int status;
struct iosb_str {
    short stat;     /* status */
    short count;    /* Byte count */
    int	  info;	    /* additional info */
};
struct iosb_str iosb;
#endif /* CMU_TCP */
/*
 * getserverbyfile	Get the name of a server from a named file.
 *			Handle white space and comments.
 *			Use NNTPSERVER environment variable if set.
 *
 *	Parameters:	"file" is the name of the file to read.
 *
 *	Returns:	Pointer to static data area containing the
 *			first non-ws/comment line in the file.
 *			NULL on error (or lack of entry in file).
 *
 *	Side effects:	None.
 */

char *
getserverbyfile(file)
char	*file;
{
    char *cp;
    extern char *getenv(), *getinfofromfile();
    static char	buf[256];

    if (cp = getenv("NNTPSERVER")) {
	(void) strcpy(buf, cp);
    } else {
	cp = getinfofromfile(file);
	if (cp == NULL) {
	    return(NULL);
	} else {
	    (void) strcpy(buf, cp);
	}
    }
    return (buf);
}

/*
 * server_init  Get a connection to the remote news server.
 *
 *	Parameters:	"machine" is the machine to connect to.
 *
 *	Returns:	-1 on error
 *			server's initial response code on success.
 *
 *	Side effects:	Connects to server.
 *			"ser_rd_fp" and "ser_wr_fp" are fp's
 *			for reading and writing to server.
 */

server_init(machine)
char	*machine;
{
	char	line[256];
#ifdef VMS_DECNET
	static  char *in_buffer = NULL, *out_buffer = NULL;

	close_server();				/* make sure it's closed */
	(void) close(sockt_rd);
	(void) close(sockt_wr);
	sockt_rd = get_dnet_socket(machine);
#else /* not VMS_DECNET */

#ifdef DECNET
	char	*cp;

	cp = index(machine, ':');

	if (cp && cp[1] == ':') {
		*cp = '\0';
		sockt_rd = get_dnet_socket(machine);
		*cp = ':';
	} else
		sockt_rd = get_tcp_socket(machine);
#else  /* DECNET */
#ifndef CMU_TCP
	sockt_rd = get_tcp_socket(machine);
#else
	sockt_rd = get_cmutcp_socket(machine);
#endif /* CMU_TCP */
#endif /* DECNET */
#endif /* VMS_DECNET */

	if (sockt_rd < 0)
		return (-1);

	/*
	 * Now we'll make file pointers (i.e., buffered I/O) out of
	 * the socket file descriptor.  Note that we can't just
	 * open a fp for reading and writing -- we have to open
	 * up two separate fp's, one for reading, one for writing.
	 */

#ifndef VMS_TCPIP
#ifndef VMS_DECNET
	if ((ser_rd_fp = fdopen(sockt_rd, "r")) == NULL) {
		perror("server_init: fdopen #1");
		return (-1);
	}

	sockt_wr = dup(sockt_rd);
	if ((ser_wr_fp = fdopen(sockt_wr, "w")) == NULL) {
		perror("server_init: fdopen #2");
		ser_rd_fp = NULL;		/* from above */
		return (-1);
	}
#else /* VMS_DECNET */
	if (in_buffer == NULL) {
	    if ((in_buffer = XtMalloc(8192)) == NULL) {
	        perror("in buffer malloc failed.");
	        return (-1);
	    }
	}
	if ((ser_rd_fp = fdopen(sockt_rd, "r")) == NULL) {
		perror("server_init: fdopen #1");
		return (-1);
	}
	if (setvbuf(ser_rd_fp, in_buffer, _IOLBF, 8192)) {
	    perror("setvbuf");
	    return (-1);
	}
	if (out_buffer == NULL) {
	    if ((out_buffer = XtMalloc(8192)) == NULL) {
	        perror("out buffer malloc failed.");
	        return (-1);
	    }
	}
	if ((ser_wr_fp = fdopen(sockt_rd, "w")) == NULL) {
		perror("server_init: fdopen #2");
		ser_rd_fp = NULL;		/* from above */
		return (-1);
	}
	if (setvbuf(ser_wr_fp, out_buffer, _IOLBF, 8192)) {
	    perror("setvbuf");
	    return (-1);
	}
#endif /* VMS_DECNET */
#endif /* VMS_TCPIP */

	ServerOpen = True;
	/* Now get the server's signon message */

	(void) get_server(line, sizeof(line));
	return (atoi(line));
}

#ifndef VMS_DECNET
#ifndef CMU_TCP
/*
 * get_tcp_socket -- get us a socket connected to the news server.
 *
 *	Parameters:	"machine" is the machine the server is running on.
 *
 *	Returns:	Socket connected to the news server if
 *			all is ok, else -1 on error.
 *
 *	Side effects:	Connects to server.
 *
 *	Errors:		Printed via perror.
 */

get_tcp_socket(machine)
char	*machine;
{
	int	s;
#ifdef h_addr
	int x = 0;
	register char **cp;
#endif /* h_addr */

#ifdef FUSION
#define gethostbyname ghbyname
#endif
	struct	sockaddr_in sin;
	struct	servent *getservbyname(), *sp;
	struct	hostent *gethostbyname(), *hp;

	if ((hp = gethostbyname(machine)) == NULL) {
		(void) fprintf(stderr, "%s: Unknown host.\n", machine);
		return (-1);
	}

#ifndef VMS
	if ((sp = getservbyname("nntp", "tcp")) ==  NULL) {
		(void) fprintf(stderr, "nntp/tcp: Unknown service.\n");
		return (-1);
	}
#endif

	(void) memset((char *) &sin, 0, sizeof(sin));
	sin.sin_family = hp->h_addrtype;
#ifndef VMS
	sin.sin_port = sp->s_port;
#else
	sin.sin_port = htons(NNTP_PORT_NUMBER);
#endif

	/*
	 * The following is kinda gross.  The name server under 4.3
	 * returns a list of addresses, each of which should be tried
	 * in turn if the previous one fails.  However, 4.2 hostent
	 * structure doesn't have this list of addresses.
	 * Under 4.3, h_addr is a #define to h_addr_list[0].
	 * We use this to figure out whether to include the NS specific
	 * code...
	 */

#if defined(h_addr)
	/* get a socket and initiate connection -- use multiple addresses */

	for (cp = hp->h_addr_list; cp && *cp; cp++) {
		s = socket(hp->h_addrtype, SOCK_STREAM, 0);
		if (s < 0) {
			(void) perror("socket");
			return (-1);
		}
		(void) memcpy((char *) &sin.sin_addr, *cp, hp->h_length);
		
		if (x < 0) {
		    (void) fprintf(stderr, "trying %s\n", inet_ntoa(sin.sin_addr));
		}
		x = connect(s, (struct sockaddr *)&sin, sizeof (sin));
		if (x == 0)
			break;
                (void) fprintf(stderr, "connection to %s: ", inet_ntoa(sin.sin_addr));
		(void) perror("");
		(void) close(s);
	}
	if (x < 0) {
		(void) fprintf(stderr, "giving up...\n");
		return (-1);
	}
#else	/* no name server */

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) { /* Get the socket */
		(void) perror("socket");
		return (-1);
	}

	/* And then connect */

	(void) memcpy((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
	if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		(void) perror("connect");
		(void) close(s);
		return (-1);
	}

#endif
	ServerOpen = True;
	return (s);
}

#else /* CMU_TCP */
/*
 * get_cmutcp_socket -- get us a CMU TCP/IP socket connected to the news server
 *
 *	Paremeters:	"machine" is the machine the server is running on.
 *
 *	Returns:	Socket connected to the news server if
 *			all is ok, else -1 on error.
 *
 *	Side effects:	Connects to server.
 *
 *	Errors:		Printed vua perror.
 */
get_cmutcp_socket (machine)
char *machine;
{
    int stat;

    /* first, open a channel to the IP0 device */
    if (((stat = SYS$ASSIGN(&cmu_ip, &cmu_chan, 0, 0)) & 1) != 1)
	return (-1);

    /* now connect to the server */

    stat = SYS$QIOW (0, cmu_chan, TCP$OPEN, &iosb, 0, 0, machine,
		     NNTP_PORT_NUMBER, 0, 1, 0, 0);
    if ((stat & 1) != 1) return (-1);
    if (iosb.stat == SS$_ABORT) return (-1);
    return (1);
}
#endif /* CMU_TCP */
#endif /* VMS_DECNET */

#if defined(DECNET) || defined(VMS_DECNET)
/*
 * get_dnet_socket -- get us a socket connected to the news server.
 *
 *	Parameters:	"machine" is the machine the server is running on.
 *
 *	Returns:	Socket connected to the news server if
 *			all is ok, else -1 on error.
 *
 *	Side effects:	Connects to server.
 *
 *	Errors:		Printed via nerror.
 */

get_dnet_socket(machine)
char	*machine;
{
#ifdef VMS
	char *connect;
	int	s,colon;

	colon = ((char *) colon=index(machine, ':')) ? ((char *)colon)-machine :
		strlen(machine);

	connect = ARRAYALLOC(char, ((int) colon+sizeof NNTPobject+1));
	(void) strncpy(connect, machine, colon);
	(void) strcpy(&connect[colon], NNTPobject);
	
	if ((s = open(connect, O_RDWR, 0)) == -1) {
	    perror("open");
	    FREE(connect);
	    return (-1);
	}

	FREE(connect);

#else /* Not VMS */

	int	s, area, node;
	struct	sockaddr_dn sdn;
	struct	nodeent *getnodebyname(), *np;

	(void) memset((char *) &sdn, 0, sizeof(sdn));

	switch (s = sscanf( machine, "%d%*[.]%d", &area, &node )) {
		case 1: 
			node = area;
			area = 0;
		case 2: 
			node += area*1024;
			sdn.sdn_add.a_len = 2;
			sdn.sdn_family = AF_DECnet;
			sdn.sdn_add.a_addr[0] = node % 256;
			sdn.sdn_add.a_addr[1] = node / 256;
			break;
		default:
			if ((np = getnodebyname(machine)) == NULL) {
			    (void) fprintf(stderr, 
				    "%s: Unknown host.\n", machine);
			    return (-1);
			} else {
			    (void) memcpy((char *) sdn.sdn_add.a_addr, 
					  np->n_addr, 
					  np->n_length);
			    sdn.sdn_add.a_len = np->n_length;
			    sdn.sdn_family = np->n_addrtype;
			}
			break;
	}
	sdn.sdn_objnum = 0;
	sdn.sdn_flags = 0;
	sdn.sdn_objnamel = strlen("NNTP");
	(void) memcpy(&sdn.sdn_objname[0], "NNTP", sdn.sdn_objnamel);

	if ((s = socket(AF_DECnet, SOCK_STREAM, 0)) < 0) {
		nerror("socket");
		return (-1);
	}

	/* And then connect */

	if (connect(s, (struct sockaddr *) &sdn, sizeof(sdn)) < 0) {
		nerror("connect");
		close(s);
		return (-1);
	}
#endif /* VMS */

	ServerOpen = True;
	return (s);
}
#endif /* DECNET or VMS */

/*
 * handle_server_response
 *
 *	Print some informative messages based on the server's initial
 *	response code.  This is here so inews, rn, etc. can share
 *	the code.
 *
 *	Parameters:	"response" is the response code which the
 *			server sent us, presumably from "server_init",
 *			above.
 *			"server" is the news server we got the
 *			response code from.
 *
 *	Returns:	-1 if the error is fatal (and we should exit).
 *			0 otherwise.
 *
 *	Side effects:	None.
 */

handle_server_response(response, server)
int	response;
char	*server;
{
    switch (response) {
	case OK_NOPOST:		/* fall through */
    		printf(
	"NOTE: This machine does not have permission to post articles.\n");
		printf(
	"      Please don't waste your time trying.\n\n");

	case OK_CANPOST:
		return (0);

	case ERR_ACCESS:
		printf(
   "This machine does not have permission to use the %s news server.\n",
		server);
		return (-1);

	default:
		printf("Unexpected response code from %s news server: %d\n",
			server, response);
		return (-1);
    }
	/*NOTREACHED*/
}


/*
 * put_server -- send a line of text to the server, terminating it
 * with CR and LF, as per ARPA standard.
 *
 *	Parameters:	"string" is the string to be sent to the
 *			server.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Talks to the server.
 *
 *	Note:		This routine flushes the buffer each time
 *			it is called.  For large transmissions
 *			(i.e., posting news) don't use it.  Instead,
 *			do the fprintf's yourself, and then a final
 *			fflush.
 */

void
put_server(string)
char *string;
{
#ifdef CMU_TCP
    static char crlf[] = "\r\n";
#endif /* CMU_TCP */
	if (!ServerOpen)
		start_server(NIL(char));
#ifdef DEBUG
	(void) fprintf(stderr, ">>> %s\n", string);
#endif
#ifndef VMS_TCPIP
	(void) fprintf(ser_wr_fp, "%s\r\n", string);
	(void) fflush(ser_wr_fp);
#else
#ifndef CMU_TCP
	(void) send(sockt_rd, string, strlen(string),0);	
	(void) send(sockt_rd, "\r\n", 2, 0);
#else
	status = SYS$QIOW (0, cmu_chan, TCP$SEND, &iosb, 0, 0,
			   string, strlen(string), 0, 0, 0, 0);
	status = SYS$QIOW (0, cmu_chan, TCP$SEND, &iosb, 0, 0,
			   crlf, 2, 0, 0, 0, 0);
#endif /* CMU_TCP */
#endif
}


/*
 * get_server -- get a line of text from the server.  Strips
 * CR's and LF's.
 *
 *	Parameters:	"string" has the buffer space for the
 *			line received.
 *			"size" is the size of the buffer.
 *
 *	Returns:	-1 on error, 0 otherwise.
 *
 *	Side effects:	Talks to server, changes contents of "string".
 */

get_server(string, size)
char	*string;
int	size;
{
	register char *cp;
#ifdef VMS_TCPIP
	static char netBuf[1025];
	static int  inBuf = 0;
	static char *bufPtr;
	static int  level = 0;
	int ret;
#endif

	if (!ServerOpen)
		start_server(NIL(char));

#ifndef VMS_TCPIP
	if (fgets(string, size, ser_rd_fp) == NULL)
		return (-1);
	if ((cp = index(string, '\r')) != NULL)
		*cp = '\0';
	else if ((cp = index(string, '\n')) != NULL)
		*cp = '\0';
#else
	if (level == 0)
	    *string = '\0';
	if (inBuf <= 0) {
#ifndef CMU_TCP
	    if ((inBuf = recv(sockt_rd, netBuf, sizeof(netBuf)-1,0)) == -1) 
		return (-1);
#else
	    status = SYS$QIOW (0, cmu_chan, TCP$READ, &iosb, 0, 0,
			       &netBuf, sizeof(netBuf)-1, 0, 0, 0, 0);
	    if ((status & 1) ! = 1) return (-1);
	    if (iosb.stat == SS$_ABORT) return (-1);
	    inBuf = iosb.count;
#endif	/* CMU_TCP */
	
	    netBuf[inBuf] = '\0';
	    bufPtr = netBuf;
	    if (*bufPtr == '\n')
		bufPtr++;
	}
	if ((cp = index(bufPtr, '\r')) != NULL) {
	    *cp = '\0';
	    strcat(string, bufPtr);
	    bufPtr = ++cp;
	    if (*bufPtr == '\n')
		bufPtr++;
	} else {
	    if ((cp = index(bufPtr, '\n')) != NULL) {
		*cp = '\0';
		strcat(string, bufPtr);
		bufPtr = ++cp;
		if (*bufPtr == '\n')
		    bufPtr++;
	    } else {
		level = 1;
		inBuf = 0;
		strcat(string, bufPtr);
		ret = get_server(string, size);
		level = 0;
		return ret;
	    }
	}
#endif

#ifdef DEBUG
	(void) fprintf(stderr, "<<< %s\n", string);
#endif

	return (0);
}


/*
 * close_server -- close the connection to the server, after sending
 *		the "quit" command.
 *
 *	Parameters:	None.
 *
 *	Returns:	Nothing.
 *
 *	Side effects:	Closes the connection with the server.
 *			You can't use "put_server" or "get_server"
 *			after this routine is called.
 */

void
close_server()
{
	char	ser_line[256];

#ifndef VMS_TCPIP
	if (!ServerOpen || ser_wr_fp == NULL || ser_rd_fp == NULL)
		return;
#else
	if (!ServerOpen || sockt_rd < 0)
		return;
#endif

	put_server("QUIT");
	(void) get_server(ser_line, sizeof(ser_line));

#ifndef VMS_TCPIP
	(void) fclose(ser_wr_fp);
	(void) fclose(ser_rd_fp);
#else
#ifndef CMU_TCP
        sys$dassgn(sockt_rd);
#else
	status = SYS$QIOW (0, cmu_chan, TCP$CLOSE, 0, 0, 0, 2, 0, 0, 0, 0, 0);
#endif /* CMU_TCP */
#endif /* VMS_TCPIP */
	ServerOpen = False;
}


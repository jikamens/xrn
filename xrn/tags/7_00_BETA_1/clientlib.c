
#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/clientlib.c,v 1.3 1994-10-10 18:46:30 jik Exp $";
#endif

/* #define DEBUG */

/*
 * nntp client interface
 *
 * Copyright (c) 1988-1993, The Regents of the University of California.
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

#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#ifndef VMS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <unixio.h>
#include <file.h>
#define NNTPobject "::\"0=NNTP\""
#define index strchr
#endif /* VMS */

#ifdef notdef
#if defined(AF_DECnet) && defined(ultrix)
# ifndef DECNET
#  define DECNET
# endif
#endif
#endif /* notdef */

#ifdef DECNET
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif

#include "codes.h"
#include "news.h"
#include "server.h"
#include "internals.h"
#include "clientlib.h"

#ifdef BSD_BFUNCS
#define memset(_Str_, _Chr_, _Len_) bzero(_Str_, _Len_)
#define memcpy(_To_, _From_, _Len_) bcopy(_From_, _To_, _Len_)
#endif

FILE	*ser_rd_fp = NULL;
FILE	*ser_wr_fp = NULL;

char	*server_init_msg = NULL;

static int get_dnet_socket _ARGUMENTS((char *));
static int get_tcp_socket _ARGUMENTS((char *));

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

char * getserverbyfile(file)
    char *file;
{
    char *cp;
    static char	buf[256];

    if ((cp = getenv("NNTPSERVER")) != 0) {
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

int server_init(machine)
    char *machine;
{
	char	line[256];
 	int	server_length;
#ifdef NNTPVIATIP
	ser_rd_fp = fdopen(3, "r"); /* 3 is dictated by tip(1) ~C command */
	ser_wr_fp = fdopen(4, "r"); /* 4 is dictated by tip(1) ~C command */
	(void) sprintf(line, "telnet %s %d", machine, 119);
	put_server(line);
	get_server(line, sizeof(line)); /* Trying ... */
	get_server(line, sizeof(line)); /* Connected to ... */
	get_server(line, sizeof(line)); /* Escape character is ... */
#else /* NNTPVIATIP */
	int	sockt_rd, sockt_wr;
#ifdef VMS
	char	*in_buffer, *out_buffer;

	sockt_rd = get_dnet_socket(machine);
#else
#ifdef DECNET
	char	*cp;
#endif /* DECNET */

#ifdef DECNET
	cp = index(machine, ':');

	if (cp && cp[1] == ':') {
		*cp = '\0';
		sockt_rd = get_dnet_socket(machine);
	} else
		sockt_rd = get_tcp_socket(machine);
#else  /* DECNET */
	sockt_rd = get_tcp_socket(machine);
#endif /* DECNET */
#endif /* VMS */

	if (sockt_rd < 0)
		return (-1);

	/*
	 * Now we'll make file pointers (i.e., buffered I/O) out of
	 * the socket file descriptor.  Note that we can't just
	 * open a fp for reading and writing -- we have to open
	 * up two separate fp's, one for reading, one for writing.
	 */

#ifndef VMS
	if ((ser_rd_fp = fdopen(sockt_rd, "r")) == NULL) {
		perror("server_init: fdopen #1");
		return (-1);
	}

	sockt_wr = dup(sockt_rd);
	if ((ser_wr_fp = fdopen(sockt_wr, "w")) == NULL) {
		perror("server_init: fdopen #2");
		(void) fclose(ser_rd_fp);
		ser_rd_fp = NULL;		/* from above */
		return (-1);
	}
#else
	if ((in_buffer = malloc(8192)) == NULL) {
	    perror("malloc failed.");
	    return (-1);
	}
	if ((ser_rd_fp = fdopen(sockt_rd, "r")) == NULL) {
		perror("server_init: fdopen #1");
		return (-1);
	}
	if (setvbuf(ser_rd_fp, in_buffer, _IOLBF, 8192)) {
	    perror("setvbuf");
	    (void) fclose(ser_rd_fp);
	    ser_rd_fp = NULL;
	    return (-1);
	}
	sockt_wr = dup(sockt_rd);
	if ((out_buffer = malloc(8192)) == NULL) {
	    perror("malloc failed.");
	    (void) fclose(ser_rd_fp);
	    ser_rd_fp = NULL;
	    return (-1);
	}
	if ((ser_wr_fp = fdopen(sockt_wr, "w")) == NULL) {
		perror("server_init: fdopen #2");
		(void) fclose(ser_rd_fp);
		ser_rd_fp = NULL;		/* from above */
		return (-1);
	}
	if (setvbuf(ser_wr_fp, out_buffer, _IOLBF, 8192)) {
	    perror("setvbuf");
	    close_server();
	    (void) fclose(ser_rd_fp);
	    (void) fclose(ser_wr_fp);
	    ser_rd_fp = ser_wr_fp = NULL;
	    return (-1);
	}
#endif
#endif /* NNTPVIATIP */

	/* Now get the server's signon message */

	if (server_init_msg)
		free(server_init_msg);
	if (! (server_init_msg = malloc(1024))) {
	     perror("malloc failed.");
	     (void) fclose(ser_rd_fp);
	     (void) fclose(ser_wr_fp);
	     ser_rd_fp = ser_wr_fp = NULL;
	     return (-1);
	}
	*server_init_msg = '\0';
	server_length = 1024;
	do {
		if (get_server(line, sizeof(line)) == -1) {
			if (feof(ser_rd_fp)) {
				fprintf(stderr, "Unexpected EOF on NNTP server socket -- server is probably malfunctioning.\n");
			}
			else {
				perror("reading from server");
			}
			(void) fclose(ser_rd_fp);
			(void) fclose(ser_wr_fp);
			ser_rd_fp = ser_wr_fp = NULL;
			return (-1);
		}
		while (strlen(line)+strlen(server_init_msg)+1 >= (unsigned) server_length) {
			server_length += 1024;
			if (! (server_init_msg = realloc(server_init_msg,
							 server_length))) {
				(void) fclose(ser_rd_fp);
				(void) fclose(ser_wr_fp);
				ser_rd_fp = ser_wr_fp = NULL;
				perror("realloc failed.");
				return (-1);
			}
		}
		strcat(server_init_msg, line);
		strcat(server_init_msg, "\n");
	} while (line[3] == '-');
		
	return (atoi(line));
}

#ifndef VMS
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
 *
 *  This is a rewrite of the original get_tcp_socket.  This rewrite was
 *  supplied by Casey Leedom <casey@gauss.llnl.gov>
 */
static int get_tcp_socket1 _ARGUMENTS((struct sockaddr_in *));

static int get_tcp_socket(machine)
    char *machine;
{
	int	s;
#ifdef h_addr
	register char **cp;
#endif /* h_addr */
	struct	sockaddr_in sin;
#ifdef SOLARIS
	struct servent *getservbyname _ARGUMENTS((char *, char *));
#else
	struct servent *getservbyname _ARGUMENTS((const char *, const char *));
#endif
        struct servent *sp;
	struct hostent *gethostbyname _ARGUMENTS((const char *)), *hp;

	(void) memset((char *) &sin, 0, sizeof(sin));
	if ((sp = getservbyname("nntp", "tcp")) ==  NULL) {
		(void) fprintf(stderr, "nntp/tcp: Unknown service.\n");
		return (-1);
	}
	sin.sin_port = sp->s_port;
	if ((sin.sin_addr.s_addr = inet_addr(machine)) != -1) {
	    sin.sin_family = AF_INET;
	    return(get_tcp_socket1(&sin));
	} else {
	    if ((hp = gethostbyname(machine)) == NULL) {
		    (void) fprintf(stderr, "%s: Unknown host.\n", machine);
		    return (-1);
	    }
	    sin.sin_family = hp->h_addrtype;
	}

	/*
	 * The following is kinda gross.  The 4.3BSD hostent structure
	 * contains a list of addresses, each of which should be tried in
	 * turn if the previous one fails.  However, 4.2 hostent structure
	 * doesn't have this list of addresses.  Under 4.3, h_addr is a
	 * #define to h_addr_list[0].  We use this to figure out whether to
	 * include the NS specific code ...
	 */

#ifdef	h_addr

	/* attempt multiple addresses */

	for (s = 0, cp = hp->h_addr_list; *cp; cp++) {
		if (s < 0)
			(void) fprintf(stderr,
				       "trying alternate address for %s\n",
				       machine);
		(void) memcpy((char *) &sin.sin_addr, *cp, hp->h_length);
		s = get_tcp_socket1(&sin);
		if (s >= 0)
			return(s);
	}
	(void) fprintf(stderr, "giving up ...\n");
	return(-1);

#else	/* 4.2BSD hostent structure */

	(void) memcpy((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
	return(get_tcp_socket1(&sin));

#endif
}

static get_tcp_socket1(sp)
    struct sockaddr_in *sp;
{
	int s;

	s = socket(sp->sin_family, SOCK_STREAM, 0);
	if (s < 0) {
		(void) perror("socket");
		return (-1);
	}
	if (connect(s, (struct sockaddr *)sp, sizeof (*sp)) < 0) {
		(void) perror("connect");
		(void) close(s);
		return(-1);
	}
	return(s);
}

#endif /* VMS */

#if defined(DECNET) || defined(VMS)
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

static int get_dnet_socket(machine)
    char *machine;
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

	if ((s = socket(AF_DECnet, SOCK_SEQPACKET, 0)) < 0) {
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

int handle_server_response(response, server)
    int	response;
    char *server;
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

	case ERR_GOODBYE:
 		printf(
	"The news service is currently not available from %s:\n\n",
		       server);
		fputs(server_init_msg, stdout);
		return(-1);
		break;

  	default:
		printf("Unexpected response code from the %s news server:\n\n",
			server);
		fputs(server_init_msg, stdout);
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

void put_server(string)
    char *string;
{
#ifdef DEBUG
	(void) fprintf(stderr, ">>> %s\n", string);
#endif
	(void) fprintf(ser_wr_fp, "%s\r\n", string);
	(void) fflush(ser_wr_fp);
#ifdef NNTPVIATIP
	{
	    char line[256];
	    get_server(line, sizeof(line));
	}
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

int get_server(string, size)
    char *string;
    int size;
{
    register char *cp;

    if (fgets(string, size, ser_rd_fp) == NULL) {
	return (-1);
    }
#ifdef NNTPVIATIP
    for (cp = string; cp < &string[size]; cp++) {
	*cp = *cp & 0x7F;
	if (*cp == '\0') {
	    break;
	}
    }
#endif
    if ((cp = index(string, '\r')) != NULL)
	*cp = '\0';
    else if ((cp = index(string, '\n')) != NULL)
	*cp = '\0';
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

void close_server()
{
	char	ser_line[256];

	if (ser_wr_fp == NULL || ser_rd_fp == NULL)
		return;

	put_server("QUIT");
	(void) get_server(ser_line, sizeof(ser_line));

	(void) fclose(ser_wr_fp);
	(void) fclose(ser_rd_fp);
	ser_wr_fp = ser_rd_fp = NULL;
}


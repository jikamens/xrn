#!/bin/awk -f

# xrn - an X-based NNTP news reader
# 
# Copyright (c) 1994-2023, Jonathan Kamens.
# 
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of the University of California not
# be used in advertising or publicity pertaining to distribution of 
# the software without specific, written prior permission.  The University
# of California makes no representations about the suitability of this
# software for any purpose.  It is provided "as is" without express or
# implied warranty.
# 
# THE UNIVERSITY OF CALIFORNIA DISCLAIMS ALL WARRANTIES WITH REGARD TO 
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
# FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR
# ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

BEGIN {
	print "/*";
	print " * This file is generated automatically from mesg_strings.h.";
	print " * Do not edit it!";
	print " */";
	print "";
	print "extern char *message_strings[];";
	print "";

	start = 0;
}

/XRN_LANG/ {
	start = 1;
	msg_num = 0;
}

/^\#/ || /^[ 	]*$/ {
	if (start == 1) {
		print
	}
}

/^\/\* < [^ >]* > / {
	if (start == 1) {
		printf("#define %s_MSG message_strings[%d]\n", $3, msg_num);
		next;
	}
}

/",/ {
	if (start == 1) {
		msg_num++;
		next;
	}
}

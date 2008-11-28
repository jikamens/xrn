#!/bin/awk -f

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

(start == 1) && (/^\#/ || /^[ 	]*$/) { print }

(start == 1) && /^\/\* < [^ >]* > / {
	printf("#define %s_MSG message_strings[%d]\n", $3, msg_num);
	next;
}

(start == 1) && /",/ {
	msg_num++;
	next;
}

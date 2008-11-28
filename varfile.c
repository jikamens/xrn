#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <errno.h>
#include "error_hnds.h"
#include "mesg.h"
#include "mesg_strings.h"
#include "varfile.h"
#include "cache.h"

#ifdef TESTING
#define utTempnam tempnam
#define mesgPane(a,b,c,d,e) fprintf(stderr, c, d, e)
#define mesgPane6(a,b,c,d,e,f) fprintf(stderr, c, d, e, f)
#define XtMalloc malloc
#define XtRealloc realloc
#define XtFree free
extern char *tempnam();
#else
#define mesgPane6(a,b,c,d,e,f) mesgPane(a,b,c,d,e,f)
#endif /* TESTING */

struct var_rec {
    char *name;
    char *value;
    char written;
};

static struct var_rec *find_var(vars, name)
struct var_rec *vars;
char *name;
{
    if (! vars)
	return 0;

    while (vars->name) {
	if (strcmp(vars->name, name) == 0)
	    return vars;
	vars++;
    }
    return 0;
}

static int parse_line(line, name, value)
char *line, **name, **value;
{
    char *end;

    if (*line++ != CACHE_VAR_CHAR)
	return 0;

    *name = line;

    if (! (*value = strchr(line, ' ')))
	return 0;

    *(*value)++ = '\0';
    if ((end = strchr(*value, '\n')))
	*end = '\0';

    return 1;
}

#define CLEANUP { \
	    (void) fclose(input); \
	    (void) fclose(output); \
	    XtFree(tmpfile); \
	    return 0; \
	    }
#define WRITE_LINE { \
	    if (output) { \
		if (fputs(line, output) == EOF) { \
		    mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG, \
			     tmpfile, errmsg(errno)); \
		    CLEANUP; \
		} \
	    } }

static struct var_rec *var_read_write_file(filename, vars)
char *filename;
struct var_rec *vars;
{
    FILE *input, *output = 0;
    struct var_rec *this_var;
    char line[BUFSIZ], *name, *val, last_chopped = 0, *tmpfile = 0;

#ifdef GCC_WALL
    tmpfile = 0;
#endif

    if (! (input = fopen(filename, "r"))) {
	if (errno != ENOENT)
	    mesgPane(XRN_SERIOUS, 0, CANT_OPEN_FILE_MSG, filename, errmsg(errno));
	if (! vars)
	    return 0;
    }

    if (vars) {
      tmpfile = utTempFile(filename);
	if (! (output = fopen(tmpfile, "w"))) {
	    mesgPane(XRN_SERIOUS, 0, CANT_OPEN_TEMP_MSG, tmpfile, errmsg(errno));
	    (void) fclose(input);
	    XtFree(tmpfile);
	    return 0;
	}
	for (this_var = vars; this_var->name; this_var++)
	    this_var->written = 0;
    } else {
	vars = (struct var_rec *) XtMalloc(sizeof(*vars));
	vars->name = 0;
    }

    if (input) {
	while (fgets(line, sizeof(line), input)) {
	    if (line[strlen(line) - 1] != '\n') {
		last_chopped = 1;
		WRITE_LINE;
		continue;
	    }
	    if (last_chopped) {
		last_chopped = 0;
		WRITE_LINE;
		continue;
	    }
	    if (parse_line(line, &name, &val)) {
		if (output) {
		    if ((this_var = find_var(vars, name))) {
			if (this_var->written)
			    continue;
			if (fprintf(output, "%c%s %s\n", CACHE_VAR_CHAR,
				    name, this_var->value) == EOF) {
			    mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG,
				     tmpfile, errmsg(errno));
			    CLEANUP;
			}
			this_var->written = 1;
		    } else if (fprintf(output, "%c%s %s\n", CACHE_VAR_CHAR,
				       name, val) == EOF) {
			mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG,
				 tmpfile, errmsg(errno));
			CLEANUP;
		    }
		} else
		    var_set_value(&vars, name, val);
	    } else
		WRITE_LINE;
	}

	(void) fclose(input);
    }
    
    if (output) {
	for (this_var = vars; this_var->name; this_var++)
	    if ((! this_var->written) &&
		(fprintf(output, "%c%s %s\n", CACHE_VAR_CHAR,
			 this_var->name, this_var->value) == EOF)) {
		mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG, tmpfile,
			 errmsg(errno));
		(void) fclose(output);
		XtFree(tmpfile);
		return 0;
	    }
	if (fclose(output) == EOF) {
	    mesgPane(XRN_SERIOUS, 0, ERROR_WRITING_SAVE_FILE_MSG, tmpfile,
		     errmsg(errno));
	    XtFree(tmpfile);
	    return 0;
	}
	if (rename(tmpfile, filename)) {
	    mesgPane6(XRN_SERIOUS, 0, ERROR_RENAMING_MSG, tmpfile, filename,
		     errmsg(errno));
	    XtFree(tmpfile);
	    return 0;
	}
    }

    XtFree(tmpfile);
    return vars;
}

#undef WRITE_LINE
#undef CLEANUP

struct var_rec *var_read_file(name)
char *name;
{
    return var_read_write_file(name, 0);
}

int var_write_file(vars, name)
struct var_rec *vars;
char *name;
{
    if (! vars)
	return 1;
    return(var_read_write_file(name, vars) ? 1 : 0);
}

char *var_get_value(vars, name)
struct var_rec *vars;
char *name;
{
    struct var_rec *var;

    if ((var = find_var(vars, name)))
	return XtNewString(var->value);
    return 0;
}

void var_set_value(vars, name, value)
struct var_rec **vars;
char *name, *value;
{
    struct var_rec *var;
    int count;

    if (! *vars) {
	*vars = (struct var_rec *) XtMalloc(sizeof(**vars));
	(*vars)->name = 0;
    } else if ((var = find_var(*vars, name))) {
	XtFree(var->value);
	var->value = XtNewString(value);
	return;
    }

    for (var = *vars, count = 0; var->name; count++, var++) /* empty */;

    *vars = (struct var_rec *) XtRealloc((char *) *vars,
					 (count + 2) * sizeof(**vars));

    (*vars)[count].name = XtNewString(name);
    (*vars)[count].value = XtNewString(value);

    (*vars)[count+1].name = 0;

    return;
}

void var_free(vars)
struct var_rec *vars;
{
    struct var_rec *this_var;

    if (! vars)
	return;

    for (this_var = vars; this_var->name; this_var++) {
	XtFree(this_var->name);
	XtFree(this_var->value);
    }
    XtFree((char *) vars);
    return;
}

#ifdef TESTING
void display_vars(vars)
struct var_rec *vars;
{
    struct var_rec *this_var;

    if (! vars) {
	fprintf(stdout, "No variables.\n");
	return;
    }

    fprintf(stdout, "Variables:\n");
    for (this_var = vars; this_var->name; this_var++)
	fprintf(stdout, "%s\t%s\n", this_var->name, this_var->value);
    fprintf(stdout, "\n");
    return;
}


    
int main()
{
    char inline[BUFSIZ], *name, *value, *nl;
    struct var_rec *vars = 0;
    int retval;

    while (fgets(inline, sizeof(inline), stdin)) {
	if ((nl = strchr(inline, '\n')))
	    *nl = '\0';
	if (*inline == 'r') {					/* read a file */
	    var_free(vars);
	    vars = var_read_file(inline + 1);
	    if (! vars)
		fprintf(stderr, "var_read_file returned NULL\n");
	} else if (*inline == 'w') {				/* write a file */
	    retval = var_write_file(vars, inline + 1);
	    if (! retval) {
		fprintf(stderr, "var_write_file returned non-zero\n");
	    }
	} else if (*inline == CACHE_VAR_CHAR) {			/* set a variable */
	    if (parse_line(inline, &name, &value)) {
		var_set_value(&vars, name, value);
		if (! vars)
		    fprintf(stderr, "var_set_value resulted in NULL\n");
	    }
	} else if (*inline == '=') {				/* get a variable */
	    value = var_get_value(vars, inline + 1);
	    if (! value)
		fprintf(stderr, "No value for %s\n", inline + 1);
	    else
		fprintf(stdout, "Value for %s is %s\n", inline + 1, value);
	} else if (*inline == 'f') {
	    var_free(vars);
	    vars = 0;
	} else if (*inline != 'l') {				/* list variables */
	    fprintf(stderr, "Unknown command: %s\n", inline);
	}
	display_vars(vars);
    }
    return(0);
}

#endif /* TESTING */

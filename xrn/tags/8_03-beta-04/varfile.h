#ifndef _VARFILE_H_
#define _VARFILE_H_

struct	var_rec *var_read_file _ARGUMENTS((char *));
int	var_write_file _ARGUMENTS((struct var_rec *, char *));
char *	var_get_value _ARGUMENTS((struct var_rec *, char *));
void	var_set_value _ARGUMENTS((struct var_rec **, char *, char *));
void	var_free _ARGUMENTS((struct var_rec *));

#endif /* _VARFILE_H_ */

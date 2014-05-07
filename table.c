#include "saber.h"
#include <stdlib.h>

/* initialized a stab struct */
stab * stabCreate(){
	stab * new = NULL;
	if(!(new = (stab *)malloc(sizeof(stab))))
		return NULL;

	new->table = NULL;
	new->expire = NULL;
	new->id = -1;

	if(!(new->table = fdictCreate()))
		goto err;
	if(!(new->expire = fdictCreate()))
		goto err;

	return new;

err:
	stabFree(new);
}

void stabFree(stab * t){
	fdictFree(t->table);
	fdictFree(t->expire);
	free(t);
}

/* high level add , operate database table */
int stabAdd(stab * t, char * key, sobj * value){
	return fdictAdd(t->table, (void *)key, value);
}

/* remove a value, and return the operation status */
int stabRemove(stab * t, char * key){
	return 1;
}

/* set a value, if not exist,will add a new one */
int stabSet(stab * t, char * key, sobj * value){
	return 1;
}

sobj * stabGet(stab * t, char * key){
	dictNode * re = fdictSearch(t->table, (void *)key);

	if(re) return (sobj *)re->value.val;
	return NULL;
}

/* replace a key-value whit the key specfied,
** if key not exist, return 0,else return 1 */
int stabReplace(stab * t, char * key, sobj * value){
	return 1;
}

/* if a key-value exist in the table,
** if exist, return 1, neither return 0 */
int stabExist(stab * t, char * key){
	return 1;
}

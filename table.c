#include "saber.h"

/* high level add , operate database table */
int stabAdd(stab * t, sobj * key, sobj * value){
	fstr * key = fstrCopy((char *)key->value);

	return fdictAdd(t->table, key, value);
}

/* remove a value, and return the operation status */
int stabRemove(stab * t, sobj * key){
	return fdictRemove(t->table, key->value);
}

/* set a value, if not exist,will add a new one */
int stabSet(stab * t, sobj * key, sobj * value){
	fstr * key = fstrCopy((char *)key->value);
	return fdictRemove(t->table, key, value);
}

sobj * stabGet(stab * t, sobj * key){
	dictNode * re = fdictSearch(t->table, key->value);

	if(re) return (sobj *)&re->value;
	return NULL;
}

/* replace a key-value whit the key specfied,
** if key not exist, return 0,else return 1 */
int stabReplace(stab * t, sobj * key, sobj * value){
	return fdictReplace(t->table, key->value, value);
}

/* if a key-value exist in the table,
** if exist, return 1, neither return 0 */
int stabExist(stab * t, sobj * key){
	return (fdictSearch(t->table, key->value) != NULL);
}




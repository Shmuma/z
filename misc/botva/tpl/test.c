#include <stdio.h>
#include <stdlib.h>
#include <tpl.h>


typedef struct {
	uint64_t num;
	int val;
	char* str;
} uberstruct_t;


int main ()
{
	tpl_node* tn;
	uberstruct_t strs[] = {
		{ num: 1, str: "one", val: 2},
		{ num: 2, str: "two", val: 2},
		{ num: 3, str: "three", val: 2},
	};
	uberstruct_t* s = strs;
	int i;
	
 	tn = tpl_map ("S(Uis)#", s, sizeof (strs)/sizeof (strs[0]));
	tpl_pack (tn, 0);
	tpl_dump (tn, TPL_FILE, "test.tpl");
	tpl_free (tn);

	return 0;
}

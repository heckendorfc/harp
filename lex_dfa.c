#include <lex.h>
#include <lex_dfa.h>
#include <edit_shell.h>

State *trap=NULL;

void add_path(State *dfa,const char *str,int token){
	int i,j;
	State *o=dfa;
	for(i=0;str[i];i++){
		if(!dfa->out){
			INIT_MEM(dfa->out,SIGMA_SIZE);
			for(j=0;j<SIGMA_SIZE;j++){
				dfa->out[j].state=trap;
			}
		}
		if(!dfa->out[(int)str[i]].state ||
				dfa->out[(int)str[i]].state==o ||
				dfa->out[(int)str[i]].state==trap){
			INIT_MEM(dfa->out[(int)str[i]].state,1);
			dfa->out[(int)str[i]].state->final=TOK_NULL;
			dfa->out[(int)str[i]].state->out=NULL;
		}
		dfa=dfa->out[(int)str[i]].state;
	}
	dfa->final=token;
	dfa->out=NULL;
}

State* generate_operator_dfa(){
	int i;

	State *dfa;
	INIT_MEM(dfa,1);

	dfa->out=NULL;
	dfa->final=0;
	INIT_MEM(dfa->out,SIGMA_SIZE);
	for(i=0;i<SIGMA_SIZE;i++){
		dfa->out[i].state=dfa;
	}

	if(!trap){
		INIT_MEM(trap,1);
		trap->final=TOK_NULL;
		trap->out=NULL;
	}

	add_path(dfa,"(",TOK_OPERATOR|TOK_OPAR);
	add_path(dfa,")",TOK_OPERATOR|TOK_CPAR);
	add_path(dfa,",",TOK_OPERATOR|TOK_COMMA);

	return dfa;
}

State* generate_quote_dfa(){
	int i;

	State *dfa;
	INIT_MEM(dfa,1);

	dfa->out=NULL;
	dfa->final=0;
	INIT_MEM(dfa->out,SIGMA_SIZE);
	for(i=0;i<SIGMA_SIZE;i++){
		dfa->out[i].state=dfa;
	}

	if(!trap){
		INIT_MEM(trap,1);
		trap->final=TOK_NULL;
		trap->out=NULL;
	}

	add_path(dfa,"\"",TOK_QUOTE);
	add_path(dfa,"'",TOK_QUOTE);

	return dfa;
}

State* generate_reserved_dfa(){
	int i;

	State *dfa;
	INIT_MEM(dfa,1);

	dfa->out=NULL;
	dfa->final=0;
	INIT_MEM(dfa->out,SIGMA_SIZE);
	for(i=0;i<SIGMA_SIZE;i++){
		dfa->out[i].state=trap;
	}

	if(!trap){
		INIT_MEM(trap,1);
		trap->final=TOK_NULL;
		trap->out=NULL;
	}

	add_path(dfa,"set",TOK_RESERVED|TOK_SET);
	add_path(dfa,"for",TOK_RESERVED|TOK_FOR);
	add_path(dfa,"in",TOK_RESERVED|TOK_IN);
	add_path(dfa,"while",TOK_RESERVED|TOK_WHILE);
	add_path(dfa,"{",TOK_RESERVED|TOK_OCBRACE);
	add_path(dfa,"}",TOK_RESERVED|TOK_CCBRACE);

	return dfa;
}

State* generate_meta_dfa(){
	int i;

	State *dfa;
	INIT_MEM(dfa,1);

	dfa->out=NULL;
	dfa->final=0;
	INIT_MEM(dfa->out,SIGMA_SIZE);
	for(i=0;i<SIGMA_SIZE;i++){
		dfa->out[i].state=dfa;
	}

	if(!trap){
		INIT_MEM(trap,1);
		trap->final=TOK_NULL;
		trap->out=NULL;
	}

	//add_path(dfa,"!",TOK_META);
	add_path(dfa,"*",TOK_META);
	add_path(dfa,"?",TOK_META);
	add_path(dfa,"[",TOK_META);
	add_path(dfa,"]",TOK_META);

	return dfa;
}

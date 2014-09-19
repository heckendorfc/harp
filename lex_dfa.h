#ifndef LEX_DFA_H
#define LEX_DFA_H

#define SIGMA_SIZE 128

typedef struct State{
	int final;
	struct StateList *out;
}State;

typedef struct StateList{
	State *state;
	//struct StateList *next;
}StateList;

State* generate_operator_dfa();
State* generate_quote_dfa();
State* generate_meta_dfa();
State* generate_reserved_dfa();

#endif

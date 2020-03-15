#include <stdio.h>
#include <stdlib.h>
#include "Stack.h"

void InitializeStack(stack *s)
{
	s->stacklist = NULL;
}

int EmptyStack(stack *s)
{
	return s->stacklist == NULL;
}

void Push(int new_obj, stack *s)
{
	stacknode *temp;

	temp = (stacknode*) malloc(sizeof(stacknode));

	if(temp == NULL)
	{
		printf("Error in allocating memory for the stack.\n");
	}
	else
	{
		temp->nodeptr = s->stacklist;
		temp->obj = new_obj;
		s->stacklist = temp;
	}
}

void Pop(stack *s, int *new_obj)
{
	stacknode *temp;

	if(s->stacklist == NULL)
	{
		printf("Stack underflow.\n");
	}
	else
	{
		temp = s->stacklist;
		*new_obj = temp->obj;
		s->stacklist = temp->nodeptr;
		free(temp);
	}
}
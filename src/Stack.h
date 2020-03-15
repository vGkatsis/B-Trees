#ifndef STACK_H
#define STACK_H

typedef struct StackNode{
	int obj;
	struct StackNode *nodeptr;
}stacknode;

typedef struct Stack{
	stacknode *stacklist;
}stack;

void InitializeStack(stack *s);
int EmptyStack(stack *s);
void Push(int new_obj, stack *s);
void Pop(stack *s, int *new_obj);

#endif
#ifndef __LINKLIST_H_
#define __LINKLIST_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Node
{
	char name[128];
	struct Node* next;
};

void AddListTill(struct Node** head,struct Node** end,char *name );

char* ScanList(struct Node* head);

struct Node* FindNode(struct Node* head,char *a );

void DeleteListHead(struct Node** head);

void DeleteListTail(struct Node** head, struct Node** end);

void FreeList(struct Node** head, struct Node** end);



void DeleteListRand(struct Node** head,struct Node** end,char* a);





#ifdef __cplusplus
}
#endif

#endif


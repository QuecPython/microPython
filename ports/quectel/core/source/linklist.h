/*
 * Copyright (c) Quectel Wireless Solution, Co., Ltd.All Rights Reserved.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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


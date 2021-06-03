
#include <stdio.h>
#include "stdlib.h"
#include "string.h"	
#include "linklist.h"
#include "helios_fs.h"

#define FILE_NAME_MAX_LEN 1024

static char* file_name_str = NULL;

struct Node* FindNode(struct Node* head,char *a );


void AddListTill(struct Node** head,struct Node** end,char *name )
{
		struct Node* temp1 =FindNode(*head,name);
		if(NULL!=temp1)
		{
			printf("file is existence\n");
			return;
		}
		//创建一个节点
		struct Node* temp=(struct Node*)calloc(1,sizeof(struct Node));		//此处注意强制类型转换

		//节点数据进行赋值
		memcpy(temp->name, name, strlen(name));
		temp->next=NULL;		
		
		//连接分两种情况1.一个节点都没有2.已经有节点了，添加到尾巴上
		if(NULL==*head)
		{	

			*head=temp;
		//	end=temp;
		}
		else
		{
		(*end)->next=temp;
	//	end=temp;			//尾结点应该始终指向最后一个
		}
		*end=temp;			//尾结点应该始终指向最后一个
}

char* ScanList(struct Node* head)
{
	struct Node *temp =head;		//定义一个临时变量来指向头
	if(file_name_str != NULL) {
		free(file_name_str);
		file_name_str = NULL;
	}
	file_name_str = calloc(1, FILE_NAME_MAX_LEN);
	
	int first = 1;
	int ret = 0;
	while (temp !=NULL)
	{
		printf("%s\n",temp->name);
		if(first) {
			first = 0;
			ret += snprintf(file_name_str + ret, FILE_NAME_MAX_LEN, "%s", temp->name);
		} else {
			ret += snprintf(file_name_str + ret, FILE_NAME_MAX_LEN, "\r\n%s", temp->name);
            
		}
		if(ret < 0) {
			printf("this is > %d\n", FILE_NAME_MAX_LEN);
			return NULL;
		}
		temp = temp->next;		//temp指向下一个的地址 即实现++操作
	}
	return file_name_str;
}

struct Node* FindNode(struct Node* head,char *a )
{
	printf("felix a = %s\n",a);
	printf("strlen a = %d\n",strlen(a));
	struct Node *temp =head;
	int num = 0;
	while(temp !=NULL)
	{
		printf(".............\n");
		printf("num = %d\n",num);
		printf("temp->name = %s\n",temp->name);
		printf(".............\n");
		num++;
	if(strncmp(a,temp->name,strlen(a)) == 0)
	{
		printf("felix no find a = %s\n",a);
		return temp;
	}
	temp = temp->next;
	}
	//没找到
		return NULL;
}

void DeleteListHead(struct Node** head)
{	//记住旧头
	//struct Node* temp=*head;
	//链表检测 
	if (NULL==*head)
	{
			printf("file is null\n");
			return;
	}

	printf("*head name = %s\n",(*head)->name);
	*head=(*head)->next;//头的第二个节点变成新的头
	printf("*head name1 = %s\n",(*head)->name);
	//free(temp);

}

void DeleteListTail(struct Node** head, struct Node** end)
{ 
	printf("DeleteListTail start333333333333333333\n");
	if (NULL == *end)
	{
		printf("file is null\n");
		return;
	}
	//链表不为空 
	//链表有一个节点
	 if (*head==*end)
	 {
	 	printf("head = end\n");
		 free(*head);
		 *head=NULL;
		 *end=NULL; 
	 }
	 else
	 {
	 	printf("head != end\n");
		//找到尾巴前一个节点
		 struct Node* temp =*head;
		 while (temp->next!=*end)
		 {
			 temp = temp->next;
		 }
		 //找到了，删尾巴
		//释放尾巴
		 free(*end);
		 //尾巴迁移
		 *end=temp;
		 //尾巴指针为NULL
		 (*end)->next=NULL;
	 }

}

void FreeList(struct Node** head, struct Node** end)
{
	//一个一个NULL
	struct Node *temp =*head;		//定义一个临时变量来指向头
	while (temp !=NULL)
	{
	//	printf("%d\n",temp->a);
		struct Node* pt =temp;
		Helios_remove((const char*)pt->name);
		temp = temp->next;		//temp指向下一个的地址 即实现++操作
		free(pt);					//释放当前
	}
	//头尾清空	不然下次的头就接着0x10
	*head =NULL;
	*end =NULL;
} 




void DeleteListRand(struct Node** head,struct Node** end,char* a)
{
    struct Node* head_temp = *head;
    struct Node* end_temp = *end;

	//链表判断 是不是没有东西
	if(NULL==head_temp)
	{
	printf("file is null\n");
	return;
	}
    //链表有东西，找这个节点
	struct Node* temp =FindNode(head_temp,a);
	if(NULL==temp)
	{
	printf("not find node\n");
	return;
	}
	//找到了,且只有一个节点
	if(head_temp==end_temp)
	{
	free(*head);
	*head=NULL;
	*head=NULL;
	}
	else if(head_temp->next==end_temp) //有两个节点
	{
	//看是删除头还是删除尾
	if(end_temp==temp)
		{	DeleteListTail(head,end); }
	else if(temp==head_temp)
		{	
			printf("start delete head\n");
			DeleteListHead(head); }	
	}
	else//多个节点
	{
		//看是删除头还是删除尾
		if(end_temp==temp)
			DeleteListTail(head,end);
		else if(temp==head_temp)
			DeleteListHead(head);	
		else	//删除中间某个节点
		{	//找要删除temp前一个，遍历
			struct Node*pt =head_temp;
			while(pt->next!=temp)
			{
			pt=pt->next;
			}
			//找到了
			//让前一个直接连接后一个 跳过指定的即可
			 pt->next=temp->next;
			 free(temp);
		
		}
	}
	

}



#include "llist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lroot * init() {
  struct lroot *root;
  // выделение памяти под корень списка
  root = (struct lroot*)malloc(sizeof(struct lroot));
  root->first_node = NULL;  // это последний узел списка
  return(root);
}

struct list * addelem(lroot *root, int number, char *name) {
  struct list *temp, *p;
  
  root->count = root->count + 1;
  
  temp = (struct list*)malloc(sizeof(list));
  p = root->first_node;  // сохранение указателя на следующий узел
  root->first_node = temp;  // предыдущий узел указывает на создаваемый
  temp->fd = number;  // сохранение поля данных добавляемого узла
  strcpy(temp->name, name);
  temp->ptr = p;    // созданный узел указывает на следующий элемент
  return(temp);
}

struct list  * deletelem(list *lst, lroot *root) {
  struct list *temp;
  temp = root->first_node;
  root->count = root->count - 1;
  if (temp != lst)
  {
	  while(temp->ptr!=lst)  // просматриваем список начиная с корня
		{    // пока не найдем узел, предшествующий lst
		  temp = temp->ptr;
		}
	  temp->ptr = lst->ptr; // переставляем указатель
	  free(lst);  // освобождаем память удаляемого узла
  } else {
	  root->first_node = lst->ptr;
	  temp->ptr = lst->ptr;
	  free(lst);
  }
  return(temp);
}

void listprint(lroot *root) {
  struct list *p;
  p = root->first_node;
  do  {
      printf("%d ",p->fd); // вывод значения элемента p
      printf("%s\n",p->name); // вывод значения элемента p
      p = p->ptr; // переход к следующему узлу
   } while(p != NULL);
}

struct list * listfind(lroot *root,char *name) {
  struct list *p;
  p = root->first_node;
  do {
	  if (!strcmp(name, p->name)) {
		return(p);
	  }
      p = p->ptr;
  } while(p != NULL);
  
  return(NULL);
}

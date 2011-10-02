/*
 * listaPuntos.c
 *
 *  Created on: 06/05/2011
 *      Author: sgarcia
 */


#include <stdlib.h>
#include "lista_puntos.h"


typedef struct _triangle {
	punto *puntos;
}triangle;


lista_puntos *new_lista_puntos() {
	lista_puntos *lista = (lista_puntos *)malloc(sizeof(lista_puntos));

	lista -> first = 0;
	lista -> last  = 0;
	lista -> size  = 0;

	return lista;
}

void add_punto(lista_puntos *lista, punto *p) {
	nodo_punto *n = (nodo_punto *)malloc(sizeof(nodo_punto));

	n -> p = p;
	n -> invariante = 0.0;
	n -> mas_cercanos = 0;
	n -> next = 0;

	if (lista -> last != 0) {
		lista -> last -> next = n;
	}

	lista -> last = n;

	if (lista -> first == 0) {
		lista -> first = lista -> last;
	}

	lista -> size++;
}


void add_punto_cercano(nodo_punto *nodo, punto *p, float angulo) {
	nodo_cercano *iter, *prev, *n;

	prev = 0;
	for (iter = nodo -> mas_cercanos;
			iter != 0 && iter -> angulo < angulo;)	{
		prev = iter;
		iter = iter -> next;
	}

	n = (nodo_cercano *)malloc(sizeof(nodo_cercano));

	n -> p = p;
	n -> angulo = angulo;

	if (iter == 0 && prev == 0) {
		n -> next = 0;
		nodo -> mas_cercanos = n;
	} else if (prev == 0){
		n -> next = iter;
		nodo -> mas_cercanos = n;
	} else {
		n -> next = iter;
		prev -> next = n;
	}
}

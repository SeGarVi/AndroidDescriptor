/*
 * listaPuntos.h
 *
 *  Created on: 06/05/2011
 *      Author: sgarcia
 */

#ifndef LISTAPUNTOS_H_
#define LISTAPUNTOS_H_

#include "punto.h"


/*
 * Estructura que define el tipo de los
 * nodos del arbol de nodos cercanos.
 */
typedef struct _nodo_cercano {
	punto *p;
	float  angulo;
	struct _nodo_cercano *next;
}nodo_cercano;


/*
 * Estructura que define el tipo de
 * los nodos de la lista de puntos.
 */
typedef struct _nodo_punto {
	punto *p;
	float invariante;
	struct nodo_cercano *mas_cercanos;
	struct _nodo_punto  *next;
}nodo_punto;


/*
 * Estructura que implementa una lista de puntos.
 */
typedef struct _lista_puntos {
	nodo_punto *first, *last;
	int size;	/* Debug */
}lista_puntos;


lista_puntos *new_lista_puntos();

void add_punto(lista_puntos *lista, punto *p);
void add_punto_cercano(nodo_punto *nodo, punto *p, float angulo);
void add_punto_cercano_debug(nodo_punto *nodo, punto *p, float angulo);

#endif /* LISTAPUNTOS_H_ */

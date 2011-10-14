/*
 * punto.c
 *
 *  Created on: 06/05/2011
 *      Author: sgarcia
 */

#include <stdlib.h>
#include "punto.hpp"

punto *new_punto() {
	punto *p = (punto *)malloc(sizeof(punto));

	/*
	 * Nos aseguramos de que los componentes
	 * para que no se acceda a regiones
	 * de memoria no deseados
	 */
	p -> x = -1;
	p -> y = -1;

	return p;
}

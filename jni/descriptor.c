#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <android/log.h>
#include <android/bitmap.h>

#include "punto.h"
#include "lista_puntos.h"
#include "ext_cv_funcs.h"

#define  LOG_TAG    "AndroidDescriptor"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


/******************************
 * Variables de configuracion *
 ******************************/
static int divisiones_ancho  	= 10;
static int divisiones_alto   	= 15;
static int cantidad_cercanos 	= 7;
static int cantidad_combinacion = 5;
static int debug = 0;


/******************************
 ** Variables para profiling **
 ******************************/
static float t_total,
	  	  	 t_suavizado,
	  	  	 t_adaptive_threshold,
	  	  	 t_convertir_grises,
	  	  	 t_threshold,
	  	  	 t_encontrar_centroides,
	  	  	 t_cross_ratio,
	  	  	 t_buscar_mas_cercanos,
	  	  	 t_calcular_area,
	  	  	 t_cercanos_en_celdas,
	  	  	 t_calcular_descriptor_documento,
	  	  	 t_clasificar_centros,
	  	  	 t_encontrar_centros,
	  	  	 t_preprocesar;

static int n_centros;


/******************************
 * Funciones de configuracion *
 ******************************/
/*void set_divisiones_ancho(int divs) {
	divisiones_ancho = divs;
}

int get_divisiones_ancho() {
	return divisiones_ancho;
}

void set_divisiones_alto(int divs) {
	divisiones_alto = divs;
}

int get_divisiones_alto() {
	return divisiones_alto;
}

void set_cantidad_cercanos(int cantidad) {
	cantidad_cercanos = cantidad;
}

int get_cantidad_cercanos() {
	return cantidad_cercanos;
}

void set_cantidad_combinacion(int cantidad) {
	cantidad_combinacion = cantidad;
}

int get_cantidad_combinacion() {
	return cantidad_combinacion;
}*/

static int **generar_combinaciones(int mayor, int menor, int n_combinaciones) {
	int  i, j, pivote_izq, pivote_der;
	int **combinaciones;

	//LOGI("generar_combinaciones running");
	pivote_der = menor-1;

	combinaciones = (int **)malloc(n_combinaciones*sizeof(int *));
	combinaciones[0] = (int *)malloc(menor * sizeof(int));

	for (i = 0; i < menor; i++) {
		combinaciones[0][i] = i;
	}

	i = 1;
	for (pivote_izq = pivote_der; pivote_izq >= 0; pivote_izq--){
		combinaciones[i] = (int *)malloc(menor * sizeof(int));
		memcpy(combinaciones[i], combinaciones[0], menor * sizeof(int));

		for (j = pivote_der; j >= pivote_izq; j--) {
			combinaciones[i][j]++;
		}
		i++;

		for (j = pivote_der; j >= pivote_izq; j--) {
			combinaciones[i] = (int *)malloc(menor * sizeof(int));
			memcpy(combinaciones[i], combinaciones[i-1], menor * sizeof(int));
			combinaciones[i][j]++;
			i++;
		}
	}

	return combinaciones;
}

static int combinacion_sin_repeticion(int mayor, int menor) {
	int factorial, numerador, limite_numerador, i;

	//LOGI("combinacion_sin_repeticion running");
	factorial = numerador = 1;
	limite_numerador = mayor - menor;
	for (i = mayor; i > 0; i--) {
		factorial *= i;

		if (i <= menor) {
			numerador *= i;
		}

		if (i <= limite_numerador ){
			numerador *= i;
		}
	}

	return factorial/numerador;
}

static float calcular_area(punto **triangulo) {
	int i;
	float semiperimetro, distancias[3], area;
	punto vector;

	clock_t fin, ini;

	//LOGI("calcular_area running");
	ini = clock() / (CLOCKS_PER_SEC / 1000);

	semiperimetro = 0;
	for (i = 0; i < 3; i++){
		vector.x = triangulo[i] -> x - triangulo[(i+1)%3] -> x;
		vector.x = triangulo[i] -> y - triangulo[(i+1)%3] -> y;

		distancias[i]  = sqrt( pow(vector.x, 2) + pow(vector.y, 2));
		semiperimetro += distancias[i];
	}

	semiperimetro /= 2;

	area = sqrt(semiperimetro*(semiperimetro - distancias[0])*
			  (semiperimetro - distancias[1])*
			  (semiperimetro - distancias[2]));

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_calcular_area = fin - ini;

	return area;
}

static float cross_ratio(int   *combinacion,
						 punto **cercanos){
	punto **triangulo;
	float  *areas;
	float	res;

	clock_t fin, ini;

	//LOGI("cross_ratio running");
	ini = clock() / (CLOCKS_PER_SEC / 1000);

	areas = (float *)malloc(4 * sizeof(float));

	triangulo = (punto **)malloc(3*sizeof(punto *));

	triangulo[0] = cercanos[0];
	triangulo[1] = cercanos[1];
	triangulo[2] = cercanos[2];
	areas[0] = calcular_area(triangulo);

	triangulo[1] = cercanos[3];
	triangulo[2] = cercanos[4];
	areas[1] = calcular_area(triangulo);

	triangulo[1] = cercanos[1];
	triangulo[2] = cercanos[3];
	areas[2] = calcular_area(triangulo);

	triangulo[1] = cercanos[2];
	triangulo[2] = cercanos[4];
	areas[3] = calcular_area(triangulo);

	res = (areas[0] * areas[1])/(areas[3] * areas[4]);

	free(areas);

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_cross_ratio = fin - ini;

	return res;
}

static float *calcular_descriptor_centroide(nodo_punto *punt,
											int n_combinaciones,
											int **combinaciones) {
	int   i;
	float *descriptor_punto;
	punto **cercanos;
	nodo_cercano *iterador;

	//LOGI("calcular_descriptor_centroide running");
	descriptor_punto = (float *)malloc(n_combinaciones*sizeof(float));

	i = 0;
	cercanos = (punto **)malloc(cantidad_cercanos*sizeof(punto *));
	for (iterador = punt -> mas_cercanos;
			iterador!=0 ;
			iterador = iterador -> next) {
		cercanos[i++] = iterador -> p;
	}

	for (i = 0; i < n_combinaciones; i++) {
		descriptor_punto[i] = cross_ratio(combinaciones[i], cercanos);
	}

	return descriptor_punto;
}

static float **calcular_descriptor_documento(lista_puntos **centros){
	int i, j, n_combinaciones, n_celdas;
	int **combinaciones;
	float **descriptor_documento;
	nodo_punto *iterador;

	clock_t fin, ini;

	ini = clock() / (CLOCKS_PER_SEC / 1000);

	//LOGI("calcular_descriptor_documento running");
	descriptor_documento = (float **)malloc(n_centros*sizeof(float*));


	n_combinaciones = combinacion_sin_repeticion(cantidad_cercanos,
						                         cantidad_combinacion);
	combinaciones = generar_combinaciones(cantidad_cercanos,
										  cantidad_combinacion,
										  n_combinaciones);


	i = 0;
	n_celdas = divisiones_ancho * divisiones_alto;
	for (j = 0; j < n_celdas; j++) {
		for (iterador = centros[j] -> first;
				iterador != 0;
				iterador = iterador -> next) {
			descriptor_documento[i++] = calcular_descriptor_centroide(iterador,
																	  n_combinaciones,
																	  combinaciones);
		}
	}

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_calcular_descriptor_documento = fin - ini;

	return descriptor_documento;
}

static int *celdas_a_calcular(int celda_actual,
							  int iteracion,
							  int *n_celdas){
	//no se tienen en cuenta los extremos en el numero de celdas!!!

	int  x, y, i,  amplitud,
		 celda_x,  celda_y,
		 inicio_x, inicio_y,
		 final_x,  final_y;
	int *resultado;

	amplitud = 2*iteracion + 1;
	*n_celdas = (4*amplitud-4);

	resultado =	(int *)malloc(*n_celdas*sizeof(int));

	for (i = 0; i < *n_celdas; i++) resultado[i] = -1;

	celda_x = celda_actual%divisiones_ancho;
	celda_y = celda_actual/divisiones_ancho;

	inicio_x = celda_x - (amplitud-1)/2;
	inicio_y = celda_y - (amplitud-1)/2;

	final_x = celda_x + (amplitud-1)/2;
	final_y = celda_y + (amplitud-1)/2;

	if (inicio_x < 0) inicio_x = 0;
	if (inicio_y < 0) inicio_y = 0;

	if (final_x >= divisiones_ancho) final_x = divisiones_ancho-1;
	if (final_y >= divisiones_alto)  final_y = divisiones_alto-1;

	i = 0;
	for (y = inicio_y; y <= final_y; y++) {
		if (y == inicio_y || y == final_y) {
			for (x = inicio_x; x <= final_x; x++) {
				resultado[i++] = y * divisiones_ancho + x;
			}
		} else {
			resultado[i++] = y * divisiones_ancho + inicio_x;
			resultado[i++] = y * divisiones_ancho + final_x;
		}
	}

	return resultado;
}

static void cercanos_en_celdas(int 		  	  celda,
							   float	     *distancias,
							   float	     *angulos,
							   nodo_punto    *punt,
							   nodo_punto   **mas_cercanos,
							   lista_puntos **centros) {
	punto		 vector;
	nodo_punto  *iterador;
	float		 distancia, angulo;
	int			 mayor, i, n_nodos;

	clock_t fin, ini;
	//LOGI("cercanos_en_celdas running");
	ini = clock() / (CLOCKS_PER_SEC / 1000);

	mayor = 0;
	for (i = 0; i < cantidad_cercanos && distancias[i] >= 0; i++) {
		if (distancias[i] > distancias[mayor]) {
			mayor = i;
		}
	}

	if (i == cantidad_cercanos)
		n_nodos = cantidad_cercanos-1;
	else
		n_nodos = i;

	for (iterador = centros[celda] -> first;
			iterador != 0; iterador = iterador -> next) {
		vector.x = iterador -> p -> x - punt -> p -> x;
		vector.y = iterador -> p -> y - punt -> p -> y;

		if (punt -> p -> x != iterador -> p -> x) {
			distancia = sqrt( pow(vector.x, 2) + pow(vector.y, 2));

			angulo = acos((double)vector.x/(double)distancia);
			if (vector.y < 0) {
				angulo = 2 * M_PI - angulo;
			}

			if (distancia < distancias[mayor]) {
				distancias[mayor] = distancia;
				angulos[mayor] = angulo;
				mas_cercanos[mayor] = iterador;

				if (n_nodos < cantidad_cercanos) {
					n_nodos++;
				}

				for (i = 0; i < cantidad_cercanos || distancias[i] < 0; i++) {
					if (distancias[i] > distancias[mayor]) {
						mayor = i;
					}
				}
			}
		}
	}

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_cercanos_en_celdas = fin - ini;
}

static void buscar_mas_cercanos(int 		  celda,
							    int		   	  ancho,
							    int		      alto,
							    nodo_punto    *punt,
							    lista_puntos **centros) {

	nodo_punto  **mas_cercanos;
	float		*distancias, *angulos;
	int 		 i, j, k,  n_nodos, celda_actual, n_celdas, max_distancia;
	int 		*celdas_donde_buscar;

	clock_t fin, ini;
	//LOGI("buscar_mas_cercanos running");
	ini = clock() / (CLOCKS_PER_SEC / 1000);

	n_nodos = 0;
	max_distancia = ancho*alto;

	distancias = (float *)malloc(cantidad_cercanos*sizeof(float));
	for (i = 0; i < cantidad_cercanos; i++) {
		distancias[i] = max_distancia;
	}

	angulos = (float *)malloc(cantidad_cercanos*sizeof(float));

	mas_cercanos = (nodo_punto **)malloc(cantidad_cercanos*sizeof(nodo_punto *));

	/*
	 * Buscamos en la misma celda
	 */
	celda_actual = celda;
	cercanos_en_celdas(celda_actual, distancias,   angulos,
					   punt,         mas_cercanos, centros);
	for (i = 0; i < cantidad_cercanos && distancias[i] < max_distancia; i++);

	if (i == cantidad_cercanos)
		n_nodos = cantidad_cercanos-1;
	else
		n_nodos = i;

	/*
	 * Buscamos en las celdas adyacentes
	 */
	celdas_donde_buscar = celdas_a_calcular(celda, 1, &n_celdas);

	for (k = 0; k<n_celdas && celdas_donde_buscar[k]!=-1; k++) {
		celda_actual = celdas_donde_buscar[k];

		cercanos_en_celdas(celda_actual, distancias,   angulos,
						   punt,         mas_cercanos, centros);
		for (i = 0; i < cantidad_cercanos && distancias[i] < max_distancia; i++);
		n_nodos = i;
	}

	//LOGI("buscar_mas_cercanos 2 %d", n_centros);
	for (j = 2; n_nodos < cantidad_cercanos && n_nodos < n_centros - 1; j++) {
		//LOGI("buscar_mas_cercanos 2 compoenentes %d iteracion: %d", n_centros, j); //Poner un límite (y si solo hay un punto?)
		celdas_donde_buscar = celdas_a_calcular(celda, j, &n_celdas);

		for (k = 0; k<n_celdas && celdas_donde_buscar[k]!=-1; k++) {
			celda_actual = celdas_donde_buscar[k];

			cercanos_en_celdas(celda_actual, distancias,   angulos,
							   punt,         mas_cercanos, centros);
			for (i = 0; i < cantidad_cercanos && distancias[i] < max_distancia; i++);
			n_nodos = i;
		}
	}

	for (i = 0; i < n_nodos; i++) {
		add_punto_cercano(punt, mas_cercanos[i] -> p, angulos[i]);
	}

	free(distancias);
	free(celdas_donde_buscar);

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_buscar_mas_cercanos = fin - ini;
}

static lista_puntos **clasificar_centros(lista_puntos *centros,
									    int		     ancho,
									    int		     alto) {

	lista_puntos **clasificados;
	nodo_punto 	 *nodo;
	int 		  n_celdas, i, x, y;
	int 		 *limites_x, *limites_y;
	clock_t fin, ini;

	ini = clock() / (CLOCKS_PER_SEC / 1000);

	//LOGI("clasificar_centros running");
	n_celdas = divisiones_ancho * divisiones_alto;

	clasificados = (lista_puntos **)malloc(n_celdas * sizeof(lista_puntos *));

	for (i=0; i < n_celdas; i++) {
		clasificados[i] = new_lista_puntos();
	}

	limites_x = (int *)malloc(divisiones_ancho*sizeof(int));
	for (i = 0; i < divisiones_ancho; i++) {
		limites_x[i] = (ancho * (i+1))/divisiones_ancho;
	}

	limites_y = (int *)malloc(divisiones_alto*sizeof(int));
	for (i = 0; i < divisiones_alto; i++) {
		limites_y[i] = (alto * (i+1))/divisiones_alto;
	}

	for (nodo = centros -> first; nodo != 0; nodo = nodo -> next) {
		for (x = 0; x < divisiones_ancho && !(nodo -> p -> x < limites_x[x]); x++);
		for (y = 0; y < divisiones_alto  && !(nodo -> p -> y < limites_y[y]); y++);

		if (nodo -> p -> x == 775 && nodo -> p -> y == 2590) {
			add_punto(clasificados[y * divisiones_ancho + x], nodo -> p);
		} else {
			add_punto(clasificados[y * divisiones_ancho + x], nodo -> p);
		}
	}

	free(limites_x);
	free(limites_y);

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_clasificar_centros = fin - ini;

	return clasificados;
}



static lista_puntos **encontrar_centros(JNIEnv *env, jobject original) {
	nodo_punto *nodo;
	lista_puntos *centros;
	lista_puntos **centros_clasificados;
	int nCeldas, j;
	AndroidBitmapInfo  info_src;
	int ret;
	int image_width, image_height;
	clock_t fin, ini;

	ini = clock() / (CLOCKS_PER_SEC / 1000);

	//LOGI("encontrar_centros running");

	if ((ret = AndroidBitmap_getInfo(env, original, &info_src)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return 0;
	}

	image_width  = info_src.width;
	image_height = info_src.height;


	centros = encontrar_centroides(env, original);

	n_centros = centros -> size;

	//LOGI("encontrar_centros running n_centros %d cantidad_cercanos %d", n_centros, cantidad_cercanos);
	if (n_centros > cantidad_cercanos) {
		centros_clasificados = clasificar_centros(centros,
												  image_width,
												  image_height);

		nCeldas = divisiones_ancho * divisiones_alto;
		for (j = 0; j < nCeldas; j++) {
			for (nodo = centros_clasificados[j] -> first;
					nodo != 0;
					nodo = nodo -> next) {
				buscar_mas_cercanos(j,
									image_width,
									image_height,
									nodo,
									centros_clasificados);
			}
		}
	}

	fin = clock() / (CLOCKS_PER_SEC / 1000);

	t_encontrar_centros = fin - ini;

	return centros_clasificados;
}
//callback function for slider , implements erosion
static int pre_procesar(JNIEnv *env, jobject original, jobject a_modificar) {

	int error;
	clock_t fin, ini;

	ini = clock() / (CLOCKS_PER_SEC / 1000);

	//LOGI("pre_procesar running");
	error = convertir_a_grises(env, original, a_modificar);

	if (!error) {
		error = adaptive_threshold(env, a_modificar, a_modificar, 255, 9);
		error = suavizar(env, a_modificar, a_modificar);
		error = threshold(env, a_modificar, a_modificar, 210);
	} else {
		fprintf(stderr, "La imagen de origen no tiene tres canales de color");
		exit(-1);
	}

	fin = clock() / (CLOCKS_PER_SEC / 1000);

	t_preprocesar = fin-ini;

	return error;
}

JNIEXPORT jfloatArray JNICALL Java_com_uab_pfc_androidDescriptor_NativeDescriptor_descriptor
  (JNIEnv *env, jobject obj, jobject bitmapOrig, jobject bitmap) {

	float 		 **descriptor;
	lista_puntos **centroides;
	int ret;
	clock_t fin, ini;
	float *prof = (float *)malloc(15*sizeof(float));
	float *t_preproceso;

	jfloatArray result;
	result = (*env)->NewFloatArray(env, 15);
	if (result == NULL) {
	    return NULL; /* out of memory error thrown */
	}

	descriptor = 0;
	centroides = 0;

	t_total = -1.0;
	t_suavizado = -1.0;
	t_adaptive_threshold = -1.0;
	t_convertir_grises = -1.0;
	t_threshold = -1.0;
	t_encontrar_centroides = -1.0;
	t_cross_ratio = -1.0;
	t_buscar_mas_cercanos = -1.0;
	t_calcular_area = -1.0;
	t_cercanos_en_celdas = -1.0;
	t_calcular_descriptor_documento = -1.0;
	t_clasificar_centros = -1.0;
	t_encontrar_centros = -1.0;
	t_preprocesar = -1.0;

	//LOGI("descriptor running");
	ini = clock() / (CLOCKS_PER_SEC / 1000);
	ret = pre_procesar(env, bitmapOrig, bitmap);

	if (ret == 0) {
		centroides = encontrar_centros(env, bitmap);

		if (n_centros > cantidad_cercanos)
			descriptor = calcular_descriptor_documento(centroides);
	}
	fin = clock() / (CLOCKS_PER_SEC / 1000);

	if (descriptor != 0)
		free(descriptor);

	if (descriptor != 0)
		free(centroides);

	/*Estructura para el tiempo*/


	t_total = fin-ini;

	t_preproceso = info_tiempos();

	prof[0] = n_centros;
	prof[1] = t_total;
	prof[2] = t_preproceso[0]; /* suavizado */
	prof[3] = t_preproceso[1]; /* adaptive_threshold */
	prof[4] = t_preproceso[2]; /* convertir_a_grises */
	prof[5] = t_preproceso[3]; /* threshold */
	prof[6] = t_preproceso[4]; /* encontrar_centroides */
	prof[7] = t_cross_ratio;
	prof[8] = t_buscar_mas_cercanos;
	prof[9] = t_calcular_area;
	prof[10] = t_cercanos_en_celdas;
	prof[11] = t_calcular_descriptor_documento;
	prof[12] = t_clasificar_centros;
	prof[13] = t_encontrar_centros;
	prof[14] = t_preprocesar;

	(*env)->SetFloatArrayRegion(env, result, 0, 15, prof);

	free(t_preproceso);
	free(prof);

	return result;
}

JNIEXPORT jint JNICALL Java_com_uab_pfc_androidDescriptor_NativeDescriptor_add
  (JNIEnv *env, jobject obj, jint val1, jint val2) {
	return (val1 + val2);
}

JNIEXPORT jstring JNICALL Java_com_uab_pfc_androidDescriptor_NativeDescriptor_helloWorld
  (JNIEnv *env, jobject obj) {
	return (*env)->NewStringUTF(env, "Hello World!");
}

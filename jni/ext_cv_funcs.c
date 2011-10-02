/*
 * ext_cv_funcs.c
 *
 *  Created on: 24/07/2011
 *      Author: kintaro
 */

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <math.h>
#include <stdlib.h>
#include <setjmp.h>
#include <time.h>

#include "lista_puntos.h"

#define  LOG_TAG    "AndroidDescriptor"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

enum { SOME_EXCEPTION = 1 };
jmp_buf state;

static uint8_t *data;
static int    step;

static float t_suavizar,
			 t_adaptive_threshold,
			 t_convertir_a_grises,
			 t_threshold,
			 t_encontrar_centroides;

typedef struct
{
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} argb;

typedef struct {
	int    height;
	int	   width;
	float *matriz;
}Nucleo;

int convertir_a_grises (JNIEnv * env, jobject src, jobject res) {
	AndroidBitmapInfo  info_src;
	void*              pixels_src;
	AndroidBitmapInfo  info_res;
	void*              pixels_res;
	argb 			  *src_line;
	uint8_t 		  *res_line;
	int                ret;
	int             y;
	int             x;

	clock_t fin, ini;

	ini = clock() / (CLOCKS_PER_SEC / 1000);


	//LOGI("convertir_a_grises running");
	if ((ret = AndroidBitmap_getInfo(env, src, &info_src)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return -1;
	}


	if ((ret = AndroidBitmap_getInfo(env, res, &info_res)) < 0) {
		LOGE("convertir_a_grises: AndroidBitmap_getInfo() failed ! error=%d", ret);
		return -1;
	}


	//////LOGI("convertir_a_grises: color image :: width is %d; height is %d;"
	//		"stride is %d; format is %d;flags is %d",
	//		info_src.width,info_src.height,
	//		info_src.stride,info_src.format,info_src.flags);
	if (info_src.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("convertir_a_grises: Bitmap format is not RGBA_8888 !");
		return -1;
	}


	//////LOGI("convertir_a_grises: gray image :: width is %d; height is %d;"
	//		"stride is %d; format is %d;flags is %d",
	//		info_res.width,info_res.height,
	//		info_res.stride,info_res.format,info_res.flags);
	if (info_res.format != ANDROID_BITMAP_FORMAT_A_8) {
		LOGE("convertir_a_grises: Bitmap format is not A_8 !");
		return -1;
	}


	if ((ret = AndroidBitmap_lockPixels(env, src, &pixels_src)) < 0) {
		LOGE("convertir_a_grises: AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	if ((ret = AndroidBitmap_lockPixels(env, res, &pixels_res)) < 0) {
		LOGE("convertir_a_grises: AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	// modify pixels with image processing algorithm

	//////LOGI("convertir_a_grises: time to modify pixels....");

	for (y=0;y<info_src.height;y++) {
		src_line = (argb *) pixels_src;
		res_line = (uint8_t *) pixels_res;
		for (x=0;x<info_src.width;x++) {
			res_line[x] = 0.3  * src_line[x].red +
						  0.59 * src_line[x].green +
						  0.11 * src_line[x].blue;
		}

		pixels_src = (char *)pixels_src + info_src.stride;
		pixels_res  = (char *)pixels_res + info_res.stride;
	}

	//////LOGI("convertir_a_grises: unlocking pixels");
	AndroidBitmap_unlockPixels(env, src);
	AndroidBitmap_unlockPixels(env, res);

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_convertir_a_grises = fin - ini;

	return 0;
}

int adaptive_threshold (JNIEnv * env, jobject src, jobject res, int max_value, int c) {
 	float kernel[9][9] = {{0.0004, 0.0012, 0.0026, 0.0040, 0.0046, 0.0040, 0.0026, 0.0012, 0.0004 },
 						  {0.0012, 0.0034, 0.0072, 0.0111, 0.0128, 0.0111, 0.0072, 0.0034, 0.0012 },
 						  {0.0026, 0.0072, 0.0149, 0.0230, 0.0267, 0.0230, 0.0149, 0.0072, 0.0026 },
 						  {0.0040, 0.0111, 0.0230, 0.0357, 0.0413, 0.0357, 0.0230, 0.0111, 0.0040 },
 						  {0.0046, 0.0128, 0.0267, 0.0413, 0.0478, 0.0413, 0.0267, 0.0128, 0.0046 },
 						  {0.0040, 0.0111, 0.0230, 0.0357, 0.0413, 0.0357, 0.0230, 0.0111, 0.0040 },
 						  {0.0026, 0.0072, 0.0149, 0.0230, 0.0267, 0.0230, 0.0149, 0.0072, 0.0026 },
 						  {0.0012, 0.0034, 0.0072, 0.0111, 0.0128, 0.0111, 0.0072, 0.0034, 0.0012 },
 						  {0.0004, 0.0012, 0.0026, 0.0040, 0.0046, 0.0040, 0.0026, 0.0012, 0.0004 }};


    float v_acum, p_acum;
    int width , height;
    int i, j, x, y;

    AndroidBitmapInfo  info_src;
    void*              pixels_src;
    AndroidBitmapInfo  info_res;
    void*              pixels_res;
    int                ret;
    uint8_t            *src_data;
    uint8_t            *ret_data;
    int asdf =0;

    clock_t fin, ini;
    ini = clock() / (CLOCKS_PER_SEC / 1000);

    //LOGI("adaptive_threshold running");


    if ((ret = AndroidBitmap_getInfo(env, src, &info_src)) < 0) {
        LOGE("adaptive_threshold: AndroidBitmap_getInfo() failed ! error=%d", ret);
        return -1;
    }


    if ((ret = AndroidBitmap_getInfo(env, res, &info_res)) < 0) {
        LOGE("adaptive_threshold: AndroidBitmap_getInfo() failed ! error=%d", ret);
        return -1;
    }



    /*////LOGI("adaptive_threshold: gray image :: width is %d; height is %d; "
    		"stride is %d; format is %d;flags is %d",
    		info_src.width,info_src.height,info_src.stride,
    		info_src.format,info_src.flags);*/
    if (info_src.format != ANDROID_BITMAP_FORMAT_A_8) {
        LOGE("adaptive_threshold: Bitmap format is not A_8 !");
        return -1;
    }

    /*////LOGI("adaptive_threshold: color image :: width is %d; height is %d; "
    		"stride is %d; format is %d;flags is %d",
    		info_res.width,info_res.height,
    		info_res.stride,info_res.format,info_res.flags);*/
    if (info_res.format != ANDROID_BITMAP_FORMAT_A_8) {
        LOGE("adaptive_threshold: Bitmap format is not A_8 !");
        return -1;
    }



    if ((ret = AndroidBitmap_lockPixels(env, src, &pixels_src)) < 0) {
        LOGE("adaptive_threshold: AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    if ((ret = AndroidBitmap_lockPixels(env, res, &pixels_res)) < 0) {
        LOGE("adaptive_threshold: AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }


    // modify pixels with image processing algorithm


    //////LOGI("adaptive_threshold: time to modify pixels....");

    src_data = (uint8_t *) pixels_src;
    ret_data = (uint8_t *) pixels_res;

    width  = info_src.width;
    height = info_src.height;

    v_acum = 0;
    p_acum = 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
        	//////LOGI("New iteration y = %d, x = %d, wid = %d, hei = %d, asdf = %d" , y, x, width, height, asdf );
			for (i=-4; i<=4; i++) {
				if ((y + i >= 0) && (y + i < height)) {
					for (j=-4;j<=4;j++) {
						if ((x + j >= 0) && (x + j < width)) {
							//////LOGI("y = %d, x = %d, i = %d, j = %d, wid = %d, hei = %d", y, x, i, j, width, height);
							v_acum += (float) ((uint8_t)(*(src_data + x + j + (y + i) * info_src.stride))) * kernel[i+4][j+4];
							p_acum += kernel[i+4][j+4];
						}
					}
				}
			}
			//////LOGI("Modificamos");
			if (((uint8_t)(*(src_data + x + y * info_src.stride))) >
				((uint8_t)(v_acum / p_acum) - c))
				*(ret_data + x + y*info_src.stride) = (uint8_t)max_value;
			else
				*(ret_data + x + y*info_src.stride) = (uint8_t)0;

            v_acum = 0;
            p_acum = 0;
            asdf++;
        }
    }

    //////LOGI("adaptive_threshold: unlocking pixels");
    AndroidBitmap_unlockPixels(env, src);
    AndroidBitmap_unlockPixels(env, res);

   	fin = clock() / (CLOCKS_PER_SEC / 1000);
    t_adaptive_threshold = fin - ini;

    return 0;
}

int suavizar(JNIEnv * env, jobject src, jobject res) {
    float v_acum, p_acum;
    int width , height;
    int i, j, x, y;

    AndroidBitmapInfo  info_src;
    void*              pixels_src;
    AndroidBitmapInfo  info_res;
    void*              pixels_res;
    int                ret;
    uint8_t            *src_data;
    uint8_t            *ret_data;

    clock_t fin, ini;
    ini = clock() / (CLOCKS_PER_SEC / 1000);

    //LOGI("suavizar running");

    if ((ret = AndroidBitmap_getInfo(env, src, &info_src)) < 0) {
        LOGE("suavizar: AndroidBitmap_getInfo() failed ! error=%d", ret);
        return -1;
    }


    if ((ret = AndroidBitmap_getInfo(env, res, &info_res)) < 0) {
        LOGE("suavizar: AndroidBitmap_getInfo() failed ! error=%d", ret);
        return -1;
    }



    /*////LOGI("suavizar: gray image :: width is %d; height is %d; "
    		"stride is %d; format is %d;flags is %d",
    		info_src.width,info_src.height,info_src.stride,
    		info_src.format,info_src.flags);*/
    if (info_src.format != ANDROID_BITMAP_FORMAT_A_8) {
        LOGE("suavizar: Bitmap format is not A_8 !");
        return -1;
    }

    /*////LOGI("suavizar: color image :: width is %d; height is %d; "
    		"stride is %d; format is %d;flags is %d",
    		info_res.width,info_res.height,
    		info_res.stride,info_res.format,info_res.flags);*/
    if (info_res.format != ANDROID_BITMAP_FORMAT_A_8) {
        LOGE("suavizar: Bitmap format is not A_8 !");
        return -1;
    }



    if ((ret = AndroidBitmap_lockPixels(env, src, &pixels_src)) < 0) {
        LOGE("suavizar: AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    if ((ret = AndroidBitmap_lockPixels(env, res, &pixels_res)) < 0) {
        LOGE("suavizar: AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }


    // modify pixels with image processing algorithm


    //////LOGI("suavizar: time to modify pixels....");

    src_data = (uint8_t *) pixels_src;
    ret_data = (uint8_t *) pixels_res;

    width  = info_src.width;
    height = info_src.height;

    v_acum = 0;
    p_acum = 0;

    for (y = 0; y < info_src.height; y++) {
        for (x = 0; x < info_src.width; x++) {
			// calc X gradient
			for (i=-1;i<=1;i++) {
				if ((y + i >= 0) && (y + i < height)) {
					for (j=-2;j<=2;j++) {
						if ((x + j >= 0) && (x + j < width)) {
							v_acum += (float) (*(src_data + x + j + (y + i)
													* info_src.stride));
							p_acum ++;
						}
					}
				}
			}

            *(ret_data + x + y*info_src.width) = ((uint8_t)(v_acum / p_acum));

            v_acum = 0;
            p_acum = 0;
        }
    }

	//////LOGI("suavizar: unlocking pixels");
    AndroidBitmap_unlockPixels(env, src);
    AndroidBitmap_unlockPixels(env, res);

    fin = clock() / (CLOCKS_PER_SEC / 1000);
    t_suavizar = fin - ini;

    return 0;
}

int threshold (JNIEnv * env, jobject src, jobject res, int max_value) {
	AndroidBitmapInfo  info_src;
	void*              pixels_src;
	AndroidBitmapInfo  info_res;
	void*              pixels_res;
	uint8_t *src_line;
	uint8_t *res_line;
	int                ret;
	int             y;
	int             x;

	clock_t fin, ini;
	ini = clock() / (CLOCKS_PER_SEC / 1000);

	//LOGI("threshold running");
	if ((ret = AndroidBitmap_getInfo(env, src, &info_src)) < 0) {
		LOGE("threshold: AndroidBitmap_getInfo() failed ! error=%d", ret);
		return -1;
	}


	if ((ret = AndroidBitmap_getInfo(env, res, &info_res)) < 0) {
		LOGE("threshold: AndroidBitmap_getInfo() failed ! error=%d", ret);
		return -1;
	}


	/*//LOGI("threshold: original image :: width is %d; height is %d;"
			"stride is %d; format is %d;flags is %d",
			info_src.width,info_src.height,
			info_src.stride,info_src.format,info_src.flags);*/
	if (info_src.format != ANDROID_BITMAP_FORMAT_A_8) {
		LOGE("threshold: Bitmap format is not A_8 !");
		return -1;
	}


	/*//LOGI("threshold: thresholded image :: width is %d; height is %d;"
			"stride is %d; format is %d;flags is %d",
			info_res.width,info_res.height,
			info_res.stride,info_res.format,info_res.flags);*/
	if (info_res.format != ANDROID_BITMAP_FORMAT_A_8) {
		LOGE("threshold: Bitmap format is not A_8 !");
		return -1;
	}


	if ((ret = AndroidBitmap_lockPixels(env, src, &pixels_src)) < 0) {
		LOGE("threshold: AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	if ((ret = AndroidBitmap_lockPixels(env, res, &pixels_res)) < 0) {
		LOGE("threshold: AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	// modify pixels with image processing algorithm

	////LOGI("threshold: time to modify pixels....");

	for (y=0;y<info_src.height;y++) {
		src_line = (uint8_t *) pixels_src;
		res_line = (uint8_t *) pixels_res;
		for (x=0;x<info_src.width;x++) {
			if ((int)src_line[x] > max_value)
				res_line[x] = (uint8_t) 255;
			else
				res_line[x] = (uint8_t) 0;
		}

		pixels_src = (char *)pixels_src + info_src.stride;
		pixels_res = (char *)pixels_res + info_res.stride;
	}

	////LOGI("threshold: unlocking pixels");
	AndroidBitmap_unlockPixels(env, src);
	AndroidBitmap_unlockPixels(env, res);

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_threshold = fin - ini;

	return 0;
}

/*****************************************************************/
/********** Codigo basado en el algoritmo para encontrar *********/
/**** connected components, cortesia de Dimosthenis Karatzas *****/
/*****************************************************************/

/* Funcion que devuelve el color del pixel
 * situado en las coordenadas (j,i) de la imagen
 */
static uint8_t PrivateGetByte(int i, int j) {
	return (uint8_t) (*(data + i * step + j));
}


/* Funcion que busca connected components en la imagen
 * devolviendo una lista de sus centros de gravedad
 */
lista_puntos *encontrar_centroides (JNIEnv * env, jobject src) {

	/* deficion de una estructura para describir
	 * secuencias de pixeles en una linea
	 */
	typedef struct {
	   int  xl;     /* posicion inicial (numero de columna */
	   int  xr;     /* posicion final (x1 <= x2)           */
	   int  y;      /* numero de fila                      */
	   long acum_x;
	   long acum_y;
	   long n_pix;
	   int  id;     /* el id de componente que lo contiene */
	} run ;

	/* deficion de una estructura para describir
	 * secuencias de pixeles en una linea (sin id)
	 */
	typedef struct {
		int xl;
		int xr;
		int y;
	} CompRun ;

	/* definicion de la estructura que describira un
	 * componente (los unicos datos que nos interesan
	 * son los necesarios para el calculo del centro de gravedad)
	 */
	typedef struct {
		int 	 in_use;
		long 	 acum_x;
		long 	 acum_y;
		long 	 n_pix;
	} Components;


	/* Para encontrar los connected components
	 * necesitamos una imane en blanco y negro
	 * Fondo = Blanco, frente = negro;
	 */
	const uint8_t BILEVEL_BLACK = 0;
	const uint8_t BILEVEL_WHITE = 255;

	const int COMPNT_ALLOC_STEP = 50;


	/* Variables para el tratamiento de los centros
	 * de gravedad de los connected components.
	 */
	lista_puntos *centros;
	punto       *p;

	int image_height;
	int image_width;

	int ConnectivityConstant;

	/* definiciones generales */
	int i, j, k, l;

	/* Definiciones para el analisis normal */
	int in_black;
	int above_runs_num, left_runs_num;  /* contadores para las secuencias
										   superiores e izquierdas */
	int overlaps;    /* numero de secuencias superiores
						que superponen a la actual */
	int new_compnt_label;
	int allocation_size;
	int n_active_components;
	Components *component;
	run *above_run, *left_run, this_run;


	clock_t fin, ini;
	ini = clock() / (CLOCKS_PER_SEC / 1000);

	/*********************************************************/
	/***** Codigo a modificar en la version para Android *****/
	/*********************************************************/

	AndroidBitmapInfo  info_src;
	void*              pixels_src;
	int                ret;
	uint8_t            *src_data;

	//LOGI("encontrar_centroides running");

	if ((ret = AndroidBitmap_getInfo(env, src, &info_src)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return 0;
	}



	/*//LOGI("gray image :: width is %d; height is %d; "
			"stride is %d; format is %d;flags is %d",
			info_src.width,info_src.height,info_src.stride,
			info_src.format,info_src.flags);*/
	if (info_src.format != ANDROID_BITMAP_FORMAT_A_8) {
		LOGE("Bitmap format is not A_8 !");
		return 0;
	}

	if ((ret = AndroidBitmap_lockPixels(env, src, &pixels_src)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}


	// modify pixels with image processing algorithm


	////LOGI("time to modify pixels....");

	data = (uint8_t *) pixels_src;

	image_width  = info_src.width;
	image_height = info_src.height;

	step = info_src.stride;
	/*********************************************************/
	/*********************************************************/
	/*********************************************************/


	/* 0 para conectividad a 4 */
	/* 1 para conectividad a 8 */
	ConnectivityConstant = 1;

	/* reserva de memoria para las secuencias y los componentes */
	above_run = (run *) malloc ((image_width/2+1)*sizeof(run));
	left_run  = (run *) malloc ((image_width/2+1)*sizeof(run));
	component = (Components *) malloc ((COMPNT_ALLOC_STEP)*sizeof(Components));
	allocation_size = COMPNT_ALLOC_STEP;

	above_runs_num      = 0;
	n_active_components = 0;
	new_compnt_label    = 0;

	this_run.acum_x = 0;
	this_run.acum_y = 0;
	this_run.n_pix  = 0;

	for (i = 0; i < image_height; i++) {
		in_black = 0;
		left_runs_num = 0;  /* en este punto no hay secuencias a la izquierda */

		for (j = 0; j < image_width; j++) {
			if ( PrivateGetByte(i, j) == BILEVEL_BLACK ) {
				if (!in_black) {
					/* hemos encontrado el principio de una secuencia */
					this_run.xl = j;
					this_run.y = i;
					in_black = 1;
				}
				
				/* Acumulamos las coordenadas, asi como el numero
				 * de pixeles para calcluar el centro de gravedad
				 */
				this_run.acum_x += j;
				this_run.acum_y += i;
				this_run.n_pix++;
			}

			if ( (in_black && PrivateGetByte(i, j) == BILEVEL_WHITE ) ||
				 (j == (image_width - 1) && in_black))				{
				/* hemos encontrado el final de la secuencia */
				in_black = 0;
				
				/* asignamos la posicion del ultimo pixel de la secuencia */
				if (j == image_width - 1) {
					if ( PrivateGetByte(i, j) == BILEVEL_WHITE ) {
						this_run.xr = j-1;
					} else {
						/* El ultimo pixel es del frente */
						this_run.xr = j;
				    }
				} else {
					this_run.xr = j - 1;
				}

				/* comprobamos si hay superposiciones entre
				 * la secuencia actual y las superiores
				 * dependiende de la conectividad (a 4 o a 8)
				 */
				overlaps = 0;  /* en principio no hay superposiciones */
				for (k = 0; k < above_runs_num; ++k) {
				
					if ((above_run[k].xl - ConnectivityConstant <= this_run.xr) &&
						(this_run.xl <= above_run[k].xr + ConnectivityConstant)) {
					 /* Superposicion */
						
					    /* Asignamos a la secuencia el id
					     * de la secuencia superior si es
					     * la primera vez que se superponen
					     */
						if (overlaps == 0) {
							this_run.id = above_run[k].id;
							
							/* Anyadimos esta secuencia al componente */
							component[this_run.id].acum_x += this_run.acum_x;
							component[this_run.id].acum_y += this_run.acum_y;
							component[this_run.id].n_pix  += this_run.n_pix;
						} else {
							/* No es la primera superposicion */

							if (this_run.id != above_run[k].id) {
								/* Las secuencias pertenecen a
								 * diferentes componentes
								 */

								component[this_run.id].acum_x +=
										component[above_run[k].id].acum_x;
								component[this_run.id].acum_y +=
										component[above_run[k].id].acum_y;
								component[this_run.id].n_pix  +=
										component[above_run[k].id].n_pix;
								
								/* El componente actual queda inhabilitado */
								component[above_run[k].id].in_use=0;
								n_active_components--;
								
								/* Cambiamos los ids de los componentes */
								int aboveID=above_run[k].id;
								
								for (l=0; l<above_runs_num; l++)
									if (above_run[l].id==aboveID)
										above_run[l].id=this_run.id;

								for (l=0; l<left_runs_num;l++)
									if (left_run[l].id==aboveID)
										left_run[l].id=this_run.id;
							}
						}
						
						overlaps++;
					}
				}

				if (overlaps == 0) {
					/* no hay superposiciones con las secuencias superiores */


					/* asignamos un id a la secuencia
					 * y empezamos un nuevo componente
					 */
					this_run.id = new_compnt_label;
					component[new_compnt_label].in_use = 1;
					
					component[new_compnt_label].acum_x = this_run.acum_x;
					component[new_compnt_label].acum_y = this_run.acum_y;
					component[new_compnt_label].n_pix  = this_run.n_pix;
					
					new_compnt_label++;
					n_active_components++;

					/* Comprobar si necesitamos mas memoria */
					if (new_compnt_label == allocation_size) {
						Components * temp = (Components *) malloc
								((allocation_size + COMPNT_ALLOC_STEP) *
										sizeof(Components));

						memcpy(temp, component, allocation_size *
													sizeof(Components));

						free(component);
						component = temp;
						allocation_size += COMPNT_ALLOC_STEP;
					}
				}
				
				/* insertamos la secuencia actual en el
				 * array de secuencias a la izquiera
				 */
				left_run[left_runs_num].id = this_run.id;
				left_run[left_runs_num].xl = this_run.xl;
				left_run[left_runs_num].xr = this_run.xr;
				left_run[left_runs_num].y = this_run.y;
				left_runs_num++;

				this_run.acum_x = 0;
				this_run.acum_y = 0;
				this_run.n_pix  = 0;
			}
		}

		/* movemos todas las secuencias del array de secuencias
		 * a la izquierda(en este punto, todas las secuencias de
		 * la linea) al array de secuencias superiores
		 */
		for (k = 0; k < left_runs_num; ++k) {
			above_run[k].id = left_run[k].id;
			above_run[k].xl = left_run[k].xl;
			above_run[k].xr = left_run[k].xr;
			above_run[k].y = left_run[k].y;
		}
		
		above_runs_num = left_runs_num;
	}

	free(left_run);
	free(above_run);

	/* Una vez tenemos los datos necesarios
	 * de cada connected component, calculamos
	 * sus centros de gravedad
	 */
	centros = new_lista_puntos();
	for (k=0;k<new_compnt_label;k++) {
		if (component[k].in_use) {
			p = new_punto();
			p -> x = component[k].acum_x/component[k].n_pix;
			p -> y = component[k].acum_y/component[k].n_pix;

			add_punto(centros, p);
		}
	}
	
	free(component);

	////LOGI("unlocking pixels");
	AndroidBitmap_unlockPixels(env, src);

	fin = clock() / (CLOCKS_PER_SEC / 1000);
	t_encontrar_centroides = fin - ini;

	return centros;
}

float *info_tiempos (){
	float *res = (float *) malloc (5*sizeof(float));

	res [0] = t_suavizar;
	res [1] = t_adaptive_threshold;
	res [2] = t_convertir_a_grises;
	res [3] = t_threshold;
	res [4] = t_encontrar_centroides;

	return res;
}

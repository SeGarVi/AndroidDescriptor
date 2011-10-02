/*
 * ext_cv_funcs.h
 *
 *  Created on: Aug 11, 2011
 *      Author: kintaro
 */

#ifndef EXT_CV_FUNCS_H_
#define EXT_CV_FUNCS_H_

int convertir_a_grises(JNIEnv * env, jobject src, jobject ret);
int adaptive_threshold (JNIEnv * env, jobject src, jobject res, int max_value, int c);
int threshold (JNIEnv * env, jobject bitmapgray, jobject src, int max_value);
int suavizar(JNIEnv * env, jobject bitmapgray,jobject bitmapedges);
lista_puntos *encontrar_centroides (JNIEnv * env, jobject src);
float *info_tiempos();

#endif /* EXT_CV_FUNCS_H_ */

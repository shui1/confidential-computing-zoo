/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class GramineJni_gramine_jni */

#ifndef _Included_GramineJni_gramine_jni
#define _Included_GramineJni_gramine_jni
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     GramineJni_gramine_jni
 * Method:    get_key
 * Signature: ([B[B[BI)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_get_1key
  (JNIEnv *, jobject, jbyteArray, jbyteArray, jbyteArray, jint);

/*
 * Class:     GramineJni_gramine_jni
 * Method:    get_file_2_buff
 * Signature: ([B[B[BJ[BI[I)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_get_1file_12_1buff
  (JNIEnv *, jobject, jbyteArray, jbyteArray, jbyteArray, jlong, jbyteArray, jint, jintArray);

/*
 * Class:     GramineJni_gramine_jni
 * Method:    get_file_size
 * Signature: ([B[B[B[J)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_get_1file_1size
  (JNIEnv *, jobject, jbyteArray, jbyteArray, jbyteArray, jlongArray);

/*
 * Class:     GramineJni_gramine_jni
 * Method:    put_result
 * Signature: ([B[B[BJ[BI)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_put_1result
  (JNIEnv *, jobject, jbyteArray, jbyteArray, jbyteArray, jlong, jbyteArray, jint);

#ifdef __cplusplus
}
#endif
#endif
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
 * Signature: (Ljava/lang/String;Ljava/lang/String;[BI)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_get_1key
  (JNIEnv *, jobject, jstring, jstring, jbyteArray, jint);

/*
 * Class:     GramineJni_gramine_jni
 * Method:    get_file_2_buff
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J[BI[I)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_get_1file_12_1buff
  (JNIEnv *, jobject, jstring, jstring, jstring, jlong, jbyteArray, jint, jintArray);

/*
 * Class:     GramineJni_gramine_jni
 * Method:    get_file_size
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[J)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_get_1file_1size
  (JNIEnv *, jobject, jstring, jstring, jstring, jlongArray);

/*
 * Class:     GramineJni_gramine_jni
 * Method:    put_result
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J[BI[I)I
 */
JNIEXPORT jint JNICALL Java_GramineJni_gramine_1jni_put_1result
  (JNIEnv *, jobject, jstring, jstring, jstring, jlong, jbyteArray, jint, jintArray);

#ifdef __cplusplus
}
#endif
#endif
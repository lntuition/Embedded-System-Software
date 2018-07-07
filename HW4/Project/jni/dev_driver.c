#include <jni.h>
#include "android/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <asm/ioctl.h>

#define LOG_TAG "MyTag"
#define LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

// in JNI functions simply call kernel module functions
JNIEXPORT jint JNICALL Java_com_example_androidex_MainActivity2_open(JNIEnv *env, jobject obj){
	int fd = open("/dev/dev_driver", O_RDWR);
	return fd;
}

JNIEXPORT jint JNICALL Java_com_example_androidex_MainActivity2_ioctl(JNIEnv *env, jobject obj, jint fd, jint type){
	unsigned int buffer = 0;
	return ioctl(fd, _IOW(242, type, unsigned int), &buffer);
}

JNIEXPORT jint JNICALL Java_com_example_androidex_MainActivity2_close(JNIEnv *env, jobject obj, jint fd){
	return close(fd);
}

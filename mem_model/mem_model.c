 #define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>

int a, b = 0;
pthread_barrier_t barrier;

void *t1_f(void *p) {
  pthread_barrier_wait(&barrier);
  a = 1;
  if (b == 0) {
    printf("t1\n");
  }
  return NULL;
}

void *t2_f(void *p) {
  pthread_barrier_wait(&barrier);
  b = 1;
  if (a == 0) {
    printf("t2\n");
  }
  return NULL;
}

int main() {

  pthread_t t1, t2;
  pthread_attr_t t1_attr, t2_attr;
  pthread_attr_init(&t1_attr);
  pthread_attr_init(&t2_attr);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(1, &cpuset);
  pthread_attr_setaffinity_np(&t1_attr, sizeof(cpuset), &cpuset);
  CPU_ZERO(&cpuset);
  CPU_SET(3, &cpuset);
  pthread_attr_setaffinity_np(&t2_attr, sizeof(cpuset), &cpuset);
  pthread_barrier_init(&barrier, NULL, 2);
  int ret;
  if ((ret = pthread_create(&t1, &t1_attr, t1_f, NULL))) {
    fprintf (stderr, "%s", strerror (ret));
  }
  if ((ret = pthread_create(&t2, &t2_attr, t2_f, NULL))) {
    fprintf (stderr, "%s", strerror (ret));
  }

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  return 0;
}

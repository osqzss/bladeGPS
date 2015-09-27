#ifndef _BLADEGPS_H
#define _BLADEGPS_H

#include <stdlib.h>
#include <stdio.h>
#include <libbladeRF.h>
#include <string.h>
#include <pthread.h>

#define TX_FREQUENCY	1575420000
#define TX_SAMPLERATE	2600000
#define TX_BANDWIDTH	2500000
#define TX_VGA1			-25
#define TX_VGA2			0

#define NUM_BUFFERS			32
#define SAMPLES_PER_BUFFER	(32 * 1024)
#define NUM_TRANSFERS		16
#define TIMEOUT_MS			1000

#define NUM_IQ_SAMPLES  (TX_SAMPLERATE / 10)
#define FIFO_LENGTH     (NUM_IQ_SAMPLES * 2)

typedef struct {
	pthread_t thread;
	pthread_mutex_t lock;
	//int error;

	struct bladerf *dev;
	int16_t *buffer;
} tx_t;

typedef struct {
	pthread_t thread;
	pthread_mutex_t lock;
	//int error;

	int ready;
	pthread_cond_t initialization_done;
} gps_t;

typedef struct {
	tx_t tx;
	gps_t gps;

	int status;
	int16_t *fifo;
	long head, tail;
	size_t sample_length;

	pthread_cond_t fifo_write_ready;

	double time;
} sim_t;

extern void *gps_task(void *arg);
extern int is_fifo_write_ready(sim_t *s);

#endif

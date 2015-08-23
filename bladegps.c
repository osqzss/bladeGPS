#define _CRT_SECURE_NO_WARNINGS

#include "bladegps.h"

void init_sim(sim_t *s)
{
	s->tx.dev = NULL;
	pthread_mutex_init(&(s->tx.lock), NULL);
	//s->tx.error = 0;

	pthread_mutex_init(&(s->gps.lock), NULL);
	//s->gps.error = 0;
	s->gps.ready = 0;
	pthread_cond_init(&(s->gps.initialization_done), NULL);

	s->status = 0;
	s->head = 0;
	s->tail = 0;
	s->sample_length = 0;

	pthread_cond_init(&(s->fifo_write_ready), NULL);

	s->time = 0.0;
}

size_t get_sample_length(sim_t *s)
{
	long length;

	length = s->head - s->tail;
	if (length < 0)
		length += FIFO_LENGTH;

	return((size_t)length);
}

size_t fifo_read(int16_t *buffer, size_t samples, sim_t *s)
{
	size_t length;
	size_t samples_remaining;
	int16_t *buffer_current = buffer;

	length = get_sample_length(s);

	if (length < samples)
		samples = length;

	length = samples; // return value

	samples_remaining = FIFO_LENGTH - s->tail;

	if (samples > samples_remaining) {
		memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples_remaining * sizeof(int16_t) * 2);
		s->tail = 0;
		buffer_current += samples_remaining * 2;
		samples -= samples_remaining;
	}

	memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples * sizeof(int16_t) * 2);
	s->tail += (long)samples;
	if (s->tail >= FIFO_LENGTH)
		s->tail -= FIFO_LENGTH;

	return(length);
}

int is_fifo_write_ready(sim_t *s)
{
	int status = 0;

	s->sample_length = get_sample_length(s);
	if (s->sample_length < NUM_IQ_SAMPLES)
		status = 1;

	return(status);
}

void *tx_task(void *arg)
{
	sim_t *s = (sim_t *)arg;
	size_t samples_populated;

	while (1) {
		int16_t *tx_buffer_current = s->tx.buffer;
		unsigned int buffer_samples_remaining = SAMPLES_PER_BUFFER;

		while (buffer_samples_remaining > 0) {
			
			samples_populated = fifo_read(tx_buffer_current, 
						buffer_samples_remaining,
						s);

			if (is_fifo_write_ready(s)) {
				pthread_cond_signal(&(s->fifo_write_ready));

				printf("\rTime = %4.1f", s->time);
				s->time += 0.1;
				fflush(stdout);
			}

			// Advance the buffer pointer.
			buffer_samples_remaining -= (unsigned int)samples_populated;
			tx_buffer_current += (2 * samples_populated);
		}

		// If there were no errors, transmit the data buffer.
		bladerf_sync_tx(s->tx.dev, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, TIMEOUT_MS);
	}
}

int start_tx_task(sim_t *s)
{
	int status;

	status = pthread_create(&(s->tx.thread), NULL, tx_task, s);

	return(status);
}

int start_gps_task(sim_t *s)
{
	int status;

	status = pthread_create(&(s->gps.thread), NULL, gps_task, s);

	return(status);
}

int main(int argc, char *argv[])
{
	sim_t s;
	char *devstr = NULL;
	int c;

	// Initialize structures
	init_sim(&s);

	// Allocate TX buffer to hold each block of samples to transmit.
	s.tx.buffer = (int16_t *)malloc(SAMPLES_PER_BUFFER * sizeof(int16_t) * 2); // for 16-bit I and Q samples
	
	if (s.tx.buffer == NULL) {
		fprintf(stderr, "Failed to allocate TX buffer.\n");
		goto out;
	}

	// Allocate FIFOs to hold 0.1 seconds of I/Q samples each.
	s.fifo = (int16_t *)malloc(FIFO_LENGTH * sizeof(int16_t) * 2); // for 16-bit I and Q samples

	if (s.fifo == NULL) {
		fprintf(stderr, "Failed to allocate I/Q sample buffer.\n");
		goto out;
	}

	// Initializing device.
	printf("Opening and initializing device...\n");

	s.status = bladerf_open(&s.tx.dev, devstr);
	if (s.status != 0) {
		fprintf(stderr, "Failed to open device: %s\n", bladerf_strerror(s.status));
		goto out;
	}

	s.status = bladerf_set_frequency(s.tx.dev, BLADERF_MODULE_TX, TX_FREQUENCY);
	if (s.status != 0) {
		fprintf(stderr, "Faield to set TX frequency: %s\n", bladerf_strerror(s.status));
		goto out;
	} 
	else {
		printf("TX frequency: %u Hz\n", TX_FREQUENCY);
	}

	s.status = bladerf_set_sample_rate(s.tx.dev, BLADERF_MODULE_TX, TX_SAMPLERATE, NULL);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX sample rate: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		printf("TX sample rate: %u sps\n", TX_SAMPLERATE);
	}

	s.status = bladerf_set_bandwidth(s.tx.dev, BLADERF_MODULE_TX, TX_BANDWIDTH, NULL);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX bandwidth: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		printf("TX bandwidth: %u Hz\n", TX_BANDWIDTH);
	}

	s.status = bladerf_set_txvga1(s.tx.dev, TX_VGA1);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX VGA1 gain: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		printf("TX VGA1 gain: %d dB\n", TX_VGA1);
	}

	s.status = bladerf_set_txvga2(s.tx.dev, TX_VGA2);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX VGA2 gain: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		printf("TX VGA2 gain: %d dB\n", TX_VGA2);
	}

	// Start GPS task.
	s.status = start_gps_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start GPS task.\n");
		goto out;
	}
	else
		printf("Creating GPS task...\n");

	// Wait until GPS task is initialized
	pthread_mutex_lock(&(s.tx.lock));
	while (!s.gps.ready)
		pthread_cond_wait(&(s.gps.initialization_done), &(s.tx.lock));
	pthread_mutex_unlock(&(s.tx.lock));

	// Fillfull the FIFO.
	if (is_fifo_write_ready(&s))
		pthread_cond_signal(&(s.fifo_write_ready));

	// Configure the TX module for use with the synchronous interface.
	s.status = bladerf_sync_config(s.tx.dev,
			BLADERF_MODULE_TX,
			BLADERF_FORMAT_SC16_Q11,
			NUM_BUFFERS,
			SAMPLES_PER_BUFFER,
			NUM_TRANSFERS,
			TIMEOUT_MS);

	if (s.status != 0) {
		fprintf(stderr, "Failed to configure TX sync interface: %s\n", bladerf_strerror(s.status));
		goto out;
	}

	// We must always enable the modules *after* calling bladerf_sync_config().
	s.status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, true);
	if (s.status != 0) {
		fprintf(stderr, "Failed to enable TX module: %s\n", bladerf_strerror(s.status));
		goto out;
	}

	// Start TX task
	s.status = start_tx_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start TX task.\n");
		goto out;
	}
	else
		printf("Creating TX task...\n");

	// Running...
	printf("Running...\n");
	printf("Press 'q' to exit.\n");
	while (1) {
		c = _getch();
		if (c=='q')
			break;
	}

	//
	// TODO: Cleaning up the threads properly.
	//

	printf("\nDone!\n");

	// Disable TX module, shutting down our underlying TX stream.
	s.status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, false);
	if (s.status != 0) {
		fprintf(stderr, "Failed to disable TX module: %s\n", bladerf_strerror(s.status));
	}

out:
	// Free up resources
	if (s.tx.buffer != NULL)
		free(s.tx.buffer);

	if (s.fifo != NULL)
		free(s.fifo);

	printf("Closing device...\n");
	bladerf_close(s.tx.dev);

	return(0);
}

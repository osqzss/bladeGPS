#define _CRT_SECURE_NO_WARNINGS

#include "bladegps.h"

// for _getch used in Windows runtime.
#ifdef _WIN32
#include <conio.h>
#include "getopt.h"
#else
#include <unistd.h>
#endif

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
	pthread_cond_init(&(s->fifo_read_ready), NULL);

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

bool is_finished_generation(sim_t *s)
{
	return s->finished;
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
			
			pthread_mutex_lock(&(s->gps.lock));
			while (get_sample_length(s) == 0)
			{
				pthread_cond_wait(&(s->fifo_read_ready), &(s->gps.lock));
			}
//			assert(get_sample_length(s) > 0);

			samples_populated = fifo_read(tx_buffer_current,
				buffer_samples_remaining,
				s);
			pthread_mutex_unlock(&(s->gps.lock));

			pthread_cond_signal(&(s->fifo_write_ready));
#if 0
			if (is_fifo_write_ready(s)) {
				/*
				printf("\rTime = %4.1f", s->time);
				s->time += 0.1;
				fflush(stdout);
				*/
			}
			else if (is_finished_generation(s))
			{
				goto out;
			}
#endif
			// Advance the buffer pointer.
			buffer_samples_remaining -= (unsigned int)samples_populated;
			tx_buffer_current += (2 * samples_populated);
		}

		// If there were no errors, transmit the data buffer.
		bladerf_sync_tx(s->tx.dev, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, TIMEOUT_MS);
		if (is_fifo_write_ready(s)) {
			/*
			printf("\rTime = %4.1f", s->time);
			s->time += 0.1;
			fflush(stdout);
			*/
		}
		else if (is_finished_generation(s))
		{
			goto out;
		}

	}
out:
	return NULL;
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

void usage(void)
{
	printf("Usage: bladegps [options]\n"
		"Options:\n"
		"  -e <gps_nav>     RINEX navigation file for GPS ephemerides (required)\n"
		"  -u <user_motion> User motion file (dynamic mode)\n"
		"  -g <nmea_gga>    NMEA GGA stream (dynamic mode)\n"
		"  -l <location>    Lat,Lon,Hgt (static mode) e.g. 35.274,137.014,100\n"
		"  -t <date,time>   Scenario start time YYYY/MM/DD,hh:mm:ss\n"
		"  -T <date,time>   Overwrite TOC and TOE to scenario start time\n"
		"  -d <duration>    Duration [sec] (max: %.0f)\n"
		"  -x <XB number>   Enable XB board, e.g. '-x 200' for XB200\n"
		"  -a <tx_vga1>     TX VGA1 (default: %d)\n"
		"  -i               Interactive mode: North='%c', South='%c', East='%c', West='%c'\n"
		"  -I               Disable ionospheric delay for spacecraft scenario\n"
		"  -p               Disable path loss and hold power level constant\n",
		((double)USER_MOTION_SIZE)/10.0, 
		TX_VGA1,
		NORTH_KEY, SOUTH_KEY, EAST_KEY, WEST_KEY);

	return;
}

int main(int argc, char *argv[])
{
	sim_t s;
	char *devstr = NULL;
	int xb_board=0;

	int result;
	double duration;
	datetime_t t0;

	int txvga1 = TX_VGA1;

	if (argc<3)
	{
		usage();
		exit(1);
	}
	s.finished = false;

	s.opt.navfile[0] = 0;
	s.opt.umfile[0] = 0;
	s.opt.g0.week = -1;
	s.opt.g0.sec = 0.0;
	s.opt.iduration = USER_MOTION_SIZE;
	s.opt.verb = TRUE;
	s.opt.nmeaGGA = FALSE;
	s.opt.staticLocationMode = TRUE; // default user motion
	s.opt.llh[0] = 40.7850916 / R2D;
	s.opt.llh[1] = -73.968285 / R2D;
	s.opt.llh[2] = 100.0;
	s.opt.interactive = FALSE;
	s.opt.timeoverwrite = FALSE;
	s.opt.iono_enable = TRUE;
	s.opt.path_loss_enable = TRUE;

	while ((result=getopt(argc,argv,"e:u:g:l:T:t:d:x:a:iIp"))!=-1)
	{
		switch (result)
		{
		case 'e':
			strcpy(s.opt.navfile, optarg);
			break;
		case 'u':
			strcpy(s.opt.umfile, optarg);
			s.opt.nmeaGGA = FALSE;
			s.opt.staticLocationMode = FALSE;
			break;
		case 'g':
			strcpy(s.opt.umfile, optarg);
			s.opt.nmeaGGA = TRUE;
			s.opt.staticLocationMode = FALSE;
			break;
		case 'l':
			// Static geodetic coordinates input mode
			// Added by scateu@gmail.com
			s.opt.nmeaGGA = FALSE;
			s.opt.staticLocationMode = TRUE;
			sscanf(optarg,"%lf,%lf,%lf",&s.opt.llh[0],&s.opt.llh[1],&s.opt.llh[2]);
			s.opt.llh[0] /= R2D; // convert to RAD
			s.opt.llh[1] /= R2D; // convert to RAD
			break;
		case 'T':
			s.opt.timeoverwrite = TRUE;
			if (strncmp(optarg, "now", 3)==0)
			{
				time_t timer;
				struct tm *gmt;
				
				time(&timer);
				gmt = gmtime(&timer);

				t0.y = gmt->tm_year+1900;
				t0.m = gmt->tm_mon+1;
				t0.d = gmt->tm_mday;
				t0.hh = gmt->tm_hour;
				t0.mm = gmt->tm_min;
				t0.sec = (double)gmt->tm_sec;

				date2gps(&t0, &s.opt.g0);

				break;
			}
		case 't':
			sscanf(optarg, "%d/%d/%d,%d:%d:%lf", &t0.y, &t0.m, &t0.d, &t0.hh, &t0.mm, &t0.sec);
			if (t0.y<=1980 || t0.m<1 || t0.m>12 || t0.d<1 || t0.d>31 ||
				t0.hh<0 || t0.hh>23 || t0.mm<0 || t0.mm>59 || t0.sec<0.0 || t0.sec>=60.0)
			{
				printf("ERROR: Invalid date and time.\n");
				exit(1);
			}
			t0.sec = floor(t0.sec);
			date2gps(&t0, &s.opt.g0);
			break;
		case 'd':
			duration = atof(optarg);
			if (duration<0.0 || duration>((double)USER_MOTION_SIZE)/10.0)
			{
				printf("ERROR: Invalid duration.\n");
				exit(1);
			}
			s.opt.iduration = (int)(duration*10.0+0.5);
			break;
		case 'x':
			xb_board=atoi(optarg);
			break;
		case 'a':
			txvga1 = atoi(optarg);
			if (txvga1>0)
				txvga1 *= -1;

			if (txvga1<BLADERF_TXVGA1_GAIN_MIN)
				txvga1 = BLADERF_TXVGA1_GAIN_MIN;
			else if (txvga1>BLADERF_TXVGA1_GAIN_MAX)
				txvga1 = BLADERF_TXVGA1_GAIN_MAX;
			break;
		case 'i':
			s.opt.interactive = TRUE;
			break;
		case 'I':
			s.opt.iono_enable = FALSE; // Disable ionospheric correction
			break;
		case 'p':
			s.opt.path_loss_enable = FALSE; // Disable path loss
			break;
		case ':':
		case '?':
			usage();
			exit(1);
		default:
			break;
		}
	}

	if (s.opt.navfile[0]==0)
	{
		printf("ERROR: GPS ephemeris file is not specified.\n");
		exit(1);
	}

	if (s.opt.umfile[0]==0 && !s.opt.staticLocationMode)
	{
		printf("ERROR: User motion file / NMEA GGA stream is not specified.\n");
		printf("You may use -l to specify the static location directly.\n");
		exit(1);
	}

	// Initialize simulator
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

	if(xb_board == 200) {
		printf("Initializing XB200 expansion board...\n");

		s.status = bladerf_expansion_attach(s.tx.dev, BLADERF_XB_200);
		if (s.status != 0) {
			fprintf(stderr, "Failed to enable XB200: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		s.status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_CUSTOM);
		if (s.status != 0) {
			fprintf(stderr, "Failed to set XB200 TX filterbank: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		s.status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_BYPASS);
		if (s.status != 0) {
			fprintf(stderr, "Failed to enable TX bypass path on XB200: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		//For sake of completeness set also RX path to a known good state.
		s.status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_CUSTOM);
		if (s.status != 0) {
			fprintf(stderr, "Failed to set XB200 RX filterbank: %s\n", bladerf_strerror(s.status));
			goto out;
		}

		s.status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_BYPASS);
		if (s.status != 0) {
			fprintf(stderr, "Failed to enable RX bypass path on XB200: %s\n", bladerf_strerror(s.status));
			goto out;
		}
	}

	if(xb_board == 300) {
		fprintf(stderr, "XB300 does not support transmitting on GPS frequency\n");
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

	//s.status = bladerf_set_txvga1(s.tx.dev, TX_VGA1);
	s.status = bladerf_set_txvga1(s.tx.dev, txvga1);
	if (s.status != 0) {
		fprintf(stderr, "Failed to set TX VGA1 gain: %s\n", bladerf_strerror(s.status));
		goto out;
	}
	else {
		//printf("TX VGA1 gain: %d dB\n", TX_VGA1);
		printf("TX VGA1 gain: %d dB\n", txvga1);
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
	printf("Press 'Ctrl+C' to abort.\n");

	// Wainting for TX task to complete.
	pthread_join(s.tx.thread, NULL);
	printf("\nDone!\n");

	// Disable TX module and shut down underlying TX stream.
	s.status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, false);
	if (s.status != 0)
		fprintf(stderr, "Failed to disable TX module: %s\n", bladerf_strerror(s.status));

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

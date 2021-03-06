/**
 * robotLink.c
 *
 * Robot serial connection and data management
 */
#include "rcc.h"

struct commCon robots[MAXROBOTID]; /* Robot buffers */

int timestamps = 1;
int logging = 0;
int hostData = 1;

int ATsatID = 0;

/**
 * Initialize the robot buffers
 */
void initRobots()
{
	int i, j;

	/* Initialize values in the struct */
	for (i = 0; i < MAXROBOTID; i++) {
		robots[i].id = i;
		robots[i].aid = -1;
		robots[i].blacklisted = 0;
		robots[i].log = 0;
		robots[i].logH = NULL;
		robots[i].hSerial = NULL;
		robots[i].port = 0;
		robots[i].up = 0;
		robots[i].lup = 0;
		robots[i].head = 0;
		robots[i].count = 0;
		robots[i].type = UNKNOWN;
		robots[i].subnet = -1;
		for (j = 0; j < NUMROBOT_POINTS; j++) {
			robots[i].xP[j] = 0;
			robots[i].yP[j] = 0;
			robots[i].upP[j] = 0;
		}
		mutexInit(&robots[i].mutex);
	}

	makeThread(&commManager, 0);
}

/**
 * Performs tasks on all robot links in intervals
 */
void commManager(void *vargp)
{
	int i, j;

	vargp = (void *) vargp;

	/* Iterate through robot list indefinitely */
	for (;;) {
		for (i = 0; i < MAXROBOTID; i++) {
			mutexLock(&robots[i].mutex);

			/* If a remote robot has been inactive for a while, deactivate. */
			if (robots[i].up + GRACETIME < clock()
				&& robots[i].type == REMOTE) {
				robots[i].up = 0;
				robots[i].head = 0;
			}

			/* If robot drawing point has been inactive, deactivate */
			for (j = 0; j < NUMROBOT_POINTS; j++) {
				if (robots[i].upP[j] + PTTIME < clock()) {
					robots[i].upP[j] = 0;
				}
			}

			/* Ping all host robots for updated remote robots. */
			if (robots[i].up
				&& robots[i].type == HOST
				&& !robots[i].blacklisted) {
				hprintf(robots[i].hSerial, "rt\n");
			}

			if (robots[i].aid != -1 && !aprilTagConnected) {
				robots[i].aid = -1;
			}

			mutexUnlock(&robots[i].mutex);
		}

		if (aprilTagConnected) {
			for (i = 0; i < MAX_APRILTAG; i++) {
				mutexLock(&aprilTagData[i].mutex);
//				if (!robots[aprilTagData[i].rid].up) {
//					aprilTagData[i].rid = -1;
//				}

				if ((aprilTagData[i].up + GRACETIME < clock())
					&& aprilTagData[i].active) {
					aprilTagData[i].active = 0;
					aprilTagData[i].up = 0;
					aprilTagData[i].rid = -1;
				}

				mutexUnlock(&aprilTagData[i].mutex);
			}
		}

		/* Sleep for a while. */
		Sleep(SLEEPTIME);
	}
}

/**
 * Spawn a thread to manage a serial connection
 */
int initCommCommander(int port)
{
	struct commInfo *info = Malloc(sizeof(struct commInfo));
	HANDLE *hSerial = Malloc(sizeof(HANDLE));

	/* Connect to the port */
	if (serialConnect(hSerial, port) < 0) {
		if (verbose) {
			fprintf(stderr, "ERROR: Failed to connect serial\n");
		}

		return (-1);
	}

	/* Fill in info struct and pass to thread */
	info->hSerial = hSerial;
	info->port = port;

	makeThread(&commCommander, info);

	return (0);
}

/**
 * Thread to manage a serial connection and input data into robot buffers
 */
void commCommander(void *vargp)
{
	int i, j, n;
	int id = 0;							// Robot ID
	int initialized = 0;				// Have we hand-shook with the robot?
	int isHost = 0;						// Is this robot a rprintf host?
	int subnet = -1;					// What is this robot's subnet?
	int rid = -1;						// Remote robot ID
	char buffer[BUFFERSIZE + 1];		// Buffers
	char rbuffer[BUFFERSIZE + 1];
	char *bufp = buffer;				// Pointer to buffers
	struct commInfo *info;				// Information on robot connection
	struct serialIO sio;				// Robust IO on serial buffer

	/* Get info from argument */
	info = ((struct commInfo *) vargp);

	/* Query the robot for its id, and if it is a host */
	hprintf(info->hSerial, "rr\n");
	hprintf(info->hSerial, "sv\n");

	/* Initialize robust IO on the serial */
	serialInitIO(&sio, info->hSerial);

	/* Manage connection indefinitely */
	for (;;) {
		/* Read a line */
		if ((n = serialReadline(&sio, bufp, BUFFERSIZE)) < 0) {
			if (verbose) {
				fprintf(stderr, "S%02d: Serial read error\n", id);
			}
			break;
		} else if (n == 0) {
			continue;
		}

		if ((bufp - buffer) >= BUFFERSIZE) {
			if (verbose) {
				fprintf(stderr, "S%02d: Buffer Overflow! Dumping contents.\n", id);
			}
			bufp = buffer;
			continue;
		}

		/* Read in more data if we haven't completed a line */
		if (bufp[n - 1] != '\n') {
			bufp += n;
			continue;
		} else {
			/* Format end to be CRLF */
			while (bufp[n - 1] == '\r' || bufp[n - 1] == '\n') {
				bufp[n--] = '\0';
			}

			bufp[n] = '\r';
			bufp[n + 1] = '\n';

			bufp = buffer;
		}

		/* Get ID and host data has been obtained from the robot */
		if (!initialized) {
			if (strncmp(buffer, "rr", 2) == 0) {
				bufp = buffer + 3;
				/* Get the two arguments */
				for (i = 0; i < 3; i++) {
					j = 0;
					while (*bufp != ',') {
						if (*bufp == '\r' && *(bufp + 1) == '\n') {
							rbuffer[j] = '\0';
							break;
						} else if (j == SBUFSIZE) {
							rbuffer[j] = '\0';
							break;
						}
						rbuffer[j] = *bufp;

						j++;
						bufp++;
					}
					bufp++;

					/* Convert hex number in buffer to correct endianness */
					if (i == 0) {
						id = convertASCIIHexWord(rbuffer);
					} else if (i == 1) {
						isHost = convertASCIIHexWord(rbuffer);
					} else if (j != SBUFSIZE) {
						subnet = convertASCIIHexWord(rbuffer);
					}
				}

				if (verbose)
					printf("S%02d: Connected to robot ID %02d\n", id, id);

				/* Initializing */
				initialized = 1;
				activateRobot(id, info);
				if (isHost) {
					robots[id].type = HOST;
				}
				if (subnet != -1) {
					robots[id].subnet = subnet;
				}
				bufp = buffer;
			} else if (strncmp(buffer, "svs", 3) == 0) {
				bufp = buffer + 4;
				/* Get the argument */
				j = 0;
				while (*bufp != ',') {
					if (*bufp == '\r' && *(bufp + 1) == '\n') {
						rbuffer[j] = '\0';
						break;
					} else if (j == SBUFSIZE) {
						rbuffer[j] = '\0';
						break;
					}
					rbuffer[j] = *bufp;

					j++;
					bufp++;
				}
				bufp++;

				/* Convert hex number in buffer to correct endianness */
				id = convertASCIIHexWord(rbuffer);

				if (verbose) {
					printf("S%02d: Connected to robot ID %02d\n", id, id);
				}

				/* Initializing */
				initialized = 1;
				activateRobot(id, info);

				bufp = buffer;
			}

			/* Keep looping until we have gotten the robot's ID. */
			continue;
		}

		/* If we get a status line from a host robot. */
		if (strncmp(buffer, "rts", 3) == 0) {
			/* This robot is a host robot. */
			robots[id].type = HOST;
			isHost = 1;

			if (hostData) {
				insertBuffer(id, buffer, 0);
			}

			/* Get Number or robots */
			if (sscanf(buffer, "rts,%d%s", &n, rbuffer) < 1) {
				continue;
			}

			/* Handle bad numbers */
			if (n > MAXROBOTID || n < 0) {
				continue;
			}

			/* Get IDs of connected robots */
			for (i = 0; i < n; i++) {
				if (sscanf(rbuffer, ",%d%s", &rid, rbuffer) < 1) {
					break;
				}

				/* Handle bad numbers */
				if (rid > MAXROBOTID || rid < 0) {
					break;
				}

				mutexLock(&robots[rid].mutex);

				/* Ignore if this is already connected as a local robot */
				if (robots[rid].hSerial != NULL) {
					mutexUnlock(&robots[rid].mutex);
					continue;
				}

				/* Update robot */
				robots[rid].type = REMOTE;
				robots[rid].up = clock();
				robots[rid].host = id;
				if (subnet != -1) {
					robots[rid].subnet = subnet;
				}

				mutexUnlock(&robots[rid].mutex);
			}
		/* If we get a data line from the robot */
		} else if (strncmp(buffer, "rtd", 3) == 0) {
			if (hostData || robots[id].type != HOST) {
				insertBuffer(id, buffer, 0);
			}

			/* Get the beginning of the remote message */
			if ((bufp = strpbrk(buffer, " ")) == NULL) {
				bufp = buffer;
				continue;
			}

			*bufp = '\0';

			/* Scan ID and data */
			if (sscanf(buffer, "rtd,%d", &rid) < 1) {
				bufp = buffer;
				continue;
			}

			if (rid > MAXROBOTID || rid < 0) {
				bufp = buffer;
				continue;
			}

			mutexLock(&robots[rid].mutex);

			/* If we aren't already connected via serial, put data in buffer. */
			if (robots[rid].hSerial == NULL) {
				/* Insert parsed line into remote robot's buffer. */
				robots[rid].type = REMOTE;
				robots[rid].host = id;
				if (subnet != -1) {
					robots[rid].subnet = subnet;
				}

				mutexUnlock(&robots[rid].mutex);

				insertBuffer(rid, bufp + 1, strlen(buffer));
			} else {
				mutexUnlock(&robots[rid].mutex);
			}

			bufp = buffer;
		} else if (strncmp(buffer, "pt", 2) == 0) {
			int x, y, ind;

			if (sscanf(buffer, "pt %d,%d,%d", &ind, &x, &y) != 3) {
				bufp = buffer;
				continue;
			}

			if (ind >= 0 && ind < NUMROBOT_POINTS) {
				mutexLock(&robots[id].mutex);

				robots[id].xP[ind] = (GLfloat) x;
				robots[id].yP[ind] = (GLfloat) y;
				robots[id].upP[ind] = clock();

				mutexUnlock(&robots[id].mutex);
			}

			bufp = buffer;
		} else {
			/* Insert the read line into robot's buffer. */
			insertBuffer(id, buffer, 0);
		}
	}

	/* On close. */
	if (id != 0) {
		if (ATsatID == id) {
			ATsatID = 0;
		}
		robots[id].hSerial = NULL;

		robots[id].display = 0;
		robots[id].subnet = -1;
		if (robots[id].log) {
			robots[id].log = 0;
			CloseHandle(robots[id].logH);
		}

		/* If blacklisted, should break out of loop due to serial read error. */
		if (robots[id].blacklisted) {
			if (verbose) {
				printf("S%02d: Blacklisted!\n", id);
			}
		/* If we exited from something else */
		} else {
			if (verbose) {
				printf("S%02d: Done!\n", id);
			}
			/* Clean up */
			commToNum[info->port] = 0;
			robots[id].type = UNKNOWN;
			robots[id].up = 0;
			robots[id].aid = -1;
			CloseHandle(*info->hSerial);
		}
	}

	Free(info->hSerial);
	Free(info);

	return;
}

/**
 * Initialize a robot buffer
 */
void activateRobot(int robotID, struct commInfo *info)
{
	mutexLock(&robots[robotID].mutex);

	commToNum[info->port] = robotID;
	robots[robotID].port = info->port;

	robots[robotID].type = LOCAL;

	robots[robotID].hSerial = info->hSerial;
	robots[robotID].up = clock();
	robots[robotID].lup = robots[robotID].up;

	mutexUnlock(&robots[robotID].mutex);
}

/**
 * Insert a line in a robot's buffer
 */
void insertBuffer(int robotID, char *buffer, int extraBytes)
{
	char lbuffer[BUFFERSIZE + APRILTAG_BUFFERSIZE + 16];

	/* Lock the robot buffer */
	mutexLock(&robots[robotID].mutex);

	strcpy(lbuffer, buffer);

	robots[robotID].lup = robots[robotID].up;
	robots[robotID].up = clock();

	if (timestamps) {
		sprintf(robots[robotID].buffer[robots[robotID].head], "%11ld, %s",
			robots[robotID].up, lbuffer);
	} else {
		sprintf(robots[robotID].buffer[robots[robotID].head], lbuffer);
	}

	/* Log data */
	if (robots[robotID].log && logging) {
		mutexUnlock(&robots[robotID].mutex);
		fetchData(lbuffer, robotID, robots[robotID].head, robots[robotID].aid, -1);
		mutexLock(&robots[robotID].mutex);

		hprintf(&robots[robotID].logH, lbuffer);
	}

	robots[robotID].bps[robots[robotID].head] =
		((float) strlen(buffer) + extraBytes) * 10000
		/ ((float) (robots[robotID].up - robots[robotID].lup));

	/* Add new message to rotating buffer */
	robots[robotID].head = (robots[robotID].head + 1) % NUMBUFFER;
	if (robots[robotID].count < NUMBUFFER) {
		robots[robotID].count++;
	}

	/* Unlock the robot buffer */
	mutexUnlock(&robots[robotID].mutex);
}

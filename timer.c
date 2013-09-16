/*
 * Copyright 2013 Takayuki Usui
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "timer.h"

double timer_stat(unsigned long value, long elapsed, long unit)
{
	return ((double)value) / elapsed * 1000000 / unit;
}

long timer_sub(struct timeval *last)
{
	struct timeval now, diff;

	if (gettimeofday(&now, 0) < 0) {
		perror("gettimeofday");
		return -1;
	}
	timersub(&now, last, &diff);
	*last = now;
	return diff.tv_sec * 1000000 + diff.tv_usec;
}

int timer(void (*handler)(int), int ms)
{
	struct sigaction sa;
	struct itimerval it;

	memset(&sa, 0, sizeof(sa));

	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	if(sigaction(SIGALRM, &sa, 0) < 0){
		perror("sigaction");
		return -1;
	}

	it.it_value.tv_sec = ms / 1000;
	it.it_value.tv_usec = (ms % 1000) * 1000;
	it.it_interval.tv_sec = ms / 1000;
	it.it_interval.tv_usec = (ms % 1000) * 1000;
	if(setitimer(ITIMER_REAL, &it, 0) < 0){
		perror("setitimer");
		return -1;
	}
	return 0;
}

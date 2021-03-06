/* Copyright (C) 2002-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>


static int kind[] =
  {
    PTHREAD_RWLOCK_PREFER_READER_NP,
    PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
    PTHREAD_RWLOCK_PREFER_WRITER_NP,
  };


static void *
tf (void *arg)
{
  pthread_rwlock_t *r = arg;

  /* Timeout: 0.3 secs.  */
  struct timeval tv;
  (void) gettimeofday (&tv, NULL);

  struct timespec ts;
  TIMEVAL_TO_TIMESPEC (&tv, &ts);
  ts.tv_nsec += 300000000;
  if (ts.tv_nsec >= 1000000000)
    {
      ts.tv_nsec -= 1000000000;
      ++ts.tv_sec;
    }

  int err = pthread_rwlock_timedwrlock (r, &ts);
  if (err == 0)
    {
      puts ("rwlock_timedwrlock returned");
      pthread_exit ((void *) 1l);
    }

  if (err != ETIMEDOUT)
    {
      printf ("err = %s (%d), expected %s (%d)\n",
	      strerror (err), err, strerror (ETIMEDOUT), ETIMEDOUT);
      pthread_exit ((void *) 1l);
    }
  puts ("child: timedwrlock failed with ETIMEDOUT");

  struct timeval tv2;
  (void) gettimeofday (&tv2, NULL);

  timersub (&tv2, &tv, &tv);

  if (tv.tv_usec < 200000)
    {
      puts ("timeout too short");
      pthread_exit ((void *) 1l);
    }

  (void) gettimeofday (&tv, NULL);
  TIMEVAL_TO_TIMESPEC (&tv, &ts);
  ts.tv_sec += 10;
  /* Note that the following operation makes ts invalid.  */
  ts.tv_nsec += 1000000000;

  err = pthread_rwlock_timedwrlock (r, &ts);
  if (err == 0)
    {
      puts ("2nd timedwrlock succeeded");
      pthread_exit ((void *) 1l);
    }
  if (err != EINVAL)
    {
      puts ("2nd timedwrlock did not return EINVAL");
      pthread_exit ((void *) 1l);
    }
  puts ("child: timedwrlock failed with EINVAL");

  return NULL;
}


static int
do_test (void)
{
  size_t cnt;
  for (cnt = 0; cnt < sizeof (kind) / sizeof (kind[0]); ++cnt)
    {
      pthread_rwlock_t r;
      pthread_rwlockattr_t a;

      if (pthread_rwlockattr_init (&a) != 0)
	{
	  printf ("round %Zu: rwlockattr_t failed\n", cnt);
	  exit (1);
	}

      if (pthread_rwlockattr_setkind_np (&a, kind[cnt]) != 0)
	{
	  printf ("round %Zu: rwlockattr_setkind failed\n", cnt);
	  exit (1);
	}

      if (pthread_rwlock_init (&r, &a) != 0)
	{
	  printf ("round %Zu: rwlock_init failed\n", cnt);
	  exit (1);
	}

      if (pthread_rwlockattr_destroy (&a) != 0)
	{
	  printf ("round %Zu: rwlockattr_destroy failed\n", cnt);
	  exit (1);
	}

      struct timeval tv;
      (void) gettimeofday (&tv, NULL);

      struct timespec ts;
      TIMEVAL_TO_TIMESPEC (&tv, &ts);

      ++ts.tv_sec;

      /* Get a read lock.  */
      if (pthread_rwlock_timedrdlock (&r, &ts) != 0)
	{
	  printf ("round %Zu: rwlock_timedrdlock failed\n", cnt);
	  exit (1);
	}
      printf ("%zu: got timedrdlock\n", cnt);

      pthread_t th;
      if (pthread_create (&th, NULL, tf, &r) != 0)
	{
	  printf ("round %Zu: create failed\n", cnt);
	  exit (1);
	}

      void *status;
      if (pthread_join (th, &status) != 0)
	{
	  printf ("round %Zu: join failed\n", cnt);
	  exit (1);
	}
      if (status != NULL)
	{
	  printf ("failure in round %Zu\n", cnt);
	  exit (1);
	}

      if (pthread_rwlock_destroy (&r) != 0)
	{
	  printf ("round %Zu: rwlock_destroy failed\n", cnt);
	  exit (1);
	}
    }

  return 0;
}

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "caputils/picotime.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/***************************************************************************
 This function converts a struct timespec to a struct timepico (us->ps)
 ***************************************************************************/
timepico timespec_to_timepico(struct timespec in)
{
  timepico out;
  out.tv_sec=in.tv_sec;
  out.tv_psec=in.tv_nsec;
  out.tv_psec*=1000;
  return out;
}

/***************************************************************************
 This function converts a struct timeval to a struct timepico (ms->ps)
 ***************************************************************************/
timepico timeval_to_timepico(struct timeval in)
{
  timepico out;
  out.tv_sec=in.tv_sec;
  out.tv_psec=in.tv_usec;
  out.tv_psec*=1000;
  out.tv_psec*=1000;
  return out;
}

/***************************************************************************
 This function compares two struct timepico (ts1<ts2=-1, ts1>ts2=1, ts1==ts2=0)
 ***************************************************************************/
int timecmp(const timepico *ts1, const timepico *ts2)
{
  if (ts1->tv_sec < ts2->tv_sec){ //if ts1 is before ts2
    return -1;
  }
  if (ts1->tv_sec > ts2->tv_sec){//if ts1 is after ts2
    return 1;
  }

  //same second
  if (ts1->tv_psec < ts2->tv_psec){//if ts1 is before ts2
    return -1;
  }
  if (ts1->tv_psec > ts2->tv_psec){//if ts1 is before ts2
    return 1;
  }
  return 0; // if ts1 and ts2 are identical
}

int timepico_from_string(timepico* dst , const char* input){
  const char* fraction;
  struct tm tm = {0,};

  dst->tv_sec  = 0;
  dst->tv_psec = 0;

  //ISO 8601 YYYY-MM-DD hh:mm:ss.x, x can be upto 12 digits
  fraction = strptime (input, "%F %T", &tm);

  //YYYYMMDD hh:mm:ss.x, x can be upto 12 digits
  if(fraction == NULL)  {
    fraction = strptime (input, "%Y%m%d %T", &tm);
  }

  //YYMMDD hh:mm:ss.x, x can be upto 12 digits
  if(fraction == NULL)  {
    fraction = strptime (input, "%y%m%d %T", &tm);
  }

  //Seconds since the epoch 1970-01-01 00:00:00 UTC
  if(fraction == NULL) {
    fraction = strptime(input, "%s", &tm);
  }

  if ( fraction == NULL ){
    return -1;
  }

  dst->tv_sec = mktime(&tm);

  if ( fraction[0] == '.' ){
    fraction++;
    size_t digits = strlen(fraction);
    dst->tv_psec = atol(fraction);

    /* convert ".1" to (0.)1000000000000 */
    while( digits<13 ) {
      dst->tv_psec *= 10;
      digits++;
    }
  }

  return 0;
}

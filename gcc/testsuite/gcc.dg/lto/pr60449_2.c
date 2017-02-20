typedef long int __time_t;
typedef long int __suseconds_t;
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
struct timezone
  {
    int tz_minuteswest;
    int tz_dsttime;
  };
typedef struct timezone *__restrict __timezone_ptr_t;
int gettimeofday1 (struct timeval *__restrict __tv,
		   __timezone_ptr_t __tz)
{
  return 0;
}

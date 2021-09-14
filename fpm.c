#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
 
#define MAX_SEAT 5

#define xy(x, y) printf("\033[%d;%dH", x, y)
#define clear_eol(x) print(x, 24, "\033[K") 

typedef enum {
	IDLE = 0,
	BUSY,
	MAX_CS_STATUS = 0xFFFFFFFF
}CS_STATUS;

typedef enum {
	THINKING = 0,
	HUNGRY,
	EATING,
	MAX_PS_STATUS = 0xFFFFFFFF
}PS_STATUS;


const char *topic[MAX_SEAT] = { "Spaghetti!", "Life", "Universe", "Everything", "Bathroom" };
const char *philosopher_name[MAX_SEAT] = {"Aristotle", "Spinoza", "Kant", "Hegel", "Russell"};

typedef struct {
  int id;
  CS_STATUS status;
  pthread_mutex_t lock;
}chopstick_t;

typedef void (*f_pickup_t)(void *usr_data);
typedef void (*f_putdown_t)(void *usr_data);
typedef void (*f_eat_t)(void *usr_data);
typedef void (*f_think_t)(void *usr_data);

typedef struct {
  int seat_id;
  const char *name;
  PS_STATUS status;
  chopstick_t *left;
  chopstick_t *right;
  f_pickup_t f_pickup;
  f_putdown_t f_putdown;
  f_eat_t f_eat;
  f_think_t f_think;
}philosopher_t;

static chopstick_t chop_sticks[MAX_SEAT];

static void pickup(void *usr_data);
static void putdown(void *usr_data);
static void eat(void *usr_data);
static void think(void *usr_data);

philosopher_t philosophers[MAX_SEAT];

static void chopstick_init(chopstick_t *cs, int max)
{
  int i;
  chopstick_t *ptr_chopstick = cs;

  for(i=0; i<max; i++)
  {
    ptr_chopstick->id = i;
    ptr_chopstick->status = IDLE;
	if (pthread_mutex_init(&ptr_chopstick->lock, NULL) != 0)
	{
	  puts("FAILED IN CHOPSTICK's MUTEX INIT!");
	}
	ptr_chopstick++;
  }
}

void philosophers_init(philosopher_t *ps, int max)
{
  int i;
  int left_cs_id, right_cs_id;
  philosopher_t *ptr_philosopher = ps;
  
  chopstick_init(chop_sticks, MAX_SEAT);

  for(i=0; i<max; i++)
  {
    ptr_philosopher->seat_id = i;
    ptr_philosopher->status = THINKING;
	ptr_philosopher->name = philosopher_name[i];
	ptr_philosopher->f_pickup = pickup;
	ptr_philosopher->f_putdown = putdown;
	ptr_philosopher->f_eat = eat;
	ptr_philosopher->f_think = think;
	
	left_cs_id = i;
	right_cs_id = (i+1)%5;
	
	if(left_cs_id < right_cs_id)
	{
	  ptr_philosopher->left = &chop_sticks[left_cs_id];
	  ptr_philosopher->right = &chop_sticks[right_cs_id];
	}
	else
	{
	  ptr_philosopher->left = &chop_sticks[right_cs_id];
	  ptr_philosopher->right = &chop_sticks[left_cs_id];
	}
	printf("%s sit at %d - left:%d right:%d\n", ptr_philosopher->name, ptr_philosopher->seat_id, ptr_philosopher->left->id, ptr_philosopher->right->id);
	ptr_philosopher++;
  }
}

static void print(int y, int x, const char *fmt, ...)
{
	static pthread_mutex_t screen = PTHREAD_MUTEX_INITIALIZER;
	va_list ap;
	va_start(ap, fmt);
 
	pthread_mutex_lock(&screen);
	xy(y + 1, x), vprintf(fmt, ap);
	xy(MAX_SEAT + 1, 1), fflush(stdout);
	pthread_mutex_unlock(&screen);
}

static void pickup(void *usr_data)
{
	philosopher_t *ptr_philosopher = (philosopher_t *)usr_data;
	
	pthread_mutex_lock(&ptr_philosopher->left->lock);
	ptr_philosopher->left->status = BUSY;
    clear_eol(ptr_philosopher->seat_id);
    print(ptr_philosopher->seat_id, 24, "chopstick%d", ptr_philosopher->left->id);
	
    pthread_mutex_lock(&ptr_philosopher->right->lock);
	ptr_philosopher->right->status = BUSY;
    print(ptr_philosopher->seat_id, 24 + 12, "chopstick%d", ptr_philosopher->right->id);
}

static void putdown(void *usr_data)
{
	philosopher_t *ptr_philosopher = (philosopher_t *)usr_data;
	
	ptr_philosopher->right->status = IDLE;
    pthread_mutex_unlock(&ptr_philosopher->right->lock);
	ptr_philosopher->left->status = IDLE;
    pthread_mutex_unlock(&ptr_philosopher->left->lock);
}

static void eat(void *usr_data) {
    int i, ration;
	philosopher_t *ptr_philosopher = (philosopher_t *)usr_data;

    ptr_philosopher->status = HUNGRY;
    clear_eol(ptr_philosopher->seat_id);
	print(ptr_philosopher->seat_id, 24, "..oO (HUNGRY-%d, need chopsticks)", ptr_philosopher->status);
	
	ptr_philosopher->f_pickup((void *)ptr_philosopher);

    ptr_philosopher->status = EATING;
	print(ptr_philosopher->seat_id, 24+24, "(EATING-%d)", ptr_philosopher->status);
    for (i = 0, ration = 3 + rand() % 8; i < ration; i++)
	{
	  print(ptr_philosopher->seat_id, 24 + 36 + i * 4, "nom", ptr_philosopher->status);
	  sleep(1);
	}
	
	ptr_philosopher->f_putdown((void *)ptr_philosopher);
	ptr_philosopher->status = THINKING;
}

static void think(void *usr_data)
{
	int i, t;
	char buf[64] = {0};

	philosopher_t *ptr_philosopher = (philosopher_t *)usr_data;
 
	do {
		clear_eol(ptr_philosopher->seat_id);
		sprintf(buf, "..oO (THINKING-%d %s)", ptr_philosopher->status, topic[t = rand() % MAX_SEAT]);
 
		for (i = 0; buf[i]; i++) {
			print(ptr_philosopher->seat_id, i+24, "%c", buf[i]);
			if (i < 5) usleep(200000);
		}
		usleep(500000 + rand() % 1000000);
	} while (t);
}

void *philosophize(void *usr_data)
{
	philosopher_t *ptr_philosopher = (philosopher_t *)usr_data;
	
    print(ptr_philosopher->seat_id, 0, "\033[K"); 
	print(ptr_philosopher->seat_id, 1, "%10s", ptr_philosopher->name);
	
	while(1) 
    {
	  ptr_philosopher->f_think(ptr_philosopher);
      ptr_philosopher->f_eat(ptr_philosopher);
    }
}
 
int main(void) {
	int i;
    pthread_t threadId[MAX_SEAT];
	
	printf("\033c");
	
	philosophers_init(philosophers, MAX_SEAT);

	for (i=0; i < MAX_SEAT; ++i) 
	{
	  if (pthread_create(threadId+i, 0, philosophize, &philosophers[i]) != 0) 
	  {
	    printf("%d-th thread create error\n", i);
        return 0;
	  }
    }
 
    for (i=0; i < MAX_SEAT; ++i)
	{
	  pthread_join(threadId[i], NULL);
	}
	
    return 0;
}
 

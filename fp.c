#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
 
#define NUM_THREADS 5
#define xy(x, y) printf("\033[%d;%dH", x, y)
#define clear_eol(x) print(x, 24, "\033[K") 
const char *topic[NUM_THREADS] = { "Spaghetti!", "Life", "Universe", "Everything", "Bathroom" };

pthread_mutex_t forks[NUM_THREADS];
 
typedef struct {
	char *name;
        int id;
	int left;
	int right;
} Philosopher;
 
Philosopher *create(char *nam, int lef, int righ) {
        static int id = 0;
	Philosopher *x = malloc(sizeof(Philosopher));
	x->name = nam;
        x->id = id++;
	x->left = lef;
	x->right = righ;
	return x; 
}

void print(int y, int x, const char *fmt, ...)
{
	static pthread_mutex_t screen = PTHREAD_MUTEX_INITIALIZER;
	va_list ap;
	va_start(ap, fmt);
 
	pthread_mutex_lock(&screen);
	xy(y + 1, x), vprintf(fmt, ap);
	xy(NUM_THREADS + 1, 1), fflush(stdout);
	pthread_mutex_unlock(&screen);
}

void eat(void *data) {
        int i, ration;
	Philosopher *foo = (Philosopher *) data;

        clear_eol(foo->id);
	print(foo->id, 24, "..oO (forks, need forks)");

	pthread_mutex_lock(&forks[foo->left]);
        clear_eol(foo->id);
        print(foo->id, 24, "fork%d", foo->left);
        pthread_mutex_lock(&forks[foo->right]);
        print(foo->id, 24 + 6, "fork%d", foo->right);

        for (i = 0, ration = 3 + rand() % 8; i < ration; i++)
		print(foo->id, 24 + 16 + i * 4, "nom"), sleep(1);

        
        pthread_mutex_unlock(&forks[foo->right]);
        pthread_mutex_unlock(&forks[foo->left]);
}

void think(void *data)
{
	int i, t;
	char buf[64] = {0};


	Philosopher *foo = (Philosopher *)data;
 
	do {
		clear_eol(foo->id);
		sprintf(buf, "..oO (%s)", topic[t = rand() % NUM_THREADS]);
 
		for (i = 0; buf[i]; i++) {
			print(foo->id, i+24, "%c", buf[i]);
			if (i < 5) usleep(200000);
		}
		usleep(500000 + rand() % 1000000);
	} while (t);
}

void *philosophize(void *data)
{
	Philosopher *foo = (Philosopher *)data;
        print(foo->id, 0, "\033[K"); 
	print(foo->id, 1, "%10s", foo->name);
	while(1) 
        {
          think(data);
          eat(data);
        }
}
 
int main(void) {
    	pthread_t threadId[NUM_THREADS];
	Philosopher *all[NUM_THREADS] = {create("Aristotle", 0 ,1), 
					 create("Spinoza", 1, 2), 
					 create("Kant", 2,3), 
					 create("Hegel", 3, 4),
					 create("Russell", 0, 4)};
	for (int i = 0; i < NUM_THREADS; i++){
		if (pthread_mutex_init(&forks[i], NULL) != 0){
			puts("FAILED IN MUTEX INIT!");
			return 0;
		}
	}

	for (int i=0; i < NUM_THREADS; ++i) {
                if (pthread_create(threadId+i, 0, philosophize, all[i]) != 0) {
                    printf("%d-th thread create error\n", i);
                    return 0;
                }
        }
 
        for (int i=0; i < NUM_THREADS; ++i)
            pthread_join(threadId[i], NULL);
        return 0;
}
 

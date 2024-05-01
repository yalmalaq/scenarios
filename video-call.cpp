#include <string>
#include <stdlib.h>
#include <vector>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void enable_cores(long mask)
{
	for (int core = 1; core < 8; core++) {
		char path[500];
	snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/online", core);
			int fd = open(path, O_WRONLY);
			if (fd < 0) {
				printf("%s\n", path);
				perror("no!");
				exit(-1);
			}
			char buf[] = "0\n";
			buf[0] = (mask & (1UL << core)) ? '1' : '0';
			int size = write(fd, buf, sizeof(buf));
			if (size < sizeof(buf)) {
				perror("no!");
				exit(-1);
			}
			close(fd);
	}
}

long read_counter()
{
		int fd = open("/sys/class/power_supply/battery/charge_counter", O_RDONLY);
		char buf[500];
		read(fd, buf, sizeof(buf));
		long ret = atol(buf);
		close(fd);
		return ret;
}

long read_energy_values() {
    int fd = open("/sys/bus/iio/devices/iio:device0/energy_value", O_RDONLY);
    char buf[500];
    ssize_t bytes_read;
    bytes_read = read(fd, buf, sizeof(buf));
    printf("%.*s", (int)bytes_read, buf);
    close(fd);
    return 0;
    }

void set_charging(bool on)
{
    int fd = open("/sys/class/power_supply/usb/device/usb_limit_sink_enable", O_WRONLY);
    if (fd < 0) {
        perror("no!");
        exit(-1);
    }
    char buf[] = "0\n";
    buf[0] = on ? '0' : '1';
    int size = write(fd, buf, sizeof(buf));
    if (size < sizeof(buf)) {
        perror("no!");
        exit(-1);
    }
    close(fd);
}



void *energy_thread(void *ignore)
{
	struct timespec small = {0, 500 * 1000UL};

	long start_value, mid_value, end_value;

	printf("wait for edge\n");
	fflush(stdout);

	start_value = read_counter();
	while (read_counter() == start_value) {
		nanosleep(&small, NULL);
	}
	start_value = read_counter();
	struct timespec start_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("start\n");
    fflush(stdout);
    printf("Start rails data:\n");
    read_energy_values();
    sleep(120);
    printf("wait for edge\n");
    fflush(stdout);

	while (read_counter() == start_value) {
		nanosleep(&small, NULL);
	}
	mid_value = read_counter();
	while (read_counter() == mid_value) {
		nanosleep(&small, NULL);
	}
	end_value = read_counter();
	struct timespec end_time;
	clock_gettime(CLOCK_MONOTONIC, &end_time);

	long micros = (end_time.tv_nsec - start_time.tv_nsec) / 1000UL + (end_time.tv_sec - start_time.tv_sec) * 1000000UL;
	double secs = micros / 1000000.0;
	double energy = 0.0138 * (end_value - start_value); 
	double power = energy / secs;


    printf("Power %lf\nEnergy %lf\nTime %lf\n", power, energy, secs);
    printf("\nEnd rails data:\n");
    read_energy_values();
    fflush(stdout);
    set_charging(true);
    exit(-1);
}

void open_url(char *url)
{
	std::string cmd = "am start -n ";
	std::string c = cmd + url;
	system(c.c_str());
}

//void scroll(bool up)
//{
//	if (up) {
//		 system("input touchscreen swipe 100 500 100 1100 300");
//	} else {
//		 system("input touchscreen swipe 100 1100 100 500 300");
//	}
//}

void actions() {
    
    // To touch somewhere on the screen
    sleep(2);
    
    system("input touchscreen tap 100 500");
    sleep(2);

    //To start playing music
    system("input touchscreen tap 700 200");
    sleep(2);

           // To go to home screen
//    system("input keyevent KEYCODE_HOME");
//    sleep(2);
    
//   system("input keyevent KEYCODE_POWER");
//   sleep(2);
    
 
}



void test()
{
	while (1) {
	std::vector<char*> urls = {"com.microsoft.teams/com.microsoft.skype.teams.Launcher"};
	for (char *s : urls) {
		open_url(s);
		for (int i = 0; i <25; i++) {
          actions();
			struct timespec ts = {1, 000 * 1000000UL};
			nanosleep(&ts, NULL);
		}
	}
	}
}

#define S(x) (1 << (x))
#define M(x) (1 << ((x) + 4))
#define L(x) (1 << ((x) + 6))

int main()
{
	set_charging(false);
    enable_cores(S(0) | S(1) | S(2) | S(3) | M(0) | M(1) | L(0) | L(1));
	//enable_cores(0xFFFFFF);
	pthread_t t;
	pthread_create(&t, NULL, energy_thread, NULL);
	test();
}

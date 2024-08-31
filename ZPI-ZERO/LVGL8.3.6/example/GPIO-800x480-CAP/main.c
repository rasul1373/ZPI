#include "lvgl/lvgl.h"
#include "lvgl/src/core/lv_obj.h"
#include "lv_conf.h"
#include "lv_drv_conf.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "lvgl/demos/ui/ui.h"
#include <pthread.h>
#include <time.h>
#include "lv_conf.h"
#include "lv_drv_conf.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>


#define MAX_BUFFER_SIZE 255
#define DISP_BUF_SIZE 131072

int fd;
char buffer[30];
char concatenatedBuffer[30]; // Adjust size as needed
ssize_t len_size;
size_t concatenatedLen = 0;
struct timeval lastReceiveTime, currentTime;

int flag_click1 = 0;

lv_indev_t * aita_indev; //pointer of indev
lv_obj_t * sys_scr; //pointer of system screen instance
lv_obj_t * head_label; //pointer of title label instance
lv_obj_t * test_btn; //pointer of test button instance
lv_obj_t * test_btn_label; //pointer of label instance for test button
int aita_counter = 0; //counter varible for btn-clicked times
lv_timer_t * aita_timer; //pointer of timer instance
int count_dn = 60; //count-down varible
lv_obj_t * countdn_label; //pointer of label instance for showing countdown numbers

/* Private function prototypes ------------------------------------ */
void aita_InitLVGL(void);
void aita_CreateMainUI(void);
void aita_InitTimer(void);
void test_timer_cb(lv_timer_t * timer);
int recinterupt = 0;
/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


int read_gpio_value(int pin) {
    char path[64];
    FILE *file;
    int value;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);

    file = fopen(path, "r");
    if (file == NULL) {
        perror("Error opening GPIO value file");
        return -1;
    }

    if (fscanf(file, "%d", &value) != 1) {
        perror("Error reading GPIO value");
        fclose(file);
        return -1;
    }

    fclose(file);
    return value;
}


int main(void) {




  system("echo 20 > /sys/class/backlight/backlight/brightness");


  system("  echo 192 > /sys/class/gpio/export ");
  system("  echo 193 > /sys/class/gpio/export ");
  system("  echo 194 > /sys/class/gpio/export ");
  system("  echo 195 > /sys/class/gpio/export ");
  system("  echo 196 > /sys/class/gpio/export ");
  system("  echo 197 > /sys/class/gpio/export ");
  system("  echo out > /sys/class/gpio/gpio192/direction ");
  system("  echo out > /sys/class/gpio/gpio193/direction ");
  system("  echo out > /sys/class/gpio/gpio194/direction ");
  system("  echo in > /sys/class/gpio/gpio195/direction ");
  system("  echo in > /sys/class/gpio/gpio196/direction ");
  system("  echo in > /sys/class/gpio/gpio197/direction ");
  
  
  
  /*LittlevGL init*/
  lv_init();

  /*Linux frame buffer device init*/
  fbdev_init();

  /*A small buffer for LittlevGL to draw the screen's content*/
  static lv_color_t buf[DISP_BUF_SIZE];

  /*Initialize a descriptor for the buffer*/
  static lv_disp_draw_buf_t disp_buf;
  lv_disp_draw_buf_init( & disp_buf, buf, NULL, DISP_BUF_SIZE);

  /*Initialize and register a display driver*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init( & disp_drv);
  disp_drv.draw_buf = & disp_buf;
  disp_drv.flush_cb = fbdev_flush;
  disp_drv.hor_res = 800;
  disp_drv.ver_res = 480;
  lv_disp_drv_register( & disp_drv);

  ui_init();

  evdev_init();
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init( & indev_drv);
  indev_drv.read_cb = evdev_read;
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  lv_indev_drv_register( & indev_drv);

  aita_InitTimer();
  
  
  lv_obj_clear_flag(ui_Switch4, LV_OBJ_FLAG_CLICKABLE);  lv_obj_clear_flag(ui_Switch5, LV_OBJ_FLAG_CLICKABLE);  lv_obj_clear_flag(ui_Switch6, LV_OBJ_FLAG_CLICKABLE);
  
  
  
  while (1) {

  
	if(lv_obj_has_state(ui_Switch1, LV_STATE_CHECKED))   system("    echo 1 > /sys/class/gpio/gpio192/value ");  else  system("    echo 0 > /sys/class/gpio/gpio192/value "); 
	if(lv_obj_has_state(ui_Switch2, LV_STATE_CHECKED))   system("    echo 1 > /sys/class/gpio/gpio193/value ");  else  system("    echo 0 > /sys/class/gpio/gpio193/value "); 
	if(lv_obj_has_state(ui_Switch3, LV_STATE_CHECKED))   system("    echo 1 > /sys/class/gpio/gpio194/value ");  else  system("    echo 0 > /sys/class/gpio/gpio194/value "); 
	
	
	  if (read_gpio_value(195) == 1) {
		lv_obj_add_state(ui_Switch4, LV_STATE_CHECKED);
	    } else {
		lv_obj_clear_state(ui_Switch4, LV_STATE_CHECKED);
	    }


	  if (read_gpio_value(196) == 1) {
		lv_obj_add_state(ui_Switch5, LV_STATE_CHECKED);
	    } else {
		lv_obj_clear_state(ui_Switch5, LV_STATE_CHECKED);
	    }



	  if (read_gpio_value(197) == 1) {
		lv_obj_add_state(ui_Switch6, LV_STATE_CHECKED);
	    } else {
		lv_obj_clear_state(ui_Switch6, LV_STATE_CHECKED);
	    }




    custom_tick_get();
    lv_task_handler();
    usleep(5000);
  }

  return 0;
}

uint32_t custom_tick_get(void) {
  static uint64_t start_ms = 0;
  if (start_ms == 0) {
    struct timeval tv_start;
    gettimeofday( & tv_start, NULL);
    start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
  }

  struct timeval tv_now;
  gettimeofday( & tv_now, NULL);
  uint64_t now_ms;
  now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

  uint32_t time_ms = now_ms - start_ms;
  return time_ms;
}

void test_timer_cb(lv_timer_t * timer) {

  char buf[10];
  if (count_dn > 0) {
    sprintf(buf, "%d", count_dn--);
  } else {

    count_dn = 60;
  }
}

void aita_InitTimer(void) {

  aita_timer = lv_timer_create(test_timer_cb, 1000, NULL);

  lv_timer_set_repeat_count(aita_timer, -1);

  lv_timer_pause(aita_timer);

}














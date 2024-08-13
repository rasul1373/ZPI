
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

int main(void) {

  fd = open("dev/ttyS2", O_RDWR | O_NDELAY | O_NOCTTY);

  struct termios options;
  options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, & options);

  system("echo 20 > /sys/class/backlight/backlight/brightness");

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
  FILE * fp;
  
  while (1) {

    fd_set readfds;
    struct timeval timeout;

    FD_ZERO( & readfds);
    FD_SET(fd, & readfds);

    gettimeofday( & currentTime, NULL);

    long int elapsedMicroseconds = (currentTime.tv_sec - lastReceiveTime.tv_sec) * 1000 + (currentTime.tv_usec - lastReceiveTime.tv_usec);

    if (elapsedMicroseconds >= 1000) {
      memset(concatenatedBuffer, 0, sizeof(concatenatedBuffer));
      concatenatedLen = 0;
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 1000; // 1 millisecond

    int selectResult = select(fd + 1, & readfds, NULL, NULL, & timeout);
    if (selectResult > 0 && FD_ISSET(fd, & readfds)) {
      // پاک کردن buffer قبل از خواندن داده جدید
      memset(buffer, 0, sizeof(buffer));

      len_size = read(fd, buffer, sizeof(buffer));
      if (len_size > 0) {
        char buf[64];
        sprintf(buf, " %s ", buffer);
        // حذف محتوای قبلی ui_Label1 و تنظیم متن جدید
        lv_label_set_text(ui_Label1, "");
        lv_label_set_text(ui_Label1, buf);
      }
    }

    if (lv_obj_get_state(ui_Button1) == 34) {
      flag_click1 = 1;
    }

    if (lv_obj_get_state(ui_Button1) == 2 && flag_click1 == 1) {
      flag_click1 = 0;

      // گرفتن متن از ui_textbox1
      const char * text = lv_textarea_get_text(ui_TextArea1);

      // اضافه کردن کاراکتر Enter به انتهای متن
      char textWithNewline[256]; // فرض می‌کنیم که طول متن حداکثر 255 کاراکتر باشد
      snprintf(textWithNewline, sizeof(textWithNewline), "%s\n", text);

      // ارسال متن از طریق سریال
      if (strlen(textWithNewline) > 0) {
        write(fd, textWithNewline, strlen(textWithNewline));
      }

      // پاک کردن متن در ui_textbox1
      lv_textarea_set_text(ui_TextArea1, "");
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














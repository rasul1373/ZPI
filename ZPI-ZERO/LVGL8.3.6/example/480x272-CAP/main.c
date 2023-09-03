
#include "lvgl/lvgl.h"
#include "lvgl/src/core/lv_obj.h"
#include "lv_conf.h"
#include "lv_drv_conf.h"
#include "lv_demo_conf.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "lvgl/demos/ui/ui.h"


#define DISP_BUF_SIZE (128 * 1024)
char sec1[5];
uint8_t flag_clicked=0;
uint32_t val;


int main(void)
{


	lv_init();

	fbdev_init();

	system("echo 33 > /sys/class/gpio/export");
		system("echo \"out\" > /sys/class/gpio/gpio33/direction");
			system("echo 1 > /sys/class/gpio/gpio33/value");

	static lv_color_t buf[DISP_BUF_SIZE];

	static lv_disp_draw_buf_t disp_buf;
	lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf = &disp_buf;
	disp_drv.flush_cb = fbdev_flush;
	disp_drv.hor_res = 480;
	disp_drv.ver_res = 272;
	lv_disp_drv_register(&disp_drv);


ui_init();



    evdev_init();
        static lv_indev_drv_t indev_drv;
          lv_indev_drv_init(&indev_drv);
          indev_drv.read_cb = evdev_read;
        indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);

	while (1)
	{


		lv_tick_inc(200);
		lv_task_handler();
		usleep(1000);

	}

	return 0;
}














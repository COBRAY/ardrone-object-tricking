#include <conio.h>
#include <stdio.h>

#include <signal.h>
#include <ardrone_testing_tool.h>
#include <VP_Api/vp_api_thread_helper.h>
#include <gtk/gtk.h>

static void init_drone_to_5556();

int control_main();


void init_drone_to_5556();
DEFINE_THREAD_ROUTINE(joystick,data)
{  
	
        control_main();
}


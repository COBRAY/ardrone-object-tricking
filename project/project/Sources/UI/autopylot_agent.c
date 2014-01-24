/*
autopylot_agent.c - methods called by autopilot for communication with AR.Drone 
                    Autopylot agent.

    Copyright (C) 2010 Simon D. Levy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of the 
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License 
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 You should also have received a copy of the Parrot Parrot AR.Drone 
 Development License and Parrot AR.Drone copyright notice and disclaimer 
 and If not, see 
   <https://projects.ardrone.org/attachments/277/ParrotLicense.txt> 
 and
   <https://projects.ardrone.org/attachments/278/ParrotCopyrightAndDisclaimer.txt>.
*/

#include <stdio.h>

#include <ardrone_tool/ardrone_tool_configuration.h>
#include <VP_Os/vp_os_malloc.h>

#include "ardrone_autopylot.h"
#include "autopylot_agent.h"
#include "autopylot_agent_comm.h"

#define IMG_BUFSIZE (3*QVGA_WIDTH*QVGA_HEIGHT*sizeof(uint8_t))

static uint8_t *imgbuf;

void agent_init() {
	
	// Allocate buffer for camera data
	imgbuf = (uint8_t *)vp_os_malloc(IMG_BUFSIZE);
	printf("@@@@agent_init() in autopylot_agent.c\n");
	printf("@@@@agent_comm_init() called in autopylot_agent.c\n");
	agent_comm_init();
	
}

void agent_update(uint8_t * camera_data) {


	printf("@@@@agent_update in autopylot_agent.c\n");
	// local copies of global data
	navdata_demo_t navdata;
	int bellycam = g_bellycam ? 1 : 0;
	

	// new commands
	commands_t commands;

	// copy to buffers
	vp_os_memcpy(imgbuf, camera_data, IMG_BUFSIZE);
	vp_os_memcpy(&navdata, g_navdata, sizeof(navdata_demo_t));
	
	// normalize angles to (-1,+1)
	navdata.phi /= 100000;
	navdata.theta /= 100000;
	navdata.psi /= 180000;

	// convert BRG to RGB
	int k;
	for (k=0; k<IMG_BUFSIZE; k+=3) {
		uint8_t tmp = imgbuf[k];
		imgbuf[k] = imgbuf[k+2];
		imgbuf[k+2] = tmp;
	}

	// default to front camera
	int img_width  = QVGA_WIDTH;
	int img_height = QVGA_HEIGHT;
	printf("@@@@default to front camera in autopylot_agent.c\n");
	printf("@@@@g_bellycam =%d\n",g_bellycam);
	// if belly camera indicated, move camera data to beginning of buffer
	if (g_bellycam) {
		int j;
		for (j=0; j<QCIF_HEIGHT; ++j) {
			memcpy(&imgbuf[j*QCIF_WIDTH*3], &imgbuf[j*QVGA_WIDTH*3], QCIF_WIDTH*3);
		}
		img_width  = QCIF_WIDTH;
		img_height = QCIF_HEIGHT;
	}

	agent_comm_act(imgbuf, img_width, img_height, bellycam==1, &navdata, &commands);

	if (g_autopilot) {
		printf("@@@@g_autopiot in autopylot_agent.c\n");

		if (commands.enable) {
			ardrone_at_set_progress_cmd(1, commands.phi, commands.theta, commands.gaz, commands.yaw);
		}

		if (commands.zap) {
			agent_zap();
		}
	}	
}

void agent_zap() {
	printf("@@@@agent_zap() in autopylot_agent.c\n");
	g_bellycam = !g_bellycam;
        int32_t channel = g_bellycam ? ZAP_CHANNEL_VERT : ZAP_CHANNEL_HORI;
        ARDRONE_TOOL_CONFIGURATION_ADDEVENT(video_channel, &channel, NULL);
}


void agent_close() {
	printf("@@@@agent_close() in autopylot_agent.c\n");
	agent_comm_close();
	vp_os_free(imgbuf);
}

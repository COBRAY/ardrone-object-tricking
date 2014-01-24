/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//File name:	   																								   //
//	ARDroneATcmd.c																								   //
//Programer:                           																			   //
//	CHEN ZHAO-WEI and HU TING-WEI																				   //
//Discription:																									   //
//	-This is one of the thread of the ball tracking program which features the moving machanism of the drone.	   //
//The control siganls are sended from PC(client) to the drone(server:192.168.1.1) by UDP trasmission(port 5556).   //
//For input(keyboard) detection,the library <conio.h> is used.													   //
//	-Due to the wrong discriptions and bugged functions in the user guide,										   //
//there will be some slight corrections which may appear to be unfamiliar in first sight.						   //
//However,we will explain the corrections among the lines.														   //
//	-Control:																									   //
//		Page UP/Page Down -> TAKEOFF/LAND																		   //
//		w/a/s/d -> FORWARD/LEFTWARD/BACKWARD/RIGHTWARD															   //
//		q/r		-> LEFTSPIN/RIGHTSPIN																			   //
//		Up/Down -> UPWARD/DOWNWARD																				   //
//		t		-> Tracing(Hold still while tracing)                                                               //
//Last Update:2014/01/21																						   //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define FORWARD 0x0077
#define BACKWARD 0x0073
#define LEFTWARD 0x0061
#define RIGHTWARD 0x0064
#define LEFTSPIN 0x0071
#define RIGHTSPIN 0x0065
#define SPEEDOWN 0x0104
#define SPEEDUP 0x0105
#define TAKEOFF 0x0153
#define LAND 0x0152
#define Up 0x0103
#define Down 0x0102
#define TRACE 0x0074
#define HOVER 0x002f
#include <conio.h>
#include <stdio.h>
#include <signal.h>
#include <ardrone_testing_tool.h>
#include <VP_Api/vp_api_thread_helper.h>
#include <ardrone_tool/ardrone_version.h>
#include <ardrone_tool/Video/video_stage.h>
#include <ardrone_tool/Video/video_recorder_pipeline.h>
#include <VP_Os/vp_os_print.h>
#include <VP_Api/vp_api_thread_helper.h>
#include <VP_Os/vp_os_signal.h>
#include <VP_Os/vp_os_delay.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <gtk/gtk.h>
#define BUFLEN 256
#define PORT 5556
#define SRV_IP "192.168.1.1"

pthread_mutex_t lock1;

static struct sockaddr_in ATcmd;
static int s,slen=sizeof(ATcmd);
static char buf[BUFLEN];

//The initailization of UDP port
//Just in case,here's tutorial http://www.abc.se/~m6695/udp.html
static void init_drone_to_5556(void)
{
		if((s=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1)
                printf("AT socket init error\n");
        
        vp_os_memset((char *) &ATcmd,0,sizeof(ATcmd));
     	ATcmd.sin_family=AF_INET;
        ATcmd.sin_port=htons(PORT);

        if(inet_aton(SRV_IP,&ATcmd.sin_addr)==0)
        {
                cprintf("init_aton failed\n");
                exit(1);
        }

		//This is an AT cmd to limit the maximum hieght of the drone(See Ch.6 for more imform)
		sprintf(buf,"AT*CONFIG=1,\"control:altitude_max\",\"3000\"\r");
	
		if(sendto(s,buf,BUFLEN,0,&ATcmd,slen)==-1)
        {
        	cprintf("send to failed\n");
        }
}

int control_main()
{
    int c;
	int seq=1;
	init_drone_to_5556();
	int in=0;
	int go=0;
	int token=0;
	int hover=0;

    while(1)
	{
		
		c=NULL;
		hover=1;
		cprintf("POS_X&Y tracing:(%d,%d)\n",POS_X,POS_Y);		
		cprintf("None_zero_area:%d\n",None_zero_area);		
		cprintf("MOV_X&Y:(%d,%d)\n",MOV_X,MOV_Y);
              
		c=getch();
        if (27==c)
            break;
        if (0==c||0xe0==c)
            c|=getch()<<8;
		
		clrscr();
		
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//	Determine the cmd we send by the ASCII stored in char c.									  //
		//	Some how the function ardrone_tool_set_progressive_cmd() won't 								  //
		//react to the parameters from -1.0~1.0.After code tracing,we find out that using float to int    //
		//instead will make this fuction work.															  //
		//	Float to int examples:																		  //
		//		(float)0.01 = (int)1008981770		(float)-0.01 = (int)-1138501878						  //
		//		(float)0.05 = (int)1028443341		(float)-0.05 = (int)-1119040307						  //
		//		(float)0.1  = (int)1036831949		(float)-0.1  = (int)-1110651699						  //
		//		(float)0.2  = (int)1045220557		(float)-0.2  = (int)-1102263091						  //
		//		(float)0.5  = (int)1056964608		(float)-0.5  = (int)-1090519040						  //
		//	Also,in fact,contradicts with the user guide,the 5th parameter in                             //
		//ardrone_tool_set_progressive_cmd() makes the drone spin and the 4th parameter makes the drone   //
		//goes up and down. 																			  //
		//	After enter tracing by holding t,the drone will adjust its position itself automatically.     //
		//		-Forward/BackWard: According to the nonzero bits in the threshold img.					  //
		//		-Left Spin/Right Spin: According to the center(global parameter POS_X,POS_Y) of the target// 
		//		 object.																				  //
		//		-Hieght: According to the center(global parameter POS_X,POS_Y) of the target              // 
		//		 object.																				  //
		//                                                                                                //
		////////////////////////////////////////////////////////////////////////////////////////////////////
		switch(c)
        {
            case FORWARD:
				clrscr();
				hover=0;			
                ardrone_tool_set_progressive_cmd(1,0,-1056964608,0,0,0);
				cprintf("moving forward\n");
				break;
            case BACKWARD:
				clrscr();
				hover=0;

				ardrone_tool_set_progressive_cmd(1,0,1056964608,0,0,0);
	                             
				cprintf("moving backward\n");
				break;
             case LEFTWARD:
				clrscr();
				hover=0;
	
				ardrone_tool_set_progressive_cmd(1,-1056964608,0,0,0,0);
	            cprintf("moving left\n");
				break;
            case RIGHTWARD:
				clrscr();
				hover=0;

				ardrone_tool_set_progressive_cmd(1,1056964608,0,0,0,0);                              
				cprintf("moving right\n");
				break;
            case LEFTSPIN:
				clrscr();
				hover=0;
				ardrone_tool_set_progressive_cmd(1,0,0,0,-1056964608,0);
                cprintf("spin left\n");
				break;
			case RIGHTSPIN:
				clrscr();
				hover=0;
				ardrone_tool_set_progressive_cmd(1,0,0,0,1056964608,0);
				cprintf("spin right\n");

				break;
            case Up:
				clrscr();
				hover=0;
				ardrone_tool_set_progressive_cmd(1,0,0,1045220557,0,0);
                cprintf("go up\n");
				break;
            case Down:
				clrscr();
				hover=0;
				ardrone_tool_set_progressive_cmd(1,0,0,-1102263091,0,0);
				cprintf("go down\n");
                
				break;
			case TAKEOFF:
				clrscr();
				hover=0;
				token=1;
				ardrone_tool_set_ui_pad_start( 1 );
				cprintf("takeoff with vpos lock\n");
				break;
			case LAND:
				clrscr();
				hover=1;
				token=0;
				ardrone_tool_set_ui_pad_start( 0 );
				cprintf("land\n");
				break;
			case HOVER:
				hover=0;
				ardrone_tool_set_progressive_cmd(1,0,0,0,0,0);
				cprintf("Hover\n");
				break;
			case TRACE:
				clrscr();
				hover=0;
				go=0;
				cprintf("POS_X&Y tracing:(%d,%d)\n",POS_X,POS_Y);
				cprintf("Central_X&Y:(%d,%d)\n",CENTERAL_X,CENTERAL_Y);
                DISTANCE_X=CENTERAL_X-POS_X;
				DISTANCE_Y=CENTERAL_Y-POS_Y;
				
				//These ranges in the if else judgment is set according to the size of the img window 
				if(DISTANCE_X<=80&&DISTANCE_X>=-80)
				{	in=1;
					cprintf("center\n");
					
				}
				else if(DISTANCE_X>=80&&DISTANCE_X<320)
				{
					in=0;
					ardrone_tool_set_progressive_cmd(1,0,0,0,-1056964608,0);				
					cprintf("left\n");
				}	
				else if(DISTANCE_X>=-320&&DISTANCE_X<-80)
				{
					in=0;
					ardrone_tool_set_progressive_cmd(1,0,0,0,1056964608,0);
					cprintf("right\n");			
				}		

				if(in==1)
				{
					if(DISTANCE_X>=20&&DISTANCE_X<80)
					{
						in=0;
						ardrone_tool_set_progressive_cmd(1,0,0,0,-1056964608,0);
						cprintf("left\n");					
					}	
					else if(DISTANCE_X>=-80&&DISTANCE_X<-20)
					{
						in=0;
						ardrone_tool_set_progressive_cmd(1,0,0,0,1056964608,0);
						cprintf("right\n");
					}
					else if(DISTANCE_X<=20&&DISTANCE_X>=-20)
						ardrone_tool_set_progressive_cmd(1,0,0,0,0,0);						
			   	}
				if(DISTANCE_Y<-20&&DISTANCE_Y>=-80)
				{	
					in=0;	
					cprintf("TRACE DOWN");	
					ardrone_tool_set_progressive_cmd(1,0,0,-1090519040,0,0);	
				}
				
				else if(DISTANCE_Y<80&&DISTANCE_Y>=20)
				{				
					in=0;
					cprintf("TRACE UP");	
					ardrone_tool_set_progressive_cmd(1,0,0,1056964608,0,0);
				}
				
				else if(DISTANCE_Y<-80&&DISTANCE_Y>=-160)
				{	
					in=0;	
					cprintf("TRACE DOWN");		
					ardrone_tool_set_progressive_cmd(1,0,0,-1110651699,0,0);
				}
				else if(DISTANCE_Y<160&&DISTANCE_Y>=80)
				{				
					in=0;
					cprintf("TRACE UP");	
					ardrone_tool_set_progressive_cmd(1,0,0,1036831949,0,0);
				}

				if(DISTANCE_X<=100&&DISTANCE_X>=-100)
				{	go=1;
					cprintf("start foward/back\n");
				}

			    if(go==1)
				{
					vp_os_delay(10);
					if(None_zero_area>=1000)
					{
						in=0;
						ardrone_tool_set_progressive_cmd(1,0,1036831949,0,0,0);
						cprintf("agent BACK\n");
					}
					else if(None_zero_area>=70&&None_zero_area<=800)
					{
						in=0;
						ardrone_tool_set_progressive_cmd(1,0,-1110651699,0,0,0);						
						cprintf("agent Forward\n");				
					}	
					go=0;
					
				}
				break;
			
        }
		if(hover==0)
		{
			vp_os_delay(20);
			ardrone_tool_set_progressive_cmd(1,0,0,0,0,0);
		}
	}
	close(s);
        return 0;
}

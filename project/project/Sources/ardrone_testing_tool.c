/**
 * @file main.c
 * @author sylvain.gaeremynck@parrot.com
 * @date 2009/07/01
 */
#include <ardrone_api.h>
#include <signal.h>
#include <ardrone_testing_tool.h>
#include <VP_Api/vp_api_thread_helper.h>

// ARDrone Tool includes
#include <ardrone_tool/ardrone_tool.h>
#include <ardrone_tool/ardrone_tool_configuration.h>
#include <ardrone_tool/ardrone_version.h>
#include <ardrone_tool/Video/video_stage.h>
#include <ardrone_tool/Video/video_recorder_pipeline.h>
#include <ardrone_tool/Navdata/ardrone_navdata_client.h>

// App includes
#include <Video/pre_stage.h>
#include <Video/display_stage.h>
#include <UI/ARDroneATcmd.h>
#define _MAIN
//autopylot
#include "ardrone_autopylot.h"
#include "autopylot_agent.h"
#include "autopylot_agent_comm.h"
//ARDroneLib
#include <utils/ardrone_time.h>
#include <ardrone_tool/Navdata/ardrone_navdata_client.h>
//#include <ardrone_tool/Control/ardrone_control.h>
#include <ardrone_tool/UI/ardrone_input.h>

//Common
#include <config.h>
#include <ardrone_api.h>
#include <pthread.h>
//VP_SDK
#include <ATcodec/ATcodec_api.h>
#include <VP_Os/vp_os_print.h>
#include <VP_Api/vp_api_thread_helper.h>
#include <VP_Os/vp_os_signal.h>
#include <VP_Os/vp_os_delay.h>

//Local project
//#include <Video/video_stage.h>


// GTK includes
#include <gtk/gtk.h>
//conio
#include <conio.h>
#include <stdio.h>

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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUFLEN 256
#define PORT 5556
#define SRV_IP "192.168.1.1"
/*
static struct sockaddr_in ATcmd;
static int s,slen=sizeof(ATcmd);
static char buf[BUFLEN];*/

pthread_mutex_t lock;
extern input_device_t gamepad;

int exit_program = 1;

pre_stage_cfg_t precfg;
display_stage_cfg_t dispCfg;

codec_type_t drone1Codec = P264_CODEC;
codec_type_t drone2Codec = H264_360P_CODEC;
ZAP_VIDEO_CHANNEL videoChannel = ZAP_CHANNEL_HORI;
#define IMG_BUFSIZE (3*QVGA_WIDTH*QVGA_HEIGHT*sizeof(uint8_t))
#ifndef _MYKONOS_TESTING_TOOL_H_
#define _MYKONOS_TESTING_TOOL_H_
#include <VP_Os/vp_os_types.h>
C_RESULT signal_exit();
#endif // _MYKONOS_TESTING_TOOL_H_
#define FILENAMESIZE (256)
char encodedFileName[FILENAMESIZE] = {0};

///// 2013/12/21 edit ///////
extern int POS_X=0;
extern int POS_Y=0;
extern int MOV_X=0;
extern int MOV_Y=0;
extern int CENTERAL_X=320;
extern int CENTERAL_Y=180;
extern int DISTANCE_X=0;
extern int DISTANCE_Y=0;
extern int DOMAIN_XY=0;
extern int None_zero_area=0;
/////////////////////////////


void controlCHandler (int signal)
{
    // Flush all streams before terminating
    fflush (NULL);
    usleep (200000); // Wait 200 msec to be sure that flush occured
    printf ("\nAll files were flushed\n");
    exit (0);
}



static int32_t exit_ihm_program = 1;

/* Implementing Custom methods for the main function of an ARDrone application */
int main(int argc, char** argv)
{	
    signal (SIGABRT, &controlCHandler);
    signal (SIGTERM, &controlCHandler);
    signal (SIGINT, &controlCHandler);
    int prevargc = argc;
    char **prevargv = argv;

    int index = 0;
    for (index = 1; index < argc; index++)
    {
        if ('-' == argv[index][0] &&
            'e' == argv[index][1])
        {
            char *fullname = argv[index];
            char *name = &fullname[2];
            strncpy (encodedFileName, name, FILENAMESIZE);
        }

        if ('-' == argv[index][0] &&
            'c' == argv[index][1])
        {
            drone1Codec = UVLC_CODEC;
            drone2Codec = H264_720P_CODEC;
        }

        if ('-' == argv[index][0] &&
            'b' == argv[index][1])
        {
            videoChannel = ZAP_CHANNEL_VERT;
        }
    }
    pthread_mutex_lock(&lock);	
    gtk_init (&prevargc, &prevargv);
    pthread_mutex_unlock(&lock);	
    return ardrone_tool_main(argc, argv);
}

/* The delegate object calls this method during initialization of an ARDrone application */
C_RESULT ardrone_tool_init_custom(void)
{

  //control_main();
  START_THREAD(joystick,NULL);
  
  //pthread_mutex_lock(&lock);
  /* Registering for a new device of game controller */
  // ardrone_tool_input_add( &gamepad );

  /* Start all threads of your application */
  /*START_THREAD( video_stage, NULL );*/

    ardrone_application_default_config.navdata_demo = TRUE;
    ardrone_application_default_config.navdata_options = (NAVDATA_OPTION_MASK(NAVDATA_DEMO_TAG) | NAVDATA_OPTION_MASK(NAVDATA_VISION_DETECT_TAG) | NAVDATA_OPTION_MASK(NAVDATA_GAMES_TAG) | NAVDATA_OPTION_MASK(NAVDATA_MAGNETO_TAG) | NAVDATA_OPTION_MASK(NAVDATA_HDVIDEO_STREAM_TAG) | NAVDATA_OPTION_MASK(NAVDATA_WIFI_TAG));
    if (IS_ARDRONE2)
    {
        ardrone_application_default_config.video_codec = drone2Codec;
    }
    else
    {
        ardrone_application_default_config.video_codec = drone1Codec;
    }
    ardrone_application_default_config.video_channel = videoChannel;
    ardrone_application_default_config.bitrate_ctrl_mode = 1;

    /**
     * Define the number of video stages we'll add before/after decoding
     */
#define EXAMPLE_PRE_STAGES 1
#define EXAMPLE_POST_STAGES 1

    /**
     * Allocate useful structures :
     * - index counter
     * - thread param structure and its substructures
     */
    uint8_t stages_index = 0;

    specific_parameters_t *params = (specific_parameters_t *)vp_os_calloc (1, sizeof (specific_parameters_t));
    specific_stages_t *example_pre_stages = (specific_stages_t *)vp_os_calloc (1, sizeof (specific_stages_t));
    specific_stages_t *example_post_stages = (specific_stages_t *)vp_os_calloc (1, sizeof (specific_stages_t));
    vp_api_picture_t *in_picture = (vp_api_picture_t *)vp_os_calloc (1, sizeof (vp_api_picture_t));
    vp_api_picture_t *out_picture = (vp_api_picture_t *)vp_os_calloc (1, sizeof (vp_api_picture_t));

    /**
     * Fill the vp_api_pictures used for video decodig
     * --> out_picture->format is mandatory for AR.Drone 1 and 2. Other lines are only necessary for AR.Drone 1 video decoding
     */
    in_picture->width = 640; // Drone 1 only : Must be greater than the drone 1 picture size (320)
    in_picture->height = 360; // Drone 1 only : Must be greater that the drone 1 picture size (240)

    out_picture->framerate = 20; // Drone 1 only, must be equal to drone target FPS
    //out_picture->format = PIX_FMT_RGB565; // MANDATORY ! Only RGB24, RGB565 are supported
    out_picture->format = PIX_FMT_RGB24;
    out_picture->width = in_picture->width;
    out_picture->height = in_picture->height;

    // Alloc Y, CB, CR bufs according to target format
    uint32_t bpp = 0;
    switch (out_picture->format)
    {
    case PIX_FMT_RGB24:
        // One buffer, three bytes per pixel
        bpp = 3;
        out_picture->y_buf = vp_os_malloc ( out_picture->width * out_picture->height * bpp );
        out_picture->cr_buf = NULL;
        out_picture->cb_buf = NULL;
        out_picture->y_line_size = out_picture->width * bpp;
        out_picture->cb_line_size = 0;
        out_picture->cr_line_size = 0;
        break;
    case PIX_FMT_RGB565:
        // One buffer, two bytes per pixel
        bpp = 2;
        out_picture->y_buf = vp_os_malloc ( out_picture->width * out_picture->height * bpp );
        out_picture->cr_buf = NULL;
        out_picture->cb_buf = NULL;
        out_picture->y_line_size = out_picture->width * bpp;
        out_picture->cb_line_size = 0;
        out_picture->cr_line_size = 0;
        break;
    default:
        fprintf (stderr, "Wrong video format, must be either PIX_FMT_RGB565 or PIX_FMT_RGB24\n");
        exit (-1);
        break;
    }

    /**
     * Allocate the stage lists
     *
     * - "pre" stages are called before video decoding is done
     *  -> A pre stage get the encoded video frame (including PaVE header for AR.Drone 2 frames) as input
     *  -> A pre stage MUST NOT modify these data, and MUST pass it to the next stage
     * - Typical "pre" stage : Encoded video recording for AR.Drone 1 (recording for AR.Drone 2 is handled differently)
     *
     * - "post" stages are called after video decoding
     *  -> The first post stage will get the decoded video frame as its input
     *   --> Video frame format depend on out_picture->format value (RGB24 / RGB565)
     *  -> A post stage CAN modify the data, as ardrone_tool won't process it afterwards
     *  -> All following post stages will use the output of the previous stage as their inputs
     * - Typical "post" stage : Display the decoded frame
     */
    example_pre_stages->stages_list = (vp_api_io_stage_t *)vp_os_calloc (EXAMPLE_PRE_STAGES, sizeof (vp_api_io_stage_t));
    example_post_stages->stages_list = (vp_api_io_stage_t *)vp_os_calloc (EXAMPLE_POST_STAGES, sizeof (vp_api_io_stage_t));

    /**
     * Fill the PRE stage list
     * - name and type are debug infos only
     * - cfg is the pointer passed as "cfg" in all the stages calls
     * - funcs is the pointer to the stage functions
     */
    stages_index = 0;

    vp_os_memset (&precfg, 0, sizeof (pre_stage_cfg_t));
    strncpy (precfg.outputName, encodedFileName, 255);

    example_pre_stages->stages_list[stages_index].name = "Encoded Dumper"; // Debug info
    example_pre_stages->stages_list[stages_index].type = VP_API_FILTER_DECODER; // Debug info
    example_pre_stages->stages_list[stages_index].cfg  = &precfg;
    example_pre_stages->stages_list[stages_index++].funcs  = pre_stage_funcs;

    example_pre_stages->length = stages_index;

    /**
     * Fill the POST stage list
     * - name and type are debug infos only
     * - cfg is the pointer passed as "cfg" in all the stages calls
     * - funcs is the pointer to the stage functions
     */
    stages_index = 0;

    vp_os_memset (&dispCfg, 0, sizeof (display_stage_cfg_t));
    dispCfg.bpp = bpp;
    dispCfg.decoder_info = in_picture;

    example_post_stages->stages_list[stages_index].name = "Decoded display"; // Debug info
    example_post_stages->stages_list[stages_index].type = VP_API_OUTPUT_SDL; // Debug info
    example_post_stages->stages_list[stages_index].cfg  = &dispCfg;
    example_post_stages->stages_list[stages_index++].funcs  = display_stage_funcs;

    example_post_stages->length = stages_index;

    /**
     * Fill thread params for the ardrone_tool video thread
     *  - in_pic / out_pic are reference to our in_picture / out_picture
     *  - pre/post stages lists are references to our stages lists
     *  - needSetPriority and priority are used to control the video thread priority
     *   -> if needSetPriority is set to 1, the thread will try to set its priority to "priority"
     *   -> if needSetPriority is set to 0, the thread will keep its default priority (best on PC)
     */
    params->in_pic = in_picture;
    params->out_pic = out_picture;
    params->pre_processing_stages_list  = example_pre_stages;
    params->post_processing_stages_list = example_post_stages;
    params->needSetPriority = 0;
    params->priority = 0;

    /**
     * Start the video thread (and the video recorder thread for AR.Drone 2)
     */
    //pthread_mutex_unlock(&lock);
    pthread_mutex_lock(&lock);
    START_THREAD(video_stage, params);
    video_stage_init();
    if (2 <= ARDRONE_VERSION ())
    {
        START_THREAD (video_recorder, NULL);
        video_recorder_init ();
    }
    pthread_mutex_unlock(&lock);
    START_THREAD(agent,NULL);
    video_stage_resume_thread ();
    
  return C_OK;
}

/* The delegate object calls this method when the event loop exit */
C_RESULT ardrone_tool_shutdown_custom(/*void*/)
{
  JOIN_THREAD( joystick );
  video_stage_resume_thread(); //Resume thread to kill it !

  /* Relinquish all threads of your application */
  JOIN_THREAD( video_stage );
  if (2 <= ARDRONE_VERSION ())
    {
        video_recorder_resume_thread ();
        JOIN_THREAD (video_recorder);
    }
  
  /* Unregistering for the current device */
  // ardrone_tool_input_remove( &gamepad );
  JOIN_THREAD(agent );
  return C_OK;
}

/* The event loop calls this method for the exit condition */
bool_t ardrone_tool_exit()
{
  return exit_ihm_program == 0;
}

C_RESULT signal_exit()
{
  exit_ihm_program = 0;

  return C_OK;
}









PROTO_THREAD_ROUTINE(gtk, data);
/* Implementing thread table in which you add routines of your application and those provided by the SDK */
PROTO_THREAD_ROUTINE(joystick,data);
PROTO_THREAD_ROUTINE(agent, data);

BEGIN_NAVDATA_HANDLER_TABLE

END_NAVDATA_HANDLER_TABLE


BEGIN_THREAD_TABLE
  THREAD_TABLE_ENTRY( navdata_update, 20 )
  THREAD_TABLE_ENTRY( joystick, 20 ) 
 // THREAD_TABLE_ENTRY( agent, 20 ) 
 // THREAD_TABLE_ENTRY( ardrone_control, 20 )
  THREAD_TABLE_ENTRY( video_stage, 20 )
  THREAD_TABLE_ENTRY(video_recorder, 20)
  THREAD_TABLE_ENTRY(gtk, 20)

END_THREAD_TABLE



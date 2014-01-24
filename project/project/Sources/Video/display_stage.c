/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//File name:	   																								   //
//	display_stage.c																								   //
//Programer:                           																			   //
//	CHEN ZHAO-WEI and HU TING-WEI																				   //
//Discription:																									   //
//	-This is one of the thread of the ball tracking program which features the img output and the target locking   //
//of the drone.Img will be processed to help object detect.And the position will be counted and stored in global   //
//parameters.													   												   //
//Last Update:2014/01/21                                                                                           //
//Edit from:nicolas.brulez@parrot.com																			   //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Self header file
#include "display_stage.h"

// GTK/Cairo headers
#include <cairo.h>
#include <gtk/gtk.h>

// OpenCV header
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <UI/imgproc.hpp>
#include <UI/imgproc_c.h>
#include <Video/highgui.hpp>
#include <Video/highgui_c.h>
#include <Video/types_c.h>
//edit 2013 12 21
#include <ardrone_testing_tool.h>

//CvRNG rng(12345);

#include <stdio.h>
#include <stdlib.h>


//#include "cv.h"
//#include "highgui.h"
// Funcs pointer definition
const vp_api_stage_funcs_t display_stage_funcs = {
    NULL,
    (vp_api_stage_open_t) display_stage_open,
    (vp_api_stage_transform_t) display_stage_transform,
    (vp_api_stage_close_t) display_stage_close
};

//OpenCV function to convert the video frames to OpenCV image objects
IplImage *ipl_image_from_data(uint8_t* data, int reduced_image, int width, int height)
{
  IplImage *currframe;
  IplImage *dst;

  currframe = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 3);
  currframe->imageData = data;
  dst = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, 3);
  cvCvtColor(currframe, dst, CV_BGR2RGB);
  cvSmooth(dst, dst, CV_GAUSSIAN,3,3, 0, 0); //smooth the original image using Gaussian kernel

  cvReleaseImage(&currframe);

  return dst;
}
IplImage* GetThresholdedImage(IplImage* img)
{
	int i;
	
	////////////////edit date:2013/10/02///////////////////
	
	IplImage* imgHSV = cvCreateImage(cvGetSize(img), 8, 3);
	cvCvtColor(img, imgHSV, CV_BGR2HSV);

	IplImage* imgThreshed = cvCreateImage(cvGetSize(img), 8, 1);
	cvInRangeS(imgHSV, cvScalar(20,100,100,0), cvScalar(30,255,255,0), imgThreshed);

	cvErode(imgThreshed,imgThreshed,0,2);

	None_zero_area=cvCountNonZero(imgThreshed);


	cvReleaseImage(&imgHSV);	
  	return imgThreshed;

}



// Extern so we can make the ardrone_tool_exit() function (ardrone_testing_tool.c)
// return TRUE when we close the video window
extern int exit_program;

// Boolean to avoid asking redraw of a not yet created / destroyed window
bool_t gtkRunning = FALSE;

// Picture size getter from input buffer size
// This function only works for RGB565 buffers (i.e. 2 bytes per pixel)
static void getPicSizeFromBufferSize (uint32_t bufSize, uint32_t *width, uint32_t *height)
{
    if (NULL == width || NULL == height)
    {
        return;
    }

    switch (bufSize)
    {
    case 50688: //QCIF > 176*144 *2bpp
        *width = 176;
        *height = 144;
        break;
    case 153600: //QVGA > 320*240 *2bpp
        *width = 320;
        *height = 240;
        break;
    case 460800: //360p > 640*360 *2bpp
        *width = 640;
        *height = 360;
        break;
    case 1843200: //720p > 1280*720 *2bpp
        *width = 1280;
        *height = 720;
        break;
    default:
        *width = 0;
        *height = 0;
        break;
    }
}

// Get actual frame size (without padding)
void getActualFrameSize (display_stage_cfg_t *cfg, uint32_t *width, uint32_t *height)
{
    if (NULL == cfg || NULL == width || NULL == height)
    {
        return;
    }

    *width = cfg->decoder_info->width;
    *height = cfg->decoder_info->height;
}

// Redraw function, called by GTK each time we ask for a frame redraw
static gboolean
on_expose_event (GtkWidget *widget,
                 GdkEventExpose *event,
                 gpointer data)
{
    display_stage_cfg_t *cfg = (display_stage_cfg_t *)data;

    if (2.0 != cfg->bpp)
    {
        return FALSE;
    }

    uint32_t width = 0, height = 0, stride = 0;
    getPicSizeFromBufferSize (cfg->fbSize, &width, &height);
    stride = cfg->bpp * width;

    if (0 == stride)
    {
        return FALSE;
    }

    uint32_t actual_width = 0, actual_height = 0;
    getActualFrameSize (cfg, &actual_width, &actual_height);
    gtk_window_resize (GTK_WINDOW (widget), actual_width, actual_height);

    cairo_t *cr = gdk_cairo_create (widget->window);

    cairo_surface_t *surface = cairo_image_surface_create_for_data (cfg->frameBuffer, CAIRO_FORMAT_RGB16_565, width, height, stride);

    cairo_set_source_surface (cr, surface, 0.0, 0.0);

    cairo_paint (cr);

    cairo_surface_destroy (surface);

    cairo_destroy (cr);

    return FALSE;
}

/**
 * Main GTK Thread.
 * On an actual application, this thread should be started from your app main thread, and not from a video stage
 * This thread will handle all GTK-related functions
 */
DEFINE_THREAD_ROUTINE(gtk, data)
{
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    display_stage_cfg_t *cfg = (display_stage_cfg_t *)data;
    cfg->widget = window;

    g_signal_connect (window, "expose-event", G_CALLBACK (on_expose_event), data);
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (window), 10, 10);
    gtk_widget_set_app_paintable (window, TRUE);
    gtk_widget_set_double_buffered (window, FALSE);

    gtk_widget_show_all (window);

    gtkRunning = TRUE;

    gtk_main ();

    gtkRunning = FALSE;

    // Force ardrone_tool to close
    exit_program = 0;

    // Sometimes, ardrone_tool might not finish properly
    // This happens mainly because a thread is blocked on a syscall
    // in this case, wait 5 seconds then kill the app
    sleep (5);
    exit (0);

    return (THREAD_RET)0;
}

C_RESULT display_stage_open (display_stage_cfg_t *cfg)
{
    // Check that we use RGB565
    if (2 != cfg->bpp)
    {
        // If that's not the case, then don't display anything
        cfg->paramsOK = FALSE;
    }
    else
    {
        // Else, start GTK thread and window
        cfg->paramsOK = TRUE;
        cfg->frameBuffer = NULL;
        cfg->fbSize = 0;
        START_THREAD (gtk, cfg);
    }
    return C_OK;
}

C_RESULT display_stage_transform (display_stage_cfg_t *cfg, vp_api_io_data_t *in, vp_api_io_data_t *out)
{
  	uint32_t width = 0, height = 0;
	double moment10;
	double moment01;
	double area;
	static int pos_x=0;
	static int pos_y=0;
	int last_x=pos_x;
	int last_y=pos_y;
	static int mov_x=0;
	static int mov_y=0;
	CvPoint from_point,to_point;


	////////////////edit date:2013/10/02///////////////////
  	getPicSizeFromBufferSize (in->size, &width, &height);

  	IplImage *frame = ipl_image_from_data((uint8_t*)in->buffers[0], 1, 640, 360);

  	cvNamedWindow("video", CV_WINDOW_AUTOSIZE);
  	cvNamedWindow("threshold", CV_WINDOW_AUTOSIZE);
  	IplImage *imgThresh = GetThresholdedImage(frame);

 	// Calculate the moments to estimate the position of the ball
	CvMoments *moments=(CvMoments*)vp_os_malloc(sizeof(CvMoments));
	cvMoments(imgThresh,moments,1);
	
	//actual moment values
	moment10=cvGetSpatialMoment(moments,1,0);
	moment01=cvGetSpatialMoment(moments,0,1);
	area=cvGetCentralMoment(moments,0,0);

	pos_x=moment10/area;
	pos_y=moment01/area;
	
	mov_x=(pos_x-last_x);
	mov_y=(pos_y-last_y);
	//point out center at RGB frame
	from_point=cvPoint(last_x,last_y);
	to_point=cvPoint(pos_x,pos_y);
	
	//Detect areas on img.
	cvLine(frame,from_point,to_point,CV_RGB(255,0,0),3,CV_AA,0);
	cvLine(frame,cvPoint(240,260),cvPoint(400,260),CV_RGB(0,255,0),3,CV_AA,0);
	cvLine(frame,cvPoint(240,100),cvPoint(400,100),CV_RGB(0,255,0),3,CV_AA,0);
	cvLine(frame,cvPoint(240,100),cvPoint(240,260),CV_RGB(0,255,0),3,CV_AA,0);
	cvLine(frame,cvPoint(400,100),cvPoint(400,260),CV_RGB(0,255,0),3,CV_AA,0);

	cvLine(frame,cvPoint(220,280),cvPoint(420,280),CV_RGB(0,255,255),3,CV_AA,0);
	cvLine(frame,cvPoint(220,80),cvPoint(420,80),CV_RGB(0,255,255),3,CV_AA,0);
	cvLine(frame,cvPoint(220,80),cvPoint(220,280),CV_RGB(0,255,255),3,CV_AA,0);
	cvLine(frame,cvPoint(420,80),cvPoint(420,280),CV_RGB(0,255,255),3,CV_AA,0);


	cvLine(frame,cvPoint(640,350),cvPoint(400,350),CV_RGB(0,0,255),3,CV_AA,0);
	cvLine(frame,cvPoint(640,10 ),cvPoint(400,10 ),CV_RGB(0,0,255),3,CV_AA,0);
	cvLine(frame,cvPoint(640,10 ),cvPoint(640,350),CV_RGB(0,0,255),3,CV_AA,0);
	cvLine(frame,cvPoint(400,10 ),cvPoint(400,350),CV_RGB(0,0,255),3,CV_AA,0);

	cvLine(frame,cvPoint(240,350),cvPoint(0,350),CV_RGB(0,0,255),3,CV_AA,0);
	cvLine(frame,cvPoint(240,10 ),cvPoint(0,10 ),CV_RGB(0,0,255),3,CV_AA,0);
	cvLine(frame,cvPoint(240,10 ),cvPoint(240,350),CV_RGB(0,0,255),3,CV_AA,0);
	cvLine(frame,cvPoint(0,10 ),cvPoint(0,350),CV_RGB(0,0,255),3,CV_AA,0);	
  	cvShowImage("video", frame);
  	cvShowImage("threshold", imgThresh);
	//printf("Graphic Center -> X: %d,Y: %d\n",pos_x,pos_y);
	
	POS_X=pos_x;
	POS_Y=pos_y;

	MOV_X=mov_x;
	MOV_Y=mov_y;
	  	
	cvWaitKey(1);
  	cvReleaseImage(&frame);
  	cvReleaseImage(&imgThresh);


    	return C_OK;
}

C_RESULT display_stage_close (display_stage_cfg_t *cfg)
{
    // Free all allocated memory
    if (NULL != cfg->frameBuffer)
    {
        vp_os_free (cfg->frameBuffer);
        cfg->frameBuffer = NULL;
    }
  
    return C_OK;
}

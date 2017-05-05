#include <iostream>
#include <stdlib.h>     /* malloc, calloc, realloc, free */

#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio/videoio.hpp>  // Video write
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "supportfunc.hpp"
//#include <thread>
//#include <X11/Xlib.h>

using namespace std;
using namespace cv;

extern "C" {
#include "detector.h"
}

// ********************************************************
// ********* support functions ****************************
// ********************************************************

VideoCapture cap_un(0); 
Mat img_cpp;

// temp storage for detected objects
std::vector<detectedBox> detectedobjects;

// convert IplImage to Mat   (NOT IN USE)
void convert_frame(IplImage* input){
    img_cpp = cv::cvarrToMat(input);
}

// draw boxes
extern "C" void label_func(int tl_x, int tl_y, int br_x, int br_y, char *names){

   string str(names);
   Scalar color;
   bool keep = false;

   if(str == "person"){  //index 01
     color = Scalar(255, 0, 0);  //coral color
     keep = true;
   }else if (str == "bike"){ //index 02
     color = Scalar(0, 0, 255);     //orange color
     keep = true;
   }else if (str == "vehicle"){ //index 03
     color = Scalar(0,255,0);      //gold color
     keep = true;
   }else{
     color = Scalar(0,0,0);          //black
   }


   if(keep){
     detectedBox tempstorage;

     if(tl_x < 0)
        tl_x = 0;
     if(tl_y < 0)
        tl_y = 0;

     if(br_x > img_cpp.cols)
        br_x = img_cpp.cols;
     if(br_y > img_cpp.rows)
        br_y = img_cpp.rows;

     tempstorage.topLeft = Point(tl_x,tl_y);
     tempstorage.bottomRight = Point(br_x,br_y);
     tempstorage.name = str;
     tempstorage.objectColor = color;

     detectedobjects.push_back(tempstorage);  //rmb to destory this
   }

}

vector<detectedBox> display_frame_cv(bool display){

    vector<detectedBox> pass_objects(detectedobjects);
   
    if(display){
   	for(int j = 0; j < detectedobjects.size(); j++){
     	    Point namePos(detectedobjects[j].topLeft.x,detectedobjects[j].topLeft.y-10);  //position of name
            rectangle(img_cpp, detectedobjects[j].topLeft, detectedobjects[j].bottomRight, detectedobjects[j].objectColor, 2, CV_AA);                  //draw bounding box
            putText(img_cpp, detectedobjects[j].name, namePos, FONT_HERSHEY_PLAIN, 2.0, detectedobjects[j].objectColor, 1.5);                          //write the name of the object
        }

        imshow("detected results", img_cpp); //display as external window
    }

    detectedobjects.clear();  //clear vector for next cycle

    return pass_objects;
}

/*
void display_frame_cv(){

   Mat img_display = img_cpp.clone();
   
   for(int j = 0; j < detectedobjects.size(); j++){
     Point namePos(detectedobjects[j].topLeft.x,detectedobjects[j].topLeft.y-10);  //position of name

     rectangle(img_cpp, detectedobjects[j].topLeft, detectedobjects[j].bottomRight, detectedobjects[j].objectColor, 2, CV_AA);  //draw bounding box
     putText(img_cpp, detectedobjects[j].name, namePos, FONT_HERSHEY_PLAIN, 2.0, detectedobjects[j].objectColor, 1.5);          //write the name of the object

   }

    detectedobjects.clear();  //clear vector for next cycle

    imshow("detected results", img_cpp);
}



// input picture frame from file
extern "C" image load_image_cv(char *filename)
{
    IplImage* src = cvLoadImage(filename);
    image out = ipl_to_image(src);

    cvReleaseImage(&src);
    rgbgr_image(out);
    return out;
}
*/

// capture from camera stream
extern "C" image load_stream_cv()
{
    cap_un >> img_cpp;

    if (img_cpp.empty()){
       cout << "Warning: frame is empty! Check camera setup" << endl;
       return make_empty_image(0,0,0);
    }

    //only for ZED Stereo!
    //cvSetImageROI(src, cvRect(0, 0, src->width/2,src->height));  
    //IplImage *dst = cvCreateImage (cvGetSize(src),src->depth, src->nChannels );
    //cvCopy(src, dst, NULL);
    //cvResetImageROI(src);
    //cvReleaseImage(dst);

    image im = mat_to_image(img_cpp);
    rgbgr_image(im);
    return im;
}


// initialization of network
void init_network_param(){

     char *datacfg = "cfg/voc.data";
     char *cfg = "cfg/tiny-yolo-voc.cfg";
     char *weights = "tiny-yolo-voc.weights";
     float thresh_desired = 0.35;
     
     //initialize c api
     setup_proceedure(datacfg, cfg, weights, thresh_desired);
}

// initialize camera setup
bool init_camera_param(int cam_id){

      cap_un.open(cam_id);
      if(!cap_un.isOpened()){
         cout << "camera stream failed to open!" <<endl;
	 return false;
      }else
         return true;
}

// run this in a loop
void process_camera_frame(bool display){
     camera_detector();      //draw frame from img_cpp;
     display_frame_cv(display);
}

//---------------------------->
//<---------------------- main ---------------------------->
//---------------------------->

int main(){
   
   // for camera 
   if(!init_camera_param(0))
       return -1;

   init_network_param();       //initialize the camera

   for(;;){  //loop

     process_camera_frame(true);
     
     if(waitKey (1) >= 0)  //break upon anykey
         break;
   }

   return 0;
}

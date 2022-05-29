#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/mat.hpp>
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string> 
#include <opencv2/core/types.hpp>
#include <thread>
#include <future>
#include <mutex>
#include <algorithm>  // std::for_each
#include <memory>


#include "Types.h"
#include "PositionServer.h"
#include "CameraDriver.h"
#include "ImageTransformer.h"

using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

#define PI 3.1415926

int runHoughTransformationTest(cv::Mat image){
  cv::Mat grayImage; // used for the result of transfroming the RGB-Image to a grayscale-image
  cv::Mat edgesDetected; // used for the result of the Canny-Edge-Detection
  cv::Mat blurred;
  std::vector<cv::Vec4i> lines; // a vector to store lines of the image, using the cv::Vec4i-Datatype
  if(! image.data ) {
    std::cout <<  "Image not found or unable to open" << std::endl ;
    return -1;
  }else{
    /*
    Now convert this image to a gray image
    */
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
    /*
    Apply a gaussian Blur
    */
    //cv::GaussianBlur(grayImage, image_blurred_with_3x3_kernel, cv::Size(3, 3), 0);
    cv::Size size(9,9);
    cv::GaussianBlur(grayImage, blurred, size, 0, 0);
    /*
    Run the canny-edge-detection 
    */
    cv::Canny(blurred, edgesDetected, 50, 200);
    /*
    Run the Hough-Transformation
    */
    cv::HoughLinesP(edgesDetected, lines, 1, CV_PI/180, 250, 100, 250);
    // Draw lines on the image
    for (size_t i=0; i<lines.size(); i++) {
      cv::Vec4i l = lines[i];
      cv::line(image, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255, 255, 255), 3, cv::LINE_AA);
    }

  }
  cv::namedWindow( "OpenCV Test Program", cv::WINDOW_AUTOSIZE );
  cv::imshow( "OpenCV Test Program", image );
  return 0;
}

int runVideoTest(cv::VideoCapture cap){
  // Create a VideoCapture object and open the input file
  // If the input is the web camera, pass 0 instead of the video file name
  cv::Mat frame;
  // Check if camera opened successfully
  if(!cap.isOpened()){
    std::cout << "Error opening video stream or file" << std::endl;
    return -1;
  }
  while(1){
    // Capture frame-by-frame
    cap >> frame;
    // If the frame is empty, break immediately
    if (frame.empty()){
      break;
    }
    // Display the resulting frame
    //imshow( "Frame", frame );
    // Run Hough Transformation and display frame
    runHoughTransformationTest(frame);
    // Press  ESC on keyboard to exit
    char c=(char)cv::waitKey(25);
    if(c==27){
      break;
    }
  }
  // When everything done, release the video capture object
  cap.release();
  // Closes all the frames
  cv::destroyAllWindows();
  return 0;
}


int testRunnableEntity(){
  std::vector<std::future<void>> futures;
  /*std::shared_ptr<ImageServer> imgSrv(new ImageServer(debuglevel));
 
  std::cout << "Spawning threads..." << std::endl;
  std::vector<std::future<void>> futures;
  for (int i = 0; i < 10; ++i){
    cv::Mat image = cv::imread("SimpleRunwayTestImage.png" ,cv::IMREAD_COLOR); 
    futures.emplace_back(std::async(std::launch::async, &ImageServer::addImageToQueue, imgSrv, std::move(image)));
  }
  
  cv::Mat res = imgSrv->getImageFromQueue();
  std::cout << res.size() << std::endl;
 */
  CameraDriver cameraDriver(Debuglevel::verbose);
  PositionServer positionServer(Debuglevel::verbose);
  ImageTransformer imageTransformer(Debuglevel::verbose);
  //cameraDriver.run();  
  //positionServer.run();
  //auto camerafutr = std::async(std::launch::async, &CameraDriver::run, &cameraDriver);
  //auto posfutr = std::async(std::launch::async, &PositionServer::run, &positionServer);
  //posfutr.get();
  //camerafutr.get();
  futures.emplace_back(std::async(std::launch::async, &CameraDriver::run, &cameraDriver));
  futures.emplace_back(std::async(std::launch::async, &PositionServer::run, &positionServer));
  futures.emplace_back(std::async(std::launch::async, &ImageTransformer::run, &imageTransformer));
  std::for_each(futures.begin(), futures.end(), [](std::future<void> &ftr) {
        ftr.wait();
  });
  std::promise<cv::Mat> prms;
  std::future<cv::Mat> ftr = prms.get_future();
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  std::thread t(&CameraDriver::sendImageFromQueue, &cameraDriver, std::move(prms));
  /*auto f = [cameraDriver](prms){ // using a lambda function here auto f = [VARIABLES FROM CALLING SCOPE GO HERE]( PARAMETERS GO HERE ){ FUNCTION GOES HERE }
    cameraDriver.sendImageFromQueue(std::move(prms)); 
  };*/
  t.join();
  cv::Mat image = ftr.get();
  cv::imshow( "OpenCV Test Program", image );
  cv::waitKey(0);
  return 0;
}

/*class bar {
public:
  void foo() {
    std::cout << "hello from member function" << std::endl;
  }
};

int main()
{
  std::thread t(&bar::foo, bar());
  t.join();
}
*/
int main(){
  int flag;
  cv::Mat image = cv::imread("SimpleRunwayTestImage.png" ,cv::IMREAD_COLOR); // sample image to test the transformation.
  cv::VideoCapture cap("testVideo002.mp4"); // sample video to test the video-processing
  flag = testRunnableEntity();
  return 0;
}


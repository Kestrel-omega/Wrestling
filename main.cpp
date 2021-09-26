#include <stdio.h>
#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "uart.h"
#include <unistd.h>

using namespace std;
using namespace cv;

string gstreamer_pipeline(int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method)
{
  return "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)" + to_string(capture_width) + ", height=(int)" +
         to_string(capture_height) + ", format=(string)NV12, framerate=(fraction)" + to_string(framerate) +
         "/1 ! nvvidconv flip-method=" + to_string(flip_method) + " ! video/x-raw, width=(int)" + to_string(display_width) + ", height=(int)" +
         to_string(display_height) + ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink drop = true";
}

int low_hue, low_saturate, low_value, high_hue, high_saturate, high_value, low_canny, high_canny;

char data[30];

void LowHue(int, void *)
{
  printf("low_ = %d\n", low_hue);
}
void HighHue(int, void *)
{
  printf("high_hue = %d\n", high_hue);
}
void LowSaturate(int, void *)
{
  printf("low_saturate = %d\n", low_saturate);
}
void HighSaturate(int, void *)
{
  printf("high_saturate = %d\n", high_saturate);
}
void LowValue(int, void *)
{
  printf("low_value = %d\n", low_value);
}
void HighValue(int, void *)
{
  printf("high_value = %d\n", high_value);
}
void LowCanny(int, void *)
{
  printf("low_canny = %d\n", low_canny);
}
void HighCanny(int, void *)
{
  printf("high_canny = %d\n", high_canny);
}

int main()
{
  system("service nvargus-daemon restart");

  int capture_width = 1280;
  int capture_height = 720;
  int display_width = 640;
  int display_height = 360;
  int framerate = 60;
  int flip_method = 0;
  int x, y;

 string pipeline = gstreamer_pipeline(capture_width,
                                       capture_height,
                                       display_width,
                                       display_height,
                                       framerate,
                                       flip_method);
  cout << "Using pipeline: \n\t" << pipeline << "\n";

  VideoCapture cap(pipeline, CAP_GSTREAMER);
  if (!cap.isOpened())
  {
    cout << "Failed to open camera." << endl;
    return (-1);
  }

  Mat img, img_hsv;
  Mat yellow_mask, yellow_image;
  Mat contour_image;
  Mat result;
  Mat element(7, 7, CV_8U, cv::Scalar(1));

  // char *data;

  //Uart UART;

  Point2f COM;

  cout << "Hit ESC to exit"
       << "\n";

  // sprintf(data, "-1,%d,%d,0,-2\n", display_width, display_height);
  //UART.sendUart((unsigned char *)data);

  low_hue = 20;
  low_saturate = 0;
  low_value = 0;
  high_hue = 32;
  high_saturate = 255;
  high_value = 255;
  low_canny = 0;
  high_canny = 255;

  namedWindow("result", 1);

  createTrackbar("low hue", "result", &low_hue, 255, LowHue);
  createTrackbar("high hue", "result", &high_hue, 255, HighHue);
  createTrackbar("low saturate", "result", &low_saturate, 255, LowSaturate);
  createTrackbar("high saturate", "result", &high_saturate, 255, HighSaturate);
  createTrackbar("low value", "result", &low_value, 255, LowValue);
  createTrackbar("high value", "result", &high_value, 255, HighValue);
  createTrackbar("low canny", "result", &low_canny, 255, LowCanny);
  createTrackbar("high canny", "result", &high_canny, 255, HighCanny);

  while (true)
  {
    vector<vector<Point>> contour;
    if (!cap.read(img))
    {
      cout << "Capture read error" << endl;
      break;
    }
    // imshow("Original", img);
    result = img;
    cvtColor(img, img_hsv, COLOR_BGR2HSV);

    Scalar lower_yellow = Scalar(low_hue, low_saturate, low_value);
    Scalar upper_yellow = Scalar(high_hue, high_saturate, high_value);

    inRange(img_hsv, lower_yellow, upper_yellow, yellow_mask);
    bitwise_and(img, img, yellow_image, yellow_mask);

    morphologyEx(yellow_mask, yellow_mask, cv::MORPH_OPEN, element);

    Canny(yellow_mask, contour_image, low_canny, high_canny);
    imshow("Contour", contour_image);
    findContours(contour_image, contour, RETR_EXTERNAL, CHAIN_APPROX_NONE);
    for (size_t i = 0; i < contour.size(); i++)
    {
      if (contour[i].size() > 50)
      {
        drawContours(result, contour, i, Scalar(255, 0, 255), 2);
        RotatedRect yellow = fitEllipse(contour[i]);
        COM = yellow.center;
        if (COM.x < 0 || COM.y < 0)
          continue;
        circle(result, COM, 5, Scalar(255, 255, 255), 2, -1);
        sprintf(data, "-1,%d,%d,%d,-2\n", (int)COM.x, (int)COM.y, (int)contour[i].size());
        //UART.sendUart((unsigned char *)data);
        cout << i  << " : "<<  data << endl;
      }
    }
    imshow("yellow", yellow_mask);
    imshow("result", result);
    int keycode = waitKey(1) & 0xff;
    if (keycode == 27)
      break;
  }
  sprintf(data, "-1,0,0,0-2\n");
  //UART.sendUart((unsigned char *)data);
  cout << data << endl;
  //UART.closeUart();
  cap.release();
  destroyAllWindows();
  return 0;
}

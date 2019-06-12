#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include "opencv2\opencv.hpp"
#include <queue>
#include <io.h>
//#define HALCONUSE
#ifdef HALCONUSE
#include "halconcpp.h"
using namespace Halcon;
#endif
#define CSWBEGIN namespace cswFunc {
#define CSWEND }

CSWBEGIN
enum Connectivity { Connected4, Connected8 };
enum GetFileStatus{ SUBDIR_INCLUDED, SUBDIR_EXCLUDED };

typedef std::vector<std::vector<cv::Point>> vv_Point;
typedef std::vector<cv::Point> v_Point;
std::vector<int> DirectionX = { 0, 1, 0, -1, 1, 1, -1, -1 };
std::vector<int> DirectionY = { 1, 0, -1, 0, 1, -1, 1, -1 };

int findContours(cv::Mat& binImg, std::vector<std::vector<cv::Point>>& contours, int flag = 0, cv::OutputArray hierarchy = cv::noArray());
int Hysteresis(const cv::Mat& srcImg, cv::Mat& binImg, int highThre, int lowThre, Connectivity flag = Connected8);

bool createFolder(const char* fDir);
void getFiles(const std::string& path, std::string succ, std::vector<std::string>& files, GetFileStatus gs = SUBDIR_INCLUDED);
static void getDirectory(std::string path, std::vector<std::string>& DirectoryPaths, int order = 1);


//Halcon函数相关模块
#ifdef HALCONUSE
cv::Mat HImageToMat(Halcon::Hobject &Hobj);
Halcon::Hobject MatToHImage(cv::Mat& pImage);
#endif
CSWEND
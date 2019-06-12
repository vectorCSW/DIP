#include "CSWFunction.h"

#ifdef HALCONUSE
cv::Mat cswFunc::HImageToMat(Halcon::Hobject &Hobj)
{
	cv::Mat pImage;
	Hlong htChannels;
	char cType[MAX_STRING];
	Hlong     width, height;
	width = height = 0;
	//转换图像格式
	convert_image_type(Hobj, &Hobj, "byte");
	count_channels(Hobj, &htChannels);
	if (htChannels == 1)
	{
		unsigned char *ptr;
		get_image_pointer1(Hobj, (Hlong *)&ptr, cType, &width, &height);
		pImage = cv::Mat(height, width, CV_8UC1);
		memcpy(pImage.data, ptr, width * height);

	}
	else if (htChannels == 3)
	{

		unsigned char *ptrRed, *ptrGreen, *ptrBlue;
		ptrRed = ptrGreen = ptrBlue = NULL;
		get_image_pointer3(Hobj, (Hlong *)&ptrRed, (Hlong *)&ptrGreen, (Hlong *)&ptrBlue, cType, &width, &height);
		pImage = cv::Mat(height, width, CV_8UC3);
		for (int row = 0; row < height; row++)
		{
			uchar* ptr = pImage.ptr<uchar>(row);
			for (int col = 0; col < width; col++)
			{
				ptr[3 * col] = ptrBlue[row * width + col];
				ptr[3 * col + 1] = ptrGreen[row * width + col];
				ptr[3 * col + 2] = ptrRed[row * width + col];
			}
		}
	}
	return pImage;
}

Halcon::Hobject cswFunc::MatToHImage(cv::Mat& pImage)
{
	Hobject Hobj = Hobject();	//20180508修改 原Hobject Hobj = NULL;会造成内存无法释放，每运行一次多占用一张图的内存
	if (3 == pImage.channels())
	{
		cv::Mat pImageRed, pImageGreen, pImageBlue;
		std::vector<cv::Mat> sbgr(3);
		cv::split(pImage, sbgr);

		int length = pImage.rows * pImage.cols;
		uchar *dataBlue = new uchar[length];
		uchar *dataGreen = new uchar[length];
		uchar *dataRed = new uchar[length];

		int height = pImage.rows;
		int width = pImage.cols;
		for (int row = 0; row < height; row++)
		{
			uchar* ptr = pImage.ptr<uchar>(row);
			for (int col = 0; col < width; col++)
			{
				dataBlue[row * width + col] = ptr[3 * col];
				dataGreen[row * width + col] = ptr[3 * col + 1];
				dataRed[row * width + col] = ptr[3 * col + 2];
			}
		}

		gen_image3(&Hobj, "byte", width, height, (Hlong)(dataRed), (Hlong)(dataGreen), (Hlong)(dataBlue));
		delete[] dataRed;
		delete[] dataGreen;
		delete[] dataBlue;
	}
	else if (1 == pImage.channels())
	{
		int height = pImage.rows;
		int width = pImage.cols;
		uchar *dataGray = new uchar[width * height];
		memcpy(dataGray, pImage.data, width * height);
		gen_image1(&Hobj, "byte", width, height, (Hlong)(dataGray));
		delete[] dataGray;
	}

	return Hobj;
}



#endif

int cswFunc::findContours(cv::Mat& binImg, std::vector<std::vector<cv::Point>>& contours, int flag /*= 0*/, cv::OutputArray hierarchy /*= cv::noArray()*/)
{
	if (binImg.empty())
		return 1;
	cv::Mat src;//(image.rows+2,image.cols+2,image.type(),cv::Scalar(0));
	cv::copyMakeBorder(binImg, src, 1, 1, 1, 1, cv::BORDER_CONSTANT, cv::Scalar(0));
	if (flag)
		cv::findContours(src, contours, hierarchy, CV_RETR_CCOMP, cv::CHAIN_APPROX_NONE, cv::Point(-1, -1));
	else
		cv::findContours(src, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE, cv::Point(-1, -1));
	return 0;
}

int cswFunc::Hysteresis(const cv::Mat& srcImg, cv::Mat& binImg, int highThre, int lowThre, Connectivity flag)
{
	if (srcImg.empty() || srcImg.channels() > 1)
		return 1;
	int DirNum = flag == Connected4 ? 4 : 8;
	binImg = cv::Mat::zeros(srcImg.size(), CV_8UC1);
	std::queue<cv::Point> pQueue;
	for (int i = 0; i < srcImg.rows; ++i)
	{
		const uchar* data = srcImg.ptr<uchar>(i);
		uchar* maskData = binImg.ptr<uchar>(i);
		for (int j = 0; j < srcImg.cols; ++j)
			if (data[j] > highThre)
			{
				pQueue.push(cv::Point(j, i));
				maskData[j] = 0xff;
			}
	}
	while (!pQueue.empty())
	{
		int x = pQueue.front().x; int y = pQueue.front().y;
		for (int dir = 0; dir < DirNum; ++dir)
		{
			int xx = x + DirectionX[dir]; int yy = y + DirectionY[dir];
			uchar& mark = binImg.at<uchar>(yy, xx);
			if (xx >= 0 && xx < srcImg.cols && yy >= 0 && yy < srcImg.rows && mark == 0)
			{
				pQueue.push(cv::Point(xx, yy));
				mark = 0xff;
			}
		}
		pQueue.pop();
	}
	return 0;
}

bool cswFunc::createFolder(const char* fDir)

{
	if (fDir == NULL) return false;
	if (_access(fDir, 0) == -1)
	{
		char mkfDir[200];
		sprintf_s(mkfDir, "mkdir %s", fDir);
		system(mkfDir);
	}
	return true;
}

void cswFunc::getFiles(const std::string& path, std::string succ, std::vector<std::string>& files, GetFileStatus gs /*= SUBDIR_INCLUDED*/)
{
	long hFile = 0;
	struct _finddata64i32_t fileinfo;
	std::string p;
	if (succ.at(0) != '.') succ = std::string(".") + succ;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录，迭代之；如果不是加入列表
			if (fileinfo.attrib & _A_SUBDIR)
			{
				if ((strcmp(fileinfo.name, ".") != 0) && (strcmp(fileinfo.name, "..") != 0))
				{
					//	files.push_back(p.assign(path).append("\\").append(fileinfo.name));
					if (gs == SUBDIR_INCLUDED)
						getFiles(p.assign(path).append("\\").append(fileinfo.name), succ, files, gs);
				}
			}
			else
			{
				if (std::string(fileinfo.name).find(succ.c_str()) != succ.npos)
					files.push_back(p.assign(path).append("\\").append(fileinfo.name));
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}

void cswFunc::getDirectory(std::string path, std::vector<std::string>& DirectoryPaths, int order /*= 1*/)
{
	long hFile = 0;
	struct _finddata64i32_t fileinfo;
	std::string p;
	if (order == 0)
	{
		DirectoryPaths.push_back(path);
		return;
	}
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录，迭代之；如果不是加入列表
			if (fileinfo.attrib & _A_SUBDIR)
			{
				if ((strcmp(fileinfo.name, ".") != 0) && (strcmp(fileinfo.name, "..") != 0))
				{
					if (order == 1)
						DirectoryPaths.push_back(p.assign(path).append("\\").append(fileinfo.name));
					else
						getDirectory(p.assign(path).append("\\").append(fileinfo.name), DirectoryPaths, order - 1);
				}
				else
				{
					;
				}
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}



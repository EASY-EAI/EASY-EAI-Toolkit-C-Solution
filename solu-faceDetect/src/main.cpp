#include "system.h"
#include <opencv2/opencv.hpp>

#include "system_opt.h"
#include "camera.h"
#include "disp.h"
#include "face_detect.h"

using namespace cv;

typedef struct{
	std::vector<det> result;
	int face_number;
}Result_t;

Mat algorithm_image;
pthread_mutex_t img_lock;
// 识别线程
void *detect_thread_entry(void *para)
{
	int ret;
	Result_t *pResult = (Result_t *)para;
	
	// 人脸检测初始化
	rknn_context ctx;
	face_detect_init(&ctx, "face_detect.model");
	
	Mat image;
	while(1)
	{
        if(algorithm_image.empty()) {
			usleep(5);
            continue;
        }
		
		pthread_mutex_lock(&img_lock);
		image = algorithm_image.clone();
		pthread_mutex_unlock(&img_lock);
		// 算法分析
		ret = face_detect_run(ctx, image, pResult->result);
		pResult->face_number = pResult->result.size();
		if(ret <= 0){
			pResult->result.clear();
			usleep(1000);
			continue;
		}
		
		printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("face number : %d\n", pResult->face_number);
		
        usleep(16*1000);
	}
	/* 人脸检测释放 */
	face_detect_release(ctx);
	return NULL;
}

int main(int argc, char **argv)
{
	int ret = 0;
	
	char *pbuf = NULL;
	int skip = 0;
	
	pthread_t mTid;
	Result_t Result;
	
	Mat image;
	// 1.打开摄像头
#define CAMERA_WIDTH	720
#define CAMERA_HEIGHT	1280
#define	IMGRATIO		3
#define	IMAGE_SIZE		(CAMERA_WIDTH*CAMERA_HEIGHT*IMGRATIO)
	ret = rgbcamera_init(CAMERA_WIDTH, CAMERA_HEIGHT, 90);
	if (ret) {
		printf("error: %s, %d\n", __func__, __LINE__);
		goto exit4;
	}
	
	pbuf = NULL;
	pbuf = (char *)malloc(IMAGE_SIZE);
	if (!pbuf) {
		printf("error: %s, %d\n", __func__, __LINE__);
		ret = -1;
		goto exit3;
	}
	
	// 跳过前10帧
	skip = 10;
	while(skip--) {
		ret = rgbcamera_getframe(pbuf);
		if (ret) {
			printf("error: %s, %d\n", __func__, __LINE__);
			goto exit2;
		}
	}
	
	// 2.创建识别线程，以及图像互斥锁
	pthread_mutex_init(&img_lock, NULL);
	Result.result.clear();
	Result.face_number = 0;
	CreateNormalThread(detect_thread_entry, &Result, &mTid);
	
	// 3.显示初始化
#define SCREEN_WIDTH	720
#define SCREEN_HEIGHT	1280
	ret = disp_init(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (ret) {
		printf("error: %s, %d\n", __func__, __LINE__);
		goto exit1;
	}
	
	// 4.(取流 + 显示)循环
	while(1){
		// 4.1、取流
		pthread_mutex_lock(&img_lock);
		ret = rgbcamera_getframe(pbuf);
		if (ret) {
			printf("error: %s, %d\n", __func__, __LINE__);
			pthread_mutex_unlock(&img_lock);
			continue;
		}
		algorithm_image = Mat(CAMERA_HEIGHT, CAMERA_WIDTH, CV_8UC3, pbuf);
		image = algorithm_image.clone();
		pthread_mutex_unlock(&img_lock);
		// 4.2、显示
		for (int i = 0; i < (int)Result.result.size(); i++)
		{
			// 标出人脸框
			int x = (int)(Result.result[i].box.x);
			int y = (int)(Result.result[i].box.y);
			int w = (int)(Result.result[i].box.width);
			int h = (int)(Result.result[i].box.height);
			rectangle(image, Rect(x, y, w, h), Scalar(0, 255, 0), 2, 8, 0);
			// 标出人脸定位标记
			for (int j = 0; j < (int)Result.result[i].landmarks.size(); ++j) {
				cv::circle(image, cv::Point((int)Result.result[i].landmarks[j].x, (int)Result.result[i].landmarks[j].y), 2, cv::Scalar(0, 255, 0), 3, 8);
			}
		}
        disp_commit(image.data, IMAGE_SIZE);
		
        usleep(20*1000);
	}

exit1:
	pthread_mutex_destroy(&img_lock);
exit2:
	free(pbuf);
	pbuf = NULL;
exit3:
	rgbcamera_exit();
exit4:
    return ret;
}

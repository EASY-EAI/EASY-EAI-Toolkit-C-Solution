#include "system.h"
#include <opencv2/opencv.hpp>

#include "system_opt.h"
#include "camera.h"
#include "disp.h"
#include "helmet_detect.h"

using namespace cv;

typedef struct{
	helmet_detect_result_group_t result_group;
	int helmet_number;
}Result_t;

Mat algorithm_image;
pthread_mutex_t img_lock;
// 识别线程
void *detect_thread_entry(void *para)
{
	int ret;
	Result_t *pResult = (Result_t *)para;
	// 人员检测模型初始化
	rknn_context ctx;
	helmet_detect_init(&ctx, "helmet_detect.model");
	
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
		ret = helmet_detect_run(ctx, image, &pResult->result_group);
		pResult->helmet_number = pResult->result_group.count;
		if(pResult->helmet_number <= 0){
			usleep(1000);
			continue;
		}
		
		printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("helmet number : %d\n", pResult->helmet_number);
		
        usleep(16*1000);
	}
	/* 员检测模型释放 */
	helmet_detect_release(ctx);
	return NULL;
}

// fmt: BGRA8888
static Scalar colorArray[10] = {
    Scalar(0,   0,   255, 255),
    Scalar(0,   255, 0,   255),
    Scalar(139, 0,   0,   255),
    Scalar(0,   100, 0,   255),
    Scalar(0,   139, 139, 255),
    Scalar(0,   206, 209, 255),
    Scalar(255, 127, 0,   255),
    Scalar(72,  61,  139, 255),
    Scalar(0,   255, 0,   255),
    Scalar(255, 0,   0,   255),
};
int plot_one_box(Mat src, int x1, int x2, int y1, int y2, char *label, char colour)
{
    int tl = round(0.002 * (src.rows + src.cols) / 2) + 1;
    rectangle(src, cv::Point(x1, y1), cv::Point(x2, y2), colorArray[(unsigned char)colour], 3);

    int tf = max(tl -1, 1);

    int base_line = 0;
    cv::Size t_size = getTextSize(label, FONT_HERSHEY_SIMPLEX, (float)tl/3, tf, &base_line);
    int x3 = x1 + t_size.width;
    int y3 = y1 - t_size.height - 3;

    rectangle(src, cv::Point(x1, y1), cv::Point(x3, y3), colorArray[(unsigned char)colour], -1);
    putText(src, label, cv::Point(x1, y1 - 2), FONT_HERSHEY_SIMPLEX, (float)tl/3, cv::Scalar(255, 255, 255, 255), tf, 8);
    return 0;
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
	Result.helmet_number = 0;
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
		for (int i = 0; i < Result.helmet_number; i++)
		{
			helmet_detect_result_t *det_result = &(Result.result_group.results[i]);			
			if( det_result->prop < 0.3 ){
				continue;
			}
			/*
			printf("%s @ (%d %d %d %d) %f\n",
				   det_result->name,
				   det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
				   det_result->prop);
			*/
			
			int x1 = det_result->box.left;
			int y1 = det_result->box.top;
			int x2 = det_result->box.right;
			int y2 = det_result->box.bottom;
			
			char label_text[50];
			memset(label_text, 0 , sizeof(label_text));
			sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop); 
			plot_one_box(image, x1, x2, y1, y2, label_text, i%10);
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

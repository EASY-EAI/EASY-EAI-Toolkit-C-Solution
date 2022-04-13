#include "system.h"
#include <opencv2/opencv.hpp>

#include "face_detect.h"
#include "face_alignment.h"
#include "face_recognition.h"
#include "system_opt.h"
#include "camera.h"
#include "disp.h"

#include "dataBase.h"
#include "keyEvent.h"

using namespace cv;

typedef struct{
	uint32_t x1;
	uint32_t y1;
	uint32_t x2;
	uint32_t y2;
	uint8_t  color[3];
	char nameStr[128];
}Result_t;

// 按键对数据库的操作
bool g_input_feature = false;
bool g_delete_all_record = false;
int dataBase_opt_handle(int eType)
{
	switch(eType){
		case KEY_PRESS:
			printf("key down\n");
			g_input_feature = true;
			break;
		case KEY_LONGPRESS:
			printf("key long press\n");	
			g_delete_all_record = true;
			break;
		case KEY_RELEASE:
			printf("key up\n");
			break;
		case KEY_CLICK:
			break;
		case KEY_DOUBLECLICK:
			break;
		default:
			break;
	}
	
	return 0;
}

Mat algorithm_image;
pthread_mutex_t img_lock;
// 识别线程
void *detect_thread_entry(void *para)
{
	int ret;
	uint64_t start_time,end_time;
	Result_t *pResult = (Result_t *)para;
	// 初始化人脸检测
	rknn_context detect_ctx;
	std::vector<det> detect_result;
	Point2f points[5];
	printf("face detect init!\n");
	ret = face_detect_init(&detect_ctx, "./face_detect.model");
	if( ret < 0){
		printf("face_detect fail! ret=%d\n", ret);
		return NULL;
	}
	// 初始化人脸识别
	rknn_context recognition_ctx;
	float face_feature[512];
	printf("face recognition init!\n");
	ret =  face_recognition_init(&recognition_ctx, "./face_recognition.model");
	if( ret < 0){
		printf("face_recognition fail! ret=%d\n", ret);
		return NULL;
	}

    // 初始化数据库
    database_init();
    // 同步数据库所有数据到内存
	faceData_t *pFaceData = (faceData_t *)malloc(MAX_USER_NUM * sizeof(faceData_t));
    memset(pFaceData, 0, MAX_USER_NUM * sizeof(faceData_t));
   int peopleNum = database_getData_to_memory(pFaceData);
	
	// 初始化按键事件
	keyEvent_init();
	set_event_handle(dataBase_opt_handle);
    
	Mat image;
	Mat face_algin;
	float similarity; //特征值相似度比对
	int face_index = 0;
	while(1)
	{
        if(algorithm_image.empty()) {
			usleep(5);
            continue;
        }
		
		pthread_mutex_lock(&img_lock);
		image = algorithm_image.clone();
		pthread_mutex_unlock(&img_lock);
		
		if(g_delete_all_record){
			g_delete_all_record = false;
			// 删除库
			database_delete_all_record();			
			// 重载数据库
			peopleNum = database_getData_to_memory(pFaceData);
		}
        
		// 人脸检测，计算出人脸位置
		ret = face_detect_run(detect_ctx, image, detect_result);
		if(ret <= 0){
			// 识别结果数据，复位
			memset(pResult, 0 , sizeof(Result_t));
			usleep(1000);
			continue;
		}
		pResult->x1 = (uint32_t)(detect_result[0].box.x);
		pResult->y1 = (uint32_t)(detect_result[0].box.y);
		pResult->x2 = (uint32_t)(detect_result[0].box.x + detect_result[0].box.width);
		pResult->y2 = (uint32_t)(detect_result[0].box.y + detect_result[0].box.height);
		for (int i = 0; i < (int)detect_result[0].landmarks.size(); ++i) {
			points[i].x = (int)detect_result[0].landmarks[i].x;
			points[i].y = (int)detect_result[0].landmarks[i].y;
		}
		// 人脸校正(从图像中裁出人脸)
		face_algin = face_alignment(image, points);
		// 人脸识别，计算特征值
		start_time = get_timeval_ms();
		face_recognition_run(recognition_ctx, &face_algin, &face_feature);
		end_time = get_timeval_ms();
		printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("face_recognition_run use time: %llu\n", end_time - start_time);
        // 特征值比对，得出id
		similarity = -0.5;
		if(peopleNum > 0){
			for(face_index = 0; face_index < peopleNum; ++face_index){
				similarity = face_recognition_comparison(face_feature, (float *)((pFaceData + face_index)->feature), 512);
				if(similarity > 0.5) {break;}
			}
		}
		printf("similarity:%f\n", similarity);
		if((face_index < peopleNum)&&(similarity > 0.5)){
			// 显示绿框
			pResult->color[0] = 0;
			pResult->color[1] = 255;
			pResult->color[2] = 0;
			// 用id，找名字
			printf("id : %s\n", (pFaceData + face_index)->id);
			database_id_is_exist((pFaceData + face_index)->id, pResult->nameStr, sizeof(pResult->nameStr));
			printf("person name : %s\n", pResult->nameStr);
			// 按键被按下，更新特征值
			if(g_input_feature){
				g_input_feature = false;
				// 特征值入库
				database_add_record((pFaceData + face_index)->id, pResult->nameStr, (char *)face_feature, sizeof(face_feature));
				// 重载数据库
				peopleNum = database_getData_to_memory(pFaceData);
			}
		}else{
			// 显示红框
			pResult->color[0] = 255;
			pResult->color[1] = 0;
			pResult->color[2] = 0;
			// 按键被按下，录入特征值
			if(g_input_feature){
				g_input_feature = false;
				char idStr[32]={0};
				char nameStr[32]={0};
				sprintf(idStr, "%05d", face_index+1);
				sprintf(nameStr, "people_%d", face_index+1);
				// 特征值入库
				database_add_record(idStr, nameStr, (char *)face_feature, sizeof(face_feature));
				// 重载数据库
				peopleNum = database_getData_to_memory(pFaceData);
			}
		}
        usleep(16*1000);
	}
	
	/* 数据库内存数据释放 */
	free(pFaceData);
	/* 数据库释放 */
    database_exit();
	/* 人脸识别释放 */
	face_recognition_release(recognition_ctx);
	/* 人脸检测释放 */
	face_detect_release(detect_ctx);
	return NULL;
}

int main(int argc, char **argv)
{
	int ret = 0;
	
	char *pbuf = NULL;
	int skip = 0;
	
	pthread_t mTid;
	Result_t *pResult = NULL;
	
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
	pResult = (Result_t *)malloc(sizeof(Result_t));
	if(NULL == pResult){
		goto exit1;
	}
	memset(pResult, 0, sizeof(Result_t));
	if(0 != CreateNormalThread(detect_thread_entry, pResult, &mTid)){
		free(pResult);
	}
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
		// 画框
		rectangle(image, Point(pResult->x1, pResult->y1), Point(pResult->x2, pResult->y2), Scalar(pResult->color[0], pResult->color[1], pResult->color[2]), 3);
        disp_commit(image.data, 0, 0);
		
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

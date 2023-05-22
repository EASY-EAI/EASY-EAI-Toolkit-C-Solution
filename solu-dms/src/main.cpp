#include "system.h"
#include <opencv2/opencv.hpp>

#include "system_opt.h"
#include "camera.h"
#include "disp.h"
#include "face_detect.h"
#include "face_landmark98.h"
#include "smoke_detect.h"
#include "phonecall_detect.h"

#include "facialMovement.h"
using namespace cv;

#define SMOKE_FLAG 0
#define PHONE_FLAG 1
#define TIRE_FLAG  2

typedef struct{
    // ================== smoke_detect ==================
    smoke_detect_result_group_t smoke_result;
    // ================ phonecall_detect ================
    phonecall_detect_result_group_t phonecall_result;
    // =================== landmask98 ===================
	int face_number;
    // face_detect
	std::vector<det> face_result;
    // landmask98
    float ratio;
    std::vector<KeyPointType> keyPoints;
    
    // ======== [output] ========
    int drvState;
}Result_t;

Mat algorithm_image;
pthread_mutex_t img_lock;
// 识别线程
void *detect_thread_entry(void *para)
{
	int ret;
	Result_t *pResult = (Result_t *)para;

    // 吸烟模型初始化
	rknn_context smokeCtx;
	smoke_detect_init(&smokeCtx, "./smoke_detect.model");
    
    // 打电话模型初始化
	rknn_context pcCtx;
	phonecall_detect_init(&pcCtx, "./phonecall_detect.model");
    
	// 疲劳驾驶相关参数、模型初始化
	int tired_count = 0;
	rknn_context faceCtx, landmarkCtx;
	face_detect_init(&faceCtx, "face_detect.model");
	face_landmark98_init(&landmarkCtx, "./face_landmark98.model");
	
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

		// ========================== 算法分析部分 ==========================
		/* 1.抽烟检测 */
    	ret = smoke_detect_run(smokeCtx, image, &pResult->smoke_result);
		if(ret < 0){printf("smoke_detect faild !!\n");}
        if(0 < pResult->smoke_result.count){
            pResult->drvState |= (0x01<<SMOKE_FLAG);
        }else{
            pResult->drvState &=~(0x01<<SMOKE_FLAG);
        }

        /* 2.打电话检测 */
	    ret = phonecall_detect_run(pcCtx, image, &pResult->phonecall_result);
		if(ret < 0){printf("phonecall_detect faild !!\n");}
        if(0 < pResult->phonecall_result.count){
            pResult->drvState |= (0x01<<PHONE_FLAG);
        }else{
            pResult->drvState &=~(0x01<<PHONE_FLAG);
        }

        /* 3.疲劳检测 */
		ret = face_detect_run(faceCtx, image, pResult->face_result);
		if(ret <= 0){
            std::vector<det>().swap(pResult->face_result);
			usleep(2 * 1000);
        }
        
		pResult->face_number = pResult->face_result.size();
    	for (int i = 0; i < pResult->face_number; i++) {
    		int x = (int)(pResult->face_result[i].box.x);
    		int y = (int)(pResult->face_result[i].box.y);
    		int w = (int)(pResult->face_result[i].box.width);
    		int h = (int)(pResult->face_result[i].box.height);
    		int max = (w > h) ? w : h;
    		// 判断图像裁剪是否越界
    		if( (x < 0) || (y < 0) || ((x +max) > image.cols) || ((y +max) > image.rows) ) {
    			continue;
    		}
            
    		cv::Mat roi_img, reize_img;
    		roi_img = image(cv::Rect(x, y, max,max));
    		roi_img = roi_img.clone();

    		resize(roi_img, reize_img, Size(256,256), 0, 0, INTER_AREA);
    		pResult->ratio = (float)max/256;

            std::vector<KeyPointType> keyPoints;
    		ret = face_landmark98_run(landmarkCtx, &reize_img, &keyPoints);
    		if(ret < 0){
                pResult->ratio = 0;
                std::vector<KeyPointType>().swap(keyPoints);
    			continue;
            }
            if(eyesClosing(keyPoints)||yawning(keyPoints)){
                tired_count++;
            }else{
                tired_count = 0;
            }
            if(3 <= tired_count){
                pResult->drvState |= (0x01<<TIRE_FLAG);
            }else{
                pResult->drvState &=~(0x01<<TIRE_FLAG);
            }
            std::vector<KeyPointType>().swap(keyPoints);
    	}
		
		printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\ndriver state : ");
        if(pResult->drvState & (0x01<<SMOKE_FLAG)){printf("  [Smoking]");}
        if(pResult->drvState & (0x01<<PHONE_FLAG)){printf("  [Phone Calling]");}
        if(pResult->drvState & (0x01<<TIRE_FLAG)) {printf("  [Is tired]");}
        printf("\n");
        
        usleep(16*1000);
	}
    
	/* 人脸检测释放 */
	face_detect_release(faceCtx);
	/* 人脸关键点定位释放 */
	face_landmark98_release(landmarkCtx);
	/* 打电话检测释放 */
	phonecall_detect_release(pcCtx);
	/* 抽烟检测释放 */
	smoke_detect_release(smokeCtx);
    
	return NULL;
}

int main(int argc, char **argv)
{
	int ret = 0;
	
	char *pbuf = NULL;
	int skip = 0;
	
	pthread_t mTid;
	Result_t Result;

    int driverState = -1;
    
	Mat tipsImage;
	Mat image;
    
    disp_screen_t screen = {0};
	// 1.打开摄像头
#define CAMERA_WIDTH	720
#define CAMERA_HEIGHT	720
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
    std::vector<det>().swap(Result.face_result);
    std::vector<KeyPointType>().swap(Result.keyPoints);
	Result.face_number = 0;
	Result.drvState = 0;
	CreateNormalThread(detect_thread_entry, &Result, &mTid);
	
	// 3.显示初始化
#define SCREEN_WIDTH	720
#define SCREEN_HEIGHT	1280

    screen.screen_width = SCREEN_WIDTH;
    screen.screen_height = SCREEN_HEIGHT;
    
    screen.wins[0].rotation = 270;
    screen.wins[0].in_fmt = IMAGE_TYPE_BGR888;
    screen.wins[0].enable = 1;
    screen.wins[0].in_w = 640;
    screen.wins[0].in_h = 720;
    screen.wins[0].HorStride = screen.wins[0].in_w;
    screen.wins[0].VirStride = screen.wins[0].in_h;
    screen.wins[0].crop_x = 0;
    screen.wins[0].crop_y = 0;
    screen.wins[0].crop_w = 640;
    screen.wins[0].crop_h = 720;
    screen.wins[0].win_x = 0;
    screen.wins[0].win_y = 0;
    screen.wins[0].win_h = 640;
    screen.wins[0].win_w = 720;
    
    screen.wins[1].rotation = 270;
    screen.wins[1].in_fmt = IMAGE_TYPE_BGR888;
    screen.wins[1].enable = 1;
    screen.wins[1].in_w = CAMERA_WIDTH;
    screen.wins[1].in_h = CAMERA_HEIGHT;
    screen.wins[1].HorStride =  CAMERA_WIDTH;
    screen.wins[1].VirStride = CAMERA_HEIGHT;
    screen.wins[1].crop_x = 0;
    screen.wins[1].crop_y = 0;
    screen.wins[1].crop_w = 640;
    screen.wins[1].crop_h = 720;
    screen.wins[1].win_x = 0;
    screen.wins[1].win_y = 640;
    screen.wins[1].win_h = 640;
    screen.wins[1].win_w = 720;

    // ret = disp_init_pro(SCREEN_WIDTH, SCREEN_HEIGHT, 640, 720, IMAGE_TYPE_RGB888, 270, 1, 2);
	ret = disp_init_pro(&screen);
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
#if 0
		for (int i = 0; i < (int)Result.face_number; i++) {
    		int x = (int)(Result.face_result[i].box.x);
    		int y = (int)(Result.face_result[i].box.y);
            printf("Result.keyPoints.size() = %d\n",Result.keyPoints.size());
            if(0 == Result.keyPoints.size()){continue;}
            
            for(int n = 0; n < (int)Result.keyPoints.size(); n++) {
                cv::circle(image,
                           Point( Result.keyPoints[n].point.x*Result.ratio + x, Result.keyPoints[n].point.y*Result.ratio + y), 
                           2, 
                           cv::Scalar(0, 255, 0),
                           2);
            }
            std::vector<KeyPointType>().swap(Result.keyPoints);
        }
#endif
        
        // 根据状态贴提示图
        if(driverState != Result.drvState){
            driverState = Result.drvState;
            if(0 == driverState){
                tipsImage = cv::imread("normal.jpg",1);
            }else if(driverState & (0x01<<SMOKE_FLAG)){
                tipsImage = cv::imread("smoking.jpg",1);
            }else if(driverState & (0x01<<PHONE_FLAG)){
                tipsImage = cv::imread("calling.jpg",1);
            }else if(driverState & (0x01<<TIRE_FLAG)){
                tipsImage = cv::imread("tired.jpg",1);
            }
            disp_commit_pro(tipsImage.data, 0, tipsImage.cols*tipsImage.rows*3);
        }
        
        disp_commit_pro(image.data, 1, IMAGE_SIZE);
        
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

#ifndef __KEYEVENT_H__
#define __KEYEVENT_H__

#include <stdbool.h>
#include <stdint.h>

#define KEY_PRESS       0 // 按下按键
#define KEY_LONGPRESS   1 // 长按按键
#define KEY_RELEASE     2 // 松开按键
#define KEY_CLICK       3 // 单击按键
#define KEY_DOUBLECLICK 4 // 双击按键

#define KEY_EVENT_PATH "/dev/input/event2" // 无触摸屏：event1，有触摸屏：event2

typedef	int (*KeyHandle)(int eType);

// 初始化按键事件监听
extern int keyEvent_init(void);
// 设置按键事件处理
extern int set_event_handle(KeyHandle handle);

#endif

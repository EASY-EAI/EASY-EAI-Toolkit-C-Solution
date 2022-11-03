#ifndef  KEYEVENT_H
#define  KEYEVENT_H

#define KEY_EVENT_PATH "/dev/input/event2" // 无触摸屏：event1，有触摸屏：event2

#include <stdbool.h>
#include <stdint.h>

#define KEY_PRESS       0 // 按下按键
#define KEY_LONGPRESS   1 // 长按按键
#define KEY_RELEASE     2 // 松开按键
#define KEY_CLICK       3 // 单击按键
#define KEY_DOUBLECLICK 4 // 双击按键

typedef int (*KeyHandle)(int eType);




extern int   Init_KeyEven();
extern int set_even_handle(KeyHandle handle);




#endif // ! _KEY_EVENT_H








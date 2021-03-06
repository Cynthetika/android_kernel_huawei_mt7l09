#include <drv_leds.h>

/****************************************************************************
 函 数 名  : drv_led_state_func_reg
 功能描述  : 本接口为led模块不编译时，提供给协议栈的打桩函数
 输入参数  : 协议栈获取点灯状态函数的指针。
 输出参数  : 无。
 返 回 值  : 无。
 注意事项  : 无。

******************************************************************************/
void drv_led_state_func_reg (pFUNCPTR p)
{}

/*****************************************************************************
 函 数 名  : drv_led_flash
 功能描述  : 本接口为led模块不编译时，三色灯设置的打桩函数。
 输入参数  : status：三色灯的状态
 输出参数  : 无。
 返 回 值  : 0:  操作成功；
             -1：操作失败。
*****************************************************************************/
int drv_led_flash(unsigned long state)
{
    return 0;
}

extern "C" void delay_1s();
extern "C" void _exit_exceution();

int main()
{
    // led io 地址
    int *led = (int *)0x2000;
    //初始化 LED 为0
    for (int i = 0; i < 8; i++)
        *(led + i) = 0;
    int i = 0;
    int time = 0;
    while (1)
    {
        //点亮LED 1秒
        *led = 1;
        delay_1s();

        time += 1;
        //熄灭LED 1秒
        *led = 0;
        delay_1s();
        
        time += 1;
        //下一个LED 的地址
        i += 2;
        led += 2;

        if (i > 7)
        {
            i %= 8;
            led = (int *)0x2000 + i;
        }
        // 30 秒后关机
        if (time > 30)
            _exit_exceution();
    }
}
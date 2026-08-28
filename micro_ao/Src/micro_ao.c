#include "micro_ao.h"
#include "main.h"  //để sử dụng irq trong thư viện

//hàm khởi tạo
void Active_ctor(Active * const me, StateHandler initial) {
    me->state = initial;
    me->head = 0;
    me->tail = 0;
    me->count = 0;
}

//hàm kích nổ hệ thống (tự động gọi INIT và ENTRY của state đầu tiên)
void Active_init(Active * const me) {
    //khởi tạo các biến nội bộ
    Event init_e = { INIT_SIG, 0 };
    (*me->state)(me, &init_e);
    //khởi tạo phần cứng
    Event entry_e = { ENTRY_SIG, 0 };
    (*me->state)(me, &entry_e);
}

//hàm ném sự kiện vào Queue (sẽ được gọi từ các hàm ngắt EXTI/UART)
void Active_post(Active * const me, Event e) {
    //hàm khoá ngắt, trường hợp cpu đang count++ mà ngắt UART ngắt ngang và cũng count++ (disable interrupt request)
    __disable_irq();
    //chỉ thêm event khi còn chỗ trong queue
    if (me->count < QUEUE_SIZE) {
        me->queue[me->head] = e;
        me->head = (me->head + 1) % QUEUE_SIZE; //quay vòng head nếu tràn
        me->count++;
    }
    else{//trường hợp tràn thì ghi đè vào sự kiện tail nếu là tín hiệu nguy hiểm (nhiệt cao, gas, lửa)
        if (e.sig == FIRE_DETECTED_SIG || e.sig == GAS_DETECTED_SIG || e.sig == TEMP_HIGH_SIG) 
            //ghi đè sự kiện khẩn cấp này vào vị trí của sự kiện cũ nhất (tail) để hàm dispatch lấy ra xử lý ngay
            me->queue[me->tail] = e;
    }
    __enable_irq();
}

//hàm xử lý sự kiện (đặt trong vòng lặp while(1) của main)
void Active_dispatch(Active * const me) {
    if (me->count > 0) {
         __disable_irq(); //vô hiệu hoá ngắt 
         
        //rút sự kiện từ vị trí tail
        Event e = me->queue[me->tail];
        me->tail = (me->tail + 1) % QUEUE_SIZE; //quay vòng tail nếu tràn
        
        // tránh lúc đang lấy event ra xử lý và cập nhật count thì bị ngắt chen ngang thay đổi count
        me->count--;
         __enable_irq(); //mở lại ngắt

        //giao sự kiện cho trạng thái hiện tại xử lý
        (*me->state)(me, &e);
    }
}

// hàm chuyển trạng thái 
void Active_tran(Active * const me, StateHandler target) {
    //báo hiệu cho state cũ dọn dẹp trước khi thoát
    Event exit_e = { EXIT_SIG, 0 };
    (*me->state)(me, &exit_e);
    
    //cập nhật con trỏ sang State MỚI
    me->state = target;
    
    //báo hiệu cho state mới khởi tạo dữ liệu khi vừa bước vào
    Event entry_e = { ENTRY_SIG, 0 };
    (*me->state)(me, &entry_e);
}
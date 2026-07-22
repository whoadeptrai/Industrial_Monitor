#include "micro_ao.h"

// 1. Khởi tạo đối tượng
void Active_ctor(Active * const me, StateHandler initial) {
    me->state = initial;
    me->head = 0;
    me->tail = 0;
    me->count = 0;
}

// 2. Kích nổ hệ thống (Tự động gọi INIT và ENTRY của state đầu tiên)
void Active_init(Active * const me) {
    Event init_e = { INIT_SIG, 0 };
    (*me->state)(me, &init_e);
    
    Event entry_e = { ENTRY_SIG, 0 };
    (*me->state)(me, &entry_e);
}

// 3. Hàm ném sự kiện vào Queue (Sẽ được gọi từ các hàm ngắt EXTI/UART)
void Active_post(Active * const me, Event e) {
    if (me->count < QUEUE_SIZE) {
        me->queue[me->head] = e;
        me->head = (me->head + 1) % QUEUE_SIZE; // Quay vòng head
        
        // Lưu ý thực chiến: Trong hệ thống thật, việc cộng trừ count 
        // đôi khi cần bọc trong __disable_irq() để tránh Race Condition.
        me->count++;
    }
}

// 4. Hàm tiêu hóa sự kiện (Đặt chết trong vòng lặp while(1) của main)
void Active_dispatch(Active * const me) {
    if (me->count > 0) {
        // Rút sự kiện từ vị trí tail
        Event e = me->queue[me->tail];
        me->tail = (me->tail + 1) % QUEUE_SIZE; // Quay vòng tail
        
        // Critical Section (Vùng găng): Đảm bảo trừ count an toàn
        // __disable_irq(); 
        me->count--;
        // __enable_irq();

        // Giao sự kiện cho trạng thái hiện tại xử lý
        (*me->state)(me, &e);
    }
}

// 5. Hàm chuyển trạng thái cực kỳ thông minh
void Active_tran(Active * const me, StateHandler target) {
    // A. Báo hiệu cho State CŨ dọn dẹp trước khi thoát
    Event exit_e = { EXIT_SIG, 0 };
    (*me->state)(me, &exit_e);
    
    // B. Cập nhật con trỏ sang State MỚI
    me->state = target;
    
    // C. Báo hiệu cho State MỚI khởi tạo dữ liệu khi vừa bước vào
    Event entry_e = { ENTRY_SIG, 0 };
    (*me->state)(me, &entry_e);
}
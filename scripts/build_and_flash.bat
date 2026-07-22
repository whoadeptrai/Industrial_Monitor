@echo off
cd /d "%~dp0\.."
echo =========================================
echo       INDUSTRIAL MONITOR TOOLCHAIN
echo =========================================

:: BƯỚC 1: CẤU HÌNH CMAKE (Chỉ chạy khi chưa có folder build để tiết kiệm thời gian)
if not exist build (
    echo.
    echo STEP 1: CONFIGURING PROJECT WITH CMAKE...
    cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE="cmake/gcc-arm-none-eabi.cmake"
)

:: BƯỚC 2: BIÊN DỊCH VỚI NINJA (Tự động chỉ build các file có thay đổi)
echo.
echo STEP 2: COMPILING FIRMWARE WITH NINJA...
cmake --build build

:: KIỂM TRA LỖI: Nếu code sai cú pháp, dừng lại ngay, không nạp code rác vào chip
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed! Please fix the code errors before flashing.
    pause
    exit /b %errorlevel%
)

:: BƯỚC 3: NẠP CODE XUỐNG CHIP (Sử dụng đúng Serial Number mạch nạp của m)
echo.
echo STEP 3: FLASHING FIRMWARE TO TARGET MCU...
STM32_Programmer_CLI -c port=SWD sn=34FF6B064248323545531157 -w build/Industrial_Monitor.elf -rst

echo.
echo =========================================
echo       DONE! MCU IS RUNNING.
echo =========================================
pause
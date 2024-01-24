################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
../009_Ltc6812/source/sys_link.cmd 

ASM_SRCS += \
../009_Ltc6812/source/dabort.asm \
../009_Ltc6812/source/os_portasm.asm \
../009_Ltc6812/source/sys_core.asm \
../009_Ltc6812/source/sys_intvecs.asm \
../009_Ltc6812/source/sys_mpu.asm \
../009_Ltc6812/source/sys_pmu.asm 

C_SRCS += \
../009_Ltc6812/source/ecap.c \
../009_Ltc6812/source/errata_SSWF021_45.c \
../009_Ltc6812/source/esm.c \
../009_Ltc6812/source/etpwm.c \
../009_Ltc6812/source/gio.c \
../009_Ltc6812/source/het.c \
../009_Ltc6812/source/notification.c \
../009_Ltc6812/source/os_croutine.c \
../009_Ltc6812/source/os_event_groups.c \
../009_Ltc6812/source/os_heap.c \
../009_Ltc6812/source/os_list.c \
../009_Ltc6812/source/os_mpu_wrappers.c \
../009_Ltc6812/source/os_port.c \
../009_Ltc6812/source/os_queue.c \
../009_Ltc6812/source/os_tasks.c \
../009_Ltc6812/source/os_timer.c \
../009_Ltc6812/source/pinmux.c \
../009_Ltc6812/source/spi.c \
../009_Ltc6812/source/sys_dma.c \
../009_Ltc6812/source/sys_main.c \
../009_Ltc6812/source/sys_pcr.c \
../009_Ltc6812/source/sys_phantom.c \
../009_Ltc6812/source/sys_pmm.c \
../009_Ltc6812/source/sys_selftest.c \
../009_Ltc6812/source/sys_startup.c \
../009_Ltc6812/source/sys_vim.c \
../009_Ltc6812/source/system.c 

C_DEPS += \
./009_Ltc6812/source/ecap.d \
./009_Ltc6812/source/errata_SSWF021_45.d \
./009_Ltc6812/source/esm.d \
./009_Ltc6812/source/etpwm.d \
./009_Ltc6812/source/gio.d \
./009_Ltc6812/source/het.d \
./009_Ltc6812/source/notification.d \
./009_Ltc6812/source/os_croutine.d \
./009_Ltc6812/source/os_event_groups.d \
./009_Ltc6812/source/os_heap.d \
./009_Ltc6812/source/os_list.d \
./009_Ltc6812/source/os_mpu_wrappers.d \
./009_Ltc6812/source/os_port.d \
./009_Ltc6812/source/os_queue.d \
./009_Ltc6812/source/os_tasks.d \
./009_Ltc6812/source/os_timer.d \
./009_Ltc6812/source/pinmux.d \
./009_Ltc6812/source/spi.d \
./009_Ltc6812/source/sys_dma.d \
./009_Ltc6812/source/sys_main.d \
./009_Ltc6812/source/sys_pcr.d \
./009_Ltc6812/source/sys_phantom.d \
./009_Ltc6812/source/sys_pmm.d \
./009_Ltc6812/source/sys_selftest.d \
./009_Ltc6812/source/sys_startup.d \
./009_Ltc6812/source/sys_vim.d \
./009_Ltc6812/source/system.d 

OBJS += \
./009_Ltc6812/source/dabort.obj \
./009_Ltc6812/source/ecap.obj \
./009_Ltc6812/source/errata_SSWF021_45.obj \
./009_Ltc6812/source/esm.obj \
./009_Ltc6812/source/etpwm.obj \
./009_Ltc6812/source/gio.obj \
./009_Ltc6812/source/het.obj \
./009_Ltc6812/source/notification.obj \
./009_Ltc6812/source/os_croutine.obj \
./009_Ltc6812/source/os_event_groups.obj \
./009_Ltc6812/source/os_heap.obj \
./009_Ltc6812/source/os_list.obj \
./009_Ltc6812/source/os_mpu_wrappers.obj \
./009_Ltc6812/source/os_port.obj \
./009_Ltc6812/source/os_portasm.obj \
./009_Ltc6812/source/os_queue.obj \
./009_Ltc6812/source/os_tasks.obj \
./009_Ltc6812/source/os_timer.obj \
./009_Ltc6812/source/pinmux.obj \
./009_Ltc6812/source/spi.obj \
./009_Ltc6812/source/sys_core.obj \
./009_Ltc6812/source/sys_dma.obj \
./009_Ltc6812/source/sys_intvecs.obj \
./009_Ltc6812/source/sys_main.obj \
./009_Ltc6812/source/sys_mpu.obj \
./009_Ltc6812/source/sys_pcr.obj \
./009_Ltc6812/source/sys_phantom.obj \
./009_Ltc6812/source/sys_pmm.obj \
./009_Ltc6812/source/sys_pmu.obj \
./009_Ltc6812/source/sys_selftest.obj \
./009_Ltc6812/source/sys_startup.obj \
./009_Ltc6812/source/sys_vim.obj \
./009_Ltc6812/source/system.obj 

ASM_DEPS += \
./009_Ltc6812/source/dabort.d \
./009_Ltc6812/source/os_portasm.d \
./009_Ltc6812/source/sys_core.d \
./009_Ltc6812/source/sys_intvecs.d \
./009_Ltc6812/source/sys_mpu.d \
./009_Ltc6812/source/sys_pmu.d 

OBJS__QUOTED += \
"009_Ltc6812\source\dabort.obj" \
"009_Ltc6812\source\ecap.obj" \
"009_Ltc6812\source\errata_SSWF021_45.obj" \
"009_Ltc6812\source\esm.obj" \
"009_Ltc6812\source\etpwm.obj" \
"009_Ltc6812\source\gio.obj" \
"009_Ltc6812\source\het.obj" \
"009_Ltc6812\source\notification.obj" \
"009_Ltc6812\source\os_croutine.obj" \
"009_Ltc6812\source\os_event_groups.obj" \
"009_Ltc6812\source\os_heap.obj" \
"009_Ltc6812\source\os_list.obj" \
"009_Ltc6812\source\os_mpu_wrappers.obj" \
"009_Ltc6812\source\os_port.obj" \
"009_Ltc6812\source\os_portasm.obj" \
"009_Ltc6812\source\os_queue.obj" \
"009_Ltc6812\source\os_tasks.obj" \
"009_Ltc6812\source\os_timer.obj" \
"009_Ltc6812\source\pinmux.obj" \
"009_Ltc6812\source\spi.obj" \
"009_Ltc6812\source\sys_core.obj" \
"009_Ltc6812\source\sys_dma.obj" \
"009_Ltc6812\source\sys_intvecs.obj" \
"009_Ltc6812\source\sys_main.obj" \
"009_Ltc6812\source\sys_mpu.obj" \
"009_Ltc6812\source\sys_pcr.obj" \
"009_Ltc6812\source\sys_phantom.obj" \
"009_Ltc6812\source\sys_pmm.obj" \
"009_Ltc6812\source\sys_pmu.obj" \
"009_Ltc6812\source\sys_selftest.obj" \
"009_Ltc6812\source\sys_startup.obj" \
"009_Ltc6812\source\sys_vim.obj" \
"009_Ltc6812\source\system.obj" 

C_DEPS__QUOTED += \
"009_Ltc6812\source\ecap.d" \
"009_Ltc6812\source\errata_SSWF021_45.d" \
"009_Ltc6812\source\esm.d" \
"009_Ltc6812\source\etpwm.d" \
"009_Ltc6812\source\gio.d" \
"009_Ltc6812\source\het.d" \
"009_Ltc6812\source\notification.d" \
"009_Ltc6812\source\os_croutine.d" \
"009_Ltc6812\source\os_event_groups.d" \
"009_Ltc6812\source\os_heap.d" \
"009_Ltc6812\source\os_list.d" \
"009_Ltc6812\source\os_mpu_wrappers.d" \
"009_Ltc6812\source\os_port.d" \
"009_Ltc6812\source\os_queue.d" \
"009_Ltc6812\source\os_tasks.d" \
"009_Ltc6812\source\os_timer.d" \
"009_Ltc6812\source\pinmux.d" \
"009_Ltc6812\source\spi.d" \
"009_Ltc6812\source\sys_dma.d" \
"009_Ltc6812\source\sys_main.d" \
"009_Ltc6812\source\sys_pcr.d" \
"009_Ltc6812\source\sys_phantom.d" \
"009_Ltc6812\source\sys_pmm.d" \
"009_Ltc6812\source\sys_selftest.d" \
"009_Ltc6812\source\sys_startup.d" \
"009_Ltc6812\source\sys_vim.d" \
"009_Ltc6812\source\system.d" 

ASM_DEPS__QUOTED += \
"009_Ltc6812\source\dabort.d" \
"009_Ltc6812\source\os_portasm.d" \
"009_Ltc6812\source\sys_core.d" \
"009_Ltc6812\source\sys_intvecs.d" \
"009_Ltc6812\source\sys_mpu.d" \
"009_Ltc6812\source\sys_pmu.d" 

ASM_SRCS__QUOTED += \
"../009_Ltc6812/source/dabort.asm" \
"../009_Ltc6812/source/os_portasm.asm" \
"../009_Ltc6812/source/sys_core.asm" \
"../009_Ltc6812/source/sys_intvecs.asm" \
"../009_Ltc6812/source/sys_mpu.asm" \
"../009_Ltc6812/source/sys_pmu.asm" 

C_SRCS__QUOTED += \
"../009_Ltc6812/source/ecap.c" \
"../009_Ltc6812/source/errata_SSWF021_45.c" \
"../009_Ltc6812/source/esm.c" \
"../009_Ltc6812/source/etpwm.c" \
"../009_Ltc6812/source/gio.c" \
"../009_Ltc6812/source/het.c" \
"../009_Ltc6812/source/notification.c" \
"../009_Ltc6812/source/os_croutine.c" \
"../009_Ltc6812/source/os_event_groups.c" \
"../009_Ltc6812/source/os_heap.c" \
"../009_Ltc6812/source/os_list.c" \
"../009_Ltc6812/source/os_mpu_wrappers.c" \
"../009_Ltc6812/source/os_port.c" \
"../009_Ltc6812/source/os_queue.c" \
"../009_Ltc6812/source/os_tasks.c" \
"../009_Ltc6812/source/os_timer.c" \
"../009_Ltc6812/source/pinmux.c" \
"../009_Ltc6812/source/spi.c" \
"../009_Ltc6812/source/sys_dma.c" \
"../009_Ltc6812/source/sys_main.c" \
"../009_Ltc6812/source/sys_pcr.c" \
"../009_Ltc6812/source/sys_phantom.c" \
"../009_Ltc6812/source/sys_pmm.c" \
"../009_Ltc6812/source/sys_selftest.c" \
"../009_Ltc6812/source/sys_startup.c" \
"../009_Ltc6812/source/sys_vim.c" \
"../009_Ltc6812/source/system.c" 



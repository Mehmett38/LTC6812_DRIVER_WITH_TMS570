################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
009_Ltc6812/source/%.obj: ../009_Ltc6812/source/%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/bin/armcl" -mv7R4 --code_state=32 --float_support=VFPv3D16 --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/temperature" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/temperature" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/ltc" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/crc" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/009_Ltc6812/include" --include_path="C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/include" -g --diag_warning=225 --diag_wrap=off --display_error_number --enum_type=packed --abi=eabi --preproc_with_compile --preproc_dependency="009_Ltc6812/source/$(basename $(<F)).d_raw" --obj_directory="009_Ltc6812/source" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

009_Ltc6812/source/%.obj: ../009_Ltc6812/source/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/bin/armcl" -mv7R4 --code_state=32 --float_support=VFPv3D16 --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/temperature" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/temperature" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/ltc" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x/crc" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/ltc681x" --include_path="C:/Users/arge.ortak.ASPILSAN/Downloads/tms570_ltc6812_driver/009_Ltc6812/include" --include_path="C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/include" -g --diag_warning=225 --diag_wrap=off --display_error_number --enum_type=packed --abi=eabi --preproc_with_compile --preproc_dependency="009_Ltc6812/source/$(basename $(<F)).d_raw" --obj_directory="009_Ltc6812/source" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '



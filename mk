CROSS_COMPILE=/home/geesun/workspace/arm/64-fvp/tools/gcc/gcc-linaro-6.2.1-2016.11-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-  \
              make -j 1 BL33=../64-fvp/output/fvp/components/fvp/uboot.bin PLAT=fvp  DEBUG=1 TRUSTED_BOARD_BOOT=1 ARM_ROTPK_LOCATION=devel_rsa  \
              ROT_KEY=plat/arm/board/common/rotpk/arm_rotprivk_rsa.pem MBEDTLS_DIR=./mbedtls GENERATE_COT=1\
              all fip V=1 SPD=tspd


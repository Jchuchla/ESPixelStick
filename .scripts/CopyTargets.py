import platform
import shutil
Import("env")

BUILD_DIR = env['PROJECT_BUILD_DIR']
PIOENV    = env['PIOENV']
BOARD     = env['BOARD']
PROGNAME  = env['PROGNAME']
BOARD_MCU = env['BOARD_MCU']

# print("BUILD_DIR " + BUILD_DIR)
# print("PIOENV " + PIOENV)
# print("BOARD " + BOARD)
# print("PROGNAME " + PROGNAME)
# print("BOARD_MCU " + BOARD_MCU)
# print("CustomTargets.py - Success")

BOARD_FLASH_MODE = env['BOARD_FLASH_MODE']
BOARD_F_FLASH = env['BOARD_F_FLASH'].removesuffix('000000L') + 'm'
# print("BOARD_FLASH_MODE " + BOARD_FLASH_MODE)
# print("BOARD_F_FLASH " + BOARD_F_FLASH)

SRC_DIR  = BUILD_DIR + "\\" + PIOENV + "\\"
SRC_BIN  = SRC_DIR + PROGNAME + ".bin"
SRC_PART = SRC_DIR + "partitions.bin"
# print("SRC_BIN " + SRC_BIN)

DST_DIR  = ".\\dist\\firmware\\" + BOARD_MCU + "\\"
DST_BIN  = DST_DIR + PIOENV + "-app.bin"
DST_PART = DST_DIR + PIOENV + "-partitions.bin"
DST_BOOT = DST_DIR + PIOENV + "-bootloader.bin"

def after_build(source, target, env):

    print("Copy: " + SRC_BIN)
    shutil.copyfile(SRC_BIN, DST_BIN)

    if("FLASH_EXTRA_IMAGES" in env):
        FLASH_EXTRA_IMAGES = env['FLASH_EXTRA_IMAGES']
        # print('FLASH_EXTRA_IMAGES: ')
        for imageId in range(len(FLASH_EXTRA_IMAGES)):
            ImagePath = FLASH_EXTRA_IMAGES[imageId][1]
            # print(ImagePath)
            if("boot_app0" in ImagePath):
                print("Copy: " + ImagePath)
                shutil.copyfile(ImagePath, DST_DIR + "boot_app0.bin")

            elif ("partitions" in ImagePath):
                print("Copy: " + ImagePath)
                shutil.copyfile(ImagePath, DST_PART)

            elif("bootloader" in ImagePath):
                SRC_BL_DIR = ImagePath.replace('${BOARD_FLASH_MODE}', BOARD_FLASH_MODE).replace("${__get_board_f_flash(__env__)}", BOARD_F_FLASH)
                # print("Copy SRC_BL_DIR: " + SRC_BL_DIR)
                print("Copy: " + SRC_BL_DIR)
                shutil.copyfile(SRC_BL_DIR, DST_BOOT)

env.AddPostAction("buildprog", after_build)

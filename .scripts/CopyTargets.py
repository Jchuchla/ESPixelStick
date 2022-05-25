import platform
import shutil
import os
Import("env")


def PrepareDestinationDirectory(DirRoot, DirPath):
    # os.system("ls -al ./")
    # print("mkdirs: Remove path - '" + DirRoot + "'")
    # shutil.rmtree(DirRoot, True)
    # os.system("ls -al ./")
    print("mkdirs: path - '" + DirPath + "'")
    os.makedirs(DirPath, 0x777, True)
    # os.system("ls -al ./")
    # print("chmod: " + DirRoot)
    if("Windows" != platform.system()):
        os.system("chmod -R a+rwx " + DirRoot)
        # os.system("ls -al " + DirPath)

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

SRC_DIR  = BUILD_DIR + "/" + PIOENV + "/"
SRC_BIN  = SRC_DIR + PROGNAME + ".bin"
SRC_PART = SRC_DIR + "partitions.bin"
SRC_DBG  = SRC_DIR + PROGNAME + ".elf"

# print("SRC_BIN " + SRC_BIN)

DST_ROOT  = "./firmware/"
DST_DIR   = DST_ROOT + BOARD_MCU + "/"
DST_BIN   = DST_DIR + PIOENV + "-app.bin"
DST_PART  = DST_DIR + PIOENV + "-partitions.bin"
DST_BOOT  = DST_DIR + PIOENV + "-bootloader.bin"

DBG_ROOT  = "./debug/"
DBG_DIR   = DBG_ROOT + BOARD_MCU + "/"
DST_DBG   = DBG_DIR + PIOENV + ".elf"

def after_build(source, target, env):

    DstPath = os.path.join("", DST_DIR)
    PrepareDestinationDirectory(DST_ROOT, DstPath)
    print("Copy from: '" + SRC_BIN + "' to '" + DST_BIN + "'")
    shutil.copyfile(SRC_BIN, DST_BIN)
    # print("Listing dir: " + DstPath)
    # os.system("ls -al " + DstPath + "/")

    DbgPath = os.path.join("", DBG_DIR)
    PrepareDestinationDirectory(DBG_ROOT, DbgPath)
    print("Copy from: '" + SRC_DBG + "' to '" + DST_DBG + "'")
    shutil.copyfile(SRC_DBG, DST_DBG)
    # print("Listing dir: " + DbgPath)
    # os.system("ls -al " + DbgPath + "/")

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

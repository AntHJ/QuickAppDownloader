/*
 * Copyright (C) 2021 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/ctrl.h>
#include <psp2/shellutil.h>
#include <psp2/net/http.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/net/netctl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "graphics.h"
#include "Archives.h"
#include "crc32.c"

#define printf psvDebugScreenPrintf

//#define DEBUG

#ifdef DEBUG
#define cprintf sceClibPrintf
#else
#define cprintf(...)
#endif

#define COLORPRINTF(color, ...)                \
	do {                                       \
		psvDebugScreenSetFgColor(color);       \
		printf(__VA_ARGS__);     \
		psvDebugScreenSetFgColor(COLOR_WHITE); \
	} while (0)

#define CHUNK_SIZE 64 * 1024
#define hasEndSlash(path) (path[strlen(path) - 1] == '/')

#include "ops.c" // misc, too clogged otherwise

#define REPO_URL "http://download.psvita.group/VPKs/"
#define F661_URL "http://de01.psp.update.playstation.org/update/psp/image/eu/2014_1212_6be8878f475ac5b1a499b95ab2f7d301/"
#define F661_FNAME "EBOOT.PBP"
#define QAD_VPK_FNAME "Quick.App.Downloader.vpk"
#define MNT_VPK_FNAME "QuickRemounter.vpk"

#define TEMP_UX0_PATH "ux0:temp/"
#define TEMP_UR0_PATH "ur0:temp/"
#define FIRM_661_PATH "ux0:app/PSPEMUCFW/"

#define BOOTSTRAP_VERSION_STR "VITA QUICK APP DOWNLOADER"

#define OPTION_COUNT 4
enum E_MENU_OPTIONS {
    MENU_EXIT = 0,
	MENU_INSTALL_QAD,
	MENU_INSTALL_MNT,
	MENU_DOWNLOAD_FW661,	
};

const char* menu_items[OPTION_COUNT] = { "Exit", "Install Quick App Downloader", "Install Quick Remounter", "Download Firmware 661 for Adrenaline" };

int __attribute__((naked, noinline)) call_syscall(int a1, int a2, int a3, int num) {
    __asm__(
        "mov r12, %0 \n"
        "svc 0 \n"
        "bx lr \n"
        : : "r" (num)
    );
}

int unzip(const char* src, const char* dst) {
    Zip* handle = ZipOpen(src);
    int ret = ZipExtract(handle, NULL, dst);
    ZipClose(handle);
    return ret;
}

int install_qad() {
    sceIoMkdir(TEMP_UX0_PATH, 0777);
    COLORPRINTF(COLOR_CYAN, "Downloading\n");
    net(1);
    int res = download_file(REPO_URL QAD_VPK_FNAME, TEMP_UX0_PATH QAD_VPK_FNAME, TEMP_UX0_PATH QAD_VPK_FNAME "_tmp", 0);
    if (res < 0) {
        if ((uint32_t)res == 0x80010013)
            printf("Could not open file for write, is ux0 present?\n");
        return res;
    }
    net(0);

    COLORPRINTF(COLOR_CYAN, "Extracting vpk\n");
    removeDir(TEMP_UX0_PATH "app");
    sceIoMkdir(TEMP_UX0_PATH "app", 0777);
    res = unzip(TEMP_UX0_PATH QAD_VPK_FNAME, TEMP_UX0_PATH "app");
    if (res < 0)
        return res;

    COLORPRINTF(COLOR_CYAN, "Promoting app\n");
    res = promoteApp(TEMP_UX0_PATH "app");
    if (res < 0)
        return res;

    COLORPRINTF(COLOR_GREEN, "All done\n");
    sceKernelDelayThread(2 * 1000 * 1000);

    return 0;
}

int install_mnt() {
    sceIoMkdir(TEMP_UX0_PATH, 0777);
    COLORPRINTF(COLOR_CYAN, "Downloading\n");
    net(1);
    int res = download_file(REPO_URL MNT_VPK_FNAME, TEMP_UX0_PATH MNT_VPK_FNAME, TEMP_UX0_PATH MNT_VPK_FNAME "_tmp", 0);
    if (res < 0) {
        if ((uint32_t)res == 0x80010013)
            printf("Could not open file for write, is ux0 present?\n");
        return res;
    }
    net(0);

    COLORPRINTF(COLOR_CYAN, "Extracting vpk\n");
    removeDir(TEMP_UX0_PATH "app");
    sceIoMkdir(TEMP_UX0_PATH "app", 0777);
    res = unzip(TEMP_UX0_PATH MNT_VPK_FNAME, TEMP_UX0_PATH "app");
    if (res < 0)
        return res;

    COLORPRINTF(COLOR_CYAN, "Promoting app\n");
    res = promoteApp(TEMP_UX0_PATH "app");
    if (res < 0)
        return res;

    COLORPRINTF(COLOR_GREEN, "All done\n");
    sceKernelDelayThread(2 * 1000 * 1000);

    return 0;
}

int download_661() {
    sceIoMkdir(FIRM_661_PATH, 0777);
    COLORPRINTF(COLOR_CYAN, "Downloading Firmware 661 for Adrenaline\n");
    net(1);
    int res = download_file(F661_URL F661_FNAME, FIRM_661_PATH "661.PBP", FIRM_661_PATH F661_FNAME "_tmp", 0);
    if (res < 0) {
        if ((uint32_t)res == 0x80010013)
            printf("Could not open file for write, is ux0 present?\n");
        return res;
    }
    net(0);
	
    COLORPRINTF(COLOR_GREEN, "All done\n");
    sceKernelDelayThread(2 * 1000 * 1000);

    return 0;
}

void main_menu(int sel) {
    psvDebugScreenClear(COLOR_BLACK);
	COLORPRINTF(COLOR_PURPLE, BOOTSTRAP_VERSION_STR "\n");
    COLORPRINTF(COLOR_WHITE, "\n---------------------------------------------\n\n");
    for (int i = 0; i < OPTION_COUNT; i++) {
        if (sel == i) {
            psvDebugScreenSetFgColor(COLOR_YELLOW);
            printf(" -> %s\n", menu_items[i]);
        } else
            printf(" -  %s\n", menu_items[i]);
        psvDebugScreenSetFgColor(COLOR_WHITE);
    }
    psvDebugScreenSetFgColor(COLOR_WHITE);
    COLORPRINTF(COLOR_WHITE, "\n---------------------------------------------\n\n");
}

int _start(SceSize args, void* argp) {
    int syscall_id = *(uint16_t*)argp;

    cprintf("preinit\n");
    sceAppMgrDestroyOtherApp();
    sceShellUtilInitEvents(0);
    sceShellUtilLock(0);
	sceIoRemove("ux0:VitaShell/internal/lastdir.txt");   // AutoFix for VitaShell
    cprintf("loading PAF\n");
    int res = load_sce_paf();
    if (res < 0)
        goto EXIT;

    psvDebugScreenInit();
    COLORPRINTF(COLOR_CYAN, BOOTSTRAP_VERSION_STR "\n");

    int sel = 0, launch_gamesd = 0;
    SceCtrlData pad;
    main_menu(sel);
    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        if (pad.buttons == SCE_CTRL_CROSS) {
            res = -1;
            if (sel == MENU_EXIT) {
                res = 0;
                goto EXIT;
            } else if (sel == MENU_INSTALL_QAD) {
                res = install_qad();
                if (res < 0) {
                    COLORPRINTF(COLOR_RED, "\nFAILED: 0x%08X\n", res);
                    sceKernelDelayThread(3 * 1000 * 1000);
                }
                sel = 0;
                main_menu(sel);
                sceKernelDelayThread(0.3 * 1000 * 1000);
            } else if (sel == MENU_DOWNLOAD_FW661) {
                res = download_661();
                if (res < 0) {
                    COLORPRINTF(COLOR_RED, "\nFAILED: 0x%08X\n", res);
                    sceKernelDelayThread(3 * 1000 * 1000);
                }
                sel = 0;
                main_menu(sel);
                sceKernelDelayThread(0.3 * 1000 * 1000);
			} else if (sel == MENU_INSTALL_MNT) {
                res = install_mnt();
                if (res < 0) {
                    COLORPRINTF(COLOR_RED, "\nFAILED: 0x%08X\n", res);
                    sceKernelDelayThread(3 * 1000 * 1000);
                }
                sel = 0;
                main_menu(sel);
                sceKernelDelayThread(0.3 * 1000 * 1000);				
				
            }
        } else if (pad.buttons == SCE_CTRL_UP) {
            if (sel != 0)
                sel--;
            main_menu(sel);
            sceKernelDelayThread(0.3 * 1000 * 1000);
        } else if (pad.buttons == SCE_CTRL_DOWN) {
            if (sel + 1 < OPTION_COUNT)
                sel++;
			main_menu(sel);
            sceKernelDelayThread(0.3 * 1000 * 1000);
        }
    }

    printf("All done!\n");

EXIT:
    cprintf("EXIT with res 0x%08X\n", res);
    printf("Exiting in 3\n");
    sceKernelDelayThread(3 * 1000 * 1000);
    
    // Remove pkg patches
    cprintf("Remove pkg patches.. \n");
    res = call_syscall(0, 0, 0, syscall_id + 1);
    if (res >= 0) {
        // Start HENkaku
        cprintf("Henkkek.. \n");
        printf("\nStarting the taihen framework...\n\n\nIf you are stuck on this screen:\n\n - Force reboot by holding the power button\n\n - Launch the exploit again\n\n - Hold Left Trigger [LT] while exiting\n\n\n\nIf the issue persists reset the taihen config.txt using the bootstrap menu\n");
        res = call_syscall(0, 0, 0, syscall_id + 0);
        psvDebugScreenClear(COLOR_BLACK);
    } else {
        // Remove sig patches
        cprintf("Remove sig patches\n");
        call_syscall(0, 0, 0, syscall_id + 2);
    }

    if (res < 0 && res != 0x8002D013 && res != 0x8002D017) {
        COLORPRINTF(COLOR_YELLOW, BOOTSTRAP_VERSION_STR "\n");
        COLORPRINTF(COLOR_WHITE, "\n---------------------------------------------\n\n");
        printf(" > Failed to start taihen! 0x%08X\n", res);
        printf(" > Please relaunch the exploit and select 'Install HENkaku'.\n");
    }

    if (launch_gamesd) {
        COLORPRINTF(COLOR_YELLOW, BOOTSTRAP_VERSION_STR "\n");
        COLORPRINTF(COLOR_WHITE, "\n---------------------------------------------\n\n");
        printf("Launching gamesd... ");
        res = call_syscall(0, 0, 0, syscall_id + 5);
        if (res < 0) {
            COLORPRINTF(COLOR_RED, "FAILED: 0x%08X\n", res);
            sceKernelDelayThread(3 * 1000 * 1000);
        } else
            COLORPRINTF(COLOR_GREEN, "OK\n");
        sceKernelDelayThread(3 * 1000 * 1000);
        printf("Exiting\n");
    }

    // Clean up
    cprintf("Cleanup.. \n");
    call_syscall(0, 0, 0, syscall_id + 3);

    cprintf("all done, exit\n");

    cprintf("unloading PAF\n");
    unload_sce_paf();

    sceShellUtilUnlock(1);

    sceKernelExitProcess(0);
    return 0;
}
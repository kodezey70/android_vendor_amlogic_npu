/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2019 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2014 - 2019 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/


#include <linux/device.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "gc_hal_kernel_linux.h"
#include "gc_hal_driver.h"

#include <linux/platform_device.h>

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_DRIVER

MODULE_DESCRIPTION("Vivante Graphics Driver");
MODULE_LICENSE("Dual MIT/GPL");


static struct class* gpuClass;

static gcsPLATFORM *platform;

static gckGALDEVICE galDevice;

static uint major = 199;
module_param(major, uint, 0644);
MODULE_PARM_DESC(major, "major device number for GC device");

static int irqLine = -1;
module_param(irqLine, int, 0644);
MODULE_PARM_DESC(irqLine, "IRQ number of GC core");

static ulong registerMemBase = 0x80000000;
module_param(registerMemBase, ulong, 0644);
MODULE_PARM_DESC(registerMemBase, "Base of bus address of GC core AHB register");

static ulong registerMemSize = 2 << 10;
module_param(registerMemSize, ulong, 0644);
MODULE_PARM_DESC(registerMemSize, "Size of bus address range of GC core AHB register");

static int irqLine2D = -1;
module_param(irqLine2D, int, 0644);
MODULE_PARM_DESC(irqLine2D, "IRQ number of G2D core if irqLine is used for a G3D core");

static ulong registerMemBase2D = 0x00000000;
module_param(registerMemBase2D, ulong, 0644);
MODULE_PARM_DESC(registerMemBase2D, "Base of bus address of G2D core if registerMemBase2D is used for a G3D core");

static ulong registerMemSize2D = 2 << 10;
module_param(registerMemSize2D, ulong, 0644);
MODULE_PARM_DESC(registerMemSize2D, "Size of bus address range of G2D core if registerMemSize is used for a G3D core");

static int irqLineVG = -1;
module_param(irqLineVG, int, 0644);
MODULE_PARM_DESC(irqLineVG, "IRQ number of VG core");

static ulong registerMemBaseVG = 0x00000000;
module_param(registerMemBaseVG, ulong, 0644);
MODULE_PARM_DESC(registerMemBaseVG, "Base of bus address of VG core");

static ulong registerMemSizeVG = 2 << 10;
module_param(registerMemSizeVG, ulong, 0644);
MODULE_PARM_DESC(registerMemSizeVG, "Size of bus address range of VG core");

#if gcdDEC_ENABLE_AHB
static ulong registerMemBaseDEC300 = 0x00000000;
module_param(registerMemBaseDEC300, ulong, 0644);

static ulong registerMemSizeDEC300 = 2 << 10;
module_param(registerMemSizeDEC300, ulong, 0644);
#endif

#ifndef gcdDEFAULT_CONTIGUOUS_SIZE
#define gcdDEFAULT_CONTIGUOUS_SIZE (4 << 20)
#endif
static ulong contiguousSize = gcdDEFAULT_CONTIGUOUS_SIZE;
module_param(contiguousSize, ulong, 0644);
MODULE_PARM_DESC(contiguousSize, "Size of memory reserved for GC");

static ulong contiguousBase = 0;
module_param(contiguousBase, ulong, 0644);
MODULE_PARM_DESC(contiguousBase, "Base address of memory reserved for GC, if it is 0, GC driver will try to allocate a buffer whose size defined by contiguousSize");

static ulong externalSize = 0;
module_param(externalSize, ulong, 0644);
MODULE_PARM_DESC(externalSize, "Size of external memory, if it is 0, means there is no external pool");

static ulong externalBase = 0;
module_param(externalBase, ulong, 0644);
MODULE_PARM_DESC(externalBase, "Base address of external memory");

static int fastClear = -1;
module_param(fastClear, int, 0644);
MODULE_PARM_DESC(fastClear, "Disable fast clear if set it to 0, enabled by default");

static int compression = -1;
module_param(compression, int, 0644);
MODULE_PARM_DESC(compression, "Disable compression if set it to 0, enabled by default");

static int powerManagement = 1;
module_param(powerManagement, int, 0644);
MODULE_PARM_DESC(powerManagement, "Disable auto power saving if set it to 0, enabled by default");

static int gpuProfiler = 0;
module_param(gpuProfiler, int, 0644);
MODULE_PARM_DESC(gpuProfiler, "Enable profiling support, disabled by default");

static ulong baseAddress = 0;
module_param(baseAddress, ulong, 0644);
MODULE_PARM_DESC(baseAddress, "Only used for old MMU, set it to 0 if memory which can be accessed by GPU falls into 0 - 2G, otherwise set it to 0x80000000");

static ulong physSize = 0;
module_param(physSize, ulong, 0644);
MODULE_PARM_DESC(physSize, "Obsolete");

static uint recovery = 1;
module_param(recovery, uint, 0644);
MODULE_PARM_DESC(recovery, "Recover GPU from stuck (1: Enable, 0: Disable)");

/* Middle needs about 40KB buffer, Maximal may need more than 200KB buffer. */
static uint stuckDump = 0;
module_param(stuckDump, uint, 0644);
MODULE_PARM_DESC(stuckDump, "Level of stuck dump content (1: Minimal, 2: Middle, 3: Maximal)");

static int showArgs = 1;
module_param(showArgs, int, 0644);
MODULE_PARM_DESC(showArgs, "Display parameters value when driver loaded");

static int mmu = 1;
module_param(mmu, int, 0644);
MODULE_PARM_DESC(mmu, "Disable MMU if set it to 0, enabled by default [Obsolete]");

static int irqs[gcvCORE_COUNT] = {[0 ... gcvCORE_COUNT - 1] = -1};
module_param_array(irqs, int, NULL, 0644);
MODULE_PARM_DESC(irqs, "Array of IRQ numbers of multi-GPU");

static uint registerBases[gcvCORE_COUNT];
module_param_array(registerBases, uint, NULL, 0644);
MODULE_PARM_DESC(registerBases, "Array of bases of bus address of register of multi-GPU");

static uint registerSizes[gcvCORE_COUNT] = {[0 ... gcvCORE_COUNT - 1] = 2 << 10};
module_param_array(registerSizes, uint, NULL, 0644);
MODULE_PARM_DESC(registerSizes, "Array of sizes of bus address range of register of multi-GPU");

static uint chipIDs[gcvCORE_COUNT] = {[0 ... gcvCORE_COUNT - 1] = gcvCHIP_ID_DEFAULT};
module_param_array(chipIDs, uint, NULL, 0644);
MODULE_PARM_DESC(chipIDs, "Array of chipIDs of multi-GPU");

static uint type = 0;
module_param(type, uint, 0664);
MODULE_PARM_DESC(type, "0 - Char Driver (Default), 1 - Misc Driver");

static int userClusterMask = 0;
module_param(userClusterMask, int, 0644);
MODULE_PARM_DESC(userClusterMask, "User defined cluster enable mask");

static int smallBatch = 1;
module_param(smallBatch, int, 0644);
MODULE_PARM_DESC(smallBatch, "Enable/disable small batch");

static ulong sRAMBases[gcvSRAM_COUNT * gcvCORE_COUNT] = {[0 ... gcvSRAM_COUNT * gcvCORE_COUNT - 1] = gcvINVALID_PHYSICAL_ADDRESS};
module_param_array(sRAMBases, ulong, NULL, 0644);
MODULE_PARM_DESC(sRAMBases, "Array of base of bus address of SRAM,INTERNAL, EXTERNAL0, EXTERNAL1..., 0xFFFFFFFF means no bus address");

#if USE_LINUX_PCIE
static int bar = 1;
module_param(bar, int, 0644);
MODULE_PARM_DESC(bar, "PCIE Bar index of GC core");

static int bars[gcvCORE_COUNT] = {[0 ... gcvCORE_COUNT - 1] = -1};
module_param_array(bars, int, NULL, 0644);
MODULE_PARM_DESC(bars, "Array of bar index of PCIE platform for multi-GPU");
#endif


static int gpu3DMinClock = 1;
static int contiguousRequested = 0;
static ulong bankSize = 0;


static gcsMODULE_PARAMETERS moduleParam;

static void
_InitModuleParam(
    gcsMODULE_PARAMETERS * ModuleParam
    )
{
    gctUINT i, j;
    gcsMODULE_PARAMETERS *p = ModuleParam;

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        p->irqs[i] = irqs[i];

        if (irqs[i] != -1)
        {
            p->registerBases[i] = registerBases[i];
            p->registerSizes[i] = registerSizes[i];
        }
#if USE_LINUX_PCIE
        p->bars[i] = bars[i];
#endif
    }

    /* Check legacy style. */
#if USE_LINUX_PCIE
    if (bar != -1)
    {
        p->bars[gcvCORE_MAJOR]         = bar;
    }
#endif

    if (irqLine != -1)
    {
        p->irqs[gcvCORE_MAJOR]          = irqLine;
        p->registerBases[gcvCORE_MAJOR] = registerMemBase;
        p->registerSizes[gcvCORE_MAJOR] = registerMemSize;
    }

    if (irqLine2D != -1)
    {
        p->irqs[gcvCORE_2D]          = irqLine2D;
        p->registerBases[gcvCORE_2D] = registerMemBase2D;
        p->registerSizes[gcvCORE_2D] = registerMemSize2D;
    }

    if (irqLineVG != -1)
    {
        p->irqs[gcvCORE_VG]          = irqLineVG;
        p->registerBases[gcvCORE_VG] = registerMemBaseVG;
        p->registerSizes[gcvCORE_VG] = registerMemSizeVG;
    }

#if gcdDEC_ENABLE_AHB
    if (registerMemBaseDEC300 && registerMemSizeDEC300)
    {
        p->registerBases[gcvCORE_DEC] = registerMemBaseDEC300;
        p->registerSizes[gcvCORE_DEC] = registerMemSizeDEC300;
    }
#endif

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        /* Not a module param. */
        p->registerBasesMapped[i] = gcvNULL;
    }

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        p->chipIDs[i] = chipIDs[i];
    }

    p->contiguousBase      = contiguousBase;
    p->contiguousSize      = contiguousSize;
    p->contiguousRequested = contiguousRequested;   /* not a module param. */

    p->externalBase = externalBase;
    p->externalSize = externalSize;

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        for (j = 0; j < gcvSRAM_COUNT; j++)
        {
            p->sRAMBases[i][j] = sRAMBases[i * gcvSRAM_COUNT + j];
        }
    }

    p->baseAddress = baseAddress;
    p->physSize    = physSize;
    p->bankSize    = bankSize;  /* not a module param. */

    p->recovery        = recovery;
    p->powerManagement = powerManagement;

    p->enableMmu = mmu;
    p->fastClear = fastClear;

    p->compression = (compression == -1) ? gcvCOMPRESSION_OPTION_DEFAULT
                   : (gceCOMPRESSION_OPTION)compression;
    p->gpu3DMinClock   = gpu3DMinClock; /* not a module param. */
    p->userClusterMask = userClusterMask;
    p->smallBatch      = smallBatch;

    p->stuckDump   = stuckDump;
    p->gpuProfiler = gpuProfiler;

    p->deviceType  = type;
    p->showArgs    = showArgs;

#if !gcdENABLE_3D
    p->irqs[gcvCORE_MAJOR]          = irqLine = -1;
    p->registerBases[gcvCORE_MAJOR] = registerMemBase = 0;
    p->registerSizes[gcvCORE_MAJOR] = registerMemSize = 0;
#endif

    p->irqs[gcvCORE_2D]          = irqLine2D = -1;
    p->registerBases[gcvCORE_2D] = registerMemBase2D = 0;
    p->registerSizes[gcvCORE_2D] = registerMemSize2D = 0;

    p->irqs[gcvCORE_VG]          = irqLineVG = -1;
    p->registerBases[gcvCORE_VG] = registerMemBaseVG = 0;
    p->registerSizes[gcvCORE_VG] = registerMemSizeVG = 0;
}

static void
_SyncModuleParam(
    const gcsMODULE_PARAMETERS * ModuleParam
    )
{
    gctUINT i, j;
    gcsMODULE_PARAMETERS *p = &moduleParam;

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        irqs[i]          = p->irqs[i];
        registerBases[i] = (ulong)p->registerBases[i];
        registerSizes[i] = (ulong)p->registerSizes[i];
 #if USE_LINUX_PCIE
        bars[i]          = p->bars[i];
 #endif
    }

    /* Sync to legacy style. */

#if USE_LINUX_PCIE
    bar               = p->bars[gcvCORE_MAJOR];
#endif
    irqLine           = p->irqs[gcvCORE_MAJOR];
    registerMemBase   = (ulong)p->registerBases[gcvCORE_MAJOR];
    registerMemSize   = (ulong)p->registerSizes[gcvCORE_MAJOR];

    irqLine2D         = p->irqs[gcvCORE_2D];
    registerMemBase2D = (ulong)p->registerBases[gcvCORE_2D];
    registerMemSize2D = (ulong)p->registerSizes[gcvCORE_2D];

    irqLineVG         = p->irqs[gcvCORE_VG];
    registerMemBaseVG = (ulong)p->registerBases[gcvCORE_VG];
    registerMemSizeVG = (ulong)p->registerSizes[gcvCORE_VG];

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        p->chipIDs[i] = chipIDs[i];
    }

    contiguousBase      = (ulong)p->contiguousBase;
    contiguousSize      = (ulong)p->contiguousSize;
    contiguousRequested = p->contiguousRequested;   /* not a module param. */

    externalBase = p->externalBase;
    externalSize = p->externalSize;

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        for (j = 0; j < gcvSRAM_COUNT; j++)
        {
            sRAMBases[i * gcvSRAM_COUNT + j] = (ulong)p->sRAMBases[i][j];
        }
    }

    baseAddress = (ulong)p->baseAddress;
    physSize    = p->physSize;
    bankSize    = p->bankSize;  /* not a module param. */

    recovery        = p->recovery;
    powerManagement = p->powerManagement;

    mmu             = p->enableMmu;
    fastClear       = p->fastClear;
    compression     = p->compression;
    gpu3DMinClock   = p->gpu3DMinClock; /* not a module param. */
    userClusterMask = p->userClusterMask;
    smallBatch      = p->smallBatch;

    stuckDump   = p->stuckDump;
    gpuProfiler = p->gpuProfiler;

    type        = p->deviceType;
    showArgs    = p->showArgs;
}

void
gckOS_DumpParam(
    void
    )
{
    gctINT i, j;

    printk("Galcore options:\n");
    if (irqLine != -1)
    {
        printk("  irqLine           = %d\n",      irqLine);
        printk("  registerMemBase   = 0x%08lX\n", registerMemBase);
        printk("  registerMemSize   = 0x%08lX\n", registerMemSize);
    }

    if (irqLine2D != -1)
    {
        printk("  irqLine2D         = %d\n",      irqLine2D);
        printk("  registerMemBase2D = 0x%08lX\n", registerMemBase2D);
        printk("  registerMemSize2D = 0x%08lX\n", registerMemSize2D);
    }

    if (irqLineVG != -1)
    {
        printk("  irqLineVG         = %d\n",      irqLineVG);
        printk("  registerMemBaseVG = 0x%08lX\n", registerMemBaseVG);
        printk("  registerMemSizeVG = 0x%08lX\n", registerMemSizeVG);
    }
#if USE_LINUX_PCIE
    if (bar != -1)
    {
        printk("  bar               = %d\n",     bar);
    }
#endif
#if gcdDEC_ENABLE_AHB
    printk("  registerMemBaseDEC300 = 0x%08lX\n", registerMemBaseDEC300);
    printk("  registerMemSizeDEC300 = 0x%08lX\n", registerMemSizeDEC300);
#endif

    printk("  contiguousSize    = 0x%08lX\n", contiguousSize);
    printk("  contiguousBase    = 0x%08lX\n", contiguousBase);
    printk("  externalSize      = 0x%08lX\n", externalSize);
    printk("  externalBase      = 0x%08lX\n", externalBase);
    printk("  bankSize          = 0x%08lX\n", bankSize);
    printk("  fastClear         = %d\n",      fastClear);
    printk("  compression       = %d\n",      compression);
    printk("  powerManagement   = %d\n",      powerManagement);
    printk("  baseAddress       = 0x%08lX\n", baseAddress);
    printk("  physSize          = 0x%08lX\n", physSize);
    printk("  recovery          = %d\n",      recovery);
    printk("  stuckDump         = %d\n",      stuckDump);
    printk("  gpuProfiler       = %d\n",      gpuProfiler);
    printk("  userClusterMask   = 0x%x\n",    userClusterMask);
    printk("  smallBatch        = %d\n",      smallBatch);

    printk("  irqs              = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        printk("%d, ", irqs[i]);
    }
    printk("\n");

#if USE_LINUX_PCIE
    printk("  bars              = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        printk("%d, ", bars[i]);
    }
    printk("\n");
#endif
    printk("  registerBases     = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        printk("0x%08X, ", registerBases[i]);
    }
    printk("\n");

    printk("  registerSizes     = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        printk("0x%08X, ", registerSizes[i]);
    }
    printk("\n");

    printk("  chipIDs           = ");
    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        printk("0x%08X, ", chipIDs[i]);
    }
    printk("\n");

    for (i = 0; i < gcvCORE_COUNT; i++)
    {
        printk("  core %d sRAMBases = ", i);

        for (j = 0; j < gcvSRAM_COUNT; j++)
        {
            printk("0x%08lX, ", sRAMBases[i * gcvSRAM_COUNT + j]);
        }
        printk("\n");
    }

    printk("Build options:\n");
    printk("  gcdGPU_TIMEOUT    = %d\n", gcdGPU_TIMEOUT);
    printk("  gcdGPU_2D_TIMEOUT = %d\n", gcdGPU_2D_TIMEOUT);
    printk("  gcdINTERRUPT_STATISTIC = %d\n", gcdINTERRUPT_STATISTIC);
}

static int drv_open(
    struct inode* inode,
    struct file* filp
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_PRIVATE_DATA_PTR data = gcvNULL;
    gctINT i;
    gctINT attached = 0;

    gcmkHEADER_ARG("inode=%p filp=%p", inode, filp);

    data = kmalloc(sizeof(gcsHAL_PRIVATE_DATA), GFP_KERNEL | __GFP_NOWARN);

    if (data == gcvNULL)
    {
        gcmkFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);
        return -ENOMEM;
    }

    data->device  = galDevice;
    data->pidOpen = _GetProcessID();

    /* Attached the process. */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (galDevice->kernels[i] != gcvNULL)
        {
            status = gckKERNEL_AttachProcess(galDevice->kernels[i], gcvTRUE);

            if (gcmIS_ERROR(status))
            {
                break;
            }

            attached = i;
        }
    }

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        for (i = 0; i < attached; i++)
        {
            if (galDevice->kernels[i] != gcvNULL)
            {
                gcmkVERIFY_OK(gckKERNEL_AttachProcess(galDevice->kernels[i], gcvFALSE));
            }
        }

        gcmkFOOTER_ARG("status=%d", status);
        return -ENOTTY;
    }

    filp->private_data = data;

    /* Success. */
    gcmkFOOTER_NO();
    return 0;
}

static int drv_release(
    struct inode* inode,
    struct file* filp
    )
{
    int ret = -ENOTTY;
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_PRIVATE_DATA_PTR data;
    gckGALDEVICE device;
    gctINT i;

    gcmkHEADER_ARG("inode=%p filp=%p", inode, filp);

    data = filp->private_data;

    if (data == gcvNULL)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): private_data is NULL\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    device = data->device;

    if (device == gcvNULL)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): device is NULL\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* A process gets detached. */
    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (galDevice->kernels[i] != gcvNULL)
        {
            gcmkVERIFY_OK(gckKERNEL_AttachProcessEx(
                galDevice->kernels[i],
                gcvFALSE,
                data->pidOpen
                ));
        }
    }

    kfree(data);
    filp->private_data = NULL;

    /* Success. */
    ret = 0;

OnError:
    gcmkFOOTER();
    return ret;
}

static long drv_ioctl(
    struct file* filp,
    unsigned int ioctlCode,
    unsigned long arg
    )
{
    long ret = -ENOTTY;
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;
    gctUINT32 copyLen;
    DRIVER_ARGS drvArgs;
    gckGALDEVICE device;
    gcsHAL_PRIVATE_DATA_PTR data;

    gcmkHEADER_ARG("filp=%p ioctlCode=%u arg=%lu",filp, ioctlCode, arg);

    data = filp->private_data;

    if (data == gcvNULL)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): private_data is NULL\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    device = data->device;

    if (device == gcvNULL)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): device is NULL\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if ((ioctlCode != IOCTL_GCHAL_INTERFACE)
    &&  (ioctlCode != IOCTL_GCHAL_KERNEL_INTERFACE)
    )
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): unknown command %d\n",
            __FUNCTION__, __LINE__,
            ioctlCode
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Get the drvArgs. */
    copyLen = copy_from_user(
        &drvArgs, (void *) arg, sizeof(DRIVER_ARGS)
        );

    if (copyLen != 0)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): error copying of the input arguments.\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Now bring in the gcsHAL_INTERFACE structure. */
    if ((drvArgs.InputBufferSize  != sizeof(gcsHAL_INTERFACE))
    ||  (drvArgs.OutputBufferSize != sizeof(gcsHAL_INTERFACE))
    )
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): input or/and output structures are invalid.\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    copyLen = copy_from_user(
        &iface, gcmUINT64_TO_PTR(drvArgs.InputBuffer), sizeof(gcsHAL_INTERFACE)
        );

    if (copyLen != 0)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): error copying of input HAL interface.\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    status = gckDEVICE_Dispatch(device->device, &iface);

    /* Redo system call after pending signal is handled. */
    if (status == gcvSTATUS_INTERRUPTED)
    {
        ret = -ERESTARTSYS;
        gcmkONERROR(status);
    }

    /* Copy data back to the user. */
    copyLen = copy_to_user(
        gcmUINT64_TO_PTR(drvArgs.OutputBuffer), &iface, sizeof(gcsHAL_INTERFACE)
        );

    if (copyLen != 0)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_ERROR, gcvZONE_DRIVER,
            "%s(%d): error copying of output HAL interface.\n",
            __FUNCTION__, __LINE__
            );

        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Success. */
    ret = 0;

OnError:
    gcmkFOOTER();
    return ret;
}

static struct file_operations driver_fops =
{
    .owner      = THIS_MODULE,
    .open       = drv_open,
    .release    = drv_release,
    .unlocked_ioctl = drv_ioctl,
#ifdef HAVE_COMPAT_IOCTL
    .compat_ioctl = drv_ioctl,
#endif
};

static struct miscdevice gal_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &driver_fops,
};

static int drv_init(void)
{
    int ret = -EINVAL;
    gceSTATUS status;
    gckGALDEVICE device = gcvNULL;
    struct class* device_class = gcvNULL;

    gcmkHEADER();

    printk(KERN_INFO "Galcore version %d.%d.%d.%d\n",
        gcvVERSION_MAJOR, gcvVERSION_MINOR, gcvVERSION_PATCH, gcvVERSION_BUILD);

    if (showArgs)
    {
        gckOS_DumpParam();
    }

    /* Create the GAL device. */
    status = gckGALDEVICE_Construct(platform, &moduleParam, &device);

    if (gcmIS_ERROR(status))
    {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
                       "%s(%d): Failed to create the GAL device: status=%d\n",
                       __FUNCTION__, __LINE__, status);

        goto OnError;
    }

    /* Start the GAL device. */
    gcmkONERROR(gckGALDEVICE_Start(device));

    if ((physSize != 0)
       && (device->kernels[gcvCORE_MAJOR] != gcvNULL)
       && (device->kernels[gcvCORE_MAJOR]->hardware->mmuVersion != 0))
    {
        /* Reset the base address */
        device->baseAddress = 0;
    }

    /* Set global galDevice pointer. */
    galDevice = device;

    if (type == 1)
    {
        /* Register as misc driver. */
        if (misc_register(&gal_device) < 0)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): misc_register fails.\n",
                __FUNCTION__, __LINE__
                );

            gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }
    }
    else
    {
        /* Register the character device. */
        int result = register_chrdev(major, DEVICE_NAME, &driver_fops);

        if (result < 0)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): Could not allocate major number for mmap.\n",
                __FUNCTION__, __LINE__
                );

            gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        if (major == 0)
        {
            major = result;
        }

        /* Create the device class. */
        device_class = class_create(THIS_MODULE, CLASS_NAME);

        if (IS_ERR(device_class))
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): Failed to create the class.\n",
                __FUNCTION__, __LINE__
                );

            gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
        device_create(device_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
#else
        device_create(device_class, NULL, MKDEV(major, 0), DEVICE_NAME);
#endif

        gpuClass  = device_class;
    }

    gcmkTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_DRIVER,
        "%s(%d): irqLine=%d, contiguousSize=%lu, memBase=0x%lX\n",
        __FUNCTION__, __LINE__,
        irqLine, contiguousSize, registerMemBase
        );

    /* Success. */
    ret = 0;

OnError:
    if (ret)
    {
        /* Roll back. */
        if (device_class)
        {
            device_destroy(device_class, MKDEV(major, 0));
            class_destroy(device_class);
        }

        if (device)
        {
            gcmkVERIFY_OK(gckGALDEVICE_Stop(device));
            gcmkVERIFY_OK(gckGALDEVICE_Destroy(device));
        }
    }

    gcmkFOOTER();
    return ret;
}

static void drv_exit(void)
{
    gcmkHEADER();

    if (type == 1)
    {
        misc_deregister(&gal_device);
    }
    else
    {
        gcmkASSERT(gpuClass != gcvNULL);
        device_destroy(gpuClass, MKDEV(major, 0));
        class_destroy(gpuClass);

        unregister_chrdev(major, DEVICE_NAME);
    }

    gcmkVERIFY_OK(gckGALDEVICE_Stop(galDevice));
    gcmkVERIFY_OK(gckGALDEVICE_Destroy(galDevice));

    gcmkFOOTER_NO();
}


struct device *galcore_device;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
int gpu_probe(struct platform_device *pdev)
#else
int __devinit gpu_probe(struct platform_device *pdev)
#endif
{
    int ret = -ENODEV;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
    static u64 dma_mask = DMA_BIT_MASK(40);
#else
    static u64 dma_mask = DMA_40BIT_MASK;
#endif


    gcmkHEADER();

    if (get_nna_status(pdev) == 1)
    {
        printk("nn is disable,should not do probe continue\n");
        return ret;
    }
    platform->device = pdev;
    galcore_device = &pdev->dev;

    galcore_device->dma_mask = &dma_mask;

    galcore_device->coherent_dma_mask = dma_mask;

    if (platform->ops->getPower)
    {
        if (gcmIS_ERROR(platform->ops->getPower(platform)))
        {
            gcmkFOOTER_NO();
            return ret;
        }
    }

    /* Gather module parameters. */
    _InitModuleParam(&moduleParam);

    if (platform->ops->adjustParam)
    {
        /* Override default module param. */
        platform->ops->adjustParam(platform, &moduleParam);
    }

    /* Update module param because drv_init() uses them directly. */
    _SyncModuleParam(&moduleParam);

    ret = drv_init();

    if (!ret)
    {
        platform_set_drvdata(pdev, galDevice);
    }

    if (ret < 0)
    {
        gcmkFOOTER_ARG(KERN_INFO "Failed to register gpu driver: %d\n", ret);
    }
    else
    {
        gcmkFOOTER_NO();
    }

    gcmkFOOTER_ARG(KERN_INFO "Success ret=%d", ret);
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
int gpu_remove(struct platform_device *pdev)
#else
int __devexit gpu_remove(struct platform_device *pdev)
#endif
{
    gcmkHEADER();

    drv_exit();

    if (platform->ops->putPower)
    {
        platform->ops->putPower(platform);
    }

    gcmkFOOTER_NO();
    return 0;
}

static int gpu_suspend(struct platform_device *dev, pm_message_t state)
{
    gceSTATUS status;
    gckGALDEVICE device;
    gctINT i;

    device = platform_get_drvdata(dev);

    if (!device)
    {
        return -1;
    }

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->kernels[i] != gcvNULL)
        {
            /* Store states. */
            {
                status = gckHARDWARE_QueryPowerManagementState(device->kernels[i]->hardware, &device->statesStored[i]);
            }

            if (gcmIS_ERROR(status))
            {
                return -1;
            }

            {
                status = gckHARDWARE_SetPowerManagementState(device->kernels[i]->hardware, gcvPOWER_OFF);
            }

            if (gcmIS_ERROR(status))
            {
                return -1;
            }

        }
    }

    return 0;
}

static int gpu_resume(struct platform_device *dev)
{
    gceSTATUS status;
    gckGALDEVICE device;
    gctINT i;
    gceCHIPPOWERSTATE   statesStored;

    device = platform_get_drvdata(dev);

    if (!device)
    {
        return -1;
    }

    for (i = 0; i < gcdMAX_GPU_COUNT; i++)
    {
        if (device->kernels[i] != gcvNULL)
        {
            {
                status = gckHARDWARE_SetPowerManagementState(device->kernels[i]->hardware, gcvPOWER_ON);
            }

            if (gcmIS_ERROR(status))
            {
                return -1;
            }

            /* Convert global state to crossponding internal state. */
            switch(device->statesStored[i])
            {
            case gcvPOWER_ON:
                statesStored = gcvPOWER_ON_AUTO;
                break;
            case gcvPOWER_IDLE:
                statesStored = gcvPOWER_IDLE_BROADCAST;
                break;
            case gcvPOWER_SUSPEND:
                statesStored = gcvPOWER_SUSPEND_BROADCAST;
                break;
            case gcvPOWER_OFF:
                statesStored = gcvPOWER_OFF_BROADCAST;
                break;
            default:
                statesStored = device->statesStored[i];
                break;
            }

            /* Restore states. */
            {
                status = gckHARDWARE_SetPowerManagementState(device->kernels[i]->hardware, statesStored);
            }

            if (gcmIS_ERROR(status))
            {
                return -1;
            }
        }
    }

    return 0;
}

#if defined(CONFIG_PM) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
#ifdef CONFIG_PM_SLEEP
static int gpu_system_suspend(struct device *dev)
{
    pm_message_t state={0};
    return gpu_suspend(to_platform_device(dev), state);
}

static int gpu_system_resume(struct device *dev)
{
    return gpu_resume(to_platform_device(dev));
}
#endif

static const struct dev_pm_ops gpu_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(gpu_system_suspend, gpu_system_resume)
};
#endif

static struct platform_driver gpu_driver = {
    .probe      = gpu_probe,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
    .remove     = gpu_remove,
#else
    .remove     = __devexit_p(gpu_remove),
#endif

    .suspend    = gpu_suspend,
    .resume     = gpu_resume,

    .driver     = {
        .owner  = THIS_MODULE,
        .name   = DEVICE_NAME,
#if defined(CONFIG_PM) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
        .pm     = &gpu_pm_ops,
#endif
    }
};

#if gcdENABLE_DRM
#if USE_LINUX_PCIE
int viv_drm_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
void viv_drm_remove(struct pci_dev *pdev);
#else
int viv_drm_probe(struct platform_device *pdev);
int viv_drm_remove(struct platform_device *pdev);
#endif
#endif

static int __init gpu_init(void)
{
    int ret = 0;

#if gcdENABLE_DRM
    gpu_driver.probe = viv_drm_probe;
    gpu_driver.remove = viv_drm_remove;
#endif

    ret = gckPLATFORM_Init(&gpu_driver, &platform);

    if (ret || !platform)
    {
        printk(KERN_ERR "galcore: Soc platform init failed.\n");
        return -ENODEV;
    }

    ret = platform_driver_register(&gpu_driver);

    if (ret)
    {
        printk(KERN_ERR "galcore: gpu_init() failed to register driver!\n");
        gckPLATFORM_Terminate(platform);
        platform = NULL;
        return -ENODEV;
    }

    platform->driver = &gpu_driver;
    return 0;
}

static void __exit gpu_exit(void)
{
    platform_driver_unregister(&gpu_driver);

    gckPLATFORM_Terminate(platform);
    platform = NULL;
}

module_init(gpu_init);
module_exit(gpu_exit);

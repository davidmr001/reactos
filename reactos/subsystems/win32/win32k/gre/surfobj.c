/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/bitmap.c
 * PURPOSE:         Surface and Bitmap Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>, taken from ReactOS eng/surface.c
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

ULONG
NTAPI
GrepBitmapFormat(WORD Bits, DWORD Compression)
{
    switch (Compression)
    {
        case BI_RGB:
            /* Fall through */
        case BI_BITFIELDS:
            switch (Bits)
            {
                case 1:
                    return BMF_1BPP;
                case 4:
                    return BMF_4BPP;
                case 8:
                    return BMF_8BPP;
                case 16:
                    return BMF_16BPP;
                case 24:
                    return BMF_24BPP;
                case 32:
                    return BMF_32BPP;
            }
            return 0;

        case BI_RLE4:
            return BMF_4RLE;

        case BI_RLE8:
            return BMF_8RLE;

        default:
            return 0;
    }
}

INT
FASTCALL
BitsPerFormat(ULONG Format)
{
    switch (Format)
    {
        case BMF_1BPP:
            return 1;

        case BMF_4BPP:
            /* Fall through */
        case BMF_4RLE:
            return 4;

        case BMF_8BPP:
            /* Fall through */
        case BMF_8RLE:
            return 8;

        case BMF_16BPP:
            return 16;

        case BMF_24BPP:
            return 24;

        case BMF_32BPP:
            return 32;

        default:
            return 0;
    }
}

INT FASTCALL
BITMAP_GetWidthBytes(INT bmWidth, INT bpp)
{
#if 0
    switch (bpp)
    {
    case 1:
        return 2 * ((bmWidth+15) >> 4);

    case 24:
        bmWidth *= 3; /* fall through */
    case 8:
        return bmWidth + (bmWidth & 1);

    case 32:
        return bmWidth * 4;

    case 16:
    case 15:
        return bmWidth * 2;

    case 4:
        return 2 * ((bmWidth+3) >> 2);

    default:
        DPRINT ("stub");
    }

    return -1;
#endif

    return ((bmWidth * bpp + 15) & ~15) >> 3;
}


/* PUBLIC FUNCTIONS **********************************************************/

HBITMAP
GreCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    PSURFACE pSurface;
    SURFOBJ *pSurfObj;
    HBITMAP hSurface;

    if (Format == 0)
        return 0;

    /* Allocate storage for surface object */
    pSurface = (PSURFACE)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_BITMAP);
    if (!pSurface) return 0;

    /* Save a handle to it */
    hSurface = pSurface->BaseObject.hHmgr;

    /* Check the format */
    if (Format == BMF_4RLE || Format == BMF_8RLE)
    {
        DPRINT1("Bitmaps with format 0x%x aren't supported yet!\n", Format);

        /* Cleanup and exit */
        GDIOBJ_FreeObjByHandle(hSurface, GDI_OBJECT_TYPE_BITMAP);
        return 0;
    }

    /* Initialize SURFOBJ */
    pSurfObj = &pSurface->SurfObj;

    pSurfObj->hsurf = (HSURF)hSurface;
    pSurfObj->sizlBitmap = Size;
    pSurfObj->iType = STYPE_BITMAP;
    pSurfObj->fjBitmap = Flags & (BMF_TOPDOWN | BMF_NOZEROINIT);

    /* Calculate byte width automatically if it was not provided */
    if (Width == 0)
        Width = BITMAP_GetWidthBytes(Size.cx, BitsPerFormat(Format));

    pSurfObj->lDelta = abs(Width);
    pSurfObj->cjBits = pSurfObj->lDelta * Size.cy;

    if (!Bits)
    {
        /* Allocate memory for bitmap bits */
        pSurfObj->pvBits = EngAllocMem(0 != (Flags & BMF_NOZEROINIT) ? 0 : FL_ZERO_MEMORY,
                                       pSurfObj->cjBits, TAG_DIB);

        if (!pSurfObj->pvBits)
        {
            /* Cleanup and exit */
            GDIOBJ_FreeObjByHandle(hSurface, GDI_OBJECT_TYPE_BITMAP);
            return 0;
        }
    }
    else
    {
        pSurfObj->pvBits = Bits;
    }

    pSurfObj->pvScan0 = pSurfObj->pvBits;

    /* Override the 0th scanline if it's topdown */
    if (Flags & BMF_TOPDOWN)
    {
        pSurfObj->pvScan0 = (PVOID)((ULONG_PTR)pSurfObj->pvBits + pSurfObj->cjBits - pSurfObj->lDelta);
        pSurfObj->lDelta = -pSurfObj->lDelta;
    }

    /* Set the format */
    pSurfObj->iBitmapFormat = Format;

    /* Unlock the surface */
    SURFACE_Unlock(pSurface);

    /* Return handle to it */
    return hSurface;
}

VOID FASTCALL
GreDeleteBitmap(HGDIOBJ hBitmap)
{
    GDIOBJ_FreeObjByHandle(hBitmap, GDI_OBJECT_TYPE_BITMAP);
}

LONG FASTCALL
GreGetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits)
{
    /* Don't copy more bytes than the buffer has */
    ulBytes = min(ulBytes, pSurf->SurfObj.cjBits);

    /* Copy actual bits */
    RtlCopyMemory(pBits, pSurf->SurfObj.pvBits, ulBytes);

    /* Return amount copied */
    return ulBytes;
}

LONG FASTCALL
GreSetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits)
{
    /* Check ulBytes */
    if (!ulBytes) return 0;

    /* Don't copy more bytes than the surface has */
    ulBytes = min(ulBytes, pSurf->SurfObj.cjBits);

    /* Copy actual bits */
    RtlCopyMemory(pSurf->SurfObj.pvBits, pBits, ulBytes);

    /* Return amount copied */
    return ulBytes;
}

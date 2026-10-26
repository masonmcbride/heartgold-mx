"""Compile selected original NitroSystem storage/lifecycle operations for ARM32.

SDK sources/headers and emitted code remain generated inputs outside the tracked
tree. Unsupported active rendering callbacks retain named guest entry identities;
this helper does not substitute a renderer for those operations.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess

from generated_files import write_text_if_changed


def _body(text, name):
    matches = list(re.finditer(
        r"^[\w* \t]+\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{.*?^\}",
        text, re.M | re.S))
    if len(matches) != 1:
        raise ValueError(("original SDK definition drift", name, len(matches)))
    return matches[0][0]


def generate(root, out, cc, sdk_root=None, nns_root=None):
    """Return (assembly, provenance records, original dependency paths)."""
    root, out = Path(root).resolve(), Path(out).resolve()
    # The parent helper emits only standard-library declarations and invokes the
    # SDK's own fixed-constant generator. No SDK header is rewritten.
    from prepare_original_2d import compiler_flags, prepare_headers, sdk_paths
    sdk_root, nns_root = sdk_paths(root, sdk_root, nns_root)
    support, support_dependencies = prepare_headers(out, sdk_root)
    out.mkdir(parents=True, exist_ok=True)
    records, dependencies, assembly = [], list(support_dependencies), []
    gx_source = sdk_root / "libraries/gx/src/gx.c"
    gx_data = re.findall(r"^u32 GXi_DmaId = GX_DEFAULT_DMAID;", gx_source.read_text(), re.M)
    if len(gx_data) != 1:
        raise ValueError("original GX default DMA declaration drift")
    dependencies.append(gx_source.resolve())
    units = [
        ("renderer", "libraries/g2d/src/g2d_Renderer.c", """
#include <nnsys/g2d/g2d_Renderer.h>
#include "g2d_Internal.h"
#include "g2di_RendererMtxStack.hpp"
#include "g2di_RendererMtxState.h"
extern void RndCoreCBFuncBeforeCell_(struct NNSG2dRndCoreInstance *, const NNSG2dCellData *);
extern void RndCoreCBFuncAfterCell_(struct NNSG2dRndCoreInstance *, const NNSG2dCellData *);
extern void RndCoreCBFuncBeforeOBJ_(struct NNSG2dRndCoreInstance *, const NNSG2dCellData *, u16);
extern void RndCoreCBFuncAfterOBJ_(struct NNSG2dRndCoreInstance *, const NNSG2dCellData *, u16);
_Static_assert(sizeof(void *) == 4 && sizeof(GXOamAttr) == 8, "ARM32/OAM ABI");
_Static_assert(sizeof(NNSG2dRndCoreInstance) == 0x70, "ARM32 renderer core");
_Static_assert(sizeof(NNSG2dRendererInstance) == 0x98, "ARM32 renderer");
_Static_assert(__builtin_offsetof(NNSG2dRendererInstance, spriteZoffsetStep) == 0x80, "GF renderer zoffset");
_Static_assert(sizeof(NNSG2dRenderSurface) == 0x48, "ARM32 surface");
_Static_assert(__builtin_offsetof(NNSG2dRenderSurface, pFuncOamRegister) == 0x28, "GF OAM callback");
_Static_assert(__builtin_offsetof(NNSG2dRenderSurface, pNextSurface) == 0x30, "surface list link");
""", ("IsNotCircularLinked_", "NNS_G2dInitRenderer",
             "NNS_G2dAddRendererTargetSurface", "NNS_G2dInitRenderSurface")),
        ("core", "libraries/g2d/src/g2d_RendererCore.c", """
#include <nnsys/g2d/g2d_RendererCore.h>
""", ("NNS_G2dInitRndCore", "NNS_G2dInitRndCoreSurface")),
        ("image", "libraries/g2d/src/g2d_Image.c", """
#include <nnsys/g2d/g2d_Image.h>
static void InitializeVRamLocation_(NNSG2dVRamLocation *);
static void SetVramLocation_(NNSG2dVRamLocation *, NNS_G2D_VRAM_TYPE, u32);
static u32 GetVramLocation_(const NNSG2dVRamLocation *, NNS_G2D_VRAM_TYPE);
static BOOL IsImageReadyToUse_(const NNSG2dVRamLocation *, NNS_G2D_VRAM_TYPE);
_Static_assert(sizeof(NNSG2dImageProxy) == 0x24, "ARM32 image proxy");
_Static_assert(sizeof(NNSG2dImagePaletteProxy) == 0x14, "ARM32 palette proxy");
""", ("InitializeVRamLocation_", "SetVramLocation_", "GetVramLocation_",
             "IsImageReadyToUse_", "NNS_G2dInitImageProxy",
             "NNS_G2dSetImageLocation", "NNS_G2dGetImageLocation",
             "NNS_G2dIsImageReadyToUse", "NNS_G2dInitImagePaletteProxy",
             "NNS_G2dSetImagePaletteLocation", "NNS_G2dGetImagePaletteLocation",
             "NNS_G2dIsImagePaletteReadyToUse")),
        ("nan", "libraries/g2d/src/load/g2d_NAN_load.c", """
#include <nnsys/g2d/load/g2d_NAN_load.h>
#include <nnsys/g2d/g2d_Load.h>
#include "g2di_Debug.h"
_Static_assert(sizeof(NNSG2dAnimBankData) == 0x18, "ARM32 animation bank");
_Static_assert(sizeof(NNSG2dAnimSequenceData) == 0x10, "ARM32 animation sequence");
_Static_assert(sizeof(NNSG2dAnimFrameData) == 8, "ARM32 animation frame");
""", ("GetUnpackedAnimBankImpl_", "CheckAnimSequenceValidity_",
             "NNS_G2dGetUnpackedAnimBank", "NNS_G2dUnpackNAN",
             "NNS_G2dGetAnimSequenceByIdx")),
        ("load", "libraries/g2d/src/load/g2d_Load.c", """
#include <nnsys/g2d/g2d_Load.h>
#include "g2di_Debug.h"
""", ("NNS_G2dFindBinaryBlock",)),
        ("oam", "libraries/g2d/src/g2d_OAM.c", """
#include <nnsys/g2d/g2d_OAM.h>
_Static_assert(sizeof(NNSG2dOamManagerInstance) == 0x1c, "ARM32 OAM instance");
_Static_assert(__builtin_offsetof(NNSG2dOamManagerInstance, managedAttrArea) == 4, "OAM attribute range");
_Static_assert(__builtin_offsetof(NNSG2dOamManagerInstance, managedAffineArea) == 0xa, "OAM affine range");
_Static_assert(__builtin_offsetof(NNSG2dOamManagerInstance, GUID) == 0x10, "OAM reservation owner");
_Static_assert(__builtin_offsetof(NNSG2dOamManagerInstance, bFastTransferEnable) == 0x14, "OAM transfer policy");
""", None),
    ]
    units.append(("dict", "libraries/g3d/src/binres/res_struct_accessor.c",
        "#include <nnsys/g3d/binres/res_struct_accessor.h>\n",
        ("NNS_G3dGetResDataByName","NNS_G3dGetResDictIdxByName")))
    for label, relative, prefix, names in units:
        path = nns_root / relative
        original = path.read_text()
        data_provenance = []
        if label == "oam":
            # The SDK's default channel is original guest state, shared by its
            # inline GX_GetDefaultDMA consumer, rather than a synthetic handle.
            prefix += "\n" + gx_data[0] + "\n"
            data_provenance.append(dict(source=str(gx_source.resolve()),
                                        source_sha256=hashlib.sha256(gx_source.read_bytes()).hexdigest(),
                                        declaration_sha256=hashlib.sha256(gx_data[0].encode()).hexdigest(),
                                        symbol="GXi_DmaId"))
        if names is None:
            # Original OAM algorithms share one reservation table and one
            # interleaved attribute/affine buffer per engine. Compile the whole
            # unit so publication/reset retain their original sole owner.
            names = tuple(re.findall(
                r"^[A-Za-z_][\w* \t]+[ \t*](\w+)\s*\([^;{}]*\)\s*\{",
                original, re.M))
            bodies = [_body(original, name) for name in names]
            code = original + "\n_Static_assert(sizeof(OamCache) == 0x540, \"original OAM cache\");\n"
        else:
            bodies = [_body(original, name) for name in names]
            code = "\n\n".join(bodies)
        source, target, depfile = (out / ("original_2d_sdk_" + label + suffix)
                                   for suffix in (".c", ".s", ".d"))
        write_text_if_changed(source, prefix + "\n" + code + "\n")
        command = [str(cc), *compiler_flags(support, sdk_root, nns_root),
                   "-Wno-static-in-inline",
                   "-I" + str(nns_root / "libraries/g2d/include"),
                   "-MMD", "-MF", str(depfile), "-S", str(source), "-o", str(target)]
        subprocess.run(command, check=True)
        text = target.read_text()
        # Compiler local labels from separately compiled units share one guest
        # assembler input; retain their edges while giving them unique names.
        text = re.sub(r"\.L([\w.$]+)", ".Lsdk2d_" + label + r"_\1", text)
        assembly.append("\n.set hg_compiler_syntax, 1\n" + text +
                        "\n.set hg_compiler_syntax, 0\n")
        dependencies.append(path.resolve())
        deps = depfile.read_text().replace("\\\n", " ").split(":", 1)[1].split()
        dependencies.extend(Path(p).resolve() for p in deps
                            if Path(p).resolve() != source.resolve())
        records.append(dict(source=str(path.resolve()),
                            source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                            functions={name: hashlib.sha256(body.encode()).hexdigest()
                                       for name, body in zip(names, bodies)},
                            declarations_sha256=hashlib.sha256(prefix.encode()).hexdigest(),
                            original_data=data_provenance,
                            compiler_target="arm-none-eabi arm946e-s Thumb",
                            adaptation="unchanged original bodies and SDK headers; ARM32 compiler ABI; release SDK assertions"))
    records.append(dict(compiler_compatibility="SDK_CW enables original little-endian bitfield and alignment declarations; unsupported CW clz assembly is bypassed by existing declared CLZ function; libc headers contain declarations only",
                        unprovided_callbacks=["RndCoreCBFuncBeforeCell_", "RndCoreCBFuncAfterCell_", "RndCoreCBFuncBeforeOBJ_", "RndCoreCBFuncAfterOBJ_"],
                        boundary="original construction/software-record initialization, canonical guest pointers, callback identities, animation relocation, and complete original OAM reservations/entry/publication/reset; active renderer operations and retained renderer callbacks remain unsupported",
                        extension_obligation="compile renderer active operations and private matrix/cache state together before admitting retained callbacks; partial init-only compilation may eliminate unobservable private state"))
    write_text_if_changed(out / "original_2d_sdk_provenance.json",
                          json.dumps(records, indent=2) + "\n")
    return "\n".join(assembly), records, sorted(set(dependencies))


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("cc", type=Path)
    parser.add_argument("--sdk-root", type=Path)
    parser.add_argument("--nns-root", type=Path)
    args = parser.parse_args()
    asm, _, _ = generate(args.root, args.out, args.cc, args.sdk_root, args.nns_root)
    write_text_if_changed(args.out / "original_2d_sdk.s", asm)

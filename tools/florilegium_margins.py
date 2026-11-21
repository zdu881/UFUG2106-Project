#!/usr/bin/env python3
"""Render upstream florilegium SVG using the installed Lua 5.3 shared library.
No TeX is needed. Upstream code remains in the explicitly supplied checkout.
"""
import argparse
import ctypes
import ctypes.util
import json
import copy
import io
import numpy as np

import cairosvg
from PIL import Image
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET

from report_theme import ACCENT, SECONDARY


def execute_lua(source):
    library = ctypes.util.find_library('lua5.3')
    if not library:
        raise RuntimeError('Install the system Lua 5.3 shared library to generate margins')
    lib = ctypes.CDLL(library)
    lib.luaL_newstate.restype = ctypes.c_void_p
    lib.luaL_openlibs.argtypes = [ctypes.c_void_p]
    lib.luaL_loadstring.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.luaL_loadstring.restype = ctypes.c_int
    lib.lua_pcallk.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_longlong, ctypes.c_void_p]
    lib.lua_pcallk.restype = ctypes.c_int
    lib.lua_tolstring.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p]
    lib.lua_tolstring.restype = ctypes.c_char_p
    lib.lua_close.argtypes = [ctypes.c_void_p]
    state = lib.luaL_newstate()
    if not state:
        raise RuntimeError('Cannot create Lua state')
    try:
        lib.luaL_openlibs(state)
        error = lib.luaL_loadstring(state, source.encode())
        if not error:
            error = lib.lua_pcallk(state, 0, 0, 0, 0, None)
        if error:
            raise RuntimeError(lib.lua_tolstring(state, -1, None).decode())
    finally:
        lib.lua_close(state)


def prepare_ornament(root):
    """Expose the complete native L-shaped garden, including its joined footer."""
    ns = '{http://www.w3.org/2000/svg}'
    drawing = root.find(ns + 'g')
    clip = drawing.find(ns + 'clipPath')
    assert clip is not None, 'Expected upstream page safety clip'
    reference = 'url(#' + clip.attrib['id'] + ')'
    drawing.remove(clip)
    for element in drawing.iter():
        if element.get('clip-path') == reference:
            del element.attrib['clip-path']
    return drawing


def fit_ornament(path, drawing):
    """Uniformly fit a connected native garden into the left/bottom margins.

    Bound every painted pixel against the protected text/footer rectangle;
    preserve the native root, turning branch, leaves and tail as one drawing.
    """
    ns = '{http://www.w3.org/2000/svg}'
    pt = 72 / 25.4
    probe = ET.Element(ns + 'svg', {'viewBox':'-100 -100 900 1050',
                                  'width':'900', 'height':'1050'})
    probe.append(copy.deepcopy(drawing))
    bitmap = Image.open(io.BytesIO(cairosvg.svg2png(bytestring=ET.tostring(probe), scale=2)))
    bounds = bitmap.getbbox()
    assert bounds and bounds[0] > 0 and bounds[1] > 0
    assert bounds[2] < bitmap.width and bounds[3] < bitmap.height
    left, top, right, bottom = [value/2-100 for value in bounds]
    left -= 1
    top -= 1
    right += 1
    bottom += 1
    ys, xs = np.where(np.asarray(bitmap)[:,:,3] > 0)
    dx = xs/2-100-left
    dy = bottom-(ys/2-100)
    # x >= 44 mm and y <= 277 mm is reserved for text and its clearance.
    # One extra millimeter protects against raster-bound estimation error.
    collision_scales = np.maximum(40*pt/np.maximum(dx,1e-6),
                                  16*pt/np.maximum(dy,1e-6))
    scale = min(0.97, 201*pt/(right-left), 276*pt/(bottom-top),
                float(collision_scales.min()))
    tx = 3*pt-scale*left
    ty = 294*pt-scale*bottom
    root = ET.Element(ns+'svg', {'width':'210mm','height':'297mm',
                                'viewBox':'0 0 595.28 841.89'})
    group = ET.SubElement(root, ns+'g', {'transform':f'translate({tx} {ty}) scale({scale})'})
    group.append(drawing)
    path.write_text(ET.tostring(root,encoding='unicode'))
    cairosvg.svg2png(url=str(path),write_to=str(path.with_suffix('.png')),dpi=300)
    print(path.name, flush=True)
    return {'file':path.name,'scale':scale,'source_bounds_pt':[left,top,right,bottom]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    upstream = args.upstream.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    commit = subprocess.check_output(['git', '-C', str(upstream), 'rev-parse', 'HEAD'], text=True).strip()
    placements = []
    ET.register_namespace('', 'http://www.w3.org/2000/svg')
    for page in range(1, 7):
        path = output / f'margin-{page}.svg'
        script = f'''package.path = {json.dumps(str(upstream / '?.lua'))} .. ';' .. package.path
-- Preserve the original connection from the vertical stems into the footer.
-- Extend its planting region, leaving space for the native tapering tail.
local grow = require('florilegium-grow')
local original_regions = grow.regions
grow.regions = function(g, layout)
    local rects, col, foot, spill = original_regions(g, layout)
    foot[2], foot[3], foot[4] = 0.55, g.textright - 2.8, 1.6
    rects[2] = foot
    return rects, col, foot, spill, foot[3]
end
local svg = require('florilegium-svg').page({{
seed=2106,page={page},paper='a4',left='48mm',right='12mm',top='17mm',bottom='18mm',
head=false,visitors=false,shells=false,density=1.0,intensity=0.8,
ink='{ACCENT[1:]}',accent='{SECONDARY[1:]}',id='margin{page}'}})
local file=assert(io.open({json.dumps(str(path))},'w'));file:write(svg);file:close()
'''
        execute_lua(script)
        drawing = prepare_ornament(ET.fromstring(path.read_text()))
        placements.append(fit_ornament(path, drawing))
    shutil.copy2(upstream/'LICENSE',output/'FLORILEGIUM_LICENSE.txt')
    (output/'provenance.json').write_text(json.dumps({'repository':'https://github.com/kevinraymond/florilegium','commit':commit,'license':'MIT','seed':2106,'pages':6,'density':1.0,'intensity':0.8,'ink':ACCENT,'accent':SECONDARY,'composition':'connected native L-shaped garden','pdf_background_dpi':300,'placements':placements,'page_safety_clip_removed':True,'horizontal_footer_ornament_omitted':False,'growth_regions':'native column and extended footer','body_margins_mm':{'left':48,'right':12,'top':20,'bottom':30},'runtime':'system liblua5.3 via ctypes'},indent=2)+'\n')


if __name__ == '__main__':
    main()

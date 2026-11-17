"""Bake the authored UV charts using physical part coordinates. Requires NumPy and Pillow."""
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont


SIZE = 2048
PAD = 8
PREFIX = 'T_GarageConcrete_'
OUT = Path(__file__).resolve().parents[2] / 'TunaSweeper/SourceArt/Environment/GarageDoorConcrete'


def quadrants(path):
    rgb = np.asarray(Image.open(path).convert('RGB'), dtype=np.float32)
    h, w = rgb.shape[:2]
    return [rgb[y+6:y+h//2-6, x+6:x+w//2-6] for y in (0, h//2) for x in (0, w//2)]


CAST, MOSS, OCHRE, OXIDE = quadrants(OUT/'SurfaceSources/WeatheringSwatches.png')
_, DARK, GREY, RUBBER = quadrants(OUT/'Textures/T_GarageConcrete_Atlas.png')


def sat(v):
    return np.clip(v, 0, 1)


def smooth(a, b, v):
    t = sat((v-a)/(b-a))
    return t*t*(3-2*t)


def noise(x, y, seed=0):
    """Continuous value noise in surface coordinates."""
    i, j = np.floor(x), np.floor(y)
    fx, fy = x-i, y-j
    fx, fy = fx*fx*(3-2*fx), fy*fy*(3-2*fy)
    def corner(dx, dy):
        # Integer hashes avoid periodic diagonal bands and float32 sine-hash artifacts.
        a = (i.astype(np.int64)+dx)*374761393 + (j.astype(np.int64)+dy)*668265263 + seed*144269
        a = (a ^ (a >> 13))*1274126177
        return ((a ^ (a >> 16)) & 65535).astype(np.float32)/65535
    return (corner(0,0)*(1-fx)+corner(1,0)*fx)*(1-fy) + (corner(0,1)*(1-fx)+corner(1,1)*fx)*fy


def swatch(tile, u, v, cm):
    def mirror(t):
        return 1 - np.abs(t % 2 - 1)
    ix = (mirror(u/cm)*(tile.shape[1]-1)).astype(np.intp)
    iy = (mirror(-v/cm)*(tile.shape[0]-1)).astype(np.intp)
    return tile[iy, ix]


def mix(a, b, mask):
    return a + (b-a)*mask[..., None]


def frame(c, x, y, z):
    ox, oz = {'FrameLeft': (-117.5, 97.5), 'FrameRight': (117.5, 97.5), 'FrameTop': (0, 212.5)}[c['asset']]
    x, z = x+ox, z+oz
    u, v = x+.81*y, z+.51*y
    coarse = noise(u/12, v/12, 3)
    mid = noise(u/3.1, v/3.1, 17)
    fine = noise(u/.74, v/.74, 9)
    mineral = swatch(CAST, u+.32*mid, v, 64)
    grain = mineral.mean(axis=2)
    stone = (mineral*.42 + grain[..., None]*.58) * np.array([.95,.945,.94])
    stone += ((coarse-.5)*12+(mid-.5)*5)[..., None]
    # A damp foot, a leaking lintel joint, and a sheltered top edge. Masks cross
    # bevels and corners because all three frame parts share assembly coordinates.
    left = np.exp(-((x+134)/11)**2)
    right = np.exp(-((x-134)/12)**2)
    foot_height = 5 + 36*left + 12*right + 4*noise(x/9, y/9, 8)
    foot = (foot_height-z)/12
    cap = 1.12 - np.sqrt(((x+133)/37)**2+((z-228)/12)**2)
    joint = .95 - np.sqrt(((x+133)/11)**2+((z-200)/22)**2)
    frontness = 1 - .52*smooth(-10, 20, y)
    wetzone = np.maximum(foot, np.maximum(cap, joint))*frontness
    growth = wetzone + (coarse-.5)*.95 + (mid-.5)*.86 + (fine-.5)*.42
    moss = smooth(.04,.46,growth)
    damp = smooth(-.7,.22,growth)
    rain = smooth(.49,.85,noise(x/2.4+.22*noise(z/15,y/10,5),z/36+y/14,24))
    rain *= np.exp(-np.maximum(230-z,0)/20)*(.4+.6*frontness)
    stone *= (1-.20*damp-.24*rain)[..., None]
    moss_color = swatch(MOSS, u+.7*mid, v-.5*coarse, 30) * np.array([.68,.78,.62])
    moss_color *= (.82+.21*mid)[...,None]
    color = mix(stone, moss_color, moss)
    pores = smooth(.75,.94,noise(u/.29,v/.29,35))*smooth(.4,.75,mid)
    color *= (1-.15*pores)[..., None]
    height = (grain-134)*.00125 + (mid-.5)*.06 - pores*.06
    height += moss*(.096+(moss_color.mean(axis=2)-55)*.001)
    rough = sat(.86+.055*(mid-.5)-.17*damp+.12*moss)
    return color, 1-.13*pores-.05*damp, rough, np.zeros_like(x), height


def metal(c, x, y, z):
    asset, part, kind = c['asset'], c['part'], c['kind']
    row = {'UpperPanel': 0, 'UpperPanel02': 1, 'UpperPanel03': 2, 'UpperPanel04': 3}.get(asset, -1)
    u, v = x+.74*y, z+.45*y+max(0, 3-row)*45
    n = noise(u/3.1,v/2.8,33)
    fine = noise(u/.36,v/.38,65)
    if kind == 'Rubber':
        color = swatch(RUBBER,u,v,33)*.84
        return color, np.ones_like(x), .82+.08*n, np.zeros_like(x), .008*(fine-.5)
    painted = kind == 'Panel'
    base = swatch(GREY if painted else DARK,u,v,72 if painted else 46)
    color = base*(.55 if painted else .43) + (51 if painted else 33)
    rough = (.56 if painted else .43)+.12*(n-.5)
    metallic = np.full_like(x, .035 if painted else .74)
    exterior = (y <= -4.45) if painted and row >= 0 else np.zeros_like(x, dtype=bool)
    stripe_edges = np.minimum(x+80, -51-x)
    stripe = smooth(-.1,.12,stripe_edges+(fine-.5)*.48)
    stripe *= exterior * (1-smooth(.75,.91,fine)*smooth(.28,.77,n)*.87)
    ochre = swatch(OCHRE,u+.23*n,v,52)*np.array([.98,1.00,.92])
    color = mix(color,ochre,stripe)
    lo, hi = c['pmin'], c['pmax']
    # The moisture stays in panel lips, chipped paint and individual fastener trails.
    axes = [abs(a)-1 for a in c['basis'][:2]]
    distances = [np.minimum(abs((x,y,z)[i]-lo[i]),abs((x,y,z)[i]-hi[i])) for i in axes]
    edge = np.minimum(*distances)
    periphery = 1-smooth(.07,.43+(n**2)*.56,edge)
    chipped = periphery*smooth(.43,.80,n*.74+fine*.26)
    if part in ('Panel_structural_skin','Inset_plate'):
        chipped *= np.where(exterior,1,.29)
    else:
        chipped *= .21
    trail = np.zeros_like(x)
    if row in (1,2) and part == 'Inset_plate':
        bolt_x = 0 if row == 1 else 62
        down = 13.9-z
        length = 6.2 if row == 1 else 4.1
        radius = .13+.30*sat(1-down/length)
        trail = (1-smooth(.05,radius+.21,abs(x-bolt_x-.1*np.sin(down))))*smooth(0,.18,down)
        trail *= (1-smooth(.5*length,length,down))*exterior
    scratches = np.zeros_like(x)
    if row == 3 and part in ('Inset_plate','Panel_structural_skin'):
        for idx, (cx,half,level,tilt) in enumerate([
                (-65,27,-10.3,.024),(-70,17,-8.9,.031),(-40,31,-12.4,-.012),
                (3,34,-11.8,-.026),(16,19,-10.6,-.031),(61,27,-9.7,-.018),(70,15,-11.1,-.022)]):
            path = level + tilt*(x-cx) + .17*np.sin(x*.18+idx)
            line = (1-smooth(.07,.32,abs(z-path)))*(1-smooth(.55*half,half,abs(x-cx)))
            line *= smooth(.24,.66,noise(x/3.8,z/2.8,19+idx))
            scratches = np.maximum(scratches,line)
        scratches *= exterior
    bare = sat(chipped+.9*scratches)
    color = mix(color, np.array([84.,81.,74.]), bare)
    metallic = metallic*(1-bare)+.85*bare
    rust = sat((chipped*(.63+.4*n)+scratches*.91)*smooth(.18,.69,n*.72+fine*.28)+.73*trail)
    rust_color = swatch(OXIDE,u,v,22)*np.array([1.12,.95,.84])
    color = mix(color,rust_color,rust)
    rough = sat(rough+stripe*.06+rust*.32)
    height = .007*(fine-.5)+stripe*.019-bare*.023+rust*.026
    return color, 1-.07*chipped, rough, metallic*(1-rust), height


def bake(charts, page):
    maps = {key: Image.new('RGB',(SIZE,SIZE),fill) for key,fill in
            [('BaseColor',(85,85,82)),('Normal',(128,128,255)),('ORM',(255,168,0))]}
    guide = Image.new('RGB',(SIZE,SIZE),(24,29,34))
    draw = ImageDraw.Draw(guide)
    for c in charts:
        x0,y0,w,h,scale = (c[k] for k in ('x','y','w','h','scale'))
        du = c['lo'][0]+(np.arange(w,dtype=np.float32)+.5-PAD)/scale
        dv = c['hi'][1]-(np.arange(h,dtype=np.float32)+.5-PAD)/scale
        # Extend boundary colour into the full gutter for filtered sampling and mips.
        u,v = np.meshgrid(np.clip(du,c['lo'][0],c['hi'][0]),np.clip(dv,c['lo'][1],c['hi'][1]))
        xyz = [None]*3
        for coord,axis in zip((u,v,np.full_like(u,c['depth'])),c['basis']):
            xyz[abs(axis)-1] = coord if axis>0 else -coord
        color,ao,rough,metalness,height = (frame if page=='Frame' else metal)(c,*xyz)
        orm = np.stack([ao,rough,metalness],axis=2)*255
        if c['rotated']:
            color,orm,height = (np.rot90(a,-1) for a in (color,orm,height))
        hy,hx = np.gradient(height*scale)
        tangent = np.stack([-hx,hy,np.ones_like(hx)],axis=2)
        normal = (tangent/np.linalg.norm(tangent,axis=2,keepdims=True)+1)*127.5
        for name,data in [('BaseColor',color),('ORM',orm),('Normal',normal)]:
            maps[name].paste(Image.fromarray(np.uint8(np.clip(data+.5,0,255))), (x0,y0))
        main = c['axis']=='front' and (c['part']=='Inset_plate' or page=='Frame')
        ink = (235,183,88) if main else (115,164,173)
        for a,b in c['outline']:
            draw.line((tuple(a),tuple(b)),fill=ink,width=2 if main else 1)
        if main and w>130 and h>130:
            try:
                font = ImageFont.truetype('arial.ttf',19)
            except OSError:
                font = ImageFont.load_default()
            draw.text((x0+PAD+9,y0+PAD+10),c['asset'],font=font,fill=(232,228,209),
                      stroke_width=2,stroke_fill=(28,34,38))
    for name,image in maps.items():
        image.save(OUT/'Textures'/f'{PREFIX}{page}_{name}.png')
    guide.save(OUT/'Previews'/f'GarageConcrete_{page}UV.png')


if __name__ == '__main__':
    layout = json.loads(Path(sys.argv[1]).read_text(encoding='utf-8'))
    for name in ('Frame','Metal'):
        bake([c for c in layout if c['page']==name], name)
        print('Baked',name,flush=True)

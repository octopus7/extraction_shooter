"""Reusable script-mode domestic prop modeling for the reference-led V2 room."""
import sys,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import common as c
import bpy
from mathutils import Vector
REF='References/Environment_Reference.png'

def cabinet():
    c.begin('SideCabinet',(80,60,85),REF)
    c.box('plinth',(0,0,4),(78,58,8),'Oak',1.2)
    c.box('case',(0,1,44),(75,54,72),'Oak',1)
    c.box('top',(0,0,82.5),(80,60,5),'Oak',1)
    for x in (-35,35):c.box('front_stile',(x,-27,45),(6,4,70),'Oak',.5)
    for z,h in ((23,28),(56,26)):
        c.box('drawer_face',(0,-28,z),(62,4,h),'Oak',.6)
        c.box('raised_panel',(0,-30.2,z),(53,1.8,h-7),'Oak',.5)
        c.cylinder('knob_stem',(0,-32,z),1.1,2,'Brass',axis=(0,1,0),vertices=16,bevel=.1)
        c.sphere('knob',(0,-33.5,z),(4,3,4),'Brass')
    c.collision((0,0,42.5),(80,60,85));c.end()

def lamp():
    c.begin('RoseTableLamp',(38,38,65),REF)
    c.lathe('turned_base',[(0,0),(12,0),(14,1),(14,3),(10,5),(5,7),(0,7)],mat='Brass',segments=40)
    c.lathe('slender_stem',[(0,6),(4,6),(3,11),(2,26),(5,30),(0,30)],mat='Brass',segments=32)
    c.sphere('warm_bulb',(0,0,33),(9,9,13),'Amber')
    c.lathe('fabric_shade',[(19,27),(10,60),(9.5,60),(18.4,27)],mat='Cream',segments=64)
    c.ring('shade_bottom_welt',(0,0,27),18.6,.4,'Rose')
    c.ring('shade_top_welt',(0,0,60),9.7,.35,'Rose')
    c.cylinder('finial_pin',(0,0,61.5),.7,4,'Brass',vertices=16,bevel=.1)
    c.sphere('finial',(0,0,64),(2,2,2),'Brass')
    c.collision((0,0,13),(28,28,26));c.collision((0,0,46),(38,38,38));c.end()

def stool():
    c.begin('SwivelStool',(55,55,65),REF)
    c.lathe('seat_wood_base',[(0,50),(23,50),(25,52),(25,54),(0,54)],mat='Oak',segments=48)
    c.sphere('rose_cushion',(0,0,57.5),(49,49,15),'Rose')
    c.ring('upholstery_piping',(0,0,57),24,.55,'Cream')
    c.lathe('adjustable_spindle',[(0,6),(7,6),(7,12),(3,17),(3,49),(7,50),(0,50)],mat='Brass',segments=32)
    for angle in [0,math.pi/2,math.pi,3*math.pi/2]:
        dx,dy=math.cos(angle),math.sin(angle)
        c.pipe('splayed_leg',[(dx*3,dy*3,14),(dx*15,dy*15,8),(dx*24,dy*24,4)],2,'Iron')
        c.sphere('rounded_foot',(dx*24,dy*24,3.5),(7,7,7),'Iron')
    c.collision((0,0,8),(55,55,16));c.collision((0,0,32),(14,14,40));c.collision((0,0,56.5),(50,50,17));c.end()

def flowerpot():
    c.begin('FlowerPot',(45,45,65),REF)
    c.lathe('clay_pot',[(0,0),(11,0),(16,23),(17,25),(17,28),(14.5,28),(13.5,25),(10,3),(0,3)],mat='Ceramic',segments=40)
    c.cylinder('soil',(0,0,24),13.5,1,'Oak',vertices=32,bevel=0)
    for i in range(9):
        a=i*math.tau/9;dx,dy=math.cos(a),math.sin(a);z=39+(i%3)*6
        c.pipe('plant_stem',[(0,0,24),(dx*7,dy*7,36),(dx*15,dy*15,z)],.6,'Leaf')
        o=c.sphere('oval_leaf',(dx*11,dy*11,z-4),(10,4,20),'Leaf');o.rotation_euler=(.4,dx*.7,a)
        if i%2==0:
            c.sphere('flower_center',(dx*15,dy*15,z+3),(3,3,3),'Amber')
            for j in range(5):
                b=j*math.tau/5
                c.sphere('rose_petal',(dx*15+3*math.cos(b),dy*15+3*math.sin(b),z+3),(5,5,2),'Rose')
    c.collision((0,0,14),(34,34,28));c.collision((0,0,44),(45,45,42));c.end()

def books():
    c.begin('BookSet',(55,30,32),REF)
    for i,(z,w,h,key) in enumerate([(4,55,8,'BookRed'),(13,51,9,'BookGreen'),(23,47,11,'BookRed')]):
        x=(-1)**i*1.5;y=i-1
        c.box('paper_block',(x,y,z),(w-2,27,h-1.2),'Paper',.25)
        for dz in (-h/2,h/2):c.box('cloth_cover',(x,y,z+dz),(w,30,.65),key,.2)
        c.box('book_spine',(x,-14.3,z),(w,1.4,h),key,.3)
        for xx in (-w*.36,w*.36):c.box('spine_gilt_band',(x+xx,-15.1,z),(.8,.15,h*.8),'Brass',0)
    c.collision((0,0,16),(55,30,32));c.end()

def mug():
    c.begin('CeramicMug',(18,14,20),REF)
    c.lathe('cup_open',[(0,0),(6,0),(7,2),(7,18),(6.8,20),(5.8,20),(5.8,2),(0,2)],mat='Ceramic',segments=40)
    c.ring('round_handle',(8,0,11),5,1.3,'Ceramic',axis=(0,1,0),scale=(.8,1,1))
    c.cylinder('tea',(0,0,16),5.7,.3,'Oak',vertices=32,bevel=0)
    c.collision((0,0,10),(18,14,20));c.end()

def curtain():
    c.begin('CurtainFrame',(125,25,135),REF)
    c.cylinder('curtain_rod',(0,0,131),1.5,121,'Brass',axis=(1,0,0),vertices=20,bevel=.2)
    for side in (-1,1):
        c.sphere('rod_finial',(side*61,0,131),(3,4,4),'Brass')
        verts=[];faces=[];cols=10;rows=20
        for j in range(rows+1):
            t=j/rows;z=128*(1-t)
            width=30-15*math.exp(-((t-.55)/.19)**2)
            center=side*(44+8*math.exp(-((t-.55)/.19)**2))
            for i in range(cols+1):
                u=i/cols
                verts.append((center+(u-.5)*width,-3+4*math.cos(u*math.pi*6),z+2*math.cos(u*math.pi*6)*(t**5)))
        for j in range(rows):
            for i in range(cols):
                a=j*(cols+1)+i;faces.append((a,a+1,a+cols+2,a+cols+1))
        cloth=c.surface('folded_gingham_curtain',verts,faces,'Rose',0,True)
        mod=cloth.modifiers.new('cloth thickness','SOLIDIFY');mod.thickness=.004
        bpy.context.view_layer.objects.active=cloth;cloth.select_set(True);bpy.ops.object.modifier_apply(modifier=mod.name);cloth.select_set(False)
        c.box('tie_back',(side*52,-8,59),(18,2,4),'Cream',.5)
        for x in [side*35,side*43,side*52]:c.ring('curtain_ring',(x,0,129),2.1,.35,'Brass',axis=(1,0,0))
        c.collision((side*50,0,65),(25,25,130))
    c.end()

def rug():
    c.begin('FloralRug240',(240,240,1.6),REF)
    m=bpy.data.materials.new('M_MoleControlV2_Rug');m.use_nodes=True
    tex=m.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(c.OUT/'Textures/T_MoleControlV2_Rug.png'))
    tex.image.pack();tex.image.filepath='//Textures/T_MoleControlV2_Rug.png'
    bs=m.node_tree.nodes['Principled BSDF'];bs.inputs['Roughness'].default_value=.95
    m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
    c.extra_material_specs['Rug']={'name':m.name,'metallic':0,'roughness':.95,'emission':0,'texture':'T_MoleControlV2_Rug'}
    o=c.cylinder('rug_disc',(0,0,.8),120,1.6,'Rose',vertices=128,bevel=.2)
    o.data.materials[0]=m
    uv=o.data.uv_layers.active
    # Exact texture circular edge is at 48% image radius; crop surrounding canvas.
    for loop in o.data.loops:
        v=o.data.vertices[loop.vertex_index].co
        uv.data[loop.index].uv=(.5+.48*v.x/1.2,.5+.48*v.y/1.2)
    c.collision((0,0,.8),(240,240,1.6));c.end()

if __name__=='__main__':
    c.init('Decor')
    for build in [cabinet,lamp,stool,flowerpot,books,mug,curtain,rug]:build()
    c.complete()

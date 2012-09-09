# vim:ts=4:et
# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

import bpy
from bpy.props import BoolProperty, FloatProperty, StringProperty, EnumProperty
from bpy.props import BoolVectorProperty, CollectionProperty, PointerProperty
from bpy.props import FloatVectorProperty, IntProperty

from .entityclass import EntityClass

def qfentity_items(self, context):
    qfmap = context.scene.qfmap
    entclasses = qfmap.entity_classes.entity_classes
    eclist = list(entclasses.keys())
    eclist.sort()
    enum = (('', '--', 'No class. Will be exported as part of the world entity.'),)
    enum += tuple(map(lambda ec: (ec, ec, entclasses[ec].comment), eclist))
    return enum

class QFEntityProp(bpy.types.PropertyGroup):
    value = StringProperty(name="")
    template_list_controls = StringProperty(default="value", options={'HIDDEN'})

class QFEntity(bpy.types.PropertyGroup):
    classname = EnumProperty(items = qfentity_items, name = "Entity Class")
    flags = BoolVectorProperty(size=12)
    fields = CollectionProperty(type=QFEntityProp, name="Fields")
    field_idx = IntProperty()

class QFEntpropAdd(bpy.types.Operator):
    '''Add an entity field/value pair'''
    bl_idname = "object.entprop_add"
    bl_label = "Entprop Add"
    def execute(self, context):
        qfentity = context.active_object.qfentity
        item = qfentity.fields.add()
        item.name = ""
        item.value = ""
        return {'FINISHED'}

class QFEntpropRemove(bpy.types.Operator):
    '''Remove an entity field/value pair'''
    bl_idname = "object.entprop_remove"
    bl_label = "Entprop Remove"
    def execute(self, context):
        qfentity = context.active_object.qfentity
        if qfentity.field_idx >= 0:
            qfentity.fields.remove(qfentity.field_idx)
        return {'FINISHED'}

class EntityPanel(bpy.types.Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'
    bl_label = 'QF Entity'

    @classmethod
    def poll(cls, context):
        return True

    def draw(self, context):
        layout = self.layout
        qfentity = context.active_object.qfentity
        qfmap = context.scene.qfmap
        if qfentity.classname:
            ec = qfmap.entity_classes.entity_classes[qfentity.classname]
        else:
            ec = EntityClass.null()
        flags = ec.flagnames + ("",) * (8 - len(ec.flagnames))
        flags += ("!easy", "!medium", "!hard", "!dm")
        row = layout.row()
        row.prop(qfentity, "classname")
        row = layout.row()
        for c in range(3):
            col = row.column()
            sub = col.column(align=True)
            for r in range(4):
                idx = c * 4 + r
                sub.prop(qfentity, "flags", text=flags[idx], index=idx)
        row = layout.row()
        col = row.column()
        col.template_list(qfentity, "fields", qfentity, "field_idx",
                          prop_list="template_list_controls", rows=3)
        col = row.column(align=True)
        col.operator("object.entprop_add", icon='ZOOMIN', text="")
        col.operator("object.entprop_remove", icon='ZOOMOUT', text="")
        if len(qfentity.fields) > qfentity.field_idx >= 0:
            row = layout.row()
            field = qfentity.fields[qfentity.field_idx]
            row.prop(field, "name", text="Field Name")

def default_brush_entity(entityclass):
    name = entityclass.name
    verts = [(-8, -8, -8),
             ( 8,  8, -8),
             (-8,  8,  8),
             ( 8, -8,  8)]
    faces = [(0, 2, 1),
             (0, 3, 2),
             (0, 1, 3),
             (1, 2, 3)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    return mesh

def entity_box(entityclass):
    name = entityclass.name
    size = entityclass.size
    color = entityclass.color
    if name in bpy.data.meshes:
        return bpy.data.meshes[name]
    verts = [(size[0][0], size[0][1], size[0][2]),
             (size[0][0], size[0][1], size[1][2]),
             (size[0][0], size[1][1], size[0][2]),
             (size[0][0], size[1][1], size[1][2]),
             (size[1][0], size[0][1], size[0][2]),
             (size[1][0], size[0][1], size[1][2]),
             (size[1][0], size[1][1], size[0][2]),
             (size[1][0], size[1][1], size[1][2])]
    faces = [(0, 1, 3, 2),
             (0, 2, 6, 4),
             (0, 4, 5, 1),
             (4, 6, 7, 5),
             (6, 2, 3, 7),
             (1, 5, 7, 3)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_raytrace = False
    mesh.materials.append(mat)
    return mesh

def set_entity_props(obj, ent):
    qfe = obj.qfentity
    if "classname" in ent.d:
        qfe.classname = ent.d["classname"]
    if "spawnflags" in ent.d:
        flags = int(float(ent.d["spawnflags"]))
        for i in range(12):
            qfe.flags[i] = (flags & (1 << i)) and True or False
    for key in ent.d.keys():
        if key in {"classname", "spawnflags", "origin"}:
            continue
        item = qfe.fields.add()
        item.name = key
        item.value = ent.d[key]

def add_entity(self, context, entclass):
    entity_class = context.scene.qfmap.entity_classes.entity_classes[entclass]
    context.user_preferences.edit.use_global_undo = False
    for obj in bpy.data.objects:
        obj.select = False
    if entity_class.size:
        mesh = entity_box(entity_class)
    else:
        mesh = default_brush_entity(entity_class)
    obj = bpy.data.objects.new(entity_class.name, mesh)
    obj.location = context.scene.cursor_location
    obj.select = True
    obj.qfentity.classname = entclass
    context.scene.objects.link(obj)
    bpy.context.scene.objects.active=obj
    context.user_preferences.edit.use_global_undo = True
    return {'FINISHED'}

def register():
    bpy.types.Object.qfentity = PointerProperty(type=QFEntity)
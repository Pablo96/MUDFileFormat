import bpy
from mathutils import Matrix
from math import radians
#############################################################
#                   STRUCTURES                              #
#############################################################

class Tag:
    def __init__(self, name, attributes, values):
        self.name = name
        self.attributes = attributes
        self.values = values
        self.children = []

class BoneTag(Tag):
    def __init__(self, id, name, translation, rotation):
        super().__init__('bone', ['id', 'name', 'translation', 'rotation'], [id, name, translation, rotation])

class PropTag(Tag):
    def __init__(self, name, value):
        super().__init__(name, ['value'], [value])
        
class NamedTag(Tag):
    def __init__(self, name, nameValue):
        super().__init__(name, ['name'], [nameValue])


class SimpleTag(Tag):
    def __init__(self, name):
        super().__init__(name, [], [])




#############################################################
#                   FUNCTIONS                               #
#############################################################

def writeTag(file, tag, level):
    for i in range(level):
        file.write("\t")
    file.write('<' + tag.name)
    
    for attribute, value in zip(tag.attributes, tag.values):
        file.write(' ' + attribute + '="' + value +'"')

    file.write(">\n")
        
    
    for child in tag.children:
        writeTag(file, child, level + 1)
            
    for i in range(level):
        file.write("\t")
    file.write("</" + tag.name + ">\n")
    return
    
def buildMesh(obj, rot_matrix):
    mesh = obj.data
    # Triangulate mesh faces
    mesh.calc_loop_triangles()
    # To access faces use "mesh.loop_triangles"
    
    meshNode = NamedTag('mesh', mesh.name)
    meshNode.attributes.append('vertexcount')
    meshNode.values.append(str(len(mesh.vertices)))
    
    for vertex in mesh.vertices:
        vertexNode = Tag('vertex', ['id'], [str(vertex.index)])
        
        #position
        vertPos = rot_matrix @ vertex.co
        value = str(vertPos.x) + ' ' + str(vertPos.y) + ' ' + str(vertPos.z)
        position  = PropTag('position', value)
        vertexNode.children.append(position)
        
        #normal
        vertNormal = rot_matrix @ vertex.normal
        value = str(vertNormal.x) + ' ' + str(vertNormal.y) + ' ' + str(vertNormal.z)
        normal  = PropTag('normal', value)
        vertexNode.children.append(normal)
        
        #bones indices and weight
        if (len(vertex.groups) > 0 ):
            groups = iter(vertex.groups)
            firstG = next(groups)
            
            value = str(firstG.group)
            wValues = str(firstG.weight)
            
            for group in groups:
                value += ' '
                value += str(group.group)
                wValues += ' '
                wValues += str(group.weight)
            
            indices = PropTag('indices', value)
            weights = PropTag('weights', wValues)
            
            vertexNode.children.append(indices)
            vertexNode.children.append(weights)
            
        #add vertex to mesh node
        meshNode.children.append(vertexNode)
    
    # mesh indices
    indicesNode = Tag('indices', ['count', 'values'],\
                  [str(len(mesh.loop_triangles) * 3), ""])
    values = ""
    
    for triangle in mesh.loop_triangles:
        for index in triangle.vertices:
            values += ' ' + str(index)
    values = values[1:]
    
    indicesNode.values[1] = values
    meshNode.children.append(indicesNode)

    return meshNode

def getRootBone(armature_obj):
    for bone in armature_obj.bones:
        if not bone.parent:
            return bone
    return None

def getBoneID(armature_obj, bone):
    return armature_obj.find(bone.name)

def buildSkeletonRecursive(parentNode, bone, armature_obj):
    id = str(getBoneID(armature_obj, bone))
    matrix = bone.matrix_local
    loc_vec = matrix.col[3]
    rot_quat = matrix.to_3x3().to_quaternion()
    translation = str(loc_vec.x) + ", " + str(loc_vec.y) + ", " + str(loc_vec.z)
    rotation = str(rot_quat.x) + ", " + str(rot_quat.y) + ", " + str(rot_quat.z) + ", " + str(rot_quat.w)
    bone_node = BoneTag(id, bone.name, translation, rotation)
    
    for child in bone.children:
        bone_node.children.append(buildSkeletonRecursive(bone_node, child, armature_obj))

    return bone_node

#TODO: Apply axis rotation to armature!!
#TODO: Fix bad export Z up-axis
def buildSkeleton(obj, rot_matrix):
#    obj.transform(rot_matrix.to_4x4())
    rootBone = getRootBone(obj)
    id = str(getBoneID(obj.bones, rootBone))
    matrix = rootBone.matrix_local
    loc_vec = matrix.col[3]
    rot_quat = matrix.to_3x3().to_quaternion()
    translation = str(loc_vec.x) + ", " + str(loc_vec.y) + ", " + str(loc_vec.z)
    rotation = str(rot_quat.x) + ", " + str(rot_quat.y) + ", " + str(rot_quat.z) + ", " + str(rot_quat.w)
    root_bone_node = BoneTag(id, rootBone.name, translation, rotation)

    for child in rootBone.children:
        root_bone_node.children.append(buildSkeletonRecursive(root_bone_node, child, obj.bones))
    
    skeleton_node = NamedTag('skeleton', obj.name)
    skeleton_node.children.append(root_bone_node)
    return skeleton_node


def buildTree(modelName, selected_only, axis_up):
    rootNode = NamedTag('model', modelName)

    rot_matrix = Matrix.Identity(3)
    if (axis_up == 'OPT_Y'):
        rot_matrix = Matrix.Rotation(radians(-90), 3, 'X')
    elif (axis_up == 'OPT_X'):
        rot_matrix = Matrix.Rotation(radians(90), 3, 'Y')
    
    if (selected_only == True):
        obj = bpy.context.object
        rootNode.children.append(buildMesh(obj, rot_matrix))
    else:
        scene = bpy.context.scene
        for obj in scene.objects:
            if obj.type == "ARMATURE":
                rootNode.children.append(buildSkeleton(obj.data, rot_matrix))
            elif obj.type == "MESH":
                rootNode.children.append(buildMesh(obj, rot_matrix))
    
    return rootNode

def write_some_data(context, filepath, selected_only, axis_up):
    print("writing file...")
    f = open(filepath, 'w', encoding='utf-8')
    
    i = filepath.rfind('\\')
    l = filepath.rfind('.')
    modelName = filepath[i+1:l]
    
    tree = buildTree(modelName, selected_only, axis_up)
    writeTag(f, tree, 0)
    
    f.close()
    print("model exported successfuly...")
    return {'FINISHED'}


##############################################################
#               BLENDER FUNCTIONS
##############################################################

# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportSomeData(Operator, ExportHelper):
    """Export the selected model and armature to MUD Renderer format"""
    bl_idname = "export_mud_model.some_data"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export MUD model and skeleton"

    # ExportHelper mixin class uses this
    filename_ext = ".mudm"

    filter_glob: StringProperty(
        default="*.mudm",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    use_setting: BoolProperty(
        name="Selected Only",
        description="Export only selected objects",
        default=False,
    )

    axis_up: EnumProperty(
        name="Up Axis",
        description="Choose wich axis is the up direction",
        items=(
            ('OPT_Z', "Z AXIS", "Z Axis as up vector"),
            ('OPT_Y', "Y AXIS", "Y Axis as up vector"),
            ('OPT_X', "X AXIS", "X Axis as up vector"),
        ),
        default='OPT_Z',
    )

    def execute(self, context):
        return write_some_data(context, self.filepath, self.use_setting, self.axis_up)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="Text Export Operator")


def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.export_mud_model.some_data('INVOKE_DEFAULT')

import bpy
import bmesh
from mathutils import Matrix, Vector
from math import radians
#############################################################
#                   STRUCTURES                              #
#############################################################

class Tag:
    def __init__(self, name, attributes , values):
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
    
#TODO: Export UV map
def buildMesh(obj, rot_matrix):
    mesh = obj.data
    # Set split edges from uv seam
    if not bpy.ops.uv.seams_from_islands.poll():
        if not mesh.is_editmode:
            bpy.ops.object.editmode_toggle()
        bpy.ops.uv.seams_from_islands(mark_seams=True, mark_sharp=True)
    
    if mesh.is_editmode:
        bpy.ops.object.editmode_toggle()
    
    # Split edges so one vertex per uv coord
    bMesh = bmesh.new()
    bMesh.from_mesh(mesh)
    sharpEdges = [edge for edge in bMesh.edges if not edge.smooth]
    bmesh.ops.split_edges(bMesh, edges = sharpEdges)
    bMesh.to_mesh(mesh)
    bMesh.free()
    
    # Triangulate mesh faces
    mesh.calc_loop_triangles()
    
    meshNode = NamedTag('mesh', mesh.name)
    meshNode.attributes.append('vertexcount')
    meshNode.values.append(str(len(mesh.vertices)))
    
    
    # Get Vertices
    for vertex in mesh.vertices:
        index = vertex.index
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
   
    # Add UVs to the vertices only if it has to
    if len(mesh.uv_layers):
        # UV coords support only 1 uv_map (the active one)
        uv_layer = mesh.uv_layers.active
        if not uv_layer:
           print("No uv layer active\n")
        
        # UV coords 
        uvValues = {}

        for tri in mesh.loop_triangles:
            for loop_index in tri.loops:
                uvCoord = uv_layer.data[loop_index].uv
                # Duplicate vector and freeze it
                uvCoord = Vector(uvCoord).freeze()
                #Vector to tuple
                uvCoord = uvCoord[:]
                vertexIndex = mesh.loops[loop_index].vertex_index
                uvValues[vertexIndex] = uvCoord
                
        # remove duplicated vectors
        #uvValues = set(uvValues)
        # check uvCoord count and vertex count
        if len(uvValues.keys()) != int(meshNode.values[1]):
            print("No UVs")

        for key, uv in uvValues.items():
            uvString = str(uv[0]) + " " + str(uv[1])
            vertUV = PropTag('uvcoord', uvString)
            vertex = meshNode.children[key]
            vertex.children.append(vertUV)

    else:
        print("WARNING: model has no UVs\n")    
    
    ###########################################################
    # Indices
    # Node
    indicesNode = Tag('indices', ['count', 'values'],\
                  [str(len(mesh.loop_triangles) * 3)])
    indicesValues = ""
    
    for triangle in mesh.loop_triangles:
        for index in triangle.vertices:
            indicesValues += ' ' + str(index)
    
    # Cut the first space
    indicesNode.values.append(indicesValues[1:])
    meshNode.children.append(indicesNode)
    
    ###########################################################
    # AABB
    aabbNode = Tag('aabb', ['max_extent', 'min_extent'], [])
    # 8x3 objects between -inf, inf
    bound_box = [Vector(corner) for corner in obj.bound_box]
    
    max_x = max_y = max_z = 0.0
    min_x = min_y = min_z = 0.0
    
    for i in range(8):
        if bound_box[i].x > max_x:
            max_x = bound_box[i].x
        elif bound_box[i].x < min_x:
            min_x = bound_box[i].x
        
        if bound_box[i].y > max_y:
            max_y = bound_box[i].y
        elif bound_box[i].y < min_y:
            min_y = bound_box[i].y
        
        if bound_box[i].z > max_z:
            max_z = bound_box[i].z
        elif bound_box[i].z < min_z:
            min_z = bound_box[i].z
    
    
    maxExtent = str(max_x) + " " +str(max_y) + " " + str(max_z)
    minExtent = str(min_x) + " " +str(min_y) + " " + str(min_z)
    
    aabbNode.values.append(maxExtent)
    aabbNode.values.append(minExtent)
    
    meshNode.children.append(aabbNode)
    
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
#TODO: Fix bad export with Z up-axis
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


def buildTree(context, modelName, selected_only, axis_up):
    rootNode = NamedTag('model', modelName)

    rot_matrix = Matrix.Identity(3)
    if (axis_up == 'OPT_Y'):
        rot_matrix = Matrix.Rotation(radians(-90), 3, 'X')
    elif (axis_up == 'OPT_X'):
        rot_matrix = Matrix.Rotation(radians(90), 3, 'Y')
    
    if (selected_only == True):
        objs = bpy.context.selected_objects
        for obj in objs:
            if obj.type == "ARMATURE":
                rootNode.children.append(buildSkeleton(obj.data, rot_matrix))
            elif obj.type == "MESH":
                rootNode.children.append(buildMesh(obj, rot_matrix))
    else:
        scene = context.scene
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
    
    tree = buildTree(context, modelName, selected_only, axis_up)
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
    bl_info = {"blender": (2, 80, 0)}

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
        default='OPT_Y',
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

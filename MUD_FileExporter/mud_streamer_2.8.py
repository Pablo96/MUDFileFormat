import time
import socket
import select
import threading
import bpy
from mathutils import Matrix


### Main thread change 'scene_changed' to True then back thread stream scene to
### MUD viewer using a TCP socket and change 'scene_changed' to False

#-------------------------------------- Globals -------------------------------------------------

scene_changed = False

#-------------------------------------- Data Structures ------------------------------------------
class Vertex:
    def __init__(self):
        self.x = self.y = self.z = 0.0
        self.nx = self.ny = self.nz = 0.0
        self.uv_x = self.uv_y = 0.0


class Mesh:
    TRIANGLES = 0
    LINES = 1
    
    def __init__(self):
        self.vertices = []
        self.indices = []
        self.mode = TRIANGLES
    
    def SetDrawMode(self, in_mode):
        self.mode = in_mode
    
    def AddVertex(self, in_vertex):
        self.vertices.append(in_vertex)


class Model:
    def __init__(self):
        self.meshes = []
        self.transform = Matrix(1.0)
    
    def SetTransform(self, in_transform):
        self.transform = in_transform
    
    def AddMesh(self, in_mesh):
        self.meshes.append(in_mesh)    


#---------------------------- Thread ------------------------------
class Streamer(threading.Thread):
    IP = "127.0.0.1"
    PORT = 56080
    initialized = False
    server_socket = None
    sockets_list = []
    
    def __init__(self):
        threading.Thread.__init__(self)
        if Streamer.initialized == False:
            # Create socket
            Streamer.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Bind address to socket
            Streamer.server_socket.bind((Streamer.IP, Streamer.PORT))
            # start socket
            Streamer.server_socket.listen()
            # add server to the list
            Streamer.sockets_list.append(Streamer.server_socket)
            Streamer.initialized = True
    
    def run(self):    
        for i in range(10):
            print(f"Connection attempt {i+1}...")
            # if one of the sockets of sockets_list have been notified to read
            # it will appear in the read list. Server will be notified when connection occours
            read_sockets, _, _= select.select(Streamer.sockets_list, [], [], 1)
            for notified_socket in read_sockets:
                if notified_socket == Streamer.server_socket:
                    client_socket, address = Streamer.server_socket.accept()
                    print(f"Connection with {address[0]}:{address[1]} set")
                    sockets_list.append(client_socket)
                    connected(client_soclet)
                else:
                    continue
        print("Couldn't establish any connection")
        # Do not foget to close the socket when done
        server_socket.close()



def connected(client_socket):
    while client_socket in sockets_list:
        # listen to 3dView events
        if scene_changed:
            stream_scene(client_socket) 
        scene_changed = False
        continue

def stream_scene(client_socket):
    for window in bpy.context.window_manager.windows:
        if window.screen is not None:
            print("screen exists")
            for area in window.screen.areas:
                space = area.spaces[0]
                if space.type == 'VIEW_3D':
                    print(space.region_3d.view_distance)
                    print("Scene streamed")
                    return
    print("Scene not streamed")


def my_handler(scene):
    scene_changed = True


#To stop the calls at the scene_update_post event level
class StopCallback(bpy.types.Operator):
    bl_idname = "scene.mud_disconnect"
    bl_label = "Disconnect"

    @classmethod
    def poll(cls, context):
        return my_handler in bpy.app.handlers.depsgraph_update_post

    def execute(self, context):
        bpy.app.handlers.depsgraph_update_post.remove(my_handler)
        return {'FINISHED'}

#To start the calls at the scene_update_post event level
class StartCallback(bpy.types.Operator):
    bl_idname = "scene.mud_connect"
    bl_label = "Connect"

    @classmethod
    def poll(cls, context):
        return my_handler not in bpy.app.handlers.depsgraph_update_post

    def execute(self, context):
        bpy.app.handlers.depsgraph_update_post.append(my_handler)
        return {'FINISHED'}

#The panel is located in the scene properties
class SceneEventsPanel(bpy.types.Panel):
    bl_label = "MUD scene viewer"
    bl_idname = "OBJECT_PT_scene_events"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.operator("scene.mud_connect")
        row = layout.row()
        row.operator("scene.mud_disconnect")


def register():
    bpy.utils.register_class(SceneEventsPanel)
    bpy.utils.register_class(StartCallback)
    bpy.utils.register_class(StopCallback)

def unregister():
    bpy.utils.unregister_class(SceneEventsPanel)
    bpy.utils.unregister_class(StartCallback)
    bpy.utils.unregister_class(StopCallback)



streamer = Streamer()

if __name__ == "__main__":
    register()
    streamer.start()
<!-- TYPES || except for SKYBOX and TERRAIN you can set a mix of them
- Example: LIT | TEXTURED | SHADOWED = 11
- UNLIT    = 0
- LIT      = 1
- TEXTURED = 2
- ALPHA    = 4
- SHADOWED = 8
- TERRAIN  = 16
- SKYBOX   = 32
-->
<!-- material tag :: REQUIRED -->
<material type="1">
    <!-- diffuse_color :: OPTIONAL -->
    <!-- if not defined default value "1, 1, 1, 1" -->
    <diffuse_color value="1.0, 0.4, 0.5 1.0"/>

    <!-- diffuse_tex :: OPTIONAL -->
    <!-- if not defined material will not be diffuse textured -->
    <!-- textured name should be a *.mudt texture name in the textures folder -->
    <diffuse_tex   name="Grass_Ground"/>

    <!-- diffuse_cubemap :: OPTIONAL -->
    <!-- if defined material is a skybox -->
    <!-- TODO: define cubemap file -->
    <diffuse_cubemap>
        <axis_pos_x url="../resources/texture/cubemaps/noon_sky/axis_right.png"/>
        <axis_neg_x url="../resources/texture/cubemaps/noon_sky/axis_left.png"/>
        <axis_pos_y url="../resources/texture/cubemaps/noon_sky/axis_top.png"/>
        <axis_neg_y url="../resources/texture/cubemaps/noon_sky/axis_bottom.png"/>
        <axis_pos_z url="../resources/texture/cubemaps/noon_sky/axis_front.png"/>
        <axis_neg_z url="../resources/texture/cubemaps/noon_sky/axis_back.png"/>
    </diffuse_cubemap>

    <!-- cast_shadows :: OPTIONAL -->
    <!-- default value "false" -->
    <cast_shadows value="true"/>
</material>
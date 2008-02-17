#
# Course configuration
#
tux_course_dim 350 1000        ;# width, length of course in m
tux_start_pt 170 1             ;# start position, measured from left rear corner
tux_angle 27                   ;# angle of course
tux_elev_scale 7.0             ;# amount by which to scale elevation data
                               ;#     offset of 0 (integer from 0 - 255)
tux_elev elev.png              ;# bitmap specifying course elevations
tux_terrain terrain.png        ;# bitmap specifying terrains type

tux_course_init

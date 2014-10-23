import knossos_python_api
from knossos_python_api import *

# render a simple line wich goes throw the whole viewport and recenter to that position

f1 = internal.FloatCoordinate(1000-128, 1000, 1000)
f2 = internal.FloatCoordinate(1128, 1000, 1000)
c1 = internal.Color4F(1, 0, 0, 1)

m = internal.Mesh(GL_LINES)
m.set_vertices([f1, f2])
m.set_colors([c1, c1])
m.set_size(30)

knossos.render_mesh(m)
knossos.move_to(1000, 1000, 1000)


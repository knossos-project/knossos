import numpy as np
import Image
import math

#
#   Simple cubing of TIFF stacks.
#

filename = "brain-srhodamine-231110.tif"

stack = Image.open(filename)

xpx = math.ceil(stack.size[0] / 128.) * 128.
ypx = math.ceil(stack.size[1] / 128.) * 128.

print("Will create (%s, %s) output" % (xpx, ypx))

xdc = xpx / 128
ydc = ypx / 128

#single = [[0 for col in range(xpx)] for row in range(ypx)]
single = np.zeros((ypx, xpx), np.dtype('b'))

curzpx = 0.

while True:
    try:
        stack.seek(curzpx)
        curframe = np.reshape(list(stack.getdata()), (stack.size[1], stack.size[0]))

        single[0:stack.size[1], 0:stack.size[0]] = curframe
    except EOFError:
        if curzpx % 128 != 0:
            single = np.zeros((ypx, xpx), np.dtype('b'))
            print("(%d, %d)" %(xpx, ypx))
        else:
            break

    for curxdc in range(0, xdc):
        for curydc in range(0, ydc):
            curzdc = math.floor(curzpx / 128.)

            curcubename = '%s_mag1_x%04d_y%04d_z%04d.raw' % (filename, curxdc, curydc, curzdc)
            fp = open(curcubename, 'ab')
            
            cubeslice = single[curydc * 128:curydc * 128 + 128,
                               curxdc * 128:curxdc * 128 + 128]

            if len(cubeslice.tostring()) == 0:
                print("xdc = %d ydc = %d zdc = %d" % (curxdc, curydc, curzdc))
            
            fp.write(cubeslice.tostring())

            fp.close()

    curzpx = curzpx + 1

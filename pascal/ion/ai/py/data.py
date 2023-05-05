import json
import numpy as np
import util
import args

a = args.get()

def load_index(f):
    try:
        with open(f) as f:
            return json.load(f)
    except:
        print('no index file found (data path must be valid)')
        exit(1)
    return null

def load_dataset(index, dataset):
    path     = a.data_path + '/' + a.model + '/' + dataset
    x        = []
    y        = np.fromfile(path + '/labels.f32', dtype='float32')
    count    = 0
    output   = index['output'] # 1 # {"inputs":[{"image0.u8":[24,32,1]}],"output":1,"validation":["eval","fatigue","lookdown"]}
    inputs   = index['inputs'] #   # [{"image0.u8":[24,32,1]}]
    for i in inputs:
        shape, f    = util.first_pair(i)
        f32         = f.endswith('.f32')
        data        = np.fromfile(path + '/' + f, dtype='float32' if f32 else 'uint8')
        dfloats     = data.astype('float32')
        if count == 0:
            count   = len(dfloats)
        else:
            util.assertion(count == len(dfloats), 'training data vector length mismatch')
        if not f32:
            dfloats = dfloats / 255.0
        size        = 1
        for s in shape:
            size   *= s
        lshape = len(shape)
        if lshape == 3:
            xx      = dfloats.reshape((count, shape[0], shape[1], shape[2]))
        elif lshape == 1:
            xx      = dfloats.reshape((count, shape[0]))
        else:
            assert(false)
        x.append(xx)
    util.assertion(len(x) > 0 and len(y) > 0, 'no data')
    y = y.reshape((-1, output))
    return x, y, output

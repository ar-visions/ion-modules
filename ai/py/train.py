from tensorflow.keras.callbacks import ModelCheckpoint

import util
import args
import data
import model

a              = args.get()
index          = data.load_index(a.path + '/' + a.model + '/index.json')
x, y, output   = data.load_dataset(index, 'train')
model, inputs  = model.create(x, output)
x_vsets        = []
y_vsets        = []
for vset in index['vsets']:
    xv, yv, _  = data.load_dataset(index, vset)
    x_vsets.append(xv)
    y_vsets.append(yv)
cp_name        = './checkpoints/' + index['name'] + '-{loss:.4f}-{epoch:02d}.hdf5'
cp_callback    = ModelCheckpoint(cp_name, monitor='loss', verbose=1, save_best_only=False, mode='max')

print('training sample count: ', len(x))

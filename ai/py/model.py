import random
import numpy      as np
import tensorflow as tf
from tensorflow import keras

from tensorflow.keras.models       import Model
from tensorflow.keras.layers       import Input
from tensorflow.keras.layers       import Dense
from tensorflow.keras.layers       import Flatten
from tensorflow.keras.layers       import Reshape
from tensorflow.keras.layers       import Concatenate
from tensorflow.keras.layers       import Conv2D
from tensorflow.keras.layers       import Conv2DTranspose
from tensorflow.keras.layers       import DepthwiseConv2D
from tensorflow.keras.layers       import BatchNormalization
from tensorflow.keras.layers       import LayerNormalization
from tensorflow.keras.layers       import Dropout
from tensorflow.keras.layers       import MaxPooling2D
from tensorflow.keras.layers       import AveragePooling2D
from tensorflow.keras.layers       import Softmax
from tensorflow.keras.optimizers   import Adam
from tensorflow.keras.optimizers   import SGD

import util
import args

a = args.get()

if not a.gpu:
    os.environ['CUDA_VISIBLE_DEVICES'] = '-1'

if a.seed:
    os.environ['PYTHONHASHSEED']=str(a.seed)
    os.environ['TF_CUDNN_DETERMINISTIC'] = '1'
    random.seed(a.seed)
    np.random.seed(a.seed)
    tf.random.set_seed(a.seed)

def create_conv(filters, fs):
    return Conv2D(filters=filters, strides=(cx,cy), kernel_size=fs, padding='same', activation='relu')
    
def create(x, output):
    imap      = { }
    minputs   = [ ]
    nodes     = [ ]
    for xi in len(x):
        shape      = x[xi].shape[1:]
        name       = 'i' + str(xi + 1)
        i          = Input(shape=shape, name=name)
        imap[name] = x[xi]
        if len(shape) == 3:
            pool = i
            for filters in convs:
                conv = create_conv(filters, fs)(pool)
                if a.batch_norm > 0:
                    conv = BatchNormalization()(conv)
                if a.reduction > 1 and a.pooling == 'max':
                    pool = MaxPooling2D(pool_size=(a.reduction,a.reduction), strides=(a.reduction,a.reduction), padding='same')(conv)
                elif a.reduction > 1 and a.pooling == 'avg':
                    pool = AveragePooling2D(pool_size=(a.reduction,a.reduction), strides=(a.reduction,a.reduction), padding='same')(conv)
                else:
                    pool = conv
            nodes.append(pool)
        else:
            util.assertion(len(shape) == 1, 'unhandled data shape')
            nodes.append(i)
        minputs.append(i)
    util.assertion(len(nodes) == 1, 'unsupported network')
    drop = nodes[0]
    for u in a.dense:
        hidden = Dense(units=u, activation='relu')(drop)
        drop   = Dropout(dropout)(hidden) if dropout > 0.0 else hidden
    output     = Dense(units=output_count, name='output' if not a.softmax else 'd_output')(drop)
    if a.softmax:
        output = Softmax(name='output')(output)
    model      = Model(inputs=minputs, outputs=output)
    return model, imap

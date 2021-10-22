import argparse

p = argparse.ArgumentParser()
p.add_argument('--seed',           type=int,       default=22,          help='seed')
p.add_argument('--gpu',            type=int,       default=0,           help='gpu')
p.add_argument('--path',           type=str,       default='./models',  help='path to data')
p.add_argument('--model',          type=str,       default='test',      help='name of model (must exist in data path)')
p.add_argument('--epochs',         type=int,       default=100,         help='number of epochs')
p.add_argument('--convs',          type=str,       default='null',      help='filter counts separated by comma')
p.add_argument('--csx',            type=int,       default=1,           help='convolution step-x')
p.add_argument('--csy',            type=int,       default=1,           help='convolution step-y')
p.add_argument('--fs',             type=str,       default='3,3',       help='filter size 3,3')
p.add_argument('--dense',          type=str,       default='240,16',    help='dense layer units')
p.add_argument('--dropout',        type=float,     default=0.0,         help='dropout amount')
p.add_argument('--cp',             type=str,       default='',          help='resume from checkpoint')
p.add_argument('--export',         type=str,       default='',          help='export to tflite model')
p.add_argument('--opt',            type=str,       default='Adam',      help='Optimizer selection (Adam, Adagrad, SGD)')
p.add_argument('--momentum',       type=float,     default=0.99,        help='Momentum value for SGD')
p.add_argument('--nesterov',       type=int,       default=1,           help='Enable nesterov momentum (true) with SGD')
p.add_argument('--lr',             type=float,     default=0.0001,      help='learning rate')
p.add_argument('--batch_size',     type=int,       default=32,          help='batch size per iteration')
p.add_argument('--reduction',      type=int,       default=2,           help='pooling reduction algorithm')
p.add_argument('--rtype',          type=str,       default='max',       help='reduction type')
p.add_argument('--batch_norm',     type=int,       default=0,           help='batch normalization')
p.add_argument('--layer_norm',     type=int,       default=0,           help='layer normalization')
p.add_argument('--softmax',        type=int,       default=0,           help='enable softmax')
p.add_argument('--tf_logging',     type=str,       default='warning',   help='tensorflow logging level')
a = p.parse_args()

a.convs = [int(i) for i in a.convs.split(',')]  if a.convs != "null" else []
a.dense = [int(i) for i in a.dense.split(',')]  if a.dense != "null" else []
a.fs    = [int(i) for i in a.fs.split(',')]     if a.fs    != "null" else []

def get():
    return a

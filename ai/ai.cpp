#include <ai/ai.hpp>
///
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
///
/// internal struct contains tflite resources
struct AInternal {
    std::unique_ptr<tflite::FlatBufferModel> mdl;
    std::unique_ptr<tflite::Interpreter>     itr;
};
///
/// construct AI model from path
AI::AI(path_t p) {
    i = new AInternal { };
    ///
    /// load model and construct interpreter with tensor inputs
    i->mdl = tflite::FlatBufferModel::BuildFromFile(p.string().c_str());
    assert(i->mdl != nullptr);
    tflite::ops::builtin::BuiltinOpResolver res;
    tflite::InterpreterBuilder builder(*i->mdl, res);
    builder(&i->itr);
    assert(i->itr != nullptr && i->itr->AllocateTensors() == kTfLiteOk);
}
///
/// forward pass data
vec<float> AI::operator()(vec<var> in) {
    assert(in.size() > 0);
    assert(equals(in[0].t, { var::f32, var::ui8, var::i8 }));
    auto     itr = i->itr.get();
    ///
    /// copy inputs, tensorflow lite performs data conversions from float to int or vice versa, if needed.
    for (size_t d = 0; d < in.size(); d++) {
        var  &dd = in[d];
        if (dd.t == var::f32)
            memcopy(dd.data<float>(),   itr->typed_input_tensor<float>  (d), dd.size());
        else
            memcopy(dd.data<uint8_t>(), itr->typed_input_tensor<uint8_t>(d), dd.size());
    }
    ///
    /// run model, return output values as a vector of floats
    assert(itr->Invoke() == kTfLiteOk);
    auto   ot = itr->output_tensor(0);
    assert(ot->dims->size == 1);
    size_t sz = ot->dims->data[0];
    float*  o = itr->typed_output_tensor<float>(0);
    return vec<float>(o, sz);
}


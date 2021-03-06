/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "libyuv/convert_argb.h"
#include "libyuv/convert_from_argb.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow_io/core/kernels/io_stream.h"

namespace tensorflow {
namespace io {
namespace {

class DecodeYUY2Op : public OpKernel {
 public:
  explicit DecodeYUY2Op(OpKernelConstruction* context) : OpKernel(context) {
    env_ = context->env();
  }

  void Compute(OpKernelContext* context) override {
    const Tensor* input_tensor;
    OP_REQUIRES_OK(context, context->input("input", &input_tensor));

    const Tensor* size_tensor;
    OP_REQUIRES_OK(context, context->input("size", &size_tensor));

    const tstring& input = input_tensor->scalar<tstring>()();

    int64 channels = 3;
    int64 height = size_tensor->flat<int32>()(0);
    int64 width = size_tensor->flat<int32>()(1);

    Tensor* image_tensor = nullptr;
    OP_REQUIRES_OK(
        context, context->allocate_output(
                     0, TensorShape({height, width, channels}), &image_tensor));

    string buffer;
    buffer.resize(width * height * 4);
    uint8* argb = (uint8*)&buffer[0];
    uint8* yuy2 = (uint8*)&input[0];
    uint32 yuy2_stride = width * 2;
    uint32 argb_stride = width * 4;
    int status =
        libyuv::YUY2ToARGB(yuy2, yuy2_stride, argb, argb_stride, width, height);
    OP_REQUIRES(
        context, (status == 0),
        errors::InvalidArgument("unable to convert yuy2 to argb: ", status));

    uint8* rgb = image_tensor->flat<uint8>().data();
    uint32 rgb_stride = width * 3;
    status =
        libyuv::ARGBToRAW(argb, argb_stride, rgb, rgb_stride, width, height);
    OP_REQUIRES(
        context, (status == 0),
        errors::InvalidArgument("unable to convert argb to rgb: ", status));
  }

 private:
  mutex mu_;
  Env* env_ TF_GUARDED_BY(mu_);
};
REGISTER_KERNEL_BUILDER(Name("IO>DecodeYUY2").Device(DEVICE_CPU), DecodeYUY2Op);

}  // namespace
}  // namespace io
}  // namespace tensorflow

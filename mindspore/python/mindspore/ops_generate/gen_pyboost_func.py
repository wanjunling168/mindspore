# Copyright 2023 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================
"""
Generate pyboost function from pyboost_op.yaml
"""

import os
import pathlib
from dataclasses import dataclass
import pyboost_utils
from pyboost_utils import get_convert_type_str, get_input_dtype, get_return_type, tuple_input_to_cpp_type, \
    number_input_to_cpp_type, get_const_number_convert, get_tuple_input_convert, get_pyboost_name, is_cube, \
    get_aclnn_interface, get_disable_flag, get_op_name, is_optional_param
import template
from template import CppTemplate
from op_proto import OpProto
from gen_utils import check_change_and_replace_file, py_licence_str


@dataclass
class FuncHeaderData:
    work_path: str
    op_header_template_path: list
    code_generate_path: list
    op_name_str: str
    operator_name: str
    call_args_with_type: list
    cpp_func_return: str


def generate_pyboost_base_op_header_code(work_path, op_name_str, operator_name, call_args_with_type, cpp_func_return):
    """ generate_pyboost_base_op_header_code """
    pyboost_op_header_str = template.PYBOOST_BASE_OP_DEFINE_TEMPLATE.replace(op_name=op_name_str,
                                                                             op_name_upper=op_name_str.upper(),
                                                                             call_args=call_args_with_type,
                                                                             return_type=cpp_func_return)
    op_header_dir_path = os.path.join(work_path, "mindspore/ccsrc/kernel/pyboost/auto_generate/")
    pathlib.Path(op_header_dir_path).mkdir(parents=True, exist_ok=True)
    tmp_op_file_path = os.path.join(op_header_dir_path, "tmp_" + operator_name + ".h")
    dst_op_file_path = os.path.join(op_header_dir_path, operator_name + ".h")
    with open(tmp_op_file_path, "w") as f:
        f.write(pyboost_op_header_str)
    check_change_and_replace_file(dst_op_file_path, tmp_op_file_path)


def generate_pyboost_op_header_code(header_data: FuncHeaderData):
    """ generate_pyboost_op_header_code """

    for tpl_path, gen_path in zip(header_data.op_header_template_path, header_data.code_generate_path):
        pyboost_op_str = tpl_path.replace(op_name=header_data.op_name_str,
                                          op_name_upper=header_data.op_name_str.upper(),
                                          operator_name=header_data.operator_name,
                                          call_args_with_type=header_data.call_args_with_type,
                                          return_type=header_data.cpp_func_return)
        op_header_dir_path = os.path.join(header_data.work_path, gen_path)
        pathlib.Path(op_header_dir_path).mkdir(parents=True, exist_ok=True)
        tmp_op_file_path = os.path.join(op_header_dir_path, "tmp_" + header_data.operator_name + ".h")
        dst_op_file_path = os.path.join(op_header_dir_path, header_data.operator_name + ".h")
        with open(tmp_op_file_path, "w") as f:
            f.write(pyboost_op_str)
        check_change_and_replace_file(dst_op_file_path, tmp_op_file_path)


class TemplatePaths:
    """
    template paths for code auto generation
    """

    def __init__(self, op_call_template_path, op_source_template_path, op_custom_template_path,
                 op_view_template_path, code_generate_path):
        self.op_call_template_path = op_call_template_path
        self.op_source_template_path = op_source_template_path
        self.op_custom_template_path = op_custom_template_path
        self.op_view_template_path = op_view_template_path
        self.code_generate_path = code_generate_path


def generate_pyboost_op_source_code(work_path, op_proto, template_paths, converter):
    """ generate_pyboost_op_source_code """
    # PyBoost source generate
    operator_name = converter.functional_name
    call_args_type = converter.call_args_types
    call_args_str = converter.call_args
    op_outputs = converter.op_outputs
    call_outputs = converter.call_func_outputs
    call_args_with_type = converter.call_args_with_types
    cpp_func_return = converter.cpp_func_return
    call_args_after_convert = converter.call_args_after_convert
    const_number_convert = converter.const_number_convert
    value_tuple_convert = converter.value_tuple_convert
    need_malloc_tensors = converter.need_malloc_tensors
    common_inputs = converter.common_inputs
    inplace_process = converter.inplace_process

    op_call_template_path = template_paths.op_call_template_path
    op_source_template_path = template_paths.op_source_template_path
    op_view_template_path = template_paths.op_view_template_path
    op_custom_template_path = template_paths.op_custom_template_path
    code_generate_path = template_paths.code_generate_path
    call_args_tensor = []
    for type, arg_name in zip(call_args_type, call_args_str):
        if type == "TensorPtr" or type == "std::optional<TensorPtr>":
            call_args_tensor.append(arg_name)

    for call_tpl, src_tpl, view_tpl, cus_tpl, gen_path in zip(op_call_template_path, op_source_template_path,
                                                              op_view_template_path, op_custom_template_path,
                                                              code_generate_path):
        malloc_inputs = ''
        is_ascend = 'ascend' in gen_path
        if is_ascend:
            for item in need_malloc_tensors:
                malloc_inputs += \
                    f'(void)runtime::DeviceAddressUtils::CreateInputAddress(device_context, {item}, "{item}");\n'
        else:
            malloc_inputs += f'std::vector<kernel::KernelTensor *> input_kernel_tensors;\n'
            for item in common_inputs:
                malloc_inputs += f'const auto &address_{item} = ' \
                                 f'runtime::DeviceAddressUtils::CreateInputAddress(device_context, {item}, "{item}");\n'
                malloc_inputs += f'(void)input_kernel_tensors.emplace_back(address_{item}->kernel_tensor().get());\n'

        # launch mode: cube or not
        # call_impl
        call_impl = ''
        customize_include = ''
        op_name_str = op_proto.class_name
        cube_math_type = ''
        get_cube_math_type = ''
        real_output = ', ' + op_outputs
        proto_operator_name = op_proto.operator_name
        if op_name_str.endswith('Ext'):
            op_name_str = op_name_str[:-3]
        if operator_name.endswith('ext'):
            operator_name = operator_name[:-4]
        if op_proto.is_view:
            call_impl = view_tpl.replace(op_name=op_proto.class_name,
                                         call_args=call_args_str,
                                         call_tensors=call_args_tensor,
                                         input=call_args_str[0])
            customize_include = "#include \"mindspore/core/ops/view/{}_strides_calc.h\"".format(proto_operator_name)
        elif is_ascend and op_proto.ascend != 'default':
            call_impl = cus_tpl.replace(call_args=call_args_str,
                                        return_values=call_outputs,
                                        customize_func=op_proto.ascend + "Customize",
                                        )
            customize_include = "#include \"plugin/device/ascend/kernel/pyboost/customize/{}.h\"".format(
                operator_name.lower())
        else:
            if is_ascend and is_cube(op_proto.class_name):
                get_cube_math_type = f'// cubeMathType: 0 - KEEP_DTYPE, 1 - ALLOW_FP32_DOWN_PRECISION\n'
                get_cube_math_type += "auto cube_math_type = GetCubeMathType();"
                cube_math_type = ', cube_math_type'
            aclnn_name = get_aclnn_interface(op_name_str)
            if inplace_process != '':
                real_output = ''

            call_impl = call_tpl.replace(aclnn_name=aclnn_name,
                                         call_args=call_args_str,
                                         call_tensors=call_args_tensor,
                                         value_tuple_convert=value_tuple_convert,
                                         const_number_convert=const_number_convert,
                                         malloc_inputs=malloc_inputs,
                                         get_cube_math_type=get_cube_math_type,
                                         cube_math_type=cube_math_type,
                                         aclnn_call_args=call_args_after_convert,
                                         return_values=call_outputs,
                                         outputs=real_output,
                                         inplace_process=inplace_process)

        pyboost_op_source_str = src_tpl.replace(op_name=op_name_str,
                                                operator_name=operator_name,
                                                call_args_with_type=call_args_with_type,
                                                return_type=cpp_func_return,
                                                customize_include=customize_include,
                                                call_impl=call_impl)
        op_header_dir_path = os.path.join(work_path, gen_path)
        tmp_op_source_file_path = os.path.join(op_header_dir_path, "tmp_" + operator_name.lower() + ".cc")
        dst_op_source_file_path = os.path.join(op_header_dir_path, operator_name.lower() + ".cc")
        with open(tmp_op_source_file_path, "w") as f:
            f.write(pyboost_op_source_str)
        check_change_and_replace_file(dst_op_source_file_path, tmp_op_source_file_path)


def generate_pyboost_op_register_source_code(work_path, all_ops, all_operator_names):
    """ generate_pyboost_op_register_source_code """
    include_str = ''
    factory_str = ''
    for op_name in all_ops:
        factory_str += "template class OpFactory<{0}>;\n".format(op_name)
    for operator_name in all_operator_names:
        include_str += "#include \"kernel/pyboost/auto_generate/{0}.h\"\n".format(operator_name)
    op_register_file_str = template.PYBOOST_OP_REGISTER_TEMPLATE.replace(op_includes=include_str,
                                                                         op_factory_templates=factory_str)
    op_register_dir_path = os.path.join(work_path, "mindspore/ccsrc/kernel/pyboost/auto_generate/")
    pathlib.Path(op_register_dir_path).mkdir(parents=True, exist_ok=True)
    tmp_op_register_file_path = os.path.join(op_register_dir_path, "tmp_" + "op_register.cc")
    dst_op_register_file_path = os.path.join(op_register_dir_path, "op_register.cc")
    with open(tmp_op_register_file_path, "w") as f:
        f.write(op_register_file_str)
    check_change_and_replace_file(dst_op_register_file_path, tmp_op_register_file_path)


def generate_pyboost_op_return_code(op_proto):
    """ generate_pyboost_op_return_code """
    returns_type = []
    for return_obj in op_proto.returns:
        returns_type.append(get_return_type(return_obj.arg_dtype))
    if len(returns_type) == 1:
        cpp_func_return = returns_type[0]
    elif not returns_type:
        raise Exception("No return")
    else:
        cpp_func_return = "std::tuple("
        cpp_func_return += ','.join(s for s in returns_type)
        cpp_func_return += ")"
    return returns_type, cpp_func_return


def generate_pyboost_op_func_return_type(op_proto):
    """ generate_pyboost_op_func_return_type """
    returns_type = []
    for return_obj in op_proto.returns:
        returns_type.append(get_return_type(return_obj.arg_dtype))
    if len(returns_type) == 1:
        cpp_func_return = returns_type[0]
    elif len(returns_type) > 1:
        cpp_func_return = "std::tuple<"
        cpp_func_return += ','.join(s for s in returns_type)
        cpp_func_return += ">"
    else:
        raise Exception("Not return found")
    return cpp_func_return


def generate_pyboost_outputs(op_proto):
    """ generate_pyboost_outputs """
    op_outputs = ''
    call_outputs = ''
    returns_type = []
    for return_obj in op_proto.returns:
        returns_type.append(get_return_type(return_obj.arg_dtype))

    if len(returns_type) == 1:
        if returns_type[0] == 'tensor::TensorPtr':
            op_outputs = 'outputs[0]'
            call_outputs = 'outputs_[0]'
        elif returns_type[0] == "std::vector<tensor::TensorPtr>":
            op_outputs = 'outputs'
            call_outputs = 'outputs_'
        else:
            raise Exception("Not support return type {}".format(returns_type[0]))
    elif len(returns_type) > 1:
        outputs_str = ''
        for i in range(len(returns_type)):
            outputs_str += 'outputs[{}],'.format(i)
        op_outputs = outputs_str[:-1]

        outputs_str = ''
        for i in range(len(returns_type)):
            outputs_str += 'outputs_[{}],'.format(i)
        outputs_str = outputs_str[:-1]
        call_outputs = "std::make_tuple(" + outputs_str + ")"

    return op_outputs, call_outputs


def generate_ops_header_files(work_path, yaml_data):
    """
    :param work_path:
    :param yaml_data:
    :return: void
    """
    extern_str = ''
    extern_template = CppTemplate("MS_EXPORT extern OpDef g${op_name};\n")
    for operator_name, operator_data in yaml_data.items():
        op_proto = OpProto.load_from_yaml(operator_name, operator_data)
        extern_str += extern_template.replace(op_name=op_proto.class_name)
    ops_header_file = template.GEN_OPS_DEF_HEADER_TEMPLATE.replace(extern_variable=extern_str)
    dir_path = os.path.join(work_path, "mindspore/core/ops/auto_generate")
    pathlib.Path(dir_path).mkdir(parents=True, exist_ok=True)
    dst_file_path = os.path.join(dir_path, "gen_ops_def.h")
    tmp_file_path = os.path.join(dir_path, "tmp_gen_ops_def.h")
    with open(tmp_file_path, "w") as f:
        f.write(ops_header_file)
    check_change_and_replace_file(dst_file_path, tmp_file_path)


def generate_parser_func(op_proto: OpProto) -> str:
    """
    Generate parser func
    :param op_proto:
    :return: str
    """
    convert_template = CppTemplate("auto $arg_name = converter.${convert_func}($arg_index);\n")
    parser_func_str = ''
    for index, arg in enumerate(op_proto.op_args):
        is_optional = is_optional_param(arg)
        convert_type_str = get_convert_type_str(arg.arg_dtype, is_optional)
        parser_func_str += convert_template.replace(arg_name=arg.arg_name, convert_func=convert_type_str,
                                                    arg_index=pyboost_utils.get_index(index))
    return parser_func_str


def generate_pyboost_functions(work_path, yaml_data):
    """
    Generate pyboost functions file from yaml.
    """
    pyboost_func_str = ''
    pyboost_func_pybind_def = ''
    pyboost_func_include_headers_str = ''
    pyboost_func_include_header_template = CppTemplate("#include \"kernel/pyboost/auto_generate/${operator_name}.h\"\n")
    for operator_name, operator_data in yaml_data.items():
        op_proto = OpProto.load_from_yaml(operator_name, operator_data)
        if not op_proto.is_pyboost:
            continue
        op_def_name_str = f"g{op_proto.class_name}"
        prim_name_str = op_proto.class_name
        operator_name = op_proto.operator_name
        if operator_name.endswith('ext'):
            operator_name = operator_name[:-4]
        op_name_str = prim_name_str
        if prim_name_str.endswith('Ext'):
            op_name_str = prim_name_str[:-3]
        op_args_str = [op_arg.arg_name for op_arg in op_proto.op_args]
        parser_body_str = generate_parser_func(op_proto)

        convert_to_tensor_template = CppTemplate(
            "auto ${output} = PyNativeAlgo::Common::StubNodeToTensor(${input});\n")
        convert_to_tensor_optional_template = CppTemplate(
            "auto ${output} = PyNativeAlgo::Common::StubNodeToTensorOptional(${input});\n")
        convert_to_tensor_list_template = CppTemplate(
            "auto ${output} = PyNativeAlgo::Common::StubNodeToValueTuple(${input});\n")

        grad_args_str = []
        call_args_str = []
        cast_args_str = []
        convert_stub_str = ''
        optional_to_value_str = ''
        for op_arg in op_proto.op_args:
            grad_arg = ''
            call_arg = ''
            cast_arg = ''
            cast_str = 'cast_'
            if pyboost_utils.is_tensor(op_arg):
                if op_arg.as_init_arg and str(op_arg.default) == 'None':
                    convert_stub_output_name = op_arg.arg_name + '_optional'
                    convert_stub_str += convert_to_tensor_optional_template.replace(output=convert_stub_output_name,
                                                                                    input=op_arg.arg_name)
                    cast_output = cast_str + convert_stub_output_name
                    convert_optional_to_value_template = CppTemplate(
                        "auto ${output} = PyNativeAlgo::PyBoost::OptionalToValue(${input});\n")
                    convert_optional_to_value_name = op_arg.arg_name + "_value"
                    optional_to_value_str += \
                        convert_optional_to_value_template.replace(input=cast_output,
                                                                   output=convert_optional_to_value_name)
                    call_arg = convert_stub_output_name
                    grad_arg = convert_optional_to_value_name
                    cast_arg = cast_output
                else:
                    convert_stub_output_name = op_arg.arg_name + "_tensor"
                    convert_stub_str += convert_to_tensor_template.replace(input=op_arg.arg_name,
                                                                           output=convert_stub_output_name)
                    call_arg = convert_stub_output_name
                    grad_arg = cast_str + convert_stub_output_name
                    cast_arg = grad_arg
            elif pyboost_utils.is_tensor_list(op_arg):
                convert_stub_output_name = op_arg.arg_name + "_tensor_list"
                convert_stub_str += convert_to_tensor_list_template.replace(input=op_arg.arg_name,
                                                                            output=convert_stub_output_name)
                call_arg = convert_stub_output_name
                grad_arg = cast_str + convert_stub_output_name
                cast_arg = grad_arg
            else:
                call_arg = op_arg.arg_name
                grad_arg = cast_str + op_arg.arg_name
                cast_arg = grad_arg
            grad_args_str.append(grad_arg)
            call_args_str.append(call_arg)
            cast_args_str.append(cast_arg)
        pyboost_func_str += template.PYBOOST_FUNCTION_TEMPLATE.replace(func_name=op_proto.pyboost_function_name,
                                                                       op_def_name=op_def_name_str,
                                                                       parser_body=parser_body_str, op_name=op_name_str,
                                                                       convert_stub=convert_stub_str,
                                                                       optional_to_value=optional_to_value_str,
                                                                       call_args=call_args_str,
                                                                       grad_args=grad_args_str,
                                                                       cast_args=cast_args_str,
                                                                       op_args=op_args_str)
        pyboost_func_str = pyboost_func_str + template.NEW_LINE + template.NEW_LINE
        pyboost_func_pybind_def += template.REGISTER_DEFINE_TEMPLATE.replace(
            pyboost_op_name=get_pyboost_name(op_proto.operator_name),
            pyboost_cfunc_name=op_proto.pyboost_function_name)
        pyboost_func_include_headers_str += pyboost_func_include_header_template.replace(operator_name=operator_name)
    register_func_str = template.REGISTER_TEMPLATE.replace(register_func=pyboost_func_pybind_def)

    pyboost_func_file = template.PYBOOST_HEADER_TEMPLATE.replace(include_op_header=pyboost_func_include_headers_str,
                                                                 function_body=pyboost_func_str,
                                                                 register_function_body=register_func_str)
    dir_path = os.path.join(work_path, "mindspore/ccsrc/pipeline/pynative/op_function/auto_generate")
    pathlib.Path(dir_path).mkdir(parents=True, exist_ok=True)
    tmp_file_path = os.path.join(dir_path, "tmp_pyboost_functions.cc")
    dst_file_path = os.path.join(dir_path, "pyboost_functions.cc")
    with open(tmp_file_path, "w") as f:
        f.write(pyboost_func_file)
    check_change_and_replace_file(dst_file_path, tmp_file_path)


def generate_inplace_process_cpp_code(op_proto):
    """ generate_ref_process_cpp_code """
    inplace_process = f'// RefOps update output by input tensor\n'
    has_ref = False
    for index, return_obj in enumerate(op_proto.returns):
        if return_obj.inplace != '':
            inplace_process += f'op->device_sync_promises()[{index}]->SetValue(' \
                               f'std::make_shared<pynative::DeviceAddressFutureData>(' \
                               f'{return_obj.inplace}_tensor->device_address(), nullptr)); '
            has_ref = True
            break
    if has_ref:
        return inplace_process
    return ''


class OpTemplateConverter:
    """
    template converter
    """

    def __init__(self, op_proto):
        self.op_proto = op_proto
        self.op_name = self.parse_op_name(op_proto.class_name)
        self.functional_name = self.parse_functional_name(op_proto.operator_name)
        self.call_args = self.parse_original_call_args(op_proto.op_args)
        self.call_args_types = self.parse_call_args_types(op_proto.op_args)
        self.call_args_with_types = self.parse_call_args_with_types(self.call_args, self.call_args_types)
        self.need_malloc_tensors = self.parse_need_malloc_tensors(op_proto.op_args, self.call_args)
        self.call_args_after_convert, self.value_tuple_convert, self.const_number_convert = \
            self.op_args_converter(op_proto.op_args, self.call_args)
        self.common_inputs = self.parse_common_inputs(self.call_args_after_convert)
        self.cpp_func_return = generate_pyboost_op_func_return_type(op_proto)
        self.op_outputs, self.call_func_outputs = generate_pyboost_outputs(op_proto)
        self.inplace_process = generate_inplace_process_cpp_code(op_proto)

    @staticmethod
    def parse_common_inputs(call_args):
        """
        :param call_args:
        :return: all args after convert
        """
        common_inputs = []
        for call_arg in call_args:
            common_inputs.append(call_arg)
        return common_inputs

    @staticmethod
    def parse_call_args_types(op_args):
        """
        :param op_args:
        :return: call_args_types
        """
        call_args_types = []
        for op_arg in op_args:
            is_optional = is_optional_param(op_arg)
            call_args_types.append(get_input_dtype(op_arg.arg_dtype, is_optional))
        return call_args_types

    @staticmethod
    def parse_call_args_with_types(call_args, call_args_types):
        """
        :param call_args:
        :param call_args_types:
        :return: call_args_with_types
        """
        call_args_with_types = []
        for type_name, arg_name in zip(call_args_types, call_args):
            call_args_with_types.append("const " + type_name + " &" + arg_name)
        return call_args_with_types

    @staticmethod
    def parse_functional_name(name):
        """
        :param name:
        :return: functional_name
        """
        functional_name = name
        if functional_name.endswith('ext'):
            functional_name = functional_name[:-4]
        return functional_name

    @staticmethod
    def parse_need_malloc_tensors(op_args, call_args):
        """
        :param op_args:
        :param call_args:
        :return: need_malloc_tensors
        """
        need_malloc_tensors = []
        for op_arg, call_arg in zip(op_args, call_args):
            if pyboost_utils.is_tensor(op_arg):
                call_arg = op_arg.arg_name + "_tensor"
                need_malloc_tensors.append(call_arg)
            if tuple_input_to_cpp_type(op_arg.arg_dtype) and pyboost_utils.is_tensor_list(op_arg):
                need_malloc_tensors.append(call_arg + "_vector")
        return need_malloc_tensors

    @staticmethod
    def parse_op_name(name):
        """
        :param name:
        :return: op_name
        """
        op_name = name
        if op_name.endswith('Ext'):
            op_name = op_name[:-3]
        return op_name

    @staticmethod
    def parse_original_call_args(op_args):
        """
        :param op_args:
        :return: call_args
        """
        call_args = []
        for op_arg in op_args:
            if pyboost_utils.is_tensor(op_arg):
                call_arg = op_arg.arg_name + "_tensor"
            elif pyboost_utils.is_tensor_list(op_arg):
                call_arg = op_arg.arg_name + "_tensor_list"
            else:
                call_arg = op_arg.arg_name
            call_args.append(call_arg)
        return call_args

    @staticmethod
    def op_args_converter(op_args, call_args):
        """Convert ValutePtr to cpp data type"""
        call_args_after_convert = []
        value_tuple_convert = []
        const_number_convert = []
        for op_arg, call_arg in zip(op_args, call_args):
            if number_input_to_cpp_type(op_arg.arg_dtype):
                call_args_after_convert.append(call_arg + "_imm")
                const_number_convert.append(get_const_number_convert(call_arg, op_arg.arg_dtype))
            elif tuple_input_to_cpp_type(op_arg.arg_dtype):
                call_args_after_convert.append(call_arg + "_vector")
                value_tuple_convert.append(get_tuple_input_convert(call_arg, op_arg.arg_dtype))
            else:
                call_args_after_convert.append(call_arg)
        return call_args_after_convert, value_tuple_convert, const_number_convert


def generate_pyboost_op_cpp_code(work_path, yaml_data):
    """
    Generate pyboost op cpp code from yaml.
    """
    op_header_template_path = [template.PYBOOST_ASCEND_OP_HEADER_TEMPLATE, template.PYBOOST_GPU_OP_HEADER_TEMPLATE,
                               template.PYBOOST_CPU_OP_HEADER_TEMPLATE]
    op_call_template_path = [template.PYBOOST_ASCEND_CALL_TEMPLATE, template.PYBOOST_GPU_CALL_TEMPLATE,
                             template.PYBOOST_CPU_CALL_TEMPLATE]
    op_source_template_path = [template.PYBOOST_ASCEND_OP_SOURCE_TEMPLATE, template.PYBOOST_GPU_OP_SOURCE_TEMPLATE,
                               template.PYBOOST_CPU_OP_SOURCE_TEMPLATE]
    op_custom_template_path = [template.PYBOOST_ASCEND_CUSTOMIZE_CALL_TEMPLATE,
                               template.PYBOOST_GPU_CUSTOMIZE_CALL_TEMPLATE,
                               template.PYBOOST_CPU_CUSTOMIZE_CALL_TEMPLATE]
    op_view_template_path = [template.PYBOOST_ASCEND_VIEW_CALL_TEMPLATE, template.PYBOOST_GPU_VIEW_CALL_TEMPLATE,
                             template.PYBOOST_CPU_VIEW_CALL_TEMPLATE]
    code_generate_path = ["mindspore/ccsrc/plugin/device/ascend/kernel/pyboost/auto_generate/",
                          "mindspore/ccsrc/plugin/device/gpu/kernel/pyboost/auto_generate/",
                          "mindspore/ccsrc/plugin/device/cpu/kernel/pyboost/auto_generate/"]

    all_op_names = []
    all_functional_names = []
    for operator_name, operator_data in yaml_data.items():
        op_proto = OpProto.load_from_yaml(operator_name, operator_data)
        if not op_proto.is_pyboost:
            continue
        converter = OpTemplateConverter(op_proto)
        functional_name = converter.functional_name

        op_name_str = converter.op_name

        all_op_names.append(op_name_str)
        all_functional_names.append(functional_name)

        call_args_with_types = converter.call_args_with_types
        cpp_func_return = converter.cpp_func_return

        generate_pyboost_base_op_header_code(work_path, op_name_str, functional_name, call_args_with_types,
                                             cpp_func_return)
        header_data = FuncHeaderData(work_path, op_header_template_path, code_generate_path, op_name_str,
                                     functional_name, call_args_with_types, cpp_func_return)
        generate_pyboost_op_header_code(header_data)
        template_paths = TemplatePaths(op_call_template_path, op_source_template_path, op_custom_template_path,
                                       op_view_template_path, code_generate_path)
        generate_pyboost_op_source_code(work_path, op_proto, template_paths, converter)
    generate_pyboost_op_register_source_code(work_path, all_op_names, all_functional_names)


def gen_pyboost_py_func(work_path, op_yaml_data, doc_data):
    """ gen_pyboost_py_func """
    gen_py = ''
    gen_py += py_licence_str
    op_desc_dict = {}
    for operator_name, operator_desc in doc_data.items():
        desc = operator_desc.get("description")
        op_desc_dict[operator_name] = desc
    gen_py += template.PYBOOST_PY_FUNC_HEADEAR
    func_def = None
    for operator_name, operator_data in op_yaml_data.items():
        op_proto = OpProto.load_from_yaml(operator_name, operator_data)
        if not op_proto.is_pyboost:
            continue
        func_def = operator_data.get('function')
        func_name = operator_name
        if func_def is not None:
            func_disable = get_disable_flag(func_def)
            if func_disable:
                continue
            item = func_def.get("name")
            if item is not None:
                func_name = item
        if func_name.endswith("_ext"):
            func_name = func_name[:-4]

        description = op_desc_dict.get(operator_name)
        args = operator_data.get('args')
        class_name = get_op_name(operator_name, operator_data.get('class'))
        func_args = []
        init_args = []
        input_args = []
        for arg_name, arg_info in args.items():
            init_value = arg_info.get('init')

            if init_value is None:
                default_key = 'default'
                default_value = arg_info.get(default_key)
                default_value = '=' + str(default_value) if default_key in arg_info else ''
                func_args.append(arg_name + default_value)
                input_args.append(arg_name)
            else:
                if init_value == 'NO_VALUE':
                    func_args.append(f"""{arg_name}""")
                    init_args.append(arg_name)
                else:
                    func_args.append(f"""{arg_name}={init_value}""")
                    init_args.append(arg_name)
        gen_py += template.PYBOOST_PY_FUNC_TEMPLATE.replace(func_name=func_name, description=description,
                                                            func_args=func_args,
                                                            init_args=init_args,
                                                            operator_name=operator_name,
                                                            class_name=class_name, input_args=input_args)
    dir_path = os.path.join(work_path, "mindspore/python/mindspore/ops/auto_generate")
    pathlib.Path(dir_path).mkdir(parents=True, exist_ok=True)
    dst_file_path = os.path.join(dir_path, "gen_pyboost_func.py")
    tmp_file_path = os.path.join(dir_path, "tmp_gen_pyboost_func.py")
    with open(tmp_file_path, "w") as f:
        f.write(gen_py)
    check_change_and_replace_file(dst_file_path, tmp_file_path)


def gen_pyboost_code(work_path, ops_yaml_data, doc_yaml_data):
    """ gen_pyboost_code """
    # generate pyboost py func
    gen_pyboost_py_func(work_path, ops_yaml_data, doc_yaml_data)
    # generate ops header file
    generate_ops_header_files(work_path, ops_yaml_data)
    # generate pyboost functions
    generate_pyboost_functions(work_path, ops_yaml_data)
    # generate pyboost backend cpp code
    generate_pyboost_op_cpp_code(work_path, ops_yaml_data)

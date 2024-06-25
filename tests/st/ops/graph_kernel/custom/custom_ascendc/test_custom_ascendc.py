# Copyright 2024 Huawei Technologies Co., Ltd
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

import pytest
import os
import shutil
import tempfile
import subprocess
from compile_utils import compile_custom_run


class TestCase():
    def setup(self):
        self.temp_dir = tempfile.mkdtemp()
        compile_custom_run(self.temp_dir)
        self.cus_path = ""
        for root, dirs, _ in os.walk(os.path.join(self.temp_dir, "custom_compiler", 'CustomProject', 'build_out')):
            if "customize" in dirs:
                self.cus_path = os.path.join(root, "customize")
                break
        if self.cus_path == "":
            assert False

        custom_name = "ASCEND_CUSTOM_OPP_PATH"
        if custom_name in os.environ:
            current_value = os.environ[custom_name]
            if current_value:
                os.environ[custom_name] = f"{self.cus_path}:{current_value}"
            else:
                os.environ[custom_name] = self.cus_path
        else:
            os.environ[custom_name] = self.cus_path

    def teardown(self):
        shutil.rmtree(self.temp_dir)

    @pytest.mark.level1
    @pytest.mark.env_onecard
    @pytest.mark.platform_arm_ascend_training
    def test_custom_add_aclop(self):
        """
        Feature: Custom op testcase
        Description: test case for AddCustom op with func_type="aclop"
        Expectation: the result match with numpy result
        """
        command = ['pytest -sv test_custom_aclop.py::test_custom_add_aclop']
        result = subprocess.run(command, shell=True, stderr=subprocess.STDOUT)
        assert result.returncode == 0

    @pytest.mark.level1
    @pytest.mark.env_onecard
    @pytest.mark.platform_arm_ascend_training
    def test_custom_add_aclop_dynamic(self):
        """
        Feature: Custom op testcase
        Description: test case for AddCustom op in dynamic shape
        Expectation: the result match with numpy result
        """
        command = ['pytest -sv test_custom_aclop.py::test_custom_add_aclop_dynamic']
        result = subprocess.run(command, shell=True, stderr=subprocess.STDOUT)
        assert result.returncode == 0

    @pytest.mark.level1
    @pytest.mark.env_onecard
    @pytest.mark.platform_arm_ascend_training
    def test_custom_add_aclop_graph(self):
        """
        Feature: Custom op testcase
        Description: test case for AddCustom op with func_type="aclop"  in graph mode
        Expectation: the result match with numpy result
        """
        command = ['pytest -sv test_custom_aclop.py::test_custom_add_aclop_graph']
        result = subprocess.run(command, shell=True, stderr=subprocess.STDOUT)
        assert result.returncode == 0

    @pytest.mark.level1
    @pytest.mark.env_onecard
    @pytest.mark.platform_arm_ascend_training
    def test_custom_add_aclnn(self):
        """
        Feature: Custom op testcase
        Description: test case for aclnnAddCustom op with func_type="aclnn"
        Expectation: the result match with numpy result
        """
        command = ['pytest -sv test_custom_aclnn.py::test_custom_add_aclnn']
        result = subprocess.run(command, shell=True, stderr=subprocess.STDOUT)
        assert result.returncode == 0

    @pytest.mark.level1
    @pytest.mark.env_onecard
    @pytest.mark.platform_arm_ascend_training
    def test_custom_add_aclnn_dynamic(self):
        """
        Feature: Custom op testcase
        Description: test case for aclnnAddCustom op in Dynamic Shape
        Expectation: the result match with numpy result
        """
        command = ['pytest -sv test_custom_aclnn.py::test_custom_add_aclnn_dynamic']
        result = subprocess.run(command, shell=True, stderr=subprocess.STDOUT)
        assert result.returncode == 0

    @pytest.mark.level1
    @pytest.mark.env_onecard
    @pytest.mark.platform_arm_ascend_training
    def test_custom_add_aclnn_cpp_infer(self):
        """
        Feature: Custom op testcase
        Description: test case for aclnnAddCustom op with func_type="aclnn", infer shape by cpp.
        Expectation: the result match with numpy result
        """
        command = ['pytest -sv test_custom_aclnn.py::test_custom_add_aclnn_cpp_infer']
        result = subprocess.run(command, shell=True, stderr=subprocess.STDOUT)
        assert result.returncode == 0

    @pytest.mark.level1
    @pytest.mark.env_onecard
    @pytest.mark.platform_arm_ascend_training
    def test_custom_add_aclnn_bprop(self):
        """
        Feature: Custom op testcase
        Description: test case for aclnnAddCustom backpropagation.
        Expectation: the result match with numpy result
        """
        command = ['pytest -sv test_custom_aclnn.py::test_custom_add_aclnn_bprop']
        result = subprocess.run(command, shell=True, stderr=subprocess.STDOUT)
        assert result.returncode == 0

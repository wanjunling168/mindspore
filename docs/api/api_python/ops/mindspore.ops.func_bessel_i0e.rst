mindspore.ops.bessel_i0e
========================

.. py:function:: mindspore.ops.bessel_i0e(x)

    逐元素计算指数缩放第一类零阶修正贝塞尔函数。

    计算公式定义如下：

    .. math::
        \begin{array}{ll} \\
            \text I_{0}e(x)=e^{(-|x|)} * I_{0}(x)=e^{(-|x|)} * \sum_{m=0}^
            {\infty} \frac{x^{2 m}}{2^{2 m} (m !)^{2}}
        \end{array}

    其中 :math:`I_{0}` 是第一类零阶修正Bessel函数。

    参数：
        - **x** (Tensor) - Tensor的输入。数据类型应为float16，float32或float64。

    返回：
        Tensor，shape和数据类型与 `x` 相同。

    异常：
        - **TypeError** - `x` 不是Tensor。
        - **TypeError** - `x` 的数据类型不是float16，float32或float64。

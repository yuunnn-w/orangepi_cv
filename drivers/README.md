## RKNPU driver 0.9.8 version
## RKNPU 驱动 0.9.8 版本

The official driver provided has issues and needs the following modifications:

官方提供的驱动存在问题，需要进行如下修改：

1. Add the following code to the `include/rknpu_mm.h` header file:
1. 将以下代码添加到 `include/rknpu_mm.h` 头文件中：

```c
static inline void vm_flags_set(struct vm_area_struct *vma, vm_flags_t flags)
{
	vma->vm_flags |= flags;
}

static inline void vm_flags_clear(struct vm_area_struct *vma, vm_flags_t flags)
{
	vma->vm_flags &= ~flags;
}
```

2. Modify the `rknpu_devfreq.c` file, comment out the code on line 237:
2. 修改 `rknpu_devfreq.c` 文件，将第 237 行的代码注释掉：

```c
// .set_soc_info = rockchip_opp_set_low_length,
```

Here is the modified code. If you need to install this driver, please recompile the kernel using [orangepi-build](https://github.com/orangepi-xunlong/orangepi-build), replace all the code in this directory to the `kernel/orange-pi-6.1-rk35xx/drivers` directory, delete the previously compiled .o files, and then recompile.

这里给出的是修改后的代码。如需安装该驱动，请使用 [orangepi-build](https://github.com/orangepi-xunlong/orangepi-build) 重新编译内核，将该目录的所有代码替换至 `kernel/orange-pi-6.1-rk35xx/drivers` 目录下，删除原先编译好的 .o 文件，然后重新编译即可。
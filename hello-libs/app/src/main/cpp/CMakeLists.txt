# CMake的最低版本需求
cmake_minimum_required(VERSION 3.4.1)

# 配置三方库
# 设置三方库的路径，CMAKE_SOURCE_DIR表示当前的目录
set(distribution_DIR) ${CMAKE_SOURCE_DIR}/../../../../distribution)

# 添加静态库支持
# 原型为：add_library(<name> <SHARED|STATIC|MODULE|UNKNOWN> IMPORTED)
# 导入库目标是引用了在工程外的一个库文件的目标，没有生成构件这个库的规则。
add_library(lib_gmath
			STATIC
			IMPORTED)

# 设置目标属性来改变它们的构建方式
# 原型：  set_target_properties(target1 target2 ...
#                             PROPERTIES prop1 value1
#                             prop2 value2 ...)
# 为一个目标设置属性。该命令的语法是列出所有你想要变更的文件，然后提供你想要设置的值。
# 你能够使用任何你想要的属性/值对，并且在随后的代码中调用GET_TARGET_PROPERTY命令取出属性的值。
set_target_properties(lib_gmath
					  PROPERTIES
					  IMPORTED_LOCATION
					  ${distribution_DIR}/gmath/lib/${ANDROID_ABI}/libgmath.a)

# 共享库也会被打包到apk中，根据app/build.gradle中的配置被发送到目标位置
# jniLibs 节就是为了这个目的。
# jniLibs 节就是为了这个目的。
add_library(lib_gperf
			SHARED
			IMPORTED)
set_target_properties(lib_gperf
					  PROPERTIES
					  IMPORTED_LOCATION
					  ${distribution_DIR}/gperf/lib/${ANDROID_ABI}/libgperf.so);